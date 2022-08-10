/*=============================================================================
 *
 * Project: a2bstack
 *
 * Copyright (c) 2015 - Analog Devices Inc. All Rights Reserved.
 * This software is subject to the terms and conditions of the license set 
 * forth in the project LICENSE file. Downloading, reproducing, distributing or 
 * otherwise using the software constitutes acceptance of the license. The 
 * software may not be used except as expressly authorized under the license.
 *
 *=============================================================================
 *
 * \file:   pool.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of a memory pool.
 *
 * The memory pool implementation here is based on the algorithm described in
 * the paper:                                                        <center><b>
 *                Fast Efficient Fixed-Size Memory Pool                     <br>
 *                    No Loops and No Overhead                              <br>
 *                          Ben Kenwright                                   <br>
 *                      Newcastle University                                <br>
 *                    Newcastle, United Kingdom                             <br>
 *                                                                 </b></center>
 * COMPUTATION TOOLS 2012: The Third International Conference on Computational
 * Logics, Algegras, Programming, Tools,and Benchmarking
 *
 * \see www.thinkmind.org/download.php?articleid=computation_tools_2012_1_10_80006
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "pool.h"

#ifdef A2B_FEATURE_MEMORY_MANAGER

/*======================= D E F I N E S ===========================*/

#define A2B_CALC_PADDING(offset, align) \
                (((align) - ((offset) & ((a2b_UIntPtr)((align) -1u)))) & \
                 ((a2b_UIntPtr)((align) - 1u)))

#define A2B_CALC_OFFSET(offset, align)    (((offset) + (align) - 1u) & \
                                            ~((a2b_UIntPtr)((align) - 1u)))

#define A2B_IS_POWER_OF_TWO(value)        (((value) != (a2b_UInt32)0u) && (!((value) & \
                                            ((value) -1u))))

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static a2b_Byte* a2b_poolIndexToAddr(a2b_Pool* pool, a2b_UIntPtr index);
static a2b_UIntPtr a2b_poolAddrToIndex(a2b_Pool* pool, a2b_Byte* addr);

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_poolIndexToAddr
*
*  Converts a block index in the memory pool into it's corresponding
*  memory address.
*
*  \param          [in]    pool     The memory pool in which to convert the 
*                                   index to an address.
* 
*  \param          [in]    index    The memory block index to convert to
*                                   an address.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         A pointer to the memory block referenced by the index.
*
******************************************************************************/
static a2b_Byte*
a2b_poolIndexToAddr
    (
    a2b_Pool*   pool,
    a2b_UIntPtr index
    )
{
    a2b_Byte* addr = A2B_NULL;

    if ( A2B_NULL != pool )
    {
        addr = pool->heapStart + (index * pool->bytesPerBlock);
    }
    return addr;
} /* a2b_poolIndexToAddr */


/*!****************************************************************************
*
*  \b              a2b_poolAddrToIndex
*
*  Converts a memory pool block address into it's corresponding block index
*  within the memory pool.
*
*  \param          [in]    pool     The memory pool in which to convert the 
*                                   address to an index. 
* 
*  \param          [in]    addr     The memory block address to convert to 
*                                   an index.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         The index of the memory block referenced by the address.
*
******************************************************************************/
static a2b_UIntPtr
a2b_poolAddrToIndex
    (
    a2b_Pool*   pool,
    a2b_Byte*   addr
    )
{
    a2b_UIntPtr idx = 0u;

    if ( A2B_NULL != pool )
    {
        idx = (((a2b_UIntPtr)(addr - pool->heapStart)) / pool->bytesPerBlock);
    }
    return idx;
} /* a2b_poolAddrToIndex */


