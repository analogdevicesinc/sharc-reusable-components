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

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stdbool.h>

#include <runtime/int/interrupt.h>
#include <adi_osal.h>

#define MAX_THREAD_SLOTS 32

/*
 * WARNING:
 *  Calling certain RTL functions within interrupts (like sprintf()) can
 *  trigger a global RTL lock and cause a FreeRTOS assertion.  If you
 *  find you're hitting the assertion in portable/GCC/ARM_CA9/port.c ->
 *  vPortEnterCritical() then define ADI_OSAL_RTL_GLOBAL_IRQ_SAFE_LOCK
 *  below.  Doing so, however, will likely result in unacceptable interrupt
 *  latency so finding and relocating the RTL function out of the IRQ is
 *  likely a better option.
 *
 *  As an additional check, uncomment and set a breakpoint on the "nop"
 *  in adi_osal_RTLGlobalsLock() and adi_osal_RTLGlobalsUnlock() to confirm
 *  this issue is not present.
 *
 */
//#define ADI_OSAL_RTL_GLOBAL_IRQ_SAFE_LOCK

static uint32_t _threadSlotAllocated = 0x00000000;
static unsigned _osalEnterCriticalRegionCount = 0;
static UBaseType_t _osalEnterCriticalRegionValue;

#if (!defined (configNUM_OSAL_TLS_POINTERS) || configNUM_OSAL_TLS_POINTERS < 1)
#error Please set configNUM_OSAL_TLS_POINTERS >= 1 \
       in FreeRTOS config
