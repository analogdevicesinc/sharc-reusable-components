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
    @file adi_osal_freertos_critical.c

    Operating System Abstraction Layer - OSAL for FreeRTOS - Critical section
    functions

*/
/** @addtogroup ADI_OSAL_Critical ADI OSAL Critical
 *  @{
 *
 * This module contains the critical section & scheduler locking APIs for the
 * FreeRTOS implementation of OSAL
 */

/*=============  I N C L U D E S   =============*/


#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_common.h"
#include "osal_freertos.h"
#include "task.h"

/*  disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm128]: illegal implicit conversion from underlying MISRA type
 *                (MISRA C 2004 rule 10.1)
 *
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm128,Pm143")
#endif
/*! @endcond */

/*=============  D A T A  =============*/

#ifdef OSAL_DEBUG
/*!
    @internal
    @var _adi_osal_gnSchedulerLockCnt
         Indicates if the code is within a Scheduler lock section. It is only
         used in debug mode to check if Unlock is called with Lock being called
         first. This needs to be a counter to allow for nesting
         Initially the scheduler is not locked
    @endinternal
*/
uint32_t _adi_osal_gnSchedulerLockCnt = 0u ;

#endif /* OSAL_DEBUG */

/*!
    @internal
    @var _adi_osal_gnCriticalRegionNestingCnt
         This variable is a counter which is incremented when
         adi_osal_EnterCriticalRegion() is called and decremented when
         adi_osal_ExitCriticalRegion is called.
         Initially we are not in a critical region.
    @endinternal
*/
int32_t _adi_osal_gnCriticalRegionNestingCnt = 0;


/*!
    @internal
    @var snCriticalRegionState
         Holds the state of the interrupt mask as of the first call to
         adi_osal_EnterCriticalRegion
    @endinternal
*/
static UBaseType_t snCriticalRegionState = 0u;


/*=============  C O D E  =============*/




/*!
  ****************************************************************************
    @brief Determines whether the scheduler is running.

    @return true  - If the scheduler is running,
    @return false - If the scheduler is not running
*****************************************************************************/

bool adi_osal_IsSchedulerActive(void)
{
    return (taskSCHEDULER_NOT_STARTED != xTaskGetSchedulerState());
}


/*!
  ****************************************************************************
    @brief Prevents rescheduling until adi_osal_SchedulerUnlock is called.

    After this function is called, the current thread does not become
    de-scheduled , even if a high-priority thread becomes ready to run.

    Note that calls to adi_osal_SchedulerLock may be nested. A count is
    maintained to ensure that a matching number of calls to
    adi_osal_SchedulerUnlock are made before scheduling is re-enabled.

    @see adi_osal_SchedulerUnlock
*****************************************************************************/

void adi_osal_SchedulerLock( void )
{
    vTaskSuspendAll();

#ifdef OSAL_DEBUG
    /* FreeRTOS xTaskResumeAll function takes care of nesting itself, so _adi_osal_gnSchedulerLockCnt
     * is only useful for diagnostics or debugging.
     */
     _adi_osal_gnSchedulerLockCnt++;
#endif /* OSAL_DEBUG */
}


/*!
  ****************************************************************************
    @brief Re-enables thread scheduling.

    This function decrements the internal count which tracks how many times
    adi_osal_SchedulerLock was called. The API relies on the RTOS to
    enable scheduling when appropriate

    @return ADI_OSAL_SUCCESS - If thread scheduling was enabled successfully
    @return ADI_OSAL_FAILED  - If the function call does not match a call to
                               adi_osal_SchedulerLock

    @see adi_osal_SchedulerLock
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_SchedulerUnlock( void )
{
#ifdef OSAL_DEBUG
	/* FreeRTOS xTaskResumeAll function takes care of nesting itself, so _adi_osal_gnSchedulerLockCnt
	 * is only useful for diagnostics or debugging.
	 */
	if (0u == _adi_osal_gnSchedulerLockCnt)
	{
		return ADI_OSAL_FAILED;       /* if the Unlock function is called before the lock, return an error */
	}
	_adi_osal_gnSchedulerLockCnt--;             /* it must be done before unlocking */
#endif /* OSAL_DEBUG */

	xTaskResumeAll();

	return ADI_OSAL_SUCCESS;
}

/*!
  ****************************************************************************
    @brief Disables interrupts to enable atomic execution of a critical region
    of code.

    Note that critical regions may be nested. A count is maintained to ensure a
    matching number of calls to adi_ExitCriticalRegion are made before
    restoring interrupts. Each critical region is also (implicitly) a scheduler
    lock.

    @see adi_osal_ExitCriticalRegion
*****************************************************************************/
void adi_osal_EnterCriticalRegion( void )
{
	if (CALLED_FROM_AN_ISR)
	{
		/* Accessing to the global count variable needs to be protected from thread
		 * switches and nested interrupts so interrupt disable is called at the very
		 * beginning. The value of the interrupts state is only saved the first time.
		 */
		UBaseType_t istate = taskENTER_CRITICAL_FROM_ISR();

		if (0 == _adi_osal_gnCriticalRegionNestingCnt)
		{
			/* Only save the state for the outermost call */
			snCriticalRegionState = istate;
		}
	}
	else
	{
		taskENTER_CRITICAL();
	}

	_adi_osal_gnCriticalRegionNestingCnt++;
}

/*!
  ****************************************************************************
    @brief Re-enables interrupts and restores the interrupt status.

    This function decrements the count of nested critical regions. Use it as a
    closing bracket to adi_osal_EnterCriticalRegion. OSAL ignores additional
    calls to adi_osal_ExitCriticalRegion while interrupts are enabled.

    @see adi_osal_EnterCriticalRegion
*****************************************************************************/

void adi_osal_ExitCriticalRegion( void )
{
	_adi_osal_gnCriticalRegionNestingCnt--;

	if (CALLED_FROM_AN_ISR)
	{
		/* When the last nesting level is reached, reenable the interrupts */
		if (_adi_osal_gnCriticalRegionNestingCnt <= 0)
		{
			_adi_osal_gnCriticalRegionNestingCnt = 0;
			taskEXIT_CRITICAL_FROM_ISR(snCriticalRegionState);
		}
	}
	else
	{
		taskEXIT_CRITICAL();
	}
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm128,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