/*!****************************************************************************
*
*  \b              a2b_poolCalcHeapSize
*
*  Calculates the size of the heap required to meet the desired alignment,
*  block size, and number of blocks. If the starting address of the heap
*  can be assumed to be aligned appropriately (such as an address provided
*  by malloc()) then the 'heapStart' parameter can be set to A2B_NULL.
*  Otherwise, a potentially non-aligned heapStart address can be specified
*  and the returned heap size will accommodate mis-matched alignment.
*
*  \param          [in]    heapStart    The expected starting address of the 
*                                       heap. If an aligned address is expected
*                                       to be provided (from a function such
*                                       as malloc()) then A2B_NULL can be
*                                       specified here. 
*
*  \param          [in]    alignment    The memory alignment required of blocks 
*                                       returned from the memory pool. This 
*                                       *must* be a power of two. 
*
*  \param          [in]    blockSize    The minimum desired block size (in 
*                                       bytes) of the memory pool. 
*
*  \param          [in]    numBlocks    The minimum number of blocks that 
*                                       should be available from the pool.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         The size of the heap (in bytes) starting at the specified 
*                  heap starting address required to hold the specified number 
*                  of blocks with the minimum block size (in bytes) and 
*                  alignment.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_UInt32
a2b_poolCalcHeapSize
    (
    a2b_Byte*   heapStart,
    a2b_UInt32  alignment,
    a2b_UInt32  blockSize,
    a2b_UInt32  numBlocks
    )
{
    a2b_UInt32 heapSize = 0u;
    a2b_UIntPtr newStart;
    a2b_UIntPtr heapEnd;
    a2b_UInt32    paddedBlkSize;

    if ( (A2B_IS_POWER_OF_TWO(alignment)) && (blockSize > (a2b_UInt32)0) && (numBlocks > (a2b_UInt32)0) )
    {
        newStart = (a2b_UIntPtr)A2B_CALC_OFFSET((a2b_UIntPtr)heapStart,
                                                alignment);
        if ( blockSize < sizeof(a2b_UIntPtr) )
        {
            blockSize = sizeof(a2b_UIntPtr);
        }
        paddedBlkSize = (a2b_UInt32)A2B_CALC_OFFSET((a2b_UIntPtr)blockSize, alignment);
        heapEnd = (a2b_UIntPtr)(newStart + (a2b_UIntPtr)((a2b_UIntPtr)paddedBlkSize * (a2b_UIntPtr)numBlocks));
        heapSize = (a2b_UInt32)(heapEnd - (a2b_UIntPtr)heapStart);
    }
    return heapSize;
} /* a2b_poolCalcHeapSize */


