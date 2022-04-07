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
 * \file:   conf.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is a GENERATED A2B system configuration file.
 *
 *=============================================================================
 */

/*============================================================================*/
/**
 * \defgroup a2bstack_conf          Stack Configuration
 *
 * Provides AD2410 stack configuration.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_CONF_H_
#define A2B_CONF_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/features.h"


/*======================= S C 5 8 9  D E F I N E S ================*/

#define A2B_CONF_MEMORY_ALIGNMENT               (1)
#define A2B_CONF_POINTER_SIZE                   (32)
#define A2B_CONF_MAX_NUM_MASTER_NODES           (4)
#define A2B_CONF_MAX_NUM_SLAVE_NODES            (10)
#define A2B_CONF_SCHEDULER_TICK_MULTIPLE        (1)
#define A2B_CONF_CHAR_BIT                       (8)

/*======================= D E F I N E S ===========================*/

/** Define the memory alignment required by the platform */
#ifndef A2B_CONF_MEMORY_ALIGNMENT
#define A2B_CONF_MEMORY_ALIGNMENT           (4u)
#endif

/** Define the number of bits in a pointer for this platform. Typical
 *  values include 64, 32, 16, etc.. */
#ifndef A2B_CONF_POINTER_SIZE
#define A2B_CONF_POINTER_SIZE               (32u)
#endif

/** Maximum number of bits in a byte. On some DSP architectures CHAR_BIT > 8 */
#ifndef A2B_CONF_CHAR_BIT
#define A2B_CONF_CHAR_BIT                   (32u)
#endif

/** Maximum number of A2B master nodes on this platform */
#ifndef A2B_CONF_MAX_NUM_MASTER_NODES
#define A2B_CONF_MAX_NUM_MASTER_NODES       (1u)
#endif

/** Maximum number of A2B slave nodes attached to each master node */
#ifndef A2B_CONF_MAX_NUM_SLAVE_NODES
#define A2B_CONF_MAX_NUM_SLAVE_NODES        (10u)
#endif

/** Define the maximum number of contexts that can be allocated and
 *  utilized by the A2B application and associated plugins per stack
 *  instance.
 */
#ifndef A2B_CONF_MAX_NUM_STACK_CONTEXTS
#define A2B_CONF_MAX_NUM_STACK_CONTEXTS (2u  + A2B_CONF_MAX_NUM_SLAVE_NODES)
#endif

/** The maximum number of open/active timers per stack instance */
#ifndef A2B_CONF_MAX_NUM_TIMERS
#define A2B_CONF_MAX_NUM_TIMERS             (2u + (2u * (1u + \
                                            A2B_CONF_MAX_NUM_SLAVE_NODES)))
#endif

/** Define the size (in characters) of the trace buffer */
#ifndef A2B_CONF_TRACE_BUF_SIZE
#define A2B_CONF_TRACE_BUF_SIZE             (256u)
#endif

/** Define the number of log channels dedicated for tracing. */
#ifndef A2B_CONF_TRACE_NUM_CHANNELS
#define A2B_CONF_TRACE_NUM_CHANNELS         (1u)
#endif


/** Define the number of available log channels */
#ifndef A2B_CONF_LOG_NUM_CHANNELS
#define A2B_CONF_LOG_NUM_CHANNELS   (A2B_CONF_TRACE_NUM_CHANNELS + 1u)
#endif


/**
 * Define the frequency to run the msg/job scheduler based on a multiple
 * of the stack "tick". The "tick" provided to the stack ultimately sets the
 * time base for the entire stack.
 */
#ifndef A2B_CONF_SCHEDULER_TICK_MULTIPLE
#define A2B_CONF_SCHEDULER_TICK_MULTIPLE    (2u)
#endif


/** Define the maximum number of I2C master devices to support per stack
 *  instance.
 */
#ifndef A2B_CONF_MAX_NUM_I2C_DEVICES
#define A2B_CONF_MAX_NUM_I2C_DEVICES        (1u)
#endif

