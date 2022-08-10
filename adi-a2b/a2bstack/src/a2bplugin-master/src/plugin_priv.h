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
 * \file:   plugin_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the definition of a A2B master node plugin.
 *
 *=============================================================================
 */

#ifndef A2B_PLUGIN_PRIV_H_
#define A2B_PLUGIN_PRIV_H_

/*======================= I N C L U D E S =========================*/

#ifndef PB_SYSTEM_HEADER
#define PB_SYSTEM_HEADER    "pb_syshdr.h"
#endif

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/conf.h"
#include "a2b/msgtypes.h"
#include "bdd_pb2.pb.h"
#include "periphutil.h"
#include "pwrdiag.h"
#include "a2bplugin-master/plugin.h"
#ifdef A2B_FEATURE_COMM_CH
#include "adi_a2b_commch_mstr.h"
#endif	/* A2B_FEATURE_COMM_CH */

/*======================= D E F I N E S ===========================*/
/*! \addtogroup a2bplugin_master_plugin
 *  @{
 */

/** Internal discovery message commands */
#define A2B_MPLUGIN_START_PERIPH_CFG    (1u + A2B_MPLUGIN_MAX)
#define A2B_MPLUGIN_CONT_PERIPH_CFG     (2u + A2B_MPLUGIN_MAX)

/* Last well defined master plugin command */
#define A2B_MPLUGIN_PRIV_MAX            (3u + A2B_MPLUGIN_MAX)

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Timer;
struct a2b_Plugin;

enum
{
	/** Plugin execution success */
    A2B_PLUGIN_EC_OK = 0,
	/** Plugin execution failed, due to Bad arguments */
    A2B_PLUGIN_EC_BAD_ARG,
	/** Plugin execution failed, due to wrong commands */
    A2B_PLUGIN_EC_UNKNOWN_CMD
};

typedef struct a2b_PeripheralNode
{
	/** Num of Peripheral Blocks */
    a2b_UInt8                   nCfgBlocks;
    /** Current cfg block [0..n-1] */
    a2b_UInt8                   cfgIdx;
    /** Last read EEPROM Address*/
    a2b_UInt16                  addr;
    /** Timer used for peripheral delays */
    struct a2b_Timer*           timer;
    /** This is needed for timers, etc. */
    struct a2b_Plugin*          plugin;
    /** index into a2b_Peripheral.node */
    a2b_Int16                   nodeAddr;
    /** Mailbox Handler */
    a2b_Handle                  mboxHnd;

} a2b_PeripheralNode;

typedef struct a2b_PeripheralCfg
{
    /** This is the optional EEPROM peripheral config passed
     *  in by the application using the EEPROM peripheral
     *  package configuration.  Once this is passed into the
     *  master plugin it MUST not be deleted by the application
     *  (the discovery process keeps a reference to the data)
     *  This can also be the pointer to the peripheral config
     *  table for the current branch.
     */
    const a2b_UInt8*    pkgCfg;
    /** This is the length of the Peripheral config table or buffer
     */
    a2b_UInt32          pkgLen;

    /** Pointer into the correct location for each node
     * to quickly and easily get its EEPROM peripheral config.
     */
    const a2b_UInt8*    nodeCfg[A2B_CONF_MAX_NUM_SLAVE_NODES];
    /** Length of the peripheral  config buffer for each node
     */
    a2b_UInt32          nodeLen[A2B_CONF_MAX_NUM_SLAVE_NODES];

} a2b_PeripheralCfg;

typedef struct a2b_Peripheral
{
    /** This is the buffer used for reading peripheral data.
     *  This cannot be on the stack because of its configurable
     *  size.  It also must be specific to a plugin and not global.
     */
    a2b_UInt8                   rBuf[A2B_MAX_PERIPHERAL_BUFFER_SIZE];
    
    #ifdef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
    /** Only one mailbox for all peripheral processing thus
     *  the reason for the blocking/wait on cfg delay.
     */
    a2b_PeripheralNode          node[1];
    #else
    a2b_PeripheralNode          node[A2B_CONF_MAX_NUM_SLAVE_NODES];
    #endif

} a2b_Peripheral;

typedef struct a2b_PluginDiscovery
{
	/** Flag to indicate whether discovery is in process
	 */
    a2b_Bool                    inDiscovery;

	/** Flag to indicate discovery is complete
	 */
    a2b_Bool                    discoveryComplete;
	/** Error code of the discovery completion
	 */
    a2b_UInt32                  discoveryCompleteCode;

    /** Bitmask for each node to indicates if there is a
     * an EEPROM available for processing.
     */
    a2b_UInt32                  hasEeprom;

    /** Bitmask for each node to indicates if there is a
     * plugin associated with a node.  This is needed for 
     * peripheral init/deinit to avoid sending to something 
     * that does not exist.  This keeps the logging and simple 
     * discovery mode processing easier/more clear.
     */
    a2b_UInt32                  hasPlugin;

    /** Bit mask for each node that indicates whether we've tried to find
     * a suitable handler (e.g. plugin) to manage the given node. This flag
     * prevents the code from trying to find a suitable handler more than
     * one time.
     */
    a2b_UInt32                  hasSearchedForHandler;

    /** Used during simple discovery with synchronous peripheral
     * processing enabled and discovey timeout checking.  Tracks 
     * when to finalize the network with master settings.  Should 
     * have NO other use, NOP for all other use cases since the 
     * value will be changed for the use cases use. 
     */
    a2b_UInt8                   simpleNodeCount;

    /** Used to track the number of plugins that we need to wait
     * to initialized.  This will prevent us from ending the
     * discovery process prior to getting a response from all 
     * plugins indicating they have intialized. 
     */
    a2b_UInt8                   pendingPluginInit;

    /** Used to track the number of plugins that we need to wait
     * to deinitialized.
     */
    a2b_UInt8                   pendingPluginDeinit;

    /** Bitmask indicating that the plugin needs a plugin init.
     * Used to avoid not or over sending the init.  If there is 
     * no plugin for the node, its bit will be zero. 
     */
    a2b_UInt32                  needsPluginInit;

    /** This is the total nodes discovered. Should treat as Read-only
     * and will not decrement (unlike simpleNoodeCount).
     */
    a2b_UInt8                   dscNumNodes;
                          
} a2b_PluginDiscovery;

