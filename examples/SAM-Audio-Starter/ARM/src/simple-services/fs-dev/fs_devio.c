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

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#if defined(__ADSPARM__)
#include <libio/device.h>
#include <libio/device_int.h>
#else
#include <device.h>
#include <device_int.h>
#endif

#include "fs_dev_adi_modes.h"

#include "fs_devman_cfg.h"
#include "fs_devman_priv.h"
#include "fs_devman.h"

#ifndef FS_DEVIO_DEVICE
#define FS_DEVIO_DEVICE     2000
#endif

#ifndef FS_DEVIO_MAX_FDS
#define FS_DEVIO_MAX_FDS    16
#endif

#ifndef FS_DEVIO_FD_OFFSET
#define FS_DEVIO_FD_OFFSET  100
#endif

typedef struct _FS_DEVIO_FD {
    bool open;
    int baseFd;
    FS_DEVMAN_DEVICE_INFO *devInfo;
} FS_DEVIO_FD;

static FS_DEVIO_FD DEVIO_FD[FS_DEVIO_MAX_FDS];

/***********************************************************************
 * Init
 ***********************************************************************/
static int _fs_devio_init(struct DevEntry *deventry)
{
    memset(&DEVIO_FD, 0, sizeof(DEVIO_FD));
    return _DEV_IS_THREADSAFE;
}

/***********************************************************************
 * Open
 ***********************************************************************/
static int _fs_devio_open(const char *name, int mode)
{
    FS_DEVMAN_DEVICE_INFO *devInfo;
    FS_DEVMAN_RESULT result;
    FS_DEVIO_FD *fdf;
    const char *fname;
    int baseFd;
    int fd;
    int i;

    devInfo = fs_devman_getInfo(name, &fname, &result);
    if (!devInfo || !devInfo->dev->fsd_open) {
        return(-1);
    }

    fd = -1; fdf = NULL;
    for (i = 0; i < FS_DEVIO_MAX_FDS; i++) {
        if (DEVIO_FD[i].open == false) {
            fd = i; fdf = &DEVIO_FD[i];
            break;
        }
    }
    if (!fdf) {
        return(-1);
    }

    baseFd = devInfo->dev->fsd_open(fname, 0, mode, devInfo);
    if (baseFd < 0) {
        return(-1);
    }

    fdf->open = true;
    fdf->baseFd = baseFd;
    fdf->devInfo = devInfo;

    return(fd + FS_DEVIO_FD_OFFSET);
}

/***********************************************************************
 * Close
 ***********************************************************************/
static int _fs_devio_close(int fd)
{
    FS_DEVIO_FD *fdf;
    FS_DEVMAN_DEVICE_INFO *devInfo;

    fd -= FS_DEVIO_FD_OFFSET;

    if ((fd < 0 || fd > FS_DEVIO_MAX_FDS)) {
        return(-1);
    }

    fdf = &DEVIO_FD[fd];
    devInfo = fdf->devInfo;
    if (!devInfo->dev->fsd_close) {
        return(-1);
    }

    fdf->open = false;

    devInfo->dev->fsd_close(fdf->baseFd, devInfo);

    return(0);
}

/***********************************************************************
 * Read / Write
 ***********************************************************************/
static int _fs_devio_read(int fd, unsigned char *buf, int size)
{
    FS_DEVIO_FD *fdf;
    FS_DEVMAN_DEVICE_INFO *devInfo;
    int readSize;

    fd -= FS_DEVIO_FD_OFFSET;

    if ((fd < 0 || fd > FS_DEVIO_MAX_FDS)) {
        return(-1);
    }

    fdf = &DEVIO_FD[fd];
    devInfo = fdf->devInfo;
    if (!devInfo->dev->fsd_read) {
        return(-1);
    }

    readSize = devInfo->dev->fsd_read(fdf->baseFd, buf, size, devInfo);
#if defined(__ADSPARM__)
    readSize = size - readSize;
#endif

    return(readSize);
}

static int _fs_devio_write(int fd, unsigned char *buf, int size)
{
    FS_DEVIO_FD *fdf;
    FS_DEVMAN_DEVICE_INFO *devInfo;
    int writeSize;

    fd -= FS_DEVIO_FD_OFFSET;

    if ((fd < 0 || fd > FS_DEVIO_MAX_FDS)) {
        return(-1);
    }

    fdf = &DEVIO_FD[fd];
    devInfo = fdf->devInfo;
    if (!devInfo->dev->fsd_write) {
        return(-1);
    }

    writeSize = devInfo->dev->fsd_write(fdf->baseFd, buf, size, devInfo);
#if defined(__ADSPARM__)
    writeSize = size - writeSize;
#endif

    return(writeSize);
}

/***********************************************************************
 * Seek
 ***********************************************************************/
