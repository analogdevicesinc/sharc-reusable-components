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
 * \file:   periphutil.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of utility routines for slave plugins and
 *          peripherals.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/
#include "a2b/pluginapi.h"
#include "periphutil.h"

#include "a2b/defs.h"
#include "a2b/msg.h"
#include "a2b/msgrtr.h"
#include "a2b/msgtypes.h"
#include "a2b/error.h"
#include "a2b/trace.h"
#include "a2b/tracectl.h"
#include "plugin_priv.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static a2b_Int32 a2b_periphSendMsgNextNode(struct a2b_Plugin* plugin,
a2b_UInt32 msgId, a2b_MsgCallbackFunc func, a2b_HResult* status);
static void a2b_periphReqDone(struct a2b_Msg* msg,a2b_Bool isCancelled);
static a2b_Int32 a2b_periphExecOp(struct a2b_Plugin*      plugin,
    a2b_UInt32              msgId,
    a2b_Int16               startNode,
    a2b_Int16               endNode,
    a2b_OpDoneFunc          cb,
    a2b_Handle              userData,
    a2b_HResult*            status);

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
 *
 * \b   a2b_periphSendMsgNextNode
 *
 * Sends a request to a slave plugin asking it to either initialize or
 * de-initialize it's peripherals. This function will skip over slave nodes
 * that don't have a plugin associated with it. For the case of initializing
 * slave node peripherals, the first failure will cause the process to stop. In
 * case the request is to de-initialize a node it will attempt to de-initialize
 * nodes until there is a catastrophic failure of some type.
 *
 * \param   [in]        plugin  The master plugin context.
 *
 * \param   [in]        msgId   The request message identifier to be sent to
 *                              the slave plugin. Should be either
 *                              A2B_MSGREQ_PLUGIN_PERIPH_INIT or
 *                              A2B_MSGREQ_PLUGIN_PERIPH_DEINIT.
 *
 * \param   [in]        func    The callback function to be called when their
 *                              is a response to the initial request to the
 *                              slave plugin.
 *
 * \param   [in,out]    status  A pointer to a variable to hold the result
 *                              of the call. It can be used to evaluate the
 *                              success or failure of the request to send
 *                              the message.
 *
 * \pre     This function should only be called as part of the serialized
 *          sequence to either initialize or de-initialize the peripherals
 *          attached to slave plugins. The state/context of the process is
 *          held within the plugin instance.
 *
 * \post    None
 *
 * \return  Returns an indication of whether the master plugin should
 *          suspend scheduling or indicate that processing is complete. Either
 *          A2B_EXEC_COMPLETE or A2B_EXEC_SUSPEND is returned.
 *
 *****************************************************************************/
