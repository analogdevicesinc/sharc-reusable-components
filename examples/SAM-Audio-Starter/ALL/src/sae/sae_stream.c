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

#include <string.h>
#include <stdbool.h>

/* Module includes */
#include "sae.h"
#include "sae_priv.h"
#include "sae_ipc.h"
#include "sae_util.h"
#include "sae_alloc.h"

static void streamMsgRx(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg)
{
    if (context->eventUsrCB) {
        context->eventUsrCB(context, (SAE_EVENT)msg->eventId,
            msg->payload, context->eventUsrPtr);
    }
    sae_unRefMsgBuffer(context, msg);
}

static SAE_STREAM *sae_findStreamByName(char *name, SAE_STREAM **streamListLast)
{
    SAE_STREAM *streamList;

    streamList = saeSharcArmIPC->streamList;
    *streamListLast = (SAE_STREAM *)&saeSharcArmIPC->streamList;
    while (streamList) {
        if (strcmp(name, streamList->streamInfo.streamName) == 0) {
            break;
        }
        *streamListLast = streamList;
        streamList = streamList->next;
    }
    return(streamList);
}

static SAE_STREAM *sae_findStream(SAE_STREAM *stream, SAE_STREAM **streamListLast)
{
    SAE_STREAM *streamList;

    streamList = saeSharcArmIPC->streamList;
    *streamListLast = (SAE_STREAM *)&saeSharcArmIPC->streamList;
    while (streamList) {
        if (streamList == stream) {
            break;
        }
        *streamListLast = streamList;
        streamList = streamList->next;
    }
    return(streamList);
}

SAE_RESULT sae_registerStream(SAE_CONTEXT *context, char *streamName, SAE_STREAM **streamPtr)
{
    SAE_RESULT result = SAE_RESULT_OK;
    SAE_STREAM *streamListLast;
    SAE_STREAM *stream;
    int len;

    /* Sanity check */
    if ((streamName == NULL) || (SAE_STRLEN(streamName) == 0)) {
        return(SAE_RESULT_BAD_STREAM_NAME);
    }

    /* Null out the incoming stream pointer */
    if (streamPtr) {
        *streamPtr = NULL;
    }

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    /* Make sure the stream does not already exist */
    stream = sae_findStreamByName(streamName, &streamListLast);
    if (stream) {
        sae_unLockIpc();
        return(SAE_RESULT_STREAM_EXISTS);
    }

    /* Allocate and add the stream to the list */
    stream = sae_malloc(sizeof(*stream));
    if (stream == NULL) {
        sae_unLockIpc();
        return(SAE_RESULT_NO_MEM);
    }

    /* Initialize the new stream */
    SAE_MEMSET(stream, 0, sizeof(*stream));
    len = SAE_STRLEN(streamName) + 1;
    stream->streamInfo.streamName = sae_malloc(len);
    SAE_MEMSET(stream->streamInfo.streamName, 0, len);
    SAE_STRCPY(stream->streamInfo.streamName, streamName);

    /* Add the new stream to the list */
    if (streamListLast == (SAE_STREAM *)&saeSharcArmIPC->streamList) {
        saeSharcArmIPC->streamList = stream;
    } else {
        streamListLast->next = stream;
    }

    /* Unlock the IPC area */
    sae_unLockIpc();

    /* Send an event to all cores */
    sae_broadcastEvent(context, SAE_EVENT_ADD_STREAM,
        stream->streamInfo.streamName, len);

    /* Return a reference to the new stream */
    if (streamPtr) {
        *streamPtr = stream;
    }

    return(result);
}

SAE_RESULT sae_unRegisterStream(SAE_CONTEXT *context, SAE_STREAM **streamPtr)
{
    SAE_RESULT result = SAE_RESULT_OK;
    SAE_STREAM *streamListLast;
    SAE_STREAM *stream;

    /* Sanity check */
    if (streamPtr == NULL) {
        return(SAE_RESULT_BAD_STREAM);
    }

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    /* Find the stream in the list */
    stream = sae_findStream(*streamPtr, &streamListLast);
    if (stream == NULL) {
        sae_unLockIpc();
        return(SAE_RESULT_BAD_STREAM);
    }

    /* Unlink the stream from the list */
    if (streamListLast == (SAE_STREAM *)&saeSharcArmIPC->streamList) {
        saeSharcArmIPC->streamList = stream->next;
    } else {
        streamListLast->next = stream->next;
    }

    /* Unlock the IPC area */
    sae_unLockIpc();

    /* Send an event to all cores */
    sae_broadcastEvent(context, SAE_EVENT_REMOVE_STREAM,
        stream->streamInfo.streamName,
        SAE_STRLEN(stream->streamInfo.streamName) + 1);

    /* Free the stream */
    sae_free(stream->streamInfo.streamName);
    sae_free(stream);

    /* Null out the stream pointer for convenience */
    *streamPtr = NULL;

    return(result);
}

SAE_RESULT sae_initStream(SAE_CONTEXT *context)
{
    SAE_RESULT result = SAE_RESULT_OK;
    context->msgRxStreamCB = streamMsgRx;
    return(result);
}

SAE_RESULT sae_deInitStream(SAE_CONTEXT *context)
{
    SAE_RESULT result = SAE_RESULT_OK;
    context->msgRxStreamCB = NULL;
    return(result);
}