typedef struct a2b_SlaveInitCtx
{
    a2b_Int16           curNode;
    a2b_Int16           lastNode;
    a2b_Handle          userData;
    a2b_HResult         lastError;
    a2b_OpDoneFunc      opDone;
} a2b_SlaveInitCtx;

#if !defined(__ADSP214xx__)
A2B_STATIC_ASSERT(sizeof(a2b_UInt32) * A2B_CONF_CHAR_BIT >=
                A2B_CONF_MAX_NUM_SLAVE_NODES,
                 "More slave plugins than will fit in mask");
#endif	/* !defined(__ADSP214xx__) */

#ifdef A2B_FEATURE_COMM_CH
typedef struct a2b_CommCh
{
	/** Array of communication channel instances one per smart slave node */
	a2b_Handle					commChHnd[A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES];

	/** Global Pool of memory used for all the communication channel instances
	* This cannot be on the stack as communication channel instances are handled
	*  by the master plugin alone
	*  */
	a2b_UInt8 					gCommChMem[A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES*sizeof(a2b_CommChMstrInfo)];

	/** Array of Slave Node Ids directly mapped to array of communication channel instances
	 * A value of -1 indicates that mapping is invalid
	**/
	a2b_Int16					commChSlvNodeIds[A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES];

	/** Pointer to the timer for timeout on non acknowledgement of  transmission over Mailbox per instance */
	struct a2b_Timer*           mboxTimeoutTimer[A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES];

	/** Stack Mailbox Handler (job queue)  for 4 byte mailbox write requests from Communication channel to plugin
	* This separate job queue is required as node authentication using communication channel happens
	*  during discovery when default job queue is in suspended execution state till discovery is complete.
	* */
	a2b_Handle                  commChMboxHnd;

} a2b_CommCh;
#endif

typedef struct a2b_Plugin
{
	/** Pointer to the stack Context for the master plugin */
    struct a2b_StackContext*    ctx;
    /** Node Signature structure for the master */
    a2b_NodeSignature           nodeSig;
    /** Node Signature structure for all the slave nodes attached to this master */
    a2b_NodeSignature           slaveNodeSig[A2B_CONF_MAX_NUM_SLAVE_NODES];
    /** Flag to indicate whether the Plugin is in use */
    a2b_Bool                    inUse;
    /** Pointer to the timer */
    struct a2b_Timer*           timer;

    /** Loaded network BDD specific to this stack */
    a2b_Bool                    bddLoaded;
    /** Pointer to the BDD Network structure */
    const bdd_Network*          bdd;

    /** Discovery tracking */
    a2b_PluginDiscovery         discovery;

    /** Power fault diagnosis context */
    a2b_PwrDiagCtx              pwrDiag;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
    /** EEPROM peripheral processing */
    a2b_Peripheral              periph;

    /** BDD related binary data for peripheral config */
    a2b_PeripheralCfg           periphCfg;
#else  /* A2B_FEATURE_EEPROM_PROCESSING */
    /** BDD related binary data for peripheral config */
        a2b_PeripheralCfg           periphCfg;
#endif /* A2B_FEATURE_EEPROM_PROCESSING */
    /** Bookkeeping information for slave plugin init/deinit */
    a2b_SlaveInitCtx            slaveInitCtx;

    /** Master plugin overrides, see A2B_MPLUGIN_DSCVRY_OVERRIDES */
    a2b_UInt32                  overrides[2];

    /** A2B_MSGREQ_PLUGIN_PERIPH_INIT TDM used for all plugins */
    a2b_TdmSettings             pluginTdmSettings;
#ifdef A2B_RUN_BIT_ERROR_TEST
    /** ADI_A2B_BERT_HANDLER used for BERT measurements*/

    ADI_A2B_BERT_HANDLER*      pBertHandler;
#endif

#ifdef DISABLE_PWRDIAG
     a2b_Bool             bDisablePwrDiag;
#endif
#ifdef A2B_FEATURE_COMM_CH
     /** Communication Channel */
     a2b_CommCh					commCh;
#endif
} a2b_Plugin;

/* Forward declarations */
struct a2b_PluginApi;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_DSO_PUBLIC a2b_Bool A2B_CALL A2B_PLUGIN_INIT(struct a2b_PluginApi* api);

/**
 @}
*/


/*! \addtogroup BERT BERT
 *  @{
 */
#ifdef A2B_RUN_BIT_ERROR_TEST
void adi_a2b_BertIntiation(a2b_UInt8 *pBertConfigBuffer, a2b_Plugin*  pPlugin);
void adi_a2b_BertUpdate(a2b_Plugin*  pPlugin);
void adi_a2b_BertStop(a2b_Plugin*  pPlugin);
#endif

/**
 @}
*/
A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_PLUGIN_PRIV_H_ */
