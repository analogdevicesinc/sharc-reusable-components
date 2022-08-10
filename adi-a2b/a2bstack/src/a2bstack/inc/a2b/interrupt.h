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
 * \file:   interrupt.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the public API for A2B interrupt handling.
 *
 *=============================================================================
 */

/*============================================================================*/
/**
 * \defgroup a2bstack_interrupt             Interrupt Module
 *  
 * This defines the public API for A2B interrupt handling.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_INTERRUPT_H_
#define A2B_INTERRUPT_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/regdefs.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_interrupt_defs        Types/Defs
 *  
 * The various defines and data types used within the interrupt modules.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/*----------------------------------------------------------------------------*/
/** \name Interrupt Mask Offsets
 *  
 *  Only used to report an error for a2b_intrGetMask()
 *
 * \{ */
/*----------------------------------------------------------------------------*/
#define A2B_INTRMASK0_OFFSET  ((a2b_UInt32)0u)
#define A2B_INTRMASK1_OFFSET  ((a2b_UInt32)8u)
#define A2B_INTRMASK2_OFFSET  ((a2b_UInt32)16u)
/** \} */

/*----------------------------------------------------------------------------*/
/** \name Interrupt Error Definitions
 *  
 *  Only used to report an error for a2b_intrGetMask()
 *
 * \{ */
/*----------------------------------------------------------------------------*/
#define A2B_INTRMASK_READERR  ((a2b_UInt32)0x01u << (a2b_UInt32)24u)
#define A2B_INTRMASK_INVPARAM ((a2b_UInt32)0x02u << (a2b_UInt32)24u)
/** \} */

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;

/** \} -- a2bstack_interrupt_defs */

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_interrupt_mask    Mask Related Functions
 *  
 * These functions are used to get/set interrupt masks on an A2B node.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_intrGetMask(
                                        struct a2b_StackContext*    ctx,
                                        a2b_Int16                   nodeAddr);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_intrSetMask(
                                        struct a2b_StackContext*    ctx,
                                        a2b_Int16                   nodeAddr,
                                        a2b_UInt32                  mask );
/** \} -- a2bstack_interrupt_mask */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_interrupt_poll    Polling Functions
 *  
 * These functions are used to start/stop interrupt polling of the A2B 
 * master node.  These functions are not needed if the application 
 * uses the A2B hardware IRQ.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_intrStartIrqPoll(
                                        struct a2b_StackContext*  ctx,
                                        a2b_UInt32                rate );

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_intrStopIrqPoll(
                                        struct a2b_StackContext*  ctx );

/** \} -- a2bstack_interrupt_poll */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_interrupt_query   Query Function
 *  
 * This function is used query the A2B master interrupt status. If 
 * an interrupt is detected it will be processed.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_intrQueryIrq(
                                        struct a2b_StackContext*  ctx );
/** \} -- a2bstack_interrupt_query */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_interrupt */

#endif /* A2B_INTERRUPT_H_ */
