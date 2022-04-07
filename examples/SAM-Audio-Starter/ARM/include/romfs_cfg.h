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

#ifndef _romfs_cfg_h
#define _romfs_cfg_h

#include "flash_map.h"

#define ROMFS_FLASH_START_ADDRESS (FLASH_ADDR)
#define ROMFS_FLASH_SECTOR_SIZE   (4 * 1024)

#define ROMFS_FS_START_ADDRESS    (FS_OFFSET)
#define ROMFS_FS_END_ADDRESS      (FS_OFFSET + FS_SIZE)

#endif

