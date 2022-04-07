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
 * \file:   stack.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Implementation of the generic A2B stack routines.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/
#include "a2b/ctypes.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/trace.h"
#include "a2b/seqchart.h"
#include "a2b/timer.h"
#include "a2b/util.h"
#include "a2b/pluginapi.h"
#include "a2b/msgrtr.h"
#include "a2b/stackctxmailbox.h"
#include "trace_priv.h"
#include "timer_priv.h"
#include "stack_priv.h"
#include "stackctx.h"
#include "msgrtr_priv.h"
#include "jobexec.h"
#include "memmgr.h"
#include "interrupt_priv.h"

/*======================= D E F I N E S ===========================*/
#define A2B_INITIALIZED_NONE        (a2b_UInt32)(0u)
#define A2B_INITIALIZED_LOG         ((a2b_UInt32)1u << (a2b_UInt32)0u)
#define A2B_INITIALIZED_TIMER       ((a2b_UInt32)1u << (a2b_UInt32)1u)
#define A2B_INITIALIZED_I2C         ((a2b_UInt32)1u << (a2b_UInt32)2u)
#define A2B_INITIALIZED_AUDIO       ((a2b_UInt32)1u << (a2b_UInt32)3u)
#define A2B_INITIALIZED_PLUGINS     ((a2b_UInt32)1u << (a2b_UInt32)4u)
#define A2B_INITIALIZED_JOBEXEC     ((a2b_UInt32)1u << (a2b_UInt32)5u)
#define A2B_INITIALIZED_TRACE       ((a2b_UInt32)1u << (a2b_UInt32)6u)
#define A2B_INITIALIZED_MEMMGR      ((a2b_UInt32)1u << (a2b_UInt32)7u)
#define A2B_INITIALIZED_MSGRTR      ((a2b_UInt32)1u << (a2b_UInt32)8u)
#define A2B_INITIALIZED_ALL         ((a2b_UInt32)0xFFFFu)


/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_stackResetI2cLastMode
*
*  Resets the I2C last access "mode" to default (uninitialized) values. These
*  values will cause the I2C read/write routines to re-initialize the A2B
*  I2C access registers and skip their (assumed) cached values.
*
*  \param          [in]    lastMode     The last access mode parameters to
*                                       be reset.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_stackResetI2cLastMode
    (
    a2b_I2cLastMode*    lastMode
    )
{
    if ( A2B_NULL != lastMode )
    {
        lastMode->access = A2B_I2C_ACCESS_UNKNOWN;
        lastMode->nodeAddr = A2B_NODEADDR_NOTUSED;
        lastMode->chipAddr = A2B_INVALID_7BIT_I2C_ADDRESS;
        lastMode->broadcast = A2B_FALSE;
    }
} /* a2b_stackResetI2cLastMode */


/*!****************************************************************************
*
*  \b              a2b_stackDestroyHeap
*
*  Private function to destroy the memory heap being used by the stack
*  instance. Once this is called there ceases to be an A2B stack instance
*  and it (and the context) should no longer be accessed.
*
*  \param          [in]    hnd      Handle to the underlying heap to deallocate.
* 
*  \param          [in]    pal      Platform abstraction layer.
* 
*  \param          [in]    ecb      Environment control block.
*
*  \pre            None
*
*  \post           Neither the A2B stack or APP context should be accessed after
*                  this function is called.
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_stackDestroyHeap
    (
    a2b_Handle*         hnd,
    const a2b_StackPal* pal,
    A2B_ECB*            ecb
    )
{
    if (  (A2B_NULL != pal) && (A2B_NULL != ecb) )
    {
        /* Now close the managed heap */
        if ( (A2B_NULL != hnd) && (A2B_NULL != pal->memMgrClose) )
        {
            pal->memMgrClose(hnd);
        }

        if ( A2B_NULL != pal->memMgrShutdown )
        {
            pal->memMgrShutdown(ecb);
        }
    }
} /* a2b_stackDestroyHeap */


