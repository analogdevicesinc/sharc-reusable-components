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
#include <stdbool.h>

/* ADI system services includes */
#include <sys/adi_core.h>

/* Module includes */
#include "sae_priv.h"
#include "sae_ipc.h"
#include "sae_irq.h"
#include "sae_lock.h"
#include "sae_alloc.h"
#include "sae_util.h"

/* Identify the entire MCAPI L2 memory region.
 *
 * These section variables come from the processor's .ld[f] file.
 * The variables defined in SHARC .ldf files are slightly different
 * than those in ARM .ld files.
 *
 * Sanitize everything to look like the ARM side.
 */
#if defined(__ADSPARM__)
#define DECL extern
#else
#define __MCAPI_common_start ___MCAPI_common_start
#define __MCAPI_sharc0_end   ___MCAPI_sharc0_end
#define __MCAPI_sharc1_end   ___MCAPI_sharc1_end
#define DECL extern "asm"
#endif

DECL int __MCAPI_common_start[1];
DECL int __MCAPI_sharc0_end[1];
DECL int __MCAPI_sharc1_end[1];

SAE_SHARC_ARM_IPC *saeSharcArmIPC = (SAE_SHARC_ARM_IPC *)__MCAPI_common_start;
SAE_CONTEXT SAE_GLOBAL_CONTEXT;

SAE_RESULT sae_unInitialize(SAE_CONTEXT **contextPtr)
{
    SAE_RESULT result = SAE_RESULT_OK;

    /* Make sure a context pointer has been passed in */
    if (contextPtr == NULL) {
        return(SAE_RESULT_ERROR);
    }

    *contextPtr = NULL;

    return(result);
}

SAE_RESULT sae_initialize(SAE_CONTEXT **contextPtr, SAE_CORE_IDX coreIdx, bool ipcMaster)
{
    SAE_RESULT result = SAE_RESULT_OK;
    SAE_CONTEXT *context;
    int *MCAPI_end;
    unsigned MCAPI_SIZE;

    /* Some LDR files have SHARC0 as the last MCAPI segment, some have
     * SHARC1 as the last MCAPI segment.  In all cases, the memory
     * is contiguous with the COMMON and ARM as the first two segments.
     * Determine the end by whichever SHARC has the highest address.
     */
    MCAPI_end = __MCAPI_sharc0_end > __MCAPI_sharc1_end ?
        __MCAPI_sharc0_end : __MCAPI_sharc1_end;

    MCAPI_SIZE = MCAPI_end - __MCAPI_common_start;

    /*
     * SHARC .ldf uses ___MCAPI_sharc0_end = MEMORY_END()
     * so the actual size is off by one.
     */
#if !defined(__ADSPARM__)
    MCAPI_SIZE += 1;
#endif

    /* The ld/ldr macros are in words, convert to bytes */
    MCAPI_SIZE *= sizeof(*__MCAPI_common_start);

    /* Make sure a context pointer has been passed in */
    if (contextPtr == NULL) {
        return(SAE_RESULT_ERROR);
    }

    /* Initialize the the ARM / SHARC IPC core struct and heap */
    if (ipcMaster) {
        SAE_MEMSET(saeSharcArmIPC, 0, sizeof(*saeSharcArmIPC));
        saeSharcArmIPC->lock = SAE_SHARC_ARM_IPC_UNLOCKED;
        sae_heapInit(saeSharcArmIPC->heap, MCAPI_SIZE - sizeof(*saeSharcArmIPC) + 1);
    } else {
        sae_heapInit(saeSharcArmIPC->heap, 0);
    }

    /* Get a reference to the global context */
    context = &SAE_GLOBAL_CONTEXT;

    /* Initialize the context */
    SAE_MEMSET(context, 0, sizeof(*context));
    context->coreIdx = coreIdx;
    context->coreID = adi_core_id();

    /* Enable the interrupt */
    sae_enableInterrupt(context, ipcMaster);

    /* Return the context */
    *contextPtr = context;

    return(result);
}

size_t sae_getMsgBufferSize(SAE_MSG_BUFFER *msg)
{
    return((size_t)msg->size);
}

void *sae_getMsgBufferPayload(SAE_MSG_BUFFER *msg)
{
    return(msg->payload);
}


SAE_MSG_BUFFER *sae_createMsgBuffer(SAE_CONTEXT *context, size_t size, void **payload)
{
    SAE_MSG_BUFFER *msg = NULL;

    /* Allocate a new message */
    msg = sae_safeMalloc(sizeof(*msg) + size);

    /* Initialize the new message buffer */
    if (msg) {
        SAE_MEMSET(msg, 0, sizeof(*msg));
        msg->ref = 1;
        msg->msgType = MSG_TYPE_USER;
        msg->payload = (void *)((uint8_t *)msg + sizeof(*msg));
        msg->size = (uint32_t)size;
        if (payload) {
            *payload = msg->payload;
        }
    }

    return(msg);
}

