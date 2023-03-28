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
 * \file:   discovery.c
 * \brief:  The implementation of an A2B master plugin discovery process.
 *
 *=============================================================================
 */
/*! \addtogroup Network_Configuration
 *  @{
 */

/** @defgroup Discovery_and_Node_Configuration Discovery and Node Configuration
 *
 * This module discovers the node and configures the nodes based on the BDD configuration
 *
 */

/*! \addtogroup Discovery_and_Node_Configuration
 *  @{
 */

/*======================= I N C L U D E S =========================*/
#include "a2b/pluginapi.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/defs.h"
#include "a2b/util.h"
#include "a2b/msg.h"
#include "a2b/msgrtr.h"
#include "a2b/trace.h"
#include "a2b/stack.h"
#include "a2b/regdefs.h"
#include "a2b/interrupt.h"
#include "a2b/stackctxmailbox.h"
#include "a2b/i2c.h"
#include "a2b/timer.h"
#include "a2b/seqchart.h"
#include "a2bplugin-master/plugin.h"
#include "discovery.h"
#include "periphcfg.h"
#include "override.h"
#include "a2b/audio.h"
#ifdef _TESSY_INCLUDES_
#include "msg_priv.h"
#include "a2b/msgtypes.h"
#include "timer_priv.h"
#include "stackctx.h"
#endif	/* _TESSY_INCLUDES_ */
#ifdef ADI_BDD
#include "adi_a2b_busconfig.h"
#endif

/*======================= D E F I N E S ===========================*/

#define A2B_MASTER_NODEBDDIDX   (0u)

/** Delay (in msec) for a node discovery */
#define A2B_DISCOVERY_DELAY     (50u)

/** Delay (in msec) to wait after a software reset */
#define A2B_SW_RESET_DELAY      (25u)


#define A2B_DEINIT_DSCVREY_END  (1u)
#define A2B_DEINIT_START        (2u)

/** Define if a search for a plugin to manage a node should be done
 * AFTER the node itself has been completely initialized. If undefined
 * then a search for the managing plugin (including opening the plugin)
 * will occur immediately after the slave node has been discovered but
 * BEFORE the node itself has been initialized.
 */
#if 1
#define FIND_NODE_HANDLER_AFTER_NODE_INIT   (1u)
#else
#undef FIND_NODE_HANDLER_AFTER_NODE_INIT
#endif

#ifdef A2B_QAC
#pragma PRQA_NO_SIDE_EFFECTS a2b_isAd242xChip
#endif
/*======================= L O C A L  P R O T O T Y P E S  =========*/

typedef enum
{
    TIMER_DSCVRY,
    TIMER_RESET
} a2b_dscvryTimer;


/*======================= D A T A  ================================*/


/*======================= M A C R O S =============================*/

#define A2B_HAS_EEPROM( plugin, nodeAddr ) \
        ((a2b_UInt32)((plugin)->discovery.hasEeprom) & ((a2b_UInt32)1u << (a2b_UInt32)(nodeAddr)))

#define A2B_HAS_PLUGIN( plugin, nodeAddr ) \
        ((a2b_UInt32)((plugin)->discovery.hasPlugin) & ((a2b_UInt32)1u << (a2b_UInt32)(nodeAddr)))

#define A2B_NEEDS_PLUGIN_INIT( plugin, nodeAddr ) \
        ((a2b_UInt32)((plugin)->discovery.needsPluginInit) & ((a2b_UInt32)1u << (a2b_UInt32)(nodeAddr)))

#define A2B_HAS_SEARCHED_FOR_HANLDER(plugin, nodeAddr ) \
        ((a2b_UInt32)((plugin)->discovery.hasSearchedForHandler) & ((a2b_UInt32)1u << (a2b_UInt32)(nodeAddr)))

/* Maps slave node address to an internal slave array index */
#define A2B_MAP_SLAVE_ADDR_TO_INDEX(a)  ((a2b_UInt16)(a))
/* Maps slave array index to slave node address */
#define A2B_MAP_SLAVE_INDEX_TO_ADDR(i)  ((a2b_Int16)(i))

/*======================= C O D E =================================*/

static a2b_Int32 	a2b_dscvryNodeComplete(a2b_Plugin* plugin, a2b_Int16 nodeAddr, a2b_Bool bDoEepromCfg, a2b_UInt32* errCode);
static void 		a2b_dscvryNetComplete(a2b_Plugin* plugin);
static a2b_Bool 	a2b_dscvryPreMasterInit(a2b_Plugin* plugin);
static a2b_Bool 	a2b_dscvryPreSlaveInit(a2b_Plugin* plugin);
static a2b_Int32 	a2b_dscvryReset(a2b_Plugin* plugin);
static void 		a2b_dscvryFindNodeHandler(a2b_Plugin* plugin, a2b_UInt16 slaveNodeIdx);
static void 		a2b_dscvryInitTdmSettings(a2b_Plugin* plugin,a2b_Int16 nodeAddr);
static void 		a2b_dscvryDeinitPluginComplete( struct a2b_Msg* msg, a2b_Bool isCancelled);
static void 		a2b_dscvryInitPluginComplete_NoEeprom(struct a2b_Msg* msg, a2b_Bool  isCancelled);
static void 		a2b_dscvryInitPluginComplete_EepromComplete(struct a2b_Msg* msg, a2b_Bool isCancelled);
static void 		a2b_onDiscTimeout(struct a2b_Timer *timer, a2b_Handle userData);
static void 		a2b_onResetTimeout(struct a2b_Timer *timer, a2b_Handle userData);
static a2b_Bool 	a2b_dscvryStartTimer(a2b_Plugin* plugin, a2b_dscvryTimer type);
static a2b_Bool 	a2b_dscvryNodeInterruptInit(a2b_Plugin* plugin, a2b_Int16 nodeBddIdx);
static const a2b_NodeSignature* a2b_getNodeSignature(a2b_Plugin* plugin, a2b_Int16 nodeAddr);
static a2b_Bool 	a2b_SimpleModeChkNodeConfig(a2b_Plugin* plugin);
static void 		adi_a2b_ReConfigSlot(a2b_Plugin* plugin, a2b_Int16 nodeAddr);
static a2b_HResult 	a2b_FinalMasterSetup(a2b_Plugin* plugin, a2b_Int16 nodeAddr);
static a2b_Bool 	adi_a2b_ConfigureNodePeri(a2b_Plugin* plugin, a2b_Int16 dscNodeAddr);
static a2b_Bool		adi_a2b_ConfigNodeOptimizAdvancedMode(a2b_Plugin* plugin, a2b_Int16 dscNodeAddr);
static a2b_UInt32 	a2b_dscvryInitPlugin(a2b_Plugin* plugin, a2b_Int16  nodeAddr, a2b_MsgCallbackFunc completeCallback);
static void 		a2b_dscvryDeinitPlugin(a2b_Plugin* plugin, a2b_UInt32  mode);
static a2b_HResult 	a2b_ConfigSpreadSpectrum(a2b_Plugin* plugin, a2b_Int16 nodeAddr);
static a2b_Bool		a2b_isAd242xChip(a2b_UInt8 vendorId, a2b_UInt8 productId);
static a2b_Bool 	a2b_stackSupportedNode(a2b_UInt8 vendorId, a2b_UInt8 productId, a2b_UInt8 version);
#ifdef A2B_FEATURE_COMM_CH
static a2b_Bool 	a2b_dscvryNodeMailboxInit (a2b_Plugin* plugin, a2b_Int16 nodeBddIdx);
static a2b_Bool 	a2b_dscvryPostAuthViaCommCh(a2b_Plugin* plugin);
static a2b_Bool 	a2b_dscvryStartCommChAuthTimer(a2b_Plugin* plugin, a2b_UInt16 delay);
static void 		a2b_onCommChAuthTimeout(struct a2b_Timer *timer, a2b_Handle userData);
#endif	/* A2B_FEATURE_COMM_CH */
/*!****************************************************************************
*
*  \b              a2b_dscvryInitTdmSettings
*
*  Initialize the master plugins TDM Settings for a specific node.
*
*  \param          [in]    plugin
*  \param          [in]    nodeAddr
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryInitTdmSettings
    (
    a2b_Plugin*         plugin,
    a2b_Int16           nodeAddr
    )
{
    a2b_Int16           nodeBddIdx = nodeAddr+1;
    const bdd_Node      *bddNodeObj;
    a2b_TdmSettings*    tdmSettings;
    a2b_UInt32          reg;
    a2b_UInt32          streamIdx;

    (void)a2b_memset( &plugin->pluginTdmSettings, 0, sizeof(a2b_TdmSettings) );

    if ( nodeBddIdx > (a2b_Int16)plugin->bdd->nodes_count- (a2b_Int16)1)
    {
        A2B_DSCVRY_ERROR0( plugin->ctx, "a2b_dscvryInitTdmSettings",
                           "Invalid nodeAddr" );
        return;
    }

    bddNodeObj  = &plugin->bdd->nodes[nodeBddIdx];
    tdmSettings = &plugin->pluginTdmSettings;

    tdmSettings->networkSampleRate =
                                 ((a2b_UInt16)plugin->bdd->sampleRate & (a2b_UInt16)0xFFFF);

    if ( bddNodeObj->i2cI2sRegs.has_i2srate )
    {
        switch ( (bddNodeObj->i2cI2sRegs.i2srate & A2B_BITM_I2SRATE_I2SRATE) )
        {
        case A2B_ENUM_I2SRATE_1X_SFF:
            tdmSettings->sampleRateMultiplier = 1u;
            break;
        case A2B_ENUM_I2SRATE_2X_SFF:
            tdmSettings->sampleRateMultiplier = 2u;
            break;
        case A2B_ENUM_I2SRATE_4X_SFF:
            tdmSettings->sampleRateMultiplier = 4u;
            break;
        /* TODO: We need to handle the Reduced Rate (RRDIV) enumerated
         * rates introduced by the AD242X chips. This likely means we will
         * need to to enumerate the possible values and include the RRDIV
         * values itself.
         */
        case A2B_ENUM_I2SRATE_SFF_RRDIV:
            tdmSettings->sampleRateMultiplier = 3u;
            break;
        case A2B_ENUM_I2SRATE_SFF_DIV_4:
            tdmSettings->sampleRateMultiplier = 2u;
            break;
        case A2B_ENUM_I2SRATE_SFF_DIV_2:
            tdmSettings->sampleRateMultiplier = 1u;
            break;
        default:
            tdmSettings->sampleRateMultiplier = 1u;
            break;

        }
    }

    switch ( bddNodeObj->i2cI2sRegs.i2sgcfg & A2B_BITM_I2SGCFG_TDMMODE )
    {
    case A2B_ENUM_I2SGCFG_TDM2:
        tdmSettings->tdmMode = 2u;
        break;
    case A2B_ENUM_I2SGCFG_TDM4:
        tdmSettings->tdmMode = 4u;
        break;
    case A2B_ENUM_I2SGCFG_TDM8:
        tdmSettings->tdmMode = 8u;
        break;
    case A2B_ENUM_I2SGCFG_TDM16:
        tdmSettings->tdmMode = 16u;
        break;
    case A2B_ENUM_I2SGCFG_TDM32:
        tdmSettings->tdmMode = 32u;
        break;
    default:
        tdmSettings->tdmMode = 2u;
        break;
    }

    reg = bddNodeObj->i2cI2sRegs.i2sgcfg;
    tdmSettings->slotSize    = (reg & A2B_BITM_I2SGCFG_TDMSS ) ? 16u : 32u;
    tdmSettings->halfCycle   = (a2b_Bool)(( reg & A2B_BITM_I2SGCFG_ALT ) ==
                                  A2B_ENUM_I2SGCFG_ALT_EN );
    tdmSettings->prevCycle   = (a2b_Bool)(( reg & A2B_BITM_I2SGCFG_EARLY ) ==
                                  A2B_ENUM_I2SGCFG_EARLY_EN );
    tdmSettings->fallingEdge = (a2b_Bool)(( reg & A2B_BITM_I2SGCFG_INV ) ==
                                  A2B_ENUM_I2SGCFG_INV_EN );

    reg = bddNodeObj->i2cI2sRegs.i2scfg;
    tdmSettings->rx.invertBclk = (a2b_Bool)(( reg & A2B_BITM_I2SCFG_RXBCLKINV ) ==
                                    A2B_ENUM_I2SCFG_RXBCLKINV_EN );
    tdmSettings->rx.interleave = (a2b_Bool)(( reg & A2B_BITM_I2SCFG_RX2PINTL ) ==
                                    A2B_ENUM_I2SCFG_RX2PINTL_EN );
    tdmSettings->rx.pinEnabled = (a2b_UInt8)(( reg & (A2B_BITM_I2SCFG_RX0EN |
                               A2B_BITM_I2SCFG_RX1EN)) >> A2B_BITP_I2SCFG_RX0EN);

    if ( bddNodeObj->i2cI2sRegs.has_i2srxoffset )
    {
        switch ( bddNodeObj->i2cI2sRegs.i2srxoffset &
                 A2B_BITM_I2SRXOFFSET_RXOFFSET )
        {
        case A2B_ENUM_I2SRXOFFSET_00:
            tdmSettings->rx.offset = 0u;
            break;
        case A2B_ENUM_I2SRXOFFSET_62:
            tdmSettings->rx.offset = 62u;
            break;
        case A2B_ENUM_I2SRXOFFSET_63:
            tdmSettings->rx.offset = 63u;
            break;
        default:
            tdmSettings->rx.offset = 0u;
            break;
        }
    }

    tdmSettings->tx.invertBclk = (a2b_Bool)(( reg & A2B_BITM_I2SCFG_TXBCLKINV ) ==
                                    A2B_ENUM_I2SCFG_TXBCLKINV_EN );
    tdmSettings->tx.interleave = (a2b_Bool)(( reg & A2B_BITM_I2SCFG_TX2PINTL ) ==
                                    A2B_ENUM_I2SCFG_TX2PINTL_EN );
    tdmSettings->tx.pinEnabled = (a2b_UInt8)((reg & (A2B_BITM_I2SCFG_TX0EN |
                              A2B_BITM_I2SCFG_TX1EN) ) >> A2B_BITP_I2SCFG_TX0EN);

    if ( bddNodeObj->i2cI2sRegs.has_i2stxoffset )
    {
        switch ( bddNodeObj->i2cI2sRegs.i2stxoffset &
                 A2B_BITM_I2STXOFFSET_TXOFFSET )
        {
        case A2B_ENUM_I2STXOFFSET_TXOFFSET_00:
            tdmSettings->tx.offset = 0u;
            break;
        case A2B_ENUM_I2STXOFFSET_TXOFFSET_01:
            tdmSettings->tx.offset = 1u;
            break;
        case A2B_ENUM_I2STXOFFSET_TXOFFSET_62:
            tdmSettings->tx.offset = 62u;
            break;
        case A2B_ENUM_I2STXOFFSET_TXOFFSET_63:
            tdmSettings->tx.offset = 63u;
            break;
        default:
            tdmSettings->tx.offset = 0u;
            break;
        }

        tdmSettings->tx.triStateBefore = (a2b_Bool)(( bddNodeObj->i2cI2sRegs.i2stxoffset &
                                           A2B_BITM_I2STXOFFSET_TSBEFORE ) ==
                                           A2B_ENUM_I2STXOFFSET_TSBEFORE_EN );
        tdmSettings->tx.triStateAfter  = (a2b_Bool)(( bddNodeObj->i2cI2sRegs.i2stxoffset &
                                           A2B_BITM_I2STXOFFSET_TSAFTER ) ==
                                           A2B_ENUM_I2STXOFFSET_TSAFTER_EN );
    }

    /* Build the downstream listing */
    tdmSettings->downstreamBcastCnt = bddNodeObj->downstreamBcastCnt;
    for (streamIdx = 0u; streamIdx < bddNodeObj->downstream_count; streamIdx++)
    {
        if ( bddNodeObj->downstream[streamIdx] < plugin->bdd->streams_count )
        {
            tdmSettings->downstream[streamIdx] = (const a2b_StreamInfo*)
                      &plugin->bdd->streams[ bddNodeObj->downstream[streamIdx] ];
        }
    }

    /* Build the upstream listing */
    tdmSettings->upstreamBcastCnt = bddNodeObj->upstreamBcastCnt;
    for (streamIdx = 0u; streamIdx < bddNodeObj->upstream_count; streamIdx++)
    {
        if ( bddNodeObj->upstream[streamIdx] < plugin->bdd->streams_count )
        {
            tdmSettings->upstream[streamIdx] = (const a2b_StreamInfo*)
                        &plugin->bdd->streams[ bddNodeObj->upstream[streamIdx] ];
        }
    }

} /* a2b_dscvryInitTdmSettings */


/*!****************************************************************************
*
*  \b              a2b_dscvryDeinitPluginComplete
*
*  Callback when A2B_MSGREQ_PLUGIN_PERIPH_DEINIT has completed processing.
*  This can be called from the dscvryEnd or the Start routines, depends on TID.
*
*  \param          [in]    msg          The response to the de-init request.
*
*  \param          [in]    isCancelled  An indication of whether the request
*                                       was cancelled.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryDeinitPluginComplete
    (
    struct a2b_Msg* msg,
    a2b_Bool        isCancelled
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)a2b_msgGetUserData( msg );

    A2B_UNUSED(isCancelled);

    if ( plugin )
    {
        /* One less pending de-initialization response. */
        plugin->discovery.pendingPluginDeinit--;

        if ( msg )
        {
            /* Whether the de-initialization of the node's peripherals failed
             * (or not) for this plugin we'll continue processing but at least
             * trace the result.
             */
            A2B_TRACE3( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                "%s DeinitPluginComplete(): node=%hd status=0x%lX",
                A2B_MPLUGIN_PLUGIN_NAME,
                &plugin->nodeSig.nodeAddr,
                &((a2b_PluginDeinit*)a2b_msgGetPayload(msg))->resp.status) );
        }

        if ( plugin->discovery.pendingPluginDeinit == 0u )
        {
            if ( A2B_DEINIT_DSCVREY_END == a2b_msgGetTid( msg ) )
            {
                a2b_dscvryEnd( plugin,
                               plugin->discovery.discoveryCompleteCode );
            }
            else
            {
                (void)a2b_dscvryReset( plugin );

                /* On A2B_EXEC_COMPLETE a2b_dscvryEnd already called and
                 * when continuing we NOP here as well.
                 */
            }
        }
    }

} /* a2b_dscvryDeinitPluginComplete */


/*!****************************************************************************
*
*  \b              a2b_dscvryInitPluginComplete_NoEeprom
*
*  Callback when A2B_MSGREQ_PLUGIN_PERIPH_INIT has completed processing.
*  This is specific to the case when No EEPROM is detected or supported.
*
*  \param          [in]    msg          The response to the peripheral init
*                                       request.
*
*  \param          [in]    isCancelled  An indication of whether or not
*                                       the request was cancelled.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryInitPluginComplete_NoEeprom
    (
    struct a2b_Msg* msg,
    a2b_Bool        isCancelled
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)a2b_msgGetUserData( msg );
    a2b_UInt32 nodeAddr = a2b_msgGetTid( msg );
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_PLUGIN,
                                        A2B_EC_INTERNAL);
    bdd_DiscoveryMode eDscMode;
    a2b_UInt32 nDscvrdNode = 0u;

     A2B_UNUSED(isCancelled);

     eDscMode = a2b_ovrGetDiscMode(plugin);

    if ( msg )
    {
		if ( A2B_HAS_PLUGIN(plugin, nodeAddr) )
		{
			plugin->discovery.pendingPluginInit--;
		}
	    nDscvrdNode = (a2b_UInt32)plugin->discovery.dscNumNodes-(a2b_UInt32)1u;
		/* Get the result of the plugin peripheral initialization */
        status = ((a2b_PluginInit*)a2b_msgGetPayload(msg))->resp.status;
    }

    /* If the plugin peripheral initialization failed then ... */
    if ( A2B_FAILED(status) )
    {
        a2b_dscvryEnd(plugin, A2B_ERR_CODE(status));
    }
    else if ( bdd_DISCOVERY_MODE_MODIFIED == eDscMode )
    {
        (void)a2b_dscvryPreSlaveInit( plugin );

        /* Now we wait for INTTYPE.DSCDONE on success */
    }
    else if((bdd_DISCOVERY_MODE_OPTIMIZED == eDscMode) ||
    		(bdd_DISCOVERY_MODE_ADVANCED == eDscMode))
    {
    	if(nDscvrdNode != nodeAddr)
    	{
    		(void)adi_a2b_ConfigNodeOptimizAdvancedMode(plugin, (a2b_Int16)nDscvrdNode);
    	}
    	else
    	{
			if(nodeAddr == ((a2b_UInt32)plugin->bdd->nodes_count-(a2b_UInt32)2u))
    		{
    			a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_OK );
    		}
		}
    }
    else
    {
        /* Completing the control statement */
    }
} /* a2b_dscvryInitPluginComplete_NoEeprom */


