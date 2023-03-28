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
 * \file:   msg.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the public API for A2B stack messages.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_msg           Message Module
 *  
 * This defines the public API for A2B stack message creation and handling.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_MSG_H_
#define A2B_MSG_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/defs.h"
#include "a2b/msgtypes.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msg_defs      Types/Defs
 *  
 * The various defines and data types used within the message modules.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Msg;
struct a2b_StackContext;

typedef void (A2B_CALL * a2b_MsgCallbackFunc)(struct a2b_Msg* msg,
                                                a2b_Bool isCancelled);
typedef void (A2B_CALL * a2b_MsgDestroyFunc)(struct a2b_Msg* msg);

/** \} -- a2bstack_msg_defs */


/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_msg_mgmt      Alloc/Ref
 *  
 * These functions are used to allocate and manage the lifetime 
 * of a message.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC struct a2b_Msg* A2B_CALL a2b_msgAlloc(
                                            struct a2b_StackContext* ctx,
                                            a2b_UInt32               type,
                                            a2b_UInt32               cmd );

A2B_DSO_PUBLIC void A2B_CALL a2b_msgRef(struct a2b_Msg* msg);
A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL  a2b_msgUnref(struct a2b_Msg* msg);

/** \} -- a2bstack_msg_mgmt */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_msg_setter    Setter Functions
 *  
 * These functions are "setter" cover routines for various 
 * fields within the message. 
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC void A2B_CALL a2b_msgSetUserData(
                                struct a2b_Msg*         msg,
                                a2b_Handle              userData,
                                a2b_MsgDestroyFunc      destroy );

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_msgSetType( struct a2b_Msg* msg,
                                                      a2b_UInt32    type);
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_msgSetCmd( struct a2b_Msg*  msg,
                                                     a2b_UInt32     cmd );
A2B_DSO_PUBLIC void A2B_CALL a2b_msgSetTid( struct a2b_Msg* msg,
                                              a2b_UInt32    tid );
/** \} -- a2bstack_msg_setter */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_msg_getter    Getter Functions
 *  
 * These functions are "getter" cover routines for various 
 * fields within the message. 
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_msgGetType( struct a2b_Msg* msg );
A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_msgGetCmd( struct a2b_Msg* msg );
A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_msgGetTid( struct a2b_Msg* msg );
A2B_DSO_PUBLIC a2b_Handle A2B_CALL a2b_msgGetUserData( struct a2b_Msg* msg );
A2B_DSO_PUBLIC a2b_Handle A2B_CALL a2b_msgGetPayload( struct a2b_Msg* msg );
A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_msgGetPayloadCapacity();
A2B_DSO_PUBLIC a2b_Int16 A2B_CALL a2b_msgGetDestNodeAddr(
                                                        struct a2b_Msg* msg );

/** \} -- a2bstack_msg_getter */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_msg */

#endif /* A2B_MSG_H_ */
