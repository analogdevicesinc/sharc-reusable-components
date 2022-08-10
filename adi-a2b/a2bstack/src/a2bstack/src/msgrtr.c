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
 * \file:   msgrtr.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the code that handles routing messages within the system.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_msgrtr 
 * \defgroup a2bstack_msgrtr_priv        \<Private\> 
 * \private 
 *  
 * This defines the message router API's that are private to the stack.
 */
/*============================================================================*/

/*======================= I N C L U D E S =========================*/

#include "a2b/ctypes.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/trace.h"
#include "a2b/seqchart.h"
#include "stack_priv.h"
#include "stackctx.h"
#include "a2b/pluginapi.h"
#include "a2b/msgrtr.h"
#include "msgrtr_priv.h"
#include "a2b/stackctxmailbox.h"
#include "a2b/msg.h"
#include "msg_priv.h"
#include "utilmacros.h"
#include "a2b/util.h"
#include "jobexec.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/
#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
static void a2b_msgRtrOnJobComplete(struct a2b_Job* job, a2b_Bool isCancelled);
#endif
static a2b_HResult a2b_msgRtrInitMasterPlugin(struct a2b_StackContext* ctx);
static a2b_Int32 a2b_msgRtrExecute(struct a2b_Job* job);
static void a2b_msgRtrOnJobDestroy(struct a2b_Job* job);

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/


#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
/*!****************************************************************************
*  \ingroup        a2bstack_msgrtr_priv
* 
*  \b              a2b_msgRtrOnJobComplete
*
*  This function is called by the job executor when a job has completed.
*  This is a proxy routine when using sequence charts or tracing.  This
*  allows us to uniformly traces message complete handling.  This
*  translates to the "real" response being delivered for a message request.
*
*  \param          [in]    job          The job that is complete. Actually
*                                       references a request message.
*
*  \param          [in]    isCancelled  An indication of whether the job
*                                       was cancelled before finishing.
*
*  \pre            Only used when #A2B_FEATURE_SEQ_CHART or #A2B_FEATURE_TRACE
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_msgRtrOnJobComplete
    (
    struct a2b_Job* job,
    a2b_Bool        isCancelled
    )
{
    a2b_Msg* msg = (a2b_Msg*)job;
    if ( msg )
    {
        A2B_SEQ_CHART3((msg->ctx,
            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(msg->destNodeAddr),
            (msg->ctx->domain == A2B_DOMAIN_APP) ?
            A2B_SEQ_CHART_ENTITY_APP :
            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                msg->ctx->ccb.plugin.nodeSig.nodeAddr),
            A2B_SEQ_CHART_COMM_REPLY,
            A2B_SEQ_CHART_LEVEL_MSGS,
            "a2b_msgRtrSendReply"
            "(m: 0x%p, cmd: %ld, ud: 0x%p)",
            msg, &msg->cmd, msg->userData));

        if ( A2B_NULL != msg->onComplete )
        {
            msg->onComplete((struct a2b_Job*)msg, isCancelled);
        }
    }

} /* a2b_msgRtrOnJobComplete */
#endif


/*!****************************************************************************
*  \ingroup        a2bstack_msgrtr_priv 
*
*  \b              a2b_msgRtrInitMasterPlugin
*
*  This function initializes the master plugin if necessary. If it has already
*  been initialized then this function will do nothing and return success.
*
*  \param          [in] ctx     A2B stack context
*
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
static a2b_HResult
a2b_msgRtrInitMasterPlugin
    (
    struct a2b_StackContext*    ctx
    )
{
    a2b_HResult ret = A2B_RESULT_SUCCESS;

    if ( A2B_NULL == ctx )
    {
        ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                A2B_FAC_MSGRTR,
                                A2B_EC_INVALID_PARAMETER);
    }
    else if ( A2B_NULL == ctx->stk->pluginList[A2B_NODEADDR_MASTER+1] )
    {
        a2b_NodeSignature   nodeSig;

        A2B_INIT_SIGNATURE( &nodeSig, A2B_NODEADDR_MASTER );
        A2B_INIT_SIGNATURE_BDD( &nodeSig, 
                                &ctx->stk->ecb->baseEcb.masterNodeInfo );

        ret = a2b_stackFindHandler( ctx, &nodeSig );
        if ( !A2B_SUCCEEDED(ret) )
        {
            A2B_TRACE2((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                        "a2b_msgRtrInitMasterPlugin(0x%p): "
                        "0x%lX, Failed to init master plugin.",
                        ctx, &ret));
        }
    }
    else
    {
        /* Completing the control statement */
    }

    return ret;

} /* a2b_msgRtrInitMasterPlugin */


