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

#ifndef _flash_map_h
#define _flash_map_h

/* Second-stage boot loader (64k reserved) */
#define BOOT0_OFFSET  (0x00000000)
#define BOOT0_SIZE    (0x00010000)

/* Third stage boot loader (USB re-flasher, 192k reserved) */
#define BOOT1_OFFSET  (BOOT0_OFFSET + BOOT0_SIZE)
#define BOOT1_SIZE    (0x00030000)

/* Application (2M reserved) */
#define APP_OFFSET    (BOOT1_OFFSET + BOOT1_SIZE)
#define APP_SIZE      (0x00200000)

#endif
