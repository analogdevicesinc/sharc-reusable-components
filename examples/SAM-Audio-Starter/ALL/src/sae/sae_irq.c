/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

/* Standard includes */
#include <string.h>

/* CCES includes */
#if defined(__ADSPARM__)
#include <services/int/adi_gic.h>
#else
#include <services/int/adi_sec.h>
#endif
#include <services/int/adi_int.h>
#include <services/tru/adi_tru.h>
#include <sys/adi_core.h>

/* Module includes */
#include "sae_priv.h"
#include "sae_irq.h"
#include "sae_ipc.h"

static void sae_interruptHandler(uint32_t iid, void *handlerArg)
{
    SAE_CONTEXT *context = (SAE_CONTEXT *)handlerArg;
    SAE_MSG_BUFFER *buffer;
    SAE_RESULT result;

    do {
        result = sae_receiveMsgBuffer(context, &buffer);
        if (buffer) {
            if (buffer->msgType == MSG_TYPE_USER) {
                if (context->msgRxUsrCB) {
                    context->msgRxUsrCB(context, buffer, buffer->payload,
                        context->msgRxUsrPtr);
                }
            } else if (buffer->msgType == MSG_TYPE_STREAM) {
                if (context->msgRxStreamCB) {
                    context->msgRxStreamCB(context, buffer);
                }
            } else {
                sae_unRefMsgBuffer(context, buffer);
            }
        }
    } while (result == SAE_RESULT_OK);
}

SAE_RESULT sae_raiseInterrupt(SAE_CONTEXT *context, int8_t coreIdx)
{
    SAE_RESULT result = SAE_RESULT_OK;
    int32_t triggerMaster;

    triggerMaster = saeSharcArmIPC->idx2trigger[coreIdx];
    if (triggerMaster > 0) {
        adi_tru_RaiseTriggerMaster(triggerMaster);
    } else {
        result = SAE_RESULT_ERROR;
    }
    return(result);
}

static SAE_RESULT sae_configTRU(SAE_CONTEXT *context)
{
    /* Initialize and configure the TRU, without reset */
    adi_tru_Init(false);
    adi_tru_ConfigureSlave(TRGS_TRU0_IRQ3,  TRGM_SOFT3); /* SLV3  ARM (core 0)*/
    adi_tru_ConfigureSlave(TRGS_TRU0_IRQ7,  TRGM_SOFT4); /* SLV7  SHARC1 (core 1) */
    adi_tru_ConfigureSlave(TRGS_TRU0_IRQ11, TRGM_SOFT5); /* SLV11 SHARC2 (core 2) */
    return(SAE_RESULT_OK);
}

static SAE_RESULT sae_configIRQ(SAE_CONTEXT *context)
{
    ADI_INT_STATUS intStatus;
    SAE_RESULT result;
    uint32_t iid;

    /* Get the interrupt ID for this core */
    iid = sae_getInterruptID();
    if (iid <= 0) {
        return(SAE_RESULT_ERROR);
    }

    /* Set the interrupt to edge-sensitive for use with the TRU (default is level-sensitive) */
#if defined(__ADSPARM__)
    adi_gic_ConfigInt(iid, ADI_GIC_INT_EDGE_SENSITIVE, ADI_GIC_INT_HANDLING_MODEL_1_N);
#else
    adi_sec_EnableEdgeSense(iid, true);
#endif

    /* Install the TRU interrupt handler */
    intStatus = adi_int_InstallHandler(iid, sae_interruptHandler, context, true);

    /* Register this trigger master in the shared L2 area */
    result = (intStatus == ADI_INT_SUCCESS) ? SAE_RESULT_OK : SAE_RESULT_ERROR;
    if (result == SAE_RESULT_OK) {
        saeSharcArmIPC->idx2trigger[context->coreIdx] = TRGM_SOFT3 + context->coreID;
    }

    return(result);
}

SAE_RESULT sae_enableInterrupt(SAE_CONTEXT *context, bool ipcMaster)
{
    SAE_RESULT result = SAE_RESULT_OK;
    if (ipcMaster) {
        result = sae_configTRU(context);
    }
    if (result == SAE_RESULT_OK) {
        result = sae_configIRQ(context);
    }
    return(result);
}

uint32_t sae_getInterruptID(void)
{
    ADI_CORE_ID coreID;
    uint32_t iid;

    iid = 0;
    coreID = adi_core_id();
    switch (coreID)
    {
#if defined(__ADSPARM__)
        case ADI_CORE_ARM:
            iid = INTR_TRU0_INT3;
            break;
#endif 
        case ADI_CORE_SHARC0:
            iid = INTR_TRU0_INT7;
            break;
        case ADI_CORE_SHARC1:
            iid = INTR_TRU0_INT11;
            break;
        default:
            break;
    }
    return(iid);
}
