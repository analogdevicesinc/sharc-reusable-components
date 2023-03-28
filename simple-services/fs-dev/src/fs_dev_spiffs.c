/**
 * Copyright (c) 2022 - Analog Devices Inc. All Rights Reserved.
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

#ifdef FS_DEVMAN_ENABLE_SPIFFS

#include "spiffs.h"

/*
 * Helpful fopen() mode cheat sheet
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

static int dev_spiffs_open(const char *path, int flags, int mode, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    spiffs_flags spiffsFlags;
    spiffs_file fd;

    spiffsFlags = 0;

#if defined(__ADSPARM__)
   if (mode & ADI_BINARY) {
       /* Binary mode, ignore */
   }
   if (mode & ADI_RW) {
       /* Read/Write mode */
       spiffsFlags |= SPIFFS_O_RDWR;
   }
   if (mode & ADI_WRITE) {
       /* Write mode */
       if ((mode & ADI_RW) == 0) {
           spiffsFlags |= SPIFFS_O_WRONLY;
       }
       spiffsFlags |= SPIFFS_O_CREAT | SPIFFS_O_TRUNC;
   } else if (mode & ADI_APPEND) {
       /* Append mode */
       if ((mode & ADI_RW) == 0) {
           spiffsFlags |= SPIFFS_O_WRONLY;
       }
       spiffsFlags |= SPIFFS_O_CREAT | SPIFFS_O_APPEND;
   } else {
       if ((mode & ADI_RW) == 0) {
           spiffsFlags |= SPIFFS_O_RDONLY;
       }
   }
#else
    if ((mode & ADI_RW) == ADI_RW)
        spiffsFlags |= SPIFFS_O_RDWR;
    else if (mode & ADI_READ)
        spiffsFlags |= SPIFFS_O_RDONLY;
    else if (mode & ADI_WRITE)
        spiffsFlags |= SPIFFS_O_WRONLY;

    if (mode & ADI_APPEND) spiffsFlags |= SPIFFS_O_APPEND;
    if (mode & ADI_CREAT) spiffsFlags |= SPIFFS_O_CREAT;
    if (mode & ADI_TRUNC) spiffsFlags |= SPIFFS_O_TRUNC;
#endif

   /* Lua looks for modules starting with a './' path */
   if ((strlen(path) > 2) && (strncmp(path, "./", 2) == 0)) {
      path += 2;
   }

    fd = SPIFFS_open(fs, path, spiffsFlags, 0);

    return(fd);
}

static int dev_spiffs_close(int fd, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    s32_t result;

    result = SPIFFS_close(fs, fd);

    return(result);
}

static ssize_t dev_spiffs_read(int fd, void *ptr, size_t len, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    s32_t result;

    result = SPIFFS_read(fs, fd, ptr, len);
    if (result < 0) {
#if defined(__ADSPARM__)
        return(0);
#else
        return(result);
#endif
    }

    return(result);
}

static ssize_t dev_spiffs_write(int fd, const void *ptr, size_t len, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    s32_t result;

    result = SPIFFS_write(fs, fd, (void *)ptr, len);
    if (result < 0) {
#if defined(__ADSPARM__)
        return(0);
#else
        return(result);
#endif
    }

    return(result);
}

static off_t dev_spiffs_lseek(int fd, off_t off, int whence, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    s32_t result;
    int spiffs_whence;

    switch (whence)
    {
        case SEEK_SET:
            spiffs_whence = SPIFFS_SEEK_SET;
            break;
        case SEEK_CUR:
            spiffs_whence = SEEK_CUR;
            break;
        case SEEK_END:
            spiffs_whence = SEEK_END;
            break;
        default:
            return -1;
    }

    result = SPIFFS_lseek(fs, fd, off, spiffs_whence);

    return(result);
}

static void *dev_spiffs_opendir(const char *name, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    FS_DEVMAN_DIR *ddir = NULL;
    spiffs_DIR *sdir = NULL;
    spiffs_DIR *ok = NULL;

    sdir = FS_DEVMAN_CALLOC(1, sizeof(*sdir));
    if (sdir) {
        ok = SPIFFS_opendir(fs, name, sdir);
        if (ok) {
            ddir = FS_DEVMAN_CALLOC(1, sizeof(*ddir));
            if (ddir) {
                ddir->dir = sdir;
            }
        }
    }

    if (!ok) {
        if (sdir) {
            FS_DEVMAN_FREE(sdir);
            sdir = NULL;
        }
        if (ddir) {
            FS_DEVMAN_FREE(ddir);
            ddir = NULL;
        }
    }

    return(ddir);
}

static FS_DEVMAN_DIRENT *dev_spiffs_readdir(void *dir, void *pdata)
{
    FS_DEVMAN_DIR *ddir = (FS_DEVMAN_DIR *)dir;
    FS_DEVMAN_DIRENT *dirent = NULL;

    struct spiffs_dirent *pe;
    struct spiffs_dirent *ok;
    size_t  size;

    pe = FS_DEVMAN_CALLOC(1, sizeof(*pe));

    ok = SPIFFS_readdir((spiffs_DIR *)ddir->dir, pe);
    if (ok) {
        dirent = &ddir->dirent;
        dirent->fsize = pe->size;
        dirent->ftime = 0;
        dirent->fdate = 0;
        dirent->flags = 0;
        size = strlen((const char *)pe->name)+1;
        if (dirent->fname) {
            FS_DEVMAN_FREE((void *)dirent->fname);
        }
        dirent->fname = FS_DEVMAN_CALLOC(size, sizeof(*dirent->fname));
        if (dirent->fname) {
            memcpy((void *)dirent->fname, pe->name, size);
        } else {
            dirent = NULL;
        }
     }

    FS_DEVMAN_FREE(pe);

    return(dirent);
}

static int dev_spiffs_closedir(void *dir, void *pdata)
{
    FS_DEVMAN_DIR *ddir = (FS_DEVMAN_DIR *)dir;
    s32_t result;

    result = SPIFFS_closedir((spiffs_DIR *)ddir->dir);

    if (ddir->dirent.fname) {
        FS_DEVMAN_FREE((void *)ddir->dirent.fname);
    }
    FS_DEVMAN_FREE(ddir->dir);
    FS_DEVMAN_FREE(ddir);

    return(result);
}

static int dev_spiffs_unlink(const char *fname, void *pdata)
{
    FS_DEVMAN_DEVICE_INFO *devInfo = (FS_DEVMAN_DEVICE_INFO *)pdata;
    spiffs *fs = (spiffs *)devInfo->usr;
    s32_t result;

    result = SPIFFS_remove(fs, fname);

    return(result);
}

static FS_DEVMAN_DEVICE FS_DEV_SPIFFS = {
  .fsd_open = dev_spiffs_open,
  .fsd_close = dev_spiffs_close,
  .fsd_read = dev_spiffs_read,
  .fsd_write = dev_spiffs_write,
  .fsd_lseek = dev_spiffs_lseek,
  .fsd_opendir = dev_spiffs_opendir,
  .fsd_readdir = dev_spiffs_readdir,
  .fsd_closedir = dev_spiffs_closedir,
  .fsd_unlink = dev_spiffs_unlink,
  .fsd_rename = NULL
};

FS_DEVMAN_DEVICE *fs_dev_spiffs_device(void)
{
    return(&FS_DEV_SPIFFS);
}

#endif