static a2b_Int32
a2b_periphSendMsgNextNode
    (
    struct a2b_Plugin*      plugin,
    a2b_UInt32              msgId,
    a2b_MsgCallbackFunc     func,
    a2b_HResult*            status
    )
{
    a2b_Int32 execStatus = A2B_EXEC_COMPLETE;
    a2b_HResult ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                        A2B_PLUGIN_EC_BAD_ARG);
    a2b_Bool tryAgain = A2B_FALSE;
    struct a2b_Msg* msg;

    msg = a2b_msgAlloc(plugin->ctx, A2B_MSG_REQUEST, msgId);
    if ( A2B_NULL == msg )
    {
        /* This is a fatal error even if we're trying to de-initialize
         * slave plugins. The process stops here because we couldn't even
         * allocate a message.
         */
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "a2b_periphSendMsgNextNode: failed to allocate request"));
        ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                A2B_EC_ALLOC_FAILURE);
        plugin->slaveInitCtx.lastError = ret;
    }
    else
    {
        a2b_msgSetUserData(msg, plugin, A2B_NULL);

        /* Try to send the message until either one is sent or there
         * is a legitimate error in sending the message to the slave
         * plugin. Note: If a node doesn't have a plugin then we just
         * skip it. Also, we always try to send a peripheral de-initialization
         * message to every slave plugin regardless of whether a prior one in
         * the network fails to de-initialize itself.
         */
        do
        {
            a2b_msgSetTid(msg, (a2b_UInt32)plugin->slaveInitCtx.curNode);
            ret = a2b_msgRtrSendRequest(msg, plugin->slaveInitCtx.curNode,
                                        func);
            if ( A2B_SUCCEEDED(ret) )
            {
                execStatus = A2B_EXEC_SUSPEND;
                tryAgain = A2B_FALSE;
            }
            else
            {
                plugin->slaveInitCtx.curNode += 1;

                /* If sending the message failed because of an unexpected
                 * error then ...
                 */
                if ( (A2B_ERR_CODE(ret) != (a2b_UInt32)A2B_EC_DOES_NOT_EXIST) &&
                    (A2B_ERR_CODE(ret) != (a2b_UInt32)A2B_EC_RESOURCE_UNAVAIL) )
                {
                    plugin->slaveInitCtx.lastError = ret;

                    /* We ignore errors when de-initializing slave plugins
                     * to make sure an attempt is made to de-initialize all
                     * the plugin's peripherals.
                     */
                    if ( A2B_MSGREQ_PLUGIN_PERIPH_DEINIT == msgId )
                    {
                        tryAgain = A2B_TRUE;
                    }
                    /* Else a message to initialize a slave plugin's
                     * peripherals.
                     */
                    else
                    {
                        /* We do *not* try to send to the next slave node if
                         * we encountered an unexpected error.
                         */
                        tryAgain = A2B_FALSE;
                    }
                }
                /* Assume we sent a message or got an error indicating the
                 * slave node doesn't have a plugin
                 */
                else
                {
                    /* If we've already processed all the slave nodes */
                    if ( plugin->slaveInitCtx.curNode >=
                        plugin->slaveInitCtx.lastNode )
                    {
                        tryAgain = A2B_FALSE;
                        ret = A2B_RESULT_SUCCESS;
                        execStatus = A2B_EXEC_COMPLETE;
                    }
                    /* Else there are more slave nodes to process */
                    else
                    {
                        tryAgain = A2B_TRUE;
                    }
                }
            }
        }
        while ( tryAgain );

        /* We no longer need a reference to the message since either
         * the queue owns it, there was an error and we're done with it, or all
         * the slave nodes have been processed.
         */
        (void)a2b_msgUnref(msg);
    }

    if ( A2B_NULL != status )
    {
        if ( execStatus == A2B_EXEC_COMPLETE )
        {
            *status = plugin->slaveInitCtx.lastError;
        }
        else
        {
            *status = A2B_RESULT_SUCCESS;
        }
    }

    return execStatus;
}


/*!****************************************************************************
 *
 * \b   a2b_periphReqDone
 *
 * This function is called when a slave plugin responds to the request to
 * either initialize or de-initialize it's attached peripherals. Based on the
 * request and response it may schedule another request to be made to
 * succeeding slave plugins.
 *
 * \param   [in]    msg         The response received from the slave plugin that
 *                              originally got the peripheral init/de-init
 *                              request.
 *
 * \param   [in]    isCancelled Indicates whether the peripheral request
 *                              was cancelled before completing.
 *
 * \pre     This function should only be called as the response callback
 *          handler.
 *
 * \post    None
 *
 * \return  None
 *
 *****************************************************************************/
