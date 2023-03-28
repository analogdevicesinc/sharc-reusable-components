# simple-services/buffer-track

## Overview

The `buffer-track` directory contains a component capable of accurately tracking buffer fill levels, without additional filtering, even when the size of the in/out transfers are large compared to the overall size of the buffer itself.

It does this by integrating the instantaneous level of the buffer over a series of buffer fill/empty cycles then computing the average fill at regular intervals during either the in or out cycle.

In most cases, a sample accurate average buffer fill level can be computed in one second or less with low-latency buffers.

This algorithm works even when the input and output transfers are based off of asynchronous clock sources (i.e. an ASRC buffer) and is not sensitive to the order of the in/out transfers.

## Required components

- A fast 32-bit timer

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the buffer-track service.

## Configure

The buffer-track service has a couple of compile-time configuration options.  See 'inc/buffer_track_cfg.h' for an example.

## Run

- Call `bufferTrackInit()` once at system startup.
- Call `bufferTrackAccum()` every time data is put into, or taken out of, the buffer.
- Call `bufferTrackCheck()` at regular intervals when data is put into, or taken out of, the buffer (must pick only one place).

## Example

### Initialize

```C
#define CGU_TS_CLK   (SYSCLK / 16)

/***********************************************************************
 * High precision timestamp
 **********************************************************************/
uint32_t getTimeStamp(void)
{
    uint32_t timeStamp;
    timeStamp = *pREG_CGU0_TSCOUNT0;
    return timeStamp;
}

/* Initialize the buffer level tracking module. */
bufferTrackInit(getTimeStamp);
```

### Before adding data

```C
/* Accumulate the fill level of the USB OUT ring buffer for
 * rate feedback calculation.
 */
samples = PaUtil_GetRingBufferReadAvailable(context->uac2RingBufferRxCodec);
bufferTrackAccum(0, samples);
```

### Before removing data

```C
bool sampleRateUpdate;
uint32_t ringBufferLevel;

samples = PaUtil_GetRingBufferReadAvailable(context->uac2RingBufferRxCodec);

/* Accumulate the fill level of the USB OUT (Rx) ring buffer
 * for sample rate feedback
 */
bufferTrackAccum(0, samples);

/* See if it is time to compute a new buffer fill level (once
 * per second @ CGU_TS_CLK).  If so, also compute a new sample rate
 * feedback value.
 */
sampleRateUpdate = bufferTrackCheck(0, CGU_TS_CLK, &ringBufferLevel);
if (sampleRateUpdate) {
    bufferTrackCalculateSampleRate(0, DAC_PREROLL_FRAMES,
        CODEC_AUDIO_CHANNELS, SAMPLE_RATE);
}
```

### Retrieve computed sample rate feedback for USB audio

```C
rate = bufferTrackGetSampleRate(0);
```

## Info

- The timer must run much faster than the interval in which data is added or removed from the buffer.  The faster the timer, the better the resolution of the result.

- The timer can roll over during any given interval at most one time.

- The call to `bufferTrackCheck()` must be called either when adding or removing data, but not both.

- `bufferTrackAccum()` must be called prior to both adding and removing data.
