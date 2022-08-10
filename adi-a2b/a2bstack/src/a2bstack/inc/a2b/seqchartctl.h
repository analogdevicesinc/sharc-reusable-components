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
 * \file:   seqchartctl.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition system sequence chart trace control functions.
 *
 *=============================================================================
 */


/*============================================================================*/
/** 
 * \defgroup a2bstack_seqchartctl       Sequence Chart Control Module
 *  
 * Definition of sequence chart control functions and supporting macros.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_SEQCHARTCTL_H_
#define A2B_SEQCHARTCTL_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/conf.h"
#include "a2b/defs.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_seqchartctl_defs      Types/Defs
 *  
 * The various defines and data types used within the sequence chart
 * control  module.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

#ifdef A2B_FEATURE_SEQ_CHART

/** Entities/actors capable of participating in A2B sequence charts */
typedef enum
{
    /** The application utilizing the A2B stack */
    A2B_SEQ_CHART_ENTITY_APP,

    /** The generic A2B stack */
    A2B_SEQ_CHART_ENTITY_STACK,

    /** The platform specific implementation layer */
    A2B_SEQ_CHART_ENTITY_PLATFORM,

    /** A master plugin */
    A2B_SEQ_CHART_ENTITY_PLUGIN_MASTER,

    /** Start of range for slave plugins (plugin 0). The
     *  master plugin must immediately precede the first slave
     *  plugin.
     */
    A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_START =
            A2B_SEQ_CHART_ENTITY_PLUGIN_MASTER + 1,

    /** One more than the last valid slave node (plugin N_max + 1) */
    A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_END =
            A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_START +
            A2B_CONF_MAX_NUM_SLAVE_NODES

} a2b_SeqChartEntity;

/** Define the mapping between a plugin (master/slave) node address and
 *  the associated sequence chart entity.
 */
#define A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(A) \
            ((a2b_SeqChartEntity)((A2B_VALID_NODEADDR(A)) ? \
            ((A) + A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_START) : \
            A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_END))


/** Enumerate the available communication types between entities */
typedef enum
{
    /** The source entity makes a request of the destination */
    A2B_SEQ_CHART_COMM_REQUEST,

    /** The source entity is replying to the destination */
    A2B_SEQ_CHART_COMM_REPLY,

    /** The source entity is providing an unsolicited notification to the
     *  destination
     */
    A2B_SEQ_CHART_COMM_NOTIFY
} a2b_SeqChartCommType;

/** Sequence traces with increasing levels generally provide additional level
 * of detail and frequency.
 */

/** Sequence will not be traced */
#define A2B_SEQ_CHART_LEVEL_NEVER   (0u)

/** Sequences associated with Level 1 will be traced */
#define A2B_SEQ_CHART_LEVEL_1       (1u << 0u)

/** Sequences associated with Level 2 will be traced */
#define A2B_SEQ_CHART_LEVEL_2       (1u << 1u)

/** Sequences associated with Level 3 will be traced */
#define A2B_SEQ_CHART_LEVEL_3       (1u << 2u)

/** Sequences associated with Level 4 will be traced */
#define A2B_SEQ_CHART_LEVEL_4       (1u << 3u)

/** Sequences associated with Level 5 will be traced */
#define A2B_SEQ_CHART_LEVEL_5       (1u << 4u)

/** Sequences associated with Level 6 will be traced */
#define A2B_SEQ_CHART_LEVEL_6       (1u << 5u)

/** Sequences associated with Level 7 will be traced */
#define A2B_SEQ_CHART_LEVEL_7       (1u << 6u)

/** Sequences will trace request/response/notify messages */
#define A2B_SEQ_CHART_LEVEL_MSGS    (1u << 7u)

/** Sequence will *always* be traced regardless of the current sequence mask */
#define A2B_SEQ_CHART_LEVEL_ALWAYS  (1u << 8u)

/** Sequence will trace I2C traffic */
#define A2B_SEQ_CHART_LEVEL_I2C     (1u << 9u)

/** Sequence will trace the discovery process AND all I2C
 *  --makes no sense without I2C sequence as well.
 */
#define A2B_SEQ_CHART_LEVEL_DISCOVERY  ((1u << 10u) | A2B_SEQ_CHART_LEVEL_I2C)

/** Sequence will trace the power fault diagnosis process AND all I2C
 *  --makes no sense without I2C sequence as well.
 */
#define A2B_SEQ_CHART_LEVEL_PWR_FAULT  ((1u << 11u) | A2B_SEQ_CHART_LEVEL_I2C)

/** All sequence levels will be traced */
#define A2B_SEQ_CHART_LEVEL_ALL     (0xFFFFu)

/** Define global sequence chart options */
#define A2B_SEQ_CHART_OPT_NONE          (0u)
#define A2B_SEQ_CHART_OPT_AUTONUMBER    (1u << 0u)
#define A2B_SEQ_CHART_OPT_TIMESTAMP     (1u << 2u)
#define A2B_SEQ_CHART_OPT_ALL           (A2B_SEQ_CHART_OPT_AUTONUMBER | \
                                        A2B_SEQ_CHART_OPT_TIMESTAMP)

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;

/** \} -- a2bstack_seqchartctl_defs */

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_seqchartctl_funct     Functions
 *  
 * These are the main sequence chart control functions.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_seqChartStart(
                                  struct a2b_StackContext*  ctx,
                                  const a2b_Char*           url,
                                  a2b_UInt32                level,
                                  a2b_UInt32                options,
                                  const a2b_Char*           title);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_seqChartInject(
                                  struct a2b_StackContext*  ctx,
                                  a2b_UInt32                level,
                                  const a2b_Char*           text);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_seqChartStop(
                                    struct a2b_StackContext* ctx);


A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_seqChartGetOptions(
                                    struct a2b_StackContext* ctx);

A2B_DSO_PUBLIC void A2B_CALL a2b_seqChartSetOptions(
                                    struct a2b_StackContext* ctx,
                                    a2b_UInt32               options);

A2B_END_DECLS

#endif  /* A2B_FEATURE_SEQ_CHART */

/** \} -- a2bstack_seqchartctl_funct */

/*======================= D A T A =================================*/

/** \} -- a2bstack_seqchartctl */

#endif /* A2B_SEQCHARTCTL_H_ */
