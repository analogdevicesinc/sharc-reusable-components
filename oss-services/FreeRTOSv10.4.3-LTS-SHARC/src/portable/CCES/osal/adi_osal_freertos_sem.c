/*
 * Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
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
    @file adi_osal_freertos_sem.c

    Operating System Abstraction Layer - OSAL for FreeRTOS - Semaphore
    functions

*/
/** @addtogroup ADI_OSAL_Sem ADI OSAL Semaphore
 *  @{
 *
 * This module contains the Semaphore APIs for the FreeRTOS implementation of
 * OSAL
 *
 */

/*=============  I N C L U D E S   =============*/

#include <stdlib.h>

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

/* disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm114]: an 'if (expression)' construct shall be followed by a compound
 *              statement. The 'else' keyword shall be followed by either a
 *              compound statement, or another 'if'  statement
 *              (MISRA C 2004 rule 14.9)
 * Error[Pm128]: illegal implicit conversion from underlying MISRA type
 *                (MISRA C 2004 rule 10.1)
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
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm114,Pm127,Pm128,Pm140,Pm141,Pm143")
#endif
/*! @endcond */


/*=============  D A T A  =============*/




/*=============  C O D E  =============*/

#if( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

/*!
  ****************************************************************************
    @brief Creates a counting semaphore  with the memory which has already been
            provided.

    @param[in] nSemObjSize - Size of the memory passed for the creation of
                             the semaphore. Not needed for FreeRTOS since kernel
                             provides the memory
    @param[in] pSemObject  - Area of memory provided to use for the semaphore.
                             Not needed for FreeRTOS since kernel provides the memory
    @param[out] phSem      - Pointer to a location to write the returned
                             semaphore ID
    @param[in]  nInitCount - Initial value for the creation of a counting
                             semaphore.
                             Semaphore will be created "unavailable" state if
                             "nInitCount" is equal to zero.

    @return ADI_OSAL_SUCCESS      - If semaphore is created successfully
    @return ADI_OSAL_FAILED       - If failed to create semaphore
    @return ADI_OSAL_BAD_COUNT    - The value specified in nInitCount is too
                                    large for the RTOS
    @return ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location
                                    (i.e an ISR)
    @return ADI_OSAL_BAD_MEMORY   - If the allocated memory is NULL or Word aligned

  Note:
      phSem  set  to "ADI_OSAL_INVALID_SEM" if semaphore creation is failed.
 *****************************************************************************/
ADI_OSAL_STATUS  adi_osal_SemCreateStatic(void* const pSemObject, uint32_t nSemObjSize, ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    ADI_OSAL_STATUS eRetStatus;
    SemaphoreHandle_t pSemaphore;

#ifdef OSAL_DEBUG
    if ((false == _adi_osal_IsMemoryAligned(pSemObject)) || (NULL == pSemObject) || (nSemObjSize < sizeof(StaticSemaphore_t)))
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_BAD_MEMORY);
    }
#endif /* OSAL_DEBUG */

#ifdef OSAL_DEBUG
    if (NULL == phSem)
    {
        return(ADI_OSAL_FAILED);
    }

 #endif /* OSAL_DEBUG */

    if (nInitCount > ADI_OSAL_SEM_MAX_COUNT)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_BAD_COUNT);
    }


    if (CALLED_FROM_AN_ISR)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return (ADI_OSAL_CALLER_ERROR);
    }

    /* the count of a FreeRTOS semaphore is BaseType_t so we typecast it. */
    pSemaphore = xSemaphoreCreateCountingStatic((UBaseType_t)ADI_OSAL_SEM_MAX_COUNT,(UBaseType_t) nInitCount, pSemObject);

    if(NULL != pSemaphore)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
        *phSem = (ADI_OSAL_SEM_HANDLE) pSemaphore;
    }
    else
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        eRetStatus = ADI_OSAL_FAILED;
    }

    return(eRetStatus);
}
/*!
  ****************************************************************************
    @brief Destroys a specified semaphore without freeing memory.

    @param[in]  hSem      - The handle of the semaphore which need to be deleted

    @return ADI_OSAL_SUCCESS          - If semaphore is deleted successfully
    @return ADI_OSAL_BAD_HANDLE       - If the specified semaphore handle is
                                        invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid
                                        location (i.e an ISR).

 *****************************************************************************/
ADI_OSAL_STATUS  adi_osal_SemDestroyStatic(ADI_OSAL_SEM_HANDLE const hSem)
{
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if(ADI_OSAL_INVALID_SEM == hSem)
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    vSemaphoreDelete( (SemaphoreHandle_t) hSem );

    return ADI_OSAL_SUCCESS;
}
#endif /* ( configSUPPORT_STATIC_ALLOCATION == 1 ) */


#if( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

