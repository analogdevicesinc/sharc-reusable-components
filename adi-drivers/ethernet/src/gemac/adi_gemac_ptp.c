/*!
*********************************************************************************
 *
 * @file:    adi_gemac_ptp.c
 *
 * @brief:   PTP module of Ethernet GEMAC driver source file
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
#include "adi_ether.h"
#include "adi_gemac_int.h"
#include "adi_gemac_proc_int.h"

#ifdef ADI_ETHER_SUPPORT_PTP

#ifdef ADI_DEBUG
#include <assert.h>
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif

/*==========  DEFINES  ==========*/

#if defined (__ADSP215xx__)
#define ENUM_PADS_PCFG0_EMAC0_PTP_CLK_SRC_RMII   (0u << BITP_PADS_PCFG0_EMAC0)
#define ENUM_PADS_PCFG0_EMAC0_PTP_CLK_SRC_SCLK   (1u << BITP_PADS_PCFG0_EMAC0)
#define ENUM_PADS_PCFG0_EMAC0_PTP_CLK_SRC_EXT    (2u << BITP_PADS_PCFG0_EMAC0)
#endif

/* Packet Filter for all PTP packets */
#define PKT_FILTER_PTP_ALL 						 (    ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC						\
													| ADI_ETHER_GEMAC_PKT_TYPE_PTP_FOLLOW_UP					\
													| ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ                  \
													| ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_RESP                 \
													| ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_REQ                 \
													| ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP                \
													| ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP_FOLLOW_UP      \
													)

