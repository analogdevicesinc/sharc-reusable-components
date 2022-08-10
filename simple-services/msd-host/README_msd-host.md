# simple-services/msd-host

## Overview

This service implements the application run-time functions required to coordinate the activities between the CLD Host Mass Storage library and msd_simple device driver.

## Required components

- 3rd-party/CLD-USB-Host-Mass-Storage-SC5xx

## Recommended components

- simple-drivers/msd-simple
- simpler-services/usb-timer

## Integrate the source

- Copy the 'inc' directory contents into a project include directory.
- Copy the 'src' directory contents into a project static library directory

## Configure

- Edit `msd_host_cfg.h` if necessary

## Run

- Initialize by calling `msd_host_init_()`
- Configure by calling `msd_host_config()`
- Start a periodic 125uS timer and call `msd_host_run()`.  This timer must
  be active prior to calling `msd_host_start()`
- Start by calling `msd_host_start()`
- Periodically call `msd_host_run()`.  I/O performance is directly linked
  to how often this function is called so the faster the better.

## Example code (SAM board)

msd.h
```
#ifndef _msd_h
#define _msd_h

#include "FreeRTOS.h"
#include "task.h"

#include "msd_simple.h"

void msdIdleTick(void);
sMSD *msdGetHandle(void);

portTASK_FUNCTION(msdTask, pvParameters);

#endif
```

msd.c
```
#include <stdint.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Application includes */
#include "usb_timer.h"
#include "msd_simple.h"
#include "msd_host.h"
#include "msd.h"

extern sMSD *msdHandle;
static MSD_HOST_APP_CONFIG msdHostCfg;

static void msd_timer_tick(void *usr)
{
    msd_host_125us_timer_tick();
}

void msdIdleTick(void)
{
    CLD_RV ret;

    vTaskSuspendAll();
    ret = msd_host_run();
    xTaskResumeAll();
}

sMSD *msdGetHandle(void)
{
    return(msdHandle);
}

portTASK_FUNCTION(msdTask, pvParameters)
{
    uint32_t whatToDo;
    CLD_RV ret;

    /* Configure MSD application settings */
    msdHostCfg.port = SC58x_USB1;
    msdHostCfg.useBuiltInVbusCtrl = CLD_FALSE;
    msdHostCfg.vbusCtrlOpenDrain = CLD_FALSE;
    msdHostCfg.vbusCtrlInverted = CLD_FALSE;
    msdHostCfg.vbusEnablePort = CLD_GPIO_PORT_A;
    msdHostCfg.vbusEnablePin = CLD_GPIO_PIN_15;
    msdHostCfg.usrPtr = NULL;

    /* Register with the periodic 125uS USB timer */
    usb_timer_register(msd_timer_tick, NULL);

    /* Initialize, configure, and start the CLD library */
    ret = msd_host_init_();
    ret = msd_host_config(&msdHostCfg);
    ret = msd_host_start();

    while (1) {

        /* Run the CLD library */
        ret = msd_host_run();

        /* Wait for something interesting or 1mS */
        whatToDo = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));
    }
}
```
