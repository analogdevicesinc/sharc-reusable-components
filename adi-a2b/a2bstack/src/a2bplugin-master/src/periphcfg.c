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
 * \file:   periphcfg.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of EEPROM peripheral config processing.
 *
 *=============================================================================
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

#ifdef A2B_FEATURE_EEPROM_PROCESSING

/*======================= D E F I N E S ===========================*/

/** Peripheral Package Constants */
#define A2B_PKG_MAX_NODES       (10u)
#define A2B_PKG_FILE_ID         (0xABu)
#define A2B_PKG_FILE_ID_IDX     (0x00u)
#define A2B_PKG_HDR_SIZE_IDX    (0x02u)

/*======================= L O C A L  P R O T O T Y P E S  =========*/

typedef enum
{
    CFGBLK_TYPE_A = 0u,
    CFGBLK_TYPE_B,
    CFGBLK_TYPE_C
} a2b_cfgBlockType;

/*======================= D A T A  ================================*/

/* Local variable used to allow overriding the value */
static a2b_UInt32 gConsecutivePeripheralCfgblocks =
                                           A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS;

/*======================= M A C R O S =============================*/

/*
 * Convenience macro to make peripheral node access.
 */
#ifdef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
#define A2B_GET_PERIPHNODE( plugin, nodeAddr ) (&(plugin)->periph.node[0])
#else
#define A2B_GET_PERIPHNODE( plugin, nodeAddr ) (&(plugin)->periph.node[(nodeAddr)])
#endif

/* Setup the error */
#ifdef _TESSY_NO_DOWHILE_MACROS_
#define A2B_DSCVRY_SETERROR( plugin, errCode ) \
	{ \
    (plugin)->discovery.discoveryComplete = A2B_TRUE; \
    (plugin)->discovery.discoveryCompleteCode = (errCode); \
	}
#else /* _TESSY_NO_DOWHILE_MACROS_ */
#define A2B_DSCVRY_SETERROR( plugin, errCode ) \
	do{ \
    (plugin)->discovery.discoveryComplete = A2B_TRUE; \
    (plugin)->discovery.discoveryCompleteCode = (a2b_UInt32)(errCode); \
	}while(0)
#endif /* _TESSY_NO_DOWHILE_MACROS_ */
/*======================= C O D E =================================*/

static void a2b_onPeripheralProcessingComplete( struct a2b_Msg* msg,
                                                a2b_Bool isCancelled );
static void a2b_onPeripheralProcessingDestroy( struct a2b_Msg* msg );

static void a2b_periMsgDestroy( a2b_Plugin*     plugin,
                                a2b_Int16       nodeAddr );

