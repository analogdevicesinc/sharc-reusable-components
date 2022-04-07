/*!
*********************************************************************************
 *
 * @file:    adi_gemac_pps_alarm.c
 *
 * @brief:   PPS and Alarm module of Ethernet GEMAC driver source file
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

#if defined(ADI_ETHER_SUPPORT_ALARM) && defined(ADI_ETHER_SUPPORT_PPS)

#ifdef ADI_DEBUG
#include <assert.h>
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif

/*========== I N L I N E    F U N C T I O N S ==========*/

/* Enable Flexible PPS */
static void pps_flexible_enable (ADI_EMAC_DEVICE* const pDev) {
	/* Enable Flexible PPS */
	pDev->pEMAC_REGS->EMAC_TM_PPSCTL |= BITM_EMAC_TM_PPSCTL_PPSEN;
}

/* Set the PPS interval */
static void pps_set_interval (
							  ADI_EMAC_DEVICE* const pDev,
							  uint32_t               nDeviceID,
							  uint32_t               nIntervalInPTPCycles
							  )
{
	volatile uint32_t* pIntRegister[] = {
			pREG_EMAC0_TM_PPS0INTVL,
			pREG_EMAC0_TM_PPS1INTVL,
			pREG_EMAC0_TM_PPS2INTVL,
			pREG_EMAC0_TM_PPS3INTVL
	};

	*pIntRegister[nDeviceID] = (nIntervalInPTPCycles - 1);
}

/* Set the PPS width */
static void pps_set_width (
						   ADI_EMAC_DEVICE* const pDev,
						   uint32_t               nDeviceID,
						   uint32_t               nWidthInPTPCycles
						   )
{
	volatile uint32_t* pIntRegister[] = {
			pREG_EMAC0_TM_PPS0WIDTH,
			pREG_EMAC0_TM_PPS1WIDTH,
			pREG_EMAC0_TM_PPS2WIDTH,
			pREG_EMAC0_TM_PPS3WIDTH
	};

	*pIntRegister[nDeviceID] = (nWidthInPTPCycles - 1);
}

/* Set the trigger mode for the given PPS */
void pps_set_trigger_mode (
		ADI_EMAC_DEVICE* const pDev,
		uint32_t nDeviceID,
		uint32_t TriggerMode
		)
{
	static volatile uint32_t BITP_TRIGGER_MODE[] = {
			BITP_EMAC_TM_PPSCTL_TRGTMODSEL,
			BITP_EMAC_TM_PPSCTL_TRGTMODSEL1,
			BITP_EMAC_TM_PPSCTL_TRGTMODSEL2,
			BITP_EMAC_TM_PPSCTL_TRGTMODSEL3
	};

	static volatile uint32_t BITM_TRIGGER_MODE[] = {
			BITM_EMAC_TM_PPSCTL_TRGTMODSEL,
			BITM_EMAC_TM_PPSCTL_TRGTMODSEL1,
			BITM_EMAC_TM_PPSCTL_TRGTMODSEL2,
			BITM_EMAC_TM_PPSCTL_TRGTMODSEL3
	};

	/* Set the trigger mode */
	pDev->pEMAC_REGS->EMAC_TM_PPSCTL =  (pDev->pEMAC_REGS->EMAC_TM_PPSCTL & (~BITM_TRIGGER_MODE[nDeviceID]))
			                          | (TriggerMode << BITP_TRIGGER_MODE[nDeviceID]);
}

static void pps_issue_cmd (
		ADI_EMAC_DEVICE* const pDev,
		uint32_t nDeviceID,
		PPS_CMD  cmd
		)
{
	static volatile uint32_t BITP_CMD[] = {
			BITP_EMAC_TM_PPSCTL_PPSCTL,
			BITP_EMAC_TM_PPSCTL_PPSCMD1,
			BITP_EMAC_TM_PPSCTL_PPSCMD2,
			BITP_EMAC_TM_PPSCTL_PPSCMD3
	};

	static volatile uint32_t BITM_CMD[] = {
			BITM_EMAC_TM_PPSCTL_PPSCTL,
			BITM_EMAC_TM_PPSCTL_PPSCMD1,
			BITM_EMAC_TM_PPSCTL_PPSCMD2,
			BITM_EMAC_TM_PPSCTL_PPSCMD3
	};

	pDev->pEMAC_REGS->EMAC_TM_PPSCTL |= (cmd << BITP_CMD[nDeviceID]);

	/* Wait for the command to complete */
	while(pDev->pEMAC_REGS->EMAC_TM_PPSCTL & BITM_CMD[nDeviceID]) {}
}

