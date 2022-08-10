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

#ifndef _init_h
#define _init_h

#include "context.h"
#include "twi_simple.h"

void system_clk_init(void);
void cgu_ts_init(void);
void gpio_init(void);
void gic_init(void);
void heap_init(void);
void flash_init(APP_CONTEXT *context);

void pcg_init(APP_CONTEXT *context);
void adau1761_init(APP_CONTEXT *context);
bool ad2425_init(APP_CONTEXT *context, uint8_t I2SGCFG, uint8_t I2SCFG);
void ad2425_reset(void);

void disable_sport_clocks(APP_CONTEXT *context);
void enable_sport_clocks(APP_CONTEXT *context);
void audio_mclk_24576_mhz(sTWI *twi);

bool ad2425_to_sport_cfg(bool master, bool rxtx,
    uint8_t I2SGCFG, uint8_t I2SCFG, SPORT_SIMPLE_CONFIG *sportCfg);

void sae_buffer_init(APP_CONTEXT *context);
void audio_routing_init(APP_CONTEXT *context);

void system_reset(APP_CONTEXT *context);

#endif