static long _fs_devio_seek(int fd, long offset, int whence)
{
    FS_DEVIO_FD *fdf;
    FS_DEVMAN_DEVICE_INFO *devInfo;

    fd -= FS_DEVIO_FD_OFFSET;

    if ((fd < 0 || fd > FS_DEVIO_MAX_FDS)) {
        return(-1);
    }

    fdf = &DEVIO_FD[fd];
    devInfo = fdf->devInfo;
    if (!devInfo->dev->fsd_lseek) {
        return(-1);
    }

    offset = devInfo->dev->fsd_lseek(fdf->baseFd, offset, whence, devInfo);

    return(offset);
}

/***********************************************************************
 * Unlink
 ***********************************************************************/
static int _fs_devio_unlink(const char *filename)
{
    FS_DEVMAN_DEVICE_INFO *devInfo;
    FS_DEVMAN_RESULT result;
    const char *fname;
    int uresult;

    devInfo = fs_devman_getInfo(filename, &fname, &result);
    if (devInfo == NULL) {
        return(-1);
    }
    if (!devInfo->dev->fsd_unlink) {
        return(-1);
    }

    uresult = devInfo->dev->fsd_unlink(fname, devInfo);

    return(uresult);
}

/***********************************************************************
 * Rename
 ***********************************************************************/
static int _fs_devio_rename(const char *oldname, const char *newname)
{
    FS_DEVMAN_DEVICE_INFO *odevInfo, *ndevInfo;
    FS_DEVMAN_RESULT result;
    const char *oname, *nname;
    int rresult;

    odevInfo = fs_devman_getInfo(oldname, &oname, &result);
    ndevInfo = fs_devman_getInfo(newname, &nname, &result);
    if ((odevInfo == NULL) || (ndevInfo == NULL) || (odevInfo != ndevInfo)) {
        return(-1);
    }
    if (!odevInfo->dev->fsd_rename) {
        return(-1);
    }

    rresult = odevInfo->dev->fsd_rename(oname, nname, odevInfo);

    return(rresult);
}

/***********************************************************************
 * ARM specific devio functions and structs
 **********************************************************************/
#if defined(__ADSPARM__)

static int _fs_devio_isatty(int fh)
{
    return(0);
}

static int _fs_devio_system(const char *cmd)
{
    return(0);
}

static clock_t _fs_devio_times(void)
{
    return(0);
}

static void _fs_devio_gettimeofday(struct timeval *tp, void *tzvp)
{
}

static int _fs_devio_kill(int processID, int signal)
{
    return(0);
}

static int _fs_devio_get_errno(void)
{
    return(0);
}

DevEntry fs_devio_deventry = {
    FS_DEVIO_DEVICE,         /* int DeviceID */
    NULL,                    /* void *data */
    _fs_devio_init,          /* int device _init(struct DevEntry *d) */
    _fs_devio_open,          /* int device _open(const char *path, int flags) */
    _fs_devio_close,         /* int device _close(int fh) */
    _fs_devio_write,         /* int device _write(int fh, char *ptr, int len) */
    _fs_devio_read,          /* int device _read(int fh, char *ptr, int len) */
    _fs_devio_seek,          /* int device _seek(int fh, int pos, int dir) */
    dev_not_claimed,         /* int stdinfd */
    dev_not_claimed,         /* int stdoutfd */
    dev_not_claimed,         /* int stderrfd */
    _fs_devio_unlink,        /* int device _unlink(const char *path) */
    _fs_devio_rename,        /* int device _rename(const char *oldpath, const char *newpath) */
    _fs_devio_system,        /* int device _system(const char *cmd) */
    _fs_devio_isatty,        /* int device _isatty(int fh) */
    _fs_devio_times,         /* clock_t device _times(void) */
    _fs_devio_gettimeofday,  /* void device _gettimeofday(struct timeval *tp, void *tzvp) */
    _fs_devio_kill,          /* int device _kill(int processID, int signal) */
    _fs_devio_get_errno      /* int device _get_errno(void) */
};

#else

static int _fs_devio_ioctl(int fildes, int request, va_list varg_list)
{
    return(-1);
}

struct DevEntry_Extension fs_devio_extension = {
    DEVFLAGS_BYTEADDRESSED,
    NULL,
    NULL
};

DevEntry fs_devio_deventry = {
    FS_DEVIO_DEVICE,
    NULL,
    &_fs_devio_init,
    &_fs_devio_open,
    &_fs_devio_close,
    &_fs_devio_write,
    &_fs_devio_read,
    &_fs_devio_seek,
    dev_not_claimed,
    dev_not_claimed,
    dev_not_claimed,
    _fs_devio_unlink,
    _fs_devio_rename,
    _fs_devio_ioctl,
    &fs_devio_extension
};

#endif

/***********************************************************************
 * Public functions
 **********************************************************************/
void fs_devio_init(void)
{
    int result;

    result = add_devtab_entry(&fs_devio_deventry);

    if (result == FS_DEVIO_DEVICE) {
        set_default_io_device(FS_DEVIO_DEVICE);
    }
}

