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
 * \file:   memmgr.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the memory management service.
 *          This memory manager is only available for the stack.  The 
 *          application and plugins do not have access to this 
 *          implementation.  If the application and/or plugin needs 
 *          its memory managment it must find its own implementation.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/conf.h"
#include "a2b/error.h"
#include "a2b/pal.h"
#include "memmgr.h"
#include "stack_priv.h"
#include "timer_priv.h"
#include "a2b/pluginapi.h"
#include "a2b/msgrtr.h"
#include "a2b/msg.h"
#include "jobexec.h"
#include "stackctx.h"
#include "msg_priv.h"
#include "msgrtr_priv.h"
#include "trace_priv.h"
#include "interrupt_priv.h"

#ifdef A2B_FEATURE_MEMORY_MANAGER

/*======================= D E F I N E S ===========================*/


#define A2B_EXPAND_AS_POOL_DESCRIPTORS(a,b,c) {b, sizeof(c)},
#define A2B_EXPAND_AS_ENUMERATION(a,b,c)      A2B_##a,

/*----------------------------------------------------------------------------*/
/** 
 * \ingroup     a2bstack_memmgr_priv
 *  
 * \def         A2B_POOL_TABLE(ENTRY) 
 *  
 * This defines the pool sizes for the complete memory manager. 
 * It defines the block [typedef] size and the number of each to 
 * manage.
 */
/*----------------------------------------------------------------------------*/
#define A2B_POOL_TABLE(ENTRY) \
    /*    Name      Num Blocks                      Block Size          */ \
    /*    ==============================================================*/ \
    ENTRY(POOL_0,   1u,                                  a2b_Stack) \
    ENTRY(POOL_1,   A2B_CONF_MAX_NUM_STACK_CONTEXTS,    a2b_StackContext) \
    ENTRY(POOL_2,   A2B_CONF_MAX_NUM_TIMERS,            a2b_Timer) \
    ENTRY(POOL_3,   A2B_CONF_TRACE_NUM_CHANNELS,        a2b_TraceChannel) \
    ENTRY(POOL_4,   A2B_CONF_MAX_NUM_JOB_EXECUTORS,     a2b_JobExecutor) \
    ENTRY(POOL_5,   A2B_CONF_MAX_NUM_JOB_QUEUES,        a2b_JobQueue) \
    ENTRY(POOL_8,   A2B_CONF_MAX_NUM_MSG_HANDLERS,      a2b_MsgRtr) \
    ENTRY(POOL_9,   A2B_CONF_MSG_POOL_SIZE,             a2b_Msg) \
    ENTRY(POOL_10,  A2B_CONF_MSG_NOTIFICATION_MAX,      a2b_MsgNotifier) \
    ENTRY(POOL_11,  A2B_CONF_MAX_NUM_MASTER_NODES,      a2b_IntrInfo)


/*======================= L O C A L  P R O T O T Y P E S  =========*/

/** Defined just to get the number of entries in the pool table */
enum {
    A2B_POOL_TABLE(A2B_EXPAND_AS_ENUMERATION)
    A2B_NUM_POOLS
};

typedef struct a2b_PoolDescriptor
{
    a2b_UInt32  numBlocks;
    a2b_UInt32  blockSize;
} a2b_PoolDescriptor;


typedef struct a2b_PoolEntry
{
    a2b_Pool    pool;
    a2b_UInt32  blockSize;
} a2b_PoolEntry;

typedef struct a2b_StackHeap
{
    a2b_PoolEntry   pools[A2B_NUM_POOLS];
    a2b_Int32       numPools;
    a2b_Bool        inUse;
} a2b_StackHeap;

static a2b_Int32 a2b_memMgrNormPoolDescriptors(
    a2b_PoolDescriptor* items,
    a2b_Int32           numItems);
static const a2b_PoolDescriptor* a2b_memMgrGetPoolDescriptors(
    a2b_Int32*  num);