/*!****************************************************************************
*  \ingroup        a2bstack_msgrtr_priv
* 
*  \b              a2b_msgRtrExecute
*
*  This function is invoked by the Job Executor and provides a bridge between
*  jobs and messages by passing in additional information pertinent to the
*  plugin's execution function.
*
*  \param          [in]    job  The job/message to execute.
*
*  \pre            None
*
*  \post           None
*
*  \return         The Job Executor's completion result indicating whether
*                  the job is completed, should be scheduled again, or
*                  suspended.                                               <br>
*                  #A2B_EXEC_COMPLETE == Execution is now complete          <br>
*                  #A2B_EXEC_SCHEDULE == Execution is unfinished - schedule
*                                        again                              <br>
*                  #A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                        scheduling until a later even      <br>
*
******************************************************************************/
static a2b_Int32
a2b_msgRtrExecute
    (
    struct a2b_Job* job
    )
{
    a2b_Msg* msg = (a2b_Msg*)job;
    a2b_HResult result = (a2b_HResult)A2B_EXEC_COMPLETE;
    if ( (msg) && (msg->destCtx) )
    {
        result = (a2b_UInt32)msg->destCtx->ccb.plugin.pluginApi->execute(msg,
                                    msg->destCtx->ccb.plugin.pluginHnd,
                                    msg->destCtx);
    }

    return (a2b_Int32)result;

} /* a2b_msgRtrExecute */


/*!****************************************************************************
*  \ingroup        a2bstack_msgrtr_priv
* 
*  \b              a2b_msgRtrOnJobDestroy
*
*  This function is called by the job executor to destroy a job that
*  it owns after the job is complete.
*
*  \param          [in]    job      The job instance to destroy
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_msgRtrOnJobDestroy
    (
    struct a2b_Job* job
    )
{
    a2b_Msg* msg = (a2b_Msg*)job;
    if ( msg )
    {
        /* Since we're managing the life-time of the derived message we
         * unreference it here. The message is referenced when successfully
         * submitted to the job executor.
         */
        (void)a2b_msgUnref(msg);
    }

} /* a2b_msgRtrOnJobDestroy */


/*!****************************************************************************
*  \ingroup        a2bstack_msgrtr_priv 
*  \private
* 
*  \b              a2b_msgRtrAlloc
*
*  Allocate/initialize the message router subsystem for a specific stack.
*
*  \param          [in]    ctx      A2B stack context
*
*  \pre            None
*
*  \post           None
*
*  \return         Allocated message router instance
*
******************************************************************************/
A2B_DSO_LOCAL struct a2b_MsgRtr*
a2b_msgRtrAlloc
    (
        struct a2b_StackContext *ctx
    )
{
    a2b_MsgRtr*     msgRtr;

    msgRtr = (a2b_MsgRtr*)A2B_MALLOC(ctx->stk, sizeof(*msgRtr));

    if ( A2B_NULL != msgRtr )
    {
        msgRtr->ctx = ctx;

        SLIST_INIT( &msgRtr->notifierHead );
    }

    return msgRtr;

} /* a2b_msgRtrAlloc */


