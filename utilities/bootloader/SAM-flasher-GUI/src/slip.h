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

#ifndef _slip_h
#define _slip_h

#ifdef __cplusplus
extern "C" {
#endif

int slip(unsigned char *pIn, int lenIn, unsigned char *pOut, int lenOut);
int unslip(unsigned char *pIn, int lenIn, int *offsetIn, unsigned char *pOut, int lenOut, int *offsetOut);

#ifdef __cplusplus
}
#endif

#endif