/** Defines the number of consecutive EEPROM peripheral
 *  configuration blocks processed in a row before
 *  pausing and allowing other jobs to execute.  This
 *  value should be tuned to attempt to maximize the
 *  I2C bus utilization while not starving other jobs.
 *  This value should be tuned based on the scheduler
 *  tick rate to maximize utilization.  A value of
 *  zero will force "serial" peripheral processing;
 *  therefore, if delays are processed they could
 *  block discovery depending on the discovery mode.
 *
 *  \note This value accounts for the read of the
 *  cfgblock from the EEPROM AND its processing
 *  (delay/write).  The delay will not block other tasks
 *  unless a value of zero is specified here.
 */
#define A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS  (2u)

/** Define the maximum number of job executors allocated per stack instance.
 *  For typical systems only a single instance is needed.
 */
#define A2B_CONF_MAX_NUM_JOB_EXECUTORS      (1u)

/** Define the maximum number of job queues that can be allocated per
 *  software stack instance.
 *  \note Master needs 3 because: master mailbox, peripheral mailbox, communication channel mailbox
 *  \note Users MUST use A2B_CONF_MAX_NUM_JOB_QUEUES which is the adjusted
 *        value at the end of this header.
 */
#define TMP_CONF_MAX_NUM_JOB_QUEUES         (A2B_CONF_MAX_NUM_SLAVE_NODES + \
                                             3u /* master plugin */)

/** Define the path separator for the platform. Need by the trace facilities. */
#define A2B_CONF_PATH_SEPARATOR             '/'

/** Define the maximum length for a plugin name.  The plugin name is mainly
 *  available for debugging (Sequence chart, tracing, etc).
 */
#define A2B_CONF_DEFAULT_PLUGIN_NAME_LEN    (32u)

/** Define the maximum number of message handlers per stack instance. Any plugin
 *  requires a message handler (master plugin as well).  Reducing this to match
 *  the number of plugins supported will reduce the footprint of the stack.
 *  This should never be smaller than the number of expected/supported plugins.
 */
#define A2B_CONF_MAX_NUM_MSG_HANDLERS       (A2B_CONF_MAX_NUM_SLAVE_NODES + 1u)

/** Define the total number of messages supported per stack instance. This
 *  defines the maximum number of messages that can be used at one time within
 *  the stack.
 */
#define A2B_CONF_MSG_POOL_SIZE              (12u)

/** Define the number of clients that can register for notifications. Reducing
 *  this value will reduce the footprint of the stack. *
 */
#define A2B_CONF_MSG_NOTIFICATION_MAX       (4u)


/** Define the minimum size (in bytes) for a message payload. The actual
 *  message payload size will be the MAXIMUM of this definition and the size of
 *  the union of all well known message types. If there are custom messages
 *  that must fit in the message payload that exceed this size then the final
 *  final payload size can be influenced by adjusting this definition.
 */
#define A2B_CONF_MSG_MIN_PAYLOAD_SIZE       (0u)


/** Define the maximum bytes that the EEPROM peripheral processing can
 *  read at a time for a single configuration block payload.  The max
 *  payload size is 4095 bytes. NOTE: If the configuration block exceeds
 *  this size problems could result depending on the data written.
 */
#define A2B_MAX_PERIPHERAL_BUFFER_SIZE      (4095u)

/** This is the number of interrupts processed in a row before waiting for
 *  the next schedule tick. -1 indicates that ALL interrupts are processed
 *  before exiting the processing loop.
 */
#define A2B_CONF_CONSECUTIVE_INTERRUPTS     (3u)

/** This macro detects whether the A2B chip is an older AD241X
 * series chips.
 */
#define A2B_IS_AD241X_CHIP(vid, pid) \
            ((a2b_Bool)(((pid == 0x01u) || (pid == 0x02u) || \
             (pid == 0x03u) || (pid == 0x10u)) && (vid == 0xADu)))

/** This macro detects whether the A2B chip is a newer AD24XX
 * series chip.
 */
#define A2B_IS_AD242X_CHIP(vid, pid) \
            ((a2b_Bool)((vid == 0xADu) && ((pid == 0x21u) || (pid == 0x22u) || \
                                           (pid == 0x23u) || (pid == 0x25u) || \
                                           (pid == 0x26u) || (pid == 0x27u) || \
                                           (pid == 0x28u) || (pid == 0x29u) || \
                                           (pid == 0x20u))))
