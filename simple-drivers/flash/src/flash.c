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

#include <string.h>
#include "flash.h"

int flash_read(const FLASH_INFO *fi, uint32_t addr, uint8_t *buf, int size)
{
    int result;
    result = fi->flash_read(fi, addr, buf, size);
    return result;
}

int flash_erase(const FLASH_INFO *fi, uint32_t addr, int size)
{
    int result;
    result = fi->flash_erase(fi, addr, size);
    return result;
}

int flash_program(const FLASH_INFO *fi, uint32_t addr, const uint8_t *buf, int size)
{
    int result;
    result = fi->flash_program(fi, addr, buf, size);
    return result;
}