/*!**************************************************************************** 
*  \ingroup        a2bstack_msgrtr_priv
*  \private
* 
*  \b              a2b_msgRtrFree
*
*  Free the message router subsystem for a specific stack.
*
*  \param          [in]    msgRtr   Message router instance from
*                                   #a2b_msgRtrAlloc
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_msgRtrFree
    (
        a2b_MsgRtr *msgRtr
    )
{
    if ( A2B_NULL != msgRtr )
    {
        a2b_MsgNotifier*    notifier;

        /* Free up any queues associated with the notification listeners */
        while ( !SLIST_EMPTY(&(msgRtr->notifierHead)) )
        {
            notifier = SLIST_FIRST(&(msgRtr->notifierHead));
            SLIST_REMOVE_HEAD(&(msgRtr->notifierHead), link);

            /* Call destroy callbacks for the userData */
            if (( notifier->userData ) && ( notifier->destroy ))
            {
                notifier->destroy( notifier->userData );
            }
            A2B_FREE((msgRtr)->ctx->stk, notifier);
        }

        A2B_FREE((msgRtr)->ctx->stk, msgRtr);
    }

} /* a2b_msgRtrFree */


/*!****************************************************************************
*
*  \b              a2b_msgRtrGetHandler
*
*  Use this to query what handlers are available for message routing. The
*  API is designed for loop iterating.
* 
*  \verbatim
   a2b_UInt32   idx = 0;
   while ( A2B_NULL != a2b_msgRtrGetHandler(ctx, &idx) )
   {
        < do something >
   }
   \endverbatim
* 
*  \param          [in]     ctx  A2B Stack Context used by the handler
*  \param          [in,out] idx  0 for the initial loop, non-zero for
*                                all other iterations.
*
*  \pre            The following are expected to be valid:
*                  ctx->stk and ctx->stk->msgRtr
*
*  \post           None
*
*  \return         A2B_NULL if the range has been exceeded.
*
******************************************************************************/
A2B_DSO_PUBLIC const a2b_NodeSignature*
a2b_msgRtrGetHandler
    (
        struct a2b_StackContext*    ctx,
        a2b_UInt32*                 idx
    )
{
    a2b_StackContext*   plugin;

    if (( A2B_NULL == ctx ) || ( A2B_NULL == idx ))
    {
        return A2B_NULL;
    }

    if ( (*idx >= A2B_ARRAY_SIZE(ctx->stk->pluginList)) )
    {
        return A2B_NULL;
    }

    for ( ; *idx < A2B_ARRAY_SIZE(ctx->stk->pluginList); (*idx)++)
    {
        plugin = ctx->stk->pluginList[*idx];
        if (plugin)
        {
            (*idx)++;
            return &plugin->ccb.plugin.nodeSig;
        }
    }

    (*idx)++;
    return A2B_NULL;

} /* a2b_msgRtrGetHandler */


