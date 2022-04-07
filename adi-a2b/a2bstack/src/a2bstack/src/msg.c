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
 * \file:   msg.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of A2B message specific APIs.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/ctypes.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/trace.h"
#include "a2b/seqchart.h"
#include "stack_priv.h"
#include "stackctx.h"
#include "a2b/pluginapi.h"
#include "a2b/msg.h"
#include "a2b/stackctxmailbox.h"
#include "msg_priv.h"
#include "utilmacros.h"
#include "a2b/util.h"
#include "jobexec.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/


/*!****************************************************************************
*
*  \b              a2b_msgAlloc
*
*  Allocate a message for use in the message router.
* 
*  @image html msgrtr_msgreq.png "Request Example"
* 
*  @image html msgrtr_msgnotify.png "Notification Example"
* 
*  \param          [in]    ctx         The A2B stack context 
*  \param          [in]    type        Type of message (#A2B_MSG_REQUEST, etc)
*  \param          [in]    cmd         command ID
*
*  \pre            None
*
*  \post           refCnt of the returned a2b_Msg is ref counted to 1.
*
*  \return         A2B_NULL if no memory available,
*                  non-NULL for a valid message
*
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_Msg*
a2b_msgAlloc
    (
    struct a2b_StackContext* ctx,
    a2b_UInt32               type,
    a2b_UInt32               cmd
    )
{
    a2b_Msg*    msg = A2B_NULL;

    if ( A2B_NULL != ctx )
    {
    	A2B_TRACE3((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
    	                    "Enter: a2b_msgAlloc(0x%p)  (type: %ld cmd: %ld)", ctx,
    	                    &type, &cmd));

        if ( ( type <= A2B_MSG_FIRST ) || ( type >= A2B_MSG_MAX ) )
        {
            A2B_TRACE1((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_ERROR),
                        "Invalid cmd: %ld", &cmd));
            return A2B_NULL;
        }

        msg = (a2b_Msg*)A2B_MALLOC(ctx->stk, sizeof(*msg));
        if ( A2B_NULL != msg )
        {
            (void)a2b_memset(msg, 0, sizeof(*msg));
            msg->refCnt         = 1u;
            msg->ctx            = ctx;
            msg->type           = type;
            
            msg->userData       = A2B_NULL;
            msg->destroy        = A2B_NULL;
            msg->tid            = 0u;
            msg->destNodeAddr   = A2B_NODEADDR_NOTUSED;
            msg->destCtx        = A2B_NULL;

            msg->job.execute    = A2B_NULL;
            msg->job.destroy    = A2B_NULL;
            msg->job.onComplete = A2B_NULL;

            if (a2b_msgSetCmd(msg,cmd) != A2B_RESULT_SUCCESS)
            {
                A2B_TRACE1((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_ERROR),
                            "Invalid cmd: %ld", &cmd));
                A2B_FREE( ctx->stk, msg );
                return A2B_NULL;
            }
        }

        A2B_TRACE2((ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_msgAlloc(0x%p): 0x%p", ctx, msg));
    }
    return msg;

} /* a2b_msgAlloc */


/*!****************************************************************************
*
*  \b              a2b_msgRef
*
*  Increment the reference count of the message. Ref counting the
*  message will prevent the message from being freed.
*
*  \param          [in]    msg    A2B message instance
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_msgRef
    (
    struct a2b_Msg* msg
    )
{
    if ( A2B_NULL != msg )
    {
        A2B_TRACE2((msg->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_msgRef(0x%p) : RefCnt=%ld", 
                    msg, &msg->refCnt));

        msg->refCnt++;

        A2B_TRACE2((msg->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_msgRef(0x%p) : RefCnt=%ld",
                    msg, &msg->refCnt));
    }

} /* a2b_msgRef */


/*!****************************************************************************
*
*  \b              a2b_msgUnref
*
*  Decrement the reference count of the message. When the reference count
 * reaches zero (0) the message is de-initialized and deallocated and
 * returned to the pool of available messages.
*
*  \param          [in]    msg    A2B message
*
*  \pre            None
*
*  \post           None
*
*  \return         Ref count value after the unref process
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_msgUnref
    (
    struct a2b_Msg* msg
    )
{
    a2b_UInt32 refCnt = 0u;

    if ( A2B_NULL != msg )
    {
        A2B_TRACE2((msg->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_msgUnref(0x%p) : RefCnt=%ld", 
                    msg, &msg->refCnt));

        if ( 0u == msg->refCnt )
        {
            A2B_TRACE1((msg->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_ERROR),
                        "Message (0x%p) is already unreferenced", msg));
        }
        else
        {
            msg->refCnt--;
            refCnt = msg->refCnt;

            A2B_TRACE2((msg->ctx, (A2B_TRC_DOM_MSGRTR | A2B_TRC_LVL_TRACE1),
                        "Exit: a2b_msgUnref(0x%p) : RefCnt=%ld",
                        msg, &msg->refCnt));

            if ( 0u == msg->refCnt )
            {
                /* See if we need to destroy the userData */
                if ( msg->destroy )
                {
                    msg->destroy( msg );
                }
                msg->userData = A2B_NULL;

                A2B_FREE( msg->ctx->stk, msg );                
            }
        }
    }

    return refCnt;

} /* a2b_msgUnref */


