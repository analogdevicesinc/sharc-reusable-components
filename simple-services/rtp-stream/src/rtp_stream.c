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

#include <stdbool.h>
#include <stdlib.h>

#include "compat/posix/sys/socket.h"

#include "rtp_stream_cfg.h"
#include "rtp_stream.h"

#ifndef RTP_MALLOC
#define RTP_MALLOC malloc
#endif

#ifndef RTP_FREE
#define RTP_FREE free
#endif

#pragma pack(1)
typedef struct _RTP_PKT_HDR {
    uint8_t flags;
    uint8_t type;
    uint16_t sequence;
    uint32_t timeStamp;
    uint32_t ssrc;
    uint8_t data[];
} RTP_PKT_HDR;
#pragma pack()

#define RTP_MAX_PACKET_SIZE  (1472)
#define RTP_MAX_PAYLOAD_SIZE (RTP_MAX_PACKET_SIZE - sizeof(RTP_PKT_HDR))

bool openRtpStream(RTP_STREAM *rs)
{
    struct sockaddr_in srcAddr;
    RTP_PKT_HDR *hdr;
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

    rs->pkt = RTP_MALLOC(RTP_MAX_PACKET_SIZE);
    memset(rs->pkt, 0, RTP_MAX_PACKET_SIZE);

    hdr = (RTP_PKT_HDR *)rs->pkt;
    rs->data = hdr->data;

    if (rs->isRx) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        setsockopt(rs->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        rs->preRoll = true;
    } else {
        hdr->flags = 0x80;
        hdr->type = 96;
        hdr->ssrc = htonl(rand());
        rs->sequence = rand();
        rs->timeStamp = 0;
        rs->maxSamples = (RTP_MAX_PACKET_SIZE - sizeof(*hdr)) / rs->wordSizeBytes;
        rs->maxSamples = (rs->maxSamples / rs->channels) * rs->channels;
        rs->maxFrames = rs->maxSamples / rs->channels;
        rs->size = sizeof(*hdr) + rs->maxFrames * rs->channels * rs->wordSizeBytes;
    }

    rs->samples = 0;
    rs->enabled = true;

    ok = true;

abort:
    return(ok);
}

unsigned rtpWriteSamplesAvailable(RTP_STREAM *rs, void **data)
{
    if (data) {
        *data = (void *)rs->data;
    }
    return(rs->maxSamples - rs->samples);
}

unsigned rtpWriteSamples(RTP_STREAM *rs, unsigned samples)
{
    RTP_PKT_HDR *hdr = (RTP_PKT_HDR *)rs->pkt;
    uint16_t *u16;
    uint32_t *u32;
    unsigned i;

    rs->samples += samples;
    rs->data += samples * rs->wordSizeBytes;

    if (rs->samples == rs->maxSamples) {
        hdr->sequence = htons(rs->sequence);
        hdr->timeStamp = htonl(rs->timeStamp);
        if (rs->wordSizeBytes == 2) {
            u16 = (uint16_t *)hdr->data;
            for (i = 0; i < rs->maxSamples; i++) {
                *u16 = htons(*u16); u16++;
            }
        } else {
            u32 = (uint32_t *)hdr->data;
            for (i = 0; i < rs->maxSamples; i++) {
                *u32 = htonl(*u32); u32++;
            }
        }
        sendto(rs->socket, hdr, rs->size, 0, (struct sockaddr *)&rs->ipAddr, sizeof(rs->ipAddr));
        rs->data = hdr->data;
        rs->samples = 0;
        rs->sequence += 1;
        rs->timeStamp += rs->maxFrames;
    }

    return(samples);
}

unsigned rtpReadSamplesAvailable(RTP_STREAM *rs, void **data)
{
    RTP_PKT_HDR *hdr = (RTP_PKT_HDR *)rs->pkt;
    ssize_t size = 0;
    uint16_t *u16;
    uint32_t *u32;
    unsigned i;

    if (rs->samples == 0) {
        socklen_t addrLen = sizeof(rs->ipAddr);
        size = recvfrom(
            rs->socket, rs->pkt, RTP_MAX_PACKET_SIZE, 0,
            (struct sockaddr *)&rs->ipAddr, &addrLen
        );
        if (size < (ssize_t)sizeof(*hdr)) {
            return(0);
        }
        rs->size = (unsigned)size;
        rs->data = hdr->data;
        rs->maxSamples = (rs->size - sizeof(*hdr)) / rs->wordSizeBytes;
        rs->maxFrames = rs->maxSamples / rs->channels;
        rs->samples = rs->maxFrames * rs->channels;
        if (rs->maxFrames == 0) {
            return(0);
        }
        if (rs->wordSizeBytes == 2) {
            u16 = (uint16_t *)hdr->data;
            for (i = 0; i < rs->maxSamples; i++) {
                *u16 = ntohs(*u16); u16++;
            }
        } else {
            u32 = (uint32_t *)hdr->data;
            for (i = 0; i < rs->maxSamples; i++) {
                *u32 = ntohl(*u32); u32++;
            }
        }

    }

    if (data) {
        *data = (void *)rs->data;
    }

    return(rs->samples);
}

unsigned rtpReadSamples(RTP_STREAM *rs, unsigned samples)
{
    rs->samples -= samples;
    rs->data += samples * rs->wordSizeBytes;
    return(rs->samples);
}

void closeRtpStream(RTP_STREAM *rs)
{
    if (rs->socket >= 0) {
        lwip_close(rs->socket);
    }
    if (rs->pkt) {
        RTP_FREE(rs->pkt);
        rs->pkt = NULL;
    }
    rs->socket = -1;
    rs->enabled = false;
}
