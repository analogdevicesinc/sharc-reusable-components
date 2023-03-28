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

/* Insure CGU_TS_CLK = SYSCLK / (2^CGU_TS_DIV) below */
#define CGU_TS_DIV   (4)
#define CGU_TS_CLK   (SYSCLK / 16)

#endif