/*!****************************************************************************
*
*  \b              a2b_msgRtrRegisterNotify
*
*  This function is used to register for specific notification messages. 
*
*  \attention 
*  The "notify" callback routine is called as a function call when a 
*  notification is made.  The callback is <b>not</b> made from a job queue. 
*  Therefore, the callback routine should be sure not to make any blocking 
*  calls.  Blocking within the callback will block the stack and could 
*  potentially result in deadlock in the stack. 
* 
*  \param          [in]    ctx              A2B stack context.
*  \param          [in]    cmd              What to register/listen for
*  \param          [in]    notify           Callback function on notification
*  \param          [in]    userData         User data to send on the
                                            notification callback.
*  \param          [in]    destroyUserData  A function that will be called
*                                           prior to destroying a notification
*                                           message. It provides a mechanism
*                                           for the caller to clean-up any
*                                           user data.
*
*  \pre            The following are expected to be valid:
*                  ctx->stk and ctx->stk->msgRtr
*
*  \post           None
*
*  \return         Msg notifier pointer on success.  This should be used 
*                  simply as a cookie.  A2B_NULL on failure.
*
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_MsgNotifier*
a2b_msgRtrRegisterNotify
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt32                  cmd,
    a2b_NotifyFunc              notify,
    a2b_Handle                  userData,
    a2b_CallbackFunc            destroyUserData
    )
{
    a2b_MsgNotifier*    notifier = A2B_NULL;

    if (( A2B_NULL == ctx ) || ( A2B_NULL == notify ))
    {
        return A2B_NULL;
    }

    A2B_TRACE1((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                "Enter: a2b_msgRtrRegisterNotify(0x%p)", ctx));

    notifier = (a2b_MsgNotifier*)A2B_MALLOC(ctx->stk, sizeof(*notifier));
    if ( A2B_NULL != notifier )
    {
        notifier->ctx       = ctx;
        notifier->cmd       = cmd;
        notifier->notify    = notify;
        notifier->userData  = userData;
        notifier->destroy   = destroyUserData;

        SLIST_INSERT_HEAD(&(ctx->stk->msgRtr->notifierHead),
                          notifier, link);
    }

    A2B_TRACE2((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                "Exit: a2b_msgRtrRegisterNotify(0x%p): 0x%p", ctx, notifier));

    return notifier;

} /* a2b_msgRtrRegisterNotify */


/*!****************************************************************************
*
*  \b              a2b_msgRtrUnregisterNotify
*
*  Unregistered for notifications
*
*  \param          [in]    notifier   registered notification cookie
*                                     from #a2b_msgRtrRegisterNotify
*
*  \pre            Must be a valid #a2b_MsgNotifier or NULL
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_msgRtrUnregisterNotify
    (
    struct a2b_MsgNotifier*    notifier
    )
{
    if (notifier)
    {
        A2B_TRACE1((notifier->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_msgRtrUnregisterNotify(0x%p)", notifier->ctx));

        if (notifier->destroy)
        {
            notifier->destroy( notifier->userData );
        }
		if(notifier->ctx != A2B_NULL)
		{
			SLIST_REMOVE(&(notifier->ctx->stk->msgRtr->notifierHead),
						 notifier, a2b_MsgNotifier, link );

			notifier->userData  = A2B_NULL;

			A2B_TRACE1((notifier->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
				"Exit: a2b_msgRtrUnregisterNotify(0x%p)", notifier->ctx));

			A2B_FREE( notifier->ctx->stk, notifier );

			notifier->ctx = A2B_NULL;

		}

    }

} /* a2b_msgRtrUnregisterNotify */


