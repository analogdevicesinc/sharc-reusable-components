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

#ifndef _ipc_h
#define _ipc_h

#include <stdint.h>

#include "sae.h"

/*
 * IPC core identifiers
 */
#define IPC_CORE_ARM         SAE_CORE_IDX_0
#define IPC_CORE_SHARC0      SAE_CORE_IDX_1
#define IPC_CORE_SHARC1      SAE_CORE_IDX_2

/*
 * Number of message buffers to pre-allocate for audio IPC exchange.  This
 * is more than what is actually needed.
 */
#define USB_IPC_RX_MSGS  (4)
#define USB_IPC_TX_MSGS  (4)

/*
 * IPC message types
 */
enum IPC_TYPE {
    IPC_TYPE_UNKNOWN = 0,
    IPC_TYPE_PING,
    IPC_TYPE_AUDIO,
    IPC_TYPE_SHARC0_READY,
    IPC_TYPE_AUDIO_ROUTING,
    IPC_TYPE_CYCLES
};

/*
 * Audio stream identifiers (IPC_TYPE_AUDIO messages)
 */
enum IPC_STREAMID {
    IPC_STREAMID_UNKNOWN = 0,
    IPC_STREAMID_CODEC_IN,
    IPC_STREAMID_CODEC_OUT,
    IPC_STREAMID_A2B_IN,
    IPC_STREAMID_A2B_OUT,
    IPC_STREAMID_USB_RX,
    IPC_STREAMID_USB_TX,
    IPC_STREAM_ID_MAX
};

/*
 * Streaming audio data message (IPC_TYPE_AUDIO messages)
 */
#pragma pack(1)
typedef struct _IPC_MSG_AUDIO {
    uint8_t streamID;
    uint8_t numChannels;
    uint8_t numFrames;
    uint8_t wordSize;
    int32_t data[1];
} IPC_MSG_AUDIO;
#pragma pack()

/*
 * Routing Information (IPC_TYPE_AUDIO_ROUTING messages)
 */
#pragma pack(1)
typedef struct _ROUTE_INFO {
    uint8_t srcID;
    uint8_t sinkID;
    uint8_t srcOffset;
    uint8_t sinkOffset;
    uint8_t channels;
} ROUTE_INFO;

typedef struct _IPC_MSG_ROUTING {
    uint8_t numRoutes;
    uint8_t reserved[3];
    ROUTE_INFO routes[1];
} IPC_MSG_ROUTING;
#pragma pack()

/*
 * Streaming audio data message (IPC_TYPE_AUDIO messages)
 */
#pragma pack(1)
typedef struct _IPC_MSG_CYCLES {
    uint32_t core;
    uint32_t cycles;
} IPC_MSG_CYCLES;
#pragma pack()

/*
 * Generic message.  Query type to determine which union'd payload to
 * use.
 */
#pragma pack(1)
typedef struct _IPC_MSG {
    uint8_t type;
    uint8_t reserved[3];
    union {
        IPC_MSG_AUDIO audio;
        IPC_MSG_ROUTING routes;
        IPC_MSG_CYCLES cycles;
    };
} IPC_MSG;
#pragma pack()

/*
 * Convenience container to hold both the SAE message buffer and associated
 * IPC_MSG/IPC_MSG_AUDIO payload.  Used by cores with audio stream sources.
 */
typedef struct _USB_IPC_SRC_MSG {
    SAE_MSG_BUFFER *msgBuffer;
    IPC_MSG *msg;
} USB_IPC_SRC_MSG;

#endif
