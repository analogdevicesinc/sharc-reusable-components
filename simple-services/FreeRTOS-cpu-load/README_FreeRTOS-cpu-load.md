# simple-services/FreeRTOS-cpu-load

## Overview

The FreeRTOS-cpu-load service tracks a the CPU load across FreeRTOS tasks and ISRs.  A single optional "background" task can be registered which will not be included in the overall CPU load measurement.

## Required components

- FreeRTOS

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

The FreeRTOS-cpu-load service has no compile-time configurable options.

## Run

- See the example code below.

## Example

### Put the following in FreeRTOSConfig.h:

```C
/* Task switch hook for idle time calculations */
void taskSwitchHook(void *taskHandle);
#define traceTASK_SWITCHED_IN()  taskSwitchHook(pxCurrentTCB)
```

### Initialize the module:

```C
/* The CGU timestamp clock was initialized elsewhere to this value */
#define CGU_TS_CLK   (SYSCLK / 16)

/***********************************************************************
 * CPU idle time / High precision timestamp functions
 **********************************************************************/
uint32_t getTimeStamp(void)
{
    uint32_t timeStamp;
    timeStamp = *pREG_CGU0_TSCOUNT0;
    return timeStamp;
}

void taskSwitchHook(void *taskHandle)
{
    cpuLoadtaskSwitchHook(taskHandle);
}

...
...
...

/* Initialize the CPU load module.  Treat the 'backgroundTask' task as an idle
 * task for CPU load calculations.
 */
cpuLoadInit(getTimeStamp, CGU_TS_CLK);
cpuLoadBackgroundTask(backgroundTaskHandle);
```

### Put the following into relevant ISRs that should also be tracked:

```C
uint32_t inCycles, outCycles;

/* Track ISR cycle count for CPU load */
inCycles = cpuLoadGetTimeStamp();

//
// DO PROCESSING HERE
//

/* Track ISR cycle count for CPU load */
outCycles = cpuLoadGetTimeStamp();
cpuLoadIsrCycles(outCycles - inCycles);
```

### Periodically compute the CPU load:

```C
/* Compute the CPU load periodically in a background task */
uint32_t percentCpuLoad, maxCpuLoad;
percentCpuLoad = cpuLoadCalculateLoad(&maxCpuLoad);
```

### Can also query the last CPU load and optionally clear the max value:

```C
uint32_t percentCpuLoad, maxCpuLoad;
percentCpuLoad = cpuLoadGetLoad(&maxCpuLoad, true);
printf("CPU Load: %u%% (%u%% peak)\n", percentCpuLoad, maxCpuLoad);
```

## Info
- ISRs calling into `cpuLoadIsrCycles()` cannot be reentrant
