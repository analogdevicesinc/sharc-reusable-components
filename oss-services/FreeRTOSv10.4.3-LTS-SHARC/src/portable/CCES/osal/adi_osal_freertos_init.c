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
    @file adi_osal_freertos_init.c

    Operating System Abstraction Layer - OSAL for FreeRTOS

*/
/** @addtogroup ADI_OSAL_Init ADI OSAL Initialization
 *  @{
 *
 *   This module contains the APIs designed to initialize the OSAL and
 *   abstract the FreeRTOS operating system from the user.
 */
/*=============  I N C L U D E S   =============*/

#include <limits.h>
#include <stdlib.h>

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

/* disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm143")
#endif
/*! @endcond */


/* "version.h" contain macro "ADI_BUILD_VER"  which gives the
    build version for OSAL for FreeRTOS. This is a generated file.
*/
#include "version.h"

/*=============  D E F I N E S  =============*/


/*! @details priority of the virtual startup "thread" */
#define STARTUP_PRIORITY    0u
#define ADI_OSAL_INVALID_HEAP_INDEX (-1)



/*=============  D A T A  =============*/
/*!
    @internal
    @var _adi_osal_gnTickPeriod
         defines the length of system ticks in microseconds
    @endinternal
 */
uint32_t _adi_osal_gnTickPeriod = UINT_MAX;


/*!
    @internal
    @var _adi_osal_oStartupVirtualThread
         This thread is not a real thread, but is active until the OS starts.
         It will allow the TLS functions to operate until the OS takes over.
    @endinternal
*/

ADI_OSAL_THREAD_INFO _adi_osal_oStartupVirtualThread;

/*=============  C O D E  =============*/

/*!
  ****************************************************************************
    @brief Initializes OSAL.

    This function initializes the internal OSAL data structure. It should be
    called during the system startup.

    @return ADI_OSAL_SUCCESS          - Initialization is done successfully.
    @return ADI_OSAL_OS_ERROR         - The version of OSAL is not compatible
                                        with the FreeRTOS version
    @return ADI_OSAL_MEM_ALLOC_FAILED - Error initializing dynamic memory heap

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_Init(void)
{
    uint32_t* pHeapMemory = NULL; /*ptr to the memory to use for the heap */
    uint32_t nHeapMemorySize = 0u;/*size of memory pointed by pHeapMemory */
    /*!
        @internal
        @var snOsalInitializationState
             Static variable to record if OSAL has already been initialized
        @endinternal
    */
#ifdef OSAL_DEBUG
    static uint32_t snOsalInitializationState = 0u ;
#endif

    /* Checks that the version of FreeRTOS is compatible with this version of OSAL */
    if (GetOSVersion() < COMPATIBLE_OS_VERSION)
    {
        return (ADI_OSAL_OS_ERROR);
    }

#ifdef OSAL_DEBUG
    /* Check if already initialized. If the parameters are the same then the
     * call succeeds. Otherwise the call fails. The priority inheritance
     * setting is not recorded so we cannot check it
     */
    if (OSAL_INITIALIZED == snOsalInitializationState)
    {
        return (ADI_OSAL_SUCCESS);
    }
#endif

    /* automatically sets the tick period based on the FreeRTOS settings */
    _adi_osal_gnTickPeriod = USEC_PER_SEC / configTICK_RATE_HZ;

    if (ADI_OSAL_SUCCESS != _adi_osal_HeapInstall(pHeapMemory,nHeapMemorySize))
    {
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }


    /* Create the thread that represents the current execution as a thread until
     * the OS actually starts
     */
//    _adi_osal_oStartupVirtualThread.nThreadSignature = ADI_OSAL_THREAD_SIGNATURE;

#ifdef OSAL_DEBUG
    snOsalInitializationState = OSAL_INITIALIZED;
#endif


    return( ADI_OSAL_SUCCESS);
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm143")
#endif
/*! @endcond */

/*
**
** EOF:
**
*/
/*@}*/