/*!****************************************************************************
*
*  \b              a2b_periphCfgPreparse
*
*  This will pre-parse EEPROM peripheral package configuration.  This will:
*  - verify the package header and CRC
*  - verify the length of the package versus the node cfg contained
*  - cache offsets and lengths to each node
*
*  \param          [in]    plugin
*
*  \pre            None
*
*  \post           None
*
* \return  A status code that can be checked with the A2B_SUCCEEDED() or
*          A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
a2b_HResult
a2b_periphCfgPreparse(
    a2b_Plugin*     plugin
    )
{
    a2b_UInt8 idx;
#ifdef A2B_FEATURE_TRACE
    a2b_UInt8 nTempVar;
#endif
    a2b_UInt32 hdrOffset;
    a2b_UInt32 offset;
    a2b_UInt32 nodeSize;
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                          A2B_EC_RESOURCE_UNAVAIL);

    if  ( (bdd_CONFIG_METHOD_HYBRID == a2b_ovrGetDiscCfgMethod(plugin)) ||
         (A2B_NULL == plugin->periphCfg.pkgCfg) )
    {
        return status;
    }

    if ( A2B_PKG_FILE_ID != plugin->periphCfg.pkgCfg[A2B_PKG_FILE_ID_IDX] )
    {
        A2B_TRACE1 ((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                     "%s a2b_periphCfgPreparse(): Bad file ID",
                     A2B_MPLUGIN_PLUGIN_NAME ));

        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                A2B_EC_INVALID_PARAMETER);
    }

    /* Process any overrides that are needed locally */
    gConsecutivePeripheralCfgblocks = A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS;
    if ( plugin->overrides[0] & A2B_MPLUGIN_FORCE_ZERO_CONSECUTIVE_CFGBLOCKS )
    {
        gConsecutivePeripheralCfgblocks =
                                (A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS) ? 0u : 2u;
    }

    /* Make sure we at least have a header */
    if ( plugin->periphCfg.pkgLen < (a2b_UInt32)
         plugin->periphCfg.pkgCfg[A2B_PKG_HDR_SIZE_IDX] )
    {
        A2B_TRACE1 ((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                     "%s a2b_periphCfgPreparse(): File too small",
                     A2B_MPLUGIN_PLUGIN_NAME ));

        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                A2B_EC_INVALID_PARAMETER);
    }

    /* Verify the header CRC */
    offset = (a2b_UInt32)plugin->periphCfg.pkgCfg[A2B_PKG_HDR_SIZE_IDX];
    if ( a2b_crc8( plugin->periphCfg.pkgCfg, 0u, (a2b_UInt32)(offset-1u)) !=
         (a2b_UInt8)plugin->periphCfg.pkgCfg[offset-1u] )
    {
        A2B_TRACE1 ((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                     "%s a2b_periphCfgPreparse(): Bad header CRC",
                     A2B_MPLUGIN_PLUGIN_NAME ));

        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                A2B_EC_INVALID_PARAMETER);
    }

    /* Setup offsets to each A2B cfg */
    hdrOffset = A2B_PKG_HDR_SIZE_IDX+1u;
    for (idx = 0u; idx < A2B_PKG_MAX_NODES; idx++)
    {
#ifdef A2B_FEATURE_TRACE
        nTempVar = idx;
#endif
        nodeSize =  (a2b_UInt32)plugin->periphCfg.pkgCfg[hdrOffset] << 24u;
        hdrOffset++;
        nodeSize |= (a2b_UInt32)plugin->periphCfg.pkgCfg[hdrOffset] << 16u;
        hdrOffset++;
        nodeSize |= (a2b_UInt32)plugin->periphCfg.pkgCfg[hdrOffset] << 8u;
        hdrOffset++;
        nodeSize |= (a2b_UInt32)plugin->periphCfg.pkgCfg[hdrOffset];
        hdrOffset++;

        plugin->periphCfg.nodeLen[idx] = nodeSize;

        A2B_TRACE3 ((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "%s a2b_periphCfgPreparse(): EEPROM %bd Cfg Size: %d",
                     A2B_MPLUGIN_PLUGIN_NAME, &nTempVar, &nodeSize ));

        if ( nodeSize )
        {
            plugin->periphCfg.nodeCfg[idx] = &plugin->periphCfg.pkgCfg[offset];
        }
        else
        {
            plugin->periphCfg.nodeCfg[idx] = A2B_NULL;
        }

        offset += nodeSize;

        /* May have configured the system for fewer nodes
         * than that found in the headers.
         */
        if ( idx >=  A2B_CONF_MAX_NUM_SLAVE_NODES)
        {
            break;
        }
    }

    /* Check the length of the file is as expected */
    if ( plugin->periphCfg.pkgLen != offset )
    {
        A2B_TRACE1 ((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                     "%s a2b_periphCfgPreparse(): File size mismatch",
                     A2B_MPLUGIN_PLUGIN_NAME ));

        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                A2B_EC_INVALID_PARAMETER);
    }

    return status;

} /* a2b_periphCfgPreparse */


/*!****************************************************************************
*
*  \b              a2b_periphCfgWriteRead
*
*  Depending on the cfg method this method will either read from the
*  EEPROM to get peripheral config (HYBRID mode), or, read a potential
*  EEPROM pkg pass in from the BDD (BDD mode).
*
* \param   [in]        plugin  The stack context associated with the
*                              write/read.
*
* \param   [in]        nodeAddr     The slave node to write/read.
*
* \param   [in]        nWrite  The number of bytes in the 'wBuf' buffer to
*                              write to the peripheral.
*
* \param   [in]        wBuf    A buffer containing the data to write. The
*                              amount of data to write is specified by the
*                              'nWrite' parameter.
*
* \param   [in]        nRead   The number of bytes to read from the
*                              peripheral. The 'rBuf' parameter must have
*                              enough space to hold this number of bytes.
*
* \param   [in,out]    rBuf    The buffer in which to write the results of
*                              the read. It's assumed the buffer is sized to
*                              accept 'nRead' bytes of data.
*
* \pre     None
*
* \post    On success 'rBuf' holds the data that was read from the
*          peripheral.
*
* \return  A status code that can be checked with the A2B_SUCCEEDED() or
*          A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
a2b_HResult
a2b_periphCfgWriteRead
    (
    a2b_Plugin*     plugin,
    a2b_Int16       nodeAddr,
    a2b_UInt16      nWrite,
    void*           wBuf,
    a2b_UInt16      nRead,
    void*           rBuf
    )
{
    a2b_UInt32 offset;
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                          A2B_EC_RESOURCE_UNAVAIL);

    if (( nodeAddr < 0 ) || ( nodeAddr >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES))
    {
        return status;
    }

    if ( bdd_CONFIG_METHOD_HYBRID == a2b_ovrGetDiscCfgMethod(plugin) )
    {
#ifdef A2B_BCF_FROM_SOC_EEPROM

        /* Reading from EEPROM connected to SOC */
        status = a2b_i2cPeriphWriteRead( plugin->ctx,
        								 A2B_NODEADDR_MASTER,
                                         A2B_I2C_EEPROM_ADDR,
                                         nWrite, wBuf,
                                         nRead,  rBuf );

