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
 * \file:   msg_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the private code that handles messages
 *          within the system.
 *
 *=============================================================================
 */

#ifndef A2B_MSG_PRIV_H_
#define A2B_MSG_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "queue.h"
#include "job.h"
#include "a2b/msg.h"
#include "a2b/features.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Stack;
struct a2b_StackContext;
struct a2b_NodeInfo;

/**
 * This defines the union of all possible payloads for messages. 
 *  
 * \attention 
 * Care should be taken when adding payload types to this union. 
 * #a2b_Msg defines a payload to accommodate the largest possible 
 * payload.  So if you add a large payload to this union EVERY 
 * reserved/allocated #a2b_Msg will reserve payload space for 
 * this message and can bloat memory usage.  One solution can 
 * be to instead pass pointers using <em>hnd</em>. 
 */
typedef union a2b_MsgPayload
{
    /** Network discovery payload */
    a2b_NetDiscovery                disc;

    /** Plugin version information */
    a2b_PluginVerInfo               verInfo;

    /** Plugin Init payload */
    a2b_PluginInit                  pluginInit;

    /** Plugin Deinit payload */
    a2b_PluginDeinit                pluginDeinit;

    /** Custom argument payload for an opaque pointer */
    a2b_Handle                      hnd;

    /** Custom argument payload for two uint32 values */
    a2b_UInt32                      param[2];

    /** Simple payload carrying results */
    a2b_HResult                     result;

    /** Power fault notification payload */
    a2b_PowerFault                  powerFault;

    /** Interrupt notification payload */
    a2b_Interrupt                   interrupt;

    /** Discovery status notification payload */
    a2b_DiscoveryStatus             discStatus;

} a2b_MsgPayload;


typedef struct a2b_Msg
{
    /** Job header needed to properly pass the message.
     *  
     * \attention 
     * Must be the first field of this structure.
     */
    a2b_Job                     job;

    /** A2B Stack context (used for Trace, etc).  This is
     *  typically the context of the allocator of the message
     *  NOT the context of the destination.
     */
    struct a2b_StackContext*    ctx;

    /** Ref count value for the message.  This is used to
     *  avoid freeing the message prematurely.
     */
    a2b_UInt32                  refCnt;

    /** Specific type of command: #A2B_MSG_REQUEST, #A2B_MSG_NOTIFY, etc */
    a2b_UInt32                  type;

    /** Specific command of a "type" to execute.
     *  (A2B_MSGREQ..., A2B_MSGNOTIFY...)
     */
    a2b_UInt32                  cmd;

    /** An opaque pointer to user data that will be returned on
     *  an #onComplete response callback.  This value is only set
     *  through #a2b_msgSetUserData().  You can get the value
     *  using ##a2b_msgGetUserData().
     */
    a2b_Handle                  userData;

    /** An optional destructor that can be used to destroy/free user
     *  data.  This value is only set through #a2b_msgSetUserData().
     */
    a2b_MsgDestroyFunc          destroy;

#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
    /** Provides a means to intercept and hook the job completion callback.
     *  This value now makes it possible to uniformly log when a job calls
     *  "onComplete".  This variable tracks the users defined completion
     *  routine, the jobs "onComplete" will be set to
     *  #a2b_msgRtrOnJobComplete which will then call this routine.
     */
    void (* onComplete)(struct a2b_Job* job, a2b_Bool isCancelled);
#endif

    /** This is a user defined transaction identifier. This has no meaning
     *  to the receiver of the message.  This value is initialized to zero.
     *  (N/A for notification messages)
     */
    a2b_UInt32                  tid;

    /** This is the destination NodeAddr to identify where the
     *  request should ultimately be routed and handled.
     *  (N/A for notification messages, but makes the code more
     *  clean and easier to handle custom request messages)
     */
    a2b_Int16                   destNodeAddr;

    /** This is filled in by the msgrtr only for well-known or
     *  custom message requests.
     */
    struct a2b_StackContext*    destCtx;

    /** Defined the actual payload bytes.  This is the largest
     *  payload allowed.  The size is the largest value of:
     *  sizeof(#a2b_MsgPayload) or #A2B_CONF_MSG_MIN_PAYLOAD_SIZE.
     */
    a2b_Byte                    payload[A2B_MAX(sizeof(a2b_MsgPayload),
                                        A2B_CONF_MSG_MIN_PAYLOAD_SIZE)];
} a2b_Msg;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_MSG_PRIV_H_ */
