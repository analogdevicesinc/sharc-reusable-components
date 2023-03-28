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
 * \file:   pwrdiag.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the A2B power diagnostics.
 *
 *=============================================================================
 */
/*! \addtogroup Network_Configuration
 *  @{
 */

/** @defgroup Power_Diagnostics Power Diagnostics
 *
 * This module detects and configures the netwok for Localized and Non-Localized Line faults.
 *
 */

/*! \addtogroup Power_Diagnostics
 *  @{
 */
/*======================= I N C L U D E S =========================*/

#include "pwrdiag.h"
#include "a2b/regdefs.h"
#include "a2b/i2c.h"
#include "a2b/timer.h"
#include "a2b/msg.h"
#include "a2b/msgrtr.h"
#include "a2b/error.h"
#include "a2b/trace.h"
#include "a2b/interrupt.h"
#include "a2b/stack.h"
#include "a2b/seqchart.h"
#include "a2b/seqchartctl.h"
#include "a2b/util.h"
#include "plugin_priv.h"
#include "discovery.h"
#include "periphutil.h"

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static void a2b_pwrDiagFreeSlavePlugins(a2b_HResult     result,
    a2b_Handle userData);
static void a2b_pwrDiagNotifyComplete(a2b_Plugin* plugin,
    a2b_Bool disableBusPower);
static void a2b_pwrDiagOnDiscoveryTimeout(struct a2b_Timer* timer,
    a2b_Handle userData);
static a2b_HResult a2b_pwrDiagStartDiscovery(a2b_Plugin* plugin);
static void a2b_pwrDiagCheckIntrStatus(a2b_Plugin* plugin);
static void a2b_pwrDiagOnSwitchSettleTimeout(struct a2b_Timer* timer,
    a2b_Handle userData);

/*======================= D E F I N E S ===========================*/

/** Define the amount of time (in msec) to allow the switch to
 * settle after disabling it.
 */
#define A2B_DIAG_SWITCH_SETTLE_TIMEOUT  (100u)

/** Define the diagnostic discovery timeout (msec) */
#define A2B_DIAG_DISCOVERY_TIMEOUT      (100u)

