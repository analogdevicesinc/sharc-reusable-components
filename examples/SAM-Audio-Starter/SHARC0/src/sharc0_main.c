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

/* Standard includes. */
#include <assert.h>

#define DO_CYCLE_COUNTS

/* CCES includes */
#include <services/int/adi_sec.h>
#include <services/gpio/adi_gpio.h>
#include <cycle_count.h>

/* Simple service includes */
#include "sae.h"

/* IPC includes */
#include "ipc.h"

SAE_CONTEXT *saeContext;
cycle_t startCycles;
cycle_t finalCycles;

enum {
    STREAM_MASK_CODEC_IN  = 0x00000001,
    STREAM_MASK_CODEC_OUT = 0x00000002,
    STREAM_MASK_A2B_IN    = 0x00000004,
    STREAM_MASK_A2B_OUT   = 0x00000008,
    STREAM_MASK_USB_RX    = 0x00000010,
    STREAM_MASK_USB_TX    = 0x00000020,
    STREAM_MASK_ALL       = 0x0000003F
};

typedef struct _STREAM_INFO {
    uint8_t numChannels;
    uint8_t numFrames;
    uint8_t wordSize;
    int32_t *data;
} STREAM_INFO;

IPC_MSG_ROUTING *routeInfo = NULL;

STREAM_INFO streamInfo[IPC_STREAM_ID_MAX];

#pragma optimize_for_speed
static void routeAudio(void)
{
    ROUTE_INFO *route;
    STREAM_INFO *src, *sink;
    uint8_t channels;
    int32_t *in, *out;
    uint8_t inChannel, outChannel;
    unsigned frame;
    unsigned channel;
    int32_t zero = 0;
    int32_t sample;
    unsigned i;

    /* Toggle LED 11 for measurement */
    adi_gpio_Toggle(ADI_GPIO_PORT_D, ADI_GPIO_PIN_2);

    if (routeInfo == NULL) {
        return;
    }

    for (i = 0; i < routeInfo->numRoutes; i++) {

        route = &routeInfo->routes[i];

        if (route->srcID == IPC_STREAMID_UNKNOWN) {
            continue;
        }
        if (route->sinkID == IPC_STREAMID_UNKNOWN) {
            continue;
        }

        src = &streamInfo[route->srcID];
        sink = &streamInfo[route->sinkID];

#if 1
        if (src->numFrames != sink->numFrames) {
            continue;
        }
        if (src->wordSize != sink->wordSize) {
            continue;
        }
        if (src->wordSize != sizeof(int32_t)) {
            continue;
        }
        if (route->srcOffset >= src->numChannels) {
            continue;
        }
        if (route->sinkOffset >= sink->numChannels) {
            continue;
        }
#endif

        inChannel = route->srcOffset;
        outChannel = route->sinkOffset;

        channels = route->channels;
        in = src->data + inChannel;
        out = sink->data + outChannel;

        for (frame = 0; frame < src->numFrames; frame++) {
            for (channel = 0; channel < channels; channel++) {
                if ((outChannel + channel) < sink->numChannels) {
                    if ((inChannel + channel) < src->numChannels) {
                        sample = *(in + channel);
                    } else {
                        sample = 0;
                    }
                    *(out + channel) = sample;
                }
            }
            in += src->numChannels;
            out += sink->numChannels;
        }

    }

}

/*
 * All audio SPORT interrupts (CODEC, A2B) have been hardware aligned
 * at startup by gating their respective bit clocks until all
 * SPORTs have been configured then turning on all clocks at once.  The
 * SPORTs count down exactly 1 frame of bit clocks before starting. This
 * is initiated on the ARM side in init.c -> enable_sport_clocks()
 *
 * The USB RX/TX piggy-back off of the CODEC ISR to also function like as a
 * time aligned SPORT.
 *
 * processAudio() waits until all streams have appeared and processes
 * audio on the last one.  Since the last ISR to fire is traditionally
 * a SPORT receive, this method assures that all audio buffers are
 * quiescent and ready for processing.
 *
 */
