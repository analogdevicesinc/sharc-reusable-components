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

/*
 * For more information regarding the VBAN protocol, please visit this
 * website: https://vb-audio.com/Services/support.htm#VBAN
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "compat/posix/sys/socket.h"

#include "vban_stream_cfg.h"
#include "vban_stream.h"

#ifndef VBAN_MALLOC
#define VBAN_MALLOC malloc
#endif

#ifndef VBAN_FREE
#define VBAN_FREE free
#endif

#ifndef VBAN_STREAM_NAME
#define VBAN_STREAM_NAME  "SAM"
#endif

#define VBAN_MAX_PACKET_SIZE  (1436)
#define VBAN_MAX_PAYLOAD_SIZE (VBAN_MAX_PACKET_SIZE - sizeof(VBAN_PKT_HDR))
#define VBAN_MAX_FRAMES       (256)

#define VBAN_FOURC        0x4E414256 // 'NABV'

#define VBAN_PROTOCOL_MASK        0xE0
#define VBAN_PROTOCOL_AUDIO       0x00
#define VBAN_PROTOCOL_SERIAL      0x20
#define VBAN_PROTOCOL_TXT         0x40
#define VBAN_PROTOCOL_SERVICE     0x60
#define VBAN_PROTOCOL_UNDEFINED_1 0x80
#define VBAN_PROTOCOL_UNDEFINED_2 0xA0
#define VBAN_PROTOCOL_UNDEFINED_3 0xC0
#define VBAN_PROTOCOL_USER        0xE0

#define VBAN_DATATYPE_MASK        0x07
#define VBAN_DATATYPE_BYTE8       0x00
#define VBAN_DATATYPE_INT16       0x01
#define VBAN_DATATYPE_INT24       0x02
#define VBAN_DATATYPE_INT32       0x03
#define VBAN_DATATYPE_FLOAT32     0x04
#define VBAN_DATATYPE_FLOAT64     0x05
#define VBAN_DATATYPE_12BITS      0x06
#define VBAN_DATATYPE_10BITS      0x07

#define VBAN_CODEC_MASK           0xF0
#define VBAN_CODEC_PCM            0x00
#define VBAN_CODEC_VBCA           0x10 //VB-AUDIO AOIP CODEC
#define VBAN_CODEC_VBCV           0x20 //VB-AUDIO VOIP CODEC
#define VBAN_CODEC_UNDEFINED_1    0x30
#define VBAN_CODEC_UNDEFINED_2    0x40
#define VBAN_CODEC_UNDEFINED_3    0x50
#define VBAN_CODEC_UNDEFINED_4    0x60
#define VBAN_CODEC_UNDEFINED_5    0x70
#define VBAN_CODEC_UNDEFINED_6    0x80
#define VBAN_CODEC_UNDEFINED_7    0x90
#define VBAN_CODEC_UNDEFINED_8    0xA0
#define VBAN_CODEC_UNDEFINED_9    0xB0
#define VBAN_CODEC_UNDEFINED_10   0xC0
#define VBAN_CODEC_UNDEFINED_11   0xD0
#define VBAN_CODEC_UNDEFINED_12   0xE0
#define VBAN_CODEC_USER           0xF0

#define VBAN_SR_MAXNUMBER 21
static long VBAN_SRList[VBAN_SR_MAXNUMBER]= {
    6000, 12000, 24000, 48000, 96000, 192000, 384000,
    8000, 16000, 32000, 64000, 128000, 256000, 512000,
    11025, 22050, 44100, 88200, 176400, 352800, 705600
};

#pragma pack(1)
typedef struct _VBAN_PKT_HDR {
    uint32_t vban;         /* contains 'V' 'B', 'A', 'N' */
    uint8_t format_SR;     /* SR index (see SRList above) */
    uint8_t format_nbs;    /* nb sample per frame (1 to 256) */
    uint8_t format_nbc;    /* nb channel (1 to 256) */
    uint8_t format_bit;    /* mask = 0x07 (nb Byte integer from 1 to 4) */
    char streamname[16];   /* stream name */
    uint32_t nuFrame;      /* growing frame number. */
    uint8_t data[];
} VBAN_PKT_HDR;
#pragma pack()

