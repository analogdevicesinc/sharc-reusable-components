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

#ifndef _usb_timer_h
#define _usb_timer_h

#include <stdbool.h>
#include <services/tmr/adi_tmr.h>

typedef void (*USB_TIMER_CALLBACK)(void *usrPtr);

ADI_TMR_HANDLE usb_timer_start(uint32_t timerNum);
bool usb_timer_register(USB_TIMER_CALLBACK cb, void *usr);

#endif
