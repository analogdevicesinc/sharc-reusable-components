/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#include <string.h>
#include <math.h>
#include <assert.h>

#include "context.h"
#include "codec_audio.h"
#include "usb_audio.h"
#include "cpu_load.h"

#include "sae.h"

/*
 *  Send the audio data by IPC to both SHARCs in parallel.  Add a
 *  a ref to the message for each SHARC to keep the message from being
 *  deallocated.
 */
static void sendMsg(SAE_CONTEXT *saeContext, SAE_MSG_BUFFER *msg)
{
    SAE_RESULT result;

    sae_refMsgBuffer(saeContext, msg);
    result = sae_sendMsgBuffer(saeContext, msg, IPC_CORE_SHARC0, true);
    if (result != SAE_RESULT_OK) {
        sae_unRefMsgBuffer(saeContext, msg);
    }
    sae_refMsgBuffer(saeContext, msg);
    result = sae_sendMsgBuffer(saeContext, msg, IPC_CORE_SHARC1, true);
    if (result != SAE_RESULT_OK) {
        sae_unRefMsgBuffer(saeContext, msg);
    }
}

void codecAudioOut(void *buffer, uint32_t maxSize, void *usrPtr)
{
    APP_CONTEXT *context = (APP_CONTEXT *)usrPtr;
    SAE_MSG_BUFFER *msg = NULL;
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    /* Get the IPC message associated with the data pointer */
    if (buffer == context->codecAudioOut[0]) {
        msg = context->codecMsgOut[0];
    } else if (buffer == context->codecAudioOut[1]) {
        msg = context->codecMsgOut[1];
    }
    assert(msg);

    /* Send the message to both SHARCs */
    sendMsg(context->saeContext, msg);

    /* Service USB Tx (IN) audio */
    msg = context->usbMsgTx[0];
    xferUsbTxAudio(context, msg);

    /* Send the message to both SHARCs */
    sendMsg(context->saeContext, msg);

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

void codecAudioIn(void *buffer, uint32_t size, void *usrPtr)
{
    APP_CONTEXT *context = (APP_CONTEXT *)usrPtr;
    SAE_MSG_BUFFER *msg = NULL;
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    /* Get the IPC message associated with the data pointer */
    if (buffer == context->codecAudioIn[0]) {
        msg = context->codecMsgIn[0];
    } else if (buffer == context->codecAudioIn[1]) {
        msg = context->codecMsgIn[1];
    }
    assert(msg);

    /* Send the message to both SHARCs */
    sendMsg(context->saeContext, msg);

    /* Service USB Rx (OUT) audio */
    msg = context->usbMsgRx[0];
    xferUsbRxAudio(context, msg);

    /* Send the message to both SHARCs */
    sendMsg(context->saeContext, msg);

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}