/*!****************************************************************************
*
*  \b              a2b_poolCreate
*
*  Creates a memory pool at the specified heap address of the designated
*  size, alignment, block size, and number of blocks. The block size and
*  number of blocks are a request which, due to alignment restrictions,
*  may have to be altered. The minimum block size is guaranteed to be met
*  but the actually number of blocks that will fit in the designated heap
*  region may be reduced due to alignment issues.
*
*  \param          [in]    pool         The pool data structure that will be 
*                                       initialized to support subsequent
*                                       allocations/deallocations.
* 
*  \param          [in]    heapStart    The starting address of the heap. If 
*                                       the heap address is not aligned then 
*                                       the actual (internal) start address 
*                                       of the pool will be offset to an 
*                                       aligned address.
*
*  \param          [in]    heapSize     The size of the heap (in bytes) 
*                                       starting at heapStart. 
*
*  \param          [in]    alignment    The memory alignment required of blocks 
*                                       returned from the memory pool. This 
*                                       *must* be a power of two.
*
*  \param          [in,out] blockSize   The minimum desired block size (in 
*                                       bytes) of the memory pool. The actual
*                                       block size may be larger due to
*                                       alignment restrictions. On input this
*                                       should be set to the requested block
*                                       size. On return this parameter will be
*                                       set to the size of the actual blocks
*                                       which may be larger than the 
*                                       requested minimum.
*
*  \param          [in,out] numBlocks   The requested number of blocks that 
*                                       should be made available from the pool.
*                                       The actual number of blocks that can
*                                       fit within the heap, due to alignment
*                                       issues, may be less. When the function
*                                       returns this variable will be updated
*                                       with the *actual* number of blocks
*                                       that could be reserved.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         Returns A2B_TRUE if the memory pool could be created or 
*                  A2B_FALSE otherwise.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_Bool
a2b_poolCreate
    (
    a2b_Pool*   pool,
    a2b_Byte*   heapStart,
    a2b_UInt32  heapSize,
    a2b_UInt32  alignment,
    a2b_UInt32* blockSize,   /* [IN] req blk size [OUT] actual blk size */
    a2b_UInt32* numBlocks    /* [IN] req num blks [OUT] actual num blks */
    )
{
    a2b_Bool created = A2B_FALSE;
    a2b_UInt32 padding;
    a2b_UIntPtr nTempVar;

    if ( (A2B_NULL != pool) && (A2B_NULL != blockSize) &&
        (A2B_NULL != numBlocks) && (A2B_IS_POWER_OF_TWO(alignment)) )
    {
        padding = (a2b_UInt32)A2B_CALC_PADDING((a2b_UIntPtr)heapStart,
                                                (a2b_UIntPtr)alignment);
        nTempVar = (a2b_UIntPtr)(A2B_CALC_OFFSET(
                (a2b_UIntPtr)heapStart, (a2b_UIntPtr)alignment));
        pool->heapStart = (a2b_Byte*) nTempVar;

        /* The blocksize must at least be large enough to hold the index to
         * the next free block.
         */
        if ( *blockSize < sizeof(a2b_UIntPtr) )
        {
            *blockSize = sizeof(a2b_UIntPtr);
        }

        pool->bytesPerBlock = (a2b_UInt32)A2B_CALC_OFFSET((a2b_UIntPtr)*blockSize, alignment);
        /*if ( 0 < pool->bytesPerBlock ) TODO Tessy fix */
        {
            pool->numBlocks = (heapSize - padding) / pool->bytesPerBlock;
        }
        /*else
        {
            pool->numBlocks = 0;
        }*/

        pool->maxAlloc = 0u;
        pool->nextFree = pool->heapStart;
        pool->numFree = pool->numBlocks;
        pool->numInitialized = 0u;

        /* Update the caller with the actual values */
        *blockSize = pool->bytesPerBlock;
        *numBlocks = pool->numBlocks;

        created = A2B_TRUE;
    }

    return created;
} /* a2b_poolCreate */


/*!****************************************************************************
*
*  \b              a2b_poolDestroy
*
*  Destroys the memory pool and releases and resources. After the pool
*  is destroyed the underlying heap can be deallocated by the user.
*
*  \param          [in]    pool     The memory pool to destroy.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           The memory pool is destroyed. The underlying heap can be
*                  deallocated or re-used for another purpose.
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_poolDestroy
    (
    a2b_Pool*   pool
    )
{
    if ( A2B_NULL != pool )
    {
        pool->bytesPerBlock  = 0u;
        pool->heapStart      = A2B_NULL;
        pool->maxAlloc       = 0u;
        pool->nextFree       = A2B_NULL;
        pool->numBlocks      = 0u;
        pool->numFree        = 0u;
        pool->numInitialized = 0u;
    }
} /* a2b_poolDestroy */


/*!****************************************************************************
*
*  \b              a2b_poolAlloc
*
*  Allocates a block of memory from the pool meeting the alignment
*  requirements set for when the pool was created. If no blocks are
*  available then A2B_NULL is returned.
*
*  \param          [in]    pool     The memory pool from where to allocate
*                                   the block.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         A pointer to an aligned memory block or A2B_NULL if no blocks
*                  are available.
*
******************************************************************************/
A2B_DSO_LOCAL void*
a2b_poolAlloc
    (
    a2b_Pool*    pool
    )
{
    a2b_Byte* blk = A2B_NULL;
    a2b_UIntPtr* p;

    if ( A2B_NULL != pool )
    {
        if ( pool->numInitialized < pool->numBlocks )
        {
            p = (a2b_UIntPtr*)a2b_poolIndexToAddr(pool, pool->numInitialized);
            *p = (a2b_UIntPtr)((a2b_UIntPtr)pool->numInitialized + 1u);
            pool->numInitialized += 1u;
        }

        if ( pool->numFree > 0u )
        {
            blk = pool->nextFree;
            pool->numFree -= 1u;

            /* Track the maximum number of allocations */
            if ( pool->numBlocks - pool->numFree > pool->maxAlloc )
            {
                pool->maxAlloc = pool->numBlocks - pool->numFree;
            }

            if ( pool->numFree != 0u )
            {
                pool->nextFree = (a2b_Byte*)a2b_poolIndexToAddr(pool,
                                        *((a2b_UIntPtr*)pool->nextFree));
            }
            else
            {
                pool->nextFree = A2B_NULL;
            }
        }
    }

    return (void*)blk;
} /* a2b_poolAlloc */


