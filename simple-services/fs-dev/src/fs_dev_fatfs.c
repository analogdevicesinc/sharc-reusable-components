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

#include <string.h>
#include <stdbool.h>

#include "fs_dev_adi_modes.h"

#include "fs_devman_cfg.h"
#include "fs_devman_priv.h"
#include "fs_devman.h"

#ifdef FS_DEVMAN_ENABLE_FATFS

#include "ff.h"

#ifndef FS_DEVIO_MAX_FATFS_FD
#define FS_DEVIO_MAX_FATFS_FD    16
#endif

typedef struct _FSIO_FATFS_FD {
    FIL f;
    bool open;
} FSIO_FATFS_FD;

static FSIO_FATFS_FD fatfsFd[FS_DEVIO_MAX_FATFS_FD];

static char *fullPath(const char *path, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    size_t size;
    char *p;

    size = strlen(devInfo->name) + strlen(path) + 1;
    p = FS_DEVMAN_CALLOC(1, size);
    if (p) {
        strcat(p, devInfo->name); strcat(p, path);
    }

    return(p);
}

/*
 * Helpful fopen() mode cheat sheet
 *
 *  POSIX   FatFs
 *  ------  --------------------------------------
 *  "r"     FA_READ
 *  "r+"    FA_READ | FA_WRITE
 *  "w"     FA_CREATE_ALWAYS | FA_WRITE
 *  "w+"    FA_CREATE_ALWAYS | FA_WRITE | FA_READ
 *  "a"     FA_OPEN_APPEND | FA_WRITE
 *  "a+"    FA_OPEN_APPEND | FA_WRITE | FA_READ
 *  "wx"    FA_CREATE_NEW | FA_WRITE
 *  "w+x"   FA_CREATE_NEW | FA_WRITE | FA_READ
 *
 *  fopen() open()
 *  mode    flags
 *  ------  --------------------------------------
 *  "r"     O_RDONLY
 *  "r+"    O_RDWR
 *  "w"     O_WRONLY | O_CREAT | O_TRUNC
 *  "w+"    O_RDWR | O_CREAT | O_TRUNC
 *  "a"     O_WRONLY | O_CREAT | O_APPEND
 *  "a+"    O_RDWR | O_CREAT | O_APPEND
 */
static int dev_fatfs_open(const char *path, int flags, int mode, void *pdata)
{
    BYTE fatfsFlags;
    FRESULT result;
    char *fp;
    FIL *f;
    int fd;
    int i;

    fatfsFlags = 0;

#if defined(__ADSPARM__)
    if (mode & ADI_BINARY) {
        /* Binary mode, ignore */
    }
    if (mode & ADI_RW) {
        /* Read/Write mode */
        fatfsFlags |= FA_READ | FA_WRITE;
    }
    if (mode & ADI_WRITE) {
        /* Write mode */
        if ((mode & ADI_RW) == 0) {
            fatfsFlags |= FA_WRITE;
        }
        fatfsFlags |= FA_CREATE_ALWAYS;
    } else if (mode & ADI_APPEND) {
        /* Append mode */
        if ((mode & ADI_RW) == 0) {
            fatfsFlags |= FA_WRITE;
        }
        fatfsFlags |= FA_OPEN_APPEND;
    } else {
        if ((mode & ADI_RW) == 0) {
            fatfsFlags |= FA_READ;
        }
    }
#else
    if ((mode & ADI_RW) == ADI_RW)
        fatfsFlags |= FA_READ | FA_WRITE;
    else if (mode & ADI_READ)
        fatfsFlags |= FA_READ;
    else if (mode & ADI_WRITE)
        fatfsFlags |= FA_WRITE;

    if (mode & ADI_APPEND) fatfsFlags |= FA_OPEN_APPEND;
    if (mode & ADI_CREAT) fatfsFlags |= FA_CREATE_ALWAYS;
    if (mode & ADI_TRUNC) fatfsFlags |= 0;
#endif

    f = NULL; fd = -1;
    for (i = 0; i < FS_DEVIO_MAX_FATFS_FD; i++) {
        if (fatfsFd[i].open == false) {
            f = &fatfsFd[i].f;
            fatfsFd[i].open = true;
            break;
        }
    }

    fp = fullPath(path, pdata);

    if (f && fp) {
        result = f_open(f, fp, fatfsFlags);
        if (result == FR_OK) {
            fd  = i;
        } else {
            fatfsFd[i].open = false;
        }
    }

    FS_DEVMAN_FREE(fp);

    return(fd);
}

static int dev_fatfs_close(int fd, void *pdata)
{
    FSIO_FATFS_FD *f;
    FRESULT fresult;
    int result = -1;

    if (fd < FS_DEVIO_MAX_FATFS_FD) {
        f = &fatfsFd[fd];
        if (f->open) {
            fresult = f_close(&f->f);
            result = (fresult == FR_OK) ? 0 : -1;
            f->open = false;
        }
    }

    return(result);
}

