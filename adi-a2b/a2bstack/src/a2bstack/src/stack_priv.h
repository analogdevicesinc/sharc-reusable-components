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
 * \file:   stack_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The private definitions and types associated with the core stack
 *          services.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_stack
 * \defgroup a2bstack_stack_priv        \<Private\> 
 * \private 
 *  
 * This defines the stack "object" API's that are private to the stack.
 *  
 * \{ */
/*============================================================================*/

#ifndef A2B_STACK_PRIV_H_
#define A2B_STACK_PRIV_H_

/*======================= I N C L U D E S =========================*/
#include "a2b/macros.h"
#include "a2b/ecb.h"
#include "a2b/stack.h"
#include "a2b/pal.h"
#include "a2b/conf.h"
#include "queue.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
#ifdef A2B_FEATURE_TRACE
struct a2b_TraceChannel;
#endif

#ifdef A2B_FEATURE_SEQ_CHART
struct a2b_SeqChartChannel;
#endif

struct a2b_Timer;
struct a2b_PluginApi;
struct a2b_JobExecutor;
struct a2b_StackContext;
struct a2b_MsgRtr;
struct a2b_IntrInfo;

typedef   enum {
    	/** I2C Access from Unknown source */
    A2B_I2C_ACCESS_UNKNOWN = (a2b_UInt32)0u,
		/** I2C Access from Master Plugin */
        A2B_I2C_ACCESS_MASTER,
		/** I2C Access from Slave Plugin */
        A2B_I2C_ACCESS_SLAVE,
		/** I2C Access from Remote peripheral */
        A2B_I2C_ACCESS_PERIPH
} ADI_A2B_I2C_ACCESS_MODE;

/**
 * Used to store that last I2C access of the stack
 */
typedef struct a2b_I2cLastMode
{
	ADI_A2B_I2C_ACCESS_MODE     access;
    a2b_Int16   nodeAddr;
    a2b_UInt16  chipAddr;
    a2b_Bool    broadcast;
} a2b_I2cLastMode;

/** Define an invalid 7-bit I2C address */
#define A2B_INVALID_7BIT_I2C_ADDRESS    (0xFFu)

/**
 * Context that stores information about a deferred callback
 */
typedef struct a2b_stackDefContext
{
    /** Link to the next context in the linked list of allocated
     *  deferred stack contexts
     */
    SLIST_ENTRY(a2b_stackDefContext)    link;

    /** Callback function */
    a2b_stackDefCbFunc                  func;

    /** A2B stack context */
    struct a2b_StackContext*            ctx;

    /** User data to pass to callback when called */
    a2b_Handle                          userData;

} a2b_stackDefContext;


typedef struct a2b_Stack
{
    /** Driver Platform Abstraction Layer (PAL) function table */
    a2b_StackPal                pal;

    /** Environment Control Block (ECB) associated with the stack */
    A2B_ECB*                    ecb;

    /** The job executor to handle scheduling/processing jobs for the stack */
    struct a2b_JobExecutor*     jobExec;

    /** The handle to the open I2C device */
    a2b_Handle                  i2cHnd;

    /** Used to remember that last I2C access mode for the stack */
    a2b_I2cLastMode             i2cMode;

    /** The handle to the open audio device */
    a2b_Handle                  audioHnd;

    /** A list of managed timers */
    SLIST_HEAD(a2b_TimerHead, a2b_Timer) timerList;

    /** Tracks the number of times the stack "tick" has been issued.  This is
     *  used with #A2B_CONF_SCHEDULER_TICK_MULTIPLE to decide when to call
     *  #a2b_jobExecSchedule to do work.
     */
    a2b_UInt32                  stackTickCnt;

    /** Interrupt info used when interrupt polling is enabled */
    struct a2b_IntrInfo*        intrInfo;

    /** The plugin vtables loaded/registered by the stack */
    struct a2b_PluginApi*       plugins;
    a2b_UInt16                  numPlugins;

    /** This tracks stack contexts for each plugin (+1 == master)
     *  Indexes into the array match the nodeAddr+1 for each plugin.
     */
    struct a2b_StackContext*    pluginList[A2B_CONF_MAX_NUM_SLAVE_NODES+1u];

    /** This is the message router to route messages internal to the stack */
    struct a2b_MsgRtr*          msgRtr;

#ifdef A2B_FEATURE_TRACE
    /** Pointer to the stack's allocated trace channel */
    struct a2b_TraceChannel*    traceChan;
#endif
#ifdef A2B_FEATURE_SEQ_CHART
    /** Pointer to the stack's sequence chart channel if active */
    struct a2b_SeqChartChannel* seqChartChan;
#endif

    /** The heap associated with the stack instance */
    a2b_Handle                  heapHnd;

    /** Initialization mask of sub-systems */
    a2b_UInt32                  initMask;

} a2b_Stack;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_BEGIN_DECLS

A2B_EXPORT A2B_DSO_LOCAL a2b_Stack* a2b_stackPrivAlloc(
                                                const a2b_StackPal*     pal, 
                                                A2B_ECB*                ecb);

A2B_EXPORT A2B_DSO_LOCAL void a2b_stackDestroy(a2b_Stack*   stk,
                                               a2b_Bool     destroyHeap);

A2B_EXPORT A2B_DSO_LOCAL void a2b_stackDestroyHeap(a2b_Handle*          hnd,
                                                   const a2b_StackPal*  pal, 
                                                   A2B_ECB*             ecb);

A2B_EXPORT A2B_DSO_LOCAL void a2b_stackResetI2cLastMode(
                                                    a2b_I2cLastMode* lastMode);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_stack_priv */

#endif /* A2B_STACK_PRIV_H_ */
