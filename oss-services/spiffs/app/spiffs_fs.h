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

#ifndef _spiffs_fs_h
#define _spiffs_fs_h

#include "spiffs.h"
#include "flash.h"

s32_t spiffs_mount(spiffs *fs, FLASH_INFO *f);
s32_t spiffs_format(spiffs *fs);

void spiffs_lock(spiffs *fs);
void spiffs_unlock(spiffs *fs);

#endif