/*!****************************************************************************
*
*  \b              a2b_dscvryInitPluginComplete_EepromComplete
*
*  Callback when A2B_MSGREQ_PLUGIN_PERIPH_INIT has completed processing.
*  This is specific to the case when an EEPROM is found and has been
*  processed to completion.
*
*  \param          [in]    msg          Response for EEPROM completion.
*
*  \param          [in]    isCancelled  Indication of whether the request
*                                       was cancelled.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryInitPluginComplete_EepromComplete
    (
    struct a2b_Msg* msg,
    a2b_Bool        isCancelled
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)a2b_msgGetUserData( msg );
    a2b_UInt32 nodeAddr = a2b_msgGetTid( msg );
    a2b_UInt32 nDscvrdNode = 0u;
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_PLUGIN,
                                        A2B_EC_INTERNAL);
    bdd_DiscoveryMode eDscMode;

     A2B_UNUSED(isCancelled);

     eDscMode = a2b_ovrGetDiscMode(plugin);

    if ( msg )
    {

        if ( A2B_HAS_PLUGIN(plugin, nodeAddr) )
        {
            plugin->discovery.pendingPluginInit--;
        }
        nDscvrdNode = (a2b_UInt32)plugin->discovery.dscNumNodes-(a2b_UInt32)1u;
        /* Get the result of the plugin peripheral initialization */
        status = ((a2b_PluginInit*)a2b_msgGetPayload(msg))->resp.status;
    }

    /* If the plugin peripheral initialization failed then ... */
    if ( A2B_FAILED(status) )
    {
        a2b_dscvryEnd(plugin, A2B_ERR_CODE(status));
    }
    else if ( bdd_DISCOVERY_MODE_MODIFIED == a2b_ovrGetDiscMode(plugin) )
    {
        (void)a2b_dscvryPreSlaveInit( plugin );

        /* If returned true:
         * Now we wait for INTTYPE.DSCDONE on success
         * Else:
         * Complete
         */
    }
    else if((bdd_DISCOVERY_MODE_OPTIMIZED == eDscMode) ||
       		(bdd_DISCOVERY_MODE_ADVANCED == eDscMode))
    {
    	if(bdd_DISCOVERY_MODE_ADVANCED == eDscMode)
		{
			/* Since EEPROM configuration done for current node,
			 * re-configure slots up until  this node
			 * and re-initialize data flow again
			 *  */
			adi_a2b_ReConfigSlot(plugin, (a2b_Int16)nodeAddr);
			(void)a2b_FinalMasterSetup(plugin, A2B_NODEADDR_MASTER);
		}

    	/**********************************************************************
    	 * Also for the new node that has been discovered configure node ,
    	 * re-configure slots and re-initialize data flow again
    	 **********************************************************************/
       	if(nDscvrdNode != nodeAddr)
       	{

       		(void)adi_a2b_ConfigNodeOptimizAdvancedMode(plugin, (a2b_Int16)nDscvrdNode);
       	}
       	else
       	{
       		if(nodeAddr == ((a2b_UInt32)plugin->bdd->nodes_count-(a2b_UInt32)2u))
       		{
				a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_OK );
       		}
       	}
    }
    else
    {
        /* This will either process the next node or
         * complete the network processing
         */

#if !defined(A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY) && \
    defined(A2B_FEATURE_EEPROM_PROCESSING)
        /* This is to avoid wasted calls to a2b_dscvryNetComplete
         * that waste cycles and confuse the UML output.
         */
        if ((( plugin->discovery.hasEeprom == 0u ) &&
             ( plugin->discovery.pendingPluginInit == 0u ) ) ||
            (( plugin->discovery.hasEeprom ) &&
             ( a2b_periphCfgUsingSync() ) &&
             ( plugin->discovery.pendingPluginInit == 0u ) ) )
#endif
        {
            a2b_dscvryNetComplete( plugin );
        }
    }

} /* a2b_dscvryInitPluginComplete_EepromComplete */


/*!****************************************************************************
*
*  \b              a2b_dscvryDeinitPlugin
*
*  This is called to initiate A2B_MSGREQ_PLUGIN_PERIPH_DEINIT ALL slave plugins.
*
*  \param          [in]    plugin
*  \param          [in]    mode
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryDeinitPlugin
    (
    a2b_Plugin*         plugin,
    a2b_UInt32          mode
    )
{
    struct a2b_Msg* msg;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_Int16 nodeAddr;

    for ( nodeAddr = 0;
          nodeAddr < (a2b_Int16)plugin->discovery.dscNumNodes;
          nodeAddr++ )
    {
        if ( A2B_HAS_PLUGIN(plugin, nodeAddr) )
        {
            msg = a2b_msgAlloc( plugin->ctx,
                                A2B_MSG_REQUEST,
                                A2B_MSGREQ_PLUGIN_PERIPH_DEINIT );
            if ( msg )
            {
                a2b_msgSetUserData( msg, (a2b_Handle)plugin, A2B_NULL );
                a2b_msgSetTid( msg, mode );
                /* Assume failure de-initializing the plugin's peripherals. */
                ((a2b_PluginDeinit*)a2b_msgGetPayload(msg))->resp.status =
                                            A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                            A2B_FAC_PLUGIN,
                                            A2B_EC_INTERNAL);

                status = a2b_msgRtrSendRequest( msg,
                                         nodeAddr,
                                         &a2b_dscvryDeinitPluginComplete );

                if ( A2B_SUCCEEDED(status) )
                {
                    plugin->discovery.pendingPluginDeinit++;
                }

                /* Job executor now owns the message,
                 * or free on error
                 */
                (void)a2b_msgUnref( msg );
            }
        }
    }

} /* a2b_dscvryDeinitPlugin */


/*!****************************************************************************
*
*  \b              a2b_dscvryInitPlugin
*
*  This is called to initiate A2B_MSGREQ_PLUGIN_PERIPH_INIT with a specific
*  slave plugin.
*
*  \param          [in]    plugin
*  \param          [in]    nodeAddr
*  \param          [in]    completeCallback
*
*  \pre            None
*
*  \post           None
*
*  \return         [add here]
*
******************************************************************************/
static a2b_UInt32
a2b_dscvryInitPlugin
    (
    a2b_Plugin*         plugin,
    a2b_Int16           nodeAddr,
    a2b_MsgCallbackFunc completeCallback
    )
{
    struct a2b_Msg* msg;
    a2b_HResult     result;
    a2b_PluginInit* pluginInit;

    if ( A2B_NEEDS_PLUGIN_INIT( plugin, nodeAddr ) )
    {
        /* clear the bit */
        plugin->discovery.needsPluginInit ^= ((a2b_UInt32)1 << (a2b_UInt32)nodeAddr);
    }

    if ( A2B_HAS_PLUGIN(plugin, nodeAddr) )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1),
                    "InitPlugin: %hd ", &nodeAddr ) );
    }
    else
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1),
                    "No Plugin, Direct Init: %hd ", &nodeAddr ) );
    }

    msg = a2b_msgAlloc( plugin->ctx,
                        A2B_MSG_REQUEST,
                        A2B_MSGREQ_PLUGIN_PERIPH_INIT );
    if ( A2B_NULL == msg )
    {
        A2B_DSCVRY_ERROR0( plugin->ctx, "a2b_dscvryInitPlugin",
                           "Cannot allocate message" );
        return (a2b_UInt32)A2B_EC_RESOURCE_UNAVAIL;
    }

    a2b_msgSetUserData( msg, (a2b_Handle)plugin, A2B_NULL );
    a2b_msgSetTid( msg, (a2b_UInt32)nodeAddr );

    if ( !A2B_HAS_PLUGIN(plugin, (a2b_UInt32)nodeAddr) )
    {
        /* Without a plugin this call would fail, so
         * we call the requested callback now.  This
         * means the msg is somewhat incomplete and
         * MUST be used carefully.
         */
        completeCallback( msg , A2B_FALSE /* not cancelled */);
        (void)a2b_msgUnref( msg );

        return (a2b_UInt32)A2B_EC_OK;
    }

    /* Build the INIT payload */
    a2b_dscvryInitTdmSettings( plugin, nodeAddr );
    pluginInit = (a2b_PluginInit*)a2b_msgGetPayload( msg );
    pluginInit->req.tdmSettings = &plugin->pluginTdmSettings;
    pluginInit->req.pNodePeriDeviceConfig = (const void *)plugin->periphCfg.pkgCfg;
    /* Assume peripheral initialization failure */
    pluginInit->resp.status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                                A2B_FAC_PLUGIN,
                                                A2B_EC_INTERNAL);

    result = a2b_msgRtrSendRequest(msg,
                                   nodeAddr, /* destNodeAddr */
                                   completeCallback);

    /* Job executor now owns the message, or free on error */
    (void)a2b_msgUnref( msg );

    if ( A2B_FAILED(result) )
    {
        A2B_DSCVRY_ERROR1( plugin->ctx, "a2b_dscvryInitPlugin",
                           "Cannot send message (%ld)", &result );
        return (a2b_UInt32)A2B_EC_IO;
    }
    plugin->discovery.pendingPluginInit++;

    return (a2b_UInt32)A2B_EC_OK;

} /* a2b_dscvryInitPlugin */


/*!****************************************************************************
*
*  \b              a2b_dscvryEnd
*
*  Terminate/End the discovery process, and changed the scheduler
*  execution to A2B_EXEC_COMPLETE.  After this call the Job executor will
*  start processing jobs again for the master plugin.
*
*  \param          [in]    plugin   Master plugin record
*  \param          [in]    errCode  A2B_EC_xxx error code of the
*                                   discovery process
*
*  \pre            ONLY called when in a suspended mode on the job queue
*                  while processing discovery.
*
*  \post           Job executor will start processing jobs again for
*                  this master plugin.
*
*  \return         None
*
******************************************************************************/
void
a2b_dscvryEnd
    (
    a2b_Plugin* plugin,
    a2b_UInt32 errCode
    )
{
    a2b_UInt8  wBuf[4u];
    struct a2b_Msg* msg;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    bdd_DiscoveryMode eDscMode;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
    a2b_Bool periphComplete = A2B_TRUE;
    a2b_UInt8 idx;
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

    /* Stop the previously running timer */
    a2b_timerStop( plugin->timer );

    eDscMode = a2b_ovrGetDiscMode(plugin);
    if ( (a2b_UInt32)A2B_EC_OK == errCode )
    {
#ifdef A2B_FEATURE_EEPROM_PROCESSING
        /* See if all peripheral config has completed */
        periphComplete = (a2b_Bool)(plugin->discovery.hasEeprom == 0u);
#endif /* A2B_FEATURE_EEPROM_PROCESSING */
    }

    /* Discovery error */
    /* Check to make sure we have not already done this processing */

    else if ( plugin->discovery.discoveryCompleteCode == (a2b_UInt32)A2B_EC_OK )
    {
        /* Setting these to prevent executing this code again */
        plugin->discovery.discoveryComplete = A2B_TRUE;
        plugin->discovery.discoveryCompleteCode = errCode;

        /* On an error we need to clear the hasEeprom tracking */
        plugin->discovery.hasEeprom = 0u;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
        /* Stop all potential peripheral processing */
        for ( idx = 0u; idx < (a2b_UInt8)A2B_ARRAY_SIZE(plugin->periph.node); idx++ )
        {
            if ( plugin->periph.node[idx].mboxHnd )
            {
                (void)a2b_stackCtxMailboxFlush( plugin->ctx,
                                          plugin->periph.node[idx].mboxHnd );

                (void)a2b_stackCtxMailboxFree( plugin->ctx,
                                         plugin->periph.node[idx].mboxHnd );
                plugin->periph.node[idx].mboxHnd = A2B_NULL;
            }
        }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

        /* Send the A2B_MSGREQ_PLUGIN_PERIPH_DEINIT to all
         * discovered plugins.
         */
        a2b_dscvryDeinitPlugin( plugin, A2B_DEINIT_DSCVREY_END );

        if ( plugin->discovery.pendingPluginDeinit )
        {
            /* Wait until Plugin Deinit has been responded to */
            return;
        }
    }
    else
    {
        /* Completing the control statement */
    }

    /* Track that discovery has completed and what the error code is
     * so if the peripheral config is still running we can
     */
    plugin->discovery.discoveryComplete = A2B_TRUE;

    /* Once an error, always an error.  We don't want to clear the
     * final error on subsequent calls.
     */
    if ((a2b_UInt32)A2B_EC_OK == plugin->discovery.discoveryCompleteCode )
    {
        plugin->discovery.discoveryCompleteCode = errCode;
    }

#ifdef A2B_FEATURE_EEPROM_PROCESSING
    /* Only complete this when the peripherals are done */
    if ( !periphComplete )
    {
        return;
    }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

    /* Are we waiting for plugin de/initialization? */
    if (( plugin->discovery.pendingPluginInit ) ||
        ( plugin->discovery.pendingPluginDeinit ))
    {
        return;
    }
	/* Power is alreadu disabled in case of power faults */
	if(errCode != (a2b_UInt32)A2B_EC_DISCOVERY_PWR_FAULT)
	{
		/* Disable power on the B side of the node */
		wBuf[0] = A2B_REG_SWCTL;
		wBuf[1] = A2B_ENUM_SWCTL_ENSW_DIS;
		if ( plugin->discovery.dscNumNodes == 0u )
		{
			status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
		}
		else
		{
			status = a2b_i2cSlaveWrite( plugin->ctx, ((a2b_Int16)plugin->discovery.dscNumNodes-(a2b_Int16)1),
										2u, wBuf );
		}
	}

    if (( A2B_FAILED(status) ) &&
        ((a2b_UInt32)A2B_EC_OK == plugin->discovery.discoveryCompleteCode))
    {
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        return;
    }

    A2B_DSCVRY_SEQGROUP1( plugin->ctx,
                          "Discovery End (err: 0x%lX)",
                          &plugin->discovery.discoveryCompleteCode );

#ifdef A2B_FEATURE_EEPROM_PROCESSING
    /* STOP ALL possible peripheral processing timers */
    for (idx = 0u; idx < A2B_ARRAY_SIZE(plugin->periph.node); idx++)
    {
        a2b_timerStop(plugin->periph.node[idx].timer);
        (void)a2b_timerUnref( plugin->periph.node[idx].timer );
        plugin->periph.node[idx].timer = A2B_NULL;

        if ( A2B_NULL != plugin->periph.node[idx].mboxHnd )
        {
            (void)a2b_stackCtxMailboxFree(plugin->ctx,
                                     plugin->periph.node[idx].mboxHnd );
        }
        plugin->periph.node[idx].mboxHnd = A2B_NULL;
    }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

    if ( (bdd_DISCOVERY_MODE_MODIFIED == eDscMode) ||
    		(bdd_DISCOVERY_MODE_OPTIMIZED == eDscMode) ||
			(bdd_DISCOVERY_MODE_ADVANCED == eDscMode))
    {
        /* Only after the peripherals have completed should we finalize
         * the master node and enable up/downstream audio transmission.
         */
        a2b_dscvryNetComplete( plugin );
    }

    wBuf[0] = A2B_REG_CONTROL;
    /* The AD242X (only) needs to be told it's a Master node BEFORE
     * the PLL locks on the SYNC pin. Once the PLL is locked, setting
     * the MSTR bit is ignored. We set it anyway so it's clear this is
     * the master node.
     */
    wBuf[1] = A2B_ENUM_CONTROL_END_DISCOVERY;
    if ( (plugin->nodeSig.hasSiliconInfo) &&
        ((a2b_isAd242xChip(plugin->nodeSig.siliconInfo.vendorId,
        plugin->nodeSig.siliconInfo.productId))) )
    {
        wBuf[1] |= (a2b_UInt8)A2B_ENUM_CONTROL_MSTR;
    }
	wBuf[1] |= (a2b_UInt8)(plugin->bdd->nodes[0].ctrlRegs.control & (A2B_ENUM_CONTROL_XCVRBINV | A2B_ENUM_CONTROL_SWBYP));
    (void)a2b_i2cMasterWrite( plugin->ctx, 2u, &wBuf );

    plugin->discovery.inDiscovery = A2B_FALSE;

    msg = a2b_msgRtrGetExecutingMsg( plugin->ctx, A2B_MSG_MAILBOX );
    if (msg)
    {
        a2b_NetDiscovery*   dscResp;
        dscResp = (a2b_NetDiscovery*)a2b_msgGetPayload( msg );

        status = A2B_RESULT_SUCCESS;
        if (plugin->discovery.discoveryCompleteCode)
        {
            /* Add the severity and facility */
            status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                      plugin->discovery.discoveryCompleteCode);
        }

        dscResp->resp.status   = status;
        dscResp->resp.numNodes = plugin->discovery.dscNumNodes;
		dscResp->resp.oLastNodeInfo = plugin->slaveNodeSig[dscResp->resp.numNodes].siliconInfo;

        A2B_SEQ_CHART2( (plugin->ctx,
                A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(plugin->nodeSig.nodeAddr),
                A2B_SEQ_CHART_ENTITY_APP,
                A2B_SEQ_CHART_COMM_REPLY,
                A2B_SEQ_CHART_LEVEL_DISCOVERY,
                "DiscoveryResp: status: 0x%08lX, numNodes: %ld",
                &dscResp->resp.status, &dscResp->resp.numNodes ));
    }

    a2b_msgRtrExecUpdate( plugin->ctx, A2B_MSG_MAILBOX, A2B_EXEC_COMPLETE );

    /*
     * Notify listeners that discovery is done
     */
    msg = a2b_msgAlloc( plugin->ctx,
                        A2B_MSG_NOTIFY,
                        A2B_MSGNOTIFY_DISCOVERY_DONE );
    if ( A2B_NULL == msg )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s a2b_dscvryEnd(): Failed to allocate DISCOVERY_DONE "
                    "notification msg", A2B_MPLUGIN_PLUGIN_NAME));
    }
    else
    {
        a2b_DiscoveryStatus* discStatus;

        discStatus = (a2b_DiscoveryStatus*)a2b_msgGetPayload(msg);
        discStatus->status = status;
        discStatus->numNodes = plugin->discovery.dscNumNodes;
        if ( A2B_FAILED(a2b_msgRtrNotify(msg)) )
        {
            A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                                "%s a2b_dscvryEnd(): Failed to emit "
                                "DISCOVERY_DONE notification",
                                A2B_MPLUGIN_PLUGIN_NAME));
        }
        /* We always unref the notification message on success or failure of
         * notification delivery
         */
        (void)a2b_msgUnref(msg);

    }

    A2B_DSCVRY_RAWDEBUG0( plugin->ctx, "dscvryEnd",
                          "== Discovery Ended ==" );

    A2B_DSCVRY_SEQEND( plugin->ctx );

} /* a2b_dscvryEnd */


