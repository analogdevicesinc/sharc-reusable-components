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

/*!
 * @brief  A crude FreeRTOS CPU load tracker
 *
 * This module tracks a rough estimate of the CPU load of a FreeRTOS
 * project.
 *
 * @file      cpu_load.h
 * @version   1.0.0
 * @copyright 2019 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef _cpu_load_h
#define _cpu_load_h

#include <stdint.h>
#include <stdbool.h>

/*!****************************************************************
 * @brief  Function called to track time
 *
 * The application must pass in a function which returns a
 * monotonically increasing high-resolution time.
 *
 * Also, the timer must not overflow a uint32_t between
 * calls to cpuLoadCalculateLoad().  It is OK if the timer
 * rolls over zero.
 *
 * This function will likely be called from an ISR.
 ******************************************************************/
typedef uint32_t (*CPU_LOAD_GET_TIME)(void);

/*!****************************************************************
 * @brief  Initializes the CPU load module
 *
 * This function initializes the CPU load tracking module
 *
 * This function is not thread safe.
 *
 * @param [in]  getTime  Pointer to a function to call to get the
 *                       current time.
 *
 *        [in]  ticksPerSecond  Number of clock ticks per second as
 *                              returned by the CPU_LOAD_GET_TIME
 *                              callback function.
 *
 ******************************************************************/
void cpuLoadInit(CPU_LOAD_GET_TIME getTime, uint32_t ticksPerSecond);

/*!****************************************************************
 * @brief  FreeRTOS task switch hook
 *
 * This function must be called from the FreeRTOS task switch hook
 *
 * @param [in]  taskHandle  Task handle as given by the FreeRTOS task
 *                          switch hook.
 ******************************************************************/
void cpuLoadtaskSwitchHook(void *taskHandle);

/*!****************************************************************
 * @brief  Accumulates CPU cycles in ISR routines
 *
 * This function accumulates CPU cycles expended during otherwise
 * idle time.  This function is expected to be called from ISR
 * handlers.
 *
 * The cycles passed in through 'isrCycles' must be the same unit of
 * measure as the CPU_LOAD_GET_TIME routine passed into cpuLoadInit().
 *
 * @param [in]  isrCycles  Task handle as given by the FreeRTOS task
 *                          switch hook.
 ******************************************************************/
void cpuLoadIsrCycles(uint32_t isrCycles);

/*!****************************************************************
 * @brief  Assign a background task
 *
 * The task given will be included in the "idle" time.  This is
 * useful to eliminate a low priority background thread running a
 * tight loop which would result in 100% CPU load.
 *
 * @param [in]  taskHandle  Task handle to exclude from CPU load.
 ******************************************************************/
void cpuLoadBackgroundTask(void *taskHandle);

/*!****************************************************************
 * @brief  Calculate a new CPU load
 *
 * Calculates the average CPU load since the last time called
 *
 * @param [out]  maxLoad  Reports back the max CPU load
 *
 * @return  Average CPU load since the last time called
 ******************************************************************/
uint32_t cpuLoadCalculateLoad(uint32_t *maxLoad);

/*!****************************************************************
 * @brief  Return the last CPU load
 *
 * Returns the last calculated CPU load
 *
 * @param [out]  maxLoad   Reports back the MAX CPU load
 * @param [in]   clearMax  Clear the max CPU load
 *
 * @return  Average CPU load
 ******************************************************************/
uint32_t cpuLoadGetLoad(uint32_t *maxLoad, bool clearMax);

/*!****************************************************************
 * @brief  Return a CPU timestamp suitable for use in
 *         cpuLoadIsrCycles()
 *
 * Returns a CPU timestamp suitable for use in cpuLoadIsrCycles()
 *
 * @return  CPU timestamp
 ******************************************************************/
uint32_t cpuLoadGetTimeStamp(void);

/*!****************************************************************
 * @brief  Convert the given cycles to microseconds
 *
 * Returns the microsecond equivalent of 'cycles'
 *
 * @param [in]   cycles  An absolute or elapsed number of CPU
 *                       cycles as returned by cpuLoadIsrCycles()
 *
 * @return  Microsecond equivalent of cycles
 ******************************************************************/
uint32_t cpuLoadCyclesToMicrosecond(uint32_t cycles);

#endif
