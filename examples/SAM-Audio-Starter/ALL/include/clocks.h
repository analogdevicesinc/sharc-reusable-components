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

#ifndef _clocks_h
#define _clocks_h

/* TODO: Make more complete */
#define OSC_CLK      (25000000)
#define CCLK         (500000000)
#define SYSCLK       (CCLK / 2)
#define SCLK0        (CCLK / 4)

/* Compute OCLK divisor */
#define OCLK         (50000000)
#define OCLK_DIV     (CCLK / OCLK)

/* Insure CGU_TS_CLK = SYSCLK / (2^CGU_TS_DIV) below */
#define CGU_TS_DIV   (4)
#define CGU_TS_CLK   (SYSCLK / 16)

/*
 * See CGU1 configuration details in init.c and keep this file
 * in sync with actual config.
 */
#define CCLK_1       (250000000)
#define SYSCLK_1     (CCLK_1 / 2)
#define SCLK1_1      (SYSCLK_1 / 1)
#define DCLK_1       (CCLK_1 / 5)

/* The MSI0 SDCARD clock is derived from CGU1/DCLK */
#define SDCLK        (DCLK_1)

#endif