#else
        /* HYBRID mode will read from the EEPROM connected to slave node */
        status = a2b_i2cPeriphWriteRead( plugin->ctx,
                                         nodeAddr,
                                         A2B_I2C_EEPROM_ADDR,
                                         nWrite, wBuf,
                                         nRead,  rBuf );
#endif
    }
    else if ( plugin->periphCfg.nodeCfg[nodeAddr] )
    {
        /* BDD mode will require the EEPROM peripheral pkg
         * (if one has been passed in)
         */
        a2b_UInt32 rIdx = 0u;
        a2b_UInt8* wBuf8 = (a2b_UInt8*)wBuf;
        a2b_UInt8* rBuf8 = (a2b_UInt8*)rBuf;
        a2b_UInt16 addr = ((a2b_UInt16)((a2b_UInt16)wBuf8[0] << (a2b_UInt16)8u)) | (a2b_UInt16)wBuf8[1];
        const a2b_UInt8* pPeriph = plugin->periphCfg.nodeCfg[nodeAddr];

        for ( offset = (a2b_UInt32)addr; offset < ((a2b_UInt32)addr + (a2b_UInt32)nRead); offset++ )
        {
            if ( offset < plugin->periphCfg.nodeLen[nodeAddr] )
            {
                rBuf8[rIdx] = pPeriph[offset];
            }
            else
            {
                /* We have exceeded this nodes image */
                return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                        A2B_EC_I2C_DATA_NACK);
            }
            rIdx++;
        }

        status = A2B_RESULT_SUCCESS;
    }
    else
    {
        /* else, no peripheral config mode for BDD mode */
    }

    return status;

} /* a2b_periphCfgWriteRead */



/*!****************************************************************************
*
*  \b              a2b_periphCfgUsingSync
*
*  Used to determine the mode we are processing the peripheral configuration.
*
*  \param          None
*
*  \pre            None
*
*  \post           None
*
*  \return         TRUE = Synchronous peripheral config mode, else ASYNC
*
******************************************************************************/
a2b_Bool
a2b_periphCfgUsingSync(void)
{
    return ((a2b_Bool)(gConsecutivePeripheralCfgblocks == 0u));

} /* a2b_periphCfgUsingSync */


/*!****************************************************************************
*
*  \b              a2b_onPeripheralDelayTimeout
*
*  Handler when a peripheral delay has timed out
*
*  \param          [in]    timer        N/A
*  \param          [in]    userData     a2b_Peripheral
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
void
a2b_onPeripheralDelayTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    )
{
    a2b_PeripheralNode* periphNode = (a2b_PeripheralNode*)userData;
    a2b_Plugin* plugin = (a2b_Plugin*)periphNode->plugin;
    a2b_Int32 ret;
    a2b_Int16 nodeAddr = periphNode->nodeAddr;

    A2B_UNUSED( timer );

    ret = a2b_periphCfgProcessing( plugin, nodeAddr );

    /* if delay is last we need to complete the message execution */

    /* For the timer it must have been suspended,
     * so change the state of the message.
     */
    if ( gConsecutivePeripheralCfgblocks != 0u )
    {
        #ifdef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
        a2b_msgRtrExecUpdate( plugin->ctx,
                              plugin->periph.node[A2B_PERIPH_MAILBOX].mboxHnd,
                              ret );
        #else
        a2b_msgRtrExecUpdate( plugin->ctx,
                              plugin->periph.node[nodeAddr].mboxHnd, ret );
        #endif
    }
    else if ( A2B_EXEC_COMPLETE == ret )
    {
        /* We are executing peripheral config synchronously and
         * have completed this nodes peripheral, move on the next.
         */
    	a2b_periMsgDestroy(plugin, nodeAddr);

    }
    else
    {
        /* Completing the control statement */
    }

} /* a2b_onPeripheralDelayTimeout */