/*!****************************************************************************
*
*  \b              a2b_msgRtrSendRequestToMailbox
*
*  This function will send the message to a specific mailbox.
*
*  \param          [in]    msg           A2B message to send/route
*  \param          [in]    mailboxHnd    Specific mailbox handle to send the
*                                        request.
*  \param          [in]    complete      function called when command completes
*  
*  \pre            msg MUST be initialized and ready to send (args, TID, etc)
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED() or
*                  #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_msgRtrSendRequestToMailbox
    (
    struct a2b_Msg*         msg,
    a2b_Handle              mailboxHnd,
    a2b_MsgCallbackFunc     complete
    )
{
    a2b_StackContext*   ctx;
    a2b_UInt16          idx;
    a2b_StackContext*   pluginCtx;
    a2b_HResult         ret = A2B_RESULT_SUCCESS;
    a2b_Bool            bSuccess = A2B_FALSE;
    a2b_JobQueue*       jobQ;

    if ( A2B_NULL == msg )
    {
        ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR,
                               A2B_EC_INVALID_PARAMETER);
        /* NOTE: Cannot trace this because no ctx available */
    }
    else
    {
        ctx = (a2b_StackContext*)msg->ctx;

        A2B_TRACE1((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_msgRtrSendRequestToMailbox(0x%p)", ctx));

        /* This will allow requests and custom messages */
        if (  A2B_MSG_REQUEST != msg->type )
        {
            ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR, 
                                   A2B_EC_INVALID_PARAMETER);
        }


        if ( A2B_SUCCEEDED(ret) )
        {
            msg->job.onComplete = (void (*)(struct a2b_Job* job, a2b_Bool isCancelled))complete;

            /* Initialize the master plugin if it hasn't been done yet */
            if ( A2B_NULL == ctx->stk->pluginList[A2B_NODEADDR_MASTER+1] )
            {
                ret = a2b_msgRtrInitMasterPlugin(msg->ctx);
            }

            if ( A2B_FAILED(ret) )
            {
                A2B_TRACE2((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                            "Exit: a2b_msgRtrSendRequestToMailbox(0x%p): "
                            "0x%lX, Failed to init master.",
                            ctx, &ret));
                return ret;
            }

            ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR, 
                                   A2B_EC_DOES_NOT_EXIST);

            /* Queue the message to the correct handler/plugin */
            for ( idx = 0u; idx < (a2b_UInt16)A2B_ARRAY_SIZE(ctx->stk->pluginList); idx++ )
            {
                pluginCtx = ctx->stk->pluginList[idx];
                jobQ = a2b_stackCtxMailboxFind( pluginCtx, mailboxHnd );
                if ( pluginCtx && jobQ )
                {
                    /* Assign a function to translate between the job's execute
                     * callback and the plugin's version.
                     */
                    msg->job.execute = &a2b_msgRtrExecute;

                    /* Track the destination context */
                    msg->destCtx = pluginCtx;

                    msg->destNodeAddr = pluginCtx->ccb.plugin.nodeSig.nodeAddr;

                    /* Intercept when the job is actually destroyed */
                    msg->job.destroy = &a2b_msgRtrOnJobDestroy;

#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
                    /* Intercept the callback that's executed when the
                     * job is complete.
                     */
                    msg->onComplete = msg->job.onComplete;
                    msg->job.onComplete = &a2b_msgRtrOnJobComplete;
#endif
                    /* Submit the message to the destined job queue */
                    bSuccess = a2b_jobExecSubmit( jobQ,
                                                  (struct a2b_Job*)msg );

                    if ( !bSuccess )
                    {
                        A2B_TRACE3((ctx, 
                                    (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                                    "a2b_msgRtrSendRequestToMailbox(0x%p, "
                                    "addr: %hd, cmd: %d): Failed",
                                    pluginCtx, 
                                    &msg->destNodeAddr,
                                    &msg->cmd));

                        ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR, 
                                               A2B_EC_INVALID_PARAMETER);
                    }
                    /* Else successfully submitted */
                    else
                    {
                        /* Add a reference to the message on behalf of the
                         * job executor.
                         */
                        a2b_msgRef(msg);
                        A2B_TRACE3((ctx, 
                            (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                            "a2b_msgRtrSendRequestToMailbox(0x%p, "
                            "addr: %hd, cmd: %d)",
                            pluginCtx, 
                            &msg->destNodeAddr,
                            &msg->cmd));

                        A2B_SEQ_CHART3((msg->ctx,
                            (msg->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                msg->ctx->ccb.plugin.nodeSig.nodeAddr),
                            (pluginCtx->domain == A2B_DOMAIN_APP) ?
                                        A2B_SEQ_CHART_ENTITY_APP :
                                        A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                        pluginCtx->ccb.plugin.nodeSig.nodeAddr),
                            A2B_SEQ_CHART_COMM_REQUEST,
                            A2B_SEQ_CHART_LEVEL_MSGS,
                            "a2b_msgRtrSendRequest"
                            "(m: 0x%p, cmd: %ld, ud: 0x%p)",
                            msg, &msg->cmd, msg->userData));

                        ret = A2B_RESULT_SUCCESS;                
                    }
                    break;
                }
            }
        }

        A2B_TRACE2((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_msgRtrSendRequestToMailbox(0x%p): 0x%lX", 
                    ctx, &ret));
    }


    return ret;

} /* a2b_msgRtrSendRequestToMailbox */


