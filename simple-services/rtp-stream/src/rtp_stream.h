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

#ifndef _rtp_stream_h
#define _rtp_stream_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "compat/posix/sys/socket.h"

typedef struct RTP_STREAM {
    bool enabled;
    void *lock;
    unsigned channels;
    unsigned wordSizeBytes;
    bool isRx;
    int socket;
    int port;
    char *ipStr;
    struct sockaddr_in ipAddr;
    void *pkt;
    uint8_t *data;
    uint32_t timeStamp;
    uint16_t sequence;
    unsigned size;
    unsigned samples;
    unsigned maxSamples;
    unsigned maxFrames;
    bool preRoll;
} RTP_STREAM;

bool openRtpStream(RTP_STREAM *rs);
void closeRtpStream(RTP_STREAM *rs);

unsigned rtpWriteSamplesAvailable(RTP_STREAM *rs, void **data);
unsigned rtpWriteSamples(RTP_STREAM *rs, unsigned samples);

unsigned rtpReadPkt(RTP_STREAM *rs);
unsigned rtpReadSamplesAvailable(RTP_STREAM *rs, void **data);
unsigned rtpReadSamples(RTP_STREAM *rs, unsigned samples);

#endif