/*!****************************************************************************
*
*  \b              a2b_onPeripheralProcessingComplete
*
*  Handler when peripheral processing is complete.
*
*  \param          [in]    msg          Response message for whether peripheral
*                                       processing is complete.
*
*  \param          [in]    isCancelled  An indication of whether the request
*                                       was cancelled before completing.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_onPeripheralProcessingComplete
    (
    struct a2b_Msg* msg,
    a2b_Bool        isCancelled
    )
{
    a2b_Plugin* plugin   = (a2b_Plugin*)a2b_msgGetUserData( msg );
    a2b_UInt32  nodeAddr = a2b_msgGetTid( msg );
    struct a2b_Timer* tmpTimer;
    a2b_PeripheralNode* periphNode;

    A2B_UNUSED( isCancelled );

    if (plugin)
    {
        periphNode = A2B_GET_PERIPHNODE( plugin, nodeAddr);

        A2B_DSCVRYNOTE_DEBUG3( plugin->ctx, "onPeripheralProcessingComplete",
                               "%bd of %bd Node %hd Peripheral Cfg Blocks Completed",
                               &periphNode->cfgIdx,
                               &periphNode->nCfgBlocks,
                               &nodeAddr );

        /* Show that this peripheral processing is done */
        plugin->discovery.hasEeprom ^= (a2b_UInt32)((a2b_UInt32)1u << nodeAddr); /* clear the bit */

        a2b_timerStop( periphNode->timer );

        /* Reset the peripheral tracking, but keep the timer */
        tmpTimer = periphNode->timer;
        (void)a2b_memset( periphNode, 0, sizeof(periphNode) );
        periphNode->timer = tmpTimer;
        periphNode->plugin = plugin;

        a2b_dscvryPeripheralProcessingComplete( plugin, (a2b_Int16)nodeAddr );
    }

} /* a2b_onPeripheralProcessingComplete */



/*!****************************************************************************
*
*  \b              a2b_periMsgDestroy
*
*  Handler called to destroy user data for peripheral processing.
*
*  \param          [in]    plugin          Message with user data to destroy.
*  \param          [in]    nodeAddr        Node address
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_periMsgDestroy
    (
    	    a2b_Plugin*     plugin,
			a2b_Int16       nodeAddr
    )
{
    struct a2b_Msg*     msg;

    msg = a2b_msgAlloc( plugin->ctx,
                        A2B_MSG_REQUEST,
                        A2B_MPLUGIN_CONT_PERIPH_CFG );
    if ( msg )
    {
        /* MUST manually trigger the call to complete processing.
         * Setting the userdata destroy to
         * a2b_onPeripheralProcessingComplete means it will get
         * call automatically for us on the a2b_msgUnref call.
         */

        a2b_msgSetUserData( msg, (a2b_Handle)plugin,
                            &a2b_onPeripheralProcessingDestroy );
        a2b_msgSetTid( msg, (a2b_UInt32)nodeAddr );

        (void)a2b_msgUnref( msg );
    }
    else
    {
        A2B_DSCVRY_ERROR0( plugin->ctx, "a2b_periMsgDestroy",
                           "Failed to create message to process next node" );
        A2B_DSCVRY_SETERROR( plugin, A2B_EC_ALLOC_FAILURE );
    }
} /* a2b_onPeripheralProcessingDestroy */


/*!****************************************************************************
*
*  \b              a2b_onPeripheralProcessingDestroy
*
*  Handler called to destroy user data for peripheral processing.
*
*  \param          [in]    msg          Message with user data to destroy.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_onPeripheralProcessingDestroy
    (
    struct a2b_Msg* msg
    )
{
    /* Handle in the same way as processing the completion of a request */
    a2b_onPeripheralProcessingComplete(msg, A2B_FALSE);
} /* a2b_onPeripheralProcessingDestroy */


