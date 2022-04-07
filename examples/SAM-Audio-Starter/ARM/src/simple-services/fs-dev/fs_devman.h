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

#ifndef _fs_devman_h
#define _fs_devman_h

#include <stdint.h>
#include <unistd.h>

typedef enum _FS_DEVMAN_RESULT {
    FS_DEVMAN_ERROR = -4,
    FS_DEVMAN_NOT_FOUND = -3,
    FS_DEVMAN_NO_ENTRIES = -2,
    FS_DEVMAN_INVALID_NAME = -1,
    FS_DEVMAN_OK = 0
} FS_DEVMAN_RESULT;

enum _FS_DEVMAN_DIRENT_FLAGS {
    FS_DEVMAN_DIRENT_FLAG_DIR = 1
};

typedef struct _FS_DEVMAN_DEVICE FS_DEVMAN_DEVICE;

/*
 * Encode/Decode fdate as:
 *  bit15:9 Year origin from 1980 (0..127)
 *  bit8:5  Month (1..12)
 *  bit4:0  Day (1..31)
 *
 * Encode/Decode ftime as:
 *  bit15:11 Hour (0..23)
 *  bit10:5  Minute (0..59)
 *  bit4:0   Second / 2 (0..29)
 *
*/
typedef struct _FS_DEVMAN_DIRENT {
  const char *fname;
  uint32_t fsize;
  uint32_t ftime;
  uint32_t fdate;
  uint8_t flags;
} FS_DEVMAN_DIRENT;

/*
 * Initialize the device manager
 */
FS_DEVMAN_RESULT fs_devman_init(void);

/*
 * Register a device.  All pointers must remain valid as long as the device
 * is registered with the device manager.
 *
 * All device names must terminate with a ':'.  All path names must begin
 * with a device prefix.
 */
FS_DEVMAN_RESULT fs_devman_register(const char *name, const FS_DEVMAN_DEVICE *dev, void *usr);
FS_DEVMAN_RESULT fs_devman_unregister(const char *name);

/*
 * Set/Get the default device.  Used when no device name is included
 * in a file or directory open.  The first registered device is the
 * default unless set otherwise.  If the default device is unregistered,
 * no device will be the default device.
 */
FS_DEVMAN_RESULT fs_devman_set_default(const char *name);
FS_DEVMAN_RESULT fs_devman_get_default(const char **name);

void *fs_devman_opendir(const char *dirname);
int fs_devman_closedir(void *dir);
FS_DEVMAN_DIRENT *fs_devman_readdir(void *dir);

#endif