/** Maximum number of burst SRF miss to be considered as line fault */
#define A2B_MAX_NUM_SRF_MISS			(10u)

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_pwrDiagFreeSlavePlugins
*
*
*  This routine is called (often as a callback) to free any slave plugins
*  associated with discovered nodes. The function is typically called after
*  all the slave plugins have been requested to de-initialize their
*  respective peripherals.
*
*  \param          [in]    result           If being called from a callback,
*                                           the result of a peripheral
*                                           de-initialization process.
*
*  \param          [in]    userData         User data from a callback but
*                                           it should/must be a master
*                                           plugin instance.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_pwrDiagFreeSlavePlugins
    (
    a2b_HResult     result,
    a2b_Handle      userData
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)userData;

    A2B_UNUSED(result);

    if ( A2B_NULL != plugin )
    {
        /* Free up all the instantiated slave node handlers to this point */
        (void)a2b_stackFreeSlaveNodeHandler(plugin->ctx, A2B_NODEADDR_NOTUSED);
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagNotifyComplete
*
*
*  This routine when power fault diagnosis is complete. It frees any
*  allocated slave plugin handlers, notifies listeners that a fault
*  occurred (with localization information), and if a regular discovery
*  was started, ends the discovery with an error indication.
*
*  \param          [in]    plugin           The master plugin instance.
*
*  \param          [in]    disableBusPower  An indication of whether or
*                                           not to disable bus power after
*                                           the diagnosis is complete.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_pwrDiagNotifyComplete
    (
    a2b_Plugin* plugin,
    a2b_Bool    disableBusPower
    )
{
    a2b_Byte    wBuf[2u];
    struct a2b_Msg* notification;
    a2b_PowerFault* fault;
    a2b_HResult result;
    a2b_Int32 status;

    if ( A2B_NULL != plugin )
    {
        if ( disableBusPower )
        {
            /* Make best effort to turn off the phantom power at the master */
            wBuf[0] = A2B_REG_SWCTL;
            wBuf[1] = 0u;
            (void)a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
			plugin->discovery.dscNumNodes = 0u;
        }

        /* Allocate a notification message */
        notification = a2b_msgAlloc(plugin->ctx,
                                    A2B_MSG_NOTIFY,
                                    A2B_MSGNOTIFY_POWER_FAULT);

        if ( A2B_NULL == notification )
        {
            A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "a2b_pwrDiagNotifyComplete: "
                    "failed to allocate notification"));
        }
        else
        {
            fault = (a2b_PowerFault*)a2b_msgGetPayload(notification);
            fault->status = plugin->pwrDiag.results.diagResult;
            fault->intrType = (a2b_UInt8)plugin->pwrDiag.results.intrType;
            fault->faultNode = plugin->pwrDiag.results.faultNode;
            /* Make best effort delivery of notification */
            result = a2b_msgRtrNotify(notification);
            if ( A2B_FAILED(result) )
            {
                A2B_TRACE1((plugin->ctx,
                        (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                        "a2b_pwrDiagNotifyComplete: "
                        "failed to emit power fault notification: 0x%lX",
                        &result));
            }

            /* We no longer need this message */
            (void)a2b_msgUnref(notification);
        }

        A2B_SEQ_RAW0( plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT, "end" );

        /* If we're in the middle of doing discovery then we need to
         * make sure we stop the discovery process and report the error.
         */
        if ( plugin->discovery.inDiscovery )
        {
            a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_DISCOVERY_PWR_FAULT);
        }
        else
        {
// 23Mar2021 (KCF) - Wait until re-discovery to deinit and free slave plugins to avoid a race condition
//                   with the application state machine which can lead to an intermittent crash.
#if 0
            /* Do a best effort clean-up of any attached slave plugins */
            status = a2b_periphDeinit(plugin, 0,
                                    plugin->pwrDiag.results.faultNode,
                                    &a2b_pwrDiagFreeSlavePlugins,
                                    (a2b_Handle)plugin,
                                    &result);
#else
            status = A2B_EXEC_COMPLETE;
            result = A2B_RESULT_SUCCESS;
#endif

            /* If the de-initialization of the slave plugin peripherals is
             * already complete then either there were none to de-initialize OR
             * some unexpected error occurred. Regardless, let's free the slave
             * plugin instances themselves.
             */
			/************* CORRECTION -- Freeing the slave plugin during error can be problematic as Deinit may be pending */
            if ( A2B_EXEC_COMPLETE == status )
            {
                if ( A2B_FAILED(result) )
                {
                    A2B_TRACE1((plugin->ctx,
                            (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                            "a2b_pwrDiagNotifyComplete: "
                            "failed de-initialize slave plugins: 0x%lX",
                            &result));
                }
				else
				{
					/* Call the callback directly to free the plugin instances */
					a2b_pwrDiagFreeSlavePlugins(result, plugin);
				}
            }
        }
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagOnDiscoveryTimeout
*
*
*  This routine is called if the power fault diagnostic times out discovering
*  a node.
*
*  \param          [in]    timer    The timer associated with the timeout
*                                   handler.
*
*  \param          [in]    userData User data assigned to the timer when
*                                   it was created. It's passed back to
*                                   the handler and treated as an opaque
*                                   object.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_pwrDiagOnDiscoveryTimeout
    (
    struct a2b_Timer*   timer,
    a2b_Handle          userData
    )
{
    a2b_Plugin* plugin = (a2b_Plugin*)userData;

    A2B_UNUSED(timer);

    if ( A2B_NULL != plugin )
    {
        /*
         * Step 3: No interrupt received before the discovery timeout.
         */
		A2B_TRACE0((plugin->ctx,
        (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
        "a2b_pwrDiagOnDiscoveryTimeout: No critical faults before the discovery timeout"));

        plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
		if ( plugin->pwrDiag.hasFault )
        {
			plugin->pwrDiag.goodNode = plugin->pwrDiag.curNode;
		}
        plugin->pwrDiag.results.faultNode = plugin->pwrDiag.goodNode;
        plugin->pwrDiag.results.diagResult = A2B_RESULT_SUCCESS;
        a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagStartDiscovery
*
*
*  This routine actually starts a modified discovery process for identifying
*  the source of a concealed power fault.
*
*  \param          [in]    plugin   The master plugin instance.
*
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
static a2b_HResult
a2b_pwrDiagStartDiscovery
    (
    a2b_Plugin* plugin
    )
{
    a2b_Byte wBuf[2];
    a2b_UInt32 mask;
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_PLUGIN,
                                        A2B_EC_INVALID_PARAMETER);
    if ( A2B_NULL != plugin )
    {
        A2B_SEQ_RAW1(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                     "== Starting Diagnostic Discovery (NodeAddr=%hd) ==",
                     &plugin->pwrDiag.curNode);

        /* Make sure the power fault interrupts are enabled for
         * the current node
         */
        mask = a2b_intrGetMask(plugin->ctx, plugin->pwrDiag.curNode);
        if ( A2B_INTRMASK_READERR == mask )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_PLUGIN,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            if ( A2B_NODEADDR_MASTER == plugin->pwrDiag.curNode )
            {
				 wBuf[0] = A2B_REG_CONTROL;
				 wBuf[1] = 0x02u;
				 result = a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
            }
            mask |= ((a2b_UInt32)A2B_BITM_INTPND0_PWRERR << (a2b_UInt32)A2B_INTRMASK0_OFFSET);
            result = a2b_intrSetMask(plugin->ctx, plugin->pwrDiag.curNode,
                                     mask);
        }

        if ( A2B_SUCCEEDED(result) )
        {
            wBuf[0] = A2B_REG_SWCTL;
            wBuf[1] = A2B_BITM_SWCTL_DIAGMODE | A2B_BITM_SWCTL_ENSW;
            if ( A2B_NODEADDR_MASTER != plugin->pwrDiag.curNode )
            {
                result = a2b_i2cSlaveWrite(plugin->ctx,
                                            plugin->pwrDiag.curNode, 2u, wBuf);
            }
            else
            {
                result = a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
            }
        }

        /* Initiate discovery */
        if ( A2B_SUCCEEDED(result) )
        {
            wBuf[0] = A2B_REG_DISCVRY;
			if((a2b_Byte)plugin->pwrDiag.curNode != (a2b_Byte)(plugin->bdd->nodes_count - 2u))
			{
				wBuf[1] = (a2b_Byte)plugin->bdd->nodes[
									plugin->pwrDiag.curNode+1].ctrlRegs.respcycs;
			}
			else
			{
				/* Discovery beyond last slave */
				wBuf[1] = (a2b_Byte)plugin->bdd->nodes[
					plugin->pwrDiag.curNode].ctrlRegs.respcycs - 4u;
			}
            result = a2b_i2cMasterWrite( plugin->ctx, 2u, &wBuf );
        }

        if ( A2B_SUCCEEDED(result) )
        {
            plugin->pwrDiag.hasFault = A2B_FALSE;
            plugin->pwrDiag.discComplete = A2B_FALSE;
            /* Single shot timer */
            a2b_timerSet( plugin->timer,
                        A2B_DIAG_DISCOVERY_TIMEOUT, 0u);
            a2b_timerSetHandler(plugin->timer,
                                &a2b_pwrDiagOnDiscoveryTimeout);
            a2b_timerSetData(plugin->timer, plugin);
            a2b_timerStart(plugin->timer);
            A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                         "...Waiting for Diagnostic Discovery Timeout...");
        }
    }

    return result;
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagCheckIntrStatus
*
*
*  This routine is part of the logic flow for identifying the source of
*  concealed power faults.
*
*  \param          [in]    plugin   The master plugin instance.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_pwrDiagCheckIntrStatus
    (
    a2b_Plugin* plugin
    )
{
    a2b_Byte    wBuf[2u];
    a2b_Byte    rBuf[1u];
    a2b_HResult result;

    if ( A2B_NULL != plugin )
    {
       /* wBuf[0] = A2B_REG_SWCTL;
        wBuf[1] = A2B_BITM_SWCTL_ENSW | A2B_ENUM_SWCTL_MODE_VOLT_ON_VIN;
        if ( A2B_NODEADDR_MASTER == plugin->pwrDiag.curNode )
        {
            result = a2b_i2cMasterWrite(plugin->ctx, 2, wBuf);
        }
        else
        {
            result = a2b_i2cSlaveWrite(plugin->ctx, plugin->pwrDiag.curNode,
                                       2, wBuf);
        }

        if ( A2B_SUCCEEDED(result) )*/
        {
            wBuf[0] = A2B_REG_INTSTAT;
            if ( A2B_NODEADDR_MASTER != plugin->pwrDiag.curNode )
            {
                result = a2b_i2cSlaveWriteRead(plugin->ctx,
                                            plugin->pwrDiag.curNode, 1u, wBuf,
                                            1u, rBuf);
            }
            else
            {
                result = a2b_i2cMasterWriteRead(plugin->ctx, 1u, wBuf, 1u, rBuf);
            }
        }

        if ( A2B_FAILED(result) )
        {
			A2B_TRACE1((plugin->ctx,
            (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
            "a2b_pwrDiagCheckIntrStatus: failed to read "
            "A2B_REG_INTSTAT => errCode=0x%X", &result));

            plugin->pwrDiag.results.diagResult = result;
            a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
        }
        else
        {
            /* If no interrupt was registered then ... */
            if ( rBuf[0] & A2B_BITM_INTSTAT_IRQ )
            {
                plugin->pwrDiag.discComplete = A2B_TRUE;
            }
            else
            {
                /* Step #7 */
                if ( plugin->pwrDiag.hasFault )
                {
                    plugin->pwrDiag.goodNode = plugin->pwrDiag.curNode;
                    plugin->pwrDiag.priorFault = plugin->pwrDiag.hasFault;
                    plugin->pwrDiag.curNode = plugin->pwrDiag.nextNode;
                    plugin->pwrDiag.nextNode += 1;

					if ((a2b_Int16)plugin->bdd->nodes_count - (a2b_Int16)1 != plugin->pwrDiag.curNode)
					{
						result = a2b_pwrDiagStartDiscovery(plugin);
					}
					else
					{
						A2B_TRACE0((plugin->ctx,
						(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
						"a2b_pwrDiagCheckIntrStatus: No pending interrupts, current node has fault & last slave"));

						plugin->pwrDiag.results.diagResult = A2B_RESULT_SUCCESS;
						plugin->pwrDiag.results.faultNode = 
                                                  plugin->pwrDiag.goodNode;
						a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
					}
                }
                else
                {
                    /* Step #8 */

                    /* Stop the discovery timer */
                    a2b_timerStop(plugin->timer);

                    /* If NO prior fault then ... */
                    if ( !plugin->pwrDiag.priorFault )
                    {
                        /* Step #8 - discovery completed and no fault was
                         * detected. Try to discover the next node.
                         */
                        plugin->pwrDiag.goodNode = plugin->pwrDiag.curNode;
                        plugin->pwrDiag.priorFault = plugin->pwrDiag.hasFault;
                        plugin->pwrDiag.curNode = plugin->pwrDiag.nextNode;
                        plugin->pwrDiag.nextNode += 1;
						if ((a2b_Int16)plugin->bdd->nodes_count - 1 != plugin->pwrDiag.curNode)
						{
							result = a2b_pwrDiagStartDiscovery(plugin);
						}
						else
						{
							A2B_TRACE0((plugin->ctx,
							(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
							"a2b_pwrDiagCheckIntrStatus: Fault is yet to come, but reached end"));

							plugin->pwrDiag.results.diagResult = A2B_RESULT_SUCCESS;
							plugin->pwrDiag.results.faultNode = 
                                                  plugin->pwrDiag.goodNode;
							a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
						}
                    }
                    else
                    {
                        /* Step 9: Prior fault exists so we're downstream of
                         * the fault or the fault is no longer present.
                         */
						A2B_TRACE0((plugin->ctx,
						(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
						"a2b_pwrDiagCheckIntrStatus: Fault is just crossed. End"));
                        plugin->pwrDiag.results.diagResult = A2B_RESULT_SUCCESS;
                        plugin->pwrDiag.results.faultNode =
                                                  plugin->pwrDiag.goodNode;
                        a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
                    }
                }

                /* If one of the steps failed then ... */
                if ( A2B_FAILED(result) )
                {
                    A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                                 "end");
                    plugin->pwrDiag.results.diagResult = result;
                    a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
                }
            }
			
		

        }
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagOnSwitchSettleTimeout
*
*
*  This timeout handler is called after the AD2410 network switch has been
*  powered on and been allowed to settle.
*
*  \param          [in]    timer    The timer associated with the timeout
*                                   handler.
*
*  \param          [in]    userData User data assigned to the timer when
*                                   it was created. It's passed back to
*                                   the handler and treated as an opaque
*                                   object.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_pwrDiagOnSwitchSettleTimeout
    (
    struct a2b_Timer*   timer,
    a2b_Handle          userData
    )
{
    a2b_HResult result;
    a2b_Plugin* plugin = (a2b_Plugin*)userData;

    A2B_UNUSED(timer);

    /* End background switch monitoring */
    A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT, "end");

    result = a2b_pwrDiagStartDiscovery(plugin);
    if ( A2B_FAILED(result) )
    {
        plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
        plugin->pwrDiag.results.diagResult = result;
        /* End diagnostic discovery */
        A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                     "end");
        a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagInit
*
*
*  This routine initializes the power fault diagnosis control block.
*
*  \param          [in]    plugin   The master plugin instance.
*
*  \pre            None
*
*  \post           The power fault diagnosis control block is initialized to
*                  default values.
*
*  \return         None
*
******************************************************************************/
void
a2b_pwrDiagInit
    (
    struct a2b_Plugin*  plugin
    )
{
    if ( A2B_NULL != plugin )
    {
        plugin->pwrDiag.nextNode = 0;
        plugin->pwrDiag.curNode = A2B_NODEADDR_NOTUSED;
        plugin->pwrDiag.goodNode = A2B_NODEADDR_NOTUSED;
        plugin->pwrDiag.priorFault = A2B_FALSE;
        plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_INIT;
        plugin->pwrDiag.discComplete = A2B_FALSE;
        plugin->pwrDiag.hasFault = A2B_FALSE;
        plugin->pwrDiag.results.diagResult = A2B_MAKE_HRESULT(
                            A2B_SEV_FAILURE, A2B_FAC_PLUGIN, A2B_EC_INTERNAL);
        plugin->pwrDiag.results.faultNode = A2B_NODEADDR_NOTUSED;
        plugin->pwrDiag.results.intrType = (a2b_Int32)A2B_ENUM_INTTYPE_MSTR_RUNNING;
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagStart
*
*
*  This routine starts the power fault diagnosis process.
*
*  \param          [in]    plugin   The master plugin instance.
*
*  \param          [in]    intrSrc  The A2B node address reported by the
*                                   AD2410 A2B_REG_INTSRC register.
*
*  \param          [in]    intrType The interrupt type (enumeration)
*                                   defined in regdefs.h and reflected
*                                   in the AD2140 programmers guide.
*
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
a2b_HResult
a2b_pwrDiagStart
    (
    struct a2b_Plugin*  plugin,
    a2b_UInt8           intrSrc,
    a2b_UInt8           intrType
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_PLUGIN,
                                        A2B_EC_INVALID_PARAMETER);
    a2b_Bool disableBusPower = A2B_FALSE;
    a2b_Byte wBuf[2u];
    a2b_UInt8 nTempVar = intrSrc & (a2b_UInt8)A2B_BITM_INTSRC_INODE;
    a2b_Int16 nodeAddr = (intrSrc & A2B_BITM_INTSRC_MSTINT) ?
                                A2B_NODEADDR_MASTER :
                                (a2b_Int16)(nTempVar);
	a2b_UInt8	nStat, nType, nSRFCount = 0u;
	a2b_UInt8   nErrCnt = 0u;
    if ( A2B_NULL != plugin )
    {
        A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                          "group Start Power Fault Diagnosis");
        A2B_SEQ_GENNOTE2(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                         "Interrupt: NodeAddr=%hd Type=%bd",
                         &nodeAddr, &intrType);
        /* Stop the timer if it's running */
        a2b_timerStop(plugin->timer);

        plugin->pwrDiag.nextNode = 0;
        plugin->pwrDiag.curNode = A2B_NODEADDR_MASTER;
        plugin->pwrDiag.goodNode = A2B_NODEADDR_MASTER;
        plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_IN_PROGRESS;
        plugin->pwrDiag.priorFault = A2B_FALSE;
        plugin->pwrDiag.discComplete = A2B_FALSE;
        plugin->pwrDiag.hasFault = A2B_FALSE;
        plugin->pwrDiag.results.diagResult = result;
        plugin->pwrDiag.results.faultNode = nodeAddr;
        plugin->pwrDiag.results.intrType = (a2b_Int32)intrType;

        switch ( intrType )
        {
            case A2B_ENUM_INTTYPE_PWRERR_NLS_GND:
            case A2B_ENUM_INTTYPE_PWRERR_NLS_VBAT:
                /*
                 * Step 1: Localization of Concealed Faults
                 */

                A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                            "group Start Localization of Concealed Faults");
                /* Turn off the bus at the master */
                wBuf[0] = A2B_REG_SWCTL;
                wBuf[1] = 0u;
                result = a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
                if ( A2B_FAILED(result) )
                {
                    /* We can't do any more diagnosis */
                    disableBusPower = A2B_TRUE;
                    plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
                    A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                                 "end");
                    A2B_TRACE1((plugin->ctx,
                                (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                                "a2b_pwrDiagStart: failed writing to "
                                "A2B_REG_SWCTL => errCode=0x%X", &result));
                }
                /* Else bus is turned off ... let things settle for a bit */
                else
                {
                    /* After turning off the bus we reset our starting
                     * conditions.
                     */

                    /* Single shot timer */
                    a2b_timerSet( plugin->timer,
                                A2B_DIAG_SWITCH_SETTLE_TIMEOUT, 0u);
                    a2b_timerSetHandler(plugin->timer,
                                        &a2b_pwrDiagOnSwitchSettleTimeout);
                    a2b_timerSetData(plugin->timer, plugin);
                    a2b_timerStart(plugin->timer);
                    plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_IN_PROGRESS;
                    A2B_SEQ_GENNOTE0( plugin->ctx, 
                                      A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                                      "Switch Powered Off - Settling Delay" );
                    A2B_SEQ_RAW0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                                 "group Background Switch Monitoring");
                }
                break;

            case A2B_ENUM_INTTYPE_PWRERR_CS:
            case A2B_ENUM_INTTYPE_PWRERR_CDISC:
            case A2B_ENUM_INTTYPE_PWRERR_CREV:
                wBuf[0] = (a2b_Byte)A2B_REG_SWCTL;
                wBuf[1] = (a2b_Byte)0u;
                /* Turn off the switch at the interrupting node. The
                 * nodes leading up to this witch are still powered.
                 */
                if ( A2B_NODEADDR_MASTER == nodeAddr )
                {
                    result = a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
                }
                else
                {
                    result = a2b_i2cSlaveWrite(plugin->ctx, nodeAddr, 2u, wBuf);
                }

                if ( A2B_FAILED(result) )
                {
                    /* We failed turning off the switch */
                    result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                            A2B_FAC_PLUGIN,
                                            A2B_EC_POWER_DIAG_FAILURE);
                    A2B_TRACE1((plugin->ctx,
                                (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                                "a2b_pwrDiagStart: failed writing to "
                                "A2B_REG_SWCTL => errCode=0x%X", &result));
                }
                plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
                break;

            case A2B_ENUM_INTTYPE_PWRERR_CS_GND:
            case A2B_ENUM_INTTYPE_PWRERR_CS_VBAT:
                wBuf[0] = A2B_REG_SWCTL;
                wBuf[1] = 0u;
                /* Turn off the switch at the master node so the
                 * entire bus is effectively shut down.
                 */
                result = a2b_i2cMasterWrite(plugin->ctx, 2u, wBuf);
                if ( A2B_FAILED(result) )
                {
                    /* We failed turning off the switch */
                    result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                            A2B_FAC_PLUGIN,
                                            A2B_EC_POWER_DIAG_FAILURE);
                    A2B_TRACE1((plugin->ctx,
                                (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                                "a2b_pwrDiagStart: failed writing to "
                                "A2B_REG_SWCTL => errCode=0x%X", &result));
                }
                plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
                break;

			case A2B_ENUM_INTTYPE_SRFERR:
				if (!plugin->discovery.inDiscovery)
				{
					/* SRF miss - typical of Open circuit */
					do
					{

						wBuf[0] = A2B_REG_INTTYPE;
						wBuf[1] = 0u;
						result = (a2b_UInt32)a2b_i2cMasterWriteRead(plugin->ctx, 1u, wBuf, 1u, &nType);
						if (A2B_FAILED(result))
						{
							/* We failed reading interrupt status register */
							result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
													A2B_FAC_PLUGIN,
													A2B_EC_POWER_DIAG_FAILURE);
							A2B_TRACE1((plugin->ctx,
										(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
										"a2b_pwrDiagStart: failed reading from "
										"A2B_REG_INTSTAT => errCode=0x%X", &result));
						}
						else
						{
							/* Continuous read */
							wBuf[0] = A2B_REG_INTSTAT;
							wBuf[1] = 0u;
							result = (a2b_UInt32)a2b_i2cMasterWriteRead(plugin->ctx, 1u, wBuf, 1u, &nStat);
							if (A2B_FAILED(result))
							{
								/* We failed reading interrupt type register */
								result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
														A2B_FAC_PLUGIN,
														A2B_EC_POWER_DIAG_FAILURE);
								A2B_TRACE1((plugin->ctx,
											(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
											"a2b_pwrDiagStart: failed reading from "
											"A2B_REG_INTTYPE => errCode=0x%X", &result));
							}
						}

						/* SRF  */
						if (nType == A2B_ENUM_INTTYPE_SRFERR)
						{
							nSRFCount++;
						}

						/* Indeterminate Interrupt Types */
						if (((nType == A2B_ENUM_INTTYPE_PWRERR_FAULT) ||
								(nType == A2B_ENUM_INTTYPE_PWRERR_CS_VBAT)) ||
								(nType == A2B_ENUM_INTTYPE_PWRERR_NLS_VBAT) ||
								(nType == A2B_ENUM_INTTYPE_BECOVF) ||
								(nType == A2B_ENUM_INTTYPE_IRPT_MSG_ERR) ||
								(nType == A2B_ENUM_INTTYPE_MSTR_RUNNING) ||
								(nType == A2B_ENUM_INTTYPE_ICRCERR))
								
						{
							nStat = 1u;
							break;
						}
						else if( (nType == A2B_ENUM_INTTYPE_CRCERR) ||
								(nType == A2B_ENUM_INTTYPE_DDERR) ||
								(nType == A2B_ENUM_INTTYPE_DPERR))
						{
							nErrCnt++;
						}
						else
						{
							/* Completing the control statement */
						}

					} while ((nStat == 1u) && (nSRFCount < A2B_MAX_NUM_SRF_MISS) && (nErrCnt < 255u));
					
					if (nStat == 1u)
					{
						plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
						/* disableBusPower = A2B_TRUE; */
					}
				}
				break;

			case A2B_ENUM_INTTYPE_STRTUP_ERR_RTF: /* Startup Error-Return to Factory */
				plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
				disableBusPower = A2B_TRUE;
				result = A2B_RESULT_SUCCESS;
				break;

            case A2B_ENUM_INTTYPE_PWRERR_FAULT: /* Indeterminate error */
			case A2B_ENUM_INTTYPE_BECOVF:
			case A2B_ENUM_INTTYPE_IRPT_MSG_ERR:
			case A2B_ENUM_INTTYPE_MSTR_RUNNING:
			case A2B_ENUM_INTTYPE_ICRCERR:

            default:
                result = A2B_RESULT_SUCCESS;
                plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
                /* disableBusPower = A2B_TRUE; */
                break;
        }


        /* Update the diagnostic results of starting the
         * power fault diagnosis. It's possible that the diagnosis
         * is complete at this point.
         */
        plugin->pwrDiag.results.diagResult = result;
		plugin->pwrDiag.results.intrType = (a2b_Int32)intrType;		/* Interrupt type for reference */
		plugin->pwrDiag.results.faultNode = nodeAddr;				/* Node addr */

        if ( plugin->pwrDiag.state == A2B_PWR_DIAG_STATE_COMPLETE )
        {
            a2b_pwrDiagNotifyComplete(plugin, disableBusPower);
        }
    }

    return result;
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagDiagnose
*
*
*  This routine is called repeatedly while there is a pending power fault
*  interrupt to diagnose the source of the interrupt.
*
*  \param          [in]    plugin   The master plugin instance.
*
*  \param          [in]    intrSrc  The A2B node address reported by the
*                                   AD2410 A2B_REG_INTSRC register.
*
*  \param          [in]    intrType The interrupt type (enumeration)
*                                   defined in regdefs.h and reflected
*                                   in the AD2140 programmers guide.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
void
a2b_pwrDiagDiagnose
    (
    struct a2b_Plugin*  plugin,
    a2b_UInt8           intrSrc,
    a2b_UInt8           intrType
    )
{
	a2b_UInt8 nTempVar = intrSrc & (a2b_UInt8)A2B_BITM_INTSRC_INODE;
    a2b_Int16 nodeAddr = (intrSrc & (a2b_UInt8)A2B_BITM_INTSRC_MSTINT) ?
    							(a2b_Int16)A2B_NODEADDR_MASTER :
								(a2b_Int16)(nTempVar);

    if ( A2B_NULL != plugin )
    {
        A2B_SEQ_GENNOTE2(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                         "Diagnose Interrupt: NodeAddr=%hd Type=%bd",
                         &nodeAddr, &intrType);

        A2B_TRACE2((plugin->ctx,
                    (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE2),
                    "a2b_pwrDiagDiagnose: processing interrupt "
                    "src=0x%02bX type=0x%02bX", &intrSrc, &intrType));

        switch ( intrType )
        {
            case A2B_ENUM_INTTYPE_PWRERR_NLS_GND:
            case A2B_ENUM_INTTYPE_PWRERR_NLS_VBAT:
                /* Step 4: An error occurred somewhere downstream of the
                 * current node.
                 */
                A2B_SEQ_GENERROR1(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                                  "Error occurred downstream of NodeAddr=%hd",
                                  &nodeAddr);
                plugin->pwrDiag.hasFault = A2B_TRUE;
                plugin->pwrDiag.results.intrType = (a2b_Int32)intrType;
                if ( plugin->pwrDiag.discComplete )
                {
                    /* Go to Step #6 - check the INTSTAT register */
                    a2b_pwrDiagCheckIntrStatus(plugin);
                }
                /* Else we keep waiting for discovery done or a timeout */
                break;

            case A2B_ENUM_INTTYPE_DSCDONE:
                /* Step #5 - discovery done is possible in presence of other
                 * interrupts.
                 */
                A2B_SEQ_GENNOTE0(plugin->ctx, A2B_SEQ_CHART_LEVEL_PWR_FAULT,
                                 "Discovery done possibly in presence"
                                 " of other interrupts");
                plugin->pwrDiag.discComplete = A2B_TRUE;
                /* Go to Step #6 - check the INTSTAT register */
                a2b_pwrDiagCheckIntrStatus(plugin);
                break;
			case A2B_ENUM_INTTYPE_PWRERR_CS_GND:
			case A2B_ENUM_INTTYPE_PWRERR_CS_VBAT:
			       A2B_TRACE2((plugin->ctx,
                            (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                            "a2b_pwrDiagDiagnose: Localized interrupt during Fault localization: "
                            "src=0x%02bX type=0x%02bX", &intrSrc, &intrType));
                plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
                plugin->pwrDiag.results.intrType = (a2b_Int32)intrType;
                plugin->pwrDiag.results.faultNode = nodeAddr;
                plugin->pwrDiag.results.diagResult = A2B_RESULT_SUCCESS;
				a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
                break;
			case A2B_ENUM_INTTYPE_PWRERR_CDISC:
				break;
			default:
                A2B_TRACE2((plugin->ctx,
                            (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                            "a2b_pwrDiagDiagnose: unexpected interrupt: "
                            "src=0x%02bX type=0x%02bX", &intrSrc, &intrType));
                plugin->pwrDiag.state = A2B_PWR_DIAG_STATE_COMPLETE;
                plugin->pwrDiag.results.intrType = (a2b_Int32)intrType;
                plugin->pwrDiag.results.faultNode = nodeAddr;
                plugin->pwrDiag.results.diagResult =
                                            A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                            A2B_FAC_PLUGIN,
                                            A2B_EC_POWER_DIAG_FAILURE);
                a2b_timerStop(plugin->timer);
                a2b_pwrDiagNotifyComplete(plugin, A2B_TRUE);
                break;
        }
    }
}


/*!****************************************************************************
*
*  \b              a2b_pwrDiagIsActive
*
*
*  This routine determines whether A2B power fault diagnosis is currently
*  active.
*
*  \param          [in]    plugin   Pointer to the master node plugin.
*
*  \pre            None
*
*  \post           None
*
*  \return         An indication of whether power fault diagnosis is active
*                  and on-going (A2B_TRUE) or not (A2B_FALSE).
*
******************************************************************************/
a2b_Bool
a2b_pwrDiagIsActive
    (
    struct a2b_Plugin*  plugin
    )
{
    a2b_Bool isActive = A2B_FALSE;

    if ( A2B_NULL != plugin )
    {
        isActive = (a2b_Bool)(plugin->pwrDiag.state == A2B_PWR_DIAG_STATE_IN_PROGRESS);

    }
    return isActive;
}

/**
 @}
*/


/**
 @}
*/
