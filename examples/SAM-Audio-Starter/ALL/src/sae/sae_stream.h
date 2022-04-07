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

#ifndef _sae_stream_h
#define _sae_stream_h

//#include "sae.h"

/*!****************************************************************
 * @brief SHARC Audio Engine stream event callback
 ******************************************************************/
typedef void (*SAE_STREAM_EVENT_CALLBACK)(SAE_CONTEXT *context, void *usrPtr);

/*!****************************************************************
 * @brief Opaque SHARC Audio Engine stream object
 ******************************************************************/
typedef struct _SAE_STREAM SAE_STREAM;

#pragma pack(1)
typedef struct _SAE_STREAM_INFO {
    char *streamName;
    uint32_t sampleRate;
    uint16_t blockSize;
    uint16_t channels;
    uint8_t sampleSizeBytes;
    uint8_t sampleFormat;
} SAE_STREAM_INFO;
#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

SAE_RESULT sae_initStream(SAE_CONTEXT *context);

SAE_RESULT sae_registerStream(SAE_CONTEXT *context, char *name, SAE_STREAM **stream);
SAE_RESULT sae_unRegisterStream(SAE_CONTEXT *context, SAE_STREAM **stream);

SAE_RESULT sae_subscribeStream(SAE_CONTEXT *context, SAE_STREAM *stream,
    SAE_STREAM_EVENT_CALLBACK cb);

SAE_RESULT sae_publishStream(SAE_CONTEXT *context, SAE_STREAM *stream);
SAE_RESULT sae_publishStreamMsgBuffer(SAE_CONTEXT *context, SAE_STREAM *stream);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
