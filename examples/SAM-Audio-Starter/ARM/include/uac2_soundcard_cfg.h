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

#ifndef _uac2_cfg_h
#define _uac2_cfg_h

#include "umm_malloc.h"

/* Sizes */
#define UAC2_SYSLOG_DEPTH  8

/* Use std memset */
#define UAC2_MEMSET   memset
#define UAC2_MEMCPY   memcpy

/* Allocate UAC2 memory from the uncached heaps */
#define UAC2_HEAP     UMM_SDRAM_UNCACHED_HEAP
#define UAC2_L2_HEAP  UMM_L2_UNCACHED_HEAP

#define UAC2_MALLOC(x)  \
    umm_malloc_heap(UAC2_HEAP, x)

#define UAC2_REALLOC(x, y) \
    umm_realloc_heap(UAC2_HEAP, x, y)

#define UAC2_CALLOC(x, y)  \
    umm_calloc_heap(UAC2_HEAP, x, y)

#define UAC2_FREE(x)    \
    umm_free_heap(UAC2_HEAP, x)

#define UAC2_L2_MALLOC_ALIGNED(x)  \
    umm_malloc_heap_aligned(UAC2_L2_HEAP, x, sizeof(uint32_t))

#define UAC2_L2_FREE_ALIGNED(x)  \
    umm_free_heap_aligned(UAC2_L2_HEAP, x)

#endif
