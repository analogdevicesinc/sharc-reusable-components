# oss-services/umm_malloc

## Overview

This service is an enhanced version of the open-source [umm_malloc](https://github.com/rhempel/umm_malloc) project.  SAM specific enhancements include

 - Support for multiple heaps with individual locks
 - FreeRTOS and bare-metal friendly
 - Arbitrary aligned memory allocations
 - 16Mb maximum heap size
 - Does not disable global interrupts while managing the heap
 - Detailed run-time heap info

## Required components

- dbglog

## Recommended components

- None

## Integrate the source

- Copy the `src` directory contents into an appropriate place in the host project
- Copy the `inc` directory contents into a project include directory.  The `inc`directory contains headers with tunable parameters.

## Configure

- Modify `umm_malloc_heaps.h` to define the heaps required for the project.
- Modify `umm_malloc.h` as required to enable or disable additional features as necessary.  This file is self-documented.

## Run

### Example initialization code

```C
#include "umm_malloc.h"

#pragma section ("l3_data", NO_INIT)
static uint8_t umm_sdram_heap1[UMM_SDRAM_HEAP1_SIZE];

#pragma section ("l3_data", NO_INIT)
static uint8_t umm_sdram_heap2[UMM_SDRAM_HEAP2_SIZE];

void umm_heap_init(void)
{
    /* Initialize the the umm_malloc managed SDRAM heaps. */
    umm_init(UMM_SDRAM_HEAP1, umm_sdram_heap1, UMM_SDRAM_HEAP1_SIZE);
    umm_init(UMM_SDRAM_HEAP2, umm_sdram_heap2, UMM_SDRAM_HEAP2_SIZE);
}
```

### Example malloc/free code

```C
#include "umm_malloc.h"

#define MALLOC_SIZE   (128)

void umm_malloc_examples(void)
{
    void *ptr;

    /* Allocate / free from the UMM_DEFAULT_HEAP */
    ptr = umm_malloc(MALLOC_SIZE);
    umm_free(ptr);

    ptr = umm_calloc(1, MALLOC_SIZE);
    ptr = umm_realloc(ptr, 2 * MALLOC_SIZE);
    umm_free(ptr);

    /* Allocate / free from a specific heap */
    ptr = umm_malloc_heap(UMM_SDRAM_HEAP2, MALLOC_SIZE);
    umm_free_heap(UMM_SDRAM_HEAP2, ptr);

    ptr = umm_calloc_heap(UMM_SDRAM_HEAP1, 1, MALLOC_SIZE);
    ptr = umm_realloc_heap(UMM_SDRAM_HEAP1, ptr, 2 * MALLOC_SIZE);
    umm_free_heap(UMM_SDRAM_HEAP1, ptr);

    /* Allocate / free a 32 byte aligned memory block from the default heap */
    ptr = umm_malloc_aligned(MALLOC_SIZE, 32);
    umm_free_aligned(UMM_SDRAM_HEAP2, ptr);

    /* Allocate / free a 16 byte aligned memory block from a specific heap */
    ptr = umm_malloc_heap_aligned(UMM_SDRAM_HEAP2, MALLOC_SIZE, 16);
    umm_free_heap_aligned(UMM_SDRAM_HEAP2, UMM_SDRAM_HEAP2, ptr);
}
```

## Info

- All available umm_malloc debug options are enabled by default in `umm_malloc_cfg.h`.
- Never mix calls between various "classes" of umm_malloc APIs.  I.e. do not use `umm_malloc_free_heap()` to free a region of memory allocated by `umm_malloc()`.
- Never mix heaps.  I.e. do not allocate from one heap and `umm_realloc_heap()` the same pointer to a different heap.
- Under FreeRTOS, realloc() requires recursive locks so heaps are protected by a recursive semaphore.  The 'configUSE_RECURSIVE_MUTEXES' feature must be enabled in 'FreeRTOSConfig.h'.
