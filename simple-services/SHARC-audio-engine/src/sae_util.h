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

#ifndef _sae_util_h
#define _sae_util_h

#include "sae.h"

void sae_lockIpc(void);
void sae_unLockIpc(void);

SAE_RESULT sae_broadcastEvent(SAE_CONTEXT *context, SAE_EVENT event, void *data, uint8_t len);

#endif
