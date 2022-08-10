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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "buffer_track.h"

typedef struct _BUFFER_TRACKER_STATE {
    uint64_t sampleSecAccum;
    uint32_t lastSampleSecAccum;
    uint32_t startSampleSecAccum;
    uint32_t lastLevel;
    uint32_t lastSampleRate;
} BUFFER_TRACKER_STATE;

static BUFFER_TRACKER_STATE bufferTrackState[NUM_BUFFER_TRACKERS];
static BUFFER_GET_TIME getTime = NULL;

static void bufferTrackResetAccum(BUFFER_TRACKER_STATE *b, uint32_t now)
{
    b->lastSampleRate = 0;
    b->sampleSecAccum = 0;
    b->lastSampleSecAccum = now;
    b->startSampleSecAccum = now;
}

void bufferTrackInit(BUFFER_GET_TIME _getTime)
{
    getTime = _getTime;
}

void bufferTrackReset(int index)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];
    uint32_t now;

    now = getTime();

    bufferTrackResetAccum(b, now);
    b->lastLevel = 0;
}

void bufferTrackAccum(int index, unsigned samples)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];

    uint32_t now;
    uint32_t then;
    uint32_t elapsed;

    now = getTime();
    then = b->lastSampleSecAccum;

    /* Calculate elapsed time since last call */
    elapsed = now - then;

    /* Accumulate fill level in 'sample*seconds' */
    b->sampleSecAccum += (uint64_t)elapsed * (uint64_t)samples;

    /* Record last time */
    b->lastSampleSecAccum = now;
}

bool bufferTrackCheck(int index, uint32_t interval, uint32_t *level)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];
    uint32_t now;
    uint32_t elapsed;
    bool updated;

    /* If bufferTrackAccum() has never been called then there's 
     * nothing to check 
     */
    if (b->sampleSecAccum == 0) {
        return false;
    }

    now = getTime();

    updated = false;

    /* Calculate elapsed time since last computation */
    elapsed = now - b->startSampleSecAccum;

    /* Calculate the average buffer fill level over the last interval */
    if (elapsed >= interval) {
        b->lastLevel = (uint32_t)(b->sampleSecAccum / elapsed);
        if (level) {
            *level = b->lastLevel;
        }
        bufferTrackResetAccum(b, now);
        updated = true;
    }

    /* Return whether or not the level was updated */
    return(updated);
}

uint32_t bufferTrackCalculateSampleRate(int index,
    uint32_t desiredSamples, uint32_t frameSize, uint32_t baseSampleRate)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];
    int32_t error;

    error = desiredSamples - (b->lastLevel / frameSize);

    b->lastSampleRate = baseSampleRate + (error / 2);

    return(b->lastSampleRate);
}

uint32_t bufferTrackGetLevel(int index)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];
    return(b->lastLevel);
}

uint32_t bufferTrackGetFrames(int index, unsigned frameSize)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];
    return(b->lastLevel / frameSize);
}

uint32_t bufferTrackGetSampleRate(int index)
{
    BUFFER_TRACKER_STATE *b = &bufferTrackState[index];
    return(b->lastSampleRate);
}
