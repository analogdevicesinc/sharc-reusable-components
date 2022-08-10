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

#ifndef _codec_audio_h
#define _codec_audio_h

#include <stdint.h>

void codecAudioOut(void *buffer, uint32_t size, void *usrPtr);
void codecAudioIn(void *buffer, uint32_t size, void *usrPtr);

#endif
