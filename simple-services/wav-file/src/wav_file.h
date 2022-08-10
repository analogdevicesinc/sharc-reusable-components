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

#ifndef _wav_file_h
#define _wav_file_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

typedef enum WAVE_FMT {
    WAVE_FMT_UNKNOWN = 0,
    WAVE_FMT_SIGNED_32BIT_LE,
    WAVE_FMT_SIGNED_16BIT_LE
} WAVE_FMT;

#pragma pack(push,1)
typedef struct SUBFMT_GUID {
    uint32_t  Data1;
    uint16_t  Data2;
    uint16_t  Data3;
    uint8_t   Data4[8];
} SUBFMT_GUID;
#pragma pack(pop)

/*
 * This is our sanitized WAVE file info
 */
typedef struct WAVE_INFO {
    uint8_t riffHead[4];
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint16_t extensionSize;
    uint16_t validBitsPerSample;
    uint32_t channelMask;
    SUBFMT_GUID subAudioFormat;
    bool Signed;
    uint32_t dataOffset;
    uint32_t dataSize;
    WAVE_FMT waveFmt;
} WAVE_INFO;

typedef struct WAV_FILE {
    char *fname;
    FILE *f;
    bool enabled;
    bool header;
    WAVE_INFO waveInfo;
    void *lock;
    size_t dataSize;
    unsigned channels;
    unsigned sampleRate;
    unsigned frameSizeBytes;
    unsigned wordSizeBytes;
    bool isSrc;
    void *fileBuf;
    size_t dataOffset;
} WAV_FILE;

bool openWave(WAV_FILE *wf);
void closeWave(WAV_FILE *wf);
size_t readWave(WAV_FILE *wf, void *buf, size_t samples);
size_t writeWave(WAV_FILE *wf, void *buf, size_t samples);
void overrideWave(WAV_FILE *wf, unsigned channels);

#endif
