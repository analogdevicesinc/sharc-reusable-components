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

#ifndef _sae_lock_h
#define _sae_lock_h

#include <stdint.h>
#include <stdbool.h>

#define SAE_SHARC_ARM_IPC_UNLOCKED   (0)
#define SAE_SHARC_ARM_IPC_LOCKED     (1)

bool sae_lock(volatile uint32_t *lock);
bool sae_unlock(volatile uint32_t *lock);

#endif
