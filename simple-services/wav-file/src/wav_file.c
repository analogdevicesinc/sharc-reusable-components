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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "wav_file_cfg.h"
#include "wav_file.h"

#ifndef WAVE_FILE_BUF_SIZE
#define WAVE_FILE_BUF_SIZE (16 * 1024)
#endif

#ifndef WAVE_FILE_CALLOC
#define WAVE_FILE_CALLOC calloc
#endif

#ifndef WAVE_FILE_FREE
#define WAVE_FILE_FREE free
#endif

/***********************************************************************
 * WAVE helper functions, typedefs and defines
 **********************************************************************/
/*
 * https://www.appletonaudio.com/blog/tag/wave-format-extensible/
 *
 * Microsoft wave file history lesson:
 *   https://docs.microsoft.com/en-us/previous-versions/windows/hardware/design/dn653308(v=vs.85)
 *
 * WAVEFORMAT:
 *   https://msdn.microsoft.com/en-us/library/windows/desktop/dd757712%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
 *
 * WAVEFORMATEX:
 *   https://msdn.microsoft.com/en-us/library/windows/desktop/dd390970(v=vs.85).aspx
 *
 * WAVEFORMATEXTENSIBLE
 *   https://docs.microsoft.com/en-us/previous-versions/windows/hardware/design/dn653308(v=vs.85)
 *
 */
#define WAVE_FORMAT_PCM         (0x0001)
#define WAVE_FORMAT_EXTENSIBLE  (0xFFFE)
#define WAVEFORMATEXTENSIBLE_MINIMUM_SIZE (22)

typedef enum WAVE_ENDIAN {
    WAVE_ENDIAN_BE = 0,
    WAVE_ENDIAN_LE
} WAVE_ENDIAN;

#pragma pack(push,1)
/* RIFF Chunk Descriptor */
typedef struct RIFF_HEADER
{
    uint8_t     RIFF[4];       // RIFF Header Magic header
    uint32_t    size;          // RIFF Chunk Size
    uint8_t     WAVE[4];       // WAVE Header
} RIFF_HEADER;

/* Sub-chunk header */
typedef struct SUB_CHUNK_HDR
{
    uint8_t     type[4];        // RIFF Header Magic header
    uint32_t    size;           // RIFF Chunk Size
} SUB_CHUNK_HDR;

/* This is the standard WAVEFORMATX struct */
typedef struct {
    uint16_t  wFormatTag;
    uint16_t  nChannels;
    uint32_t  nSamplesPerSec;
    uint32_t  nAvgBytesPerSec;
    uint16_t  nBlockAlign;
} WAVEFORMATX;

/* This is the WAVEFORMATX struct for WAVE_FORMAT_PCM */
typedef struct {
    WAVEFORMATX fmtx;
    uint16_t  wBitsPerSample;
} WAVEFORMATPCM;

/* This is the WAVEFORMATEX struct for WAVE_FORMAT_EXTENSIBLE */
typedef struct {
  uint16_t  wFormatTag;
  uint16_t  nChannels;
  uint32_t  nSamplesPerSec;
  uint32_t  nAvgBytesPerSec;
  uint16_t  nBlockAlign;
  uint16_t  wBitsPerSample;
  uint16_t  cbSize;
} WAVEFORMATEX;

/*
 * This is the WAVEFORMATEX struct for WAVE_FORMAT_EXTENSIBLE and
 * cbSize >= WAVEFORMATEXTENSIBLE_MINIMUM_SIZE
 */