static ssize_t dev_fatfs_read(int fd, void *ptr, size_t len, void *pdata)
{
    FSIO_FATFS_FD *f;
    FRESULT result;
    UINT readSize;

    f = &fatfsFd[fd];
    result = f_read(&f->f, ptr, (UINT)len, &readSize);
    if (result != FR_OK) {
#if defined(__ADSPARM__)
        return(0);
#else
        return(-1);
#endif
    }

    return((ssize_t)readSize);
}

static ssize_t dev_fatfs_write(int fd, const void *ptr, size_t len, void *pdata)
{
    FSIO_FATFS_FD *f;
    FRESULT result;
    UINT writeSize;

    f = &fatfsFd[fd];
    result = f_write(&f->f, ptr, (UINT)len, &writeSize);
    if (result != FR_OK) {
#if defined(__ADSPARM__)
        return(0);
#else
        return(-1);
#endif
    }

    return((ssize_t)writeSize);
}

static off_t dev_fatfs_lseek(int fd, off_t off, int whence, void *pdata)
{
    FSIO_FATFS_FD *f;
    FRESULT result;
    FSIZE_t newPos;

    f = &fatfsFd[fd];

    switch (whence)
    {
        case SEEK_SET:
            newPos = off;
            break;
        case SEEK_CUR:
            newPos = f_tell(&f->f) + off;
            break;
        case SEEK_END:
            newPos = f_size(&f->f) + off;
            break;
        default:
            return -1;
    }

    result = f_lseek(&f->f, newPos);
    if (result != FR_OK) {
        return -1;
    }

    return newPos;
}

static void *dev_fatfs_opendir(const char *name, void *pdata)
{
    FS_DEVMAN_DIR *ddir = NULL;
    FRESULT result;
    char *fp;
    DIR *dp = NULL;

    fp = fullPath(name, pdata);

    if (fp) {
        dp = FS_DEVMAN_CALLOC(1, sizeof(*dp));
        if (dp) {
            result = f_opendir(dp, fp);
            if (result == FR_OK) {
                ddir = FS_DEVMAN_CALLOC(1, sizeof(*ddir));
                if (ddir) {
                    ddir->dir = dp;
                }
            }
        }
    }

    if (!ddir && dp) {
        FS_DEVMAN_FREE(dp);
    }

    FS_DEVMAN_FREE(fp);

    return(ddir);
}

static FS_DEVMAN_DIRENT *dev_fatfs_readdir(void *dir, void *pdata)
{
    FS_DEVMAN_DIR *ddir = (FS_DEVMAN_DIR *)dir;
    FS_DEVMAN_DIRENT *dirent = NULL;
    FILINFO *fileInfo;
    FRESULT result;
    size_t  size;

    fileInfo = FS_DEVMAN_CALLOC(1, sizeof(*fileInfo));

    result = f_readdir(ddir->dir, fileInfo);
    if ((result == FR_OK) && fileInfo->fname[0]) {
        dirent = &ddir->dirent;
        dirent->fsize = fileInfo->fsize;
        dirent->ftime = fileInfo->ftime;
        dirent->fdate = fileInfo->fdate;
        dirent->flags = 0;
        if (fileInfo->fattrib & AM_DIR) {
            dirent->flags |= FS_DEVMAN_DIRENT_FLAG_DIR;
        }
        size = strlen(fileInfo->fname)+1;
        if (dirent->fname) {
            FS_DEVMAN_FREE((void *)dirent->fname);
        }
        dirent->fname = FS_DEVMAN_CALLOC(size, sizeof(*dirent->fname));
        if (dirent->fname) {
            memcpy((void *)dirent->fname, fileInfo->fname, size);
        } else {
            dirent = NULL;
        }
     }

    FS_DEVMAN_FREE(fileInfo);

    return(dirent);
}

static int dev_fatfs_closedir(void *dir, void *pdata)
{
    FS_DEVMAN_DIR *ddir = (FS_DEVMAN_DIR *)dir;
    FRESULT result;

    result = f_closedir((DIR *)ddir->dir);
    if (ddir->dirent.fname) {
        FS_DEVMAN_FREE((void *)ddir->dirent.fname);
    }
    FS_DEVMAN_FREE(ddir->dir);
    FS_DEVMAN_FREE(ddir);

    return((result == FR_OK) ? 0 : -1);
}

static int dev_fatfs_unlink(const char *fname, void *pdata)
{
    FRESULT result = FR_NO_FILE;
    char *fp;

    fp = fullPath(fname, pdata);
    if (fp) {
        result =  f_unlink(fp);
        FS_DEVMAN_FREE(fp);
    }

    return((result == FR_OK) ? 0 : -1);
}

static FS_DEVMAN_DEVICE FS_DEV_ROMFS = {
  .fsd_open = dev_fatfs_open,
  .fsd_close = dev_fatfs_close,
  .fsd_read = dev_fatfs_read,
  .fsd_write = dev_fatfs_write,
  .fsd_lseek = dev_fatfs_lseek,
  .fsd_opendir = dev_fatfs_opendir,
  .fsd_readdir = dev_fatfs_readdir,
  .fsd_closedir = dev_fatfs_closedir,
  .fsd_unlink = dev_fatfs_unlink,
  .fsd_rename = NULL
};

FS_DEVMAN_DEVICE *fs_dev_fatfs_device(void)
{
    return(&FS_DEV_ROMFS);
}

#endif
