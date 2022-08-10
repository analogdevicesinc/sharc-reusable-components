# simple-services/tone-generator

## Overview

The `tone-generator` service implements a simple, multi-instance
tone generator that can be used to generate sine tones at various amplitudes
and frequencies. Amplitude and frequency values can be updated on-the-fly.
Amplitude is transitioned smoothly so there is no zipper noise or discontinuities
when adjust gain.

A single instance of the tone generator requires around 0.8 MIPS at 48kHz
sampling rate.

The code is written to be thread-safe. If `tone_gen_update_gain()` or
`tone_gen_update_freq()` are called while `tone_gen_read()` is running (the
  audio render function), the audio will still be rendered properly with no
  artifacts despite new parameters being set.

## Required components

None

## Recommended components

None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header
  files in the 'inc' directory contain the configurable options for the
  tone generator service.

## Configure

The tone-generator service has a few compile-time constants that are used to
bound the min and max frequency and gain / amplitude values. It also contains
a constant to set the amplitude change rate. See 'inc/tone_gen_cfg.h' for an
example.

## Run

- See the example below

## Example

```C
#include "tone_generator.h"

// Create instances of the tone generator
TONE_GEN tone_gen_left, tone_gen_right;

/*
 * Set up the current system
 */
uint32_t system_setup( void )
{
  // Set up first tone generator at 1kHz at 0.5 amplitude
  tone_gen_init( &tone_gen_left, 1000.0, 0.5, 48000.0 );

  // Set up first tone generator at 500Hz at 0.5 amplitude
  tone_gen_init( &tone_gen_right, 500.0, 0.5, 48000.0 );


}

/*
 * Example audio callback
 */
uint32_t audio_callback(float * in_left,
                        float * in_right,
                        float * out_left,
                        float * out_right,
                        uint32_t block_size )
{
  // Generate audio tones for both left and right output channels
  tone_gen_read( &tone_gen_left, out_left, block_size );
  tone_gen_read( &tone_gen_right, out_right, block_size );

}

/*
 * Updates the gain of the current tone generators
 */
uint32_t update_gain( float new_gain )
{
    tone_gen_update_gain( &tone_gen_left, new_gain );
    tone_gen_update_gain( &tone_gen_right, new_gain );
}

/*
 * Updates the frequencies of the current tone generators
 */
uint32_t update_freq( float new_freq )
{
    tone_gen_update_freq( &tone_gen_left, new_freq );
    tone_gen_update_freq( &tone_gen_right, new_freq * 0.5 );
}


```

## Info

- This module currently relies on the `sinf()` function from `math.h`. One
  potential optimization is using a look-up table to generate the tones.
  This would require more data memory but could save a few MIPs. The current
  implementation runs at around 0.8 MIPS per instance.
- This module could be easily extended to support different tones shapes and
  various noises (white, pink, etc.)