/*!****************************************************************************
*
*  \b              a2b_stackDestroy
*
*  Private function to destroy a (possibly) partially created stack. This
*  is called either by #a2b_stackAlloc() or #a2b_stackFree() to release the
*  resources allocated for a stack. It is destroyed irrespective of the
*  stack's current reference count.
*
*  \param          [in]    stk          Possibly contains partially created stk. 
*                                       May not be fully initialized.
* 
*  \param          [in]    destroyHeap  Set to A2B_TRUE if the underlying heap 
*                                       should be destroyed after freeing the 
*                                       stack. If A2B_FALSE the heap will
*                                       remain intact.
*
*  \pre            Either a fully or partially initialized A2B stack instance
*                  can be passed to this function. Every attempt will be made
*                  to safely shutdown and de-allocate resources used by the
*                  stack.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_stackDestroy
    (
    a2b_Stack*  stk,
    a2b_Bool    destroyHeap
    )
{
    A2B_ECB* ecb;
    a2b_StackPal pal;
    a2b_Handle hnd;
    a2b_UInt16 idx;
    a2b_Timer* timer;

    if ( A2B_NULL != stk )
    {
        /* Ensure no interrupts are processed if polling */
        if ( A2B_NULL != stk->intrInfo )
        {
            a2b_intrDestroy(stk);
        }

        /*
         * Now give the platform an opportunity to gracefully shutdown
         * all the platform specific subsystems.
         */
        if ( stk->initMask & A2B_INITIALIZED_TIMER )
        {
            if ( A2B_NULL != stk->pal.timerShutdown )
            {
                stk->pal.timerShutdown(stk->ecb);
            }
        }

        if ( stk->initMask & A2B_INITIALIZED_AUDIO )
        {
            if ( A2B_NULL != stk->audioHnd )
            {
                if ( A2B_NULL != stk->pal.audioClose )
                {
                    stk->pal.audioClose(stk->audioHnd);
                }
            }

            if ( A2B_NULL != stk->pal.audioShutdown )
            {
                stk->pal.audioShutdown(stk->ecb);
            }
        }

        /* Free all the mailboxes of the plugins BEFORE freeing the
         * plugin contexts themselves. Freeing the mailboxes may
         * generate message traces which assume all plugin contexts
         * are available. If we free the mailboxes associated with each
         * plugin individually (instead of beforehand) we may encounter
         * a situation where one of the messages being canceled references
         * a plugin that is no longer available. This is why the mailboxes
         * are freed first.
         */
        for ( idx = 0u; idx < (a2b_UInt16)A2B_ARRAY_SIZE(stk->pluginList); idx++ )
        {
            if ( stk->pluginList[idx] )
            {
                a2b_stackCtxMailboxFreeAll(stk->pluginList[idx]);
            }
        }

        /* Release and possibly close all stack context */
        for (idx = 0u; idx < (a2b_UInt16)A2B_ARRAY_SIZE(stk->pluginList); idx++)
        {
            if (stk->pluginList[idx])
            {
                a2b_stackContextFree( stk->pluginList[idx] );
            }
        }

        if ( stk->initMask & A2B_INITIALIZED_MSGRTR )
        {
            a2b_msgRtrFree( stk->msgRtr );
        }

        if ( stk->initMask & A2B_INITIALIZED_PLUGINS )
        {
            if ( A2B_NULL != stk->pal.pluginsUnload )
            {
                stk->pal.pluginsUnload(stk->plugins,
                                       stk->numPlugins,
                                       stk->ecb);
            }
        }

        if ( stk->initMask & A2B_INITIALIZED_JOBEXEC )
        {
            a2b_jobExecFree(stk->jobExec);
        }

        if ( stk->initMask & A2B_INITIALIZED_I2C )
        {
            if ( A2B_NULL != stk->i2cHnd )
            {
                if ( A2B_NULL != stk->pal.i2cClose )
                {
                    stk->pal.i2cClose(stk->i2cHnd);
                }
            }

            if ( A2B_NULL != stk->pal.i2cShutdown )
            {
                stk->pal.i2cShutdown(stk->ecb);
            }
        }

        /* Free up any outstanding timers */
        while ( !SLIST_EMPTY(&stk->timerList) )
        {
            timer = SLIST_FIRST(&stk->timerList);
            /* Unref until free */
            while ( 0u != a2b_timerUnref(timer) )
            {
                /* Do nothing */
            }
        }