typedef struct {
    WAVEFORMATEX  Format;
    union {
        uint16_t wValidBitsPerSample; /* bits of precision */
        uint16_t wSamplesPerBlock;    /* valid if wBitsPerSample==0 */
        uint16_t wReserved;           /* If neither applies, set to zero. */
    } Samples;
    uint32_t     dwChannelMask;       /* which channels are present in stream */
    SUBFMT_GUID  SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
#pragma pack(pop)

static uint16_t fix_uint16(uint16_t val, WAVE_ENDIAN endian)
{
    if (endian == WAVE_ENDIAN_LE) {
        return(val);
    }
    return (val << 8) | (val >> 8 );
}

static uint32_t fix_uint32(uint32_t val, WAVE_ENDIAN endian)
{
    if (endian == WAVE_ENDIAN_LE) {
        return(val);
    }
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

static bool translateWaveFmt(WAVE_INFO *waveInfo, WAVEFORMATX *waveFormat, WAVE_ENDIAN endian)
{
    WAVEFORMATPCM *waveFormatPcm =  (WAVEFORMATPCM *)waveFormat;

    if (waveInfo->audioFormat == WAVE_FORMAT_EXTENSIBLE) {

        WAVEFORMATEX *waveFormatEx = (WAVEFORMATEX *)waveFormat;
        waveInfo->extensionSize = fix_uint16(waveFormatEx->cbSize, endian);
        if (waveInfo->extensionSize >= WAVEFORMATEXTENSIBLE_MINIMUM_SIZE) {
            WAVEFORMATEXTENSIBLE *waveFormatExtensible = (WAVEFORMATEXTENSIBLE *)waveFormat;
            waveInfo->validBitsPerSample = fix_uint16(waveFormatExtensible->Samples.wValidBitsPerSample, endian);
            waveInfo->channelMask = fix_uint32(waveFormatExtensible->dwChannelMask, endian);
            memcpy(&waveInfo->subAudioFormat, &waveFormatExtensible->SubFormat, sizeof(SUBFMT_GUID));
        }
    }

    waveInfo->audioFormat = fix_uint16(waveFormatPcm->fmtx.wFormatTag, endian);
    waveInfo->numChannels = fix_uint16(waveFormatPcm->fmtx.nChannels, endian);
    waveInfo->sampleRate = fix_uint32(waveFormatPcm->fmtx.nSamplesPerSec, endian);
    waveInfo->byteRate = fix_uint32(waveFormatPcm->fmtx.nAvgBytesPerSec, endian);
    waveInfo->blockAlign = fix_uint16(waveFormatPcm->fmtx.nBlockAlign, endian);
    waveInfo->bitsPerSample = fix_uint16(waveFormatPcm->wBitsPerSample, endian);
    waveInfo->Signed = (waveInfo->bitsPerSample > 8) ? true : false;

    if (waveInfo->validBitsPerSample > 0) {
        if (waveInfo->validBitsPerSample == 32) {
            waveInfo->waveFmt = WAVE_FMT_SIGNED_32BIT_LE;
        } else {
            waveInfo->waveFmt = WAVE_FMT_SIGNED_16BIT_LE;
        }
    } else if (waveInfo->bitsPerSample > 0) {
        if (waveInfo->bitsPerSample == 32) {
            waveInfo->waveFmt = WAVE_FMT_SIGNED_32BIT_LE;
        } else {
            waveInfo->waveFmt = WAVE_FMT_SIGNED_16BIT_LE;
        }
    } else {
        waveInfo->waveFmt = (waveInfo->blockAlign / waveInfo->numChannels) * 8;
        if (waveInfo->waveFmt == 32) {
            waveInfo->waveFmt = WAVE_FMT_SIGNED_32BIT_LE;
        } else {
            waveInfo->waveFmt = WAVE_FMT_SIGNED_16BIT_LE;
        }
    }

    return(true);
}

#define FOUND_NO_CHUNK   (0x00)
#define FOUND_FMT_CHUNK  (0x01)
#define FOUND_DATA_CHUNK (0x02)
#define FOUND_ALL_CHUNKS (FOUND_FMT_CHUNK | FOUND_DATA_CHUNK)

static bool processSubChunks(FILE *f, WAVE_INFO *waveInfo, WAVE_ENDIAN endian)
{
    SUB_CHUNK_HDR header;

    size_t nmemb;
    int found;

    found = FOUND_NO_CHUNK;

    do {
        nmemb = fread(&header, sizeof(header), 1, f);
        if (nmemb == 1)  {
            header.size = fix_uint32(header.size, endian);
            if (strncmp((const char *)header.type, "fmt ", 4) == 0) {
                uint8_t *fmtContainer = (uint8_t *)WAVE_FILE_CALLOC(1, header.size);
                nmemb = fread(fmtContainer, header.size, 1, f);
                if (nmemb == 1) {
                    WAVEFORMATX *waveFormat = (WAVEFORMATX *)fmtContainer;
                    uint16_t wFormatTag = fix_uint16(waveFormat->wFormatTag, endian);
                    if ((wFormatTag == WAVE_FORMAT_PCM) ||
                        (wFormatTag == WAVE_FORMAT_EXTENSIBLE)) {
                        bool ok = translateWaveFmt(waveInfo, waveFormat, endian);
                        if (ok) {
                            found |= FOUND_FMT_CHUNK;
                        }
                    }
                } else {
                    if (feof(f)) {
                        break;
                    }
                }
                WAVE_FILE_FREE(fmtContainer);
            } else if (strncmp((const char *)header.type, "data", 4) == 0) {
                waveInfo->dataSize = header.size;
                waveInfo->dataOffset = ftell(f);
                if (waveInfo->dataSize == 0) {
                    fseek(f, 0, SEEK_END);
                    waveInfo->dataSize = ftell(f) - waveInfo->dataOffset;
                    fseek(f, SEEK_SET, waveInfo->dataOffset);
                }
                fseek(f, header.size, SEEK_CUR);
                found |= FOUND_DATA_CHUNK;
            } else {
                fseek(f, header.size, SEEK_CUR);
            }
        } else {
            if (feof(f)) {
                break;
            }
        }
    }  while (found != FOUND_ALL_CHUNKS);

    return(found == FOUND_ALL_CHUNKS);
}

static bool isWave(WAV_FILE *waveFile)
{
    FILE *f = waveFile->f;
    WAVE_INFO *waveInfo = &waveFile->waveInfo;
    WAVE_ENDIAN endian;
    RIFF_HEADER header;
    size_t nmemb;
    bool isWave;
    bool ok;

    isWave = false;
    ok = false;

    fseek(f, 0, SEEK_SET);

    nmemb = fread(&header, sizeof(header), 1, f);
    if (nmemb == 1) {
        if (strncmp((const char *)header.RIFF, "RIFF", 4) == 0) {
            endian = WAVE_ENDIAN_LE;
            ok = true;
        } else if (strncmp((const char *)header.RIFF, "RIFX", 4) == 0) {
            endian = WAVE_ENDIAN_BE;
            ok = true;
        } else {
            ok = false;
        }
        if (ok && (strncmp((const char *)header.WAVE, "WAVE", 4) == 0)) {
            memcpy((uint8_t *)waveInfo->riffHead, (uint8_t *)header.RIFF, 4);
            isWave = processSubChunks(f, waveInfo, endian);
        }
    }

    return(isWave);
}

static bool writeWaveHeader(WAV_FILE *wf)
{
    FILE *f = wf->f;
    RIFF_HEADER riff;
    WAVEFORMATPCM fmt;
    SUB_CHUNK_HDR subChunkHdr;

    fseek(f, 0, SEEK_SET);

    memcpy(riff.RIFF, "RIFF", 4);
    memcpy(riff.WAVE, "WAVE", 4);
    riff.size = wf->dataSize + sizeof(fmt) + 2 * sizeof(subChunkHdr);
    fwrite(&riff, sizeof(riff), 1, f);

    memcpy(&subChunkHdr.type, "fmt ", 4);
    subChunkHdr.size = sizeof(fmt);
    fwrite(&subChunkHdr, sizeof(subChunkHdr), 1, f);
    fmt.fmtx.wFormatTag = WAVE_FORMAT_PCM;
    fmt.fmtx.nChannels = wf->channels;
    fmt.fmtx.nSamplesPerSec = wf->sampleRate;
    fmt.fmtx.nAvgBytesPerSec = wf->sampleRate * wf->frameSizeBytes;
    fmt.fmtx.nBlockAlign = wf->frameSizeBytes;
    fmt.wBitsPerSample = wf->wordSizeBytes * 8;
    fwrite(&fmt, sizeof(fmt), 1, f);

    memcpy(&subChunkHdr.type, "data", 4);
    subChunkHdr.size = wf->dataSize * wf->wordSizeBytes;
    fwrite(&subChunkHdr, sizeof(subChunkHdr), 1, f);

    return(true);
}

bool openWave(WAV_FILE *wf)
{
    bool ok = false;

    wf->f = fopen(wf->fname, wf->isSrc ? "rb" : "wb");
    if (wf->f) {
#ifdef WAVE_FILE_BUF_SIZE
        wf->fileBuf = (char *)WAVE_FILE_CALLOC(WAVE_FILE_BUF_SIZE, 1);
        setvbuf(wf->f, wf->fileBuf, _IOFBF, WAVE_FILE_BUF_SIZE);
#else
        wf->fileBuf = NULL;
#endif
        if (wf->isSrc) {
            ok = isWave(wf);
            if (ok) {
                fseek(wf->f, wf->waveInfo.dataOffset, SEEK_SET);
                wf->channels = wf->waveInfo.numChannels;
                wf->sampleRate = wf->waveInfo.sampleRate;
                wf->frameSizeBytes = wf->waveInfo.blockAlign;
                wf->wordSizeBytes = wf->waveInfo.bitsPerSample / 8;
                wf->dataSize = wf->waveInfo.dataSize / wf->wordSizeBytes;
                wf->dataOffset = 0;
                wf->enabled = true;
            }
        } else {
            writeWaveHeader(wf);
            wf->enabled = true;
            wf->dataOffset = 0;
            ok = true;
        }
    }

    return(ok);
}

void overrideWave(WAV_FILE *wf, unsigned channels)
{
    wf->channels = channels;
    wf->frameSizeBytes = wf->channels * wf->wordSizeBytes;
    wf->dataSize -= wf->dataSize % wf->channels;
}

void closeWave(WAV_FILE *wf)
{
    if (!wf->isSrc) {
        writeWaveHeader(wf);
    }
    if (wf->f) {
        fclose(wf->f); wf->f = NULL;
    }
    if (wf->fileBuf) {
        WAVE_FILE_FREE(wf->fileBuf); wf->fileBuf = NULL;
    }
    wf->enabled = false;
    wf->channels = 0;
}

size_t readWave(WAV_FILE *wf, void *buf, size_t samples)
{
    size_t size;
    size_t rsize;
    size_t remaining;
    bool ok;
    bool resetData;

    remaining = wf->dataSize - wf->dataOffset;
    size = samples > remaining ? remaining : samples;

    resetData = false; ok = true;

    rsize = fread(buf, wf->wordSizeBytes, size, wf->f);
    if (rsize != size) {
        if (feof(wf->f)) {
            resetData = true;
        } else if (ferror(wf->f)) {
            ok = false;
        } else if (rsize <= 0) {
            ok = false;
        }
    } else {
        wf->dataOffset += rsize;
        if (wf->dataOffset >= wf->dataSize) {
            resetData = true;
        }
    }

    if (resetData) {
        fseek(wf->f, wf->waveInfo.dataOffset, SEEK_SET);
        wf->dataOffset = 0;
    }

    return(ok ? rsize : -1);
}

size_t writeWave(WAV_FILE *wf, void *buf, size_t samples)
{
    size_t wsize;
    bool ok;

    ok = true;

    wsize = fwrite(buf, wf->wordSizeBytes, samples, wf->f);
    if (wsize != samples) {
        ok = false;
    } else {
        wf->dataSize += wsize;
    }

    return (ok ? wsize : -1);
}
