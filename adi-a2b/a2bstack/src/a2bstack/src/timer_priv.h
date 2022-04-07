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
 * \file:   timer_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This provides the definitions of the private timer services.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_timer 
 * \defgroup a2bstack_timer_priv        \<Private\> 
 * \private 
 *  
 * This defines the timer API's that are private to the stack. 
 *  
 * \{ 
 */
/*============================================================================*/

#ifndef A2B_TIMER_PRIV_H_
#define A2B_TIMER_PRIV_H_

/*======================= I N C L U D E S =========================*/
#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/timer.h"
#include "queue.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;

/** The detailed timer implementation */
typedef struct a2b_Timer
{
    /** Link to the next timer in the linked list of allocated timers */
    SLIST_ENTRY(a2b_Timer)     link;

    /** The time (in msec) the timer will expire after initially starting */
    a2b_UInt32          after;

    /** The time (in msec) the timer will be reloaded after expiring the
     *  first time. Set to 0 (zero) so the timer isn't re-loaded.
     */
    a2b_UInt32          repeat;

    /** The base epoch the timer started (units = msec) */
    a2b_UInt32          lastTime;

    /** Pointer to opaque data passed to timer callback */
    a2b_Handle          userData;

    /** The timer callback function called when timer expires */
    a2b_TimerFunc       expireFunc;

    /** Pointer back to the parent A2B stack context */
    struct a2b_StackContext*    ctx;

    /** Reference count of the timer object. Unlinked when == 0 */
    a2b_UInt16          refCnt;

    /** Bitmask containing timer status */
    a2b_UInt8           status;

} a2b_Timer;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/** (Private) Called internally by the stack */
A2B_EXPORT A2B_DSO_LOCAL void a2b_timerTick(struct a2b_StackContext* ctx);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_timer_priv */

#endif /* A2B_TIMER_PRIV_H_ */