#endif

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
static inline bool _adi_osal_IsCurrentLevelISR( void )
{
    uint32_t local_cpsr;
    uint32_t mode;

    asm ("MRS %0, CPSR" :"=r"(local_cpsr): :);

    mode = (local_cpsr & ADI_RTL_ARM_MODE_MASK);
    if ( (mode == ADI_RTL_ARM_MODE_USR) || (mode == ADI_RTL_ARM_MODE_SYS) )
        return false;
    else
        return true;
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
void adi_osal_EnterCriticalRegion( void )
{
    if (_adi_osal_IsCurrentLevelISR()) {
        if (_osalEnterCriticalRegionCount == 0) {
            _osalEnterCriticalRegionValue = taskENTER_CRITICAL_FROM_ISR();
        }
        _osalEnterCriticalRegionCount++;
    } else {
        taskENTER_CRITICAL();
    }
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
void adi_osal_ExitCriticalRegion( void )
{
    if (_adi_osal_IsCurrentLevelISR()) {
        _osalEnterCriticalRegionCount--;
        if (_osalEnterCriticalRegionCount == 0) {
            taskEXIT_CRITICAL_FROM_ISR(_osalEnterCriticalRegionValue);
        }
    } else {
        taskEXIT_CRITICAL();
    }
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_SemCreateStatic(
    void* const pSemObject, uint32_t nSemObjSize,
    ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    *phSem = (ADI_OSAL_SEM_HANDLE)xSemaphoreCreateCounting(
        (UBaseType_t)ADI_OSAL_SEM_MAX_COUNT, (UBaseType_t)nInitCount
    );
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_SemDestroyStatic(ADI_OSAL_SEM_HANDLE const hSem)
{
    vSemaphoreDelete((SemaphoreHandle_t)hSem);
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_SemPend(ADI_OSAL_SEM_HANDLE const hSem,
    ADI_OSAL_TICKS nTimeoutInTicks)
{
    TickType_t nTimeout = 0u;
    ADI_OSAL_STATUS osalStatus = ADI_OSAL_SUCCESS;
    BaseType_t semStatus;

    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            nTimeout = 0;
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            nTimeout = portMAX_DELAY;
            break;
        default:
            nTimeout = (TickType_t)nTimeoutInTicks;
            break;
    }

    semStatus = xSemaphoreTake((SemaphoreHandle_t)hSem, nTimeout);
    if (semStatus == pdFALSE) {
        osalStatus = ADI_OSAL_FAILED;
    }

    return(osalStatus);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_SemPost(ADI_OSAL_SEM_HANDLE const hSem)
{
    if (!_adi_osal_IsCurrentLevelISR()) {
        xSemaphoreGive((SemaphoreHandle_t)hSem);
    } else {
        BaseType_t contextSwitch;
        xSemaphoreGiveFromISR((SemaphoreHandle_t)hSem, &contextSwitch);
        portYIELD_FROM_ISR(contextSwitch);
    }
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
bool adi_osal_IsSchedulerActive(void)
{
    return(taskSCHEDULER_NOT_STARTED != xTaskGetSchedulerState());
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
void adi_osal_SchedulerLock(void)
{
    vTaskSuspendAll();
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_SchedulerUnlock(void)
{
    xTaskResumeAll();
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_RTLGlobalsLock( void)
{
#ifdef ADI_OSAL_RTL_GLOBAL_IRQ_SAFE_LOCK
    adi_osal_EnterCriticalRegion();
#else
#if 0
    if (_adi_osal_IsCurrentLevelISR()) {
        asm("nop;");
    }
#endif
    adi_osal_SchedulerLock();
#endif
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_RTLGlobalsUnlock( void)
{
#ifdef ADI_OSAL_RTL_GLOBAL_IRQ_SAFE_LOCK
    adi_osal_ExitCriticalRegion();
#else
#if 0
    if (_adi_osal_IsCurrentLevelISR()) {
        asm("nop;");
    }
#endif
    adi_osal_SchedulerUnlock();
#endif
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexCreate(ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    *phMutex = (ADI_OSAL_MUTEX_HANDLE)xSemaphoreCreateRecursiveMutex();
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexDestroy(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    vSemaphoreDelete((SemaphoreHandle_t)hMutex);
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexCreateStatic(
    void* const pMutexObject, uint32_t nMutexObjSize,
    ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    adi_osal_MutexCreate(phMutex);
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexDestroyStatic(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    adi_osal_MutexDestroy(hMutex);
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexPend(ADI_OSAL_MUTEX_HANDLE const hMutex,
    ADI_OSAL_TICKS nTimeoutInTicks)
{
    ADI_OSAL_STATUS osalStatus = ADI_OSAL_SUCCESS;
    TickType_t nTimeout = 0u;
    BaseType_t semStatus;

    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            nTimeout = 0;
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            nTimeout = portMAX_DELAY ;
            break;
        default:
            nTimeout = (TickType_t)nTimeoutInTicks;
            break;
    }

    semStatus = xSemaphoreTakeRecursive((SemaphoreHandle_t)hMutex, nTimeout);
    if (semStatus == pdFALSE) {
        osalStatus = ADI_OSAL_FAILED;
    }

    return(osalStatus);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexPost(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    xSemaphoreGiveRecursive((SemaphoreHandle_t)hMutex);
    return(ADI_OSAL_SUCCESS);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_ThreadSlotAcquire(
    ADI_OSAL_TLS_SLOT_KEY *pnThreadSlotKey,
    ADI_OSAL_TLS_CALLBACK_PTR pTerminateCallbackFunc)
{
    ADI_OSAL_STATUS ret = ADI_OSAL_FAILED;
    int threadSlots;

    threadSlots =
        configNUM_OSAL_TLS_POINTERS < MAX_THREAD_SLOTS ?
        configNUM_OSAL_TLS_POINTERS : MAX_THREAD_SLOTS;

    vTaskSuspendAll();

    if (*pnThreadSlotKey == ADI_OSAL_TLS_UNALLOCATED) {
        for (int i = 0; i < threadSlots; i++) {
            uint32_t bit = (1u << i);
            if ((_threadSlotAllocated & bit) == 0) {
                _threadSlotAllocated |= bit;
                *pnThreadSlotKey = i;
                ret = ADI_OSAL_SUCCESS;
                break;
            }
        }
    } else {
        if (_threadSlotAllocated & (1u << *pnThreadSlotKey)) {
            ret = ADI_OSAL_SUCCESS;
        } else {
            ret = ADI_OSAL_SLOT_NOT_ALLOCATED;
        }
    }

    xTaskResumeAll();

    return(ret);
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_ThreadSlotRelease(
    ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey)
{
    vTaskSuspendAll();
    _threadSlotAllocated &= ~(1u << nThreadSlotKey);
    xTaskResumeAll();
    return ADI_OSAL_SUCCESS;
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_ThreadSlotSetValue(
    ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey,
    ADI_OSAL_SLOT_VALUE   slotValue)
{
    vTaskSetThreadLocalStoragePointer(NULL, nThreadSlotKey, slotValue);
    return ADI_OSAL_SUCCESS;
}

/*****************************************************************************
    WARNING: THIS IS NOT THE ORIGINAL ADI FreeRTOS OSAL FUNCTION!!
*****************************************************************************/
ADI_OSAL_STATUS adi_osal_ThreadSlotGetValue(
    ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey,
    ADI_OSAL_SLOT_VALUE   *pSlotValue)
{
    *pSlotValue = pvTaskGetThreadLocalStoragePointer(NULL, nThreadSlotKey);
    return ADI_OSAL_SUCCESS;
}

