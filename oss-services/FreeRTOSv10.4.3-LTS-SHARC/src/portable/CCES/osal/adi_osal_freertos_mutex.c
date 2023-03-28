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
    @file adi_osal_freertos_mutex.c

  Operating System Abstraction Layer - OSAL for FreeRTOS - Mutex functions

*/
/** @addtogroup ADI_OSAL_Mutex ADI OSAL Mutex
 *  @{
 * This module contains the Mutex APIs for the FreeRTOS implementation of OSAL

 * @note: priority inheritance is always enabled in FreeRTOS
 */

/*=============  I N C L U D E S   =============*/

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

#include <limits.h>
#include <stdlib.h>

/* disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm114]: an 'if (expression)' construct shall be followed by a compound
 *              statement. The 'else' keyword shall be followed by either a
 *              compound statement, or another 'if'  statement
 *              (MISRA C 2004 rule 14.9)
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
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm114,Pm140,Pm141,Pm143")
#endif
/*! @endcond */



/*=============  D A T A  =============*/




/*=============  C O D E  =============*/



#if( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_RECURSIVE_MUTEXES == 1 ) )

/*!
  ****************************************************************************
    @brief Creates a mutex.

    @param[out] phMutex  - Pointer to a location to write the returned mutex
                           handle

    @return ADI_OSAL_SUCCESS      - If mutex is created successfully
    @return ADI_OSAL_FAILED       - If failed to create mutex
    @return ADI_OSAL_MEM_ALLOC_FAILED - If memory could not be allocated
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location
    @return ADI_OSAL_BAD_MEMORY       - If the allocated memory is not word aligned.
    @return ADI_OSAL_MEM_TOO_SMALL    - If there isn't sufficient memory to create
                                           the mutex
     "phMutex" points to NULL if mutex creation is failed.

 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreate(ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    ADI_OSAL_STATUS eRetStatus;
    SemaphoreHandle_t pSemaphore;

#ifdef OSAL_DEBUG
    if (NULL == phMutex)
    {
        return ADI_OSAL_FAILED;
    }

#endif /* OSAL_DEBUG */

    if (CALLED_FROM_AN_ISR)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return ADI_OSAL_CALLER_ERROR;
    }

    pSemaphore = xSemaphoreCreateRecursiveMutex();

    if(NULL != pSemaphore)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
        *phMutex = (ADI_OSAL_MUTEX_HANDLE) pSemaphore;
    }
    else
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        eRetStatus = ADI_OSAL_FAILED;
    }

    return eRetStatus;
}

/*!
  ****************************************************************************
    @brief This function is used to delete a mutex.

    @param[in] hMutex      - Handle of the mutex to be deleted

    @return ADI_OSAL_SUCCESS      - If mutex is deleted successfully
    @return ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location
      @note
      Only owner is authorized to release the acquired mutex. But it
      can "destroyed" by  other task.

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_MutexDestroy(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if(ADI_OSAL_INVALID_MUTEX == hMutex)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    vSemaphoreDelete( (SemaphoreHandle_t) hMutex );

    return ADI_OSAL_SUCCESS;
}

#endif /* ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) */

#if( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_RECURSIVE_MUTEXES == 1 ) )

/*!
  ****************************************************************************
    @brief Creates a mutex using user allocated memory for the mutex

    THIS FUNCTION IS THE SAME AS adi_osal_MutexCreate except that memory is
    passed to it instead of relying on dynamic memory.

    @param[out] phMutex      - Pointer to a location to write the returned
                               mutex handle
    @param[in] nMutexObjSize - Size of the memory passed for the creation of
                               the mutex
    @param[in] pMutexObject  - Area of memory provided to us for the mutex

    @return ADI_OSAL_SUCCESS            - If mutex is created successfully
    @return ADI_OSAL_FAILED             - If failed to create mutex
    @return ADI_OSAL_MEM_TOO_SMALL    - If there isn't sufficient memory to create
                                           the mutex
    @return ADI_OSAL_CALLER_ERROR       - If the function is invoked from an invalid
                                        location
    @return ADI_OSAL_BAD_MEMORY       - If the allocated memory is not word aligned.

    @see adi_osal_MutexCreate
    @see adi_osal_MutexDestroyStatic
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexCreateStatic(void* const pMutexObject, uint32_t nMutexObjSize, ADI_OSAL_MUTEX_HANDLE *phMutex)
{
    ADI_OSAL_STATUS eRetStatus;
    SemaphoreHandle_t pMutex;

#ifdef OSAL_DEBUG
    if ((false == _adi_osal_IsMemoryAligned(pMutexObject)) || (NULL == pMutexObject) || (nMutexObjSize < sizeof(StaticSemaphore_t)))
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return ADI_OSAL_BAD_MEMORY;
    }

    if (NULL == phMutex)
    {
        return ADI_OSAL_FAILED;
    }

#endif /* OSAL_DEBUG */

    if (CALLED_FROM_AN_ISR)
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        return ADI_OSAL_CALLER_ERROR;
    }

    pMutex = xSemaphoreCreateRecursiveMutexStatic(pMutexObject);

    if(NULL != pMutex)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
        *phMutex = (ADI_OSAL_MUTEX_HANDLE) pMutex;
    }
    else
    {
        *phMutex = ADI_OSAL_INVALID_MUTEX;
        eRetStatus = ADI_OSAL_FAILED;
    }

    return eRetStatus ;
}
#endif /* ( configSUPPORT_STATIC_ALLOCATION == 1 ) */