/*!****************************************************************************
*
*  \b              a2b_onDiscTimeout
*
*  Handle the discovery timeout.
*
*  \param          [in]    timer
*  \param          [in]    userData
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_onDiscTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)userData;
    a2b_HResult ret;
    a2b_UInt8   dscNumNodes = plugin->discovery.dscNumNodes;
    a2b_Bool bNetConfigFlag = A2B_FALSE;

    A2B_UNUSED(timer);

    /* Check the interrupt status one more time in
     * case of a timing race condition.
     */
    ret = a2b_intrQueryIrq( plugin->ctx );

    if (( A2B_SUCCEEDED(ret) ) && ( dscNumNodes != plugin->discovery.dscNumNodes ))
    {
        /* Discovery Done, no error, node already handled */
        return;
    }

    A2B_DSCVRY_ERROR0( plugin->ctx, "onDiscTimeout", "DISCOVERY TIMEOUT" );

    bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);
    if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
    {
    	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_BUSY;
    	a2b_dscvryNetComplete(plugin);
    }
    else
    {
    a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_BUSY );
    }

} /* a2b_onDiscTimeout */


#ifdef A2B_FEATURE_EEPROM_PROCESSING
/*!****************************************************************************
*
*  \b              a2b_dscvryPeripheralProcessingComplete
*
*  Handler when peripheral processing is complete.
*
*  \param          [in]    plugin
*  \param          [in]    nodeAddr
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
void
a2b_dscvryPeripheralProcessingComplete
    (
    a2b_Plugin* plugin,
    a2b_Int16   nodeAddr
    )
{
    if (plugin)
    {
#ifdef FIND_NODE_HANDLER_AFTER_NODE_INIT
        a2b_dscvryFindNodeHandler(plugin,
                                A2B_MAP_SLAVE_ADDR_TO_INDEX(nodeAddr));
#endif
        (void)a2b_dscvryInitPlugin( plugin,
                              nodeAddr,
                              &a2b_dscvryInitPluginComplete_EepromComplete );
    }

} /* a2b_dscvryPeripheralProcessingComplete */

#endif /* A2B_FEATURE_EEPROM_PROCESSING */


/*!****************************************************************************
*
*  \b              a2b_onResetTimeout
*
*  Handle the reset timeout.
*
*  \param          [in]    timer
*  \param          [in]    userData
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_onResetTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)userData;

    A2B_UNUSED(timer);

    if (plugin->discovery.inDiscovery)
    {
        /* Initialize the Master Node */
        if (a2b_dscvryPreMasterInit(plugin))
        {
            /* Discovery has started */
            return;
        }
    }

    /* Must be an error or in the wrong state, quit the complete command */
    a2b_msgRtrExecUpdate( plugin->ctx, A2B_MSG_MAILBOX, A2B_EXEC_COMPLETE );

} /* a2b_onResetTimeout */


/*!****************************************************************************
*
*  \b              a2b_dscvryStartTimer
*
*  Generate/Start the discovery timer.
*
*  \param          [in]    plugin
*  \param          [in]    type
*
*  \pre            None
*
*  \post           None
*
*  \return         [add here]
*
******************************************************************************/
static a2b_Bool
a2b_dscvryStartTimer
    (
    a2b_Plugin*     plugin,
    a2b_dscvryTimer type
    )
{
    /* Default is for the discovery timer */
    a2b_UInt32 delay = A2B_DISCOVERY_DELAY;
    a2b_TimerFunc timerFunc = &a2b_onDiscTimeout;

    if ( TIMER_RESET == type )
    {
        /* Setup for the reset timer */
#ifdef ADI_BDD
		delay = (plugin->bdd->policy.discoveryStartDelay > 0u ? plugin->bdd->policy.discoveryStartDelay : A2B_SW_RESET_DELAY);
#endif
        timerFunc = &a2b_onResetTimeout;
    }

    /* Stop the previously running timer */
    a2b_timerStop( plugin->timer );

    /* Single shot timer */
    a2b_timerSet( plugin->timer, delay, 0u );
    a2b_timerSetHandler(plugin->timer, timerFunc);
    a2b_timerSetData(plugin->timer, plugin);
    a2b_timerStart( plugin->timer );

    return A2B_TRUE;

} /* a2b_dscvryStartTimer */


/*!****************************************************************************
*
*  \b              a2b_dscvryNodeInterruptInit
*
*  Initialize a nodes interrupt registers
*
*  \param          [in]    plugin       plugin specific data
*
*  \param          [in]    nodeBddIdx   0=master, 1=slave0, etc
*
*  \pre            None
*
*  \post           None
*
*  \return         FALSE=error
*                  TRUE=success
*
******************************************************************************/
static a2b_Bool
a2b_dscvryNodeInterruptInit
    (
    a2b_Plugin* plugin,
    a2b_Int16   nodeBddIdx
    )
{
    a2b_Int16 nodeAddr = nodeBddIdx-1;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_UInt32 nRetVal;

    if ((nodeBddIdx < 0) || (nodeBddIdx >= (a2b_Int16)plugin->bdd->nodes_count))
    {
        return A2B_FALSE;
    }

    A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                          "Interrupt Registers" );

    if (plugin->bdd->nodes[nodeBddIdx].has_intRegs)
    {
        a2b_UInt8 wBuf[4];
        a2b_UInt32 mask = 0u;

        if (plugin->bdd->nodes[nodeBddIdx].intRegs.has_intmsk0)
        {
            mask |= (plugin->bdd->nodes[nodeBddIdx].intRegs.intmsk0 <<
                     A2B_INTRMASK0_OFFSET);
        }
        if (plugin->bdd->nodes[nodeBddIdx].intRegs.has_intmsk1)
        {
            nRetVal = a2b_ovrApplyIntrActive(plugin, nodeBddIdx,
                    A2B_REG_INTMSK1) << A2B_INTRMASK1_OFFSET;
            mask |= nRetVal;
        }
        if (plugin->bdd->nodes[nodeBddIdx].intRegs.has_intmsk2)
        {
            nRetVal = a2b_ovrApplyIntrActive(plugin, nodeBddIdx,
                    A2B_REG_INTMSK2) << A2B_INTRMASK2_OFFSET;
            mask |= nRetVal;
        }

        /* The last node in the network should *not* have the power fault
         * interrupts enabled since it would always trigger. Since there is
         * no connection on the "B" side of the transceiver it will always
         * report an open-circuit condition.
         */
        if (nodeBddIdx + 1 >= (a2b_Int16)plugin->bdd->nodes_count)
        {
            mask &= (a2b_UInt32)(~((a2b_UInt32)A2B_BITM_INTPND0_PWRERR << (a2b_UInt32)A2B_INTRMASK0_OFFSET));
        }

        A2B_TRACE2( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE3),
                    "%s NodeInterruptInit(): setIntrMask(0x%lX)",
                    A2B_MPLUGIN_PLUGIN_NAME, &mask) );

        (void)a2b_intrSetMask( plugin->ctx, nodeAddr, mask );

        if (plugin->bdd->nodes[nodeBddIdx].intRegs.has_becctl)
        {
            wBuf[0] = A2B_REG_BECCTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeBddIdx].intRegs.becctl;

            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }

            if ( A2B_FAILED(status) )
            {
                A2B_DSCVRY_SEQEND( plugin->ctx );
                return A2B_FALSE;
            }
        }
    }

    A2B_DSCVRY_SEQEND( plugin->ctx );

    return A2B_TRUE;

} /* a2b_dscvryNodeInterruptInit */


/*!****************************************************************************
*
*  \b              a2b_dscvryNodeComplete
*
*  Configuration of master/slave node after discovery
*
*  \param          [in]    plugin        plugin specific data
*  \param          [in]    nodeAddr      -1=master, 0=slave0, 1=slave1, etc
*  \param          [in]    bDoEepromCfg   Configure node from EEPROM
*  \param          [in]    errCode        Pointer to the Error code passed
*  										  from this function
**
*  \pre            Should call this routine only for the master node
*
*  \post           On failures the a2b_dscvryEnd() is expected to be called
*                  outside this function.
*
*  \return         status - sucess (0u)
*							failure (0xFFFFFFFFu)
******************************************************************************/
static a2b_Int32
a2b_dscvryNodeComplete
    (
    a2b_Plugin* plugin,
    a2b_Int16   nodeAddr,
    a2b_Bool    bDoEepromCfg,
    a2b_UInt32* errCode
    )
{
    a2b_UInt8 wBuf[4];
    a2b_Int16 nodeIdx = nodeAddr+1;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_UInt32 i2cCount = 0u;
    a2b_Int32 retCode;
    a2b_Bool bGroupLogged = A2B_FALSE;
    a2b_Bool isAd242x = A2B_FALSE;
    a2b_Bool isAd242xMaster = A2B_FALSE;
    a2b_Bool bRetVal;
    bdd_DiscoveryMode eDiscMode;

    eDiscMode = a2b_ovrGetDiscMode(plugin);
    A2B_DSCVRY_SEQGROUP1( plugin->ctx,
                          "NodeComplete for nodeAddr %hd", &nodeAddr );

    *errCode = (a2b_UInt32)A2B_EC_OK;

#ifndef A2B_FEATURE_EEPROM_PROCESSING
    A2B_UNUSED( bDoEepromCfg );
#endif

    if ((nodeAddr < A2B_NODEADDR_MASTER) ||
        (nodeIdx >= (a2b_Int16)plugin->bdd->nodes_count))
    {
        A2B_DSCVRY_SEQEND( plugin->ctx );
        *errCode = (a2b_UInt32)A2B_EC_INVALID_PARAMETER;
        return A2B_EXEC_COMPLETE_FAIL;
    }

    if ( plugin->nodeSig.hasSiliconInfo )
    {
        isAd242xMaster = a2b_isAd242xChip(
                                        plugin->nodeSig.siliconInfo.vendorId,
                                        plugin->nodeSig.siliconInfo.productId);
    }

    if ( A2B_NODEADDR_MASTER == nodeAddr )
    {
        isAd242x = isAd242xMaster;
    }
    else if ( plugin->slaveNodeSig[nodeAddr].hasSiliconInfo )
    {
        isAd242x = a2b_isAd242xChip(
                plugin->slaveNodeSig[nodeAddr].siliconInfo.vendorId,
                plugin->slaveNodeSig[nodeAddr].siliconInfo.productId);
    }
    else
    {
        /* Completing the control statement */
    }

    /*---------------------------------------------------*/
    /* Some simple setup prior to further initialization */
    /*---------------------------------------------------*/

    /* NOTE: A2B_REG_NODEADR managed by I2C, no need to set it */

    if ( (bdd_DISCOVERY_MODE_SIMPLE == eDiscMode) &&
        (A2B_NODEADDR_MASTER == nodeAddr) )
    {
        /*if ( A2B_SUCCEEDED(status) )*/
        {
            wBuf[0] = A2B_REG_SWCTL;
            wBuf[1] = A2B_BITM_SWCTL_ENSW;
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            i2cCount++;
        }
    }

    /*-------------------*/
    /* Control Registers */
    /*-------------------*/

    A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                          "Control Registers" );

    if (( A2B_SUCCEEDED(status) ) &&
        (plugin->bdd->nodes[nodeIdx].ctrlRegs.has_bcdnslots))
    {
        wBuf[0] = A2B_REG_BCDNSLOTS;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.bcdnslots;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if (( A2B_SUCCEEDED(status) ) &&
        (plugin->bdd->nodes[nodeIdx].ctrlRegs.has_ldnslots))
    {
        wBuf[0] = A2B_REG_LDNSLOTS;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.ldnslots;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if (( A2B_SUCCEEDED(status) ) &&
        (plugin->bdd->nodes[nodeIdx].ctrlRegs.has_lupslots))
    {
        wBuf[0] = A2B_REG_LUPSLOTS;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.lupslots;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if(((a2b_UInt32)nodeIdx < (a2b_UInt32)(plugin->bdd->nodes_count - (a2b_UInt32)1u)) &&
    		(bdd_DISCOVERY_MODE_ADVANCED != eDiscMode))
    {
#ifdef ADI_BDD
		if ( A2B_SUCCEEDED(status) &&
			(plugin->bdd->nodes[nodeIdx].ctrlRegs.has_dnslots))
#else
		if ( A2B_SUCCEEDED(status) )
#endif
		{
			wBuf[0] = A2B_REG_DNSLOTS;
			wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.dnslots;
			if (A2B_NODEADDR_MASTER == nodeAddr)
			{
				status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			}
			else
			{
				status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
			}
			i2cCount++;
		}

#ifdef ADI_BDD
		if ( A2B_SUCCEEDED(status) &&
				(plugin->bdd->nodes[nodeIdx].ctrlRegs.has_upslots))
#else
		if ( A2B_SUCCEEDED(status) )
#endif
		{
			wBuf[0] = A2B_REG_UPSLOTS;
			wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.upslots;
			if (A2B_NODEADDR_MASTER == nodeAddr)
			{
				status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			}
			else
			{
				status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
			}
			i2cCount++;
		}
    }

    A2B_DSCVRY_SEQEND( plugin->ctx );

    /*-------------------*/
    /* I2C/I2S Registers */
    /*-------------------*/

    bGroupLogged = A2B_FALSE;
#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2ccfg))
#else
	if ( A2B_SUCCEEDED(status) )
#endif
    {
        bGroupLogged = A2B_TRUE;
        A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "I2C/I2S Registers" );

        wBuf[0] = A2B_REG_I2CCFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2ccfg;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

	if ((A2B_SUCCEEDED(status)) &&
		(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_syncoffset))
	{
		wBuf[0] = A2B_REG_SYNCOFFSET;
		wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.syncoffset;
		if (A2B_NODEADDR_MASTER == nodeAddr)
		{
			status = a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
		}
		else
		{
			status = a2b_i2cSlaveWrite(plugin->ctx, nodeAddr, 2u, wBuf);
		}
		i2cCount++;
	}

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2sgcfg))
#else
	if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_I2SGCFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2sgcfg;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2scfg))
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_I2SCFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2scfg;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if (( A2B_SUCCEEDED(status) ) &&
        (plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2srate))
    {
        wBuf[0] = A2B_REG_I2SRATE;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2srate;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if (( A2B_SUCCEEDED(status) ) &&
        (plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2stxoffset))
    {
        wBuf[0] = A2B_REG_I2STXOFFSET;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2stxoffset;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if (( A2B_SUCCEEDED(status) ) &&
        (plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2srxoffset))
    {
        wBuf[0] = A2B_REG_I2SRXOFFSET;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2srxoffset;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_pdmctl))
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_PDMCTL;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.pdmctl;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }
	/* Chiron  - Start */
#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
			(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_pdmctl2))
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_PDMCTL2;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.pdmctl2;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

	/* chiron - End */
#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_errmgmt))
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_ERRMGMT;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.errmgmt;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if ( bGroupLogged )
    {
        A2B_DSCVRY_SEQEND(plugin->ctx);
    }


    /*-------------------*/
    /* Pin I/O Registers */
    /*-------------------*/

    bGroupLogged = A2B_FALSE;
    if ( A2B_SUCCEEDED(status) )
    {
        bGroupLogged = A2B_TRUE;
        A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "Pin I/O Registers" );

        /* Only the AD24XX (e.g. older AD2410 chips) have a CLKCFG register */
        if ( (plugin->bdd->nodes[nodeIdx].pinIoRegs.has_clkcfg) && (!isAd242x) )
        {
            wBuf[0] = A2B_REG_CLKCFG;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].pinIoRegs.clkcfg;

            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
            	status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
            	status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].pinIoRegs.has_gpiodat) )
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_GPIODAT;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].pinIoRegs.gpiodat;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].pinIoRegs.has_gpiooen))
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_GPIOOEN;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].pinIoRegs.gpiooen;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].pinIoRegs.has_gpioien))
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_GPIOIEN;
        wBuf[1] = (a2b_UInt8)a2b_ovrApplyIntrActive(plugin, nodeIdx, A2B_REG_GPIOIEN);
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].pinIoRegs.has_pinten) )
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_PINTEN;
        wBuf[1] = (a2b_UInt8)a2b_ovrApplyIntrActive(plugin, nodeIdx, A2B_REG_PINTEN);
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].pinIoRegs.has_pintinv) )
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_PINTINV;
        wBuf[1] = (a2b_UInt8)a2b_ovrApplyIntrActive(plugin, nodeIdx, A2B_REG_PINTINV);
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

#ifdef ADI_BDD
    if ( A2B_SUCCEEDED(status) &&
    		(plugin->bdd->nodes[nodeIdx].pinIoRegs.has_pincfg) )
#else
    if ( A2B_SUCCEEDED(status) )
#endif
    {
        wBuf[0] = A2B_REG_PINCFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].pinIoRegs.pincfg;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }


    /*
     * Additional AD242X registers
     */

    /*--------------------------------------*/
    /* Clock Config Registers - AD242X only */
    /*--------------------------------------*/

    if ( (A2B_SUCCEEDED(status)) && (isAd242x) &&
        (plugin->bdd->nodes[nodeIdx].pinIoRegs.has_clk1cfg) )
    {
        wBuf[0] = A2B_REG_CLK1CFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].pinIoRegs.clk1cfg;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if ( (A2B_SUCCEEDED(status)) && (isAd242x) &&
        (plugin->bdd->nodes[nodeIdx].pinIoRegs.has_clk2cfg) )
    {
        wBuf[0] = A2B_REG_CLK2CFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].pinIoRegs.clk2cfg;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }

    if ( bGroupLogged )
    {
        A2B_DSCVRY_SEQEND(plugin->ctx);
    }

    /*------------------------------------------------------*/
    /* Slot Enhancement Registers (AD242x only, slave only) */
    /*------------------------------------------------------*/

    if ( (A2B_SUCCEEDED(status)) &&
        (isAd242x) &&
        (A2B_NODEADDR_MASTER != nodeAddr) &&
        (plugin->bdd->nodes[nodeIdx].has_slotEnh) )
    {
        A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "Slot Enhancement Registers" );

        if ( /*(A2B_SUCCEEDED(status)) &&*/
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_upmask0) )
        {
            wBuf[0] = A2B_REG_UPMASK0;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.upmask0;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_upmask1) )
        {
            wBuf[0] = A2B_REG_UPMASK1;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.upmask1;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_upmask2) )
        {
            wBuf[0] = A2B_REG_UPMASK2;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.upmask2;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_upmask3) )
        {
            wBuf[0] = A2B_REG_UPMASK3;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.upmask3;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_upoffset) )
        {
            wBuf[0] = A2B_REG_UPOFFSET;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.upoffset;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_dnmask0) )
        {
            wBuf[0] = A2B_REG_DNMASK0;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.dnmask0;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_dnmask1) )
        {
            wBuf[0] = A2B_REG_DNMASK1;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.dnmask1;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_dnmask2) )
        {
            wBuf[0] = A2B_REG_DNMASK2;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.dnmask2;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_dnmask3) )
        {
            wBuf[0] = A2B_REG_DNMASK3;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.dnmask3;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].slotEnh.has_dnoffset) )
        {
            wBuf[0] = A2B_REG_DNOFFSET;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].slotEnh.dnoffset;
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            i2cCount++;
        }

        A2B_DSCVRY_SEQEND(plugin->ctx);
    }


    /*--------------------------------------------*/
    /* GPIO over Distance Registers (AD242x only) */
    /*--------------------------------------------*/
    if ( (A2B_SUCCEEDED(status)) &&
        (isAd242x) &&
        (plugin->bdd->nodes[nodeIdx].has_gpioDist) )
    {
        A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "GPIO Over Distance Registers" );

        if ( /*(A2B_SUCCEEDED(status)) &&*/
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod0msk) )
        {
            wBuf[0] = A2B_REG_GPIOD0MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod0msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod1msk) )
        {
            wBuf[0] = A2B_REG_GPIOD1MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod1msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod2msk) )
        {
            wBuf[0] = A2B_REG_GPIOD2MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod2msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod3msk) )
        {
            wBuf[0] = A2B_REG_GPIOD3MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod3msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod4msk) )
        {
            wBuf[0] = A2B_REG_GPIOD4MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod4msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod5msk) )
        {
            wBuf[0] = A2B_REG_GPIOD5MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod5msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod6msk) )
        {
            wBuf[0] = A2B_REG_GPIOD6MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod6msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiod7msk) )
        {
            wBuf[0] = A2B_REG_GPIOD7MSK;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiod7msk;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }
        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpioddat) )
        {
            wBuf[0] = A2B_REG_GPIODDAT;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpioddat;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpiodinv) )
        {
            wBuf[0] = A2B_REG_GPIODINV;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpiodinv;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if ( (A2B_SUCCEEDED(status)) &&
            (plugin->bdd->nodes[nodeIdx].gpioDist.has_gpioden) )
        {
            wBuf[0] = A2B_REG_GPIODEN;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].gpioDist.gpioden;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        A2B_DSCVRY_SEQEND(plugin->ctx);
    }

