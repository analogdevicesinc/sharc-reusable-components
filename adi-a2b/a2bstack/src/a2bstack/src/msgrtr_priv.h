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
 * \brief:  This is the private code that handles routing messages
 *          within the system.
 *
 *=============================================================================
 */

#ifndef A2B_MSGRTR_PRIV_H_
#define A2B_MSGRTR_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "queue.h"
#include "a2b/msgrtr.h"
#include "a2b/defs.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Stack;
struct a2b_StackContext;
struct a2b_NodeInfo;


typedef struct a2b_MsgNotifier
{
    /** Opaque pointer used by the SLIST queue.h routines to
     *  manage a linked list of a2b_MsgNotifier instances.
     */
    SLIST_ENTRY(a2b_MsgNotifier)    link;

    /** A2B Stack Context */
    a2b_StackContext*               ctx;

    /** Specific notification command (A2B_MSGNOTIFY...) */
    a2b_UInt32                      cmd;

    /** Callback routine when a notification needs to be delivered. */
    a2b_NotifyFunc                  notify;

    /** User data from the listener/client registration, passed on
     *  notification callback call.
     */
    a2b_Handle                      userData;

    /** An optional routine called when the user data needs destroying. */
    a2b_CallbackFunc                destroy;

} a2b_MsgNotifier;


typedef struct a2b_MsgRtr
{
    /** A2B Stack Context -- kept for tracing */
    struct a2b_StackContext*    ctx;

    /** This is a list of registered notifiers */
    SLIST_HEAD(a2b_MsgNotifierHead, a2b_MsgNotifier)  notifierHead;

} a2b_MsgRtr;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT A2B_DSO_LOCAL struct a2b_MsgRtr* a2b_msgRtrAlloc(
                                    struct a2b_StackContext *ctx);
A2B_EXPORT A2B_DSO_LOCAL void a2b_msgRtrFree(
                                    a2b_MsgRtr *msgRtr);

A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_MSGRTR_PRIV_H_ */