SAE_RESULT sae_refMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg)
{
    /* Don't allow reference counter to wrap back to zero */
    if (msg->ref == 0xFF) {
        return(SAE_RESULT_REFERENCE_ERROR);
    }

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    msg->ref++;

    /* Unlock the IPC area */
    sae_unLockIpc();

    return(SAE_RESULT_OK);
}

SAE_RESULT sae_unRefMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg)
{
    /* only decrement if reference counter is greater than zero */
    if (!msg->ref) {
        return(SAE_RESULT_REFERENCE_ERROR);
    }

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    msg->ref--;
    if (msg->ref == 0) {
        sae_free(msg);
    }

    /* Unlock the IPC area */
    sae_unLockIpc();

    return(SAE_RESULT_OK);
}

static bool sae_queueFull(SAE_IPC_MSG_QUEUE *msgQueue)
{
    return (((msgQueue->head + 1) & (IPC_MAX_MSG_QUEUE_SIZE - 1)) == msgQueue->tail);
}

static bool sae_queueEmpty(SAE_IPC_MSG_QUEUE *msgQueue)
{
    return (msgQueue->head == msgQueue->tail);
}

static SAE_RESULT sae_queueMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg,
    uint8_t dstCoreIdx)
{
    SAE_IPC_MSG_QUEUE *msgQueue = &saeSharcArmIPC->msgQueues[dstCoreIdx];
    SAE_RESULT result = SAE_RESULT_OK;

    /* Ensure the destination has been initialized and is able to receive
     * messages and interrupts.
     */
    if (saeSharcArmIPC->idx2trigger[dstCoreIdx] == 0) {
        return(SAE_RESULT_CORE_NOT_READY);
    }

    if (!sae_queueFull(msgQueue)) {
        msgQueue->queue[msgQueue->head] = (void *)msg;
        msgQueue->head = ((msgQueue->head + 1) & (IPC_MAX_MSG_QUEUE_SIZE - 1));
    } else {
        result = SAE_RESULT_QUEUE_FULL;
    }

    return(result);
}

static SAE_RESULT sae_dequeueMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER **msg)
{
    SAE_IPC_MSG_QUEUE *msgQueue = &saeSharcArmIPC->msgQueues[context->coreIdx];
    SAE_RESULT result = SAE_RESULT_OK;

    if (!sae_queueEmpty(msgQueue)) {
        *msg = (SAE_MSG_BUFFER *)msgQueue->queue[msgQueue->tail];
        msgQueue->tail = ((msgQueue->tail + 1) & (IPC_MAX_MSG_QUEUE_SIZE - 1));
    } else {
        *msg = NULL;
        result = SAE_RESULT_QUEUE_EMPTY;
    }

    return(result);
}

SAE_RESULT sae_receiveMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER **msg)
{
    SAE_RESULT result = SAE_RESULT_OK;

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    /* Get the message from the message queue */
    result = sae_dequeueMsgBuffer(context, msg);

    /* Unlock the IPC area */
    sae_unLockIpc();

    return(result);
}

SAE_RESULT sae_sendMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg,
    uint8_t dstCoreIdx, bool signalDstCore)
{
    SAE_RESULT result = SAE_RESULT_OK;

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    /* Fill out source information */
    msg->srcCoreIdx = context->coreIdx;

    /* Put the message on the destination's message queue */
    result = sae_queueMsgBuffer(context, msg, dstCoreIdx);

    /* Unlock the IPC area */
    sae_unLockIpc();

    /* Signal the other core */
    if ((result == SAE_RESULT_OK) && signalDstCore) {
        result = sae_raiseInterrupt(context, dstCoreIdx);
    }

    return(result);
}

SAE_RESULT sae_registerMsgReceivedCallback(SAE_CONTEXT *context,
    SAE_MSG_RECEIVED_CALLBACK cb, void *usrPtr)
{
    context->msgRxUsrCB = cb;
    context->msgRxUsrPtr = usrPtr;
    return(SAE_RESULT_OK);
}

SAE_RESULT sae_registerEventCallback(SAE_CONTEXT *context,
    SAE_EVENT_CALLBACK cb, void *usrPtr)
{
    context->eventUsrCB = cb;
    context->eventUsrPtr = usrPtr;
    return(SAE_RESULT_OK);
}

SAE_RESULT sae_heapInfo(SAE_CONTEXT *context, SAE_HEAP_INFO *heapInfo)
{
    SAE_RESULT result;
    bool ok;
    ok = sae_safeHeapInfo(heapInfo);
    result = ok ? SAE_RESULT_OK : SAE_RESULT_CORRUPT_HEAP;
    return(result);
}