/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
* 
*  \b              a2b_memMgrNormPoolDescriptors
*
*  This function sorts the array memory pool descriptors into ascending
*  order by block size and then merges entries with the same block size.
*  Returns the total number of merged descriptors which may be less than
*  the number passed into the function. The normalized descriptors are
*  stored in-place and thus overwrite the original descriptors passed in.
*
*  \param          [in]    items        An array of pool descriptors.
* 
*  \param          [in]    numItems     The number of descriptors in the array.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           The descriptors are sorted in order of ascending block size
*                  with entries with the same block size merged into a single
*                  entry.
*
*  \return         Returns the number of pool descriptors where descriptors with
*                  the same block size have been merged into a single entry.
*                  As a result the number returned may be less than the number
*                  passed into the function.
*
******************************************************************************/
static a2b_Int32
a2b_memMgrNormPoolDescriptors
    (
    a2b_PoolDescriptor* items,
    a2b_Int32           numItems
    )
{
    a2b_Int32   i;
    a2b_Int32   j;
    a2b_Int32   count;
    a2b_PoolDescriptor swap;

    /* Shell sort the pool descriptors by block size */
    for ( i = 1; i < numItems; ++i )
    {
        j = i;
        while ( (j > 0) && (items[j-1].blockSize > items[j].blockSize) )
        {
            swap = items[j];
            items[j] = items[j-1];
            items[j-1] = swap;
            j = j - 1;
        }
    }

    /* Combine descriptors with identical block sizes */
    count = (0 < numItems) ? 1 : 0;
    j = 0;
    for ( i = 1; i < numItems; ++i )
    {
        if ( items[j].blockSize == items[i].blockSize )
        {
            items[j].numBlocks += items[i].numBlocks;
        }
        else
        {
            j++;
            items[j] = items[i];
            count++;
        }
    }

    return count;
} /* a2b_memMgrNormPoolDescriptors */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
* 
*  \b              a2b_memMgrGetPoolDescriptors
*
*  Returns an array of pool descriptors sorted in order of ascending
*  block size and each entry with a unique block size (e.g. no duplicate
*  sizes).
*
*  \param          [in]    num      A pointer to an integer where the length 
*                                   of the descriptor array will be stored.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         An array of pool descriptors.
*
******************************************************************************/
static const a2b_PoolDescriptor*
a2b_memMgrGetPoolDescriptors
    (
    a2b_Int32*  num
    )
{
    /* Global pool of descriptors for tracking the memory pool.*/
    static a2b_PoolDescriptor   gsPoolDescriptors[] =
        { A2B_POOL_TABLE(A2B_EXPAND_AS_POOL_DESCRIPTORS) };

    static a2b_Int32 numDesc = -1;

    if ( 0 > numDesc )
    {
        numDesc = a2b_memMgrNormPoolDescriptors(gsPoolDescriptors,
                                        (a2b_Int32)A2B_ARRAY_SIZE(gsPoolDescriptors));
    }

    if ( A2B_NULL != num )
    {
        *num = numDesc;
    }

    return gsPoolDescriptors;
} /* a2b_memMgrGetPoolDescriptors */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrGetMinHeapSize
*
*  Computes the minimum heap size (in bytes) required to hold all the
*  data needed to be written/read by an instance of the A2B stack.
*  This figure is in part influenced by many parameters specified in
*  a2b/conf.h. It factors in alignment restrictions based on the start of
*  a theoretical heap specified in the arguments to this function.
*
*  \param          [in]    heapStart    The memory address of where the heap 
*                                       would be located. This impacts 
*                                       alignment. If it can be assumed the 
*                                       address will be aligned then A2B_NULL
*                                       can be specified here.
* 
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         The minimum number of bytes required to be allocated by an
*                  A2B stack instance. This is the per *instance* amount.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_UInt32
a2b_memMgrGetMinHeapSize
    (
    a2b_Byte*   heapStart
    )
{
    a2b_UInt32 heapSize = 0u;
    a2b_UInt32 poolSize;
    const a2b_PoolDescriptor* desc;
    a2b_Int32 numDesc;
    a2b_Int32 idx;

    desc = a2b_memMgrGetPoolDescriptors(&numDesc);
    if ( A2B_NULL != desc )
    {
        for ( idx = 0; idx < numDesc; ++idx )
        {
            poolSize = a2b_poolCalcHeapSize(heapStart,
                                            A2B_CONF_MEMORY_ALIGNMENT,
                                            desc[idx].blockSize,
                                            desc[idx].numBlocks);
            heapStart += poolSize;
            heapSize += poolSize;
        }
    }
    return heapSize;
} /* a2b_memMgrGetMinHeapSize */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrInit
*
*  Does any global initialization required by the memory manager service.
* 
*  This routine currently does nothing.
*
*  \param          [in]    ecb      The environment control block.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_HResult
a2b_memMgrInit
    (
    A2B_ECB*    ecb
    )
{
    A2B_UNUSED(ecb);

    return A2B_RESULT_SUCCESS;
} /* a2b_memMgrInit */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrOpen
*
*  Opens a memory managed heap located at the specified address and of the
*  specified size. If the A2B stack's heap cannot be opened and managed
*  at the specified location (perhaps because the size is insufficient)
*  then the returned handle will be A2B_NULL. The managed heap will use
*  memory pools to avoid fragmentation within the managed region.
*
*  \param          [in]    heap         The memory address of the start of
*                                       the managed region.
* 
*  \param          [in]    heapSize     The size of the managed region in bytes.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         The handle to the stack's heap.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_Handle
a2b_memMgrOpen
    (
    a2b_Byte*   heap,
    a2b_UInt32  heapSize
    )
{
    /* An array of all the available memory-pooled heaps */
    static a2b_StackHeap gsStackHeaps[A2B_CONF_MAX_NUM_MASTER_NODES];
    const a2b_PoolDescriptor* desc;
    a2b_StackHeap* stackHeap = A2B_NULL;
    a2b_UInt16 idx;
    a2b_Int32 poolIdx;
    a2b_UInt32 poolSize;
    a2b_UInt32 blockSize;
    a2b_UInt32 numBlocks;
    a2b_Bool allocFailed = A2B_FALSE;
    a2b_UInt32 accumSize = 0u;
    a2b_Int32 nTempVar = 0;
    a2b_Bool bResult= A2B_FALSE;

    for ( idx = 0u; (a2b_UInt16)idx < A2B_ARRAY_SIZE(gsStackHeaps); ++idx )
    {
        if ( !gsStackHeaps[idx].inUse )
        {
            stackHeap = &gsStackHeaps[idx];
            desc = a2b_memMgrGetPoolDescriptors(&stackHeap->numPools);
            for ( poolIdx = 0; poolIdx < stackHeap->numPools; ++poolIdx )
            {
                blockSize = desc[poolIdx].blockSize;
                numBlocks = desc[poolIdx].numBlocks;
                poolSize = a2b_poolCalcHeapSize(heap,
                                                A2B_CONF_MEMORY_ALIGNMENT,
                                                blockSize, numBlocks);
                accumSize += poolSize;
                if (accumSize <= heapSize) 
                {
                    bResult = a2b_poolCreate(&stackHeap->pools[poolIdx].pool, heap,
                            poolSize, A2B_CONF_MEMORY_ALIGNMENT, &blockSize,
                            &numBlocks);
                }
                /* If we failed creating this pool then ... */
                if ((accumSize > heapSize) || (!bResult))
                {
                    nTempVar = poolIdx;
                    /* Destroy any pools that were already allocated */
                    while ( nTempVar >= 1 )
                    {
                        nTempVar--;
                        a2b_poolDestroy(&stackHeap->pools[nTempVar].pool);
                    }
                    stackHeap = A2B_NULL;
                    allocFailed = A2B_TRUE;
                    break;
                }
                stackHeap->pools[poolIdx].blockSize = blockSize;
                heap += blockSize * numBlocks;
            }

            if ( !allocFailed )
            {
                stackHeap->inUse = A2B_TRUE;
            }
            break;
        }
    }

    return (a2b_Handle)stackHeap;
} /* a2b_memMgrOpen */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrMalloc
*
*  Allocates a chunk of memory from the A2B stack instance's memory pool.
*
*  \param          [in]    hnd      The handle of the stack's heap.
* 
*  \param          [in]    size     The amount of memory (in bytes) to allocate 
*                                   from the stack's heap.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         Returns an aligned pointer to memory or A2B_NULL if memory
*                  could not be allocated.
*
******************************************************************************/
A2B_DSO_LOCAL void*
a2b_memMgrMalloc
    (
    a2b_Handle  hnd,
    a2b_UInt32  size
    )
{
    a2b_StackHeap* stackHeap = (a2b_StackHeap*)hnd;
    void* mem = A2B_NULL;
    a2b_Int32 idx;

    if ( A2B_NULL != stackHeap )
    {
        /* Look for the pool with the smallest block size that will
         * satisfy the memory request.
         */
        for ( idx = 0; idx < stackHeap->numPools; ++idx )
        {
            if ( size <= stackHeap->pools[idx].blockSize )
            {
                mem = a2b_poolAlloc(&stackHeap->pools[idx].pool);
                if ( A2B_NULL != mem )
                {
                    break;
                }
            }
        }
    }

    return mem;
} /* a2b_memMgrMalloc */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrFree
*
*  Frees memory (e.g. deallocates) a block of memory back to the
*  stack's managed heap.
*
*  \param          [in]    hnd      The handle of the stack's heap.
* 
*  \param          [in]    p        The pointer to the memory to free.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_memMgrFree
    (
    a2b_Handle  hnd,
    void*       p
    )
{
    a2b_StackHeap* stackHeap = (a2b_StackHeap*)hnd;
    a2b_Int32 idx;

    if ( A2B_NULL != stackHeap )
    {
        /* Look for the pool where the memory request came from
         */
        for ( idx = 0; idx < stackHeap->numPools; ++idx )
        {
            if ( a2b_poolContainsPtr(&stackHeap->pools[idx].pool, p) )
            {
                a2b_poolFree(&stackHeap->pools[idx].pool, p);
                break;
            }
        }
    }
} /* a2b_memMgrFree */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrClose
*
*  Closes the stack's managed heap. All resources associated with the
*  heap are freed.
*
*  \param          [in]    hnd      The handle to the stack's heap.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           Memory should not be requested from a stack heap that has
*                  been closed.
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_HResult
a2b_memMgrClose
    (
    a2b_Handle  hnd
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_MEM_MGR,
                                        A2B_EC_INVALID_PARAMETER);
    a2b_Int32 idx;

    a2b_StackHeap* stackHeap = (a2b_StackHeap*)hnd;
    if ( (A2B_NULL != stackHeap) && (stackHeap->inUse) )
    {
        /* Destroy all the pools managed by the stack's heap */
        for ( idx = 0; idx < stackHeap->numPools; ++idx )
        {
            a2b_poolDestroy(&stackHeap->pools[idx].pool);
        }

        /* The heap is freed so now return it to the system */
        stackHeap->inUse = A2B_FALSE;
        result = A2B_RESULT_SUCCESS;
    }

    return result;
} /* a2b_memMgrClose */


/*!****************************************************************************
*  \ingroup        a2bstack_memmgr_priv
*  \private
* 
*  \b              a2b_memMgrShutdown
*
*  Global shutdown procedure required by the memory manager service.
* 
*  This routine currently does nothing.
*
*  \param          [in]    ecb      The environment control block.
*
*  \pre            #A2B_FEATURE_MEMORY_MANAGER must be enabled
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_HResult
a2b_memMgrShutdown
    (
    A2B_ECB*    ecb
    )
{
    A2B_UNUSED(ecb);
    return A2B_RESULT_SUCCESS;
} /* a2b_memMgrShutdown */

#endif /* A2B_FEATURE_MEMORY_MANAGER */
