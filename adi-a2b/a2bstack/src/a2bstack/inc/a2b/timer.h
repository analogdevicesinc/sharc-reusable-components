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
 * \file:   timer.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This provides the definitions of the timer services.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_timer         Timer Module
 *  
 * This provides the definitions of the timer services.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_TIMER_H_
#define A2B_TIMER_H_

/*======================= I N C L U D E S =========================*/
#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;
struct a2b_Timer;

/** Timer destroy or expiration callback function */
typedef void (A2B_CALL * a2b_TimerFunc)(struct a2b_Timer* timer,
                                        a2b_Handle userData);

/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_DSO_PUBLIC struct a2b_Timer* A2B_CALL a2b_timerAlloc(
                                            struct a2b_StackContext* ctx,
                                            a2b_TimerFunc            onTimeout, 
                                            a2b_Handle               userData);

A2B_DSO_PUBLIC void A2B_CALL a2b_timerRef(struct a2b_Timer* timer);
A2B_DSO_PUBLIC a2b_UInt16 A2B_CALL a2b_timerUnref(struct a2b_Timer* timer);

A2B_DSO_PUBLIC void A2B_CALL a2b_timerSet(struct a2b_Timer* timer,
                                             a2b_UInt32     after,
                                             a2b_UInt32     repeat);
A2B_DSO_PUBLIC void A2B_CALL a2b_timerSetHandler(struct a2b_Timer*  timer,
                                                     a2b_TimerFunc  onTimeout);
A2B_DSO_PUBLIC void A2B_CALL a2b_timerSetData(struct a2b_Timer* timer,
                                                a2b_Handle      userData);
A2B_DSO_PUBLIC void A2B_CALL a2b_timerRepeat(struct a2b_Timer* timer);
A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_timerRemaining(
                                            struct a2b_Timer* timer);

A2B_DSO_PUBLIC void A2B_CALL a2b_timerStart(struct a2b_Timer* timer);
A2B_DSO_PUBLIC void A2B_CALL a2b_timerStop(struct a2b_Timer* timer);

A2B_DSO_PUBLIC a2b_Bool A2B_CALL a2b_timerIsActive(struct a2b_Timer* timer);
A2B_DSO_PUBLIC void a2b_ActiveDelay(struct a2b_StackContext* ctx, a2b_UInt32 nTime);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_timer */

#endif /* A2B_TIMER_H_ */