static void
a2b_periphReqDone
    (
    struct a2b_Msg* msg,
    a2b_Bool        isCancelled
    )
{
    struct a2b_Plugin* plugin;
    a2b_HResult* result;
    a2b_Int32 execStatus;
    a2b_HResult ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                        A2B_PLUGIN_EC_BAD_ARG);

    if ( A2B_NULL != msg )
    {
        /* We need to either send a message to the next slave node
         * or call the callback if we're done or there was an unexpected
         * error.
         */
        plugin = a2b_msgGetUserData(msg);
        result = (a2b_HResult*)a2b_msgGetPayload(msg);

        if ( isCancelled )
        {
            A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                "a2b_periphReqDone: request was cancelled for "
                "slave node %hd", &plugin->slaveInitCtx.curNode));
            if ( plugin->slaveInitCtx.opDone != A2B_NULL )
            {
                plugin->slaveInitCtx.opDone(
                    A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                     A2B_EC_CANCELLED),
                                     plugin->slaveInitCtx.userData);
            }
        }
        else if ( A2B_NULL == result )
        {
            A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                "a2b_periphReqDone: failed to receive results for "
                "slave node %hd", &plugin->slaveInitCtx.curNode));
            if ( plugin->slaveInitCtx.opDone != A2B_NULL )
            {
                plugin->slaveInitCtx.opDone(
                    A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                     A2B_EC_RESOURCE_UNAVAIL),
                                     plugin->slaveInitCtx.userData);
            }
        }
        else if ( plugin->slaveInitCtx.opDone != A2B_NULL )
        {
            if ( a2b_msgGetCmd(msg) == A2B_MSGREQ_PLUGIN_PERIPH_DEINIT )
            {
                if ( A2B_FAILED(*result) )
                {
                    plugin->slaveInitCtx.lastError = *result;
                }


                if ( plugin->slaveInitCtx.curNode >=
                    plugin->slaveInitCtx.lastNode )
                {
                    plugin->slaveInitCtx.opDone(plugin->slaveInitCtx.lastError,
                                             plugin->slaveInitCtx.userData);
                }
                else
                {
                    execStatus = a2b_periphSendMsgNextNode(plugin,
                                            a2b_msgGetCmd(msg),
                                            &a2b_periphReqDone, &ret);
                    if ( execStatus == A2B_EXEC_COMPLETE )
                    {
                        if ( A2B_FAILED(ret) )
                        {
                            plugin->slaveInitCtx.lastError = ret;
                        }
                        plugin->slaveInitCtx.opDone(
                                            plugin->slaveInitCtx.lastError,
                                            plugin->slaveInitCtx.userData);
                    }
                }
            }
            /* Else the original request was to initialize attached
             * peripherals.
             */
            else
            {
                if ( A2B_FAILED(*result) )
                {
                    plugin->slaveInitCtx.opDone(*result,
                                             plugin->slaveInitCtx.userData);
                }
                else
                {
                    if ( plugin->slaveInitCtx.curNode >=
                        plugin->slaveInitCtx.lastNode )
                    {
                        plugin->slaveInitCtx.opDone(*result,
                                             plugin->slaveInitCtx.userData);
                    }
                    else
                    {
                        execStatus = a2b_periphSendMsgNextNode(plugin,
                                                a2b_msgGetCmd(msg),
                                                &a2b_periphReqDone, &ret);
                        if ( A2B_FAILED(ret) ||
                            (execStatus == A2B_EXEC_COMPLETE) )
                        {
                            plugin->slaveInitCtx.opDone(ret,
                                             plugin->slaveInitCtx.userData);
                        }
                    }
                }
            }
        }
        else
        {
            /* Completing the control statement */
        }
    }
}


/*!****************************************************************************
 *
 * \b   a2b_periphExecOp
 *
 * This function executes the requested operation(s) to either initialize or
 * de-initialize the peripherals attached to slave plugins. The function
 * essentially kicks-off the operation which may take several iterations
 * depending on the start/end node addresses that are specified.
 *
 * \param   [in]        plugin      The plugin context for the master node.
 *
 * \param   [in]        msgId       The message identifier of the request
 *                                  to send. Should be one of the following:
 *                                  A2B_MSGREQ_PLUGIN_PERIPH_INIT or
 *                                  A2B_MSGREQ_PLUGIN_PERIPH_DEINIT.
 *
 * \param   [in]        startNode   The starting node address of the slave
 *                                  plugin to receive a request.
 *
 * \param   [in]        endNode     The ending node address of the slave
 *                                  plugin to receive a request. Must be
 *                                  >= than the startNode.
 *
 * \param   [in]        cb          The callback function to call when
 *                                  all the slave plugins have been processed.
 *
 * \param   [in]        userData    Data provided by the caller which will
 *                                  be returned in the completion callback.
 *
 * \param   [in,out]    status      A variable to hold the result of kicking
 *                                  off the execution of the slave plugin
 *                                  peripheral initialization /
 *                                  de-initialization. Can be used to tell
 *                                  whether or not the operation succeed.
 *
 * \pre     This function should only be called internally once to start the
 *          process and should not be called again until the process is
 *          complete.
 *
 * \post    None
 *
 * \return  Returns an indication of whether the master plugin should
 *          suspend scheduling or that processing is complete. Either
 *          A2B_EXEC_COMPLETE or A2B_EXEC_SUSPEND is returned.
 *
 *****************************************************************************/
static a2b_Int32
a2b_periphExecOp
    (
    struct a2b_Plugin*      plugin,
    a2b_UInt32              msgId,
    a2b_Int16               startNode,
    a2b_Int16               endNode,
    a2b_OpDoneFunc          cb,
    a2b_Handle              userData,
    a2b_HResult*            status
    )
{
    a2b_Int32 execStatus = A2B_EXEC_COMPLETE;
    a2b_HResult ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                        A2B_PLUGIN_EC_BAD_ARG);

    if ( (A2B_NULL != plugin) && (A2B_NULL != cb) &&
       (startNode <= endNode) )
    {
        plugin->slaveInitCtx.curNode = startNode;
        plugin->slaveInitCtx.lastNode = endNode;
        plugin->slaveInitCtx.userData = userData;
        plugin->slaveInitCtx.lastError = A2B_RESULT_SUCCESS;
        plugin->slaveInitCtx.opDone = cb;

        execStatus = a2b_periphSendMsgNextNode(plugin,
                                                msgId,
                                                &a2b_periphReqDone,
                                                &ret);
    }

    if ( A2B_NULL != status )
    {
        *status = ret;
    }

    return execStatus;
}


