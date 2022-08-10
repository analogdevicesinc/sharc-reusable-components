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
 * \file:   features.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The A2B stack features that can be enabled/disabled.
 *
 *=============================================================================
 */

/*============================================================================*/
/**
 * \defgroup a2bstack_features          Stack Features
 *
 * The A2B stack features that can be enabled/disabled.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_FEATURES_H_
#define A2B_FEATURES_H_

/*======================= I N C L U D E S =========================*/

/*======================= S C 5 8 9  D E F I N E S ================*/

#define A2B_FEATURE_ENV_OVERRIDE

#define A2B_FEATURE_MEMORY_MANAGER
#define A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
#define ENABLE_AD242x_SUPPORT
#define A2B_FEATURE_SEQ_CHART
#define A2B_FEATURE_TRACE
#define A2B_STATIC_PLUGIN

/* These options are specific to ADI's Sigma Studio BCF processing */
//#define A2B_FEATURE_COMM_CH
//#define ENABLE_PERI_CONFIG_BCF
//#define ADI_SIGMASTUDIO_BCF
//#define ADI_BDD

// The EEPROM processing feature should be disabled if peripheral configs
// are included in the Sigma Studio BCF.
#ifndef ENABLE_PERI_CONFIG_BCF
#define A2B_FEATURE_EEPROM_PROCESSING
#endif

/*======================= D E F I N E S ===========================*/

/*============================================
 *
 * Turn off/on specific A2B features and APIs
 * The enabled features below are reasonable defaults
 *
 =============================================
 */

/* If the features are NOT being overridden by environmental settings then ...
 */
#ifndef A2B_FEATURE_ENV_OVERRIDE

/**
 * This option will build the master plugin for static linkage
 */
#define A2B_STATIC_PLUGIN

/**
 * This option enables or disables running BERT on the target
 */
/* #define A2B_RUN_BIT_ERROR_TEST */

/**
 * This option controls whether 64-bit integers are available and
 * also whether 64-bit pointers can be formatted in traces.
 */
/* #define A2B_FEATURE_64_BIT_INTEGER */

/**
 * This option controls whether the Power Diagnostics are to be done
 * for the network or not. Normally diagnostics are run by default.
 * If defined, the option to do Power diagnostics is controlled by
 * sending the master plugin a A2B_MSGREQ_NET_DISBALE_LINEDIAG message.
 */
/* #define DISABLE_PWRDIAG */

/**
 * This option controls whether sequence charts are supported or not.
 */
/* #define A2B_FEATURE_SEQ_CHART */

/**
 * This options controls whether trace is support or not.
 */
/* #define A2B_FEATURE_TRACE */

/**
 * This option controls whether the internal fixed pool based memory
 * management services are built into the A2B stack.
 */
#define A2B_FEATURE_MEMORY_MANAGER

/** When defined a single mailbox is used for all peripheral
 *  config processing.  So each node that needs processing
 *  will add to the one mailbox, thus forcing the first node
 *  to configure its peripheral to completion until the next
 *  nodes peripheral can be configured--regardless of cfg delay
 *  processing.
 *
 *  The amount of work done by each node for peripheral
 *  config is defined by #A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS.
 *  The peripheral will process #A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS
 *  cfg blocks in a row before suspending to allow others to do work.
 *  When #A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY is disabled
 *  other nodes will have a chance to process peripheral config.
 *
 *  This setting does however have the side effect of then
 *  following when #A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
 *  is disabled.
 *  -# possibly adding more delay on a cfg block delay because
 *  other peripherals might be in line for processing--also
 *  depending on #A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS and number
 *  of nodes ready to process peripherals.
 *  -# will require some more memory use for tracking each node
 *  separately and more mailboxes.
 *  -# will not conform exactly to the ADI discovery for Simple
 *  and Modified since it can "interleave" multiple peripheral
 *  config depending on the system setup.
 *
 *  If you have no peripherals to configure then this should be
 *  enabled. Also, if the peripherals have no delays you may also
 *  want this feature enabled.
 *
 *  When disabled this feature attempts to maximize the I2C traffic
 *  while one node has processed a peripheral cfg delay or
 *  #A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS != 0 which
 *  allows other peripheral cfg for other nodes to be processed.
 *  When disabled using Simple discovery the node init will be
 *  completed for all nodes, then all peripheral config is queued
 *  and will execute cooperatively.  Whereas when enabled the node
 *  init is completed followed by the nodes peripheral processing
 *  followed by the next nodes int, and so on.
 *
 *  \note If #A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS == 0 then this
 *        \#define will be automatically defined.
 *
 *  \note This effects processing of other peripheral config
 *        while one node is processing a delay.  This does NOT
 *        effect the discovery of new nodes when using Modified
 *        Discovery.
 */
#define A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY

/** This feature enables the processing of EEPROM configuration
 *  from either the actual EEPROM hardware (if available) OR
 *  the EEPROM config package binary.  If no EEPROM cfg blocks
 *  for peripheral configuration are defined then you can free
 *  up some resources by disabling this feature.
 */
#define A2B_FEATURE_EEPROM_PROCESSING

/** Internal Feature Definitions */
/**
 * This option controls whether 242x is supported in the BDD
 */
#define ENABLE_AD242x_SUPPORT

/**
 * This option controls whether BDD is to be parsed from
 * Busconfig file generated from SigmaStudio.
 */
#define ADI_SIGMASTUDIO_BCF

/**
 * This option controls whether Communication Channel needs to be supported
 * Communication Channel enables node to node message communication using
 * A2B Mailbox registers
 */
/* #define A2B_FEATURE_COMM_CH */

#endif /* not A2B_FEATURE_ENV_OVERRIDE */

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_features */

#endif /* A2B_FEATURES_H_ */
