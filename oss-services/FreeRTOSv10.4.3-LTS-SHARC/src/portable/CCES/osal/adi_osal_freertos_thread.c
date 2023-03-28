/*
 * Copyright (C) 2016-2022 Analog Devices Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
 */

/*!
    @file adi_osal_freertos_thread.c

    Operating System Abstraction Layer - OSAL for FreeRTOS - Thread
    functions


*/
/** @addtogroup ADI_OSAL_Threads ADI OSAL Threads
 *  @{
 *
 * This module contains the Thread APIs for the FreeRTOS implementation of
 * OSAL.
 */
/*=============  I N C L U D E S   =============*/

#include <string.h>                                                             /* for strncpy */
#include <stddef.h>

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

/* disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm088]: pointer arithmetic should not be used (MISRA C 2004 rule 17.4)
 *
 * Error[Pm127]: a 'U' suffix shall be applied to all constants of 'unsigned' type
 *                (MISRA C 2004 rule 10.6)
 *
 * Error[Pm128]: illegal implicit conversion from underlying MISRA type
 *                (MISRA C 2004 rule 10.1)
 *
 * Error[Pm138]: conversions shall not be performed between a pointer to a
 *                 function and any other type other than an integral type
 *                 (MISRA C 2004 rule 11.1)
 *
 * Error[Pm139]: conversions shall not be performed between a pointer to object
 *                 and any type other than an integral type
 *                (MISRA C 2004 rule 11.2)
 *
 * Error[Pm140]: a cast should not be performed between a pointer type and an
 *                integral type (MISRA C 2004 rule 11.3)
 *
 * Error[Pm141]: a cast should not be performed between a pointer to object type
 *                  and a different pointer to object type(MISRA C 2004 rule 11.4)
 *
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm088,Pm127,Pm128,Pm138,Pm139,Pm140,Pm141,Pm143")
#endif
/*! @endcond */
/*=============  D A T A  =============*/




/*=============  C O D E  =============*/



/*!
  ****************************************************************************
    @brief Creates a thread and puts it in the ready state.

    @param[in] pThreadAttr - Pointer to the (pre-initialized)
                             ADI_OSAL_THREAD_ATTR structure
    @param[out] phThread   - Pointer to a location to return the thread handle
                             if the thread is created successfully

    @return ADI_OSAL_SUCCESS              - if thread creation is successful
    @return ADI_OSAL_FAILED               - if thread creation fails
    @return ADI_OSAL_BAD_THREAD_FUNC      - if the specified function address is Null
    @return ADI_OSAL_BAD_STACK_SIZE       - Stack size is not word aligned
    @return ADI_OSAL_BAD_THREAD_NAME      - If the specified thread name exceeds
                                            ADI_OSAL_MAX_THREAD_NAME
    @return ADI_OSAL_CALLER_ERROR         - if function is invoked from an
                                            invalid location (i.e an ISR)
    @return ADI_OSAL_MEM_ALLOC_FAILED     - Memory allocation for the RTOS object
                                            failed
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadCreate(ADI_OSAL_THREAD_HANDLE *phThread, const ADI_OSAL_THREAD_ATTR *pThreadAttr)
{
#ifdef OSAL_DEBUG
    if (NULL == phThread)
    {
        return ADI_OSAL_FAILED;
    }
#endif

    *phThread = ADI_OSAL_INVALID_THREAD;

#ifdef OSAL_DEBUG
    /* verify that the given structure is not NULL before starting to access the fields */
    if (NULL == pThreadAttr)
    {
        return ADI_OSAL_FAILED;
    }

    if (NULL == pThreadAttr->pThreadFunc)
    {
        return ADI_OSAL_BAD_THREAD_FUNC;
    }

    if (0u == pThreadAttr->nStackSize)
    {
        return ADI_OSAL_BAD_STACK_SIZE;
    }

    if (0u != (pThreadAttr->nStackSize & 0x3u))
    {
        return ADI_OSAL_BAD_STACK_SIZE;
    }

    if (strlen(pThreadAttr->szThreadName) > ADI_OSAL_MAX_THREAD_NAME)
    {
        return ADI_OSAL_BAD_THREAD_NAME;
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phThread = ADI_OSAL_INVALID_THREAD;
        return ADI_OSAL_CALLER_ERROR;
    }
#endif

    /* Convert the stack size from bytes to elements of StackType_t */
    const uint32_t nStkSize = pThreadAttr->nStackSize / sizeof(StackType_t);

    /*  ADI_OSAL_FREERTOS_BASE_PRIO is the idle task priority defined
     *  to tskIDLE_PRIORITY */
    const UBaseType_t nAssignedPrio = pThreadAttr->nPriority + ADI_OSAL_FREERTOS_BASE_PRIO;

    /* Creating a task by calling native OS call */
    TaskHandle_t nativeThread;
    const BaseType_t nRetValue =
    		xTaskCreate ((TaskFunction_t) pThreadAttr->pThreadFunc,
    				pThreadAttr->szThreadName,
					nStkSize,
					pThreadAttr->pTaskAttrParam,
					nAssignedPrio,
					&nativeThread);

    switch (nRetValue)
    {
    case  pdPASS :
    	/* The FreeRTOS native task handle is used as the thread handle */
    	*phThread = (ADI_OSAL_THREAD_HANDLE) nativeThread;
    	return ADI_OSAL_SUCCESS;

    	/* Not enough memory to create task */
    case  errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
    	return ADI_OSAL_MEM_ALLOC_FAILED;

    default:
    	return ADI_OSAL_FAILED;
    }
}

