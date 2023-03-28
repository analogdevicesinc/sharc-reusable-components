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

#ifndef _spiffs_fs_cfg_h
#define _spiffs_fs_cfg_h

#include "flash_map.h"
#include "umm_malloc.h"

#define SPIFFS_FS_CALLOC            umm_calloc
#define SPIFFS_FS_FREE              umm_free
#define SPIFFS_FS_SIZE              SPIFFS_SIZE
#define SPIFFS_FS_OFFSET            SPIFFS_OFFSET
#define SPIFFS_FS_ERASE_BLOCK_SIZE  ERASE_BLOCK_SIZE
#define SPIFFS_FS_FLASH_PAGE_SIZE   FLASH_PAGE_SIZE


#endif
