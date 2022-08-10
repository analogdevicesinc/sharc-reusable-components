/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
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

#if defined(__ADSPARM__)
#include <libio/device.h>
#include <libio/device_int.h>
#else
#include <device.h>
#include <device_int.h>
#endif

#include "romfs.h"
#include "romfs_devman.h"

#define ROMFS_DEVICE   2000

#if defined(__ADSPARM__)
#define ADI_READ     0x0000
#define ADI_BINARY   0x0001
#define ADI_RW       0x0002
#define ADI_WRITE    0x0004
#define ADI_APPEND   0x0008
#else
#define ADI_READ     0x0001
#define ADI_WRITE    0x0002
#define ADI_APPEND   0x0004
#define ADI_TRUNC    0x0008
#define ADI_CREAT    0x0010
#define ADI_RW       (ADI_READ | ADI_WRITE)
#define ADI_BINARY   0x0020
#endif
static int fsio_init(struct DevEntry *deventry)
{
   return _DEV_IS_THREADSAFE;
}

static int fsio_open(const char *name, int mode)
{
   int flags, fd;

   flags = 0;

#if defined(__ADSPARM__)
   if (mode & ADI_BINARY) {
       /* Binary mode, ignore */
   }
   if (mode & ADI_RW) {
       /* Read/Write mode */
       flags |= O_RDWR;
   }
   if (mode & ADI_WRITE) {
       /* Write mode */
       if ((flags & ADI_RW) == 0) {
           flags |= O_WRONLY;
       }
       flags |= O_CREAT | O_TRUNC;
   } else if (mode & ADI_APPEND) {
       /* Append mode */
       if ((flags & ADI_RW) == 0) {
           flags |= O_WRONLY;
       }
       flags |= O_CREAT | O_APPEND;
   } else {
       if ((flags & ADI_RW) == 0) {
           flags |= O_RDONLY;
       }
   }
#else
   if ((mode & ADI_RW) == ADI_RW)
      flags |= O_RDWR;
   else if (mode & ADI_READ)
      flags |= O_RDONLY;
   else if (mode & ADI_WRITE)
      flags |= O_WRONLY;

   if (mode & ADI_APPEND) flags |= O_APPEND;
   if (mode & ADI_CREAT) flags |= O_CREAT;
   if (mode & ADI_TRUNC) flags |= O_TRUNC;
#endif

   /* Lua looks for modules starting with a './' path */
   if ((strlen(name) > 2) && (strncmp(name, "./", 2) == 0)) {
      name += 2;
   }

   fd = fs_open(name, flags);

   return(fd);
}

static int fsio_close(int fd)
{
   fs_close(fd);
   return(0);
}

static int fsio_write(int fd, unsigned char *buf, int size)
{
    int writeSize;
    writeSize = fs_write(fd, buf, size);
#if defined(__ADSPARM__)
    writeSize = size - writeSize;
#endif
    return(writeSize);
}

static int fsio_read(int fd, unsigned char *buf, int size)
{
    int readSize;

    readSize = fs_read(fd, buf, size);
#if defined(__ADSPARM__)
    readSize = size - readSize;
#endif
    return(readSize);
}

static long fsio_seek(int fd, long offset, int whence)
{
    offset = fs_seek(fd, offset, whence);
    return(offset);
}

static int fsio_unlink(const char *filename)
{
    return(-1);
}

static int fsio_rename(const char *oldname, const char *newname)
{
    return(-1);
}

/***********************************************************************
 * ARM specific devio functions and structs
 **********************************************************************/
#if defined(__ADSPARM__)

static int fsio_isatty(int fh)
{
    return(0);
}

static int fsio_system(const char *cmd)
{
    return(0);
}

static clock_t fsio_times(void)
{
    return(0);
}

static void fsio_gettimeofday(struct timeval *tp, void *tzvp)
{
}

static int fsio_kill(int processID, int signal)
{
    return(0);
}

static int fsio_get_errno(void)
{
    return(0);
}

DevEntry fsio_deventry = {
    ROMFS_DEVICE,          /* int DeviceID */
    NULL,                  /* void *data */
    fsio_init,             /* int device _init(struct DevEntry *d) */
    fsio_open,             /* int device _open(const char *path, int flags) */
    fsio_close,            /* int device _close(int fh) */
    fsio_write,            /* int device _write(int fh, char *ptr, int len) */
    fsio_read,             /* int device _read(int fh, char *ptr, int len) */
    fsio_seek,             /* int device _seek(int fh, int pos, int dir) */
    dev_not_claimed,       /* int stdinfd */
    dev_not_claimed,       /* int stdoutfd */
    dev_not_claimed,       /* int stderrfd */
    fsio_unlink,           /* int device _unlink(const char *path) */
    fsio_rename,           /* int device _rename(const char *oldpath, const char *newpath) */
    fsio_system,           /* int device _system(const char *cmd) */
    fsio_isatty,           /* int device _isatty(int fh) */
    fsio_times,            /* clock_t device _times(void) */
    fsio_gettimeofday,     /* void device _gettimeofday(struct timeval *tp, void *tzvp) */
    fsio_kill,             /* int device _kill(int processID, int signal) */
    fsio_get_errno         /* int device _get_errno(void) */
};

#else

static int fsio_ioctl(int fildes, int request, va_list varg_list)
{
    return(-1);
}

struct DevEntry_Extension fsio_extension = {
    DEVFLAGS_BYTEADDRESSED,
    NULL,
    NULL
};

DevEntry fsio_deventry = {
    ROMFS_DEVICE,
    NULL,
    &fsio_init,
    &fsio_open,
    &fsio_close,
    &fsio_write,
    &fsio_read,
    &fsio_seek,
    dev_not_claimed,
    dev_not_claimed,
    dev_not_claimed,
    fsio_unlink,
    fsio_rename,
    fsio_ioctl,
    &fsio_extension
};

#endif

/***********************************************************************
 * Public functions
 **********************************************************************/
void romfs_devio_init(void)
{
    int result;

    result = add_devtab_entry(&fsio_deventry);

    if (result == ROMFS_DEVICE) {
        set_default_io_device(ROMFS_DEVICE);
    }
}