/*!
  ****************************************************************************
    @brief Deletes a thread (hence can no longer be scheduled)
           In the FreeRTOS port, a task can delete itself without issues

    @param[in] hThread - Handle of the thread to be deleted, NULL for self

    @return ADI_OSAL_SUCCESS      -  If successfully removed the thread from
                                     the system
    @return ADI_OSAL_FAILED       -  If failed to delete the thread
    @return ADI_OSAL_BAD_HANDLE   -  If the specified thread handle is invalid
    @return ADI_OSAL_CALLER_ERROR -  If function is invoked from an invalid
                                     location (i.e an ISR)

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadDestroy(ADI_OSAL_THREAD_HANDLE const hThread)
{
#ifdef OSAL_DEBUG
    /* check validity of the handle */
    if (ADI_OSAL_INVALID_THREAD == hThread)
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }
#endif

    vTaskDelete((TaskHandle_t)hThread);

    return ADI_OSAL_SUCCESS;
}



/*!
  ****************************************************************************
    @brief Returns the native handle of the current thread.

    @param[in] phThread - pointer to a location to write the current thread
                          native handle upon successful.

    @return ADI_OSAL_SUCCESS  - if the OS was running and there is a thread
    @return ADI_OSAL_FAILED   - if location handle address is invalid or this API
                                is called before the OS has started where no threads
                                are running
    @note "phThread" will be set to "ADI_OSAL_INVALID_THREAD" if not successful.

    @see adi_osal_ThreadGetHandle
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadGetNativeHandle(void **phThread)
{
#ifdef OSAL_DEBUG
    if (NULL == phThread)
    {
        return(ADI_OSAL_FAILED);
    }

    if (CALLED_BEFORE_OS_RUNNING)
    {
        * (ADI_OSAL_THREAD_HANDLE*) phThread = ADI_OSAL_INVALID_THREAD;
        return(ADI_OSAL_FAILED);
    }
#endif

    /*Call FreeRTOS API to retrieve the current task handle*/
    *(TaskHandle_t *)phThread = xTaskGetCurrentTaskHandle();

    return (ADI_OSAL_SUCCESS);
}

/*!
  ****************************************************************************
    @brief Returns the handle of the current thread if it is an OSAL thread.

    @param[in] phThread - pointer to a location to write the current OSAL thread
                          handle upon successful.

    @return ADI_OSAL_SUCCESS if the current thread was created with OSAL
    @return ADI_OSAL_FAILED  if the current thread was not created with OSAL

    @note "phThread" will be set to "ADI_OSAL_INVALID_THREAD" if not successful.

    @see adi_osal_ThreadGetNativeHandle
*****************************************************************************/

/* a typecast is necessary here because the thread handle type is incomplete
 * and is just an abstract pointer, the real structure (the type of
 * _adi_osal_oStartupVirtualThread) cannot be used directly because it is
 * hidden from the public interface. */

