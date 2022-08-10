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

#ifndef _msd_host_h
#define _msd_host_h

#include <stdint.h>
#include <stdbool.h>

#include "msd_host_cfg.h"
#include "cld_sc5xx_msd_usb_host_lib.h"

typedef struct _MSD_HOST_APP_CONFIG {
    CLD_USB_Port  port;
    CLD_Boolean   useBuiltInVbusCtrl;
    CLD_Boolean   vbusCtrlOpenDrain;
    CLD_Boolean   vbusCtrlInverted;
    CLD_GPIO_Port vbusEnablePort;
    CLD_GPIO_PIN  vbusEnablePin;
    void          *usrPtr;
} MSD_HOST_APP_CONFIG;

typedef struct _MSD_HOST_INFO {
    uint64_t capacity;
} MSD_HOST_INFO;

#ifdef __cplusplus
extern "C"{
#endif

CLD_RV msd_host_init_(void);
CLD_RV msd_host_config(MSD_HOST_APP_CONFIG *cfg);
CLD_RV msd_host_start(void);
CLD_RV msd_host_run(void);
CLD_RV msd_host_stop(void);

/*
 * Call this function from an application supplied 125uS USB tick timer.
 * I.e. simple-services/usb-timer.  Ensure this timer is running prior
 * to calling msd_host_start() and while calling msd_host_run().
 */
CLD_RV msd_host_125us_timer_tick(void);

CLD_Boolean msd_host_ready(void);

CLD_Boolean msd_host_info(MSD_HOST_INFO *info);

typedef void (*MSD_HOST_READ_CALLBACK)(bool success, void *usr);
typedef void (*MSD_HOST_WRITE_CALLBACK)(bool success, void *usr);

CLD_RV msd_host_read(void *data, uint64_t sector, uint32_t count,
    MSD_HOST_READ_CALLBACK cb, void *usr);
CLD_RV msd_host_write(void *data, uint64_t sector, uint32_t count,
    MSD_HOST_WRITE_CALLBACK cb, void *usr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
