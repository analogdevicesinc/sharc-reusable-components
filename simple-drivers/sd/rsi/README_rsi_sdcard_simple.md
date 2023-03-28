# simple-drivers/sd/rsi

## Overview

The `sdcard_simple` driver provides low-level block I/O functions for an SD card
through the ADI RSI device driver

## Required components

- adi-drivers/rsi

## Recommended components

- FreeRTOS
- simple-services/fs-dev

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

- None

## Run

- None

## Example Code

Detecting and mounting with FatFs
```
#include <stdbool.h>

#include "sdcard_simple.h"
#include "syslog.h"
#include "umm_malloc.h"
#include "ff.h"

sSDCARD *sdcardHandle;
bool sdPresent

static void sdcardCheck(APP_CONTEXT *context)
{
    bool sd;
    SDCARD_SIMPLE_RESULT sdResult;
    SDCARD_SIMPLE_INFO sdInfo;
    char *sdTypes[] = SDCARD_TYPE_STRINGS;
    static FATFS *fs = NULL;
    FRESULT fatResult;

    /* Check for sdcard insertion/removal */
    sdResult = sdcard_present(sdcardHandle);
    sd = (sdResult == SDCARD_SIMPLE_SUCCESS);
    if (sd != sdPresent) {
        syslog_printf(
            "SDCARD: %s\n", sd ? "Inserted" : "Removed"
        );
        if (sd) {
            sdResult = sdcard_start(sdcardHandle);
            if (sdResult == SDCARD_SIMPLE_SUCCESS) {
                sdResult = sdcard_info(sdcardHandle, &sdInfo);
                if (sdResult == SDCARD_SIMPLE_SUCCESS) {
                    syslog_printf("SDCARD: Type %s\n",
                        sdTypes[sdInfo.type]
                    );
                    syslog_printf("SDCARD: Capacity %llu bytes\n",
                        (unsigned long long)sdInfo.capacity
                    );
                    fs = umm_malloc(sizeof(*fs));
                    fatResult = f_mount(fs, "sd:", 1);
                    if (fatResult == FR_OK) {
                        syslog_printf("SDCARD: FatFs mounted");
                    } else {
                        syslog_printf("SDCARD: FatFs error (%d)!", fatResult);
                    }
                } else {
                    syslog_print("SDCARD: Error getting info!\n");
                }
            } else {
                syslog_print("SDCARD: Error starting!\n");
            }
        } else {
            if (fs) {
                fatResult = f_unmount("sd:");
                if (fatResult == FR_OK) {
                    syslog_printf("SDCARD: FatFs unmount");
                } else {
                    syslog_printf("SDCARD: FatFs error (%d)!", fatResult);
                }
                umm_free(fs);
                fs = NULL;
            }
            sdcard_stop(sdcardHandle);
        }
        sdPresent = sd;
    }
}
```