/*!****************************************************************************
*
*  \b              a2b_msgRtrSendRequest
*
*  This function will send a message to the default message mailbox.
* 
*  \attention
*  If you are unclear whether or not you need to send the request to
*  a specific mailbox you should use this routine.
* 
*  <br>
*  @image html msgrtr_msgreq.png "Request Example"
* 
*  \param          [in]    msg           A2B message for routing
*  \param          [in]    destNodeAddr  destination nodeAddr
*  \param          [in]    complete      function called when command completes
*
*  \pre            msg MUST be initialized and ready to send (args, TID, etc)
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_msgRtrSendRequest
    (
    struct a2b_Msg*       msg,
    a2b_Int16             destNodeAddr,
    a2b_MsgCallbackFunc   complete
    )
{
    a2b_StackContext* pluginCtx;
    a2b_Handle mailboxHnd;
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_MSGRTR,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != msg) && (A2B_NULL != msg->ctx) &&
        (A2B_MSG_REQUEST == msg->type) &&
        ( destNodeAddr <= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES ) &&
         ( destNodeAddr >= A2B_NODEADDR_MASTER ) )
    {
       if ( A2B_NULL == msg->ctx->stk->pluginList[A2B_NODEADDR_MASTER+1] )
       {
           /* Initialize the master plugin if it hasn't been done yet */
           result = a2b_msgRtrInitMasterPlugin(msg->ctx);
       }
       else
       {
           result = A2B_RESULT_SUCCESS;
       }

       if ( A2B_SUCCEEDED(result) )
       {
           /* Find the default mailbox handle for the specified destination
            * node.
            */

           pluginCtx = msg->ctx->stk->pluginList[destNodeAddr+1];
           if ( A2B_NULL != pluginCtx )
           {
               /* Lookup the default mailbox for this plugin */
               mailboxHnd = a2b_stackCtxMailboxFind( pluginCtx, A2B_NULL );
               result = a2b_msgRtrSendRequestToMailbox( msg, mailboxHnd,
                                                       complete );
           }
           else
           {
               result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                           A2B_FAC_MSGRTR,
                                           A2B_EC_DOES_NOT_EXIST);
           }
       }
    }

    return result;
} /* a2b_msgRtrSendRequest */


/*!****************************************************************************
*
*  \b              a2b_msgRtrNotify
*
*  Function called to send a notification to registered listeners/clients.
* 
*  \attention
*  All listeners are called through a function call, <b>not</b> the
*  job executor.
* 
*  @image html msgrtr_msgnotify.png "Notification Example"
* 
*  \param          [in]    msg    A2B Message for routing
*
*  \pre            None
*
*  \post           msg is NOT ref counted at all since the listeners are
*                  simply function calls.  If the msg is kept by a caller it
*                  should be ref counted at that point.
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_msgRtrNotify
    (
    struct a2b_Msg* msg
    )
{
    a2b_MsgRtr*         msgRtr;
    a2b_MsgNotifier*    notifier = A2B_NULL;
    a2b_HResult         ret      = A2B_RESULT_SUCCESS;

    A2B_TRACE1((((a2b_Msg*)msg)->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                "Enter: a2b_msgRtrNotify(0x%p)", msg));

    if ( A2B_NULL != msg )
    {
        msgRtr = (msg->ctx->stk->msgRtr);

        /* This will allow notify and custom messages */
        if ( A2B_MSG_REQUEST == msg->type )
        {
            ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR, 
                                   A2B_EC_INVALID_PARAMETER);
        }
        else
        {
            /* Iterate and call registered clients */
            SLIST_FOREACH(notifier, &msgRtr->notifierHead, link)
            {
                if ( msg->cmd == notifier->cmd )
                {
                    A2B_SEQ_CHART3((msg->ctx,
                                (msg->ctx->domain == A2B_DOMAIN_APP) ?
                                A2B_SEQ_CHART_ENTITY_APP :
                                A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                        msg->ctx->ccb.plugin.nodeSig.nodeAddr),
                                (notifier->ctx->domain == A2B_DOMAIN_APP) ?
                                    A2B_SEQ_CHART_ENTITY_APP :
                                    A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                    notifier->ctx->ccb.plugin.nodeSig.nodeAddr),
                                A2B_SEQ_CHART_COMM_NOTIFY,
                                A2B_SEQ_CHART_LEVEL_MSGS,
                                "a2b_msgRtrNotify(m: 0x%p, cmd: %ld, ud: 0x%p)",
                                msg, &notifier->cmd, notifier->userData));
                    A2B_TRACE3((((a2b_Msg*)msg)->ctx,
                                (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_DEBUG),
                                "a2b_msgRtrNotify(m: 0x%p, cmd: %ld, ud: 0x%p)",
                                msg, &notifier->cmd, notifier->userData ));

                    notifier->notify( msg, notifier->userData );
                }
            }
        }
    }

    A2B_TRACE2((((a2b_Msg*)msg)->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                "Exit: a2b_msgRtrNotify(0x%p): 0x%lX", msg, &ret));

    return ret;

} /* a2b_msgRtrNotify */