/*!
  ****************************************************************************
    @brief Creates a counting semaphore.

    @param[out] phSem      - Pointer to a location to write the returned
                             semaphore ID
    @param[in]  nInitCount - Initial value for the creation of a counting
                             semaphore.
                             Semaphore will be created "unavailable" state if
                             "nInitCount" is equal to zero.

    @return ADI_OSAL_SUCCESS      - If semaphore is created successfully
    @return ADI_OSAL_FAILED       - If failed to create semaphore
    @return ADI_OSAL_BAD_COUNT    - The value specified in nInitCount is too
                                    large for the RTOS
    @return ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location
                                    (i.e an ISR)

  Note:
      phSem  set  to "ADI_OSAL_INVALID_SEM" if semaphore creation is failed.
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemCreate(ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount)
{
    ADI_OSAL_STATUS eRetStatus;
    SemaphoreHandle_t pSemaphore;

#ifdef OSAL_DEBUG
    if (NULL == phSem)
    {
        return ADI_OSAL_FAILED;
    }

 #endif /* OSAL_DEBUG */

    if (nInitCount > ADI_OSAL_SEM_MAX_COUNT)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return ADI_OSAL_BAD_COUNT;
    }


    if (CALLED_FROM_AN_ISR)
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        return ADI_OSAL_CALLER_ERROR;
    }

    /* the count of a FreeRTOS semaphore is BaseType_t so we typecast it. */
    pSemaphore = xSemaphoreCreateCounting((UBaseType_t)ADI_OSAL_SEM_MAX_COUNT,(UBaseType_t) nInitCount);

    if(NULL != pSemaphore)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
        *phSem = (ADI_OSAL_SEM_HANDLE) pSemaphore;
    }
    else
    {
        *phSem = ADI_OSAL_INVALID_SEM;
        eRetStatus = ADI_OSAL_FAILED;
    }

    return eRetStatus;
}

/*!
  ****************************************************************************
    @brief Deletes a specified semaphore.

    @param[in]  hSem      - The handle of the semaphore which need to be deleted

    @return ADI_OSAL_SUCCESS          - If semaphore is deleted successfully
    @return ADI_OSAL_BAD_HANDLE       - If the specified semaphore handle is
                                        invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid
                                        location (i.e an ISR).

 *****************************************************************************/

ADI_OSAL_STATUS  adi_osal_SemDestroy(ADI_OSAL_SEM_HANDLE const hSem)
{
    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if(ADI_OSAL_INVALID_SEM == hSem)
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    vSemaphoreDelete( (SemaphoreHandle_t) hSem );

    return ADI_OSAL_SUCCESS;
}

#endif /* ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) */

/*!
  ****************************************************************************
  @brief Returns the size of a semaphore object.

  This function can be used by the adi_osal_SemCreateStatic function to
  determine what the object size should be for this particular RTOS
  implementation.

  Parameters:
      None

    @return size of a semaphore object in bytes.

    @see adi_osal_SemCreateStatic

*****************************************************************************/

uint32_t adi_osal_SemGetObjSize(void)
{
    return ( FREERTOS_SEMAPHORE_OBJ_SIZE );
}


/*!
  ****************************************************************************
    @brief Waits for access to a semaphore

    If the specified semaphore is acquired, its count will be decremented.


    @param[in]  hSem             - Handle of the semaphore to obtain
    @param[in]  nTimeoutInTicks  - Specify the number of system ticks after
                                   which obtaining the semaphore will return.

            Valid timeouts are:

            ADI_OSAL_TIMEOUT_NONE     No wait. Results in an immediate return
                                      from this service regardless of whether
                                      or not it was successful
            ADI_OSAL_TIMEOUT_FOREVER  Wait option for calling task to suspend
                                      indefinitely  until a semaphore instance
                                      is obtained
            1....0xFFFFFFFE           Selecting a numeric value specifies the
                                      maximum time limit (in system ticks ) for
                                      obtaining a semaphore


    @return ADI_OSAL_SUCCESS    -  If semaphore acquired successfully
    @return ADI_OSAL_TIMEOUT    -  If the API failed to acquire the semaphore
                                   within the specified time limit
    @return ADI_OSAL_BAD_HANDLE -  If the specified semaphore handle is invalid
    @return ADI_OSAL_BAD_TIME   -  If the specified time is invalid for the RTOS
    @return ADI_OSAL_CALLER_ERROR- If the function is invoked from an invalid
                                   location
    @return ADI_OSAL_FAILED      - If pend operation is failed

 *****************************************************************************/

ADI_OSAL_STATUS  adi_osal_SemPend(ADI_OSAL_SEM_HANDLE const hSem, ADI_OSAL_TICKS nTimeoutInTicks)
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

    if(hSem == ADI_OSAL_INVALID_SEM)
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

    switch (xSemaphoreTake((SemaphoreHandle_t) hSem, nTimeout))
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
    @brief Posts a semaphore

    The semaphore count will be incremented if the specified semaphore is
    posted successfully and there were no threads pending on it

    @param[in]  hSem - Handle of the semaphore to be posted

    @return ADI_OSAL_SUCCESS     - If semaphore was posted successfully
      @return ADI_OSAL_FAILED      - If an error occured while posting the
                                   specified semaphore
    @return ADI_OSAL_BAD_HANDLE  - If the specified semaphore handle is invalid
    @return ADI_OSAL_COUNT_OVERFLOW - If the semaphore count has exceeded the
                                      max limit

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_SemPost(ADI_OSAL_SEM_HANDLE const hSem)
{
    BaseType_t nRetValue;
    ADI_OSAL_STATUS eRetStatus;
    BaseType_t nHigherPriorityTaskWoken = pdFALSE;

#ifdef OSAL_DEBUG
    if(hSem == ADI_OSAL_INVALID_SEM)
    {
        return ADI_OSAL_BAD_HANDLE;
    }
#endif

    if (CALLED_FROM_AN_ISR)
    {
         nRetValue = xSemaphoreGiveFromISR((SemaphoreHandle_t) hSem, &nHigherPriorityTaskWoken );

        if(nRetValue != errQUEUE_FULL)
        {
            /* If nHigherPriorityTaskWoken was set to true you we should yield */
               portYIELD_FROM_ISR( nHigherPriorityTaskWoken );
        }
    }
    else
    {
        nRetValue = xSemaphoreGive((SemaphoreHandle_t) hSem);
    }

    switch (nRetValue)
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

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm114,Pm128,Pm140,Pm141,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
