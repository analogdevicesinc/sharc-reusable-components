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
 * \file:   stackctxmailbox.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the code that handles mailbox handling for a stack context.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_stackctxmailbox        Stack Context Mailbox Module
 *  
 * This code that handles mailbox handling for a stack context.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_STACKCTXMAILBOX_H_
#define A2B_STACKCTXMAILBOX_H_

/*======================= I N C L U D E S =========================*/

#include "defs.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;
struct a2b_JobQueue;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_DSO_PUBLIC a2b_Handle A2B_CALL a2b_stackCtxMailboxAlloc(
                                                struct a2b_StackContext* ctx,
                                                a2b_JobPriority priority);

A2B_DSO_PUBLIC a2b_Bool A2B_CALL a2b_stackCtxMailboxFree(
                                                struct a2b_StackContext* ctx,
                                                a2b_Handle mailboxHnd);

A2B_DSO_PUBLIC void A2B_CALL a2b_stackCtxMailboxFreeAll(
                                                struct a2b_StackContext* ctx);

A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_stackCtxMailboxCount(
                                        struct a2b_StackContext* ctx);

A2B_DSO_PUBLIC struct a2b_JobQueue* A2B_CALL a2b_stackCtxMailboxFind(
                                                struct a2b_StackContext* ctx,
                                                a2b_Handle mailboxHnd);

A2B_DSO_PUBLIC a2b_Bool A2B_CALL a2b_stackCtxMailboxFlush(
                                                struct a2b_StackContext* ctx,
                                                a2b_Handle mailboxHnd);


A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_stackctxmailbox */

#endif /* A2B_STACKCTXMAILBOX_H_ */
