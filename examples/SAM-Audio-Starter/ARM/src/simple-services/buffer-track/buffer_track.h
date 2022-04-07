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

/*!
 * @brief  Circular buffer fill level tracker
 *
 * This module precisely tracks and computes the fill level of
 * circular buffers even in the presence of large input and output
 * block transfers.
 *
 * @file      buffer_track.h
 * @version   1.0.0
 * @copyright 2019 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef _buffer_track_h
#define _buffer_track_h

#include <stdint.h>
#include <stdbool.h>

#include "buffer_track_cfg.h"

#ifndef NUM_BUFFER_TRACKERS
#define NUM_BUFFER_TRACKERS  (1)
#endif

/*!****************************************************************
 * @brief  Function called to track time
 *
 * The application must pass in a function which returns a
 * monotonically increasing high-resolution time.
 *
 * The base tick of the timer should be significantly faster
 * than the call rate to bufferTrackAccum() for accurate level
 * calculations.
 *
 * Also, the timer must not overflow a uint32_t value between
 * calls to bufferTrackAccum() or during the interval passed
 * to bufferTrackCheck().  It is OK if the timer rolls over zero.
 *
 * This function will likely be called from an ISR.
 ******************************************************************/
typedef uint32_t (*BUFFER_GET_TIME)(void);

/*!****************************************************************
 * @brief  Initializes the buffer tracking module
 *
 * This function initializes the buffer tracking module
 *
 * This function is not thread safe.
 *
 * @param [in]  getTime  Pointer to a function to call to get the
 *                       current time.
 *
 ******************************************************************/
void bufferTrackInit(BUFFER_GET_TIME getTime);

/*!****************************************************************
 * @brief  Resets / Initializes a circular buffer monitor
 *
 * This function resets the buffer level monitor state associated
 * with 'index'.
 *
 * This function is not thread safe.
 *
 * @param [in]   index     Index of buffer being monitored
 *
 ******************************************************************/
void bufferTrackReset(int index);

/*!****************************************************************
 * @brief  Accumulates the current fill level of the circular
 *         buffer
 *
 * This function accumulates the current fill level of the circular
 * buffer in samples.  This function should be called immediately
 * before any samples are placed into, or removed from, the
 * buffer.
 *
 * This function is not thread safe and is intended to be called
 * from the sample in and sample out ISRs without nesting.
 *
 * @param [in]   index     Index of buffer being monitored
 * @param [in]   samples   Current buffer fill level in samples
 *
 ******************************************************************/
void bufferTrackAccum(int index, unsigned samples);

/*!****************************************************************
 * @brief  Determines whether or not the circular buffer fill level
 *         should be updated.
 *
 * This function computes a new buffer fill level if 'interval'
 * time has elapsed since the last computation.  The computed
 * buffer fill level is returned in 'level' if non-NULL.
 *
 * This function is not thread safe and is intended to be called
 * from an ISR without nesting.  This function must exclusively
 * be called by either the ISR putting data into the buffer or
 * the ISR taking data out of the buffer.
 *
 * @param [in]   index     Index of buffer being monitored
 * @param [in]   interval  Update interval
 * @param [out]  level     Computed buffer level in samples
 *                         (can be NULL)
 *
 * @return Returne 'true' if a new fill level was computed otherwise
 *         false.
 ******************************************************************/
bool bufferTrackCheck(int index, uint32_t interval, uint32_t *level);

/*!****************************************************************
 * @brief  Get the last computed buffer fill level in samples.
 *
 * This function returns the last computed buffer fill level in
 * samples.
 *
 * This function is thread safe.
 *
 * @param [in]   index     Index of buffer being monitored
 *
 * @return Buffer fill level in samples
 ******************************************************************/
uint32_t bufferTrackGetLevel(int index);

/*!****************************************************************
 * @brief  Get the last computed buffer fill level in frames.
 *
 * This function returns the last computed buffer fill level in
 * frames.  'frameSize' should be the number of samples per frame
 * of incoming data.
 *
 * This function is thread safe.
 *
 * @param [in]   index     Index of buffer being monitored
 * @param [in]   frameSize Samples per incoming frame
 *
 * @return Buffer fill level in frames (samples / frameSize)
 ******************************************************************/
uint32_t bufferTrackGetFrames(int index, unsigned frameSize);

/*!****************************************************************
 * @brief  Get the last computed sample rate feedback.
 *
 * This function returns the incoming sample rate necessary to
 * return the buffer to 'desiredSamples' over the course of the next
 * interval.
 *
 * The output of this function can be directly used for USB Audio
 * sample rate feedback.
 *
 * This function is thread safe.
 *
 * @param [in]   index     Index of buffer being monitored
 *
 * @return Sample rate in hz
 ******************************************************************/
uint32_t bufferTrackGetSampleRate(int index);

/*!****************************************************************
 * @brief  Calculates the incoming sample required to fill
 *
 * This function calculates the incoming sample rate necessary to
 * return the buffer half way between the actual level in samples
 * and 'desiredSamples' over the course of the next interval.
 * 'frameSize' should be the number of samples per frame of incoming
 * data.
 *
 * This function is not thread safe.
 *
 * @param [in]   index           Index of buffer being monitored
 * @param [in]   desiredSamples  Desired buffer fill level in samples
 * @param [in]   frameSize       Number of channels in a frame of audio
 *                               samples
 * @param [in]   baseSampleRate  Sample rate of samples
 *
 * @return Sample rate in hz
 ******************************************************************/
uint32_t bufferTrackCalculateSampleRate(int index,
    uint32_t desiredSamples, uint32_t frameSize, uint32_t baseSampleRate);

#endif
