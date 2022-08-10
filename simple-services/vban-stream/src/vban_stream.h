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

#ifndef _vban_stream_h
#define _vban_stream_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "compat/posix/sys/socket.h"

typedef struct VBAN_STREAM {
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
    uint32_t sequence;
    unsigned size;
    unsigned samples;
    unsigned maxSamples;
    unsigned maxFrames;
    uint8_t format_SR;
    unsigned systemSampleRate;
    unsigned streamChannels;
    bool sync;
    bool preRoll;
} VBAN_STREAM;

bool vbanOpenStream(VBAN_STREAM *rs);
void vbanCloseStream(VBAN_STREAM *rs);

unsigned vbanWriteSamplesAvailable(VBAN_STREAM *rs, void **data);
unsigned vbanWriteSamples(VBAN_STREAM *rs, unsigned samples);

unsigned vbanReadPkt(VBAN_STREAM *rs);
unsigned vbanReadSamplesAvailable(VBAN_STREAM *rs, void **data);
unsigned vbanReadSamples(VBAN_STREAM *rs, unsigned samples);

#endif