static ADI_ETHER_RESULT ptp_config (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	uint32_t tmp32u;

#if defined(ADI_DEBUG)
	if (arg0 == NULL) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif

	ADI_ETHER_GEMAC_PTP_CONFIG* pConfig = (ADI_ETHER_GEMAC_PTP_CONFIG*)arg0;
	ADI_ETHER_CALLBACK_FN pfCallback = (ADI_ETHER_CALLBACK_FN)arg1;

#if defined(ADI_DEBUG)
	if (	(	(pConfig->eClkSrc != ADI_ETHER_GEMAC_PTP_CLK_SRC_SCLK)
			 && (pConfig->eClkSrc != ADI_ETHER_GEMAC_PTP_CLK_SRC_PHY)
			 && (pConfig->eClkSrc != ADI_ETHER_GEMAC_PTP_CLK_SRC_EXT))
		|| (pConfig->nClkFreq <= pConfig->nPTPClkFreq)
		|| (pConfig->nInitTime.NanoSecond >= (1000*1000*1000)))
	{
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}

	if (pDev->PTPDevice.bEnabled) {
		return ADI_ETHER_RESULT_INVALID_SEQUENCE;
	}
#endif

	/* Set up the PTP clock source */
	tmp32u = (*pREG_PADS0_PCFG0 & (~BITM_PADS_PCFG0_EMAC0));
	if (pConfig->eClkSrc == ADI_ETHER_GEMAC_PTP_CLK_SRC_PHY) {
		tmp32u |= ENUM_PADS_PCFG0_EMAC0_PTP_CLK_SRC_RMII;
	}
	if (pConfig->eClkSrc == ADI_ETHER_GEMAC_PTP_CLK_SRC_SCLK) {
		tmp32u |= ENUM_PADS_PCFG0_EMAC0_PTP_CLK_SRC_SCLK;
	}
	if (pConfig->eClkSrc == ADI_ETHER_GEMAC_PTP_CLK_SRC_EXT) {
		tmp32u |= ENUM_PADS_PCFG0_EMAC0_PTP_CLK_SRC_EXT;
	}
	*pREG_PADS0_PCFG0 = tmp32u;

	/* Set the Timestamp Control Register */
	pDev->pEMAC_REGS->EMAC_TM_CTL = 0u;

	/* Clear the Rx Filter Flag */
	pDev->PTPDevice.nRxFilterFlag = 0u;

	/* Set the required filter in the internal PTP config structure */
	/* Set the Rx filter */
	if (pConfig->RxPktFilter & ADI_ETHER_GEMAC_PKT_TYPE_ALL) {
		pDev->PTPDevice.nRxFilterFlag |= BITM_EMAC_TM_CTL_TSENALL;
	} else {
		uint32_t PktToBeFiltered = pConfig->RxPktFilter & PKT_FILTER_PTP_ALL;

		switch (PktToBeFiltered)
		{
		case 	  (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_FOLLOW_UP
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_RESP:

			pDev->PTPDevice.nRxFilterFlag |=  (0u << BITP_EMAC_TM_CTL_SNAPTYPSEL);
			break;

		case  (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC:

			pDev->PTPDevice.nRxFilterFlag |=  (  (0u << BITP_EMAC_TM_CTL_SNAPTYPSEL)
												| BITM_EMAC_TM_CTL_TSEVNTENA);
			break;

		case (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ:

			pDev->PTPDevice.nRxFilterFlag |= (  (0u << BITP_EMAC_TM_CTL_SNAPTYPSEL)
					 	 	 	 	 	 	  | BITM_EMAC_TM_CTL_TSMSTRENA
					 	 	 	 	 	 	  | BITM_EMAC_TM_CTL_TSEVNTENA);
			break;

		case 	  (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_FOLLOW_UP
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_RESP
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_REQ
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP_FOLLOW_UP:

			pDev->PTPDevice.nRxFilterFlag |= (  (1u << BITP_EMAC_TM_CTL_SNAPTYPSEL));
			break;

		case 	  (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC
				| (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_REQ
				| (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP:

			pDev->PTPDevice.nRxFilterFlag |= (  (1u << BITP_EMAC_TM_CTL_SNAPTYPSEL)
					 	 	 	 	 	 	  | BITM_EMAC_TM_CTL_TSEVNTENA);
			break;

		case      (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_REQ
				| (uint32_t)ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP:

			pDev->PTPDevice.nRxFilterFlag |= (  (1u << BITP_EMAC_TM_CTL_SNAPTYPSEL)
					 	 	 	 	 	 	  | BITM_EMAC_TM_CTL_TSMSTRENA
					 	 	 	 	 	 	  | BITM_EMAC_TM_CTL_TSEVNTENA);
			break;

		case      (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC
				| (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ:

			pDev->PTPDevice.nRxFilterFlag |= (  (2u << BITP_EMAC_TM_CTL_SNAPTYPSEL));
			break;

		case      (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_REQ
				| (uint32_t) ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP:

			pDev->PTPDevice.nRxFilterFlag |= (  (3u << BITP_EMAC_TM_CTL_SNAPTYPSEL));
			break;

		default:
			return ADI_ETHER_RESULT_PARAM_NOT_SUPPORTED;
			break;

		}

		if (pConfig->RxPktFilter & ADI_ETHER_GEMAC_PKT_TYPE_PTP_OVER_IPV4_UDP) {
			pDev->PTPDevice.nRxFilterFlag |= ENUM_EMAC_TM_CTL_E_TSTMP_IPV4;
		}
		if (pConfig->RxPktFilter & ADI_ETHER_GEMAC_PKT_TYPE_PTP_OVER_IPV6_UDP) {
			pDev->PTPDevice.nRxFilterFlag |= ENUM_EMAC_TM_CTL_E_TSTMP_IPV6;
		}
		if (pConfig->RxPktFilter & ADI_ETHER_GEMAC_PKT_TYPE_PTP_OVER_ETHERNET_FRAME) {
			pDev->PTPDevice.nRxFilterFlag |= ENUM_EMAC_TM_CTL_E_PTP_OV_ETHER;
		}
		if (pConfig->RxPktFilter & ADI_ETHER_GEMAC_PKT_TYPE_PTP_V2) {
			pDev->PTPDevice.nRxFilterFlag |= ENUM_EMAC_TM_CTL_E_PKT_SNOOP_V2;
		}
	}

	pDev->PTPDevice.nInitTime = pConfig->nInitTime;
	pDev->PTPDevice.nInputClkFreq = pConfig->nClkFreq;
	pDev->PTPDevice.PTPClkFreq = pConfig->nPTPClkFreq;
	pDev->PTPDevice.pfCallback = pfCallback;



	return ADI_ETHER_RESULT_SUCCESS;
}

static ADI_ETHER_RESULT ptp_enable (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	bool bEnable = (bool)arg0;
	uint32_t tmp32u;
	uint64_t tmp64u;

	if (bEnable) {
		/* Set the Timestamp Control Register */
		pDev->pEMAC_REGS->EMAC_TM_CTL = 0u;

		/* Set the Rx Filter */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= pDev->PTPDevice.nRxFilterFlag;

		/* Enable PTP */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSENA;

		/* Enable Digital Roll over */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSCTRLSSR;

		/* Set the sub-second increment register */
		tmp32u = (1000*1000*1000)/pDev->PTPDevice.PTPClkFreq;
		pDev->pEMAC_REGS->EMAC_TM_SUBSEC = tmp32u;

		/* Set the ADDEND for frequency */
		tmp64u = pDev->PTPDevice.PTPClkFreq;
		tmp64u = tmp64u << 32;
		tmp64u = tmp64u / pDev->PTPDevice.nInputClkFreq;
		pDev->pEMAC_REGS->EMAC_TM_ADDEND = tmp64u;

		/* Set the Addend Update bit and set the update for fine update */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSADDREG | ENUM_EMAC_TM_CTL_EN_FINE_UPDT;

		/* Wait for the Addend Update bit to clear */
		while ((pDev->pEMAC_REGS->EMAC_TM_CTL & BITM_EMAC_TM_CTL_TSADDREG) != 0u) {}

		/* Set the update for fine update */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= ENUM_EMAC_TM_CTL_EN_FINE_UPDT;


		/* Set the init time */
		pDev->pEMAC_REGS->EMAC_TM_HISEC    = pDev->PTPDevice.nInitTime.HSecond;
		pDev->pEMAC_REGS->EMAC_TM_SECUPDT  = pDev->PTPDevice.nInitTime.LSecond;
		pDev->pEMAC_REGS->EMAC_TM_NSECUPDT = pDev->PTPDevice.nInitTime.NanoSecond;

		/* Initialize the timestamp */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= ENUM_EMAC_TM_CTL_EN_TS_INIT;

		/* Wait for the timestamp initialize bit to clear */
		while ((pDev->pEMAC_REGS->EMAC_TM_CTL & BITM_EMAC_TM_CTL_TSINIT) != 0u) {}

	} else {
		/* Set the Timestamp Control Register to disable and clear config */
		pDev->pEMAC_REGS->EMAC_TM_CTL = 0u;
	}

	pDev->PTPDevice.bEnabled = bEnable;

	return ADI_ETHER_RESULT_SUCCESS;
}

static ADI_ETHER_RESULT ptp_get_cur_time (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	ADI_ETHER_TIME* pTime = (ADI_ETHER_TIME*)arg0;

#ifdef ADI_DEBUG
	if (arg0 == NULL) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif

	pTime->NanoSecond = pDev->pEMAC_REGS->EMAC_TM_NSEC;
	pTime->LSecond    = pDev->pEMAC_REGS->EMAC_TM_SEC;
	pTime->HSecond    = pDev->pEMAC_REGS->EMAC_TM_HISEC;

	return ADI_ETHER_RESULT_SUCCESS;
}

static ADI_ETHER_RESULT ptp_update_freq (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	uint32_t nClkFreq = (uint32_t)arg0;
	uint64_t tmp64u;

#ifdef ADI_DEBUG
	if ((nClkFreq == 0) || (nClkFreq < pDev->PTPDevice.PTPClkFreq)) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif

	if (pDev->PTPDevice.bEnabled) {

		/* Set the ADDEND for frequency */
		tmp64u = pDev->PTPDevice.PTPClkFreq;
		tmp64u = tmp64u << 32;
		tmp64u = tmp64u / nClkFreq;
		pDev->pEMAC_REGS->EMAC_TM_ADDEND = tmp64u;

		/* Set the Addend Update bit */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSADDREG;

		/* Wait for the Addend Update bit to clear */
		while ((pDev->pEMAC_REGS->EMAC_TM_CTL & BITM_EMAC_TM_CTL_TSADDREG) != 0u) {}

		/* Set the update for fine update */
		pDev->pEMAC_REGS->EMAC_TM_CTL |= ENUM_EMAC_TM_CTL_EN_FINE_UPDT;
	} else {
		pDev->PTPDevice.nInputClkFreq = nClkFreq;
	}

	return ADI_ETHER_RESULT_SUCCESS;
}

static ADI_ETHER_RESULT ptp_apply_time_offset (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	ADI_ETHER_TIME* pTime = (ADI_ETHER_TIME*)arg0;
	bool bAdd = (bool)arg1;

#ifdef ADI_DEBUG
	if (!pDev->PTPDevice.bEnabled) {
		return ADI_ETHER_RESULT_INVALID_SEQUENCE;
	}

	if ((pTime == NULL) || (pTime->NanoSecond >= 1000000000)) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif

	uint64_t nSecTimeCur =   (uint64_t)pDev->pEMAC_REGS->EMAC_TM_HISEC * ((uint64_t)1 << 32u)
			               + (uint64_t)pDev->pEMAC_REGS->EMAC_TM_SEC;

	uint64_t nSecOffset = (uint64_t)pTime->HSecond * ((uint64_t)1 << 32u) + (uint64_t)pTime->LSecond;
	int64_t tmp64i_1, tmp64i_2, tmp64i_3;
	int32_t HSecChange;

	if (!bAdd)
	{
		if (  (nSecTimeCur < nSecOffset)
			||((nSecTimeCur == nSecOffset) && (pDev->pEMAC_REGS->EMAC_TM_NSEC < pTime->NanoSecond)))
		{
			return ADI_ETHER_RESULT_INVALID_PARAM;
		}
	}

	tmp64i_1 =   (int64_t)pDev->pEMAC_REGS->EMAC_TM_SEC * ((int64_t)(1000*1000*1000)) + (int64_t)pDev->pEMAC_REGS->EMAC_TM_NSEC;
	tmp64i_2 = (int64_t)pTime->LSecond * ((int64_t)(1000*1000*1000)) + (int64_t)pTime->NanoSecond;

	if (bAdd) {
		tmp64i_3 =  (tmp64i_1 + tmp64i_2);
		HSecChange = pTime->HSecond;
	} else {
		tmp64i_3 = (tmp64i_1 - tmp64i_2);
		HSecChange = -1 * (int32_t)pTime->HSecond;
	}

	if (tmp64i_3 < 0) {
		HSecChange--;
	}

	if (tmp64i_3 > ((int64_t)(1000*1000*1000) << 32)) {
		HSecChange++;
	}

	pDev->pEMAC_REGS->EMAC_TM_SECUPDT  = pTime->LSecond;
	pDev->pEMAC_REGS->EMAC_TM_NSECUPDT = pTime->NanoSecond;
	if (!bAdd) {
		pDev->pEMAC_REGS->EMAC_TM_NSECUPDT |= 0x80000000u;
	}

	pDev->pEMAC_REGS->EMAC_TM_CTL |= ENUM_EMAC_TM_CTL_EN_UPDATE;
	while((pDev->pEMAC_REGS->EMAC_TM_CTL & ENUM_EMAC_TM_CTL_EN_UPDATE) != 0) {}

	pDev->pEMAC_REGS->EMAC_TM_HISEC += HSecChange;


	return ADI_ETHER_RESULT_SUCCESS;
}

ADI_ETHER_RESULT gemac_PTPModuleIO (
									ADI_EMAC_DEVICE*      const pDev,
  								    ADI_ETHER_MODULE_FUNC const Func,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
        							)
{
	ADI_ETHER_RESULT eResult = ADI_ETHER_RESULT_SUCCESS;

	if (!(pDev->Capability & ADI_EMAC_CAPABILITY_PTP)) {
		return ADI_ETHER_RESULT_NOT_SUPPORTED;
	}

	switch(Func)
	{
	case ADI_ETHER_MODULE_FUNC_PTP_CFG:
		eResult = ptp_config(pDev,arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_PTP_EN:
		eResult = ptp_enable(pDev,arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_PTP_GET_CUR_TIME:
		eResult = ptp_get_cur_time(pDev, arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_PTP_UPDATE_FREQ:
		eResult = ptp_update_freq(pDev, arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_PTP_APPLY_TIME_OFFSET:
		eResult = ptp_apply_time_offset(pDev, arg0, arg1, arg2);
		break;

	default:
		eResult = ADI_ETHER_RESULT_NOT_SUPPORTED;
		break;
	}

	return eResult;
}

#endif