static void processAudio(IPC_MSG_AUDIO *audio, uint32_t streamMask,
    bool clear)
{
    STREAM_INFO *stream;

    static uint32_t processMask = 0x00000000;

    stream = &streamInfo[audio->streamID];
    stream->numChannels = audio->numChannels;
    stream->numFrames = audio->numFrames;
    stream->wordSize = audio->wordSize;
    stream->data = audio->data;

    if (clear) {
        memset(stream->data, 0,
            audio->numChannels * audio->numFrames * audio->wordSize);
    }

    processMask |= streamMask;
    if (processMask == STREAM_MASK_ALL) {
        processMask = 0x00000000;
        START_CYCLE_COUNT(startCycles);
        routeAudio();
        STOP_CYCLE_COUNT(finalCycles, startCycles);
    }
}

static void newAudio(IPC_MSG_AUDIO *audio)
{
    uint32_t streamMask = 0x00000000;
    bool clear = false;

    switch (audio->streamID) {
        case IPC_STREAMID_CODEC_IN:
            streamMask = STREAM_MASK_CODEC_IN;
            break;
        case IPC_STREAMID_CODEC_OUT:
            streamMask = STREAM_MASK_CODEC_OUT;
            clear = true;
            break;
        case IPC_STREAMID_A2B_IN:
            streamMask = STREAM_MASK_A2B_IN;
            break;
        case IPC_STREAMID_A2B_OUT:
            streamMask = STREAM_MASK_A2B_OUT;
            clear = true;
            break;
        case IPC_STREAMID_USB_RX:
            streamMask = STREAM_MASK_USB_RX;
            break;
        case IPC_STREAMID_USB_TX:
            streamMask = STREAM_MASK_USB_TX;
            clear = true;
            break;
        default:
            break;
    }

    if (streamMask) {
        processAudio(audio, streamMask, clear);
    }
}

static void ipcMsgRx(SAE_CONTEXT *saeContext, SAE_MSG_BUFFER *buffer,
    void *payload, void *usrPtr)
{
    SAE_MSG_BUFFER *ipcBuffer;
    SAE_RESULT result;
    IPC_MSG *msg = (IPC_MSG *)payload;
    IPC_MSG_AUDIO *audio;
    IPC_MSG *replyMsg;

    /* Process the message */
    switch (msg->type) {
        case IPC_TYPE_PING:
            ipcBuffer = sae_createMsgBuffer(saeContext, sizeof(*replyMsg), (void **)&replyMsg);
            replyMsg->type = IPC_TYPE_PING;
            result = sae_sendMsgBuffer(saeContext, ipcBuffer, IPC_CORE_ARM, true);
            if (result != SAE_RESULT_OK) {
                sae_unRefMsgBuffer(saeContext, ipcBuffer);
            }
            break;
        case IPC_TYPE_AUDIO:
            audio = (IPC_MSG_AUDIO *)&msg->audio;
            newAudio(audio);
            break;
        case IPC_TYPE_AUDIO_ROUTING:
            routeInfo = (IPC_MSG_ROUTING *)&msg->routes;
            break;
        case IPC_TYPE_CYCLES:
            ipcBuffer = sae_createMsgBuffer(saeContext, sizeof(*replyMsg), (void **)&replyMsg);
            replyMsg->type = IPC_TYPE_CYCLES;
            replyMsg->cycles.core = IPC_CORE_SHARC0;
            replyMsg->cycles.cycles = (uint32_t)finalCycles;
            result = sae_sendMsgBuffer(saeContext, ipcBuffer, IPC_CORE_ARM, true);
            if (result != SAE_RESULT_OK) {
                sae_unRefMsgBuffer(saeContext, ipcBuffer);
            }
            break;
        default:
            break;
    }

    /* Done with the message so decrement the ref count */
    result = sae_unRefMsgBuffer(saeContext, buffer);
}

int main(int argc, char **argv)
{
    static uint8_t gpioMemory[ADI_GPIO_CALLBACK_MEM_SIZE];
    uint32_t numCallbacks;

    /* Initialize the SEC */
    adi_sec_Init();

    /* Initialize GPIO */
    adi_gpio_Init(gpioMemory, sizeof(gpioMemory), &numCallbacks);

    /* Initialize the SHARC Audio Engine */
    sae_initialize(&saeContext, SAE_CORE_IDX_1, false);

    /* Register an IPC message Rx callback */
    sae_registerMsgReceivedCallback(saeContext, ipcMsgRx, NULL);

    while (1) {
        asm("nop;");
    }
}