#ifdef A2B_FEATURE_TRACE
        /* Cleanup the trace before turning off logging */
        if ( stk->initMask & A2B_INITIALIZED_TRACE )
        {
            a2b_traceFree(stk->traceChan);
        }
#endif

        /* Since trace and sequence charts rely on the logging
         * facilities this is that last thing (before the memory management
         * system) that is shutdown.
         */
        if ( stk->initMask & A2B_INITIALIZED_LOG )
        {
            if ( A2B_NULL != stk->pal.logShutdown )
            {
                stk->pal.logShutdown(stk->ecb);
            }
        }

        /*
         * NOTE: This *must* be the last thing deallocated
         */
        if ( stk->initMask & A2B_INITIALIZED_MEMMGR )
        {
            /* You have to be *very* careful releasing the stack heap
             * since the stack itself is allocated from this heap.
             */

            /* Make temporary copies of these which
             * are ultimately owned by the A2B stack application so they
             * won't go out of scope when the stack is deallocated.
             */
            ecb = stk->ecb;
            pal = stk->pal;
            hnd = stk->heapHnd;

            /* Try to free the stack instance itself */
            if ( A2B_NULL != pal.memMgrFree )
            {
                /* We can't reference 'stk' past here */
                pal.memMgrFree(hnd, stk);
            }

            /* If the underlying memory heap should be destroyed too */
            if ( destroyHeap )
            {
                a2b_stackDestroyHeap(hnd, &pal, ecb);
            }
        }
    }
} /* a2b_stackDestroy */


#ifdef A2B_FEATURE_MEMORY_MANAGER
/*!****************************************************************************
*
*  \b              a2b_stackGetMinHeapSize
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
 *                                      can be specified here.
*
*  \pre            Only available when #A2B_FEATURE_MEMORY_MANAGER is enabled.
*
*  \post           None
*
*  \return         The minimum number of bytes required to be allocated by an 
*                  A2B stack instance. This is the per *instance* amount.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_stackGetMinHeapSize
    (
    a2b_Byte*   heapStart
    )
{
    return a2b_memMgrGetMinHeapSize(heapStart);
} /* a2b_stackGetMinHeapSize */
#endif /* A2B_FEATURE_MEMORY_MANAGER */


/*!****************************************************************************
*
*  \b              a2b_stackPalInit
*
*  Sets basic stack defaults for certain fields of the PAL and ECB.
*
*  \param          [in]    pal      The PAL to initialize.
* 
*  \param          [in]    ecb      The environment control block to initialize.
*
*  \pre            NOP if #A2B_FEATURE_SEQ_CHART is *not* enabled.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackPalInit
    (
    a2b_StackPal*   pal,
    A2B_ECB*        ecb
    )
{
    if ( (A2B_NULL != pal)  && (A2B_NULL != ecb) )
    {
#ifdef A2B_FEATURE_MEMORY_MANAGER
        pal->memMgrInit = &a2b_memMgrInit;
        pal->memMgrOpen = &a2b_memMgrOpen;
        pal->memMgrMalloc = &a2b_memMgrMalloc;
        pal->memMgrFree = &a2b_memMgrFree;
        pal->memMgrClose = &a2b_memMgrClose;
        pal->memMgrShutdown = &a2b_memMgrShutdown;

        /* Assume a memory aligned heap start address */
        ecb->baseEcb.heap = A2B_NULL;
        ecb->baseEcb.heapSize = a2b_memMgrGetMinHeapSize(A2B_NULL);
#endif
    }
} /* a2b_stackPalInit */