/*!****************************************************************************
*
*  \b              a2b_periphCfgInitProcessing
*
*  Initial EEPROM processing to decide if we need to later process the
*  EEPROM asynchronously.
*
*  \param          [in]    plugin
*  \param          [in]    nodeAddr     node adddress [0 (slave0)..(n-1)]
*
*  \pre            The EEPROM has been identified and is in an ADI format
*  \pre            The EEPROM header CRC has been verified
*
*  \post           None
*
*  \return         A2B_EXEC_COMPLETE == Execution is now complete (success)
*                  A2B_EXEC_COMPLETE_FAIL == Execution is now complete (failed)
*                  A2B_EXEC_SCHEDULE == Execution is unfinished - schedule again
*                  A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                       scheduling until a later event
*
******************************************************************************/
a2b_Int32
a2b_periphCfgInitProcessing
    (
    a2b_Plugin* plugin,
    a2b_Int16   nodeAddr
    )
{
    a2b_UInt8 nCfgBlocks = 0u;
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_UInt8 wBuf[2];
    a2b_Int32 retCode = A2B_EXEC_COMPLETE;
#ifdef A2B_BCF_FROM_SOC_EEPROM
    a2b_UInt16 nOffset;
#endif

    if ( bdd_CONFIG_METHOD_AUTO == a2b_ovrGetDiscCfgMethod(plugin) )
    {
        /* Currently not supported [Not using BDD, only uses the EEPROM] */
        A2B_SEQ_GENERROR0( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                           "No Support for AUTO Config Mode" );
        return A2B_EXEC_COMPLETE;
    }

    A2B_DSCVRY_SEQGROUP1( plugin->ctx,
                          "periphCfgInitProcessing for nodeAddr %hd",
                          &nodeAddr );
#ifdef A2B_BCF_FROM_SOC_EEPROM
    /* To read the number of EEPROM */
    A2B_GET_UINT16_BE(nOffset, plugin->periphCfg.pkgCfg , 2* nodeAddr);
    nOffset += 3u;
    A2B_PUT_UINT16_BE(nOffset, wBuf,0);
#else
    /* Read the number of config blocks in the EEPROM */
    /* [Two byte internal EEPROM address] */
    wBuf[0] = 0u;
    wBuf[1] = 5u;
#endif
    status  = a2b_periphCfgWriteRead( plugin,
                                      nodeAddr,
                                      2u,  wBuf,
                                      1u,  &nCfgBlocks );
    if ( A2B_FAILED(status) )
    {
        A2B_DSCVRY_ERROR0( plugin->ctx, "periphCfgInitProcessing",
                           "Failed to read EEPROM Num Cfg Blocks" );

        A2B_DSCVRY_SEQEND( plugin->ctx );
        A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
        return A2B_EXEC_COMPLETE_FAIL;
    }

    if ( nCfgBlocks == 0u )
    {
#ifndef A2B_BCF_FROM_SOC_EEPROM
        A2B_DSCVRY_WARN0( plugin->ctx, "periphCfgInitProcessing",
                          "EEPROM Cfg Blocks count is Zero" );

        A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
#endif

		/* We are executing peripheral config synchronously and
		 * have completed this nodes peripheral, move on the next.
		 */
    	a2b_periMsgDestroy(plugin, nodeAddr);

        A2B_DSCVRY_SEQEND( plugin->ctx );

        return A2B_EXEC_COMPLETE;
    }

    A2B_DSCVRYNOTE_DEBUG1( plugin->ctx, "periphCfgInitProcessing",
                           "%bd Cfg Blocks Available",
                           &nCfgBlocks );

    if ( gConsecutivePeripheralCfgblocks != 0u )
    {
        /* Parallel peripheral process during discovery */

        struct a2b_Msg*             msg;
        a2b_UInt32*                 payload;

        /* We will complete the original command later */
        retCode = A2B_EXEC_SUSPEND;

        msg = a2b_msgAlloc( plugin->ctx,
                            A2B_MSG_REQUEST,
                            A2B_MPLUGIN_START_PERIPH_CFG );
        if ( msg )
        {
            a2b_msgSetUserData( msg, (a2b_Handle)plugin, A2B_NULL );

            /* Set the start data */
            payload = (a2b_UInt32*)a2b_msgGetPayload( msg );
            payload[0] = (a2b_UInt32)nCfgBlocks;

            /* Store the nodeAddr in the TID */
            a2b_msgSetTid(msg, (a2b_UInt32)nodeAddr);

            #ifdef A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY
            status = a2b_msgRtrSendRequestToMailbox( msg,
                               plugin->periph.node[A2B_PERIPH_MAILBOX].mboxHnd,
                               &a2b_onPeripheralProcessingComplete );
            #else
            status = a2b_msgRtrSendRequestToMailbox( msg,
                                       plugin->periph.node[nodeAddr].mboxHnd,
                                       a2b_onPeripheralProcessingComplete );
            #endif
            if ( A2B_FAILED(status) )
            {
                retCode = A2B_EXEC_COMPLETE_FAIL;
                A2B_DSCVRY_ERROR0( plugin->ctx, "periphCfgInitProcessing",
                                   "Failed to send message to process EEPROM" );
                A2B_DSCVRY_SETERROR( plugin, status );
            }
            else
            {
                /* Job executor now owns the message */
                (void)a2b_msgUnref( msg );
            }
        }
        else
        {
            retCode = A2B_EXEC_COMPLETE_FAIL;
            A2B_DSCVRY_ERROR0( plugin->ctx, "periphCfgInitProcessing",
                               "Failed to create message to process EEPROM" );
            A2B_DSCVRY_SETERROR( plugin, A2B_EC_ALLOC_FAILURE );
        }
    }
    else
    {
        a2b_PeripheralNode* periphNode = A2B_GET_PERIPHNODE( plugin, nodeAddr );

#ifdef A2B_BCF_FROM_SOC_EEPROM

        A2B_GET_UINT16_BE(periphNode->addr, plugin->periphCfg.pkgCfg, 2u* nodeAddr);
        /* 4 bytes from marker */
        periphNode->addr 	  += 4u;
        periphNode->cfgIdx     = 0u;
        periphNode->nodeAddr   = nodeAddr;
        periphNode->nCfgBlocks = nCfgBlocks;

#else
        periphNode->addr       = 0x0008u;
        periphNode->cfgIdx     = 0u;
        periphNode->nodeAddr   = nodeAddr;
        periphNode->nCfgBlocks = nCfgBlocks;
#endif


        /* Process all peripheral configuration blocks in synchronous */
        retCode = a2b_periphCfgProcessing( plugin, nodeAddr );
		if ( A2B_EXEC_COMPLETE == retCode )
		{
			/* We are executing peripheral config synchronously and
			 * have completed this nodes peripheral, move on the next.
			 */
	    	a2b_periMsgDestroy(plugin, nodeAddr);
		}
    }

    A2B_DSCVRY_SEQEND( plugin->ctx );

    return retCode;

} /* a2b_periphCfgInitProcessing */


