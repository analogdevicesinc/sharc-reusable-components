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
 * \file:   tracectl.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition system trace control functions.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_tracectl          Trace Control module
 *  
 * Definition system trace control functions.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_TRACECTL_H_
#define A2B_TRACECTL_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/conf.h"
#include "a2b/features.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_tracectl_defs         Types/Defs
 *  
 * The various defines and data types used within the trace control module.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

#ifdef A2B_FEATURE_TRACE

/** \name   Bitmask of trace levels bits 12..0 */
#define A2B_TRC_LVL_TRACE3      (1u << 0u) /*!< Node Interrupt Mask */
#define A2B_TRC_LVL_TRACE2      (1u << 1u) /*!< Typically more verbose tracing */
#define A2B_TRC_LVL_TRACE1      (1u << 2u) /*!< Typical Function In/Out Tracing */
#define A2B_TRC_LVL_DEBUG       (1u << 3u) /*!< Debug level messages */
#define A2B_TRC_LVL_INFO        (1u << 4u) /*!< Specific information with respect to node discovery and configuration */
#define A2B_TRC_LVL_WARN        (1u << 5u) /*!< Warnings */
#define A2B_TRC_LVL_ERROR       (1u << 6u) /*!< Errors */
#define A2B_TRC_LVL_FATAL       (1u << 7u) /*!< Fatal Errors */
                                
#define A2B_TRC_LVL_OFF         (0u) /*!< No Trace Level */
#define A2B_TRC_LVL_ALL         (0xFFFu) /*!< Trace all levels */
#define A2B_TRC_LVL_DEFAULT     (A2B_TRC_LVL_FATAL | \
                                 A2B_TRC_LVL_ERROR | \
                                 A2B_TRC_LVL_WARN) /*!< Trace warnings, errors and fatal errors */

/** \name   Bitmask of trace domains bits 31..13 */
#define A2B_TRC_DOM_STACK       (1u << 12u) /*!< Trace messages wrt Stack */
#define A2B_TRC_DOM_TICK        (1u << 13u) /*!< Trace messages wrt Stack Tick */
#define A2B_TRC_DOM_TIMERS      (1u << 14u) /*!< Trace messages wrt Timers */
#define A2B_TRC_DOM_MSGRTR      (1u << 15u) /*!< Trace messages wrt Message transactions */
#define A2B_TRC_DOM_PLUGIN      (1u << 16u) /*!< Trace messages wrt Plugins */
#define A2B_TRC_DOM_I2C         (1u << 17u) /*!< Trace messages wrt I2C transactions */

#define A2B_TRC_DOM_OFF         (0u) /*!< Trace no domain */
#define A2B_TRC_DOM_ALL         (0xFFFFF000u) /*!< Trace all domain */

/** \name   High level trace controls */
#define A2B_TRC_OFF             (0u)  /*!< Trace off */
#define A2B_TRC_ALL             (0xFFFFFFFFu) /*!< Trace all levels and domains */

/** \} -- needed for last name */
/** \} -- a2bstack_tracectl_defs */

/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
struct a2b_StackContext;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_tracectl_funct        Functions
 *  
 * These functions support basic trace controls.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_BEGIN_DECLS

A2B_DSO_PUBLIC void A2B_CALL a2b_traceSetMask(struct a2b_StackContext*  ctx,
                                                a2b_UInt32              mask);

A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_traceGetMask(
                                                struct a2b_StackContext* ctx);

A2B_DSO_PUBLIC void A2B_CALL a2b_traceInject(struct a2b_StackContext*   ctx,
                                               a2b_UInt32               level, 
                                               const a2b_Char*          text);

/** \} -- a2bstack_tracectl_funct */

A2B_END_DECLS

#endif /* A2B_FEATURE_TRACE */

/*======================= D A T A =================================*/

/** \} -- a2bstack_tracectl */

#endif /* Guard for A2B_TRACECTL_H_ */