#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )


/*!
  ****************************************************************************
    @brief Deletes a mutex without freeing memory

    This API is designed to destroy a mutex that has been allocated with
    adi_osal_MutexCreateStatic().

    @param[in] hMutex      - Handle of the mutex to be deleted

    @return ADI_OSAL_SUCCESS      - If mutex is deleted successfully
    @return ADI_OSAL_BAD_HANDLE   - If the specified mutex handle is invalid
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location
    @note
      Only owner is authorized to release the acquired mutex. But it
      can "destroyed" by  other task.

    @see adi_osal_MutexCreateStatic
    @see adi_osal_MutexDestroy

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexDestroyStatic(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if(ADI_OSAL_INVALID_MUTEX == hMutex)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    vSemaphoreDelete( (SemaphoreHandle_t) hMutex );

    return ADI_OSAL_SUCCESS;
}

#endif /* ( configSUPPORT_STATIC_ALLOCATION == 1 ) */

/*!
  ****************************************************************************
  @brief Returns the size of a mutex object.

  This function can be used by the adi_osal_MutexCreateStatic function to
  determine what the object size should be for this particular RTOS
  implementation.

  Parameters:
      None

    @return size of a mutex object in bytes.



*****************************************************************************/

uint32_t adi_osal_MutexGetObjSize(void)
{
    return ( FREERTOS_SEMAPHORE_OBJ_SIZE );
}

#if( configUSE_RECURSIVE_MUTEXES == 1 )

/*!
  ****************************************************************************
  @brief Acquires a mutex with a timeout

  This function is used to lock a mutex (acquire a resource)

    @param[in] hMutex            - Handle of the mutex which need to be acquired
    @param[in] nTimeoutInTicks   - Specify the number of system ticks for
                                   acquiring the mutex

      Valid timeouts are:

        ADI_OSAL_TIMEOUT_NONE       -  No wait. Results in an immediate return
                                       from this service  regardless of whether
                                       or not it was successful

        ADI_OSAL_TIMEOUT_FOREVER    -  Wait option for calling task to suspend
                                       indefinitely until a specified  mutex is
                                       obtained

        1 ....0xFFFFFFFE            -  Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining specified mutex

    @return ADI_OSAL_SUCCESS      - If the specified mutex is locked
                                    successfully
    @return ADI_OSAL_TIMEOUT      - If the specified time limit expired.
    @return ADI_OSAL_BAD_HANDLE   - If the specified mutex ID is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location
    @return ADI_OSAL_BAD_TIME     - If the timeout value is invalid
    @return ADI_OSAL_FAILED       - If pend operation is failed due to timeout


*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPend(ADI_OSAL_MUTEX_HANDLE const hMutex, ADI_OSAL_TICKS nTimeoutInTicks)
{
    ADI_OSAL_STATUS eRetStatus;
    TickType_t nTimeout = 0u;

#ifdef OSAL_DEBUG

    if((nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) &&
        (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
        return ADI_OSAL_BAD_TIME;
    }

    if (CALLED_BEFORE_OS_RUNNING)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if(hMutex == ADI_OSAL_INVALID_MUTEX)
    {
        return ADI_OSAL_BAD_HANDLE;
    }

#endif

    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            nTimeout = FREERTOS_NONBLOCKING_CALL;
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            nTimeout = portMAX_DELAY ;
            break;
        default:
            nTimeout = (TickType_t)nTimeoutInTicks ;
            break;
     }  /* end of switch */

    switch (xSemaphoreTakeRecursive((SemaphoreHandle_t) hMutex, nTimeout ))
    {
        case  pdTRUE :
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

        case errQUEUE_EMPTY:
            eRetStatus = ADI_OSAL_TIMEOUT;
            break;

        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return eRetStatus;
}



/*!
  ****************************************************************************

    @brief Unlocks a mutex.

    @param[in] hMutex      - Handle of the mutex which needs to be unlocked

    @return ADI_OSAL_SUCCESS          - If mutex is un locked successfully
    @return ADI_OSAL_FAILED           - If failed unlock mutex
    @return ADI_OSAL_BAD_HANDLE       - If the specified mutex ID is invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid location
                                        (i.e an ISR)

    @note
         Mutex can be successfully released by its owner : Only the task which
         acquired it can release it. Any attempt to release it by non-owner will
         result in error.

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MutexPost(ADI_OSAL_MUTEX_HANDLE const hMutex)
{
    ADI_OSAL_STATUS eRetStatus;

#ifdef OSAL_DEBUG
    if (hMutex == ADI_OSAL_INVALID_MUTEX)
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif

    switch (xSemaphoreGiveRecursive((SemaphoreHandle_t) hMutex))
    {
        case  pdTRUE :
            eRetStatus = ADI_OSAL_SUCCESS;
            break;
#ifdef OSAL_DEBUG
        case errQUEUE_FULL:
            eRetStatus = ADI_OSAL_COUNT_OVERFLOW;
            break;
#endif
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    }

    return eRetStatus;
}

#endif /* configUSE_RECURSIVE_MUTEXES == 1 */

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm114,Pm140,Pm141,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
