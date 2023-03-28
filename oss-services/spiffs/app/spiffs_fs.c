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

#include <stdio.h>

#ifdef FREE_RTOS
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#include "spiffs.h"
#include "spiffs_fs.h"
#include "spiffs_fs_cfg.h"

#include "spi_simple.h"

#ifndef SPIFFS_FS_CALLOC
#define SPIFFS_FS_CALLOC calloc
#endif

#ifndef SPIFFS_FS_FREE
#define SPIFFS_FS_FREE free
#endif

#ifndef SPIFFS_FS_SIZE
#error Must define SPIFFS_FS_SIZE
#endif

#ifndef SPIFFS_FS_OFFSET
#error Must define SPIFFS_FS_OFFSET
#endif

#ifndef SPIFFS_FS_ERASE_BLOCK_SIZE
#error Must define SPIFFS_FS_ERASE_BLOCK_SIZE
#endif

#ifndef SPIFFS_FS_FLASH_PAGE_SIZE
#error Must define SPIFFS_FS_FLASH_PAGE_SIZE
#endif

typedef struct SPIFFS_FS {
    FLASH_INFO *fi;
#ifdef FREE_RTOS
    SemaphoreHandle_t lock;
#endif
    u8_t *spiffs_cache_buf;
    u32_t cache_size;
    u8_t *spiffs_work_buf;
    u8_t *spiffs_fds;
    u32_t filedescs_size;
} SPIFFS_FS;

static s32_t my_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
    SPIFFS_FS *FS = (SPIFFS_FS *)fs->user_data;
    int ok = flash_read(FS->fi, addr, dst, size);
    return(ok == FLASH_OK ? SPIFFS_OK : -1);
}

static s32_t my_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
    SPIFFS_FS *FS = (SPIFFS_FS *)fs->user_data;
    int ok = flash_program(FS->fi, addr, src, size);
    return(ok == FLASH_OK ? SPIFFS_OK : -1);
}

static s32_t my_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
    SPIFFS_FS *FS = (SPIFFS_FS *)fs->user_data;
    int ok = flash_erase(FS->fi, addr, size);
    return(ok == FLASH_OK ? SPIFFS_OK : -1);
}

void spiffs_lock(spiffs *fs)
{
#ifdef FREE_RTOS
    SPIFFS_FS *FS = (SPIFFS_FS *)fs->user_data;
    xSemaphoreTake(FS->lock, portMAX_DELAY);
#endif
}

void spiffs_unlock(spiffs *fs)
{
#ifdef FREE_RTOS
    SPIFFS_FS *FS = (SPIFFS_FS *)fs->user_data;
    xSemaphoreGive(FS->lock);
#endif
}

s32_t spiffs_mount(spiffs *fs, FLASH_INFO *fi)
{
    spiffs_config cfg;
    SPIFFS_FS *FS;

    FS = SPIFFS_FS_CALLOC(1, sizeof(*FS));
    FS->spiffs_work_buf =
        SPIFFS_FS_CALLOC(SPIFFS_FS_FLASH_PAGE_SIZE*2, sizeof(u8_t));
    FS->filedescs_size =
        SPIFFS_buffer_bytes_for_filedescs(fs, 32);
    FS->spiffs_fds =
        SPIFFS_FS_CALLOC(FS->filedescs_size, sizeof(u8_t));
#if SPIFFS_CACHE
    FS->cache_size = SPIFFS_buffer_bytes_for_cache(fs, 32);
    FS->spiffs_cache_buf = SPIFFS_FS_CALLOC(FS->cache_size, sizeof(u8_t));
#else
    FS->cache_size = 0;
    FS->spiffs_cache_buf = NULL;
#endif
#ifdef FREE_RTOS
    FS->lock = xSemaphoreCreateMutex();
#endif
    FS->fi = fi;
    fs->user_data = FS;

    cfg.phys_size = SPIFFS_FS_SIZE;
    cfg.phys_addr = SPIFFS_FS_OFFSET;
    cfg.phys_erase_block = SPIFFS_FS_ERASE_BLOCK_SIZE;
    cfg.log_block_size = SPIFFS_FS_ERASE_BLOCK_SIZE * 16;
    cfg.log_page_size = SPIFFS_FS_FLASH_PAGE_SIZE;

    cfg.hal_read_f = my_spiffs_read;
    cfg.hal_write_f = my_spiffs_write;
    cfg.hal_erase_f = my_spiffs_erase;

    int res = SPIFFS_mount(fs,
        &cfg,
        FS->spiffs_work_buf,
        FS->spiffs_fds,
        FS->filedescs_size,
        FS->spiffs_cache_buf,
        FS->cache_size,
        0
    );

    return(res);
}

void spiffs_unmount(spiffs *fs, FLASH_INFO **fi)
{
    SPIFFS_FS *FS = (SPIFFS_FS *)fs->user_data;

    if (SPIFFS_mounted(fs)) {
        SPIFFS_unmount(fs);
    }

#ifdef FREE_RTOS
    if (FS->lock) {
        vSemaphoreDelete(FS->lock);
    }
#endif

    if (FS->spiffs_cache_buf) {
        SPIFFS_FS_FREE(FS->spiffs_cache_buf);
    }
    if (FS->spiffs_work_buf) {
        SPIFFS_FS_FREE(FS->spiffs_work_buf);
    }
    if (FS->spiffs_fds) {
        SPIFFS_FS_FREE(FS->spiffs_fds);
    }

    if (fi) {
        *fi = FS->fi;
    }

    SPIFFS_FS_FREE(FS);
}

s32_t spiffs_format(spiffs *fs)
{
    FLASH_INFO *fi;
    s32_t ok;

    if (SPIFFS_mounted(fs)) {
        SPIFFS_unmount(fs);
    }

    ok = SPIFFS_format(fs);

    spiffs_unmount(fs, &fi);

    if (ok == SPIFFS_OK) {
        ok = spiffs_mount(fs, fi);
    }

    return(ok);
}
