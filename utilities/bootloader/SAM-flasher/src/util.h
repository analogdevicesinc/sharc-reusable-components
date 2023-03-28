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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include <sys/types.h>

#ifndef _WIN32
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef int clockid_t;
int clock_gettime(clockid_t clk_id, struct timespec *tp);
#endif

uint64_t clock_elapsed(struct timespec *start, struct timespec *end);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_H */