static void pps_wait_for_target_time (
								       ADI_EMAC_DEVICE* const 	pDev,
								       uint32_t 				nDeviceID
								       )
{
	static volatile uint32_t* pNSecRegister[] = {
			pREG_EMAC0_TM_PPS0NTGTM,
			pREG_EMAC0_TM_PPS1NTGTM,
			pREG_EMAC0_TM_PPS2NTGTM,
			pREG_EMAC0_TM_PPS3NTGTM
	};

	/* Wait for the target time busy bit to clear */
	while (*pNSecRegister[nDeviceID] & BITM_EMAC_TM_PPS0NTGTM_TSTRBUSY) { }
}

static void pps_set_target_time (
		ADI_EMAC_DEVICE* const 	pDev,
		uint32_t 				nDeviceID,
		ADI_ETHER_TIME* 		pTime
		)
{
	static volatile uint32_t* pSecRegister[] = {
			pREG_EMAC0_TM_PPS0TGTM,
			pREG_EMAC0_TM_PPS1TGTM,
			pREG_EMAC0_TM_PPS2TGTM,
			pREG_EMAC0_TM_PPS3TGTM
	};


	static volatile uint32_t* pNSecRegister[] = {
			pREG_EMAC0_TM_PPS0NTGTM,
			pREG_EMAC0_TM_PPS1NTGTM,
			pREG_EMAC0_TM_PPS2NTGTM,
			pREG_EMAC0_TM_PPS3NTGTM
	};

	/* Configure the target time */
	*pSecRegister[nDeviceID] = pTime->LSecond;
	*pNSecRegister[nDeviceID] = pTime->NanoSecond;
}


#ifdef ADI_DEBUG
static int compare_time(ADI_ETHER_TIME* pTime1, ADI_ETHER_TIME* pTime2)
{
	int64_t nSec1, nSec2, nDiff;

	nSec1 = (int64_t)pTime1->LSecond + ((uint64_t)1 << 32u) * (int64_t)pTime1->HSecond;
	nSec2 = (int64_t)pTime2->LSecond + ((uint64_t)1 << 32u) * (int64_t)pTime2->HSecond;

	nDiff = nSec1 - nSec2;

	if (nDiff == 0) {
		nDiff = pTime1->NanoSecond - pTime2->NanoSecond;
	}

	if (nDiff < 0) {
		nDiff = -1;
	} else if (nDiff > 0) {
		nDiff = 1;
	}

	return nDiff;
}

static int compare_cur_time(ADI_EMAC_DEVICE* const pDev, ADI_ETHER_TIME* pTime)
{
	int64_t nSec1, nSec2, nDiff;

	nSec1 = (int64_t)pTime->LSecond + ((uint64_t)1 << 32u) * (int64_t)pTime->HSecond;
	nSec2 =   (int64_t)pDev->pEMAC_REGS->EMAC_TM_SEC
			+ (int64_t)pDev->pEMAC_REGS->EMAC_TM_HISEC * ((uint64_t)1 << 32u);

	nDiff = nSec1 - nSec2;

	if (nDiff == 0) {
		nDiff = (int64_t)pTime->NanoSecond - (int64_t)pDev->pEMAC_REGS->EMAC_TM_NSEC;
	}

	if (nDiff < 0) {
		nDiff = -1;
	} else if (nDiff > 0) {
		nDiff = 1;
	}

	return nDiff;
}
#endif