/*!****************************************************************************
*
*  \b              a2b_stackPrivAlloc
*
*  Function that allocates an available internal stack instance.
*
*  \param          [in]    pal      Pointer to the platform abstraction layer 
*                                   to be assigned to this stack instance.
* 
*  \param          [in]    ecb      The environment control block (ECB) to 
*                                   associate with the allocated stack instance.
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns a stack instance. Otherwise, if there is no available
*                  stack instance then return A2B_NULL.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_Stack*
a2b_stackPrivAlloc
    (
    const a2b_StackPal *pal,
    A2B_ECB *ecb
    )
{
    a2b_Stack *stk = A2B_NULL;
    a2b_Handle heapHnd = A2B_NULL;
    a2b_HResult status;

    status = pal->memMgrInit(ecb);
    if ( A2B_SUCCEEDED(status) )
    {
        heapHnd = pal->memMgrOpen(ecb->baseEcb.heap, ecb->baseEcb.heapSize);
    }

    if ( A2B_NULL != heapHnd )
    {
        stk = pal->memMgrMalloc(heapHnd, sizeof(*stk));
    }

    /* If we failed to allocate a stack then ... */
    if ( A2B_NULL == stk )
    {
        /* Close the stack heap and shutdown the memory manager */
        a2b_stackDestroyHeap(heapHnd, pal, ecb);
    }
    /* Else initialize the stack */
    else
    {
        (void)a2b_memset(stk, 0, sizeof(*stk));
        stk->pal = *pal;
        stk->ecb = ecb;
        SLIST_INIT(&stk->timerList);
        stk->plugins = A2B_NULL;
        stk->numPlugins = (a2b_UInt16)0;
        stk->heapHnd = heapHnd;
        stk->initMask = A2B_INITIALIZED_MEMMGR;

#ifdef A2B_FEATURE_SEQ_CHART
        stk->seqChartChan = A2B_NULL;
#endif
    }

    return stk;
} /* a2b_stackPrivAlloc */



/*!****************************************************************************
*
*  \b              a2b_stackAlloc
*
*  Allocated and initializes the A2B stack context with A2B network
*  configuration.
*
*  \param          [in]    pal      A pointer to the (previously) initialized 
*                                   Platform Abstraction Layer (PAL) structure. 
*                                   This provides all the necessary platform 
*                                   abstractions required by the stack.
* 
*  \param          [in]    ecb      The environment control block (ECB) for
*                                   the driver.
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns a pointer to an A2B stack context or A2B_NULL if one
*                  couldn't be allocated either due to an error in the
*                  allocation process or because the necessary resources
*                  were unavailable.
*
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_StackContext*
a2b_stackAlloc
    (
    const struct a2b_StackPal *pal,
    A2B_ECB *ecb
    )
{
    a2b_Stack *stk = A2B_NULL;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_StackContext *ctx = A2B_NULL;
    a2b_ContextCtrlBlk ccb;

    /* Zero it out for now */
    (void)a2b_memset(&ccb.app, 0, sizeof(ccb.app));

    if ( (A2B_NULL != pal) && (A2B_NULL != ecb) )
    {
        /* Allocate a stack instance that's not in use */
        stk = a2b_stackPrivAlloc(pal, ecb);
        if ( A2B_NULL == stk )
        {
            /* Can't create a stack context without the stack, so exit */
            return A2B_NULL;
        }

        /* Try to allocate a context for the application.
         * Ensure the stack gets initialized in the context now 
         * since it is used in the other API calls.
         */
        ctx = a2b_stackContextAlloc(stk, A2B_DOMAIN_APP, &ccb);
        if ( A2B_NULL == ctx )
        {
            /* Allocation failed - release the underlying stack and heap */
            a2b_stackDestroy(stk, A2B_TRUE);
            return A2B_NULL;
        }
        ctx->stk = stk;

        (void)a2b_memset( &stk->pluginList[0], 0, sizeof(stk->pluginList) );

        if ( A2B_NULL != stk->pal.timerInit )
        {
            status = stk->pal.timerInit(ctx->stk->ecb);
            if ( A2B_SUCCEEDED(status) )
            {
                stk->initMask |= A2B_INITIALIZED_TIMER;
            }
        }

        if ( A2B_SUCCEEDED(status) )
        {
            if ( A2B_NULL != stk->pal.logInit )
            {
                status = stk->pal.logInit(stk->ecb);
                if ( A2B_SUCCEEDED(status) )
                {
                    stk->initMask |= A2B_INITIALIZED_LOG;
                }
            }
        }

        if ( A2B_SUCCEEDED(status) )
        {
            if ( A2B_NULL != stk->pal.i2cInit )
            {
                status = stk->pal.i2cInit(stk->ecb);
                if ( A2B_SUCCEEDED(status) )
                {
                    stk->initMask |= A2B_INITIALIZED_I2C;
                    stk->i2cHnd = stk->pal.i2cOpen(
                            stk->ecb->baseEcb.i2cAddrFmt,
                            stk->ecb->baseEcb.i2cBusSpeed,
                            stk->ecb);
                    if ( A2B_NULL == stk->i2cHnd )
                    {
                        status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                                A2B_FAC_STACK,
                                                A2B_EC_ALLOC_FAILURE);
                    }
                    else
                    {
                        a2b_stackResetI2cLastMode(&stk->i2cMode);
                    }
                }
            }
        }

        if ( A2B_SUCCEEDED(status) )
        {
            if ( A2B_NULL != stk->pal.audioInit )
            {
                status = stk->pal.audioInit(stk->ecb);
                if ( A2B_SUCCEEDED(status) )
                {
                    stk->initMask |= A2B_INITIALIZED_AUDIO;
                    if ( A2B_NULL != stk->pal.audioOpen )
                    {
                        stk->audioHnd = stk->pal.audioOpen();
                        if ( A2B_NULL == stk->audioHnd )
                        {
                            status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                                      A2B_FAC_STACK,
                                                      A2B_EC_ALLOC_FAILURE);
                        }
                    }
                }
            }
        }

        if ( A2B_SUCCEEDED(status) )
        {
            if ( A2B_NULL != stk->pal.pluginsLoad )
            {
                status = stk->pal.pluginsLoad(&stk->plugins,
                                              &stk->numPlugins, stk->ecb);
                if ( A2B_SUCCEEDED(status) )
                {
                    stk->initMask |= A2B_INITIALIZED_PLUGINS;
                }
            }
        }