/*!****************************************************************************
*
*  \b              a2b_msgRtrExecUpdate
*
*  Updates the execution action for the given A2B plugin context.           <br>
*                                                                           <br>
*  For example, if you are processing a request within a plugin you
*  can begin processing the request and return #A2B_EXEC_SUSPEND
*  from the execute callback.  Then when the request finally completes
*  you can call this routine to change the action of the message
*  to the completed state.  This will result in the complete callback
*  being called.
* 
*  \param          [in]    ctx          A2B stack plugin context.
*
*  \param          [in]    mailboxHnd   The mailbox handle.
*
*  \param          [in]    action       The execution action. Can be one
*                                       of the following: #A2B_EXEC_COMPLETE,
*                                       #A2B_EXEC_SCHEDULE, and
*                                       #A2B_EXEC_SUSPEND
*
*  \pre            The context must be a plugin context rather than an
*                  application context.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_msgRtrExecUpdate
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  mailboxHnd,
    a2b_Int32                   action
    )
{
    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        a2b_JobQueue*   jobQ;

        /* Lookup the mailbox */
        jobQ = (a2b_JobQueue*)a2b_stackCtxMailboxFind( ctx, mailboxHnd );
        if ( jobQ )
        {
            a2b_jobExecUpdate( jobQ, action );
        }
    }
} /* a2b_msgExecUpdate */


/*!****************************************************************************
*
*  \b              a2b_msgRtrGetExecutingMsg
*
*  This grabs the newest message from the contexts job queue.  This can be
*  used to avoid the need to keep a message reference within the plugin at
*  all times.  Instead you can call this to get the message being processed
*  then add the response data and complete the execution.
*
*  \param          [in]    ctx              A2B stack plugin context.
*
*  \param          [in]    mailboxHnd       The mailbox handle
*
*  \pre            The context must be a plugin context rather than an
*                  application context.
*
*  \post           None
*
*  \return         Currently executing message or A2B_NULL if none
*
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_Msg*
a2b_msgRtrGetExecutingMsg
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  mailboxHnd
    )
{
    if ( ctx && (A2B_DOMAIN_PLUGIN == ctx->domain) )
    {
        a2b_JobQueue*   jobQ;

        /* Lookup the mailbox */
        jobQ = (a2b_JobQueue*)a2b_stackCtxMailboxFind( ctx, mailboxHnd );
        if ( jobQ )
        {
            return (struct a2b_Msg*)SIMPLEQ_FIRST(&jobQ->qHead);
        }
    }

    return A2B_NULL;

} /* a2b_msgExecUpdate */
