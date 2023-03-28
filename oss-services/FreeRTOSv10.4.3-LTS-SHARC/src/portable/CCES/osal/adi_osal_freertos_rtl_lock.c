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
    @file adi_osal_freertos_rtl_lock.c

    Operating System Abstraction Layer - OSAL for FreeRTOS


*/
/** @addtogroup ADI_OSAL_RTL_LOCK ADI OSAL RTL_LOCK
 *  @{
 *
 *  This module contains the APIs designed to abstract the runtime library
 *  locking mechanism so that each RTOS can adapt to them according to its needs.
 */

/*=============  I N C L U D E S   =============*/


#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"


/*!
  ****************************************************************************
    @brief Acquires the RTL global lock

    @details
    API used by the runtime library for its internal locking mechanism. In the
    case of FreeRTOS this is implemented as a scheduler region lock because
    functions like heap_free must be allowed in the task destruction hook which
    is called with the scheduler lock held.

    Other RTOS might implement this API as a mutex pend.

    @return ADI_OSAL_SUCCESS - In all cases

    @see adi_osal_SchedulerLock
    @see adi_osal_RTLGlobalsUnlock

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_RTLGlobalsLock( void)
{
    adi_osal_SchedulerLock();

    return( ADI_OSAL_SUCCESS);
}


/*!
  ****************************************************************************
    @brief Releases the RTL global lock

    @details
    API used by the runtime library for its internal locking mechanism. In the
    case of FreeRTOS this is implemented as a scheduler region unlock because
    functions like heap_free must be allowed in the task destruction hook which
    is called with the scheduler lock held.

    @return ADI_OSAL_SUCCESS - If the lock was released successfully
    @return ADI_OSAL_FAILED  - If the function call does not match a call to
                               adi_osal_RTLGlobalsLock/adi_osal_SchedulerLock

    @see adi_osal_SchedulerUnlock
    @see adi_osal_RTLGlobalsLock
*****************************************************************************/


ADI_OSAL_STATUS adi_osal_RTLGlobalsUnlock( void)
{
    return (adi_osal_SchedulerUnlock());

}

/*
**
** EOF:
**
*/
/*@}*/