#ifdef A2B_FEATURE_TRACE
        if ( A2B_SUCCEEDED(status) )
        {
			if(stk->ecb->baseEcb.traceUrl != A2B_NULL)
			{
				/* The trace channel must be allocated *after* the logging
				 * service has been initialized.
				 */
				stk->traceChan = a2b_traceAlloc(ctx,
												stk->ecb->baseEcb.traceUrl);
				if ( A2B_NULL == stk->traceChan )
				{
					status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
											  A2B_EC_ALLOC_FAILURE);
				}
				else
				{
					/* Set the initial user defined trace domain/level */
					a2b_traceSetMask(ctx, stk->ecb->baseEcb.traceLvl);

					stk->initMask |= A2B_INITIALIZED_TRACE;
				}
			}
        }
#endif

        if ( A2B_SUCCEEDED(status) )
        {
            stk->jobExec = a2b_jobExecAlloc(ctx);

            if ( A2B_NULL == stk->jobExec )
            {
                status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                          A2B_EC_ALLOC_FAILURE);
            }
            else
            {
                stk->initMask |= A2B_INITIALIZED_JOBEXEC;
            }
        }

        if ( A2B_SUCCEEDED(status) )
        {
            stk->msgRtr = a2b_msgRtrAlloc( ctx );

            if ( A2B_NULL != stk->msgRtr )
            {
                stk->initMask |= A2B_INITIALIZED_MSGRTR;
            }
            else
            {
                status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                          A2B_EC_ALLOC_FAILURE);
            }
        }

        /* If any of the initialization/allocation steps failed then
         * tear it all down.
         */
        if ( A2B_FAILED(status) )
        {
            /* Destroys the stack */
            a2b_stackContextFree(ctx);
            return A2B_NULL;
        }
    }

    return ctx;
} /* a2b_stackAlloc */


