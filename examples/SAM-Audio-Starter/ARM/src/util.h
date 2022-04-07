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

#ifndef _util_h
#define _util_h

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

uint32_t getTimeStamp(void);
void delay(unsigned ms);
uint32_t elapsedTimeMs(uint32_t elapsed);

void copyAndConvert(
    void *src, unsigned srcWordSize, unsigned srcChannels,
    void *dst, unsigned dstWordSize, unsigned dstChannels,
    unsigned frames, bool zero
);

time_t util_time(time_t *tloc);

#endif
