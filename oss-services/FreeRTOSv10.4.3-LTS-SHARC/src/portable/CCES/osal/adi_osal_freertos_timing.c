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
    @file adi_osal_freertos_timing.c

    Operating System Abstraction Layer - OSAL for Free RTOS - Timing
    functions

*/
/** @addtogroup ADI_OSAL_Timing ADI OSAL Timing
 *  @{
 *
 * This module contains the Timing APIs for the FreeRTOS implementation of
 * OSAL
 */

/*=============  I N C L U D E S   =============*/

#include <limits.h>

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

/*  disable misra diagnostics as necessary
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

/*=============  D A T A  =============*/




/*=============  C O D E  =============*/


/*!
  ****************************************************************************
    @brief Returns the duration of a tick period in microseconds.

    @param[out] pnTickPeriod - pointer to a location to write the tick period
                               in microseconds.

    @return ADI_OSAL_SUCCESS - If the function successfully returned the
                               duration of the tick period in microseconds.
    @return ADI_OSAL_FAILED  - If the tick period is set to UINT_MAX
  Notes:
      This function  helps to convert  time units to system ticks which is
      needed by the pend APIs of message-Q,semaphore,mutex,event  and to
      put the task in "sleep" mode.

                                                   No. Microsec in one second
      Duration of the tick period (in micro second) =  -------------------------
                                                   No of ticks in one second
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_TickPeriodInMicroSec(uint32_t *pnTickPeriod)
{
    /* In Free RTOS there no API to get Tick Period in Micro Sec. For this reason
     *  they must be set by calling adi_osal_Config.
     */
    if (UINT_MAX == _adi_osal_gnTickPeriod)
    {
        *pnTickPeriod = UINT_MAX;
        return ADI_OSAL_FAILED ;
    }
    else
    {
        *pnTickPeriod = _adi_osal_gnTickPeriod;
        return ADI_OSAL_SUCCESS ;
    }
}



/*!
  ****************************************************************************
  @brief Processes a clock tick

  This indicates to the OS that a tick period is completed.

*****************************************************************************/

void adi_osal_TimeTick(void)
{
    /* In FreeRTOS the Tick ISR is provided by the port itself and hence just return*/
    return;
}



/*!
  ****************************************************************************
    @brief Returns the current value of the continuously incrementing timer
           tick counter.

    The counter increments once for every timer interrupt.

    @param[out] pnTicks - pointer to a location to write the current value of
                          the tick counter.

    @return ADI_OSAL_SUCCESS - If the function successfully returned the tick
                               counter value

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetCurrentTick(uint32_t *pnTicks )
{

    /* FreeRTOS has separate tick count function for ISR */
    if(CALLED_FROM_AN_ISR)
    {
        *pnTicks = (uint32_t)xTaskGetTickCountFromISR();
    }
    else
    {
        *pnTicks = (uint32_t)xTaskGetTickCount();
    }

    return ADI_OSAL_SUCCESS;
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