static ADI_ETHER_RESULT pps_config (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	uint32_t nDeviceID = (uint32_t)arg0;
	ADI_ETHER_GEMAC_PPS_CONFIG* pPPSConfig = (ADI_ETHER_GEMAC_PPS_CONFIG*) arg1;
	ADI_ETHER_CALLBACK_FN pfCallback = (ADI_ETHER_CALLBACK_FN) arg2;
	ADI_EMAC_PPS_ALARM_DEVICE* pPPSDevice = &pDev->PPS_Alarm_Devices[nDeviceID];


#ifdef ADI_DEBUG
	if (pPPSConfig == NULL) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}

	if (!pDev->PTPDevice.bEnabled) {
		return ADI_ETHER_RESULT_INVALID_SEQUENCE;
	}

	if (
		   /* Confirm that Start time is past the current time */
		   (compare_cur_time(pDev, &pPPSConfig->StartTime) <= 0)
		   /* Verify that end-time is past start time for fixed len mode */
		|| (    (pPPSConfig->ePulseMode ==  ADI_ETHER_GEMAC_PPS_PULSE_MODE_FIXED_LEN)
		     && (compare_time(&pPPSConfig->StartTime, &pPPSConfig->EndTime) >= 0))
		   /* Verify that Interval is greater than width */
		|| (	(pPPSConfig->ePulseMode != ADI_ETHER_GEMAC_PPS_PULSE_MODE_SINGLE)
			&&	(compare_time(&pPPSConfig->Interval, &pPPSConfig->Width) <= 0))
		)
	{
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif /* ADI_DEBUG */


	if (pPPSDevice->bConfigured) {
		if (pPPSDevice->bAlarm) {
			return ADI_ETHER_RESULT_DEVICE_IN_USE;
		}
		if (pPPSDevice->bEnabled) {
			return ADI_ETHER_RESULT_INVALID_SEQUENCE;
		}
	}


	/* Get the number of ns per PTP clock for interval */
	uint32_t NumNSPerPTPClk = (1000*1000*1000)/(pDev->PTPDevice.PTPClkFreq);

	/* Get the number of ns per PTP clock for width */
	uint64_t nWidthNS =    ((uint64_t)pPPSConfig->Width.LSecond * (uint64_t)(1000*1000*1000))
						+ ((uint64_t)pPPSConfig->Width.NanoSecond);
	uint64_t nWidthPTPCycles = nWidthNS/NumNSPerPTPClk;

#ifdef ADI_DEBUG
	if (nWidthPTPCycles > ((uint64_t)1 << 32)) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}
#endif

	if (pPPSConfig->ePulseMode != ADI_ETHER_GEMAC_PPS_PULSE_MODE_SINGLE)
	{
		uint64_t nIntervalNS =   ((uint64_t)pPPSConfig->Interval.LSecond * (uint64_t)(1000*1000*1000))
							   + ((uint64_t)pPPSConfig->Interval.NanoSecond);
		uint64_t nIntervalPTPCycles = nIntervalNS/NumNSPerPTPClk;

#ifdef ADI_DEBUG
		if (nIntervalPTPCycles > ((uint64_t)1 << 32)) {
			return ADI_ETHER_RESULT_INVALID_PARAM;
		}
#endif

		pPPSDevice->nInterval = nIntervalPTPCycles - 1;
	}

	/* Copy the required data inside */
	pPPSDevice->eTriggerMode = ((pPPSConfig->bGenerateInterrupt)
			  	  	  	  	  	  	  ? PPS_TRIG_MODE_INTERRUPT_PULSE :
			  	  	  	  	  	  	    PPS_TRIG_MODE_PULSE_ONLY);
	pPPSDevice->ePulseMode    = pPPSConfig->ePulseMode;
	pPPSDevice->StartTime     = pPPSConfig->StartTime;
	pPPSDevice->EndTime       = pPPSConfig->EndTime;
	pPPSDevice->pfCallback    = pfCallback;
	pPPSDevice->nWidth        = nWidthPTPCycles;

	/* Set the device as configured */
	pPPSDevice->bAlarm = false;
	pPPSDevice->bConfigured = true;
	pPPSDevice->bEnabled = false;
	pPPSDevice->bTriggerPending = false;

	return ADI_ETHER_RESULT_SUCCESS;
}

