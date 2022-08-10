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

#ifndef _adau1761_h
#define _adau1761_h

#include <stdint.h>
#include <stdbool.h>

#include "twi_simple.h"

// ADAU driver return values
typedef enum
{
    ADAU_SUCCESS,                 // The API call is success
    ADAU_CORRUPT_INIT_FILE,       // SS-generated initialization file is corrupt
    ADAU_TWI_TIMEOUT_ERROR,       // A TWI timeout error occurred while trying to communicate with device
    ADAU_PLL_LOCK_TIMEOUT_ERROR,      // The PLL of the device we're driving failed to lock
    ADAU_SIMPLE_ERROR               // General failure
} BM_ADAU_RESULT;

BM_ADAU_RESULT init_adau1761(sTWI *twi, uint8_t adau_address);

#endif
