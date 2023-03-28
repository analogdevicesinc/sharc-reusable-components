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

#ifndef _wav_file_cfg_h
#define _wav_file_cfg_h

#include "umm_malloc.h"

#define WAVE_FILE_CALLOC          umm_calloc
#define WAVE_FILE_FREE            umm_free

#define WAVE_FILE_BUF_SIZE       (16 * 1024)

#endif