/*!****************************************************************************
*
*  \b              a2b_periphCfgStartProcessing
*
*  [add description here]
*
*  \param          [in]    plugin
*  \param          [in]    nodeAddr     node adddress [0 (slave0)..(n-1)]
*
*  \pre            The EEPROM has been identified and is in an ADI format
*  \pre            The EEPROM header CRC has been verified
*
*  \post           None
*
*  \return         A2B_EXEC_COMPLETE == Execution is now complete (success)
*                  A2B_EXEC_COMPLETE_FAIL == Execution is now complete (failed)
*                  A2B_EXEC_SCHEDULE == Execution is unfinished - schedule again
*                  A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                       scheduling until a later event
*
******************************************************************************/
a2b_Int32
a2b_periphCfgStartProcessing
    (
    struct a2b_Msg* msg
    )
{
    a2b_Int32 retCode = A2B_EXEC_COMPLETE;

    a2b_Plugin* plugin   = (a2b_Plugin*)a2b_msgGetUserData( msg );
    a2b_UInt32  nodeAddr = a2b_msgGetTid( msg );
    a2b_UInt32* payload = (a2b_UInt32*)a2b_msgGetPayload( msg );
    a2b_UInt8 nCfgBlocks = 0u;
    a2b_PeripheralNode* periphNode;

    if ( payload )
    {
        nCfgBlocks = (a2b_UInt8)payload[0];
    }

    A2B_DSCVRY_SEQGROUP1( plugin->ctx,
                          "periphCfgStartProcessing for nodeAddr %hd",
                          &nodeAddr );

    if ( nCfgBlocks == 0u )
    {
        A2B_DSCVRY_WARN0( plugin->ctx, "periphCfgStartProcessing",
                          "EEPROM Cfg Blocks is Zero" );
        A2B_DSCVRY_SEQEND( plugin->ctx );
        return A2B_EXEC_COMPLETE;
    }

    A2B_DSCVRYNOTE_DEBUG1( plugin->ctx, "periphCfgStartProcessing",
                           "Start Processing %bd Cfg Blocks",
                           &nCfgBlocks );

    periphNode = A2B_GET_PERIPHNODE( plugin, nodeAddr );

#ifdef A2B_BCF_FROM_SOC_EEPROM

        A2B_GET_UINT16_BE(periphNode->addr, plugin->periphCfg.pkgCfg, 2u* nodeAddr);
        /* 4 bytes from marker */
        periphNode->addr 	  += 4u;
        periphNode->cfgIdx     = 0u;
        periphNode->nodeAddr   = nodeAddr;
        periphNode->nCfgBlocks = nCfgBlocks;

#else
		periphNode->addr       = 0x0008u;
		periphNode->cfgIdx     = 0u;
		periphNode->nodeAddr   = (a2b_Int16)nodeAddr;
		periphNode->nCfgBlocks = nCfgBlocks;
#endif
    /* Change so the execution flow will change */
    (void)a2b_msgSetCmd( msg, A2B_MPLUGIN_CONT_PERIPH_CFG );

    /* Begin processing all peripheral configuration blocks */
    retCode = a2b_periphCfgProcessing( plugin, (a2b_Int16)nodeAddr );

    A2B_DSCVRY_SEQEND( plugin->ctx );

    return retCode;

} /* a2b_periphCfgStartProcessing */


