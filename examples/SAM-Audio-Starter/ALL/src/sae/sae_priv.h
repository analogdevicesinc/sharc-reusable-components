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

#ifndef _sae_priv_h
#define _sae_priv_h

#include "sae.h"
#include "sae_stream.h"

/*!****************************************************************
 * @brief SHARC Audio Engine message received callback
 ******************************************************************/
typedef void (*SAE_STREAM_MSG_RECEIVED_CALLBACK)(SAE_CONTEXT *context, SAE_MSG_BUFFER *buffer);

enum {
    MSG_TYPE_UNKNOWN = 0,
    MSG_TYPE_USER,
    MSG_TYPE_STREAM
};

#pragma pack(1)
struct _SAE_MSG_BUFFER {
    uint8_t ref;                /**< Buffer reference count. */
    uint8_t srcCoreIdx;         /**< Source core index. */
    uint8_t msgType;            /**< Message type */
    uint8_t eventId;            /**< Event ID */
    uint32_t size;              /**< Size of the allocated message */
    void *payload;              /**< Pointer to the allocated message payload */
};
#pragma pack()

#pragma pack(1)
typedef struct _SAE_SUBSCRIBER_INFO {
    uint8_t coreIdx;
    void *audioCallBack;
    void *usrPtr;
} SAE_SUBSCRIBER_INFO;
#pragma pack()

#pragma pack(1)
struct _SAE_STREAM {
    uint8_t coreIdx;
    void *audioCallBack;
    void *usrPtr;
    SAE_STREAM_INFO streamInfo;
    void *next;
};
#pragma pack()

struct _SAE_CONTEXT {
    int8_t coreIdx;
    uint8_t coreID;
    SAE_STREAM_MSG_RECEIVED_CALLBACK msgRxStreamCB;
    SAE_MSG_RECEIVED_CALLBACK msgRxUsrCB;
    SAE_EVENT_CALLBACK eventUsrCB;
    void *msgRxUsrPtr;
    void *eventUsrPtr;
};

#endif
