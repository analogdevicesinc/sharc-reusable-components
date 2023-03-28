/**
 * Copyright (c) 2022 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#ifndef _cs2100_h
#define _cs2100_h

#include <stdint.h>

#include "twi_simple.h"

/*!****************************************************************
 * @brief CS2100 driver API result codes.
 ******************************************************************/
typedef enum CS2100_RESULT {
    CS2100_SUCCESS,          /**< No error */
    CS2100_ERROR,            /**< Generic error */
} CS2100_RESULT;

/*!****************************************************************
 * @brief CS2100 driver RefClkDiv values.
 ******************************************************************/
typedef enum CS2100_REF_CLK_DIV {
    CS2100_REF_CLK_DIV4 = 0x0,  /**< Divide by 4 (32 - 75MHz) */
    CS2100_REF_CLK_DIV2 = 0x1,  /**< Divide by 2 (16 - 37.5Mhz)*/
    CS2100_REF_CLK_DIV1 = 0x2,  /**< Divide by 1 (8 - 18.75MHz)*/
} CS2100_REF_CLK_DIV;

/*!****************************************************************
 * @brief CS2100 driver ClkIn_Bw values.
 ******************************************************************/
typedef enum CS2100_CLK_IN_BW {
    CS2100_CLK_IN_BW_1HZ   = 0x0,  /**< Clock In Bandwidth 1Hz */
    CS2100_CLK_IN_BW_2HZ   = 0x1,  /**< Clock In Bandwidth 2Hz */
    CS2100_CLK_IN_BW_4HZ   = 0x2,  /**< Clock In Bandwidth 4Hz */
    CS2100_CLK_IN_BW_8HZ   = 0x3,  /**< Clock In Bandwidth 8Hz */
    CS2100_CLK_IN_BW_16HZ  = 0x4,  /**< Clock In Bandwidth 16Hz */
    CS2100_CLK_IN_BW_32HZ  = 0x5,  /**< Clock In Bandwidth 32Hz */
    CS2100_CLK_IN_BW_64HZ  = 0x6,  /**< Clock In Bandwidth 64Hz */
    CS2100_CLK_IN_BW_128HZ = 0x7,  /**< Clock In Bandwidth 128Hz */
} CS2100_CLK_IN_BW;

/*!****************************************************************
 * @brief CS2100 driver LFRatioCfg values.
 ******************************************************************/
typedef enum CS2100_RATIO_CFG {
    CS2100_RATIO_CFG_HIGH_MULT     = 0x0,  /**< Might Multiplier (20.12) */
    CS2100_RATIO_CFG_HIGH_ACCURACY = 0x1,  /**< High Accuracy (12.20) */
} CS2100_RATIO_CFG;

/*!****************************************************************
 * @brief CS2100 driver initial configuration.
 ******************************************************************/
typedef struct CS2100_CFG {
    CS2100_REF_CLK_DIV refClkDiv;
    CS2100_CLK_IN_BW clkInBw;
    CS2100_RATIO_CFG ratioCfg;
    double ratio;
} CS2100_CFG;

#ifdef __cplusplus
extern "C" {
#endif

CS2100_RESULT cs2100_set_ratio(sTWI *twiHandle, uint8_t address,
    double ratio);

CS2100_RESULT cs2100_init(sTWI *twiHandle, uint8_t address, CS2100_CFG *cfg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