/*!****************************************************************************
*
*  \b              a2b_stackFreeSlaveNodeHandler
*
*  This routine frees/deallocates the plugin instantiation for a specific
*  slave node or all the slave nodes. This might typically be done if/when an
*  A2B network power fault occurs and the network is reset. Generally, this
*  should only be called by the master node plugin instance but is also
*  available to the application.
* 
*  \note
*  This function does *not* unload the slave plugin but rather frees
*  an instance of the associated plugin if it exits.
*
*  \param          [in]    ctx          The A2B stack context. Must be
*                                       either the application context
*                                       <em>or</em> the master node
*                                       plugin context.
*
*  \param          [in]    nodeAddr     The A2B slave node address. If equal
*                                       to #A2B_NODEADDR_NOTUSED then
*                                       <em>all</em> the slave plugin instances
*                                       will be freed.
*
*  \pre            None
*
*  \post           If a slave plugin instance exists for that node address
*                  then it is "closed" and the plugin *instance* freed. The
*                  plugin itself, however, is not unloaded.
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() macro for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_stackFreeSlaveNodeHandler
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                            A2B_EC_INVALID_PARAMETER);
    a2b_UInt16 idx;
    struct a2b_StackContext* plugCtx;

    /* If the context is not NULL AND the node address is NOT the master node
     * AND (the node address is "not used" OR a valid node address) AND
     * (the context is from the application domain OR the context is associated
     * with the master node context then) ...
     */
    if ( (A2B_NULL != ctx) &&
         ((A2B_NODEADDR_NOTUSED == nodeAddr) ||
           ((nodeAddr >= (A2B_NODEADDR_MASTER+1)) &&
             (nodeAddr < (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES))) &&
         ((A2B_DOMAIN_APP == ctx->domain) ||
          ((A2B_DOMAIN_PLUGIN == ctx->domain) &&
           (A2B_NODEADDR_MASTER == ctx->ccb.plugin.nodeSig.nodeAddr))) )
    {
        /* Search for the specified slave context to free */
        for ( idx = 0u; idx < (a2b_UInt16)A2B_ARRAY_SIZE(ctx->stk->pluginList); idx++ )
        {
            /* We need to skip over the master plugin instance */
            plugCtx = ctx->stk->pluginList[idx];
            if ( (A2B_NULL != plugCtx) &&
                (A2B_DOMAIN_PLUGIN == plugCtx->domain) &&
                (((A2B_NODEADDR_NOTUSED == nodeAddr) &&
                  (A2B_NODEADDR_MASTER !=
                   plugCtx->ccb.plugin.nodeSig.nodeAddr)) ||
                 (nodeAddr == plugCtx->ccb.plugin.nodeSig.nodeAddr)) )
            {
                /* This will "close" the slave plugin handler and NULL
                 * out the pointer in the pluginList.
                 */
                a2b_stackContextFree(plugCtx);
                result = A2B_RESULT_SUCCESS;
                if ( nodeAddr != A2B_NODEADDR_NOTUSED )
                {
                    break;
                }
            }
        }

        if ( A2B_RESULT_SUCCESS != result)
        {
            /* No such slave plugin instance exists */
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                        A2B_EC_DOES_NOT_EXIST);
        }
    }

    return result;
} /* a2b_stackFreeSlaveNodeHandler */


