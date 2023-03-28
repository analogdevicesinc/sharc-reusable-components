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

#ifndef __a2b_pal_h
#define __a2b_pal_h

#define A2B_PAL_VER_MAJOR   (0)
#define A2B_PAL_VER_MINOR   (0)
#define A2B_PAL_VER_RELEASE (0)

#include "a2b/pal.h"

void a2b_palInit(struct a2b_StackPal *pal, A2B_ECB *ecb);

#endif
