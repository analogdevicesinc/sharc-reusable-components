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

#ifndef _sae_irq_h
#define _sae_irq_h

#include "sae.h"

SAE_RESULT sae_enableInterrupt(SAE_CONTEXT *context, bool ipcMaster);
SAE_RESULT sae_raiseInterrupt(SAE_CONTEXT *context, int8_t coreIdx);
uint32_t sae_getInterruptID(void);

#endif