/*------------------*/
/* SETTER FUNCTIONS */
/*------------------*/

/*!****************************************************************************
*
*  \b              a2b_msgSetUserData
*
*  Set the user data for a message.
* 
*  Getter routine: #a2b_msgGetUserData 
*
*  \param          [in]    msg         A2B message instance
* 
*  \param          [in]    userData    An opaque pointer to user defined data
* 
*  \param          [in]    destroy     Function called then the message
*                                      is destroyed. This gives the creator
*                                      of the message a chance to free/cleanup
*                                      its userData.
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_msgSetUserData
    (
    struct a2b_Msg*         msg,
    a2b_Handle              userData,
    a2b_MsgDestroyFunc      destroy
    )
{
    if ( msg )
    {
        msg->userData       = userData;
        msg->destroy        = destroy;
    }

} /* a2b_msgSetUserData */


/*!****************************************************************************
*
*  \b              a2b_msgSetType
*
*  Set the type for the message
* 
*  Getter routine: #a2b_msgGetType
*  
*  \param          [in]    msg     A2B message instance
* 
*  \param          [in]    type    Type of message (#A2B_MSG_REQUEST, etc)
*
*  \pre            None
*
*  \post           None
*
*  \return         #A2B_EC_INVALID_PARAMETER if msg == NULL or type out of range
*                  else, #A2B_RESULT_SUCCESS
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_msgSetType
    ( 
    struct a2b_Msg* msg, 
    a2b_UInt32      type
    )
{
    if ( msg )
    {
        if (( type > A2B_MSG_FIRST ) && ( type < A2B_MSG_MAX ))
        {
            msg->type = type;
            return A2B_RESULT_SUCCESS;
        }
    }
    return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR, 
                            A2B_EC_INVALID_PARAMETER);
} /* a2b_msgSetType */


/*!****************************************************************************
*
*  \b              a2b_msgSetCmd
*
*  Set the command for the message.
* 
*  Getter routine: #a2b_msgGetCmd
*
*  \param          [in]    msg    A2B message instance
* 
*  \param          [in]    cmd    Command (A2B_MSGREQ..., A2B_MSGNOTIFY...)
*
*  \pre            None
*
*  \post           None
*
*  \return         #A2B_EC_INVALID_PARAMETER if msg == NULL or cmd out of range
*                  else, #A2B_RESULT_SUCCESS
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_msgSetCmd
    ( 
    struct a2b_Msg* msg,
    a2b_UInt32      cmd 
    )
{
    a2b_HResult ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_MSGRTR, 
                                       A2B_EC_INVALID_PARAMETER);
    if ( msg )
    {
        switch (msg->type)
        {
        case A2B_MSG_REQUEST:
            if ((( cmd > A2B_MSGREQ_FIRST ) && ( cmd < A2B_MSGREQ_MAX )) ||
                ( cmd >= A2B_MSGREQ_CUSTOM ))
            {
                ret = A2B_RESULT_SUCCESS;
            }
            break;

        case A2B_MSG_NOTIFY:
            if ((( cmd > A2B_MSGNOTIFY_FIRST ) && 
                 ( cmd < A2B_MSGNOTIFY_MAX )) ||
                ( cmd >= A2B_MSGNOTIFY_CUSTOM ))
            {
                ret = A2B_RESULT_SUCCESS;
            }
            break;

        default:
            break;
        }
    }

    if ( A2B_SUCCEEDED(ret) )
    {
        msg->cmd = cmd;
    }

    return ret;

} /* a2b_msgSetCmd */


/*!****************************************************************************
*
*  \b              a2b_msgSetTid
*
*  Set the transaction identifier (TID) for the message.  This is simply
*  a a2b_UInt32 that can be used for any purpose.  This has no meaning
*  to the receiver of the message.  
* 
*  Getter routine: #a2b_msgGetTid
* 
*  \param          [in]    msg    A2B message instance
* 
*  \param          [in]    tid    Transaction ID
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_msgSetTid
    (
    struct a2b_Msg* msg,
    a2b_UInt32      tid 
    )
{
    if ( msg )
    {
        msg->tid = tid;
    }
} /* a2b_msgSetTid */



