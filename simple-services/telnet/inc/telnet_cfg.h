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

#ifndef _telnet_cfg_h
#define _telnet_cfg_h

#include "umm_malloc.h"

#define TELNET_PORT            23
#define TELNET_MAX_CONNECTIONS 5

#define TELNET_CALLOC          umm_calloc
#define TELNET_FREE            umm_free

#endif
