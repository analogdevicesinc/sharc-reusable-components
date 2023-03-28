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
 * \file:   stackctx.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the A2B stack context functions.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/util.h"
#include "a2b/pal.h"
#include "a2b/msg.h"
#include "a2b/defs.h"
#include "a2b/stackctxmailbox.h"
#include "stackctx.h"
#include "stack_priv.h"
#include "jobexec.h"
#include "utilmacros.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_stackContextAlloc
*
*  Allocate a stack context.
*
*  \param          [in]    stk
*  \param          [in]    domain
*  \param          [in]    ccb
*
*  \pre            None
*
*  \post           None
*
*  \return         [add here]
*
******************************************************************************/
A2B_DSO_LOCAL a2b_StackContext*
a2b_stackContextAlloc
    (
    struct a2b_Stack*   stk,
    a2b_ContextDomain   domain,
    a2b_ContextCtrlBlk* ccb
    )
{
    a2b_UInt16 idx;
    a2b_StackContext *ctx;

    ctx = A2B_MALLOC(stk, sizeof(*ctx));

    if ( A2B_NULL != ctx )
    {
        /* Only the "APP" domain owns the stack pointer and is responsible
         * for freeing it when it's ref-count goes to zero.
         */
        ctx->stk = stk;
        ctx->domain = domain;

        /* Do a straight bitwise copy of the data. */
        ctx->ccb = *ccb;
        if ( A2B_DOMAIN_PLUGIN == domain )
        {
            SLIST_INIT( &ctx->ccb.plugin.mailboxList );

            /* Create the default message mailbox */
            if ( A2B_NULL == a2b_stackCtxMailboxAlloc( ctx, A2B_JOB_PRIO1 ) )
            {
                /* Allocation failed */
                a2b_stackContextFree(ctx);
                return A2B_NULL;
            }

            idx = ((a2b_UInt16)ctx->ccb.plugin.nodeSig.nodeAddr+(a2b_UInt16)1);

            /* Add this context to the stack for tracking */
            if (( idx >= A2B_ARRAY_SIZE(ctx->stk->pluginList) ) /*||
                ( ctx->ccb.plugin.nodeSig.nodeAddr+1 < 0 )*/)
            {
                a2b_stackContextFree(ctx);
                return A2B_NULL;
            }

            ctx->stk->pluginList[idx] = ctx;
        }
    }

    return ctx;
} /* a2b_stackContextAlloc */


/*!****************************************************************************
*
*  \b              a2b_stackContextFree
*
*  Free a stack context.
*
*  \param          [in]    ctx  A2B Stack Context
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_stackContextFree
    (
    a2b_StackContext*   ctx
    )
{
    a2b_ContextDomain domain;
    a2b_UInt16 idx;
    a2b_Handle heapHnd;
    a2b_StackPal pal;
    A2B_ECB* ecb;

    if ( A2B_NULL != ctx )
    {
        domain = ctx->domain;

        /* Else must be the plugin domain */
        if ( A2B_DOMAIN_PLUGIN == domain )
        {
            if (( ctx->ccb.plugin.pluginApi ) && 
                ( ctx->ccb.plugin.pluginApi->close ))
            {
                ctx->ccb.plugin.pluginApi->close(ctx->ccb.plugin.pluginHnd);
            }

            /* Free all allocated mailboxes */
            a2b_stackCtxMailboxFreeAll( ctx );

            /* Remove this context from the stack */
            for ( idx = 0u; idx < (a2b_UInt16)A2B_ARRAY_SIZE(ctx->stk->pluginList); idx++ )
            {
                if ( ctx == ctx->stk->pluginList[idx] )
                {
                    ctx->stk->pluginList[idx] = A2B_NULL;
                    break;
                }
            }
        }

        /* Remember these in case we need them later */
        heapHnd = ctx->stk->heapHnd;
        pal = ctx->stk->pal;
        ecb = ctx->stk->ecb;

        /* If the APP context then ... */
        if ( A2B_DOMAIN_APP == domain )
        {
            /* Destroy the stack but *not* the underlying heap. This
             * call will end up recursively calling this function again
             * to free the individual plugin contexts.
             */
            a2b_stackDestroy(ctx->stk, A2B_FALSE);
        }


        /* Free the context whether it's an app or plugin context */
        pal.memMgrFree(heapHnd, ctx);

        if ( A2B_DOMAIN_APP == domain )
        {
            /* Now it's safe to destroy the heap */
            a2b_stackDestroyHeap(heapHnd, &pal, ecb);
        }
    }
} /* a2b_stackContextFree */


/*!****************************************************************************
*
*  \b              a2b_stackContextFind
*
*  Grab a specific context based on the nodeAddr.
*
*  \param          [in]    ctx       A2B Stack Context
* 
*  \param          [in]    nodeAddr  nodeAddr to lookup
*                                    node address: [A2B_NODEADDR_MASTER(-1)..n]
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_NULL if no plugin available for the node or out of range
*                  Else, A2B Stack Context
*
******************************************************************************/
A2B_DSO_LOCAL struct a2b_StackContext*
a2b_stackContextFind
    (
    a2b_StackContext*   ctx,
    a2b_Int16           nodeAddr
    )
{
    if ((A2B_NULL == ctx) || (!A2B_VALID_NODEADDR(nodeAddr)))
    {
        return A2B_NULL;
    }

    if ((A2B_NODEADDR_MASTER == nodeAddr ) && 
        (A2B_NULL == ctx->stk->pluginList[0]))
    {
        a2b_NodeSignature   nodeSig;

        A2B_INIT_SIGNATURE( &nodeSig, A2B_NODEADDR_MASTER );
        A2B_INIT_SIGNATURE_BDD( &nodeSig, 
                                &ctx->stk->ecb->baseEcb.masterNodeInfo );

        (void)a2b_stackFindHandler( ctx, &nodeSig );
    }

    return ctx->stk->pluginList[nodeAddr+1];

} /* a2b_stackContextFind */

