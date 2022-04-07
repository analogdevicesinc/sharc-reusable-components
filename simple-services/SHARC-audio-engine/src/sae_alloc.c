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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Alignment == Book-keeping == 2 * sizeof(uint32_t)
 * Do not modify these values without further testing.
 *
 */
#define ALIGNMENT       (8)
#define BOOK_KEEPING    (2*sizeof(uint32_t))

#define BSIZE(size)     (size + BOOK_KEEPING)

#define GET(ptr)        (*(uint32_t *)(ptr))
#define SET(ptr,x)      (*(uint32_t *)(ptr)=(x))

#define GET_SIZE(ptr)   (GET(ptr) & ~(ALIGNMENT-1))
#define GET_FREE(ptr)   ((GET(ptr) & 0x1) == 0)

#define BLK_PREV(ptr)   ((uint32_t *)(ptr)-2)
#define BLK_START(ptr)  ((uint32_t *)(ptr)-1)
#define BLK_END(ptr)    ((uint32_t *)(ptr+BLK_SIZE(ptr))-2)

#define BLK_FREE(ptr)   (GET_FREE(BLK_START(ptr)))
#define BLK_SIZE(ptr)   (GET_SIZE(BLK_START(ptr)))

#define NEXT(ptr)       (ptr+BLK_SIZE(ptr))
#define PREV(ptr)       (ptr-GET_SIZE(BLK_PREV(ptr)))

#define MARK_BLK_START(ptr, size, alloc) \
                        SET(BLK_START(ptr), (size) | (alloc))
#define MARK_BLK_END(ptr, size, alloc)  \
                        SET(BLK_END(ptr), (size) | (alloc))
#define MARK_BLK(ptr, size, alloc) \
                        MARK_BLK_START(ptr,size,alloc); \
                        MARK_BLK_END(ptr,size,alloc);

/* Aligns or resizes up/dn to 'align' which must be a power of 2 */
#define ALIGN_UP(size, align) (((size) + ((align)-1)) & ~((align)-1))
#define ALIGN_DN(size, align) ((size) & ~((align)-1))

static char *sae_heap_start;

int sae_alloc_init(void *memory, size_t size)
{
    char *start, *end;

    /* Align incoming memory pointer upward */
    start = (char *)ALIGN_UP((uintptr_t)memory, ALIGNMENT);

    /* Reserve space at the top for specialized block markers */
    start += ALIGN_UP(2*ALIGNMENT, ALIGNMENT);

    /* Size of 0 indicates that this is not the IPC master and
     * that only the heap start address should be stored */
    if (size == 0) {
        sae_heap_start = start;
        return 0;
    }

    /* Align top downward, but point to end of aligned section */
    end = (char *)ALIGN_DN((uintptr_t)memory + size, ALIGNMENT);

    /* Put some specialized block markers at the front to simplify coalescing
     * and heap consistency checks.
     */
    SET(BLK_PREV(start), ALIGNMENT | 0x1);
    SET(BLK_START(PREV(start)), ALIGNMENT | 0x1);

    /* Mark a zero length allocated block at the end to signal end of heap */
    MARK_BLK_START(end, 0, 1);

    /* Calculate the new size */
    size = (uintptr_t)end - (uintptr_t)start;

    /* Allocate a single free block */
    MARK_BLK(start, size, 0);

    sae_heap_start = start;

    return 0;
}

void *sae_alloc_malloc(size_t size)
{
    char *ptr = NULL;
    char *next = NULL;
    size_t osize;

    if (size > 0) {
        size = ALIGN_UP(BSIZE(size), ALIGNMENT);
        ptr = sae_heap_start;
        while (BLK_SIZE(ptr) > 0) {
            if (BLK_FREE(ptr) && (BLK_SIZE(ptr) >= size)) {
                break;
            }
            ptr = NEXT(ptr);
        }
        if (BLK_SIZE(ptr)) {
            if (BLK_SIZE(ptr) >= size + ALIGN_UP(ALIGNMENT+BOOK_KEEPING, ALIGNMENT)) {
                osize = BLK_SIZE(ptr);
                MARK_BLK(ptr, size, 1);
                next = ptr + BLK_SIZE(ptr);
                size = osize - BLK_SIZE(ptr);
                MARK_BLK(next, size, 0);
            } else {
                MARK_BLK(ptr, BLK_SIZE(ptr), 1);
            }
        } else {
            ptr = NULL;
        }
    }

    return(ptr);
}