#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
    if (plugin->bdd->nodes[nodeIdx].nodeDescr.oCustomNodeIdSettings.bReadFrmCommCh == A2B_FALSE)
    {
#endif
#endif	/* A2B_FEATURE_COMM_CH */
		/*---------------------------------------------*/
		/* Mailbox Registers (AD242x only, slave only) */
		/*---------------------------------------------*/

		if ( (A2B_SUCCEEDED(status)) &&
			(isAd242x) &&
			(plugin->bdd->nodes[nodeIdx].has_mbox) &&
			(A2B_NODEADDR_MASTER != nodeAddr) )
		{
			A2B_DSCVRY_SEQGROUP0( plugin->ctx, "Mailbox Registers" );

			if (/* (A2B_SUCCEEDED(status)) && */
				(plugin->bdd->nodes[nodeIdx].mbox.has_mbox0ctl) )
			{
				wBuf[0] = A2B_REG_MBOX0CTL;
				wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].mbox.mbox0ctl;
				status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
				i2cCount++;
			}

			if ( (A2B_SUCCEEDED(status)) &&
				(plugin->bdd->nodes[nodeIdx].mbox.has_mbox1ctl) )
			{
				wBuf[0] = A2B_REG_MBOX1CTL;
				wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].mbox.mbox1ctl;
				status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
				i2cCount++;
			}

			A2B_DSCVRY_SEQEND(plugin->ctx);
		}
#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
	}
#endif
#endif	/* A2B_FEATURE_COMM_CH */

	A2B_DSCVRY_SEQGROUP0( plugin->ctx, "242x I2S Registers" );
    /* A2B_REG_SUSCFG - AD242X only for slave nodes */
    if ( (A2B_SUCCEEDED(status)) &&
        (A2B_NODEADDR_MASTER != nodeAddr) &&
        (isAd242x) &&
        (plugin->bdd->nodes[nodeIdx].ctrlRegs.has_suscfg ))
    {
        wBuf[0] = A2B_REG_SUSCFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.suscfg;
        status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        i2cCount++;
    }

    /* A2B_REG_I2SRRSOFFS - AD242X only, slave only */
    if ( (A2B_SUCCEEDED(status)) &&
        ((A2B_NODEADDR_MASTER != nodeAddr)) &&
        (isAd242x) &&
        (plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2srrsoffs ))
    {
        wBuf[0] = A2B_REG_I2SRRSOFFS;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2srrsoffs;
        status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        i2cCount++;
    }

    /* A2B_REG_I2SRRCTL - AD242X only */
    if ( (A2B_SUCCEEDED(status)) && (isAd242x) &&
        (plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2srrctl) )
    {
        wBuf[0] = A2B_REG_I2SRRCTL;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2srrctl;
        if (A2B_NODEADDR_MASTER == nodeAddr)
        {
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        }
        else
        {
            status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
        }
        i2cCount++;
    }
    A2B_DSCVRY_SEQEND(plugin->ctx);

    /*------------------*/
    /* Tuning Registers */
    /*------------------*/

    if (plugin->bdd->nodes[nodeIdx].has_tuningRegs)
    {
        A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "Tuning Registers" );

        if (( A2B_SUCCEEDED(status) ) &&
            (plugin->bdd->nodes[nodeIdx].tuningRegs.has_vregctl))
        {
            wBuf[0] = A2B_REG_VREGCTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].tuningRegs.vregctl;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if (( A2B_SUCCEEDED(status) ) &&
            (plugin->bdd->nodes[nodeIdx].tuningRegs.has_txactl))
        {
            wBuf[0] = A2B_REG_TXACTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].tuningRegs.txactl;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if (( A2B_SUCCEEDED(status) ) &&
            (plugin->bdd->nodes[nodeIdx].tuningRegs.has_rxactl))
        {
            wBuf[0] = A2B_REG_RXACTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].tuningRegs.rxactl;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if (( A2B_SUCCEEDED(status) ) &&
            (plugin->bdd->nodes[nodeIdx].tuningRegs.has_txbctl))
        {
            wBuf[0] = A2B_REG_TXBCTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].tuningRegs.txbctl;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }

        if (( A2B_SUCCEEDED(status) ) &&
            (plugin->bdd->nodes[nodeIdx].tuningRegs.has_rxbctl))
        {
            wBuf[0] = A2B_REG_RXBCTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].tuningRegs.rxbctl;
            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }
            i2cCount++;
        }
        A2B_DSCVRY_SEQEND(plugin->ctx);
    }

    /*---------------------*/
    /* Interrupt Registers */
    /*---------------------*/

    bRetVal = a2b_dscvryNodeInterruptInit(plugin, nodeIdx);
    if (( A2B_SUCCEEDED(status) ) && (!bRetVal))
    {
        A2B_DSCVRY_ERROR0( plugin->ctx, "nodeComplete",
                           "Failed to set node interrupts" );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        *errCode = (a2b_UInt32)A2B_EC_IO;
        return A2B_EXEC_COMPLETE_FAIL;
    }

    /*---------------------*/
    /* Final Master Setup  */
    /*---------------------*/

    if ( A2B_SUCCEEDED(status) )
    {
        if ((A2B_NODEADDR_MASTER != nodeAddr) &&
        		(bdd_DISCOVERY_MODE_SIMPLE == eDiscMode))
        {
		    /* Don't enable switch to last slave */
        	if(nodeAddr != (plugin->bdd->nodes_count - 2))
        	{
				wBuf[0] = A2B_REG_SWCTL;
				wBuf[1] = A2B_BITM_SWCTL_ENSW;
				status  = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
				i2cCount++;
        	}
        }
    }

    /*-----------------------*/
    /* Peripheral Processing */
    /*-----------------------*/
    retCode = A2B_EXEC_COMPLETE;
#ifdef A2B_FEATURE_EEPROM_PROCESSING
    if (( A2B_SUCCEEDED(status) ) && (bDoEepromCfg) &&
        ( A2B_HAS_EEPROM(plugin, nodeAddr) ) )
    {
        retCode = a2b_periphCfgInitProcessing( plugin, nodeAddr );
        if ( A2B_EXEC_COMPLETE_FAIL == retCode )
        {
            /* On an error we need to clear the hasEeprom tracking */
            plugin->discovery.hasEeprom = 0u;

            A2B_DSCVRY_ERROR0( plugin->ctx, "nodeComplete",
                               "EEPROM Processing Failed" );
            A2B_DSCVRY_SEQEND( plugin->ctx );
            *errCode = (a2b_UInt32)A2B_EC_IO;
            return A2B_EXEC_COMPLETE_FAIL;
        }
    }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */


    /*-------------------*/
    /* Status Processing */
    /*-------------------*/

    if ( A2B_FAILED(status) )
    {
        A2B_DSCVRY_ERROR1( plugin->ctx, "nodeComplete",
                           "I2C failure at operation: %ld", &i2cCount );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        *errCode = (a2b_UInt32)A2B_EC_INTERNAL;
        return A2B_EXEC_COMPLETE_FAIL;
    }

    A2B_DSCVRY_SEQEND( plugin->ctx );

    return retCode;

} /* a2b_dscvryNodeComplete */

/*!****************************************************************************
*
*  \b              a2b_FinalMasterSetup
*
*  Final setup of master node after discovery. This is called at the last for
*  all modes of discovery except Advanced Mode. In advanced mode, this function
*  is repeatedly called after reconfiguration of slots at each node.
*
*  \param          [in]    plugin           plugin specific data
*
*  \param          [in]    nodeAddr          NodeAddress -1=master
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_TRUE == NetConfiguration is to be done
*                  A2B_FALSE == NetConfiguration is not to be done
*
******************************************************************************/
static a2b_HResult
a2b_FinalMasterSetup(a2b_Plugin* plugin,
		a2b_Int16   nodeAddr)
{
	a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_Bool isAd242xMaster = A2B_FALSE;
    a2b_UInt32 i2cCount = 0u;
    a2b_UInt8 wBuf[4u];
    a2b_Int16 nodeIdx = nodeAddr+1;


    if ((A2B_NODEADDR_MASTER == nodeAddr) && (plugin != A2B_NULL))
    {
		wBuf[0] = A2B_REG_NODEADR;
		wBuf[1] = 0x0u;
		status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );

        if ( plugin->nodeSig.hasSiliconInfo )
        {
            isAd242xMaster = a2b_isAd242xChip(
                                            plugin->nodeSig.siliconInfo.vendorId,
                                            plugin->nodeSig.siliconInfo.productId);
        }

        A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "Final Master Setup" );

		/* Configuration of Spread spectrum - AD2428 onwards */
		(void)a2b_ConfigSpreadSpectrum(plugin, nodeAddr);

		wBuf[0] = A2B_REG_NODEADR;
		wBuf[1] = (a2b_UInt8)0;
		status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
		i2cCount++;

		if (plugin->bdd->nodes[nodeIdx].ctrlRegs.has_slotfmt)
		{
			wBuf[0] = A2B_REG_SLOTFMT;
			wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.slotfmt;
			status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			i2cCount++;
		}

		if ( A2B_SUCCEEDED(status) )
		{
			/* Enable the up and downstream slots */
			wBuf[0] = A2B_REG_DATCTL;

			if (plugin->bdd->nodes[nodeIdx].ctrlRegs.has_datctl)
			{
				wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].ctrlRegs.datctl;
			}
			else
			{
				wBuf[1] = A2B_BITM_DATCTL_UPS | A2B_BITM_DATCTL_DNS;
			}
			status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			i2cCount++;
		}

		/* A2B_REG_I2SRRATE - AD242X only, Master node only */
		if ( (A2B_SUCCEEDED(status)) &&
			(isAd242xMaster) &&
			(plugin->bdd->nodes[nodeIdx].i2cI2sRegs.has_i2srrate ))
		{
			wBuf[0] = A2B_REG_I2SRRATE;
			wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.i2srrate;
			status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			i2cCount++;
		}

		if ( A2B_SUCCEEDED(status) )
		{
			/* Push all shadowed/cached registers */
			wBuf[0] = A2B_REG_CONTROL;
			/* The AD242X (only) needs to be told it's a Master node
			 * BEFORE the PLL locks on the SYNC pin. Once the PLL is
			 * locked, setting the MSTR bit is ignored. We set it
			 * anyway so it's clear this is the master node.
			 */
			wBuf[1] = A2B_ENUM_CONTROL_START_NS;
			if ( isAd242xMaster )
			{
				wBuf[1] |= (a2b_UInt8)A2B_ENUM_CONTROL_MSTR;
			}
			/* Adding additional bit fields with AD2428 onwards */
			wBuf[1] |= (a2b_UInt8)(plugin->bdd->nodes[nodeIdx].ctrlRegs.control & (A2B_ENUM_CONTROL_XCVRBINV | A2B_ENUM_CONTROL_SWBYP));

			status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			i2cCount++;
		}
		A2B_DSCVRY_SEQEND( plugin->ctx );
    }
    return status;

} /* a2b_FinalMasterSetup */


/*!****************************************************************************
*
*  \b              a2b_ConfigSpreadSpectrum
*
*  Final setup of Spread Spectrum Configuration.
*
*  \param          [in]    plugin           plugin specific data
*
*  \param          [in]    nodeAddr          NodeAddress -1=master
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_TRUE == NetConfiguration is to be done
*                  A2B_FALSE == NetConfiguration is not to be done
*
******************************************************************************/
static a2b_HResult
a2b_ConfigSpreadSpectrum(a2b_Plugin* plugin,
		a2b_Int16   nodeAddr)
{
	a2b_HResult status = A2B_RESULT_SUCCESS;

#ifdef ADI_BDD
    a2b_Bool isAd2428Master = A2B_FALSE;
    a2b_UInt8 wBuf[4u], nIndex;
    a2b_Int16 nodeIdx = nodeAddr+1;
	a2b_Int16 slvNodeAddr = 0;
	a2b_Int16 slvNodeIdx = 0;
	a2b_Bool commnSpreadSpectrum = A2B_FALSE;

    if ((A2B_NODEADDR_MASTER == nodeAddr) && (plugin != A2B_NULL))
    {
		commnSpreadSpectrum = (a2b_Bool)plugin->bdd->policy.has_common_SSSettings;

		wBuf[0] = A2B_REG_NODEADR;
		wBuf[1] = 0x0u;
		status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
        if ( plugin->nodeSig.hasSiliconInfo )
        {
            isAd2428Master = A2B_IS_AD2428X_CHIP(
                                            plugin->nodeSig.siliconInfo.vendorId,
                                            plugin->nodeSig.siliconInfo.productId);
        }
		if(isAd2428Master)
		{
			wBuf[0] = A2B_REG_PLLCTL;
			wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeIdx].i2cI2sRegs.pllctl;
			/* Check whether all nodes should have common spread spectrum settings */
			if(commnSpreadSpectrum)
			{
				status = a2b_i2cSlaveBroadcastWrite(plugin->ctx, 2u, wBuf);
			}
			else
			{
				status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
			}
		}
		if((plugin->discovery.dscNumNodes) && (commnSpreadSpectrum == A2B_FALSE))
		{
			/* If every node has separate settings */
			for(nIndex = 0u; nIndex < plugin->discovery.dscNumNodes; nIndex++)
			{
				slvNodeAddr = (a2b_Int16)nIndex;
				slvNodeIdx = slvNodeAddr + 1;
				if(plugin->bdd->nodes[slvNodeIdx].i2cI2sRegs.has_pllctl)
				{
					wBuf[0u] = A2B_REG_PLLCTL;
					wBuf[1u] = (a2b_UInt8)plugin->bdd->nodes[slvNodeIdx].i2cI2sRegs.pllctl;
					status = a2b_i2cSlaveWrite(plugin->ctx, slvNodeAddr , 2u, wBuf );
				}

			}
		}
    }
#endif

    return status;

} /* a2b_ConfigSpreadSpectrum */


/*!****************************************************************************
*
*  \b              a2b_dscvryNetComplete
*
*  Final cleanup steps of network discovery
*  (i.e. when all nodes have been found.)
*
*  In the case of Simplified Discovery this method can be called more than once.
*
*  \param          [in]    plugin   plugin specific data
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryNetComplete
    (
    a2b_Plugin* plugin
    )
{
    a2b_UInt8 wBuf[4];
    a2b_Int16 nodeAddr;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_Bool initMaster = A2B_TRUE;
    a2b_UInt32 errCode = (a2b_UInt32)A2B_EC_OK;
    a2b_Int32 retCode;
    a2b_Bool bDoEepromCfg = A2B_TRUE;

#ifdef A2B_FEATURE_SEQ_CHART
    a2b_Bool bSeqGroupShown = A2B_FALSE;
#endif

#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
    a2b_Int16 nTempVar;
#if !defined(A2B_FEATURE_SEQ_CHART)
    A2B_UNUSED(nTempVar);
#endif
#endif

    if ( !plugin->discovery.inDiscovery )
    {
        return;
    }

    if ( bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin) )
    {
        a2b_Bool bContLoop;

#ifndef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
        /* When disabled using Simple discovery the node init will be
         * completed for all nodes, then all peripheral config is queued
         * and will execute cooperatively.  Whereas when enabled the node
         * init is completed followed by the nodes peripheral processing
         * followed by the next nodes int, and so on.
         */
        bDoEepromCfg = A2B_FALSE;
#endif /* A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY */

        /* Start from the latest to the first (per spec) */
        for ( nodeAddr = ((a2b_Int16)plugin->discovery.simpleNodeCount-(a2b_Int16)1);
              nodeAddr > A2B_NODEADDR_MASTER;
              nodeAddr-- )
        {
#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
            nTempVar = nodeAddr;
#endif
            bContLoop = A2B_FALSE;

            A2B_DSCVRY_SEQGROUP0_COND( plugin->ctx, bSeqGroupShown,
                                       "NetComplete" );

            retCode = a2b_dscvryNodeComplete( plugin, nodeAddr,
                                              bDoEepromCfg, &errCode );
            plugin->discovery.simpleNodeCount--;

            if ( A2B_EXEC_COMPLETE_FAIL == retCode )
            {
                A2B_DSCVRY_ERROR1( plugin->ctx, "NetComplete",
                                   "Failed to complete node %hd init",
                                   &nTempVar );
                a2b_dscvryEnd( plugin, errCode );
            }
            else if ( A2B_EXEC_COMPLETE == retCode )
            {
                /* a2b_dscvryInitPlugin() will also check for an
                 * available plugin, however, if no plugin is
                 * available for a plugin it will call the callback
                 * right away.  So to avoid a situation where few
                 * or no plugins are in our setup we could possibly
                 * blow the stack on some systems.  So to avoid
                 * growing the stack unnecessarily we check here
                 * for a plugin and if not available we will loop
                 * to the next plugin.
                 */
#ifdef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY

#ifdef FIND_NODE_HANDLER_AFTER_NODE_INIT
                a2b_dscvryFindNodeHandler(plugin,
                            A2B_MAP_SLAVE_ADDR_TO_INDEX(nodeAddr));
#endif
                if ( A2B_HAS_PLUGIN(plugin, nodeAddr) )
                {
                    errCode = a2b_dscvryInitPlugin( plugin, nodeAddr,
                                  &a2b_dscvryInitPluginComplete_EepromComplete );
                    if ((a2b_UInt32)A2B_EC_OK != errCode )
                    {
                        a2b_dscvryEnd( plugin, errCode );
                    }
                    /* else, waiting for plugin message to process */
                }
                else
#endif /* A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY */
                {
                    /* Process the next plugin using this routine */
                    bContLoop = A2B_TRUE;
                }
            }
            else
            {
                /* Completing the control statement */
            }
            /* else, waiting for peripheral node processing to complete */

            if ( !bContLoop )
            {
                A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
                return;
            }
        }

#ifndef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
        for ( nodeAddr = plugin->discovery.dscNumNodes-1;
              nodeAddr > A2B_NODEADDR_MASTER;
              nodeAddr-- )
        {
            retCode = A2B_EXEC_COMPLETE;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
            if ( A2B_HAS_EEPROM(plugin, nodeAddr) )
            {
                A2B_DSCVRY_SEQGROUP0_COND( plugin->ctx, bSeqGroupShown,
                                           "NetComplete" );

                retCode = a2b_periphCfgInitProcessing( plugin, nodeAddr );
                if ( A2B_EXEC_COMPLETE_FAIL == retCode )
                {
                    /* On an error we need to clear the hasEeprom tracking */
                    plugin->discovery.hasEeprom = 0;

                    A2B_DSCVRY_ERROR0( plugin->ctx, "nodeComplete",
                                       "EEPROM Processing Failed" );
                    a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_IO );
                    A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
                    return;
                }
            }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

            if ( A2B_EXEC_COMPLETE == retCode )
            {
#ifdef FIND_NODE_HANDLER_AFTER_NODE_INIT
                a2b_dscvryFindNodeHandler(plugin,
                                        A2B_MAP_SLAVE_ADDR_TO_INDEX(nodeAddr));
#endif
                if ( A2B_HAS_PLUGIN(plugin, nodeAddr) &&
                     A2B_NEEDS_PLUGIN_INIT(plugin, nodeAddr) )
                {
                    /* This ensures we will only queue the message to
                     * the peripheral mailbox for this node.
                     */
                    errCode = a2b_dscvryInitPlugin( plugin, nodeAddr,
                              a2b_dscvryInitPluginComplete_EepromComplete );
                    if ( A2B_EC_OK != errCode )
                    {
                        a2b_dscvryEnd( plugin, errCode );
                        A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
                        return;
                    }
                    /* else, waiting for plugin message to process */
                }
            }