ADI_OSAL_STATUS  adi_osal_ThreadGetHandle(ADI_OSAL_THREAD_HANDLE *phThread)
{
#ifdef OSAL_DEBUG
    if (CALLED_BEFORE_OS_RUNNING)
    {
        *phThread  = (ADI_OSAL_THREAD_HANDLE) &_adi_osal_oStartupVirtualThread;
        return ADI_OSAL_SUCCESS;
    }

#if 0 /* BW */
    if ( !_adi_osal_IsOSALThread(hThreadNode) )
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif
#endif

    /* The current thread was an OSAL thread so we return the OSAL handle */
    *phThread = (ADI_OSAL_THREAD_HANDLE)xTaskGetCurrentTaskHandle();

    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief Returns the name of the currently executing thread


    @param[out] pszTaskName     - Pointer to the location to return the thread
                                  name
    @param[in]  nNumBytesToCopy - Number of bytes from the name to copy into
                                  the memory supplied as output argument

    @return ADI_OSAL_FAILED     - Unable to copy the task name
    @return ADI_OSAL_SUCCESS    - Successfully copied
    @return ADI_OSAL_BAD_HANDLE - The specified string pointer is invalid.

  *****************************************************************************/

ADI_OSAL_STATUS adi_osal_ThreadGetName(char_t *pszTaskName,
                                       uint32_t nNumBytesToCopy)
{
#ifdef OSAL_DEBUG
    if (NULL == pszTaskName)
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif

	const char_t *pcNativeName = pcTaskGetTaskName(NULL);
	int index = 0;

	while ('\0' != pcNativeName[index])
	{
		pszTaskName[index] = pcNativeName[index];
		index += 1;

		if (index >= nNumBytesToCopy)
	    {
	        return ADI_OSAL_FAILED;
	    }
	}

	pszTaskName[index] = '\0';

	return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief Returns the priority of a given thread.

    @param[in]  hThread      - handle to the thread that the API will return
                               the priority of.
    @param[out] pnThreadPrio - pointer to a location to write the thread
                               priority.

    @return ADI_OSAL_SUCCESS        -  If successfully returns the priority of the
                                       current thread
    @return ADI_OSAL_CALLER_ERROR   -  If function is invoked from an invalid
                                       location (i.e an ISR)
    @return ADI_OSAL_BAD_HANDLE     -  If the specified thread handle is invalid

    @note "*pnThreadPrio" will be set to "ADI_OSAL_INVALID_PRIORITY" if called from isr.

    @see adi_osal_ThreadSetPrio
*****************************************************************************/

#if INCLUDE_uxTaskPriorityGet == 1
ADI_OSAL_STATUS adi_osal_ThreadGetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY *pnThreadPrio)
{
#ifdef OSAL_DEBUG
	/*  Not allowed from an ISR */
	if (CALLED_FROM_AN_ISR)
	{
		*pnThreadPrio = (ADI_OSAL_PRIORITY) ADI_OSAL_INVALID_PRIORITY;
		return ADI_OSAL_CALLER_ERROR;
	}

	/* check validity of the handle */
	if (ADI_OSAL_INVALID_THREAD == hThread)
	{
		return ADI_OSAL_BAD_HANDLE;
	}
#if 0 /* BW */
	if ( !_adi_osal_IsOSALThread(hThreadNode) )
	{
		return ADI_OSAL_BAD_HANDLE;
	}
#endif
#endif /* OSAL_DEBUG */

	*pnThreadPrio = (ADI_OSAL_PRIORITY) uxTaskPriorityGet((TaskHandle_t)hThread);

	return ADI_OSAL_SUCCESS;
}
#endif


/*!
  ****************************************************************************
    @brief Changes the priority of the specified thread.

    @param[in] hThread       - Handle of the thread whose priority is to be
                               changed.
    @param[in] nNewPriority  - New desired priority.


    @return ADI_OSAL_SUCCESS      - If successfully changed the priority of the
                                    specified thread
    @return ADI_OSAL_BAD_PRIORITY - If the specified priority is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location (i.e an ISR)
    @return ADI_OSAL_BAD_HANDLE   -  If the specified thread handle is invalid
    @see adi_osal_ThreadGetPrio
*****************************************************************************/

#if INCLUDE_vTaskPrioritySet == 1
ADI_OSAL_STATUS adi_osal_ThreadSetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY nNewPriority)
{
#ifdef OSAL_DEBUG
    /* check validity of the handle */
    if (ADI_OSAL_INVALID_THREAD == hThread)
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#if 0 /* BW */
    if ( !_adi_osal_IsOSALThread(hThreadNode) )
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif
    /* FreeRTOS API cannot capture called from ISR Error, hence move it outside
     * OSAL_DEBUG */
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    /* FreeRTOS API doesn't check the prio when config assert is disabled hence move it outside
     * OSAL_DEBUG */
    if(nNewPriority >= configMAX_PRIORITIES)
    {
        return ADI_OSAL_BAD_PRIORITY;
    }
#endif

    const UBaseType_t nNewPrio = (UBaseType_t) (nNewPriority + ADI_OSAL_FREERTOS_BASE_PRIO);

    vTaskPrioritySet((TaskHandle_t)hThread, nNewPrio );

    return ADI_OSAL_SUCCESS;
}
#endif

/*!
  ****************************************************************************
    @brief Stops the current thread running for the specified time in system
    ticks.

    @param[in] nTimeInTicks - Amount of time  to sleep in system ticks.

    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location (i.e an ISR)
    @return ADI_OSAL_BAD_TIME     - If nTimeInTicks is more than ADI_OSAL_MAX_TIMEOUT
    @return ADI_OSAL_SUCCESS      - If successfully completed the "sleep" period

*****************************************************************************/
ADI_OSAL_STATUS  adi_osal_ThreadSleep(ADI_OSAL_TICKS nTimeInTicks)
{
#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if (CALLED_IN_SCHED_LOCK_REGION)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if(nTimeInTicks > ADI_OSAL_MAX_TIMEOUT)
    {
        return ADI_OSAL_BAD_TIME;
    }

#endif /* OSAL_DEBUG */

    if (0u != nTimeInTicks)
    {
    	/* nTimeTicks is non-zero, so call the RTOS delay function */
        vTaskDelay((TickType_t) nTimeInTicks);
    }

    return ADI_OSAL_SUCCESS;
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm088,Pm127,Pm128,Pm138,Pm139,Pm140,Pm141,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
