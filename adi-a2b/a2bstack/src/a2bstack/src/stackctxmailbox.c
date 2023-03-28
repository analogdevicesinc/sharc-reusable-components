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
 * \file:   stackctxmailbox.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the code that handles mailbox handling for a stack context.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/ctypes.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/trace.h"
#include "a2b/seqchart.h"
#include "a2b/stackctxmailbox.h"
#include "a2b/pluginapi.h"
#include "a2b/msgrtr.h"
#include "a2b/msg.h"
#include "a2b/util.h"
#include "stack_priv.h"
#include "stackctx.h"
#include "msgrtr_priv.h"
#include "msg_priv.h"
#include "jobexec.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/


/*!****************************************************************************
*
*  \b              a2b_stackCtxMailboxAlloc
*
*  Create a new mailbox for a stack context.  This hooks the mailbox into
*  the stacks job executor (main loop) processing.  This allows you to
*  create a mailbox for async processing.
*
*  \param          [in]    ctx        A2B stack context
* 
*  \param          [in]    priority   Priority of the mailbox
*                                     [0 [highest] .. 4 [lowest]]
*
*  \pre            Function restricted to ONLY plugins
*
*  \post           None
*
*  \return         Mailbox handle or A2B_NULL if a mailbox could not be
*                  allocated.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Handle
a2b_stackCtxMailboxAlloc
    (
    struct a2b_StackContext*   ctx,
    a2b_JobPriority            priority 
    )
{
    a2b_Handle mboxHnd = A2B_NULL;

    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        a2b_JobQueue* jobQ;
        a2b_JobQueue* lastJobQ = A2B_NULL;
        a2b_JobQueue* newJobQ  = a2b_jobExecAllocQueue( ctx->stk->jobExec,
                                                        priority );
        if ( A2B_NULL == newJobQ )
        {
            return mboxHnd;
        }

        /* Find the last node so we can append it to the end */
        SLIST_FOREACH( jobQ, &ctx->ccb.plugin.mailboxList, link2 )
        {
            lastJobQ = jobQ;
        }

        if ( A2B_NULL == lastJobQ )
        {
            SLIST_INSERT_HEAD( &ctx->ccb.plugin.mailboxList, newJobQ, link2 );
        }
        else
        {
            SLIST_INSERT_AFTER( lastJobQ, newJobQ, link2 );
        }

        mboxHnd = newJobQ;
    }

    return mboxHnd;

} /* a2b_stackCtxMailboxAlloc */


/*!****************************************************************************
*
*  \b              a2b_stackCtxMailboxFree
*
*  Frees the mailbox previously allocate by #a2b_stackCtxMailboxAlloc.
*
*  \param          [in]    ctx          A2B stack context
* 
*  \param          [in]    mailboxHnd   The mailbox to free
*
*  \pre            Function restricted to ONLY plugins
*
*  \post           The mailbox is **NOT** immediately free but scheduled
*                  to be released later.
*
*  \return         Returns A2B_TRUE if the mailbox was de-allocated or
*                  A2B_FALSE otherwise.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
a2b_stackCtxMailboxFree
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  mailboxHnd
    )
{
    a2b_JobQueue*   jobQ;
    a2b_Bool        isFreed = A2B_FALSE;

    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        /* Find the queue/mailbox to delete */
        SLIST_FOREACH( jobQ, &ctx->ccb.plugin.mailboxList, link2 )
        {
            if ( jobQ == (a2b_JobQueue*)mailboxHnd )
            {
                /* Remove the mailbox/job queue from the linked list */
                SLIST_REMOVE(&ctx->ccb.plugin.mailboxList, jobQ,
                            a2b_JobQueue, link2);

                /* Actually unreference the queue */
                (void)a2b_jobExecUnrefQueue(jobQ);
                
                /* Indicate it was unreferenced */
                isFreed = A2B_TRUE;
                
                break;
            }
        }
    }

    return isFreed;
}


/*!****************************************************************************
*
*  \b              a2b_stackCtxMailboxFreeAll
*
*  Free all mailboxes owned by this plugin context.
*
*  \param          [in]    ctx    The plugin context owning the mailboxes.
*
*  \pre            The context *must* come from the plugin domain.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackCtxMailboxFreeAll
    (
    struct a2b_StackContext*    ctx
    )
{
    a2b_JobQueue* jobQ;

    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        /* Removes all mailbox/job queues in the list */
        while ( !SLIST_EMPTY(&ctx->ccb.plugin.mailboxList) )
        {
            jobQ = SLIST_FIRST(&ctx->ccb.plugin.mailboxList);
            (void)a2b_stackCtxMailboxFree( ctx, jobQ );
        }
    }

} /* a2b_stackCtxMailboxFreeAll */


/*!****************************************************************************
*
*  \b              a2b_stackCtxMailboxCount
*
*  This is used to get the number of mailboxes configured for a specific
*  stack context.
*
*  \param          [in]    ctx      A2B stack plugin context.
*
*  \pre            None
*
*  \post           None
*
*  \return         The number of mailboxes [1..n]. Every stack context has at
*                  least one default mailbox.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_stackCtxMailboxCount
    (
    struct a2b_StackContext*    ctx
    )
{
    a2b_JobQueue*   jobQ;
    a2b_UInt32      mailboxCnt = 0u;

    SLIST_FOREACH( jobQ, &ctx->ccb.plugin.mailboxList, link2 )
    {
        mailboxCnt++;
    }

    return mailboxCnt;

} /* a2b_stackCtxMailboxCount */


/*!****************************************************************************
*
*  \b              a2b_stackCtxMailboxFind
*
*  Lookup the mailbox/job queue for a stack context.
*
*  \param          [in]    ctx              A2B stack plugin context.
* 
*  \param          [in]    mailboxHnd       Mailbox handle or A2B_NULL for
*                                           the default mailbox.
*
*  \pre            Restricted to ONLY plugins.
*
*  \post           None
*
*  \return         The mailbox job queue
*
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_JobQueue*
a2b_stackCtxMailboxFind
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  mailboxHnd
    )
{
    a2b_JobQueue* jobQ;

    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        SLIST_FOREACH( jobQ, &ctx->ccb.plugin.mailboxList, link2 )
        {
            if ( (mailboxHnd == A2B_NULL) ||
                (jobQ == (a2b_JobQueue*)mailboxHnd) )
            {
                return jobQ;
            }
        }
    }

    return A2B_NULL;

} /* a2b_stackCtxMailboxFind */


/*!****************************************************************************
*
*  \b              a2b_stackCtxMailboxFlush
*
*  Flush everything out of a specific mailbox.
*
*  \param          [in]    ctx              A2B stack plugin context.
* 
*  \param          [in]    mailboxHnd       Mailbox handle or A2B_NULL for
*                                           the default mailbox.
*
*  \pre            Restricted to ONLY plugins.
*
*  \post           None
*
*  \return         A2B_FALSE == mailbox Not found,
*                  A2B_TRUE == operation carried out
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
a2b_stackCtxMailboxFlush
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  mailboxHnd
    )
{
    a2b_JobQueue* jobQ;

    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        jobQ = a2b_stackCtxMailboxFind( ctx, mailboxHnd );
        if ( jobQ )
        {
            a2b_jobExecFlushQueue(jobQ);
            return A2B_TRUE;
        }
    }

    return A2B_FALSE;

} /* a2b_stackCtxMailboxFlush */