/*!****************************************************************************
*
*  \b              a2b_poolFree
*
*  Returns the specified memory block back to the memory pool. It becomes
*  available to be re-used.
*
*  \param          [in]    pool     The memory pool where the memory block
*                                   will be returned.
* 
*  \param          [in]    p        Pointer to the memory block returned
*                                   in a2b_poolAlloc
* 
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
* 
*  \pre            The memory block pointer address falls within the range of
*                  memory managed by the pool.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_poolFree
    (
    a2b_Pool*    pool,
    void*        p
    )
{
    if ( A2B_NULL != pool )
    {
        if ( a2b_poolContainsPtr(pool, p) )
        {
            if ( pool->nextFree != A2B_NULL )
            {
                (*(a2b_UIntPtr*)p) = a2b_poolAddrToIndex(pool, pool->nextFree);
                pool->nextFree = (a2b_Byte*)p;
            }
            else
            {
                *((a2b_UIntPtr*)p) = pool->numBlocks;
                pool->nextFree = (a2b_Byte*)p;
            }
            pool->numFree += 1u;
        }
    }
} /* a2b_poolFree */


/*!****************************************************************************
*
*  \b              a2b_poolMaxAlloc
*
*  Returns the maximum number of blocks that were allocated from the pool
*  during its existence.
*
*  \param          [in]    pool     The memory pool to get the maximum
*                                   number of allocations.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         The maximum number of blocks that were allocated from the 
*                  pool at any given time. This is the upper threshold on 
*                  allocations from the pool.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_UInt32
a2b_poolMaxAlloc
    (
    const a2b_Pool* pool
    )
{
    a2b_UInt32 maxAlloc = 0u;

    if ( A2B_NULL != pool )
    {
        maxAlloc = pool->maxAlloc;
    }

    return maxAlloc;
} /* a2b_poolMaxAlloc */


/*!****************************************************************************
*
*  \b              a2b_poolContainsPtr
*
*  Returns A2B_TRUE if the specified pointer falls within the address range
*  managed by the pool or A2B_FALSE otherwise. This does *not* imply it is a
*  valid block pointer (e.g. that it points to the start of a block) or that
*  it has been allocated (or is free).
* 
*  \param          [in]    pool     The memory pool to check for "p"
* 
*  \param          [in]    p        The memory pointer to check to see if it 
*                                   falls within than range of memory managed
*                                   by the pool.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled
*
*  \post           None
*
*  \return         Returns A2B_TRUE if the pointer falls within the range 
*                  of memory managed by the pool or A2B_FALSE otherwise.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_Bool
a2b_poolContainsPtr
    (
    const a2b_Pool* pool,
    void*           p
    )
{
    a2b_Bool contained = A2B_FALSE;

    if ( A2B_NULL != pool )
    {
        if ( ((a2b_Byte*)p >= pool->heapStart) &&
            ((a2b_Byte*)p < (pool->heapStart +
            (pool->numBlocks * pool->bytesPerBlock))) )
        {
            contained = A2B_TRUE;
        }
    }
    return contained;
} /* a2b_poolContainsPtr */

#endif  /* A2B_FEATURE_MEMORY_MANAGER */
