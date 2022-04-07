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

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* CCES includes */
#include <services/gpio/adi_gpio.h>

/* Project includes */
#include "init.h"
#include "clocks.h"

/* The priorities assigned to the tasks (higher number == higher prio). */
#define LED_FLASH_PRIORITY          (tskIDLE_PRIORITY + 1)
#define STARTUP_TASK_LOW_PRIORITY   (tskIDLE_PRIORITY + 2)
#define STARTUP_TASK_HIGH_PRIORITY  (tskIDLE_PRIORITY + 3)

/* The check task uses the sprintf function so requires a little more stack. */
#define STARTUP_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE + 512)
#define GENERIC_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE)

#if   defined (__SAM_V1__)
#define   STATUS_LED_GPIO_PORT ADI_GPIO_PORT_D
#define   STATUS_LED_GPIO_PIN  ADI_GPIO_PIN_1
#elif defined (__EZKIT_573__)
#define   STATUS_LED_GPIO_PORT ADI_GPIO_PORT_E
#define   STATUS_LED_GPIO_PIN  ADI_GPIO_PIN_13
#endif

/***********************************************************************
 * CPU idle time / High precision timestamp functions
 **********************************************************************/
uint32_t getTimeStamp(void)
{
    uint32_t timeStamp;
    timeStamp = *pREG_CGU0_TSCOUNT0;
    return timeStamp;
}

uint32_t elapsedTimeMs(uint32_t elapsed)
{
    return(((1000ULL) * (uint64_t)elapsed) / CGU_TS_CLK);
}

void taskSwitchHook(void *taskHandle)
{
}

/***********************************************************************
 * Misc application utility functions (util.h)
 **********************************************************************/
void delay(unsigned ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/***********************************************************************
 * Tasks
 **********************************************************************/

static portTASK_FUNCTION( ledFlashTask, pvParameters )
{
    TickType_t flashRate, lastFlashTime;

    /* Configure the LED to flash at a 1Hz rate */
    flashRate = pdMS_TO_TICKS(500);
    lastFlashTime = xTaskGetTickCount();

    /* Spin forever flashing the LED */
    while (1) {
        adi_gpio_Toggle(STATUS_LED_GPIO_PORT, STATUS_LED_GPIO_PIN);
        vTaskDelayUntil( &lastFlashTime, flashRate );
    }
}

static portTASK_FUNCTION( startupTask, pvParameters )
{
    /* Start the LED flash task */
    xTaskCreate( ledFlashTask, "LedFlashTask", GENERIC_TASK_STACK_SIZE,
        NULL, LED_FLASH_PRIORITY, NULL );

    /* Lower the startup task priority */
    vTaskPrioritySet( NULL, STARTUP_TASK_LOW_PRIORITY);

    /* Spin forever (basic background main loop) */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

int main(int argc, char *argv[])
{
    /* Initialize system clocks */
    system_clk_init();

    /* Enable the CGU timestamp */
    cgu_ts_init();

    /* Initialize the GIC */
    gic_init();

    /* Initialize GPIO */
    gpio_init();

    /* Create the tasks */
    xTaskCreate( startupTask, "StartupTask", STARTUP_TASK_STACK_SIZE,
        NULL, STARTUP_TASK_HIGH_PRIORITY, NULL );

    /* Start the scheduler. */
    vTaskStartScheduler();

    return(0);
}

/*-----------------------------------------------------------
 * FreeRTOS critical error and debugging hooks
 *-----------------------------------------------------------*/
void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    ( void ) pcFile;
    ( void ) ulLine;

    /* Disable interrupts so the tick interrupt stops executing, then sit in a loop
    so execution does not move past the line that failed the assertion. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(STATUS_LED_GPIO_PORT, STATUS_LED_GPIO_PIN);
    while (1);
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(STATUS_LED_GPIO_PORT, STATUS_LED_GPIO_PIN);
    while (1);
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Run time allocation failure checking is performed if
    configUSE_MALLOC_FAILED_HOOK is defined.  This hook
    function is called if an allocation failure is detected. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(STATUS_LED_GPIO_PORT, STATUS_LED_GPIO_PIN);
    while (1);
}

/*-----------------------------------------------------------*/
