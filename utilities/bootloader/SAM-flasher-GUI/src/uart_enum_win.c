/*
 *  Copyright 2016 Frank Hunleth
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * WARNING: This file has been modified from the original source for use
 *          in the SAM Flasher utility.
 */

#ifdef __WIN32__

#include <initguid.h> // Fixes linker errors with GUID_* identifiers. Must come first.
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <ntddmodm.h>
#include <Cfgmgr32.h>
#include <stdio.h>
#include <stdbool.h>

#include "uart_enum.h"

static char *device_registry_property(HDEVINFO device_info_set,
                                    PSP_DEVINFO_DATA device_info_data,
                                    DWORD property)
{
    DWORD data_type = 0;
    char *buffer = malloc(MAX_PATH + 1);
    DWORD bytes_required = MAX_PATH;
    for (;;) {
        if (SetupDiGetDeviceRegistryProperty(device_info_set, device_info_data, property, &data_type,
                                             (PBYTE) buffer,
                                             bytes_required, &bytes_required)) {
            break;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER
                || (data_type != REG_SZ && data_type != REG_EXPAND_SZ)) {
            return NULL;
        }
        buffer = realloc(buffer, bytes_required + 2);
    }
    return buffer;
}

static char *strtoupper(char *str)
{
    for (char *s = str; *s; s++)
        *s = toupper(*s);

    return str;
}

static char *device_instance_id(DWORD device_instance_number)
{
    char buffer[MAX_DEVICE_ID_LEN + 1];
    if (CM_Get_Device_ID(
                device_instance_number,
                buffer,
                MAX_DEVICE_ID_LEN,
                0) != CR_SUCCESS) {
        return NULL;
    }
    return strtoupper(buffer);
}

static int parse_device_id(const char *instance_id,
                                 const char *identifier_prefix,
                                 int identifier_size)
{
    const char *loc = strstr(instance_id, identifier_prefix);
    if (!loc)
        return 0;

    char buffer[identifier_size + 1];
    memcpy(buffer, loc + strlen(identifier_prefix), identifier_size);
    return strtol(buffer, NULL, 16);
}

static int device_vid(const char *instance_id)
{
    static const int vid_len = 4;
    int result = parse_device_id(
                instance_id, "VID_", vid_len);
    if (!result)
        result = parse_device_id(
                    instance_id, "VEN_", vid_len);
    return result;
}

static int device_pid(const char *instance_id)
{
    static const int pid_len = 4;
    int result = parse_device_id(
                instance_id, "PID_", pid_len);
    if (!result)
        result = parse_device_id(
                    instance_id, "DEV_", pid_len);
    return result;
}


static char *device_port_name(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
{
    const HKEY key = SetupDiOpenDevRegKey(deviceInfoSet, deviceInfoData, DICS_FLAG_GLOBAL,
                                          0, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE)
        return NULL;

    static const char * const keyTokens[] = {
        "PortName\0",
        "PortNumber\0"
    };

    static const int keyTokensCount = sizeof(keyTokens) / sizeof(keyTokens[0]);

    char *name = NULL;
    char *buffer = malloc((MAX_PATH + 1));
    DWORD bytes_required = MAX_PATH;

    for (int i = 0; i < keyTokensCount; ++i) {
        DWORD data_type = 0;

        for (;;) {
            const LONG rc = RegQueryValueEx(key, keyTokens[i], NULL, &data_type,
                                             (PBYTE) buffer, &bytes_required);
            if (rc == ERROR_MORE_DATA) {
                buffer = realloc(buffer, bytes_required + 2);
                continue;
            } else if (rc == ERROR_SUCCESS) {
                if (data_type == REG_SZ) {
                    name = strdup(buffer);
                } else if (data_type == REG_DWORD) {
                    name = malloc(16);
                    sprintf(name, "COM%d", (int) *((PDWORD) buffer));
                }
            }
            break;
        }

        if (name)
            break;
    }

    free(buffer);
    RegCloseKey(key);

    return name;
}

static bool uart_exists(const struct serial_info *info, const char *port_name)
{
    for (const struct serial_info *i = info; i != NULL; i = i->next) {
        if (strcmp(i->name, port_name) == 0) {
            return true;
        }
    }
    return false;
}

struct serial_info *find_serialports()
{
    // Thanks to QtSerialPort for the list of classes and interfaces to scan. It was
    // much, much more helpful than the docs I found.
    struct setup_args {
        GUID guid;
        DWORD flags;
    } setup_tokens[4];
    static const int setup_token_count = 4;
    setup_tokens[0].guid = GUID_DEVCLASS_PORTS;
    setup_tokens[0].flags = DIGCF_PRESENT;
    setup_tokens[1].guid = GUID_DEVCLASS_MODEM;
    setup_tokens[1].flags = DIGCF_PRESENT;
    setup_tokens[2].guid = GUID_DEVINTERFACE_COMPORT;
    setup_tokens[2].flags = DIGCF_PRESENT | DIGCF_DEVICEINTERFACE;
    setup_tokens[3].guid = GUID_DEVINTERFACE_MODEM;
    setup_tokens[3].flags = DIGCF_PRESENT | DIGCF_DEVICEINTERFACE;
    char test[128];

    struct serial_info *info = NULL;

    for (int i = 0; i < setup_token_count; ++i) {
        const HDEVINFO device_info_set = SetupDiGetClassDevs(&setup_tokens[i].guid, NULL, NULL, setup_tokens[i].flags);
        if (device_info_set == INVALID_HANDLE_VALUE) {
            return info;
        }

        SP_DEVINFO_DATA device_info_data;
        memset(&device_info_data, 0, sizeof(device_info_data));
        device_info_data.cbSize = sizeof(device_info_data);

        DWORD index = 0;
        while (SetupDiEnumDeviceInfo(device_info_set, index++, &device_info_data)) {

            char *port_name = device_port_name(device_info_set, &device_info_data);

            if (!port_name)
                continue;

            if (strncmp("LPT", port_name, 3) == 0) {
                free(port_name);
                continue;
            }

            // SetupDiEnumDeviceInfo reports devices multiple times, so
            // prune out duplicates
            if (uart_exists(info, port_name)) {
                free(port_name);
                continue;
            }

            struct serial_info *new_info = serial_info_alloc();
            new_info->name = port_name;
            new_info->description = device_registry_property(device_info_set, &device_info_data, SPDRP_DEVICEDESC);
            new_info->manufacturer = device_registry_property(device_info_set, &device_info_data, SPDRP_MFG);
            new_info->serial_number = NULL; // TODO
            new_info->vid = 0;
            new_info->pid = 0;

            char *instance_id = device_instance_id(device_info_data.DevInst);
            if (instance_id) {
                new_info->vid = device_vid(instance_id);
                new_info->pid = device_pid(instance_id);
            }
            new_info->next = info;
            info = new_info;
        }
        SetupDiDestroyDeviceInfoList(device_info_set);
    }

    return info;
}
#endif
