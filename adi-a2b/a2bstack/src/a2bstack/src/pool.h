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
 * \file:   pool.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definition of a fixed-size memory pool service.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_pool              Memory Pool Module 
 *  
 * The definition of a fixed-size memory pool service.
 *  
 * The memory pool implementation here is based on the algorithm described in
 * the paper:                                                        <center><b>
 *                  Fast Efficient Fixed-Size Memory Pool                   <br>
 *                      No Loops and No Overhead                            <br>
 *                            Ben Kenwright                                 <br>
 *                        Newcastle University                              <br>
 *                      Newcastle, United Kingdom                           <br>
 *                                                                 </b></center>
 * COMPUTATION TOOLS 2012: The Third International Conference on Computational
 * Logics, Algegras, Programming, Tools,and Benchmarking
 *
 * \see http://www.thinkmind.org/download.php?articleid=computation_tools_2012_1_10_80006 
 *  
 * \{ */
/** 
 * \defgroup a2bstack_pool_priv         \<Private\>
 * \private 
 *
 * This defines the memory pool API's that are private to the stack. 
 *  
 * \{ */
/*============================================================================*/

#ifndef A2B_POOL_H_
#define A2B_POOL_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/features.h"

#ifdef A2B_FEATURE_MEMORY_MANAGER

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

typedef struct a2b_Pool {
    /** The total number of blocks being managed in the pool */
    a2b_UInt32  numBlocks;

    /** The number of bytes per block size */
    a2b_UInt32  bytesPerBlock;

    /** Aligned pointer to the start of the memory heap */
    a2b_Byte*   heapStart;

    /** The number of free blocks */
    a2b_UInt32  numFree;

    /** The number of initialized blocks */
    a2b_UInt32  numInitialized;

    /** Pointer to the next free block */
    a2b_Byte*   nextFree;

    /** Maximum number of blocks ever allocated */
    a2b_UInt32  maxAlloc;
} a2b_Pool;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT A2B_DSO_LOCAL a2b_UInt32 a2b_poolCalcHeapSize(
                                a2b_Byte* heapStart,
                                a2b_UInt32 alignment,
                                a2b_UInt32 blockSize,
                                a2b_UInt32 numBlocks);

A2B_EXPORT A2B_DSO_LOCAL a2b_Bool a2b_poolCreate(
                            a2b_Pool* pool, a2b_Byte* heapStart,
                            a2b_UInt32 heapSize,
                            a2b_UInt32 alignment,
                            a2b_UInt32* blockSize,
                            a2b_UInt32* numBlocks);

A2B_EXPORT A2B_DSO_LOCAL void a2b_poolDestroy(a2b_Pool* pool);

A2B_EXPORT A2B_DSO_LOCAL void* a2b_poolAlloc(a2b_Pool* pool);

A2B_EXPORT A2B_DSO_LOCAL void a2b_poolFree(a2b_Pool* pool, void* p);

A2B_EXPORT A2B_DSO_LOCAL a2b_UInt32 a2b_poolMaxAlloc(
                                        const a2b_Pool* pool);

A2B_EXPORT A2B_DSO_LOCAL a2b_Bool a2b_poolContainsPtr(
                                    const a2b_Pool* pool, void* p);

A2B_END_DECLS

/*======================= D A T A =================================*/

#endif /* A2B_FEATURE_MEMORY_MANAGER */

/** \} -- a2bstack_pool_priv */

/** \} -- a2bstack_pool */


#endif /* A2B_POOL_H_ */
