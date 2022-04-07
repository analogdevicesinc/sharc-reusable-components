#ifndef __UMM_MALLOC_HEAPS_H__
#define __UMM_MALLOC_HEAPS_H__

/*
 * When defining the heap size(s) below, be aware that UMM_MALLOC
 * has a per-heap size limit of 0x7FFFF blocks of UMM_BLOCK_SIZE bytes
 * each (see umm_malloc.h).
 */

#define UMM_SDRAM_HEAP1_SIZE        ( 16 * 1024 * 1024 )
#define UMM_SDRAM_HEAP2_SIZE        ( 32 * 1024 * 1024 )

#define UMM_HEAP_NAMES      \
{                           \
    "UMM_SDRAM_HEAP1",      \
    "UMM_SDRAM_HEAP2"       \
}

typedef enum {
    UMM_SDRAM_HEAP1 = 0,
    UMM_SDRAM_HEAP2,
    UMM_NUM_HEAPS
} umm_heap_t;

/*
 * This macro which sets the default heap used for generic
 * umm_malloc/umm_free calls
 */
#define UMM_DEFAULT_HEAP  UMM_SDRAM_HEAP1

#endif

