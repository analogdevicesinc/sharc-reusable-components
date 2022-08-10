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

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string.h>
#include <stdio.h>
#include "util.h"

#define GT_BILLION (1E9L)

uint64_t clock_elapsed(struct timespec *start, struct timespec *end)
{
    uint64_t elapsed;
    struct timespec zero = { 0, 0 };

    if (!start) {
        start = &zero;
    }
    elapsed = GT_BILLION * (end->tv_sec - start->tv_sec) + end->tv_nsec - start->tv_nsec;
    return(elapsed);
}

#ifdef _WIN32
int clock_gettime(int dummy, struct timespec *ct)
{
    static BOOL g_first_time = 1;
    static LARGE_INTEGER g_counts_per_sec;
    LARGE_INTEGER count;
    if (g_first_time) {
        g_first_time = 0;
        if (0 == QueryPerformanceFrequency(&g_counts_per_sec)) {
            g_counts_per_sec.QuadPart = 0;
        }
    }
    if ((NULL == ct) || (g_counts_per_sec.QuadPart <= 0) ||
            (0 == QueryPerformanceCounter(&count))) {
        return -1;
    }
    ct->tv_sec = count.QuadPart / g_counts_per_sec.QuadPart;
    ct->tv_nsec = ((count.QuadPart % g_counts_per_sec.QuadPart) * GT_BILLION) / g_counts_per_sec.QuadPart;
    return 0;
}
#endif