static int vbanGetSrIdx(long sampleRate)
{
    int i;
    int idx;

    idx = -1;
    for (i = 0; i < VBAN_SR_MAXNUMBER; i++) {
        if (VBAN_SRList[i] == sampleRate) {
            idx = i;
            break;
        }
    }

    return(idx);
}

static int vbanGetFmt(int wordSizeBytes)
{
    int fmt = -1;
    switch (wordSizeBytes) {
        case 2:
            fmt = VBAN_DATATYPE_INT16;
            break;
        case 3:
            fmt = VBAN_DATATYPE_INT24;
            break;
        case 4:
            fmt = VBAN_DATATYPE_INT32;
            break;
        default:
            break;
    }
    return(fmt);
}

bool vbanOpenStream(VBAN_STREAM *rs)
{
    struct sockaddr_in srcAddr;
    VBAN_PKT_HDR *hdr;
    bool ok = false;
    int ret;

    memset(&rs->ipAddr, 0, sizeof(rs->ipAddr));
    rs->ipAddr.sin_family = AF_INET;
    rs->ipAddr.sin_len = sizeof(rs->ipAddr);
    ok = inet_aton(rs->ipStr, &rs->ipAddr.sin_addr.s_addr);
    if (!ok) {
        goto abort;
    }
    rs->ipAddr.sin_port = htons(rs->port);

    memset(&srcAddr, 0, sizeof(srcAddr));
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_len = sizeof(srcAddr);
    srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srcAddr.sin_port = htons(rs->port);

    rs->socket = socket(AF_INET, SOCK_DGRAM, 0);
    ok = (rs->socket >= 0);
    if (!ok) {
        goto abort;
    }

    ret = bind(rs->socket, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
    ok = (ret == 0);
    if (!ok) {
        goto abort;
    }

    rs->pkt = VBAN_MALLOC(VBAN_MAX_PACKET_SIZE);
    memset(rs->pkt, 0, VBAN_MAX_PACKET_SIZE);

    hdr = (VBAN_PKT_HDR *)rs->pkt;
    rs->data = hdr->data;
    rs->format_SR = vbanGetSrIdx(rs->systemSampleRate) | VBAN_PROTOCOL_AUDIO;

    if (rs->isRx) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        setsockopt(rs->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        rs->preRoll = true;
    } else {
        hdr->vban = VBAN_FOURC;
        hdr->format_SR = rs->format_SR;
        hdr->format_bit = vbanGetFmt(rs->wordSizeBytes) | VBAN_CODEC_PCM;
        hdr->format_nbc = rs->channels - 1;
        strcpy(hdr->streamname, VBAN_STREAM_NAME);
        rs->sequence = rand();
        rs->maxSamples = (VBAN_MAX_PACKET_SIZE - sizeof(*hdr)) / rs->wordSizeBytes;
        rs->maxSamples = (rs->maxSamples / rs->channels) * rs->channels;
        rs->maxFrames = rs->maxSamples / rs->channels;
        if (rs->maxFrames > VBAN_MAX_FRAMES) {
            rs->maxFrames = VBAN_MAX_FRAMES;
            rs->maxSamples = rs->maxFrames * rs->channels;
        }
        rs->size = sizeof(*hdr) + rs->maxFrames * rs->channels * rs->wordSizeBytes;
    }

    rs->samples = 0;
    rs->enabled = true;

    ok = true;

abort:
    return(ok);
}

unsigned vbanWriteSamplesAvailable(VBAN_STREAM *rs, void **data)
{
    if (data) {
        *data = (void *)rs->data;
    }
    return(rs->maxSamples - rs->samples);
}

unsigned vbanWriteSamples(VBAN_STREAM *rs, unsigned samples)
{
    VBAN_PKT_HDR *hdr = (VBAN_PKT_HDR *)rs->pkt;

    rs->samples += samples;
    rs->data += samples * rs->wordSizeBytes;

    if (rs->samples == rs->maxSamples) {
        hdr->format_nbs = rs->maxFrames - 1;
        hdr->nuFrame = rs->sequence;
        sendto(rs->socket, hdr, rs->size, 0, (struct sockaddr *)&rs->ipAddr, sizeof(rs->ipAddr));
        rs->data = hdr->data;
        rs->samples = 0;
        rs->sequence += 1;
    }

    return(samples);
}

static bool vbanPktOk(VBAN_STREAM *rs, VBAN_PKT_HDR *hdr, unsigned size)
{
    bool ok = false;

    /* Check FOURC magic */
    if (hdr->vban != VBAN_FOURC) {
        goto abort;
    }

    /* Confirm compatible PCM sample rate */
    if (hdr->format_SR != rs->format_SR) {
        goto abort;
    }

    /* We only know PCM audio */
    if ((hdr->format_bit & VBAN_CODEC_MASK) != VBAN_CODEC_PCM) {
        goto abort;
    }

    /* We only know 16-bit and 32-bit audio */
    if ((hdr->format_bit & VBAN_DATATYPE_MASK) == VBAN_DATATYPE_INT32) {
        rs->wordSizeBytes = 4;
    } else if ((hdr->format_bit & VBAN_DATATYPE_MASK) == VBAN_DATATYPE_INT16) {
        rs->wordSizeBytes = 2;
    } else {
        goto abort;
    }

#if 0
    /* Confirm sequence */
    if (rs->sync) {
        if (hdr->nuFrame == rs->sequence) {
            rs->sequence++;
        } else {
            rs->sync = false;
            goto abort;
        }
    } else {
        rs->sequence = hdr->nuFrame + 1;
        rs->sync = true;
        goto abort;
    }
#endif

    /* Extract other relevant info */
    rs->size = size;
    rs->data = hdr->data;
    rs->maxSamples = (size - sizeof(*hdr)) / rs->wordSizeBytes;
    rs->maxFrames = hdr->format_nbs + 1;
    rs->streamChannels = hdr->format_nbc + 1;
    rs->samples = rs->maxFrames * rs->streamChannels;

    /* Confirm packet size */
    if (rs->samples != rs->maxSamples) {
        goto abort;
    }

    ok = true;

abort:
    return(ok);
}

unsigned vbanReadSamplesAvailable(VBAN_STREAM *rs, void **data)
{
    VBAN_PKT_HDR *hdr = (VBAN_PKT_HDR *)rs->pkt;
    ssize_t size = 0;

    if (rs->samples == 0) {
        socklen_t addrLen = sizeof(rs->ipAddr);
        size = recvfrom(
            rs->socket, rs->pkt, VBAN_MAX_PACKET_SIZE, 0,
            (struct sockaddr *)&rs->ipAddr, &addrLen
        );
        if (size < (ssize_t)sizeof(*hdr)) {
            return(0);
        }
        if (!vbanPktOk(rs, hdr, (unsigned)size)) {
            return(0);
        }
    }

    if (data) {
        *data = (void *)rs->data;
    }

    return(rs->samples);
}

unsigned vbanReadSamples(VBAN_STREAM *rs, unsigned samples)
{
    rs->samples -= samples;
    rs->data += samples * rs->wordSizeBytes;
    return(rs->samples);
}

void vbanCloseStream(VBAN_STREAM *rs)
{
    if (rs->socket >= 0) {
        lwip_close(rs->socket);
    }
    if (rs->pkt) {
        VBAN_FREE(rs->pkt);
        rs->pkt = NULL;
    }
    rs->socket = -1;
    rs->enabled = false;
}
