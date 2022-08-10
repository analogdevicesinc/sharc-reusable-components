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

#ifndef _sae_alloc_h
#define _sae_alloc_h

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

int sae_heapInit(void *memory, size_t size);
void *sae_safeMalloc(size_t size);
void sae_safeFree(void *mem);
void *sae_malloc(size_t size);
void sae_free(void *mem);
bool sae_safeHeapInfo(SAE_HEAP_INFO *heapInfo);

#endif
