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

/* Standard lib includes */
#include <stddef.h>

/* ADI service includes */
#include <services/tmr/adi_tmr.h>

#include "clocks.h"
#include "cdc.h"
#include "user_cdc.h"

/**
 * Mainline state machine states.
 */
typedef enum
{
    MAIN_STATE_SYSTEM_INIT,                     /*!< Initialize the system */
    MAIN_STATE_USER_INIT,                       /*!< Initialize the User code */
    MAIN_STATE_RUN,                             /*!< Runtime state */
    MAIN_STATE_ERROR                            /*!< Error State */
} Main_States;

/***********************************************************************
 * Prototypes
 **********************************************************************/
static void timer_init (void);

/***********************************************************************
 * Generic functions
 **********************************************************************/
unsigned char TimerMemory[ADI_TMR_MEMORY];

void cdc_main_loop(void)
{
    static Main_States main_state = MAIN_STATE_SYSTEM_INIT;
    User_Init_Return_Code rv;

    /* Begin adding your custom code here */
    switch (main_state)
    {
        case MAIN_STATE_SYSTEM_INIT:
            timer_init();
            main_state = MAIN_STATE_USER_INIT;
            break;
        case MAIN_STATE_USER_INIT:
            rv = user_cdc_init();
            if (rv == USER_CDC_INIT_SUCCESS) {
                main_state = MAIN_STATE_RUN;
            }
            else if (rv == USER_CDC_INIT_FAILED) {
                main_state = MAIN_STATE_ERROR;
            }
        break;

        case MAIN_STATE_RUN:
            user_cdc_main();
            break;

        case MAIN_STATE_ERROR:
            break;
    }
}

#define CDC_TIMER_PERIOD  (SCLK0 / 8000)

static void timer_init (void)
{
    ADI_TMR_HANDLE   hTimer;
    ADI_TMR_RESULT   eTmrResult;

    /* Configure a 1ms Timer using the ADI system services */

    /* Open the timer */
    if( (eTmrResult = adi_tmr_Open (0,
                        TimerMemory,
                        ADI_TMR_MEMORY,
                        user_cdc_timer_handler,
                        0,
                        &hTimer)) != ADI_TMR_SUCCESS)
     {
         /* Failed to open the timer handle the error here */
     }

     /*
     * Use the GP timer's API's to configure and enable the timer
     *
     */

    /* Set the mode to PWM OUT */
    eTmrResult = adi_tmr_SetMode(hTimer, ADI_TMR_MODE_CONTINUOUS_PWMOUT);

    /* Set the IRQ mode to get interrupt after timer counts to Delay + Width */
    eTmrResult = adi_tmr_SetIRQMode(hTimer, ADI_TMR_IRQMODE_WIDTH_DELAY);

    /* Set the Period */
    eTmrResult = adi_tmr_SetPeriod(hTimer, CDC_TIMER_PERIOD);

    /* Set the timer width */
    eTmrResult = adi_tmr_SetWidth(hTimer, CDC_TIMER_PERIOD / 2);

    /* Set the timer delay */
    eTmrResult = adi_tmr_SetDelay(hTimer, 100);

    /* Enable the timer */
    eTmrResult = adi_tmr_Enable(hTimer, true);
}
