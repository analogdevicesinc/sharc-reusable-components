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
 * \file:   stack.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition/prototypes for the A2B client-side stack API.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_stack         Stack Module
 *  
 * Definition/prototypes for the A2B client-side stack API.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_STACK_H_
#define A2B_STACK_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/pal.h"
#include "a2b/ecb.h"
#include "a2b/features.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;
struct a2b_StackPal;
struct a2b_NodeInfo;

/** This is the definition of deferred function callbacks */
typedef void (A2B_CALL *a2b_stackDefCbFunc)(struct a2b_StackContext*,
                                            a2b_Handle);

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/**
 * Function to do basic default initialization of the platform abstraction
 * layer (PAL). Platform (PAL) implementations should call this function
 * *first* before doing platform-specific overrides.
 */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackPalInit(a2b_StackPal* pal,
                                                A2B_ECB*    ecb);

/**
 * Function to allocate individual A2B stacks since there can be more than 
 * one A2B master node on a platform. Each master node requires a separate 
 * stack instance.
 */
A2B_DSO_PUBLIC struct a2b_StackContext* A2B_CALL a2b_stackAlloc(
                            const struct a2b_StackPal*  pal,
                            A2B_ECB*                    ecb);

/** * Function to deallocate individual A2B stack. */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackFree(struct a2b_StackContext* ctx);

/**
 * Called at a periodic interval so the stack can schedule
 * processing of timers and interrupts.
 */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackTick(struct a2b_StackContext* ctx);

/** This call will search for a plugin to handle a node. */
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_stackFindHandler(
                                     struct a2b_StackContext*       ctx,
                                     const a2b_NodeSignature*       nodeSig);

/**
 * This will free a slave node plugin instance based on it's node
 * address. It can only be invoked from an application context or the
 * master plugin context.
 */
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_stackFreeSlaveNodeHandler(
                                        struct a2b_StackContext*    ctx,
                                        a2b_Int16                   nodeAddr);

#ifdef A2B_FEATURE_MEMORY_MANAGER
/**
 * The function will return the minimum heap size (in bytes) required for a
 * *single* instance of the A2B stack. The application is welcome to provide a
 * larger block of memory but if it wishes to utilize the built-in pooled
 * memory management features then the heap size returned here should be
 * considered the lower limit based on platform-specific definitions in
 * a2b/conf.h.
 */
A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_stackGetMinHeapSize(
                                                        a2b_Byte* heapStart);
#endif

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_stack_ver         Versions
 *  
 * These are functions to get version and build information for the
 * stack and PAL. 
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/** Functions to get A2B Stack version information. */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackGetVersion(a2b_UInt32*    major,
                                                   a2b_UInt32*  minor,
                                                   a2b_UInt32*  release);

/** Functions to get A2B Stack build information. */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackGetBuild(a2b_UInt32*          buildNum,
                                            const a2b_Char** const  buildDate,
                                            const a2b_Char** const  buildOwner,
                                            const a2b_Char** const  buildSrcRev,
                                            const a2b_Char** const  buildHost);

/** Functions to get A2B Stack PAL version information. */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackPalGetVersion(a2b_StackPal*   pal,
                                                      a2b_UInt32*   major,
                                                      a2b_UInt32*   minor,
                                                      a2b_UInt32*   release);

/** Functions to get A2B Stack PAL build information. */
A2B_DSO_PUBLIC void A2B_CALL a2b_stackPalGetBuild(a2b_StackPal*     pal,
                                            a2b_UInt32*             buildNum,
                                            const a2b_Char** const  buildDate,
                                            const a2b_Char** const  buildOwner,
                                            const a2b_Char** const  buildSrcRev,
                                            const a2b_Char** const  buildHost);
/** \} -- a2bstack_stack_ver */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_stack */

#endif /* A2B_STACK_H_ */
