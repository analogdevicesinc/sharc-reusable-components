/* ----------------------------------------------------------------------------
 * umm_malloc.h - a memory allocator for embedded systems (microcontrollers)
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 */

#ifndef UMM_MALLOC_H
#define UMM_MALLOC_H

#include <stddef.h>

#include "umm_malloc_heaps.h"

/*
 * The ALIGNED struct is always immediately below the returned pointer
 * from umm_malloc_heap_aligned().
 *
 * Alignment must be a power of 2 (i.e. 2^n).
 *
 */
typedef struct ALIGNED {
    void *basePtr;
} ALIGNED;


/* ------------------------------------------------------------------------ */

void  umm_init( umm_heap_t HEAP_TYPE, void *addr, unsigned int size );
unsigned short int umm_block_size(void);

void *umm_malloc( size_t size );
void *umm_calloc( size_t num, size_t size );
void *umm_realloc( void *ptr, size_t size );
void  umm_free( void *ptr );

void *umm_malloc_heap( umm_heap_t heap, size_t size );
void *umm_calloc_heap( umm_heap_t heap, size_t num, size_t size );
void *umm_realloc_heap( umm_heap_t heap, void *ptr, size_t size );
void  umm_free_heap( umm_heap_t heap, void *ptr );

void *umm_malloc_aligned( size_t size, size_t alignment );
void *umm_malloc_heap_aligned( umm_heap_t heap, size_t size, size_t alignment );
void  umm_free_aligned( void *ptr );
void  umm_free_heap_aligned( umm_heap_t heap, void *ptr );


/* ------------------------------------------------------------------------ */

#endif /* UMM_MALLOC_H */
