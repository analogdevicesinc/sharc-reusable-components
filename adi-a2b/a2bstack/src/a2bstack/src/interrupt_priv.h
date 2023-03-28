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
 * \file:   interrupt_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the private API for A2B interrupt handling.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_interrupt 
 * \defgroup a2bstack_interrupt_priv        \<Private\> 
 * \private 
 *  
 * This defines the private API for A2B interrupt handling. 
 *  
 * \{ 
 */
/*============================================================================*/

#ifndef A2B_INTERRUPT_PRIV_H_
#define A2B_INTERRUPT_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;
struct a2b_Timer;

/** Data used to track interrupt polling. */
typedef struct a2b_IntrInfo
{
    /** Timer used when interrupt polling is enabled */
    struct a2b_Timer*           timer;

    /** This is the interrupt mask specified by the application
     *  for each node in the system.  These values are OR'ed with
     *  the plugin values.  This means the app cannot disable the
     *  interrupts needed by the plugins. (+1 for the master)
     */
    a2b_UInt8                   appMask[A2B_CONF_MAX_NUM_SLAVE_NODES + 1];

    /** This is the interrupt mask specified by the plugins
     *  for each node in the system.  These values are OR'ed with
     *  the application values.  This means the app cannot disable the
     *  interrupts needed by the plugins. (+1 for the master)
     */
    a2b_UInt8                   pluginMask[A2B_CONF_MAX_NUM_SLAVE_NODES + 1];

} a2b_IntrInfo;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT A2B_DSO_LOCAL void a2b_intrDestroy( struct a2b_Stack* stk );

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_interrupt_priv */

#endif /* A2B_INTERRUPT_PRIV_H_ */