/*!****************************************************************************
*
*  \b              a2b_periphCfgProcessing
*
*  [add description here]
*
*  \param          [in]    plugin
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
a2b_periphCfgProcessing
    (
    a2b_Plugin* plugin,
    a2b_Int16 nodeAddr
    )
{
    a2b_HResult status = A2B_RESULT_SUCCESS;
    a2b_UInt8 wBuf[2];
    a2b_UInt8 crc8;

    a2b_Bool bA2bReg = A2B_FALSE;
    a2b_UInt8 cfgType;
    a2b_UInt8 cfgCrc;
    a2b_UInt16 numExec;
    a2b_UInt16 payloadLen,payloadDataLen;
    a2b_PeripheralNode* periphNode;
    a2b_UInt8 regAddr;

    periphNode = A2B_GET_PERIPHNODE( plugin, nodeAddr);

    A2B_DSCVRY_SEQGROUP1( plugin->ctx,
                          "periphCfgProcessing for nodeAddr %hd",
                          &nodeAddr );

    for (numExec = 0u;
         periphNode->cfgIdx < periphNode->nCfgBlocks;
         periphNode->cfgIdx++)
    {
        A2B_DSCVRY_SEQGROUP2( plugin->ctx,
                          "nodeAddr %hd, cfgBlock %bd",
                          &nodeAddr,
                          &periphNode->cfgIdx);

        /* Read the config block header bytes */
        /* [Two byte internal EEPROM address] */
        wBuf[0] = (a2b_UInt8)(periphNode->addr >> 8u);
        wBuf[1] = (a2b_UInt8)(periphNode->addr & 0xFFu);
        status  = a2b_periphCfgWriteRead( plugin,
                                          nodeAddr,
                                          2u,  wBuf,
                                          3u,  plugin->periph.rBuf );
        if ( A2B_FAILED(status) )
        {
            A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
                               "Failed to read EEPROM Cfg Blocks %bd Header",
                               &periphNode->cfgIdx );
            A2B_DSCVRY_SEQEND( plugin->ctx );
            A2B_DSCVRY_SEQEND( plugin->ctx );
            A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
            return A2B_EXEC_COMPLETE;
        }
        periphNode->addr += 3u;

        A2B_TRACE6( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG),
                    "node: %hd cfg[%bd]:[%04hX] hdr:[%02bX,%02bX,%02bX]",
                    &nodeAddr,               &periphNode->cfgIdx,
                    &periphNode->addr,       &plugin->periph.rBuf[0],
                    &plugin->periph.rBuf[1], &plugin->periph.rBuf[2] ) );

        cfgType = (plugin->periph.rBuf[0] >> 4u);
        cfgCrc  = plugin->periph.rBuf[2];

        payloadLen = (a2b_UInt16)((a2b_UInt16)(((a2b_UInt16)plugin->periph.rBuf[0]) << (a2b_UInt16)8u) |
                       ((a2b_UInt16)plugin->periph.rBuf[1])) & (a2b_UInt16)0xFFFu;
        payloadDataLen=payloadLen;
        /* Read the payload if needed */
        if ( (a2b_UInt8)CFGBLK_TYPE_C == cfgType )
        {
            crc8 = a2b_crc8(plugin->periph.rBuf, 0u, 2u);

            /* Verify the CRC */
            if ( cfgCrc != crc8)
            {
                A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
                                   "Bad Type C Cfg Block %bd CRC",
                                   &periphNode->cfgIdx );
                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
                return A2B_EXEC_COMPLETE;
            }

            A2B_SEQ_GENNOTE1( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                              "Type C: Delay %hd", &payloadLen );

            /* Set a timer for the delay */
            /*---------------------------*/
            if ( A2B_NULL == periphNode->timer )
            {
                periphNode->timer = a2b_timerAlloc( plugin->ctx,
                                                    A2B_NULL, periphNode );

                a2b_timerSetHandler( periphNode->timer,
                                     &a2b_onPeripheralDelayTimeout );
            }

            if ( periphNode->timer )
            {
                /* Single shot timer */
                a2b_timerSet( periphNode->timer, (a2b_UInt32)payloadLen, 0u );

                /* MUST be set here in case of
                 * A2B_FEATURE_WAIT_ON_PERIPHERAL_CFG_DELAY because
                 * it reuses the same periphNode tracking.
                 */
                periphNode->nodeAddr = nodeAddr;

                a2b_timerSetData( periphNode->timer, periphNode );

                a2b_timerStart( periphNode->timer );

                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SEQEND( plugin->ctx );
                periphNode->cfgIdx++;
                A2B_DSCVRY_RAWDEBUG1( plugin->ctx,
                                      "periphCfgProcessing",
                                      "...%hd msec Peripheral Delay...",
                                      &payloadLen );
                return A2B_EXEC_SUSPEND;
            }
            else
            {
                /* Bail out since we couldn't allocate a timer */
                A2B_DSCVRY_ERROR0( plugin->ctx, "periphCfgProcessing",
                                   "Failed to allocate a timer" );
                A2B_DSCVRY_SETERROR( plugin, A2B_EC_ALLOC_FAILURE );
            }
        }
        else
        {
            /* The cfgCrc is byte[2] which for this message
             * it equates to the addr/reg
             */
            regAddr = cfgCrc;

            if ( payloadLen > A2B_MAX_PERIPHERAL_BUFFER_SIZE )
            {
                A2B_DSCVRY_ERROR2( plugin->ctx, "periphCfgProcessing",
                       "EEPROM Cfg Block %bd Payload (%hd) Exceeds the Buffer",
                       &periphNode->cfgIdx,
                       &payloadLen );

                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
                return A2B_EXEC_COMPLETE;
            }

            if ( (a2b_UInt8)CFGBLK_TYPE_B == cfgType )
            {
                /* Determine the CRC for the portion of the cfg block
                 * that we have read thus far.  We'll complete the CRC
                 * calculation after the payload has been read.
                 */
                crc8 = a2b_crc8(plugin->periph.rBuf, 0u, 3u);
            }

            /* Read the payload */

            wBuf[0] = (a2b_UInt8)(periphNode->addr >> 8u);
            wBuf[1] = (a2b_UInt8)(periphNode->addr & 0xFFu);
            status  = a2b_periphCfgWriteRead( plugin,
                                              nodeAddr,
                                              2u, wBuf,
                                              payloadLen,
                                              plugin->periph.rBuf );
            if ( A2B_FAILED(status) )
            {
                A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
                                   "Failed to read EEPROM Cfg Block %bd Header",
                                   &periphNode->cfgIdx );
                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SEQEND( plugin->ctx );
                A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
                return A2B_EXEC_COMPLETE;
            }

            if ( (a2b_UInt8)CFGBLK_TYPE_B == cfgType )
            {
                /* Complete the CRC calculation */
                crc8 = a2b_crc8Cont(plugin->periph.rBuf, crc8, 0u,
                                    ((a2b_UInt32)payloadLen-1u));

                if ( crc8 != plugin->periph.rBuf[payloadLen-1u] )
                {
                    A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
                                       "Bad Type B Cfg Block %bd CRC",
                                       &periphNode->cfgIdx );
                    A2B_DSCVRY_SEQEND( plugin->ctx );
                    A2B_DSCVRY_SEQEND( plugin->ctx );
                    A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
                    return A2B_EXEC_COMPLETE;
                }
                /* Actual payload to be written is excluding the CRC byte */
                payloadDataLen=payloadDataLen-1u;
            }

            bA2bReg = A2B_FALSE;/* Reset for synchornous case */
            if ( regAddr == 0x00u )
            {
                bA2bReg = A2B_TRUE;
                regAddr = plugin->periph.rBuf[0];
            }

