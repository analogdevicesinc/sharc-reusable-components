/*!
*********************************************************************************
 *
 * @file:    adi_gemac_av.c
 *
 * @brief:   AV module of Ethernet GEMAC driver source file
 *
 * @version: $Revision: 25625 $
 *
 * @date:    $Date: 2016-03-18 07:26:22 -0400 (Fri, 18 Mar 2016) $
 * ------------------------------------------------------------------------------
 *
 * Copyright (c) 2011-2016 Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Modified versions of the software must be conspicuously marked as such.
 * - This software is licensed solely and exclusively for use with processors
 *   manufactured by or for Analog Devices, Inc.
 * - This software may not be combined or merged with other code in any manner
 *   that would cause the software to become subject to terms and conditions
 *   which differ from those listed here.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights of one
 *   or more patent holders.  This license does not release you from the
 *   requirement that you obtain separate licenses from these patent holders
 *   to use this software.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
 * PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/*========== I N C L U D E ==========*/

#include "adi_ether.h"
#include "adi_ether_gemac.h"
#include "adi_gemac_int.h"
#include "adi_gemac_proc_int.h"

#ifdef ADI_ETHER_SUPPORT_AV

#ifdef ADI_DEBUG
#include <assert.h>
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif

static ADI_ETHER_RESULT av_setProfile (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	ADI_ETHER_GEMAC_AV_PROFILE* pAVProfile = (ADI_ETHER_GEMAC_AV_PROFILE*)arg0;

#ifdef ADI_DEBUG
	if (pAVProfile == NULL) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
	if (pDev->Started) {
		return ADI_ETHER_RESULT_INVALID_SEQUENCE;
	}

	/* DMA2 cannot be enabled if DMA1 is disabled */
	if (
			/* Check the parameters in AV Profile */
		    (   (pAVProfile->PTPPktChannel != ADI_ETHER_GEMAC_DMA_CHANNEL_0)
		     && (pAVProfile->PTPPktChannel != ADI_ETHER_GEMAC_DMA_CHANNEL_1)
		     && (pAVProfile->PTPPktChannel != ADI_ETHER_GEMAC_DMA_CHANNEL_2))

		||  (   (pAVProfile->AVControlPktChannel != ADI_ETHER_GEMAC_DMA_CHANNEL_0)
			 && (pAVProfile->AVControlPktChannel != ADI_ETHER_GEMAC_DMA_CHANNEL_1)
			 && (pAVProfile->AVControlPktChannel != ADI_ETHER_GEMAC_DMA_CHANNEL_2))

			/* Check the CBS parameters if enabled */
		||  (   (pAVProfile->Chan1.bEnableCBS)
		     && (
		    		 ((pAVProfile->Chan1.idleSlopeCredit > 8.0) || (pAVProfile->Chan1.idleSlopeCredit < 0.0))
		    	|| 	 ((pAVProfile->Chan1.sendSlopeCredit > 8.0) || (pAVProfile->Chan1.sendSlopeCredit < 0.0))
		    	||   ((pAVProfile->Chan1.MaxCredit >  131072.0) || (pAVProfile->Chan1.MaxCredit < 0.0))
		    	||   ((pAVProfile->Chan1.MinCredit < -131072.0) || (pAVProfile->Chan1.MinCredit > 0.0))
				))
			/* Check the CBS parameters if enabled */
		||  (   (pAVProfile->Chan2.bEnableCBS)
			 && (
					 ((pAVProfile->Chan2.idleSlopeCredit > 8.0) || (pAVProfile->Chan2.idleSlopeCredit < 0.0))
				|| 	 ((pAVProfile->Chan2.sendSlopeCredit > 8.0) || (pAVProfile->Chan2.sendSlopeCredit < 0.0))
				||   ((pAVProfile->Chan2.MaxCredit >  131072.0) || (pAVProfile->Chan2.MaxCredit < 0.0))
				||   ((pAVProfile->Chan2.MinCredit < -131072.0) || (pAVProfile->Chan2.MinCredit > 0.0))
				))
			/* Tx/Rx of DMA1 cannot be enabled if it is not supported */
		||	(((pDev->Capability & ADI_EMAC_CAPABILITY_AV_DMA1) == 0u) && (pAVProfile->Chan1.TxDMA.bEnabled || pAVProfile->Chan1.RxDMA.bEnabled))
			/* Tx/Rx of DMA2 cannot be enabled if it is not supported */
		||	(((pDev->Capability & ADI_EMAC_CAPABILITY_AV_DMA2) == 0u) && (pAVProfile->Chan2.TxDMA.bEnabled || pAVProfile->Chan2.RxDMA.bEnabled))
			/* DMA2 cannot be enabled if DMA1 is disabled */
		||	(!pAVProfile->Chan1.TxDMA.bEnabled && pAVProfile->Chan2.TxDMA.bEnabled)
			/* DMA2 cannot be enabled if DMA1 is disabled */
		||  (!pAVProfile->Chan1.RxDMA.bEnabled && pAVProfile->Chan2.RxDMA.bEnabled)
		    /* Number of reserved descriptor should be atleast 1 */
		||  ((pAVProfile->Chan1.TxDMA.bEnabled) && (pAVProfile->Chan1.TxDMA.nNumReservedDesc == 0))
		||  ((pAVProfile->Chan2.TxDMA.bEnabled) && (pAVProfile->Chan2.TxDMA.nNumReservedDesc == 0))
		||  ((pAVProfile->Chan1.RxDMA.bEnabled) && (pAVProfile->Chan1.RxDMA.nNumReservedDesc == 0))
		||  ((pAVProfile->Chan2.RxDMA.bEnabled) && (pAVProfile->Chan2.RxDMA.nNumReservedDesc == 0))

		    /* Check whether enough DMA descriptors are available */
		||  ((pAVProfile->Chan1.TxDMA.nNumReservedDesc + pAVProfile->Chan2.TxDMA.nNumReservedDesc + 1u /* Minimum 1 for DMA 0 */) >= pDev->Tx.nDMADescNum)
		||  ((pAVProfile->Chan1.RxDMA.nNumReservedDesc + pAVProfile->Chan2.RxDMA.nNumReservedDesc + 1u /* Minimum 1 for DMA 0 */) >= pDev->Rx.nDMADescNum)
		)
	{
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif

	/* Clean the AV Device structure */
	memset(&pDev->AVDevice, 0u, sizeof(pDev->AVDevice));

	/* Enable AV feature */
	pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_AV_EN;

	/* IF (DMA1 Tx is enabled) */
	if (pAVProfile->Chan1.TxDMA.bEnabled) {
		pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA1_EN | ADI_EMAC_AV_CONFIG_DMA1_TX_EN;
		pDev->AVDevice.Chan1.Tx.NumReservedDesc = pAVProfile->Chan1.TxDMA.nNumReservedDesc;
	}

	/* IF (DMA1 Rx is enabled) */
	if (pAVProfile->Chan1.RxDMA.bEnabled) {
		pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA1_EN | ADI_EMAC_AV_CONFIG_DMA1_RX_EN;
		pDev->AVDevice.Chan1.Rx.NumReservedDesc = pAVProfile->Chan1.RxDMA.nNumReservedDesc;
	}

	/* IF (DMA2 Tx is enabled) */
	if (pAVProfile->Chan2.TxDMA.bEnabled) {
		pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA2_EN | ADI_EMAC_AV_CONFIG_DMA2_TX_EN;
		pDev->AVDevice.Chan2.Tx.NumReservedDesc = pAVProfile->Chan2.TxDMA.nNumReservedDesc;
	}

	/* IF (DMA2 Rx is enabled) */
	if (pAVProfile->Chan2.RxDMA.bEnabled) {
		pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA2_EN | ADI_EMAC_AV_CONFIG_DMA2_RX_EN;
		pDev->AVDevice.Chan2.Rx.NumReservedDesc = pAVProfile->Chan2.RxDMA.nNumReservedDesc;
	}

	/* IF (DMA1 is enabled) */
	if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN) {
		if (pAVProfile->Chan1.bEnableCBS) {
			pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA1_CBS_EN;
			pDev->AVDevice.Chan1.nCBSCtlReg      =(pAVProfile->Chan1.bResetCredit) ? 0u : BITM_EMAC_DMA1_CHCBSCTL_CC;
			pDev->AVDevice.Chan1.nIdleSlopeReg   = pAVProfile->Chan1.idleSlopeCredit * 1024;
			pDev->AVDevice.Chan1.nSendSlopeReg   = pAVProfile->Chan1.sendSlopeCredit * 1024;
			pDev->AVDevice.Chan1.nHiCreditReg    = pAVProfile->Chan1.MaxCredit * 1024;
			pDev->AVDevice.Chan1.nLowCreditReg   = (uint32_t)((int32_t)(pAVProfile->Chan1.MinCredit * 1024.0));
		}

		if (pAVProfile->Chan1.bEnableSlotComparison) {
			pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA1_SLOT_EN;

			pDev->AVDevice.Chan1.nSlotCtlStatReg = (
													  ((pAVProfile->Chan1.bEnableAdvanceSlotCheck) ? BITM_EMAC_DMA1_CHSFCS_ASC : 0u)
													| BITM_EMAC_DMA1_CHSFCS_ESC
													);
		}
	}

	/* IF (DMA2 is enabled) */
	if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN) {
		if (pAVProfile->Chan2.bEnableCBS) {
			pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA2_CBS_EN;
			pDev->AVDevice.Chan2.nCBSCtlReg      = (pAVProfile->Chan2.bResetCredit) ? 0u : BITM_EMAC_DMA2_CHCBSCTL_CC;
			pDev->AVDevice.Chan2.nIdleSlopeReg   = pAVProfile->Chan2.idleSlopeCredit * 1024;
			pDev->AVDevice.Chan2.nSendSlopeReg   = pAVProfile->Chan2.sendSlopeCredit * 1024;
			pDev->AVDevice.Chan2.nHiCreditReg    = pAVProfile->Chan2.MaxCredit * 1024;
			pDev->AVDevice.Chan2.nLowCreditReg   = (uint32_t)((int32_t)(pAVProfile->Chan2.MinCredit * 1024.0));
		}

		if (pAVProfile->Chan2.bEnableSlotComparison) {
			pDev->AVDevice.config |= ADI_EMAC_AV_CONFIG_DMA2_SLOT_EN;

			pDev->AVDevice.Chan2.nSlotCtlStatReg = (
													  ((pAVProfile->Chan2.bEnableAdvanceSlotCheck) ? BITM_EMAC_DMA2_CHSFCS_ASC : 0u)
													| BITM_EMAC_DMA2_CHSFCS_ESC
													);
		}

	}

	/* Set the AV_MAC_Ctl Register */
	pDev->AVDevice.nAVMACCtlReg = (
									  (((uint32_t)pAVProfile->PTPPktChannel)       << BITP_EMAC_MAC_AVCTL_PTPCH)
									| (((uint32_t)pAVProfile->AVControlPktChannel) << BITP_EMAC_MAC_AVCTL_AVCH)
									| (((uint32_t)pAVProfile->AVPriority)  << BITP_EMAC_MAC_AVCTL_AVP)
									| (((uint32_t)pAVProfile->AVEtherType) << BITP_EMAC_MAC_AVCTL_AVT)
									| ((pAVProfile->bEnableTaggedNonAVQueueing) ? (uint32_t)BITM_EMAC_MAC_AVCTL_VQE : 0u)
								   );

	return ADI_ETHER_RESULT_SUCCESS;
}


ADI_ETHER_RESULT gemac_AV_ModuleIO (
										  ADI_EMAC_DEVICE*      const pDev,
										  ADI_ETHER_MODULE_FUNC const Func,
										  void*                       arg0,
										  void*                       arg1,
										  void*                       arg2
        								  )
{
	ADI_ETHER_RESULT eResult = ADI_ETHER_RESULT_SUCCESS;

	if (!(pDev->Capability & ADI_EMAC_CAPABILITY_AV)) {
		return ADI_ETHER_RESULT_NOT_SUPPORTED;
	}

	switch(Func)
	{
	case ADI_ETHER_MODULE_FUNC_AV_SET_PROFILE:
		eResult = av_setProfile(pDev,arg0, arg1, arg2);
		break;

	default:
		eResult = ADI_ETHER_RESULT_NOT_SUPPORTED;
		break;
	}

	return eResult;
}

#endif
