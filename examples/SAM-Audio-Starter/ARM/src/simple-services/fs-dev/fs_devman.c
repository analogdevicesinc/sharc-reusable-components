/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "fs_devman_cfg.h"
#include "fs_devman_priv.h"
#include "fs_devman.h"

typedef struct _FS_DEVMAN_DEVICE_ENTRY {
    bool allocated;
    bool isDefault;
    FS_DEVMAN_DEVICE_INFO info;
} FS_DEVMAN_DEVICE_ENTRY;

static FS_DEVMAN_DEVICE_ENTRY deviceEntries[FS_DEVMAN_MAX_DEVICES];
static unsigned numDevices;

FS_DEVMAN_RESULT fs_devman_init(void)
{
    memset(deviceEntries, 0, sizeof(deviceEntries));
    numDevices = 0;
    return(FS_DEVMAN_OK);
}

FS_DEVMAN_RESULT fs_devman_register(const char *name,
    const FS_DEVMAN_DEVICE *dev, void *usr)
{
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    FS_DEVMAN_RESULT result = FS_DEVMAN_NO_ENTRIES;
    unsigned i;

    if (name == NULL) {
        return(FS_DEVMAN_INVALID_NAME);
    }

    i = strlen(name);
    if ((i < 2) || (name[i-1] != ':')) {
        return(FS_DEVMAN_INVALID_NAME);
    }

    for (i = 0; i < FS_DEVMAN_MAX_DEVICES; i++) {
        if (deviceEntries[i].allocated == false) {
            entry = &deviceEntries[i];
            break;
        }
    }

    if (entry) {
        entry->allocated = true;
        entry->info.name = name;
        entry->info.dev = dev;
        entry->info.usr = usr;
        if (numDevices == 0) {
            entry->isDefault = true;
        }
        numDevices++;
        result = FS_DEVMAN_OK;
    }

    return(result);
}

static FS_DEVMAN_DEVICE_ENTRY *fs_devman_find(
    const char *name, const char **fname, bool defaultOk)
{
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    unsigned colonIdx;
    int i;

    if (name == NULL) {
        return(NULL);
    }

    /* Find a matching device */
    for (i = 0; i < FS_DEVMAN_MAX_DEVICES; i++) {
        entry = &deviceEntries[i];
        if (entry->allocated) {
            colonIdx = strlen(entry->info.name) - 1;
            if ( (strncmp(name, entry->info.name, colonIdx) == 0) &&
                 (entry->info.name[colonIdx] == ':') ) {
                if (fname) {
                    *fname = &name[colonIdx + 1];
                }
                return(entry);
            }
        }
    }

    /* Search for the default device if no match */
    if (defaultOk) {
        /* Must not have a volume prefix at all */
        if (strchr(name, ':') != NULL) {
            return(NULL);
        }
        /* Search for the default volume */
        for (i = 0; i < FS_DEVMAN_MAX_DEVICES; i++) {
            entry = &deviceEntries[i];
            if (entry->allocated && entry->isDefault) {
                if (fname) {
                    *fname = name;
                }
                return(entry);
            }
        }
    }

    return(NULL);
}

FS_DEVMAN_RESULT fs_devman_unregister(const char *name)
{
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    FS_DEVMAN_RESULT result = FS_DEVMAN_NOT_FOUND;

    entry = fs_devman_find(name, NULL, false);
    if (entry) {
        memset(entry, 0, sizeof(*entry));
        numDevices--;
        result = FS_DEVMAN_OK;
    }

    return(result);
}

FS_DEVMAN_RESULT fs_devman_set_default(const char *name)
{
    FS_DEVMAN_RESULT result = FS_DEVMAN_NOT_FOUND;
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    int i;

    entry = fs_devman_find(name, NULL, false);
    if (entry) {
        for (i = 0; i < FS_DEVMAN_MAX_DEVICES; i++) {
            deviceEntries[i].isDefault = false;
        }
        entry->isDefault = true;
        result = FS_DEVMAN_OK;
    }

    return(result);
}

FS_DEVMAN_RESULT fs_devman_get_default(const char **name)
{
    FS_DEVMAN_RESULT result = FS_DEVMAN_NOT_FOUND;
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    int i;

    for (i = 0; i < FS_DEVMAN_MAX_DEVICES; i++) {
        entry = &deviceEntries[i];
        if (entry->isDefault) {
            if (name) {
                *name = entry->info.name;
            }
            result = FS_DEVMAN_OK;
        }
    }

    return(result);
}

FS_DEVMAN_RESULT fs_devman_get_default(const char **name);

FS_DEVMAN_DEVICE_INFO *fs_devman_getInfo(
    const char *name, const char **fname,
    FS_DEVMAN_RESULT *result)
{
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    FS_DEVMAN_DEVICE_INFO *info = NULL;
    FS_DEVMAN_RESULT _result = FS_DEVMAN_NOT_FOUND;

    if (name) {
        entry = fs_devman_find(name, fname, true);
        if (entry) {
            info = &entry->info;
            _result = FS_DEVMAN_OK;
        }
    }

    if (result) {
        *result = _result;
    }

    return(info);
}

void *fs_devman_opendir(const char *dirname)
{
    FS_DEVMAN_RESULT result = FS_DEVMAN_NOT_FOUND;
    FS_DEVMAN_DEVICE_ENTRY *entry = NULL;
    FS_DEVMAN_DEVICE_INFO *devInfo;
    FS_DEVMAN_DIR *ddir = NULL;
    const char *dname;

    entry = fs_devman_find(dirname, &dname, true);
    if (entry) {
        devInfo = &entry->info;
        if (!devInfo->dev->fsd_opendir) {
            return(NULL);
        }
        ddir = (FS_DEVMAN_DIR *)devInfo->dev->fsd_opendir(dname, devInfo);
        if (ddir) {
            ddir->devInfo = devInfo;
            result = FS_DEVMAN_OK;
        }
    }

    return(ddir);
}

int fs_devman_closedir(void *dir)
{
    FS_DEVMAN_DIR *ddir;
    FS_DEVMAN_DEVICE_INFO *devInfo;
    int result;

    if (dir == NULL) {
        return(FS_DEVMAN_ERROR);
    }

    ddir = (FS_DEVMAN_DIR *)dir;
    devInfo = ddir->devInfo;
    if (!devInfo->dev->fsd_closedir) {
        return(-1);
    }

    result = devInfo->dev->fsd_closedir(ddir, devInfo);

    return(result);
}

FS_DEVMAN_DIRENT *fs_devman_readdir(void *dir)
{
    FS_DEVMAN_DIR *ddir;
    FS_DEVMAN_DEVICE_INFO *devInfo;
    FS_DEVMAN_DIRENT *dirent;

    if (dir == NULL) {
        return(NULL);
    }

    ddir = (FS_DEVMAN_DIR *)dir;
    devInfo = ddir->devInfo;
    if (!devInfo->dev->fsd_readdir) {
        return(NULL);
    }

    dirent = devInfo->dev->fsd_readdir(ddir, devInfo);

    return(dirent);
}