#ifdef A2B_FEATURE_EEPROM_PROCESSING
            else if (( A2B_EXEC_SUSPEND == retCode ) &&
                     (a2b_periphCfgUsingSync()) )
            {
                /* MUST be waiting for a delay to complete
                 * while synchronous cfg blocks processing
                 * is expected.
                 */
                A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
                return;
            }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */
        }

        if (( plugin->discovery.hasEeprom ) ||
            (plugin->discovery.pendingPluginInit ) )
        {
            initMaster = A2B_FALSE;
        }

#endif /* A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY */

    }
    else /* if ( bdd_DISCOVERY_MODE_MODIFIED == a2b_ovrGetDiscMode(plugin) )*/
    {
        A2B_DSCVRY_SEQGROUP0_COND( plugin->ctx, bSeqGroupShown,
                                   "NetComplete" );

        /* If the nodeAddr we were trying to discover was Slave0
         * then we skip this final step on the node.
         */
        if (plugin->discovery.dscNumNodes != 0u)
        {
            A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                                  "Adjust Node Power" );


            /* We need to adjust the power setting on the discovered nodes */
            for ( nodeAddr = ((a2b_Int16)plugin->discovery.dscNumNodes-(a2b_Int16)2);
                  nodeAddr > A2B_NODEADDR_MASTER;
                  nodeAddr-- )
            {
#ifdef A2B_FEATURE_TRACE
                nTempVar = nodeAddr;
#endif
                wBuf[0] = A2B_REG_SWCTL;
                wBuf[1] = A2B_ENUM_SWCTL_ENSW_EN;
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
                if ( A2B_FAILED(status) )
                {
                    A2B_DSCVRY_ERROR1( plugin->ctx, "NetComplete",
                                       "Cannot adjust power on nodeAddr: %hd",
                                       &nTempVar );
                    initMaster = A2B_FALSE;
                    break;
                }
            }

			wBuf[0] = A2B_REG_SWCTL;
            wBuf[1] = A2B_ENUM_SWCTL_ENSW_EN;
            status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            if ( A2B_FAILED(status) )
            {
                A2B_DSCVRY_ERROR1( plugin->ctx, "NetComplete",
                                    "Cannot adjust power on nodeAddr: %hd",
                                    &nTempVar );
                initMaster = A2B_FALSE;
            }

            A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
        }
    }

    if ( bdd_DISCOVERY_MODE_ADVANCED == a2b_ovrGetDiscMode(plugin) )
    {
    	initMaster = A2B_FALSE;
    }

    if ( initMaster )
    {
        A2B_DSCVRY_SEQGROUP0_COND( plugin->ctx, bSeqGroupShown,
                                   "NetComplete" );

        a2b_dscvryInitTdmSettings( plugin, A2B_NODEADDR_MASTER );
        status = a2b_audioConfig( plugin->ctx, &plugin->pluginTdmSettings );

        if ( A2B_FAILED(status) )
        {
            A2B_DSCVRY_ERROR1( plugin->ctx, "NetComplete",
                               "Cannot config audio for nodeAddr: %hd",
                               &nodeAddr );
            a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_IO );
            A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
            return;
        }

        /* Do final setup of the master node and enable the up/downstream */
        if (a2b_dscvryNodeComplete( plugin, A2B_NODEADDR_MASTER,
                                 A2B_TRUE, &errCode ) == A2B_EXEC_COMPLETE_FAIL)
        {
            A2B_DSCVRY_ERROR0( plugin->ctx, "NetComplete",
                               "Failed to complete master node init" );
        }
        else
        {
			status = a2b_FinalMasterSetup(plugin, A2B_NODEADDR_MASTER);
			if(status != A2B_RESULT_SUCCESS)
			{
				A2B_DSCVRY_ERROR0( plugin->ctx, "NetComplete",
								   "Failed to complete master node init" );
			}
        }

        if ( bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin) )
        {
            /* We only reach this is Simple discovery has completed */
        	if((plugin->discovery.discoveryCompleteCode == (a2b_UInt32)A2B_EC_BUSY) ||
        	   (plugin->discovery.discoveryCompleteCode == (a2b_UInt32)A2B_EC_PERMISSION) ||
			   (plugin->discovery.discoveryCompleteCode == (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH)
			   )
        	{
        		errCode = plugin->discovery.discoveryCompleteCode;
        	}
            a2b_dscvryEnd( plugin, errCode );
        }
    }

    A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );

} /* a2b_dscvryNetComplete */


/*!****************************************************************************
*
*  \b              a2b_dscvryPreSlaveInit
*
*  Steps taken before finding the next slave node during discovery
*
*  \param          [in]    plugin       plugin specific data
*
*  \pre            None
*
*  \post           The following registers are altered:
*                  A2B_REG_SWCTL
*                  A2B_REG_DISCVRY
*
*  \return         FALSE=error
*                  TRUE=success
*
******************************************************************************/
static a2b_Bool
a2b_dscvryPreSlaveInit
    (
    a2b_Plugin* plugin
    )
{
    a2b_UInt8 wBuf[4u];
	a2b_HResult status;
	a2b_UInt32 value;
	struct a2b_StackContext* ctx = plugin->ctx;

	/* Discovered node information */
	a2b_Int16 dscNodeBddIdx = (a2b_Int16)plugin->discovery.dscNumNodes;
	a2b_Int16 dscNodeAddr = dscNodeBddIdx - 1;

	/* Must add one to account for the fact that the nodes_count
	 * also includes the master node.
	 */
	if (plugin->discovery.dscNumNodes + (a2b_UInt8)1 >= (a2b_UInt8)plugin->bdd->nodes_count)
	{
		A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
			"%s PreSlaveInit(): No more BDD slave nodes",
			A2B_MPLUGIN_PLUGIN_NAME));

		if (bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin))
		{
			/* For simple discovery we need to now complete the network
			 * and ensure the peripherals are complete before we call
			 * a2b_dscvryEnd.  If we have peripherals to work on an early
			 * call to a2b_dscvryEnd it will just exit.
			 */
			a2b_dscvryNetComplete(plugin);
		}
		else if (bdd_DISCOVERY_MODE_MODIFIED == a2b_ovrGetDiscMode(plugin)) /* bdd_DISCOVERY_MODE_MODIFIED */
		{
			/* We have completed the modified discovery.  We will call the
			 * a2b_dscvryEnd and it will check if the peripherals have
			 * finished processing
			 */
			a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_OK);
		}
		else
		{
			/* completing control statement */
		}

		return A2B_FALSE;
	}

	A2B_DSCVRY_SEQGROUP0(plugin->ctx,
		"PreSlaveInit");

	if (plugin->bdd->nodes[dscNodeBddIdx].ctrlRegs.has_control)
	{
		/* Write to Control register to Set LVDS Polarity, applicable only after AD2428
		*/
		wBuf[0] = A2B_REG_CONTROL;
		wBuf[1] = (a2b_UInt8)(plugin->bdd->nodes[dscNodeBddIdx].ctrlRegs.control & (A2B_ENUM_CONTROL_XCVRBINV | A2B_ENUM_CONTROL_SWBYP));
		status = a2b_i2cSlaveWrite(ctx, dscNodeAddr, 2u, wBuf);

		if (A2B_FAILED(status))
		{
			A2B_DSCVRY_ERROR1(ctx, "PreSlaveInit",
				"Cannot write to CONTROL Reg nodeAddr: %hd",
				&dscNodeAddr);
			a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_INTERNAL);
			A2B_DSCVRY_SEQEND(plugin->ctx);
			return A2B_FALSE;
		}
    }

    /* We need to enable phantom power to the next node,
     * to do so we write to the just discovered node.
     */
    wBuf[0] = A2B_REG_SWCTL;
    wBuf[1] = A2B_ENUM_SWCTL_ENSW_EN;
    status = a2b_i2cSlaveWrite( ctx, dscNodeAddr, 2u, wBuf );

    if ( A2B_FAILED(status) )
    {
        A2B_DSCVRY_ERROR1( ctx, "PreSlaveInit",
                           "Cannot enable phantom power on nodeAddr: %hd",
                           &dscNodeAddr );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }

    /* Only listen for power fault interrupts if it's not the last node
     * in the network. It's likely the last node will always report an
     * open circuit power fault and therefore that interrupt is not enabled
     * on this node
     */

    /* Set up INTMSK0 so we can see power fault interrupts during discovery */
    value = a2b_intrGetMask( ctx, dscNodeAddr );
    if ( value == A2B_INTRMASK_READERR )
    {
        A2B_DSCVRY_ERROR1( ctx, "PreSlaveInit",
                           "Cannot read INTMSK0-2 on nodeAddr: %hd",
                           &dscNodeAddr );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }
    else
    {
        /* Unmask all interrupts which might be helpful for diagnosing
         * discovery failures.
         */
        value |= (
               (a2b_UInt32)A2B_BITM_INTPND0_PWRERR
#if 0
             | A2B_BITM_INTPND0_SRFERR
             | A2B_BITM_INTPND0_BECOVF
             | A2B_BITM_INTPND0_DPERR
             | A2B_BITM_INTPND0_CRCERR
             | A2B_BITM_INTPND0_DDERR
             | A2B_BITM_INTPND0_HDCNTERR
#endif
             ) << (a2b_UInt32)A2B_INTRMASK0_OFFSET
             ;

        status = a2b_intrSetMask( ctx, dscNodeAddr, value );
        if ( A2B_FAILED(status) )
        {
            A2B_DSCVRY_ERROR1( ctx, "PreSlaveInit",
                               "Cannot write to INTMSK0-2 on nodeAddr: %hd",
                               &dscNodeAddr );

            a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_INTERNAL);
            A2B_DSCVRY_SEQEND( plugin->ctx );
            return A2B_FALSE;
        }
    }

    /* Setup/Start the discovery timer */
    if (!a2b_dscvryStartTimer( plugin, TIMER_DSCVRY ))
    {
        a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_INTERNAL);
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }

    wBuf[0] = A2B_REG_DISCVRY;
    wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[dscNodeBddIdx+1].ctrlRegs.respcycs;
    status = a2b_i2cMasterWrite( ctx, 2u, &wBuf );

    if ( A2B_FAILED(status) )
    {
        A2B_DSCVRY_ERROR0( ctx, "PreSlaveInit", "Cannot discover next node" );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }

    /* Now we wait for INTTYPE.DSCDONE */

    A2B_DSCVRY_RAWDEBUG0( plugin->ctx, "dscvryPreSlaveInit",
                          "...Waiting for INTTYPE.DSCDONE..." );
    A2B_DSCVRY_SEQEND( plugin->ctx );

    return A2B_TRUE;

} /* a2b_dscvryPreSlaveInit */


/*!****************************************************************************
*
*  \b              a2b_dscvryPreMasterInit
*
*  Steps taken to init the master node before discovery
*
*  \param          [in]    plugin       plugin specific data
*
*  \pre            None
*
*  \post           The following registers are altered:
*                  A2B_REG_INTMSK0/1/2
*                  A2B_REG_INTPND2
*                  A2B_REG_RESPCYCS
*                  A2B_REG_CONTROL
*                  A2B_REG_PLLCTL
*                  A2B_REG_I2SGCFG
*                  A2B_REG_SWCTL
*                  A2B_REG_DISCVRY
*
*  \return         FALSE=error
*                  TRUE=success
*
******************************************************************************/
static a2b_Bool
a2b_dscvryPreMasterInit
    (
    a2b_Plugin* plugin
    )
{
    a2b_UInt8 wBuf[4];
    a2b_UInt8 rBuf[3];
    a2b_Bool isAd242x = A2B_FALSE;
	a2b_Bool isAd2428 = A2B_FALSE;
    a2b_UInt8 masterBddIdx = A2B_MASTER_NODEBDDIDX;
    struct a2b_StackContext* ctx = plugin->ctx;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_UInt32 i2cCount = 0u;
    a2b_UInt32 errCode;
    a2b_Int32 nNodeAddr;
#ifdef A2B_FEATURE_SEQ_CHART
    a2b_Bool bSeqGroupShown = A2B_FALSE;
#endif
	const bdd_Node      *bddNodeObj;

	bddNodeObj = &plugin->bdd->nodes[0];

    /* The one node would be the master node only */
    if ( 1u == plugin->bdd->nodes_count )
    {
        A2B_DSCVRY_ERROR0( ctx, "PreMasterInit", "No slave nodes" );
        return A2B_FALSE;
    }

    A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                          "PreMasterInit" );

    /* Read the master node's VID/PID/Version */
    wBuf[0] = A2B_REG_VENDOR;
    status  = a2b_i2cMasterWriteRead( plugin->ctx, 1u, wBuf, 3u, rBuf );
    if ( A2B_FAILED(status) )
    {
        A2B_SEQ_GENNOTE0( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                          "Failed to read vid/pid/version of the master node" );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }
    plugin->nodeSig.hasSiliconInfo = A2B_TRUE;
    plugin->nodeSig.siliconInfo.vendorId = rBuf[0u];
    plugin->nodeSig.siliconInfo.productId = rBuf[1u];
    plugin->nodeSig.siliconInfo.version = rBuf[2u];
	if (plugin->nodeSig.siliconInfo.productId == 0x29u)
	{
		if ((bddNodeObj->nodeDescr.product != plugin->nodeSig.siliconInfo.productId))
		{
			A2B_SEQ_GENNOTE0(plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
				"master pid violation");
			A2B_DSCVRY_SEQEND(plugin->ctx);
			a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_INTERNAL);
			return A2B_FALSE;
		}
	}

    isAd242x = a2b_isAd242xChip(plugin->nodeSig.siliconInfo.vendorId,
                                  plugin->nodeSig.siliconInfo.productId);

	isAd2428 = A2B_IS_AD2428X_CHIP(plugin->nodeSig.siliconInfo.vendorId,
		plugin->nodeSig.siliconInfo.productId);

    A2B_TRACE4( (ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                        "%s Master Node: Silicon vid/pid/ver: "
                        "%02bX/%02bX/%02bX",
                        A2B_MPLUGIN_PLUGIN_NAME,
                        &(plugin->nodeSig.siliconInfo.vendorId),
                        &(plugin->nodeSig.siliconInfo.productId),
                        &(plugin->nodeSig.siliconInfo.version) ));
    A2B_SEQ_GENNOTE3( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                      "Master Node Silicon: vid/pid/ver: %02bX/%02bX/%02bX",
                      &(plugin->nodeSig.siliconInfo.vendorId),
                      &(plugin->nodeSig.siliconInfo.productId),
                      &(plugin->nodeSig.siliconInfo.version) );

    /* Initialize Interrupts */
    /*-----------------------*/
    if (!a2b_dscvryNodeInterruptInit(plugin, (a2b_Int16)masterBddIdx))
    {
        A2B_SEQ_GENNOTE0( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                          "Failed to init node interrupts" );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }

#ifdef A2B_FEATURE_COMM_CH
    /* Initialize Mailbox registers  */
    /*-----------------------*/
    if (!a2b_dscvryNodeMailboxInit(plugin, (a2b_Int16)masterBddIdx))
    {
        A2B_SEQ_GENNOTE0( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                          "Failed to init mailbox registers" );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }
#endif	/* A2B_FEATURE_COMM_CH */

    /* Clear the DSCDONE pending flag prior to starting discovery */
    wBuf[0] = A2B_REG_INTPND2;
    wBuf[1] = A2B_BITM_INTPND2_DSCDONE;
    status  = a2b_i2cMasterWrite(ctx, 2u, wBuf);
    i2cCount++;

    /* NOTE: A2B_REG_NODEADR managed by I2C, no need to set it */

    if ( A2B_SUCCEEDED(status) )
    {
        /* Set the response cycle timing */
        wBuf[0] = A2B_REG_RESPCYCS;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[masterBddIdx].ctrlRegs.respcycs;
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
        i2cCount++;
    }

	if (A2B_SUCCEEDED(status))
	{
		/* Push respcycs to appropriate nodes */
		wBuf[0] = A2B_REG_CONTROL;
		/* The AD242X (only) needs to be told it's a Master node BEFORE
		 * the PLL locks on the SYNC pin. Once the PLL is locked, setting
		 * the MSTR bit is ignored. We set it anyway so it's clear this is
		 * the master node.
		 */
		wBuf[1] = A2B_ENUM_CONTROL_START_NS;
		if (isAd242x)
		{
			wBuf[1] |= (a2b_UInt8)A2B_ENUM_CONTROL_MSTR;
		}
		if (isAd2428)
		{
			wBuf[1] |= (a2b_UInt8)(plugin->bdd->nodes[masterBddIdx].ctrlRegs.control & (A2B_ENUM_CONTROL_XCVRBINV | A2B_ENUM_CONTROL_SWBYP));
	    }
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
        i2cCount++;
    }

    /* PLLCTL only exists on older AD24XX chips (e.g. 2410) */
    if ( (A2B_SUCCEEDED(status)) && (!isAd242x) )
    {
        /* PLL configuration */
        wBuf[0] = A2B_REG_PLLCTL;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[masterBddIdx].i2cI2sRegs.pllctl;
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
        i2cCount++;
    }

    if ( A2B_SUCCEEDED(status) )
    {
        /* Rev1.0 requirement -- effects the PLL */
        wBuf[0] = A2B_REG_I2SGCFG;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[masterBddIdx].i2cI2sRegs.i2sgcfg;
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
        i2cCount++;
    }

    if ( A2B_SUCCEEDED(status) )
    {
        wBuf[0] = A2B_REG_SWCTL;
        wBuf[1] = A2B_BITM_SWCTL_ENSW;
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
        i2cCount++;
    }

    /* Setup/Start the discovery timer */
    if (!a2b_dscvryStartTimer( plugin, TIMER_DSCVRY ))
    {
        A2B_SEQ_GENERROR0( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                           "Failed to init discovery timer" );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }

    if ( A2B_SUCCEEDED(status) )
    {
        /* Start Discovery of Next Node */
        wBuf[0] = A2B_REG_DISCVRY;
        wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[masterBddIdx+(a2b_UInt8)1].ctrlRegs.respcycs;
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
        i2cCount++;
    }

    if ( A2B_FAILED(status) )
    {
        A2B_DSCVRY_ERROR1( plugin->ctx, "PreMasterInit",
                           "I2C failure at operation: %ld", &i2cCount );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }

    if(bdd_DISCOVERY_MODE_ADVANCED == a2b_ovrGetDiscMode(plugin))
    {
		a2b_dscvryInitTdmSettings( plugin, A2B_NODEADDR_MASTER );
		status = a2b_audioConfig( plugin->ctx, &plugin->pluginTdmSettings );

		if ( A2B_FAILED(status) )
		{
			nNodeAddr = A2B_NODEADDR_MASTER;
			A2B_DSCVRY_ERROR1( plugin->ctx, "dscvryPreMasterInit",
							   "Advanced Discovery - Cannot config audio for nodeAddr: %hd",
							   &nNodeAddr );
			a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_IO );
			A2B_DSCVRY_SEQEND_COND( plugin->ctx, bSeqGroupShown );
			return A2B_FALSE;
		}

		/* Do final setup of the master node and enable the up/downstream */
		if (a2b_dscvryNodeComplete( plugin, A2B_NODEADDR_MASTER,
								 A2B_TRUE, &errCode ) == A2B_EXEC_COMPLETE_FAIL)
		{
			A2B_DSCVRY_ERROR0( plugin->ctx, "dscvryPreMasterInit",
							   "Advanced Discovery - Failed to complete master node init" );
		}
    }
    /* Now we wait for INTTYPE.DSCDONE */

    A2B_DSCVRY_RAWDEBUG0( plugin->ctx, "dscvryPreMasterInit",
                          "...Waiting for INTTYPE.DSCDONE..." );
    A2B_DSCVRY_SEQEND( plugin->ctx );

    return A2B_TRUE;

} /* a2b_dscvryPreMasterInit */

