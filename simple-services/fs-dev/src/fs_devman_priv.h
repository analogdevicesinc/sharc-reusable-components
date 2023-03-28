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

#ifndef _fs_devman_priv_h
#define _fs_devman_priv_h

#include <stdlib.h>
#if defined(__ADSPARM__)
#include <unistd.h>
#else
#define off_t long int
#define ssize_t long int
#endif

#include "fs_devman_cfg.h"
#include "fs_devman.h"

#ifndef FS_DEVMAN_CALLOC
#define FS_DEVMAN_CALLOC      calloc
#endif

#ifndef FS_DEVMAN_FREE
#define FS_DEVMAN_FREE        free
#endif

#ifndef FS_DEVMAN_MAX_DEVICES
#define FS_DEVMAN_MAX_DEVICES 4
#endif

typedef struct _FS_DEVMAN_DEVICE_INFO {
  const char *name;
  const FS_DEVMAN_DEVICE *dev;
  void *usr;
} FS_DEVMAN_DEVICE_INFO;

typedef struct _FS_DEVMAN_DIR {
    FS_DEVMAN_DEVICE_INFO *devInfo;
    FS_DEVMAN_DIRENT dirent;
    void *dir;
} FS_DEVMAN_DIR;

FS_DEVMAN_DEVICE_INFO *fs_devman_getInfo(
    const char *name, const char **fname,
    FS_DEVMAN_RESULT *result
);

struct _FS_DEVMAN_DEVICE {
  int (*fsd_open)(const char *path, int flags, int mode, void *pdata);
  int (*fsd_close)(int fd, void *pdata);
  ssize_t (*fsd_read)(int fd, void *ptr, size_t len, void *pdata);
  ssize_t (*fsd_write) (int fd, const void *ptr, size_t len, void *pdata);
  off_t (*fsd_lseek)(int fd, off_t off, int whence, void *pdata);
  void *(*fsd_opendir)(const char* name, void *pdata);
  FS_DEVMAN_DIRENT *(*fsd_readdir)(void *dir, void *pdata);
  int (*fsd_closedir)(void *dir, void *pdata);
  int (*fsd_unlink)(const char *fname, void *pdata);
  int (*fsd_rename)(const char *oldname, const char *newname, void *pdata);
};

#endif
