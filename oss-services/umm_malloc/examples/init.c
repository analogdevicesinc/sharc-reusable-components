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