/** This macro detects whether the A2B chip is a newer AD2428x (AD2428, AD2426, AD2427)
 * series chip.
 */
#define A2B_IS_AD2428X_CHIP(vid, pid) \
	((a2b_Bool)((vid == 0xADu) && ((pid == 0x26u) || (pid == 0x27u) || \
	(pid == 0x28u)||(pid == 0x29u) || (pid == 0x20u))))

/** This macro detects whether the A2B chip is a AD2442x (AD2428, AD2426, AD2427)
 * series chip.
 */
#define A2B_IS_AD2442X_CHIP(vid, pid) \
	((a2b_Bool)((vid == 0xADu) && ((pid == 0x25u) || (pid == 0x24u))))

/** This defines the logic used when a node is discovered
 *  to verify that the discovered node can be supported
 *  by the stack. (vid=vendorId, pid=productId, ver=version)
 */
#define A2B_STACK_SUPPORTED_NODE(vid, pid, ver) \
            ( \
              /* Older AD241X chips with versions 0x10 & 0x21 */ \
              (a2b_Bool)(A2B_IS_AD241X_CHIP(vid, pid) || \
              /* Newer AD242X chips and version 0x00 */ \
              A2B_IS_AD242X_CHIP(vid, pid)) \
            )

/** Maximum number of smart A2B slave nodes with comm channel support */
#ifdef A2B_FEATURE_COMM_CH
	#ifndef A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES
	/** Define the number of smart slaves that are required to be supported
	 * in master plugin for messaging via communication channel.One
	 * communication channel instance is created per smart slave.
	 * \note The global memory is reserved for thsese number of instances
	 * in master plugin hence this macro controls the memory footprint of
	 * communication channels.
	 * */
	#define A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES		(3u)
#endif
#endif

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_END_DECLS

/*======================= D A T A =================================*/



/*===================== DO NOT EDIT ===============================*/

#if A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS == 0
/** This setting expects ALL cfg blocks to be processed
 *  prior to ANY other peripheral config.  Therefore we
 *  enable the #A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
 *  to avoid the uneccessary excess mailboxes/memory for
 *  something that will not be used.
 */
#define A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
#endif

#ifndef A2B_FEATURE_EEPROM_PROCESSING
/** When EEPROM processing is disabled we should ensure
 *  the smallest footprint, so make sure the job queue
 *  size is not increased.  Reduce by one since one was
 *  added for peripheral processing which is NOT needed.
 */
#define TMP_ADJ_CONF_MAX_NUM_JOB_QUEUES     (TMP_CONF_MAX_NUM_JOB_QUEUES-1u)

#elif defined(A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY)
/** Only one mailbox for all peripheral processing thus
 *  the reason for the blocking/wait on cfg delay.
 */
#define TMP_ADJ_CONF_MAX_NUM_JOB_QUEUES     TMP_CONF_MAX_NUM_JOB_QUEUES

#else
/** When #A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY is enabled
 *  one mailbox is used per node for possible peripheral
 *  processing. We will need to increase the number of
 *  job queues needed.
 */
#define TMP_ADJ_CONF_MAX_NUM_JOB_QUEUES     (TMP_CONF_MAX_NUM_JOB_QUEUES +  \
                                         (A2B_CONF_MAX_NUM_SLAVE_NODES-1u))
#endif

#ifndef A2B_FEATURE_COMM_CH
/** When communication channel is disabled reduce by one
 * since one was added for communication channel to master plugin
 * messaging which is NOT needed.
 */
#define A2B_CONF_MAX_NUM_JOB_QUEUES		(TMP_ADJ_CONF_MAX_NUM_JOB_QUEUES-1u)
#else
#define A2B_CONF_MAX_NUM_JOB_QUEUES		TMP_ADJ_CONF_MAX_NUM_JOB_QUEUES
#endif


/*===================== DO NOT EDIT ===============================*/

/** \} -- a2bstack_conf */

#endif /* A2B_CONF_H_ */