#ifdef A2B_FEATURE_COMM_CH
static a2b_Bool a2b_dscvryNodeMailboxInit (a2b_Plugin* plugin, a2b_Int16 nodeBddIdx)
{
    a2b_Int16 nodeAddr = nodeBddIdx-1;
    a2b_HResult status = A2B_RESULT_SUCCESS;

    if ((nodeBddIdx < 0) || (nodeBddIdx >= (a2b_Int16)plugin->bdd->nodes_count))
    {
        return A2B_FALSE;
    }

    A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                          "Mailbox Registers" );

    if (plugin->bdd->nodes[nodeBddIdx].has_mbox)
    {
        a2b_UInt8 wBuf[4];

        if (plugin->bdd->nodes[nodeBddIdx].mbox.has_mbox0ctl)
        {
            /* Program MBOX 0 register */
            wBuf[0] = A2B_REG_MBOX0CTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeBddIdx].mbox.mbox0ctl;

            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }

            if ( A2B_FAILED(status) )
            {
                A2B_DSCVRY_SEQEND( plugin->ctx );
                return A2B_FALSE;
            }
        }

        if (plugin->bdd->nodes[nodeBddIdx].mbox.has_mbox1ctl)
        {
            /* Program MBOX 1 register */
            wBuf[0] = A2B_REG_MBOX1CTL;
            wBuf[1] = (a2b_UInt8)plugin->bdd->nodes[nodeBddIdx].mbox.mbox1ctl;

            if (A2B_NODEADDR_MASTER == nodeAddr)
            {
                status = a2b_i2cMasterWrite( plugin->ctx, 2u, wBuf );
            }
            else
            {
                status = a2b_i2cSlaveWrite( plugin->ctx, nodeAddr, 2u, wBuf );
            }

            if ( A2B_FAILED(status) )
            {
                A2B_DSCVRY_SEQEND( plugin->ctx );
                return A2B_FALSE;
            }
        }
    }

    A2B_DSCVRY_SEQEND( plugin->ctx );

    return A2B_TRUE;

} /* a2b_dscvryNodeMailboxInit */
#endif	/* A2B_FEATURE_COMM_CH */

/*!****************************************************************************
*
*  \b               a2b_getNodeSignature
*
*  Returns the node signature information for the specified node address.
*
*  \param           [in]    plugin      The A2B master plugin instance.
*
*  \param           [in]    nodeAddr    The node address to retrieve its
*                                       signature. Slave node addresses are
*                                       in the range [0, N] while the master
*                                       node is A2B_NODEADDR_MASTER.
*
*  \pre             None
*
*  \post            None
*
*  \return          Returns a pointer to the constant node signature structure.
*                   The caller should **not** attempt to free this structure.
*
******************************************************************************/
static const a2b_NodeSignature*
a2b_getNodeSignature
    (
    a2b_Plugin* plugin,
    a2b_Int16   nodeAddr
    )
{
    const a2b_NodeSignature*    nodeSig;
    a2b_UInt32                  iter = 0u;

    /* Search for the node signature information for this slave node */
    do
    {
        nodeSig = a2b_msgRtrGetHandler(plugin->ctx, &iter);
        if ( (nodeSig != A2B_NULL) &&
            (nodeSig->nodeAddr == nodeAddr) )
        {
            break;
        }
    }
    while ( A2B_NULL != nodeSig );

    return nodeSig;
} /* a2b_getNodeSignature */


/*!****************************************************************************
*
*  \b              a2b_dscvryFindNodeHandler
*
*  If a search hasn't been done to find a handler (plugin) to manage this
*  node then try to find one. If this search has already been done then the
*  function returns immediately.
*
*  \param          [in]    plugin       The master plugin specific data.
*
*  \param          [in]    slaveNodeIdx Zero-based index into a slave node
*                                       array where index 0 is the first
*                                       slave, index 1 is second slave, etc...
*
*  \pre            None
*
*  \post           If a handler (e.g. plugin) has been found for this node
*                  then it is bound to this node.
*
*  \return         None
*
******************************************************************************/
static void
a2b_dscvryFindNodeHandler
    (
    a2b_Plugin* plugin,
    a2b_UInt16  slaveNodeIdx
    )
{
    a2b_HResult                 status;
    const a2b_NodeSignature*    nodeSig = &plugin->slaveNodeSig[slaveNodeIdx];

    /* For this use-case the slave node index is identical to the node
     * address.
     */
    a2b_Int16                   nodeAddr =
                                    A2B_MAP_SLAVE_INDEX_TO_ADDR(slaveNodeIdx);

    /* If we've already searched for a handler to manage this plugin
     * then ...
     */
    if ( A2B_HAS_SEARCHED_FOR_HANLDER(plugin, nodeAddr) )
    {
        /* No need to search again */
        return;
    }

    /* Look for a plugin to manage this node */
    status = a2b_stackFindHandler( plugin->ctx, nodeSig);

    /* Mark it as being searched - whether successful or not */
    plugin->discovery.hasSearchedForHandler |= ((a2b_UInt32)1 << (a2b_UInt32)nodeAddr);

    if ( A2B_FAILED(status) )
    {
        A2B_TRACE2( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG),
                    "%s findNodeHandler(): No plugin for node: %hd",
                    A2B_MPLUGIN_PLUGIN_NAME, &nodeAddr ));
    }
    else
    {
        /* Track that a plugin exists and will manage this node */
        plugin->discovery.hasPlugin |= ((a2b_UInt32)1 << (a2b_UInt32)nodeAddr);
        plugin->discovery.needsPluginInit |= ((a2b_UInt32)1 << (a2b_UInt32)nodeAddr);

        /* Get the updated signature with the actual plugin name */
        nodeSig = a2b_getNodeSignature(plugin, nodeAddr);
        A2B_SEQ_GENNOTE2( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                          "Plugin '%s' opened to manage node: %hd",
                          nodeSig->pluginName, &nodeAddr);
    }
}


/*!****************************************************************************
*
*  \b              a2b_dscvryNodeDiscovered
*
*  Called after a node has been discovered.  This can be used to setup
*  the new node depending on the discovery process being used.
*
*  \param          [in]    plugin   plugin specific data
*
*  \pre            Called on DSCDONE
*
*  \post           The following registers are altered:
*                  A2B_REG_SWCTL
*
*  \return         FALSE=error
*                  TRUE=success
*
******************************************************************************/
a2b_Bool
a2b_dscvryNodeDiscovered
    (
    a2b_Plugin* plugin
    )
{
    a2b_Bool 	bRet = A2B_FALSE, bNetConfigFlag = A2B_FALSE;
#ifdef ADI_SIGMASTUDIO_BCF
    a2b_Bool nFlgSupIdMatch = A2B_TRUE, nFlgGpioMatch = A2B_TRUE;
#endif
    a2b_UInt8 	wBuf[4u];
    a2b_UInt8 	rBuf[8u];
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_Int16 	dscNodeAddr = (a2b_Int16)plugin->discovery.dscNumNodes;
    a2b_Int16 	dscNodeIdx = dscNodeAddr+1;
    struct a2b_StackContext* ctx = plugin->ctx;
    bdd_DiscoveryMode 	eDiscMode;
#ifdef ADI_SIGMASTUDIO_BCF
    a2b_UInt16 			nRead;
	a2b_UInt8 			nIdx, rBufCustomNodeId[50u], rBufGpio[8u];
#endif
	a2b_NodeSignature   nodeSig;
	const bdd_Node      *bddNodeObj;
	const bdd_Node      *bddMstrNodeObj;
	a2b_Bool verifyNodeDescr;
#ifdef A2B_FEATURE_COMM_CH
	a2b_CommChMsg		oCommChMsgGetCustNodeId;
	A2B_COMMCH_RET		bCommChRet;
	a2b_UInt16			nCommChInstNo;
#endif	/* A2B_FEATURE_COMM_CH */

    eDiscMode = a2b_ovrGetDiscMode(plugin);
    if (!plugin->discovery.inDiscovery)
    {
        /* Ignore DSCDONE when we are NOT discovering */
        return A2B_FALSE;
    }

    A2B_DSCVRY_SEQGROUP1( ctx,
                          "NodeDiscovered nodeAddr %hd", &dscNodeAddr);

    /* Stop the previously running timer */
    a2b_timerStop( plugin->timer );

    /* Enable phantom power with external switch mode
     *
     * NOTE: v3 ADI documentation shows the Simple Discovery flow
     *       sending this only to the Master, which the documentation
     *       verbiage says to send it to the slave.  We will send it to
     *       the slave node.
     */
    wBuf[0] = A2B_REG_SWCTL;
    wBuf[1] = A2B_BITM_SWCTL_ENSW | A2B_ENUM_SWCTL_MODE_VOLT_ON_VIN;
    if ( A2B_NODEADDR_MASTER == dscNodeAddr - 1 )
    {
        status = a2b_i2cMasterWrite( ctx, 2u, wBuf );
    }
    else
    {
        /* Alters the previously discovered nodes SWCTL */
        status = a2b_i2cSlaveWrite( ctx, dscNodeAddr - 1, 2u, wBuf );
    }

    /* NOTE: A2B_REG_NODEADR managed by I2C, no need to set it */

    A2B_DSCVRY_SEQGROUP0( ctx,
                          "A2B VID/PID/VERSION/CAP" );

    /* Read the new nodes VID/PID/VERSION/CAP */
    wBuf[0] = A2B_REG_VENDOR;
    status  = a2b_i2cSlaveWriteRead( ctx, dscNodeAddr, 1u, wBuf, 4u, rBuf );

    A2B_DSCVRY_SEQEND( plugin->ctx );

    bddNodeObj = &plugin->bdd->nodes[dscNodeIdx];
	bddMstrNodeObj = &plugin->bdd->nodes[0];

    if ( A2B_SUCCEEDED(status) )
    {
#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
        a2b_UInt16          capability = rBuf[3u];
#endif

        A2B_INIT_SIGNATURE( &nodeSig, dscNodeAddr );

        /* Silicon VID/PID/VER */
        nodeSig.hasSiliconInfo         = A2B_TRUE;
        nodeSig.siliconInfo.vendorId   = rBuf[0u];
        nodeSig.siliconInfo.productId  = rBuf[1u];
        nodeSig.siliconInfo.version    = rBuf[2u];

        A2B_TRACE6( (ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                    "%s nodeDiscovered(): Silicon node/vid/pid/ver/cap: "
                    "%02hX/%02bX/%02bX/%02bX/%02bX",
                    A2B_MPLUGIN_PLUGIN_NAME, &dscNodeAddr,
                    &nodeSig.siliconInfo.vendorId,
                    &nodeSig.siliconInfo.productId,
                    &nodeSig.siliconInfo.version,
                    &capability ));

        A2B_SEQ_GENNOTE4( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                          "Silicon vid/pid/ver/cap: %02bX/%02bX/%02bX/%02bX",
                          &nodeSig.siliconInfo.vendorId,
                          &nodeSig.siliconInfo.productId,
                          &nodeSig.siliconInfo.version,
                          &capability );

        /* Verify the stack supports this node */
        if (!a2b_stackSupportedNode(   nodeSig.siliconInfo.vendorId,
                                       nodeSig.siliconInfo.productId,
                                       nodeSig.siliconInfo.version ))
        {
            A2B_DSCVRY_ERROR5( ctx, "nodeDiscovered",
                               "Incompatible node %hd (%02bX/%02bX/%02bX/%02bX)",
                               &dscNodeAddr,
                               &nodeSig.siliconInfo.vendorId,
                               &nodeSig.siliconInfo.productId,
                               &nodeSig.siliconInfo.version,
                               &capability );

            bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);
            if((bdd_DISCOVERY_MODE_SIMPLE == eDiscMode) && (bNetConfigFlag))
            {
            	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_PERMISSION;
            	a2b_dscvryNetComplete(plugin);
            }
            else
            {
				a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_PERMISSION );
            }

            A2B_DSCVRY_SEQEND( plugin->ctx );
            return A2B_FALSE;
        }

		/* Mandatory checking for AD2420 & AD2429 */
		verifyNodeDescr = (a2b_Bool)bddNodeObj->verifyNodeDescr;
		if ((nodeSig.siliconInfo.productId == 0x29u) || (bddMstrNodeObj->nodeDescr.product == 0x29u))
		{
			verifyNodeDescr = A2B_TRUE;
		}

        /* Optionally validate the node descriptor info */
        if ((verifyNodeDescr) &&
            (( bddNodeObj->nodeDescr.product !=
                                   nodeSig.siliconInfo.productId ) ||
             ( bddNodeObj->nodeDescr.version !=
                                   nodeSig.siliconInfo.version )) )
        {

			/* Copy the signature information to the plugin */
			plugin->slaveNodeSig[dscNodeAddr] = nodeSig;
            A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
                              "Failed Authentication ",
                              &dscNodeAddr );

            bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);
            if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
            {
            	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_PERMISSION;
            	a2b_dscvryNetComplete(plugin);
            }
            else
            {
				a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_PERMISSION );
            }
            A2B_DSCVRY_SEQEND( plugin->ctx );
            return A2B_FALSE;
        }

        nodeSig.hasI2cCapability = (a2b_Bool)((rBuf[3u] & A2B_BITM_CAPABILITY_I2CAVAIL) != 0u);

#ifdef ADI_BDD
		/* Custom Node Identification - Start */
		if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.bCustomNodeIdAuth == A2B_TRUE)
		{
			/* Authenticate by reading from EEPROM memory */
			if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.bReadFrmMemory == A2B_TRUE)
			{
				wBuf[0u]  =  (a2b_UInt8)(bddNodeObj->nodeDescr.oCustomNodeIdSettings.nReadMemAddr >> 8u);
				wBuf[1u]  =  (a2b_UInt8)(bddNodeObj->nodeDescr.oCustomNodeIdSettings.nReadMemAddr & 0xFFu);

				nRead = (a2b_UInt16)bddNodeObj->nodeDescr.oCustomNodeIdSettings.nNodeIdLength;

				status = a2b_i2cPeriphWriteRead(plugin->ctx,
												dscNodeAddr,
												(a2b_UInt16)bddNodeObj->nodeDescr.oCustomNodeIdSettings.nDeviceAddr,
												2u, wBuf,
												nRead, rBufCustomNodeId);

				if (A2B_SUCCEEDED(status))
				{
					for(nIdx=0u; nIdx<nRead ; nIdx++)
					{
						if(rBufCustomNodeId[nIdx] != (a2b_UInt8)(bddNodeObj->nodeDescr.oCustomNodeIdSettings.nNodeId[nIdx]))
						{
							nFlgSupIdMatch = A2B_FALSE;
							break;
						}
					}

					if(nFlgSupIdMatch == A2B_FALSE)
					{
						/* Copy the signature information to the plugin */
						plugin->slaveNodeSig[dscNodeAddr] = nodeSig;
			            A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
			                              "Node %hd: Custom Node Id Authentication Failed via read from memory",
			                              &dscNodeAddr );

			            bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);
			            if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
			            {
			            	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH;
			            	a2b_dscvryNetComplete(plugin);
			            }
			            else
			            {
							a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH );
			            }
			            A2B_DSCVRY_SEQEND( plugin->ctx );
			            return (A2B_FALSE);
					}
				}
	            else
	            {
				   A2B_DSCVRYNOTE_DEBUG1( ctx, "nodeDiscovered", "Node %hd: Failed to read EEPROM",
	                                       &dscNodeAddr );

		            bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);
		            if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
		            {
		            	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH;
		            	a2b_dscvryNetComplete(plugin);
		            }
		            else
		            {
						a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH );
		            }
					 return A2B_FALSE;

	            }
			}

			/* Authenticate by GPIO pins */
			if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.bReadGpioPins == A2B_TRUE)
			{
				wBuf[0u]  =  A2B_REG_GPIOIN;

				status = a2b_i2cSlaveWriteRead(plugin->ctx,
											   dscNodeAddr,
											   1u, wBuf,
											   8u, rBufGpio);

				if (A2B_SUCCEEDED(status))
				{
					for(nIdx=0u; nIdx<8u ; nIdx++)
					{
						if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.aGpio[nIdx] != A2B_IGNORE)
						{
							if(rBufGpio[nIdx] != bddNodeObj->nodeDescr.oCustomNodeIdSettings.aGpio[nIdx])
							{
								nFlgGpioMatch = A2B_FALSE;
								break;
							}
						}
					}

					if(nFlgGpioMatch == A2B_FALSE)
					{
						/* Copy the signature information to the plugin */
						plugin->slaveNodeSig[dscNodeAddr] = nodeSig;
			            A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
			                              "Node %hd Custom Node Id Authentication via GPIO ",
			                              &dscNodeAddr );

			            bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);
			            if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
			            {
			            	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH;
			            	a2b_dscvryNetComplete(plugin);
			            }
			            else
			            {
							a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH );
			            }
			            A2B_DSCVRY_SEQEND( plugin->ctx );
			            return (A2B_FALSE);
					}
				}
				else
	            {
	                A2B_DSCVRYNOTE_DEBUG1( ctx, "nodeDiscovered",
	                                       "Node %hd Custom Node Id Authentication from GPIO sequence failed",
	                                       &dscNodeAddr );
	            }
			}

#ifdef A2B_FEATURE_COMM_CH
			/* Authenticate by getting the authentication message via communication channel */
			if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.bReadFrmCommCh == A2B_TRUE)
			{
				/* Program the mailbox registers so that we can exchange the authentication request and response messages over mailbox */
				bRet = a2b_dscvryNodeMailboxInit(plugin, dscNodeIdx);
				if(bRet == A2B_FALSE)
				{
					A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
											   "Node %hd authorization via Communication channel ,"
											   "Programming of Mailbox registers failed ",
											   &dscNodeAddr );
				}

		        /* Copy the signature information to the plugin */
		        plugin->slaveNodeSig[dscNodeAddr] = nodeSig;

				/* Prepare and send authentication message */
				oCommChMsgGetCustNodeId.nMsgId			= A2B_COMMCH_MSG_REQ_SLV_NODE_SIGNATURE;
				oCommChMsgGetCustNodeId.nMsgLenInBytes 	= 0u;
				oCommChMsgGetCustNodeId.pMsgPayload		= NULL;
				bRet = a2b_GetCommChInstIdForSlvNode(plugin, dscNodeAddr, &nCommChInstNo);
				if(bRet == A2B_TRUE)
				{
					bCommChRet = adi_a2b_CommChMstrTxMsg(plugin->commCh.commChHnd[nCommChInstNo], &oCommChMsgGetCustNodeId , (a2b_Int8)dscNodeAddr);
					if(bCommChRet == A2B_COMMCH_FAILED)
					{
						A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
															   "Node %hd authorization via Communication channel ,"
															   "Transmission of request Node id message failed ",
															   &dscNodeAddr );
					}
				}
				else
				{
					A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
														   "Node %hd authorization via Communication channel ,"
														   "Communication Channel Instance for slave node address not available ",
														   &dscNodeAddr );
				}

			}
#endif	/* A2B_FEATURE_COMM_CH */
		}
		/* Custom Node Identification - End */
#endif