static ADI_ETHER_RESULT pps_enable (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	uint32_t nDeviceID = (uint32_t)arg0;
	bool bEnable = (bool)arg1;
	ADI_EMAC_PPS_ALARM_DEVICE* pPPSDevice = &pDev->PPS_Alarm_Devices[nDeviceID];

#ifdef ADI_DEBUG
	if (bEnable) {
		/* Confirm that Start time is past the current time */
		if (compare_cur_time(pDev, &pPPSDevice->StartTime) <= 0)
		{
			return ADI_ETHER_RESULT_FAILED;
		}
	}
#endif

	if (bEnable) {
		if (!pPPSDevice->bEnabled) {

			/* Enable Flexible PPS */
			pps_flexible_enable(pDev);


			/* Wait for the target time register */
			pps_wait_for_target_time(pDev, nDeviceID);

			/* Set the start time */
			pps_set_target_time (pDev, nDeviceID, &pPPSDevice->StartTime);

			/* Set the width */
			pps_set_width (pDev, nDeviceID, pPPSDevice->nWidth);

			if (pPPSDevice->ePulseMode != ADI_ETHER_GEMAC_PPS_PULSE_MODE_SINGLE) {
				/* Set the interval of the target time */
				pps_set_interval(pDev, nDeviceID, pPPSDevice->nInterval);
			}


			/* Set the trigger mode */
			pps_set_trigger_mode (pDev, nDeviceID, pPPSDevice->eTriggerMode);


			/* Issue the start command */
			pps_issue_cmd (pDev, nDeviceID,
					(pPPSDevice->ePulseMode == ADI_ETHER_GEMAC_PPS_PULSE_MODE_SINGLE) ?
							PPS_CMD_START_SINGLE_PULSE : PPS_CMD_START_PULSE_TRAIN
					);

 			if (pPPSDevice->ePulseMode == ADI_ETHER_GEMAC_PPS_PULSE_MODE_FIXED_LEN) {
				/* Set the end time */
				pps_wait_for_target_time(pDev, nDeviceID);
				pps_set_target_time (pDev, nDeviceID, &pPPSDevice->EndTime);
				pps_issue_cmd(pDev, nDeviceID, PPS_CMD_STOP_PULSE_AT_TIME);

				/* IF (Trigger is enabled) */
				if ((pPPSDevice->eTriggerMode == PPS_TRIG_MODE_INTERRUPT_ONLY) || (pPPSDevice->eTriggerMode == PPS_TRIG_MODE_INTERRUPT_PULSE))
				{
					/* Set the start time in the target register to get interrupt for the start of the pulse */
					pps_wait_for_target_time(pDev, nDeviceID);
					pps_set_target_time (pDev, nDeviceID, &pPPSDevice->StartTime);
				}


			}

 			/* Set the trigger interrupt if interrupt is enabled */
 			if ((pPPSDevice->eTriggerMode == PPS_TRIG_MODE_INTERRUPT_ONLY) || (pPPSDevice->eTriggerMode == PPS_TRIG_MODE_INTERRUPT_PULSE))
 			{
 				/* Enable Trigger Pending */
 				pPPSDevice->bTriggerPending = true;
 				pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSTRIG;
 			}

			/* Set the enabled flag as enabled */
			pPPSDevice->bEnabled = true;
		}
	} else {
		if (pPPSDevice->bEnabled) {

			/* Give the cancel start command */
			pps_issue_cmd(pDev, nDeviceID, PPS_CMD_CANCEL_START);

			/* Give STOP Pulse Train immediately command */
			pps_issue_cmd(pDev, nDeviceID, PPS_CMD_STOP_TRAIN_IMMEDIATELY);

			/* Set the enabled flag as disabled */
			pPPSDevice->bEnabled = false;

			/* Disable Trigger Pending */
			pPPSDevice->bTriggerPending = false;
		}
	}

	return ADI_ETHER_RESULT_SUCCESS;
}

	static ADI_ETHER_RESULT alarm_config (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	uint32_t nDeviceID = (uint32_t)arg0;
	ADI_ETHER_TIME *pTime = (ADI_ETHER_TIME*)arg1;
	ADI_ETHER_CALLBACK_FN pfCallback = (ADI_ETHER_CALLBACK_FN) arg2;
	ADI_EMAC_PPS_ALARM_DEVICE* pPPSDevice = &pDev->PPS_Alarm_Devices[nDeviceID];

#ifdef ADI_DEBUG
	if (pTime == NULL) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}

	if (!pDev->PTPDevice.bEnabled) {
		return ADI_ETHER_RESULT_INVALID_SEQUENCE;
	}