/*------------------*/
/* GETTER FUNCTIONS */
/*------------------*/


/*!****************************************************************************
*
*  \b              a2b_msgGetType
*
*  Get the message type (#A2B_MSG_REQUEST, etc)
* 
*  Setter routine: #a2b_msgSetType
*
*  \param          [in]    msg    A2B message instance
*
*  \pre            None
*
*  \post           None
*
*  \return         #A2B_MSG_UNKNOWN if msg == NULL,
*                  else type of message
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_msgGetType
    ( 
    struct a2b_Msg* msg
    )
{
    if ( msg )
    {
        return msg->type;
    }
    return A2B_MSG_UNKNOWN;
} /* a2b_msgGetType */


/*!****************************************************************************
*
*  \b              a2b_msgGetCmd
*
*  Get the message command (A2B_MSGREQ..., A2B_MSGNOTIFY...)
* 
*  Setter routine: #a2b_msgSetCmd
* 
*  \param          [in]    msg    A2B message instance
*
*  \pre            None
*
*  \post           None
*
*  \return         #A2B_MSGREQ_UNKNOWN if msg == NULL,
*                  else command of message
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_msgGetCmd
    ( 
    struct a2b_Msg* msg
    )
{
    if ( msg )
    {
        return msg->cmd;
    }
    return A2B_MSGREQ_UNKNOWN;
} /* a2b_msgGetCmd */


/*!****************************************************************************
*
*  \b              a2b_msgGetDestNodeAddr
*
*  Retrieves the node address of the message destination.
* 
*  This function is typically used by a plugin to retrieve pertinent
*  information associated with a receive message (request) that is being
*  processed by the plugn.
*
*  \param          [in]    msg    A2B message instance
*
*  \pre            Not relevant for a Notify message
* 
*  \pre            This message is only valid after the call
*                  to #a2b_msgRtrSendRequest.
*
*  \post           None
*
*  \return         #A2B_NODEADDR_NOTUSED if msg == NULL and not a
*                  notify message, else destination nodeAddr of message
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Int16
a2b_msgGetDestNodeAddr
    ( 
    struct a2b_Msg* msg
    )
{
    if ( msg && (msg->type != A2B_MSG_NOTIFY) )
    {
        return msg->destNodeAddr;
    }
    return A2B_NODEADDR_NOTUSED;
} /* a2b_msgGetDestNodeAddr */


/*!****************************************************************************
*
*  \b              a2b_msgGetTid
*
*  Get the transaction ID of the message
* 
*  More details found on setter routine: #a2b_msgSetTid
*
*  \param          [in]    msg    A2B message instance
*
*  \pre            None
*
*  \post           Always zero for notify message types
*
*  \return         0 if msg == NULL,
*                  else transaction ID of message
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_msgGetTid
    ( 
    struct a2b_Msg* msg
    )
{
    if ( msg )
    {
        return msg->tid;
    }
    return 0u;
} /* a2b_msgGetTid */


/*!****************************************************************************
*
*  \b              a2b_msgGetUserData
*
*  Get the user data out of the message
* 
*  Setter routine: #a2b_msgSetUserData 
* 
*  \param          [in]    msg    A2B message instance
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_NULL if msg == NULL,
*                  else userData of message
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Handle
a2b_msgGetUserData
    ( 
    struct a2b_Msg* msg
    )
{
    if ( msg )
    {
        return msg->userData;
    }
    return A2B_NULL;
} /* a2b_msgGetUserData */


/*!****************************************************************************
*
*  \b              a2b_msgGetPayload
*
*  Grab the specific messages types payload of data.
*
*  \param          [in]    msg    A2B message instance
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_NULL if msg == NULL,
*                  else payload of message
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Handle
a2b_msgGetPayload
    ( 
    struct a2b_Msg* msg 
    )
{
    if ( msg )
    {
        return msg->payload;
    }

    return A2B_NULL;

} /* a2b_msgGetPlayload */


/*!****************************************************************************
*
*  \b              a2b_msgGetPayloadCapacity
*
*  Returns the maximum capacity (in bytes) reserved for payload area of a
*  message.
*
*  \pre            None
*
*  \post           None
*
*  \return         The maximum capacity (in bytes) of the area reserved for
*                  the message payload.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_msgGetPayloadCapacity(void)
{
    return sizeof(((a2b_Msg*)0)->payload);
} /* a2b_msgGetPayloadCapacity */