/*!****************************************************************************
 *
 * \b   a2b_periphInit
 *
 * This function executes the requested operation(s) to initialize
 * the peripherals attached to slave plugins. The function essentially
 * kicks-off the operation which may take several iterations depending on the
 * start/end node addresses that are specified. Once started, this function
 * *SHOULD NOT* be called again until it either returns A2B_EXEC_COMPLETE or
 * the provided callback function is called at the termination of the process.
 *
 * \param   [in]        plugin      The plugin context for the master node.
 *
 * \param   [in]        startNode   The starting node address of the slave
 *                                  plugin to receive a request.
 *
 * \param   [in]        endNode     The ending node address of the slave
 *                                  plugin to receive a request. Must be
 *                                  >= than the startNode.
 *
 * \param   [in]        cb          The callback function to call when
 *                                  all the slave plugins have been processed.
 *
 * \param   [in]        userData    Data provided by the caller which will
 *                                  be returned in the completion callback.
 *
 * \param   [in,out]    status      A variable to hold the result of kicking
 *                                  off the execution of the slave plugin
 *                                  peripheral initialization /
 *                                  de-initialization. Can be used to tell
 *                                  whether or not the operation succeed.
 *
 * \pre     This function should only be called once to start the process and
 *          should not be called again until the process is complete.
 *
 * \post    None
 *
 * \return  Returns an indication of whether the master plugin should
 *          suspend scheduling or the processing is complete. Either
 *          A2B_EXEC_COMPLETE or A2B_EXEC_SUSPEND is returned. Suspended
 *          execution means the master plugin should not process another
 *          message until the provided callback is invoked indicating the
 *          success/failure of the operation.
 *
 *****************************************************************************/
a2b_Int32
a2b_periphInit
    (
    struct a2b_Plugin*      plugin,
    a2b_Int16               startNode,
    a2b_Int16               endNode,
    a2b_OpDoneFunc          cb,
    a2b_Handle              userData,
    a2b_HResult*            status
    )
{
    return a2b_periphExecOp(plugin,
                            A2B_MSGREQ_PLUGIN_PERIPH_INIT,
                            startNode,
                            endNode,
                            cb,
                            userData,
                            status);
}


/*!****************************************************************************
 *
 * \b   a2b_periphDeinit
 *
 * This function executes the requested operation(s) to de-initialize
 * the peripherals attached to slave plugins. The function essentially
 * kicks-off the operation which may take several iterations depending on the
 * start/end node addresses that are specified. Once started, this function
 * *SHOULD NOT* be called again until it either returns A2B_EXEC_COMPLETE or
 * the provided callback function is called at the termination of the process.
 *
 * \param   [in]        plugin      The plugin context for the master node.
 *
 * \param   [in]        startNode   The starting node address of the slave
 *                                  plugin to receive a request.
 *
 * \param   [in]        endNode     The ending node address of the slave
 *                                  plugin to receive a request. Must be
 *                                  >= than the startNode.
 *
 * \param   [in]        cb          The callback function to call when
 *                                  all the slave plugins have been processed.
 *
 * \param   [in]        userData    Data provided by the caller which will
 *                                  be returned in the completion callback.
 *
 * \param   [in,out]    status      A variable to hold the result of kicking
 *                                  off the execution of the slave plugin
 *                                  peripheral initialization /
 *                                  de-initialization. Can be used to tell
 *                                  whether or not the operation succeed.
 *
 * \pre     This function should only be called once to start the process and
 *          should not be called again until the process is complete.
 *
 * \post    None
 *
 * \return  Returns an indication of whether the master plugin should
 *          suspend scheduling or the processing is complete. Either
 *          A2B_EXEC_COMPLETE or A2B_EXEC_SUSPEND is returned. Suspended
 *          execution means the master plugin should not process another
 *          message until the provided callback is invoked indicating the
 *          success/failure of the operation.
 *
 *****************************************************************************/
a2b_Int32
a2b_periphDeinit
    (
    struct a2b_Plugin*      plugin,
    a2b_Int16               startNode,
    a2b_Int16               endNode,
    a2b_OpDoneFunc          cb,
    a2b_Handle              userData,
    a2b_HResult*            status
    )
{
    return a2b_periphExecOp(plugin,
                            A2B_MSGREQ_PLUGIN_PERIPH_DEINIT,
                            startNode,
                            endNode,
                            cb,
                            userData,
                            status);
}

