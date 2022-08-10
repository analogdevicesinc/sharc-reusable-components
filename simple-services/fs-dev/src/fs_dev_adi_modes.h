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

#ifndef _fs_dev_adi_modes_h
#define _fs_dev_adi_modes_h

#if defined(__ADSPARM__)
#define ADI_READ     0x0000
#define ADI_BINARY   0x0001
#define ADI_RW       0x0002
#define ADI_WRITE    0x0004
#define ADI_APPEND   0x0008
#else
#define ADI_READ     0x0001
#define ADI_WRITE    0x0002
#define ADI_APPEND   0x0004
#define ADI_TRUNC    0x0008
#define ADI_CREAT    0x0010
#define ADI_RW       (ADI_READ | ADI_WRITE)
#define ADI_BINARY   0x0020
#endif

#endif