/*!****************************************************************************
*
*  \b              a2b_stackFindHandler
*
*  This routine is called to find a handler for a specific node.  We scan 
*  through the plugins list trying to open each one giving the nodeInfo and
*  nodeAddr of the discovered node. If the plugin can manage this node then
*  a valid (instantiated) handle is returned, else it returns null and the
*  next plugin is tried. Once a non-null handle is returned it is assumed
*  this is the managing plugin. If no plugin handles a node then it's
*  assumed to be a very dumb node.
*
*  \param          [in]    ctx         A2B Stack Context
* 
*  \param          [in]    nodeSig     Signature of the node you'd like to find
*
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*
*  \pre            This function should **only** be called by the Master plugin
*                  during discovery.
*
*  \post           Job queue is created if the plugin handles the node.
*
*  \post           If successful, a plugin is instantiated by the call.
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() macro for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_stackFindHandler
    (
    struct a2b_StackContext*   ctx,
    const a2b_NodeSignature*   nodeSig
    )
{
    a2b_UInt16 idx;
    a2b_PluginApi* plugin;
    a2b_StackContext* pluginCtx;
    a2b_Handle retHdl;
    a2b_ContextCtrlBlk  ccb;
#ifdef A2B_FEATURE_TRACE
    a2b_Int16 nodeAddr = nodeSig->nodeAddr;
#endif

    if (( A2B_NULL != ctx ) && ( ctx->stk->plugins ))
    {
        (void)a2b_memset(&ccb.plugin, 0, sizeof(ccb.plugin));

        /* Init the ccb prior to the a2b_stackContextAlloc */
        ccb.plugin.nodeSig   = *nodeSig;
        ccb.plugin.pluginApi = A2B_NULL;
        ccb.plugin.pluginHnd = A2B_NULL;

        /* Allocate a stack context for the plugin */
        pluginCtx = a2b_stackContextAlloc( ctx->stk, 
                                           A2B_DOMAIN_PLUGIN, &ccb );
        if ( A2B_NULL == pluginCtx )
        {
            /* Allocation failed */
            A2B_TRACE1((ctx, (A2B_TRC_DOM_STACK | A2B_TRC_LVL_ERROR), 
               "Failed to alloc stackContext for: addr:%hd",
               &nodeAddr ));
            return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK, 
                                    A2B_EC_ALLOC_FAILURE);
        }
        pluginCtx->stk = ctx->stk;

        for (idx = 0u; idx < ctx->stk->numPlugins; idx++)
        {
            plugin = &ctx->stk->plugins[idx];

            retHdl = plugin->open(pluginCtx, nodeSig);
            if ( retHdl )
            {
                pluginCtx->ccb.plugin.pluginApi = plugin;
                pluginCtx->ccb.plugin.pluginHnd = retHdl;
                (void)a2b_strncpy(pluginCtx->ccb.plugin.nodeSig.pluginName,
                    plugin->name,
                    (a2b_Size)A2B_ARRAY_SIZE(pluginCtx->ccb.plugin.nodeSig.pluginName)-1u);
                return A2B_RESULT_SUCCESS;
            }
        }

        a2b_stackContextFree( pluginCtx );

        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK, 
                                A2B_EC_RESOURCE_UNAVAIL);
    }

    return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK, 
                            A2B_EC_INVALID_PARAMETER);

} /* a2b_stackFindHandler */


/*!****************************************************************************
*
*  \b              a2b_stackFree
*
*  Frees the A2B stack instance and releases all resources.
*
*  \param          [in]    ctx      The A2B stack instance to free.
*
*  \pre            None
*
*  \post           The A2B stack instance is no longer valid and should not
*                  be used.
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackFree
    (
    struct a2b_StackContext *ctx
    )
{
    if ( (A2B_NULL != ctx) && (A2B_DOMAIN_APP == ctx->domain) )
    {
        a2b_stackContextFree(ctx);
    }
} /* a2b_stackFree */


