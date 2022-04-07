# simple-drivers/msd/CLD-msd_host

## Overview

The `msd_simple` driver provides low-level block I/O functions for a USB Host
Mass Storage device.

## Required components

- simple-services/msd-host

## Recommended components

- FreeRTOS
- simple-services/fs-dev

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

- None

## Run

- None

## Example code

Detecting and mounting with FatFs
```
#include <stdbool.h>

#include "msd_simple.h"
#include "syslog.h"
#include "umm_malloc.h"
#include "ff.h"

sSDCARD *msdHandle;
bool msdPresent

static void msdCheck(APP_CONTEXT *context)
{
    bool msd;
    MSD_SIMPLE_RESULT msdResult;
    MSD_SIMPLE_INFO msdInfo;
    static FATFS * = NULL;
    FRESULT fatResult;

    /* Check for sdcard insertion/removal */
    msdResult = msd_present(msdHandle);
    msd = (msdResult == MSD_SIMPLE_SUCCESS);
    if (msd != msdPresent) {
        syslog_printf(
            "USB MSD: %s\n", msd ? "Inserted" : "Removed"
        );
        if (msd) {
            msdResult = msd_info(msdHandle, &msdInfo);
            if (msdResult == MSD_SIMPLE_SUCCESS) {
                syslog_printf("USB MSD: Capacity %llu bytes\n",
                    (unsigned long long)msdInfo.capacity
                );
                fs = umm_malloc(sizeof(*fs));
                fatResult = f_mount(fs, "usb:", 1);
                if (fatResult == FR_OK) {
                    syslog_printf("USB MSD: FatFs mounted");
                } else {
                    syslog_printf("USB MSD: FatFs error (%d)!", fatResult);
                }
            } else {
                syslog_print("USB MSD: Error getting info!\n");
            }
        } else {
            if (fs) {
                fatResult = f_unmount("usb:");
                if (fatResult == FR_OK) {
                    syslog_printf("USB MSD: FatFs unmount");
                } else {
                    syslog_printf("USB MSD: FatFs error (%d)!", fatResult);
                }
                umm_free(fs);
                fs = NULL;
            }
        }

        msdPresent = msd;
    }
}
```
