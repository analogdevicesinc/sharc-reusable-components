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
 * \file:   msgrtr.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the code that handles routing messages within the system.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_msgrtr        Message Router Module
 *  
 * This defines the public API for A2B stack message routing within
 * the A2B system (stack/application/plugins).
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_MSGRTR_H_
#define A2B_MSGRTR_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/msg.h"
#include "defs.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgrtr_defs       Types/Defs
 *  
 * The various defines and data types used within the message router module.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Msg;
struct a2b_StackContext;
struct a2b_NodeInfo;
struct a2b_MsgNotifier;

/** Notification callback must have this signature */
typedef void (A2B_CALL * a2b_NotifyFunc)(struct a2b_Msg*, a2b_Handle);

/** \} -- a2bstack_msgrtr_defs */

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgrtr_notification   Notifications
 *  
 * These are functions to un/register for message notifications within 
 * the A2B system (stack/application/plugins).
 *  
 * <!--
 * @startuml msgrtr_msgnotify.png
 * group App Registers for Notification
 * App -> MsgRtr: a2b_registerNotify( ctx,\n\t\t A2B_MSGNOTIFY_DISCOVERY_DONE, cb, ... )
 * App <- MsgRtr: A2B_RESULT_SUCCESS
 * end 
 * ||| 
 * group Master Plugin Processing
 * note over Plugin: Discovery Completed
 * Plugin -> MsgRtr: a2b_msgAlloc( ctx, A2B_MSG_NOTIFY,\n\t\t A2B_MSGNOTIFY_DISCOVERY_DONE ) 
 * note over App 
 * App will now populate the message for its own needs 
 * using: a2b_msgSetTid, a2b_msgSetUserData, etc 
 * end note  
 * Plugin <- MsgRtr: a2b_Msg* msg 
 * Plugin -> MsgRtr: a2b_msgRtrNotify( msg )
 * App <<-- Plugin: onNotify(...)
 * end
 * @enduml
 * --> 
 * @image html msgrtr_msgnotify.png "Notification Example"
 *  
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC struct a2b_MsgNotifier* A2B_CALL a2b_msgRtrRegisterNotify(
                                struct a2b_StackContext*    ctx,
                                a2b_UInt32                  cmd,
                                a2b_NotifyFunc              notify,
                                a2b_Handle                  userData,
                                a2b_CallbackFunc            destroyUserData );

A2B_DSO_PUBLIC void A2B_CALL a2b_msgRtrUnregisterNotify(
                                struct a2b_MsgNotifier*     notifier );

/** \} -- a2bstack_msgrtr_notification */

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgrtr_tx         Transmit
 *  
 * These are functions to transmit messages or notifications within 
 * the A2B system (stack/application/plugins).
 *  
 * <br> 
 * @image html msgrtr_msgnotify.png "Notification Example" 
 * <br><hr><br>
 *  
 * <!-- 
 * @startuml msgrtr_msgreq.png
 * App -> MsgRtr: a2b_msgAlloc( ctx, A2B_MSG_NOTIFY,\n A2B_MSGNOTIFY_DISCOVERY_DONE ) 
 * App <- MsgRtr: a2b_Msg* msg 
 * note over App 
 * App will now populate the message for its own needs 
 * using: a2b_msgSetTid, a2b_msgSetUserData, etc 
 * end note 
 * App -> MsgRtr: a2b_msgRtrSendRequest() 
 * note over MsgRtr 
 * The MsgRtr will route the message to the correct handler.
 * The MsgRtr will post this on the destination Job queue
 * and return.
 * end note 
 * App <- MsgRtr: A2B_RESULT_SUCCESS 
 * ||| 
 * note over MsgRtr 
 * Once the request has been serviced, call the user callback. 
 * end note 
 * App <<-- MsgRtr: onComplete
 * @enduml
 * -->
 * @image html msgrtr_msgreq.png "Request Example" 
 *  
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_msgRtrSendRequestToMailbox(
                                            struct a2b_Msg*       msg,
                                            a2b_Handle            mailboxHnd,
                                            a2b_MsgCallbackFunc   complete);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_msgRtrSendRequest(
                                            struct a2b_Msg*       msg,
                                            a2b_Int16             destNodeAddr,
                                            a2b_MsgCallbackFunc   complete);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_msgRtrNotify(struct a2b_Msg* msg);

/** \} -- a2bstack_msgrtr_tx */

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgrtr_misc       Misc
 *  
 * These are miscellaneous public helper functions.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/** Updates the execution action for the "active" message being processed */
A2B_DSO_PUBLIC void A2B_CALL a2b_msgRtrExecUpdate(
                                                struct a2b_StackContext* ctx,
                                                a2b_Handle mailboxHnd,
                                                a2b_Int32 action);

/** Retrieves the message currently being executed by the specified plugin
 * stack context.
 */
A2B_DSO_PUBLIC struct a2b_Msg* A2B_CALL a2b_msgRtrGetExecutingMsg(
                                                struct a2b_StackContext* ctx,
                                                a2b_Handle mailboxHnd);

A2B_DSO_PUBLIC const a2b_NodeSignature* A2B_CALL a2b_msgRtrGetHandler(
                                                struct a2b_StackContext*   ctx,
                                                a2b_UInt32*                idx);

/** \} -- a2bstack_msgrtr_misc */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_msgrtr */

#endif /* A2B_MSGRTR_H_ */
