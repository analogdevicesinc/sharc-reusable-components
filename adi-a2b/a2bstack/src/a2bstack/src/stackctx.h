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
 * \file:   stackctx.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The stack context utilized by both A2B stack applications and
 *          plugins.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_stackctx          Stack Context Module
 *  
 * The stack context utilized by both A2B stack applications and plugins.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_STACKCTX_H_
#define A2B_STACKCTX_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/defs.h"
#include "a2b/pluginapi.h"
#include "queue.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_stackctx_defs     Types/Defs
 *  
 * The various defines and data types used within the stack context module.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Stack;
struct a2b_PluginApi;
struct a2b_JobQueue;

/**
 * The domain under which the stack context applies.
 */
typedef enum
{
    /** Within the domain of the application utilizing the A2B stack */
    A2B_DOMAIN_APP,

    /** Within the domain of a plugin loaded by the A2B stack */
    A2B_DOMAIN_PLUGIN

} a2b_ContextDomain;


/**
 * The context control block for the stack context.
 */
typedef union a2b_ContextCtrlBlk
{
    /** A2B_DOMAIN_APP configuration */
    struct {
        /* TDB */
        a2b_UInt32 dummy;
    } app;

    /** A2B_DOMAIN_PLUGIN configuration */
    struct {
        /** Node signature (version, nodeAddr, etc) */
        a2b_NodeSignature       nodeSig;

        /** The A2B plugin vtable associated with the context */
        struct a2b_PluginApi*   pluginApi;

        /** The handle returned by the opened plugin */
        a2b_Handle              pluginHnd;

        /** This is the mailboxes (job queues) specific to the plugin for
         *  jobs going into the plugin, not jobs leaving the plugin
         */
        SLIST_HEAD(a2b_JobQListHead, a2b_JobQueue)  mailboxList;

    } plugin;
}  a2b_ContextCtrlBlk;


/** 
* <!--
* @startuml stackctx-lifecycle.png
* 
* participant App
* participant Stack
* participant JobExec
* participant StackCtx
* participant MasterPlugin
* participant Plugin
* 
* title Example Lifecycle of a Stack
* 
* group Plugin Instantiated from Master Plugin on Discovery
* MasterPlugin -> Stack: a2b_stackFindHandler( stkCtx )
* activate Stack #DarkSalmon
* note over JobExec
* MUST first allocate a context for the plugin.open() in case
* it can initialize and needs to store a reference.
* end note
* Stack -> StackCtx: a2b_stackContextAlloc(A2B_DOMAIN_PLUGIN)
* activate StackCtx #DarkSalmon
* StackCtx -> JobExec: a2b_jobExecAllocQueue()
* StackCtx <-- JobExec
* Stack <-- StackCtx 
* deactivate StackCtx 
* Stack -> Plugin: open()
* Stack <-- Plugin: non-NULL
* note over Stack
* Add the new stkCtx to the stk->pluginList for tracking.
* end note
* MasterPlugin <-- Stack
* deactivate Stack
* end
* 
* |||
* 
* group Stack [APP Context] is Freed
* App -> Stack: a2b_stackFree(ctx)
* activate Stack #DarkSalmon
* Stack -> StackCtx: a2b_stackContextFree(ctx)
* activate StackCtx
* StackCtx -> Stack: a2b_stackDestroy()
* note over Stack
* Stop Interrupts, 
* Shutdown Timers, 
* Close/shutdown audio,
* Free JobExec,
* Close/shutdown I2C,
* Free timers,
* Free trace log,
* Free memory manager
* end note
* 
* StackCtx -> Stack: a2b_stackDestroyHeap()
* StackCtx <-- Stack
* deactivate Stack
* App <-- StackCtx
* 
* deactivate StackCtx
* end
* 
* @enduml
* -->
* @image html stackctx-lifecycle.png "Example Lifecycle of a Stack"
*/
typedef struct a2b_StackContext
{
    /** Pointer to the A2B Stack that provides the context */
    struct a2b_Stack* stk;

    a2b_ContextDomain domain;

    a2b_ContextCtrlBlk ccb;

} a2b_StackContext;

/** \} -- a2bstack_stackctx_defs */


/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_stackctx_funct    Functions
 *  
 * These are functions to manage the life of a stack context (malloc/free/etc).
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_EXPORT A2B_DSO_LOCAL a2b_StackContext* a2b_stackContextAlloc(
                                                    struct a2b_Stack*   stk,
                                                    a2b_ContextDomain   domain,
                                                    a2b_ContextCtrlBlk* ccb);

A2B_EXPORT A2B_DSO_LOCAL void a2b_stackContextFree(a2b_StackContext* ctx);

A2B_EXPORT A2B_DSO_LOCAL struct a2b_StackContext* a2b_stackContextFind(
                                                a2b_StackContext*    ctx,
                                                a2b_Int16            nodeAddr);

A2B_END_DECLS

/** \} -- a2bstack_stackctx_funct */

/*======================= D A T A =================================*/

/** \} -- a2bstack_stackctx */

#endif /* A2B_STACKCTX_H_ */
