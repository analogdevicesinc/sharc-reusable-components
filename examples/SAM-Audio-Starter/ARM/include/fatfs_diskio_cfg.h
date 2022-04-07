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

#ifndef _fatfs_diskio_cfg_h
#define _fatfs_diskio_cfg_h

/* Use application util_time() function instead of standard
 * c runtime time()
 */
#include "util.h"
#define FATFS_DISKIO_TIME util_time

/* Enable SD card support through sdcard_simple driver */
#define FATFS_DISKIO_ENABLE_SDCARD
#define FATFS_DISKIO_SDCARD_DEVICE 0

/* Enable USB Mass Storage support through msd_simple driver */
//#define FATFS_DISKIO_ENABLE_MSD
//#define FATFS_DISKIO_MSD_DEVICE    1

#endif