static void *sae_alloc_coalesce(char *ptr)
{
    size_t size = BLK_SIZE(ptr);
    char *prev = PREV(ptr);
    bool pfree = BLK_FREE(prev);
    char *next = NEXT(ptr);
    bool nfree = BLK_FREE(next);

    if (!pfree && !nfree) {
        return(NULL);
    }
    if (!pfree && nfree) {
        size += BLK_SIZE(next);
        MARK_BLK(ptr, size, 0);
    } else if (pfree && !nfree) {
        size += BLK_SIZE(prev);
        MARK_BLK(prev, size, 0);
    } else {
        size += BLK_SIZE(prev) + BLK_SIZE(next);
        MARK_BLK(prev, size, 0);
    }
    return(ptr);
}


void sae_alloc_free(void *ptr)
{
    MARK_BLK((char *)ptr, BLK_SIZE((char *)ptr), 0);
    sae_alloc_coalesce(ptr);
}

bool sae_alloc_checkheap(void)
{
    char *ptr = sae_heap_start;
    char *prev = PREV(sae_heap_start);

    if ((BLK_SIZE(prev) != ALIGNMENT) || BLK_FREE(prev)) {
        return(false);
    }
    while (BLK_SIZE(ptr) > 0) {
        if ((uintptr_t)ptr % ALIGNMENT) {
            return(false);
        }
        if (GET(BLK_START(ptr)) != GET(BLK_END(ptr))) {
            return(false);
        }
        ptr = NEXT(ptr);
    }
    if ((BLK_SIZE(ptr) != 0) || BLK_FREE(ptr)) {
        return(false);
    }
    return(true);
}

#include "sae_util.h"

bool sae_safeHeapInfo(SAE_HEAP_INFO *heapInfo)
{
    char *ptr = sae_heap_start;
    size_t size;
    bool ok;

    if (heapInfo == NULL) {
        return(false);
    }
    sae_lockIpc();
    ok = sae_alloc_checkheap();
    if (ok) {
        memset(heapInfo, 0, sizeof(*heapInfo));
        while (BLK_SIZE(ptr) > 0) {
            size = BLK_SIZE(ptr);
            if (BLK_FREE(ptr)) {
                heapInfo->freeBlocks++;
                heapInfo->freeSize += size;
                if (size > heapInfo->maxContigFreeSize) {
                    heapInfo->maxContigFreeSize = size;
                }
            } else {
                heapInfo->allocBlocks++;
                heapInfo->allocSize += size;
            }
            heapInfo->totalBlocks++;
            ptr = NEXT(ptr);
        }
    }
    sae_unLockIpc();
    return(ok);
}

bool sae_safeHeapCheck(void)
{
    bool ok;
    sae_lockIpc();
    ok = sae_alloc_checkheap();
    sae_unLockIpc();
    return(ok);
}

void *sae_safeMalloc(size_t size)
{
    void *mem;

    sae_lockIpc();
    mem = sae_alloc_malloc(size);
    sae_unLockIpc();

    return(mem);
}

void sae_safeFree(void *mem)
{
    sae_lockIpc();
    sae_alloc_free(mem);
    sae_unLockIpc();
}

void *sae_malloc(size_t size)
{
    return(sae_alloc_malloc(size));
}

void sae_free(void *mem)
{
    sae_alloc_free(mem);
}

int sae_heapInit(void *memory, size_t size)
{
    return(sae_alloc_init(memory, size));
}