/*!****************************************************************************
*
*  \b              a2b_stackTick
*
*  This function should be called periodically to provide a system
*  'tick' for the stack in order to schedule necessary processing.
*
*  \param          [in]    ctx      The A2B stack instance.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackTick
    (
    struct a2b_StackContext *ctx
    )
{
#ifdef A2B_FEATURE_TRACE
    a2b_UInt32 curTime;
#endif

    if ( (A2B_NULL != ctx) && (A2B_DOMAIN_APP == ctx->domain) )
    {
        A2B_SEQ_CHART1((ctx,
                        A2B_SEQ_CHART_ENTITY_APP,
                        A2B_SEQ_CHART_ENTITY_STACK,
                        A2B_SEQ_CHART_COMM_REQUEST,
                        A2B_SEQ_CHART_LEVEL_7,
                        "a2b_stackTick(0x%p)", ctx));

#ifdef A2B_FEATURE_TRACE
        curTime = ctx->stk->pal.timerGetSysTime();
        A2B_TRACE1((ctx, (A2B_TRC_DOM_TICK | A2B_TRC_LVL_TRACE2),
                    "Current time = %lu",&curTime));
#endif

        /* Give the timer services a tick and let it dispatch any timeouts */
        a2b_timerTick(ctx);

        if ( 0u == (ctx->stk->stackTickCnt % A2B_CONF_SCHEDULER_TICK_MULTIPLE) )
        {
            a2b_jobExecSchedule(ctx->stk->jobExec);
        }

        /* Track the number of times this function is called. It's okay
         * if this number rolls over.
         */
        ctx->stk->stackTickCnt++;

        A2B_SEQ_CHART1((ctx,
                        A2B_SEQ_CHART_ENTITY_STACK,
                        A2B_SEQ_CHART_ENTITY_APP,
                        A2B_SEQ_CHART_COMM_REPLY,
                        A2B_SEQ_CHART_LEVEL_7,
                        "a2b_stackTick(0x%p)", ctx));
    }
} /* a2b_stackTick */


/*!****************************************************************************
*
*  \b              a2b_stackPalGetVersion
*
*  Returns the A2B stack PAL's major, minor, and/or release version.
*  This is the version assigned to the platform abstraction layer when it's
*  built and is encoded in the binary library.
*
*  \param       [in]       pal      The initialized pointer to the PAL instance.
* 
*  \param       [in,out]   major    The major version of the PAL. If you are
*                                   not interested in the value pass in
*                                   A2B_NULL.
* 
*  \param       [in,out]   minor    The minor version of the PAL. If you are
*                                   not interested in the value pass in
*                                   A2B_NULL.
* 
*  \param       [in,out]   release  The release version of the PAL. If you are
*                                   not interested in the value pass in
*                                   A2B_NULL.
*
*  \pre         A pointer to an already initialized PAL structure.
*
*  \post        None
*
*  \return      None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackPalGetVersion
    (
    a2b_StackPal*   pal,
    a2b_UInt32*     major,
    a2b_UInt32*     minor,
    a2b_UInt32*     release
    )
{
    if ( A2B_NULL != pal )
    {
        pal->getVersion(major, minor, release);
    }
} /* a2b_stackPalGetVersion */


/*!****************************************************************************
*
*  \b              a2b_stackPalGetBuild
*
*  Returns information about the PAL build including build number,
*  date, owner, source revision, and/or host machine that did the build.
*
*  \param      [in]       pal          The initialized pointer to the PAL
*                                      instance.
* 
*  \param      [in,out]   buildNum     An incrementing build number. If you are
*                                      not interested in the value pass in
*                                      A2B_NULL.
* 
*  \param      [in,out]   buildDate    The date the library was built. If you
*                                      are not interested in the value pass in
*                                      A2B_NULL.
* 
*  \param      [in,out]   buildOwner   The owner/user who built this software.
*                                      If you are not interested in the value
*                                      pass in A2B_NULL.
*
*  \param      [in,out]   buildSrcRev  The source control revision associated 
*                                      with the built PAL. If you are not
*                                      interested in the value pass in A2B_NULL.
* 
*  \param      [in,out]   buildHost    The host machine where the PAL was
*                                      built. If you are not interested in the
*                                      value pass in A2B_NULL.
*
*  \pre        A pointer to an already initialized PAL structure.
*
*  \post       None
*
*  \return     None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackPalGetBuild
    (
    a2b_StackPal*       pal,
    a2b_UInt32*         buildNum,
    const a2b_Char**    const buildDate,
    const a2b_Char**    const buildOwner,
    const a2b_Char**    const buildSrcRev,
    const a2b_Char**    const buildHost
    )
{
    if ( A2B_NULL != pal )
    {
        pal->getBuild(buildNum, buildDate, buildOwner, buildSrcRev, buildHost);
    }
} /* a2b_stackPalGetBuild */
