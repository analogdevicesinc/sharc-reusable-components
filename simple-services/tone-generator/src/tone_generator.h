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
 * @brief  Tone generator service
 *
 * A multi-instance, thread-safe tone generation module
 *
 * @file      tone_generator.h
 * @version   1.0.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef _TONE_GEN_H
#define _TONE_GEN_H

#include "../inc/tone_generator_cfg.h"

/*!****************************************************************
 * @brief  The minimum frequency for the tone generator
 ******************************************************************/
#ifndef TONE_GEN_MIN_FREQ_HZ
#define TONE_GEN_MIN_FREQ_HZ	10.0
#endif

/*!****************************************************************
 * @brief  The maximum frequency for the tone generator
 ******************************************************************/
#ifndef TONE_GEN_MAX_FREQ_HZ
#define TONE_GEN_MAX_FREQ_HZ	20000.0
#endif


/*!****************************************************************
 * @brief  The minimum amplitude / gain for the tone generator
 ******************************************************************/
#ifndef TONE_GEN_MIN_GAIN_LIN
#define TONE_GEN_MIN_GAIN_LIN	0.0
#endif


/*!****************************************************************
 * @brief  The maximum amplitude / gain for the tone generator
 ******************************************************************/
#ifndef TONE_GEN_MAX_GAIN_LIN
#define TONE_GEN_MAX_GAIN_LIN	2.0
#endif

/*!****************************************************************
 * @brief  Determines gain scale rate. This value is added to or
 *         subtracted from the current gain value until the
 *         target gain value has been reached. At 48kHz, a value of
 *         0.0001 is around 20ms.
 ******************************************************************/
#define TONE_GEN_GAIN_INC		0.001


/**
 * @brief  This struct stores state information for each instance
 *         of the tone generator
 */
typedef struct {
	float t, t_inc;
	float target_gain, current_gain;
	float freq_hz;
	float sample_rate_hz;
} TONE_GEN;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes an instance of the tone generator
 * @details [long description]
 *
 * @param c Pointer to C struct for this instance
 * @param freq_hz Tone frequency in Hertz
 * @param gain_linear Gain / amplitude (linear)
 * @param audio_sample_rate Audio sample rate in Hertz
 */
void 	tone_gen_init( TONE_GEN * c, float freq_hz, float gain_linear, float audio_sample_rate );

/**
 * @brief Updates the frequency of an existing tone generator instance
 *
 * @param c Pointer to C struct for this instance
 * @param new_freq_hz Updated tone frequency in Hertz
 */
void	tone_gen_update_freq( TONE_GEN * c, float new_freq_hz );

/**
 * @brief Updates the gain of an existing tone generator instance
 *
 * @param c Pointer to C struct for this instance
 * @param new_gain Updated gain / amplitude (linear)
 */
void	tone_gen_update_gain( TONE_GEN * c, float new_gain );

/**
 * @brief Reads a block of audio for this instance of the tone generator
 *
 * @param c Pointer to C struct for this instance
 * @param out_buffer Pointer to floating point output buffer
 * @param audio_block_size Number of samples to produce (i.e. size of output buffer in samples)
 */
void 	tone_gen_read( TONE_GEN * c, float * out_buffer, uint32_t audio_block_size );

#ifdef __cplusplus
}
#endif

#endif
