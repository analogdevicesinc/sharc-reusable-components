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

#ifndef _fs_dev_spiffs_h
#define _fs_dev_spiffs_h

#include "fs_devman_cfg.h"
#include "fs_devman.h"

#ifdef FS_DEVMAN_ENABLE_SPIFFS
FS_DEVMAN_DEVICE *fs_dev_spiffs_device(void);
#endif

#endif