#endif

	if (pPPSDevice->bConfigured) {
		if (!pPPSDevice->bAlarm) {
			return ADI_ETHER_RESULT_DEVICE_IN_USE;
		}
		if (pPPSDevice->bEnabled) {
			return ADI_ETHER_RESULT_INVALID_SEQUENCE;
		}
	}

	/* Enable Flexible PPS */
	pps_flexible_enable(pDev);

	/* Copy the required data inside */
	pPPSDevice->StartTime = *pTime;
	pPPSDevice->pfCallback = pfCallback;

	/* Set the device as configured */
	pPPSDevice->bAlarm = true;
	pPPSDevice->bConfigured = true;
	pPPSDevice->bEnabled = false;
	pPPSDevice->bTriggerPending = false;

	return ADI_ETHER_RESULT_SUCCESS;
}



static ADI_ETHER_RESULT alarm_enable (
									ADI_EMAC_DEVICE*      const pDev,
									void*                       arg0,
									void*                       arg1,
									void*                       arg2
									)
{
	uint32_t nDeviceID = (uint32_t)arg0;
	bool bEnable = (bool)arg1;
	ADI_EMAC_PPS_ALARM_DEVICE* pPPSDevice = &pDev->PPS_Alarm_Devices[nDeviceID];

#ifdef ADI_DEBUG
	if (bEnable) {
		/* Confirm that alarm time is not past the current time */
		if (compare_cur_time(pDev, &pPPSDevice->StartTime) <= 0)
		{
			return ADI_ETHER_RESULT_FAILED;
		}
	}
#endif

	if (bEnable) {
		if (!pPPSDevice->bEnabled) {


			/* Wait for the target time register */
			pps_wait_for_target_time(pDev, nDeviceID);

			/* Set the alarm time */
			pps_set_target_time (pDev, nDeviceID, &pPPSDevice->StartTime);

			/* Enable Trigger Pending */
			pPPSDevice->bTriggerPending = true;

			/* Set the enabled flag as enabled */
			pPPSDevice->bEnabled = true;

			/* Set the trigger mode */
			pps_set_trigger_mode (
								  pDev,
								  nDeviceID,
								  PPS_TRIG_MODE_INTERRUPT_ONLY
								  );


			/* Issue the start command */
			pps_issue_cmd (pDev, nDeviceID, PPS_CMD_START_SINGLE_PULSE);

			/* Enable Trigger */
			pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSTRIG;
		}
	} else {
		if (pPPSDevice->bEnabled) {

			/* Issue the start command */
			pps_issue_cmd (
					pDev,
					nDeviceID,
					PPS_CMD_CANCEL_START
					);

			/* Set the enabled flag as disabled */
			pPPSDevice->bEnabled = false;

			/* Disable Trigger Pending */
			pPPSDevice->bTriggerPending = false;
		}
	}

	return ADI_ETHER_RESULT_SUCCESS;
}


ADI_ETHER_RESULT gemac_PPS_AlarmModuleIO (
										  ADI_EMAC_DEVICE*      const pDev,
										  ADI_ETHER_MODULE_FUNC const Func,
										  void*                       arg0,
										  void*                       arg1,
										  void*                       arg2
        								  )
{
	ADI_ETHER_RESULT eResult = ADI_ETHER_RESULT_SUCCESS;
	uint32_t nDeviceID = (uint32_t)arg0;

	if (!(pDev->Capability & ADI_EMAC_CAPABILITY_PPS)) {
		return ADI_ETHER_RESULT_NOT_SUPPORTED;
	}

	if (nDeviceID >= NUM_PPS_ALARM_DEVICES) {
		return ADI_ETHER_RESULT_INVALID_PARAM;
	}

	switch(Func)
	{
	case ADI_ETHER_MODULE_FUNC_PPS_CFG:
		eResult = pps_config(pDev,arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_PPS_EN:
		eResult = pps_enable(pDev,arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_ALARM_CFG:
		eResult = alarm_config(pDev,arg0, arg1, arg2);
		break;

	case ADI_ETHER_MODULE_FUNC_ALARM_EN:
		eResult = alarm_enable(pDev,arg0, arg1, arg2);
		break;

	default:
		eResult = ADI_ETHER_RESULT_NOT_SUPPORTED;
		break;
	}

	return eResult;
}

#endif
