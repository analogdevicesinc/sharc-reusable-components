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

#ifndef _telnet_h
#define _telnet_h

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

portTASK_FUNCTION(telnetTask, pvParameters);
void telnet_start(void);

#endif
