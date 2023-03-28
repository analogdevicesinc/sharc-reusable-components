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
#ifndef _pink_h
#define _pink_h

#include <stdbool.h>

#include "pa-pinknoise_cfg.h"

#ifndef PINK_MAX_RANDOM_ROWS
#define PINK_MAX_RANDOM_ROWS   (30)
#endif

typedef struct
{
    long      pink_Rows[PINK_MAX_RANDOM_ROWS];
    long      pink_RunningSum;   /* Used to optimize summing of generators. */
    int       pink_Index;        /* Incremented each sample. */
    int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
    float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
}
PinkNoise;

bool InitializePinkNoise( PinkNoise *pink, int numRows );
float GeneratePinkNoise( PinkNoise *pink );

#endif
