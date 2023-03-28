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
 * @brief     Simple, efficient, PCG configuration routine
 *
 *   This PCG driver supports:
 *    - Multiple input clock configurations
 *    - External frame synchronization
 *
 * @file      pcg_simple.h
 * @version   1.0.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef _pcg_simple_h
#define _pcg_simple_h

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief PCG resource to configure
 */
typedef enum {
    PCG_A,
    PCG_B,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    PCG_C,
    PCG_D,
#endif
} PCG_RESOURCE;

/**
 * @brief PCG clock source
 */
typedef enum {
    PCG_SRC_DAI_PIN,    /**< Use a DAI pin as clock source */
    PCG_SRC_CLKIN,      /**< Use processor CLKIN signal as clock source */
    PCG_SRC_SCLK_PLL    /**< Use SCLK as clock source */
} PCG_CLK_SRC;


/**
 * @brief Configuration struct used to initialize the PCG
 */
typedef struct {
    PCG_RESOURCE pcg;               /**< PCG to configure */

    PCG_CLK_SRC  clk_src;           /**< PCG clock source */
    uint32_t     clk_in_dai_pin;    /**< DAI pin number for clock (needed only if clock source is DAI) */

    bool         sync_to_fs;        /**< Whether or not to sync to an external framesync */
    uint32_t     fs_in_dai_pin;     /**< DAI pin for FS (needed only if syncing to external framesync*/

    uint32_t     bitclk_div;                /**< Divisor for BCLK reduction */
    uint32_t     lrclk_clocks_per_frame;    /**< Number of bits per LR clock frame (typically 64, 256 or 512) */

} PCG_SIMPLE_CONFIG;

/**
 * @brief Simple SPDIF driver API result codes.
 */
typedef enum {
    PCG_SIMPLE_SUCCESS = 0,      /**< No error */
} PCG_SIMPLE_RESULT;


#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief Configures and enables a PCG
 *
 * @param config Pointer to configuration object
 * @return PCG_SIMPLE_RESULT result
 */
PCG_SIMPLE_RESULT pcg_open( PCG_SIMPLE_CONFIG * config);

PCG_SIMPLE_RESULT pcg_enable( PCG_RESOURCE pcg, bool enable);

#ifdef __cplusplus
}
#endif

#endif