#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
		/* Authenticate by getting the authentication message via communication channel */
		if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.bReadFrmCommCh == A2B_FALSE)
		{
#endif
#endif	/* A2B_FEATURE_COMM_CH */
			/* Used only during simple discovery with sync periph processing */
			plugin->discovery.simpleNodeCount++;
			plugin->discovery.dscNumNodes++;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
        if ((( nodeSig.hasI2cCapability ) && ( !bddNodeObj->ignEeprom ) &&
             ( ( plugin->overrides[0u] & A2B_MPLUGIN_IGN_EEPROM ) == 0u )) ||
            ( plugin->periphCfg.nodeCfg[dscNodeAddr] ))
        {
#ifndef A2B_BCF_FROM_SOC_EEPROM
            A2B_DSCVRY_SEQGROUP0( ctx,
                                  "Look for EEPROM at 0x50" );

            /* Read the EEPROM header             */
            /* [Two byte internal EEPROM address] */
            wBuf[0] = 0u;
            wBuf[1] = 0u;
            status = a2b_periphCfgWriteRead( plugin,
                                             dscNodeAddr,
                                             2u,  wBuf,
                                             8u,  rBuf);

            if (( A2B_SUCCEEDED(status) ) &&
                ( rBuf[0] == A2B_MARKER_EEPROM_CONFIG ))
            {
                a2b_UInt8 crc8 = a2b_crc8(rBuf, 0u, 7u);

                if ( crc8 == rBuf[7] )
                {
                    /* EEPROM VID/PID/VER */
                    nodeSig.hasEepromInfo        = A2B_TRUE;
                    nodeSig.eepromInfo.vendorId  = rBuf[1];
                    nodeSig.eepromInfo.productId = rBuf[2];
                    nodeSig.eepromInfo.version   = rBuf[3];

                    A2B_TRACE5( (ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                                "%s nodeDiscovered(): EEPROM node/vid/pid/ver: "
                                "%02hX/%02bX/%02bX/%02bX",
                                A2B_MPLUGIN_PLUGIN_NAME, &dscNodeAddr,
                                &nodeSig.eepromInfo.vendorId,
                                &nodeSig.eepromInfo.productId,
                                &nodeSig.eepromInfo.version ));

                    A2B_SEQ_GENNOTE3( plugin->ctx,
                                      A2B_SEQ_CHART_LEVEL_DISCOVERY,
                                      "EEPROM vid/pid/ver: %02bX/%02bX/%02bX",
                                      &nodeSig.eepromInfo.vendorId,
                                      &nodeSig.eepromInfo.productId,
                                      &nodeSig.eepromInfo.version );

                    /* When this override is set we will parse the version
                     * (which we did) but we will not indicate that there
                     * was an EEPRROM so we will avoid peripheral processing
                     */
                    if (( plugin->overrides[0] & A2B_MPLUGIN_EEPROM_VER_ONLY ) == 0u )
                    {
                        /* See if we have an override for this specific node */
                        if (( plugin->overrides[1] & (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr)) == (a2b_UInt32)0u )
                        {
                            plugin->discovery.hasEeprom |= (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr);
                        }
                        else
                        {
                            A2B_DSCVRYNOTE_DEBUG1( plugin->ctx,
                                       "nodeDiscovered",
                                       "Override Set, Ignoring node %hd EEPROM",
                                       &dscNodeAddr );
                        }
                    }

                    /* Optionally validate the node descriptor info */
                    if (( bddNodeObj->verifyNodeDescr ) &&
                        (( bddNodeObj->nodeDescr.vendor !=
                                               nodeSig.eepromInfo.vendorId ) ||
                         ( bddNodeObj->nodeDescr.product !=
                                               nodeSig.eepromInfo.productId ) ||
                         ( bddNodeObj->nodeDescr.version !=
                                               nodeSig.eepromInfo.version )) )
                    {
                        /* clear the bit */
                        plugin->discovery.hasEeprom ^= (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr);

                        A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
                                          "Node %hd EEPROM failed verification",
                                          &dscNodeAddr );

                        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_PERMISSION );
                        A2B_DSCVRY_SEQEND( plugin->ctx );
                        return A2B_FALSE;
                    }
                }
                else
                {
                    A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
                                       "Node %hd EEPROM header CRC incorrect",
                                       &dscNodeAddr );
                }
            }
            else
            {
                A2B_DSCVRYNOTE_DEBUG1( ctx, "nodeDiscovered",
                                       "Node %hd EEPROM not found",
                                       &dscNodeAddr );
            }

            A2B_DSCVRY_SEQEND( plugin->ctx );
#else
            plugin->discovery.hasEeprom |= (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr);
#endif
        }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

        /* BDD VID/PID/VER */
        nodeSig.hasBddInfo        = A2B_TRUE;
        nodeSig.bddInfo.vendorId  = (a2b_UInt8)bddNodeObj->nodeDescr.vendor;
        nodeSig.bddInfo.productId = (a2b_UInt8)bddNodeObj->nodeDescr.product;
        nodeSig.bddInfo.version   = (a2b_UInt8)bddNodeObj->nodeDescr.version;

        A2B_TRACE5( (ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                    "%s nodeDiscovered(): BDD node/vid/pid/ver: "
                    "%02hX/%02bX/%02bX/%02bX",
                    A2B_MPLUGIN_PLUGIN_NAME, &dscNodeAddr,
                    &nodeSig.bddInfo.vendorId,
                    &nodeSig.bddInfo.productId,
                    &nodeSig.bddInfo.version ));

        A2B_SEQ_GENNOTE3( plugin->ctx,
                          A2B_SEQ_CHART_LEVEL_DISCOVERY,
                          "BDD vid/pid/ver: %02bX/%02bX/%02bX",
                          &nodeSig.bddInfo.vendorId,
                          &nodeSig.bddInfo.productId,
                          &nodeSig.bddInfo.version );

        /* Copy the signature information to the plugin */
        plugin->slaveNodeSig[dscNodeAddr] = nodeSig;

#ifndef FIND_NODE_HANDLER_AFTER_NODE_INIT
        /* We'll attempt to find and open a plugin that can manage the
         * slave node *before* completing the full initialization of the
         * node.
         */
        a2b_dscvryFindNodeHandler(plugin, dscNodeAddr);
#endif
#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
		}	/* If bReadFrmCommCh == A2B_FALSE */
#endif
#endif	/* A2B_FEATURE_COMM_CH */

    }	/* Successful reading of VID, PID */
    else
    {
        A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
                           "Cannot read VID/PID/VER/CAP for node: %hd",
                           &dscNodeAddr );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_PERMISSION );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_FALSE;
    }
    A2B_DSCVRY_SEQEND( plugin->ctx );

#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
	if(bddNodeObj->nodeDescr.oCustomNodeIdSettings.bReadFrmCommCh == A2B_FALSE)
	{
#endif
#endif	/* A2B_FEATURE_COMM_CH */
		if ( bdd_DISCOVERY_MODE_MODIFIED == eDiscMode )
		{
			(void)adi_a2b_ConfigureNodePeri(plugin, dscNodeAddr);

		} /* end Modified processing */
		else if((bdd_DISCOVERY_MODE_OPTIMIZED == eDiscMode) ||
				(bdd_DISCOVERY_MODE_ADVANCED == eDiscMode))
		{
			if(plugin->discovery.pendingPluginInit == (a2b_UInt8)0)
			{
				bRet = adi_a2b_ConfigNodeOptimizAdvancedMode(plugin, dscNodeAddr);
			}
		}
		else
		{
			bRet = a2b_dscvryPreSlaveInit( plugin );
		}
#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
	}
#endif
#endif	/* A2B_FEATURE_COMM_CH */

    /* Now we wait for INTTYPE.DSCDONE on success */

    return bRet;

} /* a2b_dscvryNodeDiscovered */

#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
/*!****************************************************************************
*
*  \b              adi_a2b_MstrPluginCommChStatusCb
*
*  Master plugin communication channel status callback function
*
*  \param          [in]    pPlugin         Pointer to master plugin specific data
*  \param          [in]    pRxMsg          Pointer to message
*  \param		   [in]	   eEventType      Enumeration of the event type
*  \param		   [in]	   nNodeAddr       Node address from which the msg is received
*
*  \pre            None
*
*  \post           None
*
*  \return          Returns a Boolean flag of whether the node was configured
*  					successfully.
******************************************************************************/
a2b_Bool adi_a2b_MstrPluginCommChStatusCb(void* pPlugin, a2b_CommChMsg *pMsg, A2B_COMMCH_EVENT eEventType, a2b_Int8 nNodeAddr)
{
	a2b_UInt32			nIdx;
	a2b_NodeSignature   nodeSig;
	a2b_Bool 			bRet, nFlgSupIdMatch = A2B_TRUE, bNetConfigFlag = A2B_FALSE;
	a2b_Plugin* 		plugin 		= (a2b_Plugin*)pPlugin;
    a2b_Int16 			dscNodeAddr = (a2b_Int16)plugin->discovery.dscNumNodes;
    a2b_Int16 			dscNodeIdx 	= dscNodeAddr+1;
    const bdd_Node		*bddNodeObj = &plugin->bdd->nodes[dscNodeIdx];

    A2B_INIT_SIGNATURE( &nodeSig, dscNodeAddr );

	switch(eEventType)
	{
	case A2B_COMMCH_EVENT_RX_MSG:
		if(pMsg->nMsgId == A2B_COMMCH_MSG_RSP_SLV_NODE_SIGNATURE)
		{
			/* Stop authentication timeout timer */
			a2b_timerStop( plugin->timer );

			if (pMsg->nMsgLenInBytes == bddNodeObj->nodeDescr.oCustomNodeIdSettings.nNodeIdLength)
			{
				for (nIdx = 0u; nIdx < pMsg->nMsgLenInBytes; nIdx++)
				{
					if (pMsg->pMsgPayload[nIdx] != bddNodeObj->nodeDescr.oCustomNodeIdSettings.nNodeId[nIdx])
					{
						nFlgSupIdMatch = A2B_FALSE;
						break;
					}
				}
			}
			else
			{
				nFlgSupIdMatch = A2B_FALSE;
			}

			if(nFlgSupIdMatch == A2B_FALSE)	/* End discovery */
			{
				/* Copy the signature information to the plugin */
				plugin->slaveNodeSig[dscNodeAddr] = nodeSig;

	            A2B_DSCVRY_ERROR1( plugin->ctx, "nodeDiscovered", "Node %hd: Custom Node Id Authentication Failed while reading via communication channel", &dscNodeAddr );

	            bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);

	            if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
	            {
	            	plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH;
	            	a2b_dscvryNetComplete(plugin);
	            }
	            else
	            {
					a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_AUTH );
	            }

	            A2B_DSCVRY_SEQEND( plugin->ctx );
	            return (A2B_FALSE);
			}
			else
			{
				bRet = a2b_dscvryPostAuthViaCommCh(plugin);
			}
		}
		break;
	case A2B_COMMCH_EVENT_TX_DONE:

		/* If request message is successfully transmitted start the timeout timer
		 * as per timeout specified in bdd
		 * */
		if(pMsg->nMsgId == A2B_COMMCH_MSG_REQ_SLV_NODE_SIGNATURE)
		{
			bRet = a2b_dscvryStartCommChAuthTimer(plugin, bddNodeObj->nodeDescr.oCustomNodeIdSettings.nTimeOut);
		}
		break;
	case A2B_COMMCH_EVENT_TX_TIMEOUT:

		/* If request message transmission has timed out
		 * then end discovery
		 * */
		if(pMsg->nMsgId == A2B_COMMCH_MSG_REQ_SLV_NODE_SIGNATURE)
		{
			/* Copy the signature information to the plugin */
			plugin->slaveNodeSig[dscNodeAddr] = nodeSig;

			A2B_DSCVRY_ERROR1( plugin->ctx, "nodeDiscovered", "Node %hd: Custom Node Id Authentication Failed : Transmission via communication channel timed out ", &dscNodeAddr );

			bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);

			if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
			{
				plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_TIMEOUT;
				a2b_dscvryNetComplete(plugin);
			}
			else
			{
				a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_TIMEOUT );
			}

			A2B_DSCVRY_SEQEND( plugin->ctx );
			return (A2B_FALSE);
		}
		break;
	default :
		bRet = A2B_FALSE;
		break;
	}
	return bRet;
}

/*!****************************************************************************
*
*  \b              a2b_dscvryPostAuthViaCommCh
*
*  Function used to continue the discovery flow after the current discovered node
*  has been authorized via communication channel.This requires a separate flow
*  since authorization by communication channel requires a non-blocking communi-
*  cation between master and slave node spanning multiple ticks.
*
*  \param          [in]    plugin         Pointer to master plugin
*  \pre            None
*
*  \post           None
*
*  \return          Returns a Boolean flag of whether the node was configured
*  					successfully.
******************************************************************************/
static a2b_Bool a2b_dscvryPostAuthViaCommCh(a2b_Plugin* plugin)
{
    a2b_Bool 	bRet;
    a2b_Int16 	dscNodeAddr = (a2b_Int16)plugin->discovery.dscNumNodes;
    a2b_Int16 	dscNodeIdx = dscNodeAddr+1;
    bdd_DiscoveryMode 	eDiscMode;
    a2b_NodeSignature   nodeSig;
    const bdd_Node      *bddNodeObj = &plugin->bdd->nodes[dscNodeIdx];
    struct a2b_StackContext* ctx = plugin->ctx;
#ifdef A2B_FEATURE_EEPROM_PROCESSING
    a2b_UInt8 	wBuf[4u];
    a2b_UInt8 	rBuf[8u];
    a2b_HResult status = A2B_RESULT_SUCCESS;
#endif

    eDiscMode = a2b_ovrGetDiscMode(plugin);
	/* Used only during simple discovery with sync periph processing */
	plugin->discovery.simpleNodeCount++;
	plugin->discovery.dscNumNodes++;

    /* Copy the signature information from the plugin */
    nodeSig = plugin->slaveNodeSig[dscNodeAddr];

#ifdef A2B_FEATURE_EEPROM_PROCESSING
	if ((( nodeSig.hasI2cCapability ) && ( !bddNodeObj->ignEeprom ) &&
		 ( ( plugin->overrides[0u] & A2B_MPLUGIN_IGN_EEPROM ) == 0u )) ||
		( plugin->periphCfg.nodeCfg[dscNodeAddr] ))
	{
		A2B_DSCVRY_SEQGROUP0( ctx,
							  "Look for EEPROM at 0x50" );

		/* Read the EEPROM header             */
		/* [Two byte internal EEPROM address] */
		wBuf[0] = 0u;
		wBuf[1] = 0u;
		status = a2b_periphCfgWriteRead( plugin,
										 dscNodeAddr,
										 2u,  wBuf,
										 8u,  rBuf);

		if (( A2B_SUCCEEDED(status) ) &&
			( rBuf[0] == A2B_MARKER_EEPROM_CONFIG ))
		{
			a2b_UInt8 crc8 = a2b_crc8(rBuf, 0u, 7u);

			if ( crc8 == rBuf[7] )
			{
				/* EEPROM VID/PID/VER */
				nodeSig.hasEepromInfo        = A2B_TRUE;
				nodeSig.eepromInfo.vendorId  = rBuf[1];
				nodeSig.eepromInfo.productId = rBuf[2];
				nodeSig.eepromInfo.version   = rBuf[3];

				A2B_TRACE5( (ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
							"%s nodeDiscovered(): EEPROM node/vid/pid/ver: "
							"%02hX/%02bX/%02bX/%02bX",
							A2B_MPLUGIN_PLUGIN_NAME, &dscNodeAddr,
							&nodeSig.eepromInfo.vendorId,
							&nodeSig.eepromInfo.productId,
							&nodeSig.eepromInfo.version ));

				A2B_SEQ_GENNOTE3( plugin->ctx,
								  A2B_SEQ_CHART_LEVEL_DISCOVERY,
								  "EEPROM vid/pid/ver: %02bX/%02bX/%02bX",
								  &nodeSig.eepromInfo.vendorId,
								  &nodeSig.eepromInfo.productId,
								  &nodeSig.eepromInfo.version );

				/* When this override is set we will parse the version
				 * (which we did) but we will not indicate that there
				 * was an EEPRROM so we will avoid peripheral processing
				 */
				if (( plugin->overrides[0] & A2B_MPLUGIN_EEPROM_VER_ONLY ) == 0u )
				{
					/* See if we have an override for this specific node */
					if (( plugin->overrides[1] & (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr)) == (a2b_UInt32)0u )
					{
						plugin->discovery.hasEeprom |= (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr);
					}
					else
					{
						A2B_DSCVRYNOTE_DEBUG1( plugin->ctx,
								   "nodeDiscovered",
								   "Override Set, Ignoring node %hd EEPROM",
								   &dscNodeAddr );
					}
				}

				/* Optionally validate the node descriptor info */
				if (( bddNodeObj->verifyNodeDescr ) &&
					(( bddNodeObj->nodeDescr.vendor !=
										   nodeSig.eepromInfo.vendorId ) ||
					 ( bddNodeObj->nodeDescr.product !=
										   nodeSig.eepromInfo.productId ) ||
					 ( bddNodeObj->nodeDescr.version !=
										   nodeSig.eepromInfo.version )) )
				{
					/* clear the bit */
					plugin->discovery.hasEeprom ^= (a2b_UInt32)((a2b_UInt32)1u << dscNodeAddr);

					A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
									  "Node %hd EEPROM failed verification",
									  &dscNodeAddr );

					a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_PERMISSION );
					A2B_DSCVRY_SEQEND( plugin->ctx );
					return A2B_FALSE;
				}
			}
			else
			{
				A2B_DSCVRY_ERROR1( ctx, "nodeDiscovered",
								   "Node %hd EEPROM header CRC incorrect",
								   &dscNodeAddr );
			}
		}
		else
		{
			A2B_DSCVRYNOTE_DEBUG1( ctx, "nodeDiscovered",
								   "Node %hd EEPROM not found",
								   &dscNodeAddr );
		}
		A2B_DSCVRY_SEQEND( plugin->ctx );
	}
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

	/* BDD VID/PID/VER */
	nodeSig.hasBddInfo        = A2B_TRUE;
	nodeSig.bddInfo.vendorId  = (a2b_UInt8)bddNodeObj->nodeDescr.vendor;
	nodeSig.bddInfo.productId = (a2b_UInt8)bddNodeObj->nodeDescr.product;
	nodeSig.bddInfo.version   = (a2b_UInt8)bddNodeObj->nodeDescr.version;

	A2B_TRACE5( (ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
				"%s nodeDiscovered(): BDD node/vid/pid/ver: "
				"%02hX/%02bX/%02bX/%02bX",
				A2B_MPLUGIN_PLUGIN_NAME, &dscNodeAddr,
				&nodeSig.bddInfo.vendorId,
				&nodeSig.bddInfo.productId,
				&nodeSig.bddInfo.version ));

	A2B_SEQ_GENNOTE3( plugin->ctx,
					  A2B_SEQ_CHART_LEVEL_DISCOVERY,
					  "BDD vid/pid/ver: %02bX/%02bX/%02bX",
					  &nodeSig.bddInfo.vendorId,
					  &nodeSig.bddInfo.productId,
					  &nodeSig.bddInfo.version );

	/* Copy the signature information to the plugin */
	plugin->slaveNodeSig[dscNodeAddr].bddInfo = nodeSig.bddInfo;

#ifndef FIND_NODE_HANDLER_AFTER_NODE_INIT
	/* We'll attempt to find and open a plugin that can manage the
	 * slave node *before* completing the full initialization of the
	 * node.
	 */
	a2b_dscvryFindNodeHandler(plugin, dscNodeAddr);
