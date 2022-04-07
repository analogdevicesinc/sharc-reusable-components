# oss-services/FreeRTOSv10.2.1-ARM

## Overview

The `FreeRTOSv10.2.1-ARM` directory contains a cleaned-up version of the
ADI port (version 1.3.1) of FreeRTOS for the SC589 ARM core applied to
FreeRTOS version 10.2.1.

Unrelated files have been stripped for clarity.  The OSAL components and
UART console I/O files delivered with the ADI port have also been stripped.

There are many dire warnings in the 1.3.1 documentation about using this
version with the pre-compiled system services and device drivers.  With the
OSAL and UART I/O stripped out, this port is very similar to the previous
version that did not have such warnings.  Proceed with caution if using
ADI system services and device drivers with this module as it's unclear
as to the root-cause of the warnings.

## Required components

- None

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain a reasonable FreeRTOS configuration as a starting point.

The following directories must be part of the include path:

```
src/include
src/portable/GCC/ARM_CA9
```

Source from the following directories must be built:

```
src/portable/MemMang
src/portable/CCES/ARM_CA5
src/portable/GCC/ARM_CA9
```

## Configure

FreeRTOS has many configurable options.  Refer to the FreeRTOS documentation for additional details.

## Run

The following code illustrates how to initialize FreeRTOS and start a task.

```C
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* The priorities assigned to the tasks (higher number == higher prio). */
#define STARTUP_TASK_HIGH_PRIORITY  (tskIDLE_PRIORITY + 1)

/* Define stack sizes for the tasks */
#define STARTUP_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE + 4192)

static portTASK_FUNCTION( startupTask, pvParameters )
{
    while (1) {
        /* Do something interesting here */
    }
}

int main(int argc, char *argv[])
{
    /* Put system init code here */

    /* Create the tasks */
    xTaskCreate( startupTask, "StartupTask", STARTUP_TASK_STACK_SIZE,
        NULL, STARTUP_TASK_HIGH_PRIORITY, NULL );

    /* Start the scheduler. */
    vTaskStartScheduler();
}

/*-----------------------------------------------------------*/

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    ( void ) pcFile;
    ( void ) ulLine;

    /* Disable interrupts so the tick interrupt stops executing, then sit in a loop
    so execution does not move past the line that failed the assertion. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(ADI_GPIO_PORT_D, ADI_GPIO_PIN_1);
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
    while (1);
}

/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook( void )
{
    /* Run time allocation failure checking is performed if
    configUSE_MALLOC_FAILED_HOOK is defined.  This hook
    function is called if an allocation failure is detected. */
    taskDISABLE_INTERRUPTS();
    while (1);
}

/*-----------------------------------------------------------*/
```

