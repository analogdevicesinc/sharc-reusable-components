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

#include "sae.h"
#include "sae_priv.h"
#include "sae_ipc.h"
#include "sae_lock.h"
#include "sae_alloc.h"

void sae_lockIpc(void)
{
    /* Disable local interrupts */
    SAE_ENTER_CRITICAL();

    /* Get an exclusive lock on the IPC area */
    while (!sae_lock(&saeSharcArmIPC->lock));
}

void sae_unLockIpc(void)
{
    /* Unlock the dest IPC struct */
    sae_unlock(&saeSharcArmIPC->lock);

    /* Reenable local interrupts */
    SAE_EXIT_CRITICAL();
}

SAE_RESULT sae_broadcastEvent(SAE_CONTEXT *context, SAE_EVENT event, void *data, uint8_t len)
{
    SAE_MSG_BUFFER *msg;
    SAE_RESULT result, sendResult;
    void *payload;
    int i;

    /* Create a message buffer */
    msg = sae_createMsgBuffer(context, len, &payload);
    if ((msg == NULL) || (payload == NULL)) {
        return(SAE_RESULT_ERROR);
    }

    /* Set the fields */
    msg->msgType = MSG_TYPE_STREAM;
    msg->eventId = event;

    /* Copy the payload */
    SAE_MEMCPY(payload, data, len);

    /* Assume everything is OK */
    result = SAE_RESULT_OK;

    /* Dispatch the event to all cores */
    for (i = 0; i < IPC_MAX_CORES; i++) {
        sae_refMsgBuffer(context, msg);
        sendResult = sae_sendMsgBuffer(context, msg, i, true);
        if (sendResult != SAE_RESULT_OK) {
            sae_unRefMsgBuffer(context, msg);
            result = sendResult;
        }
    }

    /* Unref the message from myself, last recipient will free it. */
    sae_unRefMsgBuffer(context, msg);

    return(result);
}
