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