#ifdef A2B_FEATURE_SEQ_CHART
            if ( A2B_SEQ_CHART_ENABLED( plugin->ctx,
                                        A2B_SEQ_CHART_LEVEL_DISCOVERY ) )
            {
                const a2b_Char* cType = "Type A";
                const a2b_Char* cInfo = "I2C Addr";

                if ( CFGBLK_TYPE_B == cfgType )
                {
                    cType = "Type B";
                }

                if ( bA2bReg )
                {
                    cInfo = "A2B Reg";
                }

                A2B_SEQ_GENNOTE4( plugin->ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY,
                                  "%s: %s: 0x%02bX len: %hd",
                                  cType, cInfo, &regAddr,
                                  &payloadLen );
            }
#endif
            if ( !bA2bReg )
            {

                /* Write to the peripheral */
                status = a2b_i2cPeriphWrite( plugin->ctx,
                                             nodeAddr,
                                             (a2b_UInt16)regAddr,
											 payloadDataLen,
                                             &plugin->periph.rBuf );
                if ( A2B_FAILED(status) )
                {
                    if ( plugin->overrides[0] & A2B_MPLUGIN_IGN_PERIPH_ERR )
                    {
                        /* We are going to ignore this error and
                           continue processing */
                        A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
                               "Override Set, Ignoring EEPROM Cfg Block %bd Error",
                               &periphNode->cfgIdx );
                    }
                    else
                    {
                        A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
                               "EEPROM Cfg Block %bd Peripheral Write Error",
                               &periphNode->cfgIdx );

                        A2B_DSCVRY_SEQEND( plugin->ctx );
                        A2B_DSCVRY_SEQEND( plugin->ctx );
                        A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
                        return A2B_EXEC_COMPLETE;
                    }
                }
            }
            else
            {
                /* Write to the A2B Slave */
              	status=a2b_i2cSlaveWrite(plugin->ctx,
										nodeAddr,
										payloadDataLen,
										plugin->periph.rBuf);
            	if ( A2B_FAILED(status) )
                {
            		A2B_DSCVRY_ERROR1( plugin->ctx, "periphCfgProcessing",
            		                               "EEPROM Cfg Block %bd Slave Write Error",
            		                               &periphNode->cfgIdx );

					A2B_DSCVRY_SEQEND( plugin->ctx );
					A2B_DSCVRY_SEQEND( plugin->ctx );
					A2B_DSCVRY_SETERROR( plugin, A2B_EC_IO );
					return A2B_EXEC_COMPLETE;
                }
            }

            periphNode->addr += payloadLen;
        }

        A2B_DSCVRY_SEQEND( plugin->ctx );

        /* See if we need to take a break to let others do some work */
        numExec++;
        if (( gConsecutivePeripheralCfgblocks != 0u ) &&
            ( numExec == gConsecutivePeripheralCfgblocks ))
        {
            periphNode->cfgIdx++;
            break;
        }
    }

    A2B_DSCVRY_SEQEND( plugin->ctx );

    if ( periphNode->cfgIdx >= periphNode->nCfgBlocks )
    {
        return A2B_EXEC_COMPLETE;
    }

    /* Schedule this on the next tick */
    return A2B_EXEC_SCHEDULE;

} /* a2b_periphCfgProcessing */


#endif /* A2B_FEATURE_EEPROM_PROCESSING */







