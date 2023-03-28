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

/*!
 * @brief  Implementation of tone generator function
 *
 * @file      tone_generator.c
 * @version   1.0.0
 * @sa        tone_generator.c
 *
*/

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "tone_generator.h"

#define PI2	6.2831853072


#pragma optimize_for_space
void 	tone_gen_init( TONE_GEN * c, float freq_hz, float gain_linear, float audio_sample_rate ) {
	assert(c);
	assert(audio_sample_rate);

	// Bound frequency in valid range
	c->freq_hz = fmin( fmax(TONE_GEN_MIN_FREQ_HZ, freq_hz), TONE_GEN_MAX_FREQ_HZ);

	// Set increment value for tone frequency and sample rate
	c->t_inc = PI2 * (c->freq_hz / audio_sample_rate);
	c->t = 0.0;

	// Set sample rate for future freq update calculations
	c->sample_rate_hz = audio_sample_rate;

	// Set amplitude
	c->target_gain = fmin( fmax(TONE_GEN_MIN_GAIN_LIN, gain_linear), TONE_GEN_MAX_GAIN_LIN);
	c->current_gain = 0.0;

}


#pragma optimize_for_speed
void	tone_gen_update_freq( TONE_GEN * c, float new_freq_hz ) {

	// Update frequency and bound to valid range
	c->freq_hz = fmin( fmax(TONE_GEN_MIN_FREQ_HZ, new_freq_hz), TONE_GEN_MAX_FREQ_HZ);

	// Update increment value
	c->t_inc = PI2 * (c->freq_hz / c->sample_rate_hz);


}



#pragma optimize_for_speed
void	tone_gen_update_gain( TONE_GEN * c, float new_gain ) {

	// Bound and update amplitude target
	c->target_gain = fmin( fmax(TONE_GEN_MIN_GAIN_LIN, new_gain), TONE_GEN_MAX_GAIN_LIN);

}



#pragma optimize_for_speed
void 	tone_gen_read( TONE_GEN * c, float * out_buffer, uint32_t audio_block_size ) {

	// Load state variables from struct for faster performance
	float gain = c->current_gain;
	float target_gain = c->target_gain;
	float t = c->t;
	float inc = c->t_inc;

	for (int i=0;i<audio_block_size;i++) {

		// Generate and scale tone
		out_buffer[i] = gain * sinf(t);

		// Keep t between 0 and PI2 as lose precision as t increases
		t += inc;
		if (t >= PI2) {
			t -= PI2;
		}

		// Check to see if we need to adjust current gain
		if (gain != target_gain) {
			if (fabs(target_gain - gain) <= TONE_GEN_GAIN_INC) {
				gain = target_gain;
			}
			if (target_gain > gain) {
				gain += TONE_GEN_GAIN_INC;
			} else if (target_gain < gain) {
				gain -= TONE_GEN_GAIN_INC;
			}
		}

	}

	// Save state variables back to struct
	c->current_gain = gain;
	c->t = t;

}