#endif

	if ( bdd_DISCOVERY_MODE_MODIFIED == eDiscMode )
	{
		(void)adi_a2b_ConfigureNodePeri(plugin, dscNodeAddr);

	} /* end Modified processing */
	else if((bdd_DISCOVERY_MODE_OPTIMIZED == eDiscMode) ||
			(bdd_DISCOVERY_MODE_ADVANCED == eDiscMode))
	{
		if(plugin->discovery.pendingPluginInit == (a2b_UInt8)0)
		{
			bRet = adi_a2b_ConfigNodeOptimizAdvancedMode(plugin, dscNodeAddr);
		}
	}
	else
	{
		bRet = a2b_dscvryPreSlaveInit( plugin );
	}

	return bRet;
}

/*!****************************************************************************
*
*  \b              a2b_dscvryStartCommChAuthTimer
*
*  Generate/Start the authorization timer for timeout on reception of slave node
*  id response upon initiation of a request from master plugin.
*  NOTE: This timer instance is shared for discovery timeout as well
*  since authorization happens after a node is discovered and since timer started
*  for discovery timeout is stopped at this moment, this sharing is possible
*  \param          [in]    plugin
*  \param          [in]    delay
*
*  \pre            None
*
*  \post           None
*
*  \return         [add here]
*
******************************************************************************/
static a2b_Bool
a2b_dscvryStartCommChAuthTimer
    (
    a2b_Plugin*     plugin,
	a2b_UInt16 delay
    )
{
    a2b_TimerFunc timerFunc = &a2b_onCommChAuthTimeout;

    /* Stop the previously running timer */
    a2b_timerStop( plugin->timer );

    /* Single shot timer */
    a2b_timerSet( plugin->timer, (a2b_UInt32)delay, 0u );
    a2b_timerSetHandler(plugin->timer, timerFunc);
    a2b_timerSetData(plugin->timer, plugin);
    a2b_timerStart( plugin->timer );

    return A2B_TRUE;

} /* a2b_dscvryStartCommChAuthTimer */

/*!****************************************************************************
*
*  \b              a2b_onCommChAuthTimeout
*
*  Handle the communication channel authorization timeout.
*
*  \param          [in]    timer
*  \param          [in]    userData
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_onCommChAuthTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)userData;
    a2b_NodeSignature   nodeSig;
	a2b_Int16 			dscNodeAddr = (a2b_Int16)plugin->discovery.dscNumNodes;
	a2b_Bool bNetConfigFlag = A2B_FALSE;

	A2B_UNUSED(timer);
	A2B_INIT_SIGNATURE( &nodeSig, dscNodeAddr );

	/* Copy the signature information to the plugin */
	plugin->slaveNodeSig[dscNodeAddr] = nodeSig;

	/* End the discovery */
	A2B_DSCVRY_ERROR1( plugin->ctx, "nodeDiscovered", "Node %hd: Custom Node Id Authentication Failed : Timed out on response from slave via communication channel ", &dscNodeAddr );

	bNetConfigFlag = a2b_SimpleModeChkNodeConfig(plugin);

	if((bdd_DISCOVERY_MODE_SIMPLE == a2b_ovrGetDiscMode(plugin)) && (bNetConfigFlag))
	{
		plugin->discovery.discoveryCompleteCode = (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_TIMEOUT;
		a2b_dscvryNetComplete(plugin);
	}
	else
	{
		a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_CUSTOM_NODE_ID_TIMEOUT );
	}

	A2B_DSCVRY_SEQEND( plugin->ctx );
}
#endif
#endif	/* A2B_FEATURE_COMM_CH */

/*!****************************************************************************
*
*  \b              adi_a2b_ConfigureNodePeri
*
*  Configure node and peripheral in modified and optimized modes of discovery
*
*  \param          [in]    plugin           plugin specific data
*  \param		   [in]	   dscNodeAddr      The address of the node that has
*      			   							to be configured
*
*  \pre            None
*
*  \post           None
*
*  \return          Returns a Boolean flag of whether the node was configured
*  					successfully.
******************************************************************************/
static a2b_Bool
adi_a2b_ConfigureNodePeri(a2b_Plugin* plugin, a2b_Int16 dscNodeAddr)
{

    a2b_UInt32 errCode;

    a2b_Int32 retCode = a2b_dscvryNodeComplete( plugin, dscNodeAddr,
                                                A2B_TRUE, &errCode );
    switch (retCode)
    {
        case A2B_EXEC_COMPLETE:
            /* No EEPROM Cfg (or done), so init the plugin now */
#ifdef FIND_NODE_HANDLER_AFTER_NODE_INIT
            a2b_dscvryFindNodeHandler(plugin, (a2b_UInt16)dscNodeAddr);
#endif
            if ( A2B_HAS_PLUGIN( plugin, dscNodeAddr ) )
            {
                if ((a2b_UInt32)A2B_EC_OK != a2b_dscvryInitPlugin( plugin, dscNodeAddr,
                                    &a2b_dscvryInitPluginComplete_NoEeprom ))
                {
                    return A2B_FALSE;
                }
                return A2B_TRUE;
            }
            return A2B_FALSE;

        case A2B_EXEC_COMPLETE_FAIL:
            a2b_dscvryEnd( plugin, errCode );
            return A2B_FALSE;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
        case A2B_EXEC_SCHEDULE:
        case A2B_EXEC_SUSPEND:
            if ( a2b_periphCfgUsingSync() )
            {
                /* Node peripheral processing has not completed,
                 * processing, will resume later--would be a delay
                 * cfg block or async processing.
                 */
                return A2B_TRUE;
            }
            return A2B_FALSE;
            break;
#endif /* A2B_FEATURE_EEPROM_PROCESSING */
        default:
        	return A2B_FALSE;

    }

} /* adi_a2b_ConfigureNodePeri */

/*!****************************************************************************
*
*  \b              adi_a2b_ConfigNodeOptimizAdvancedMode
*
*  Configure node and peripheral in optimized and advanced modes of discovery
*
*  \param          [in]    plugin           plugin specific data
*  \param          [in]    dscNodeAddr      Destination Node address
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns the status of slave intialization for next node
*  				   discovery
*  				   FALSE=error
*                  TRUE=success
******************************************************************************/
static a2b_Bool
adi_a2b_ConfigNodeOptimizAdvancedMode(a2b_Plugin* plugin, a2b_Int16 dscNodeAddr)
{
    a2b_Bool bRet;

	bRet = a2b_dscvryPreSlaveInit( plugin );
	/* Configure Node registers and start plugin initialization */
	(void)adi_a2b_ConfigureNodePeri(plugin, dscNodeAddr);

	if(bdd_DISCOVERY_MODE_ADVANCED == a2b_ovrGetDiscMode(plugin))
	{
		/******************************************************
		 * Reconfigure slots for all Nodes discovered till
		 * now and enable data flow.Note: If EEPROM is present
		 * A2B registers may yet  be written so delay this
		 * till EEPROM and slave plugin initialization is complete
		 * Note: 'hasEeprom' is assigned when node is discovered
		 *****************************************************/
		if(!(A2B_HAS_EEPROM(plugin, dscNodeAddr)))
		{
			/* Re-configure Down and Up slot values for this node */
			adi_a2b_ReConfigSlot(plugin, dscNodeAddr);
			/* Enable Downstream and Upstream data flow */
			(void)a2b_FinalMasterSetup(plugin, A2B_NODEADDR_MASTER);
		}
	}

	return bRet;
}

/*!****************************************************************************
*
*  \b              a2b_dscvryReset
*
*  Reset discovery (variables, A2B, etc)
*
*  \param          [in]    plugin   plugin specific data
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_EXEC_COMPLETE == Execution is now complete
*                  A2B_EXEC_SCHEDULE == Execution is unfinished - schedule again
*                  A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                       scheduling until a later event
*
******************************************************************************/
static a2b_Int32
a2b_dscvryReset
    (
    a2b_Plugin* plugin
    )
{
    a2b_UInt8 wBuf[4];
    a2b_UInt8 rBuf[4];
    a2b_HResult status;
#if defined(A2B_FEATURE_TRACE) && defined(A2B_FEATURE_EEPROM_PROCESSING)
    a2b_UInt8 nTempVar;
#endif
#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
    a2b_Int32   mode = a2b_ovrGetDiscMode(plugin);
#endif

#ifdef A2B_FEATURE_EEPROM_PROCESSING
    (void)a2b_periphCfgPreparse( plugin );
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

    /* Unload any instantiated slave plugins */
    (void)a2b_stackFreeSlaveNodeHandler( plugin->ctx, A2B_NODEADDR_NOTUSED );

    /* Some discovery tracking variables need resetting */
    (void)a2b_memset( &plugin->discovery, 0, sizeof(a2b_PluginDiscovery) );
    plugin->discovery.inDiscovery = A2B_TRUE;

#ifdef A2B_FEATURE_EEPROM_PROCESSING
#ifndef A2B_FEATURE_COMM_CH
    if ( a2b_stackCtxMailboxCount(plugin->ctx) !=
         A2B_ARRAY_SIZE(plugin->periph.node)+1u )
#else
	if (a2b_stackCtxMailboxCount(plugin->ctx) !=
		A2B_ARRAY_SIZE(plugin->periph.node) + 2u)
#endif
    {
        a2b_UInt8 idx;

        for ( idx = 0u; idx < A2B_ARRAY_SIZE(plugin->periph.node); idx++ )
        {
#ifdef A2B_FEATURE_TRACE
            nTempVar = idx;
#endif
            /* Init some static tracking needed for timers, etc */
            plugin->periph.node[idx].nodeAddr = (a2b_Int16)idx;
            plugin->periph.node[idx].plugin   = plugin;

            /* Create the mailbox for the master plugin
             * discovery time EEPROM peripheral config
             * handling/processing.  This allows use to
             * do peripheral config in parallel to the
             * discovery of other nodes.
             */
            plugin->periph.node[idx].mboxHnd = a2b_stackCtxMailboxAlloc(
                                                                plugin->ctx,
                                                                A2B_JOB_PRIO0 );
            if ( A2B_NULL == plugin->periph.node[idx].mboxHnd )
            {
                A2B_TRACE1( (plugin->ctx,
                         (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                         "dscvryReset: failed to create peripheral mailbox %bd",
                         &nTempVar));
            }
        }
    }
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

#if defined(A2B_FEATURE_SEQ_CHART)
    A2B_SEQ_RAW1( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                  "== Starting Discovery Mode %ld ==",
                  &mode );
#endif

#if defined(A2B_FEATURE_TRACE)
    A2B_TRACE2( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG),
                "%s dscvryReset(): Starting DiscoveryMode %ld",
                A2B_MPLUGIN_PLUGIN_NAME, &mode) );
#endif

    /* Read the master node's VID/PID */
    wBuf[0] = A2B_REG_VENDOR;
    status  = a2b_i2cMasterWriteRead( plugin->ctx, 1u, wBuf, 2u, rBuf );
    if ( A2B_FAILED(status) )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s dscvryReset(): Cannot read master vid/pid",
                    A2B_MPLUGIN_PLUGIN_NAME) );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        return A2B_EXEC_COMPLETE;
    }

    /* Do a software reset on the A2B master node */
    wBuf[0] = A2B_REG_CONTROL;
    /* The AD242X (only) needs to be told it's a Master node BEFORE
     * the PLL locks on the SYNC pin. Once the PLL is locked, setting
     * the MSTR bit is ignored. We set it anyway so it's clear this is
     * the master node.
     */
    wBuf[1] = A2B_ENUM_CONTROL_RESET_PE;
    if ( a2b_isAd242xChip(rBuf[0u] /* vid */, rBuf[1u] /* pid */) )
    {
        wBuf[1] |= (a2b_UInt8)A2B_ENUM_CONTROL_MSTR;
    }
    status  = a2b_i2cMasterWrite( plugin->ctx, 2u, &wBuf );

    if ( A2B_FAILED(status) )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s dscvryReset(): Cannot reset master",
                    A2B_MPLUGIN_PLUGIN_NAME) );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        return A2B_EXEC_COMPLETE;
    }

    if (!a2b_dscvryStartTimer( plugin, TIMER_RESET ))
    {
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INTERNAL );
        return A2B_EXEC_COMPLETE;
    }

    A2B_DSCVRY_RAWDEBUG0( plugin->ctx, "dscvryReset", "...Reset Delay..." );

    return A2B_EXEC_SUSPEND;

} /* a2b_dscvryReset */


/*!****************************************************************************
*
*  \b              a2b_dscvryStart
*
*  Start the discovery process
*
*  \param          [in]    plugin           plugin specific data
*
*  \param          [in]    deinitFirst      deinit slave nodes prior
*                                           to discovery
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_EXEC_COMPLETE == Execution is now complete
*                  A2B_EXEC_SCHEDULE == Execution is unfinished - schedule again
*                  A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                       scheduling until a later event
*
******************************************************************************/
a2b_Int32
a2b_dscvryStart
    (
    a2b_Plugin* plugin,
    a2b_Bool    deinitFirst
    )
{
	bdd_DiscoveryMode eDiscMode;
    if ( a2b_ovrGetDiscCfgMethod(plugin) == bdd_CONFIG_METHOD_AUTO )
    {
        /* Currently not supported at this time */
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s dscvryStart(): AUTO Config not supported",
                     A2B_MPLUGIN_PLUGIN_NAME) );
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INVALID_PARAMETER );
        return A2B_EXEC_COMPLETE;
    }

    eDiscMode = a2b_ovrGetDiscMode(plugin);
    if ( (bdd_DISCOVERY_MODE_SIMPLE  != eDiscMode) &&
        (bdd_DISCOVERY_MODE_MODIFIED != eDiscMode) &&
		(bdd_DISCOVERY_MODE_OPTIMIZED  != eDiscMode) &&
		(bdd_DISCOVERY_MODE_ADVANCED  != eDiscMode))
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s dscvryStart(): unsupported discovery mode",
                     A2B_MPLUGIN_PLUGIN_NAME ));
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INVALID_PARAMETER );
        return A2B_EXEC_COMPLETE;
    }

    if ( plugin->discovery.inDiscovery )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s dscvryStart(): already in the process of discovery",
                     A2B_MPLUGIN_PLUGIN_NAME ));
        a2b_dscvryEnd( plugin, (a2b_UInt32)A2B_EC_INVALID_STATE );
        return A2B_EXEC_COMPLETE;
    }

    if ( plugin->bdd->nodes_count == 1u )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "%s dscvryStart(): No slave nodes",
                    A2B_MPLUGIN_PLUGIN_NAME ));
        return A2B_EXEC_COMPLETE;
    }

    if ( deinitFirst )
    {
        a2b_dscvryDeinitPlugin( plugin, A2B_DEINIT_START );
        if ( plugin->discovery.pendingPluginDeinit )
        {
            /* Pending deinit messages so wait to reset and start */
            return A2B_EXEC_SUSPEND;
        }
    }

    return a2b_dscvryReset( plugin );

} /* a2b_dscvryStart */

/*!****************************************************************************
*
*  \b              a2b_SimpleModeChkNodeConfig
*
*  Check if previously discovered nodes are to be configured
*
*  \param          [in]    plugin           plugin specific data
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_TRUE == NetConfiguration is to be done
*                  A2B_FALSE == NetConfiguration is not to be done
*
******************************************************************************/
static a2b_Bool
a2b_SimpleModeChkNodeConfig(a2b_Plugin* plugin)
{
	a2b_Bool bIsConfigReqd = A2B_FALSE;
	if((plugin->discovery.dscNumNodes !=0u) &&
			((plugin->discovery.dscNumNodes != (plugin->bdd->nodes_count)-1u)))
		{
			bIsConfigReqd = A2B_TRUE;
		}
    return bIsConfigReqd;

} /* a2b_SimpleModeChkNodeConfig */

/*!****************************************************************************
*
*  \b              adi_a2b_ReConfigSlot
*
*  This function is responsible for (re)configuring  pass down-slots and
*  pass up-slots
*
*  \param          [in]    plugin           plugin specific data
*  				   [in]	   nodeAddr			Node address of the current node
*  				   							for which the slots has to be
*  				   							reconfigured.
*
*  \pre            None
*
*  \post           None
*
*
******************************************************************************/
static void
adi_a2b_ReConfigSlot(a2b_Plugin* plugin,
		       a2b_Int16   nodeAddr)
{

    a2b_UInt8 wBuf[4];
    a2b_UInt8 nDnslots = 0u;
    a2b_UInt8 nUpslots = 0u;
    a2b_UInt8 nMaxBCDSlots = 0u;
    a2b_UInt8 nIndex = 0u;
    a2b_UInt16 nNodeIdx = (a2b_UInt16)((a2b_UInt32)nodeAddr+1u);
    a2b_HResult status;


    A2B_DSCVRY_SEQGROUP0( plugin->ctx,
                              "Reconfig Slots Registers" );


	nUpslots = (a2b_UInt8)plugin->bdd->nodes[nNodeIdx].ctrlRegs.lupslots;
	nDnslots = (a2b_UInt8)plugin->bdd->nodes[nNodeIdx].ctrlRegs.ldnslots;

	nNodeIdx--;
	for(nIndex=0u; nIndex< plugin->discovery.dscNumNodes-1u; nIndex++)
	{
		wBuf[0u] = A2B_REG_DNSLOTS;
		wBuf[1u] = nDnslots;
		status = a2b_i2cSlaveWrite(plugin->ctx, ((a2b_Int16)nNodeIdx - (a2b_Int16)1u), 2u, wBuf );

		wBuf[0u] = A2B_REG_UPSLOTS;
		wBuf[1u] = nUpslots;
		status = a2b_i2cSlaveWrite(plugin->ctx, ((a2b_Int16)nNodeIdx - (a2b_Int16)1u), 2u, wBuf );

		nUpslots += (a2b_UInt8)plugin->bdd->nodes[nNodeIdx].ctrlRegs.lupslots;
		nDnslots += (a2b_UInt8)plugin->bdd->nodes[nNodeIdx].ctrlRegs.ldnslots;

		if((plugin->bdd->nodes[nNodeIdx].ctrlRegs.has_bcdnslots) &&
				(nMaxBCDSlots < plugin->bdd->nodes[nNodeIdx].ctrlRegs.bcdnslots))
		{
		   nMaxBCDSlots = (a2b_UInt8)plugin->bdd->nodes[nNodeIdx].ctrlRegs.bcdnslots;
		}

		nNodeIdx--;
	}

		wBuf[0u] = A2B_REG_DNSLOTS;
		wBuf[1u] = nDnslots + nMaxBCDSlots;
		status  = a2b_i2cMasterWrite( plugin->ctx, 2u, &wBuf );

		wBuf[0u] = A2B_REG_UPSLOTS;
		wBuf[1u] = nUpslots;
		status  = a2b_i2cMasterWrite( plugin->ctx, 2u, &wBuf );


	A2B_DSCVRY_SEQEND(plugin->ctx);
} /* adi_a2b_ReConfigSlot */

/*!****************************************************************************
*
*  \b              a2b_isAd242xChip
*
*  This function detects whether the A2B chip is a newer AD24XX
*  series chip.
*
*  \param          [in]    vendorId         Vendor Identifier
*  				   [in]	   productId		Product Identifier
*  \pre            None
*
*  \post           None
*
*
******************************************************************************/

static a2b_Bool a2b_isAd242xChip(a2b_UInt8 vendorId, a2b_UInt8 productId)
{
	return (A2B_IS_AD242X_CHIP(vendorId, productId));
}

/*!****************************************************************************
*
*  \b              a2b_isAd242xChip
*
*  This function detects whether the stack supportes the A2B chip.
*
*  \param          [in]    vendorId         Vendor Identifier
*  				   [in]	   productId		Product Identifier
*  				   [in]	   version			Version Number
*  \pre            None
*
*  \post           None
*
*
******************************************************************************/
static a2b_Bool a2b_stackSupportedNode(a2b_UInt8 vendorId, a2b_UInt8 productId, a2b_UInt8 version)
{
	return (A2B_STACK_SUPPORTED_NODE(vendorId, productId, version));
}

/**
 @}
*/


/**
 @}
*/
