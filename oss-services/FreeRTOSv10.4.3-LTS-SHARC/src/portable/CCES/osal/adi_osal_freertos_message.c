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
    @file adi_osal_freertos_message.c

    Operating System Abstraction Layer - OSAL for FreeRTOS - Message Queue
    functions


*/
/** @addtogroup ADI_OSAL_Message ADI OSAL Message Queues
 *  @{
 *
 *   This module contains the Message Queue APIs for the FreeRTOS implementation
 *   of OSAL
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
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm114,Pm127,Pm140,Pm143")
#endif
/*! @endcond */


/*=============  D A T A  =============*/




/*=============  C O D E  =============*/


#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

/*!
  ****************************************************************************
    @brief Creates a message queue used for inter-task communication.

    The Message is always a pointer which points to the base of
    a buffer which contains the actual message (indirect message). Hence the
    size of the message in the queue is always 4 bytes (a pointer).


    @param[out] phMsgQ    - Pointer to a location to write the returned message
                            queue ID
    @param[in]  aMsgQ     - Buffer to be used to store the messages
    @param[in]  nMaxMsgs  - Maximum number of messages the queue can hold

    @return ADI_OSAL_SUCCESS       - If message queue is created successfully
    @return ADI_OSAL_FAILED        - If failed to create message queue
    @return ADI_OSAL_CALLER_ERROR  - If function is invoked from an invalid
                                     location
    @return ADI_OSAL_BAD_HANDLE    - If the pointer to copy the handle is
                                     provided as NULL

*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueueCreate(ADI_OSAL_QUEUE_HANDLE *phMsgQ, void* aMsgQ[], uint32_t nMaxMsgs)
{
    ADI_OSAL_STATUS eRetStatus;
    QueueHandle_t pMessageQ;

    if (NULL == phMsgQ)
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        return ADI_OSAL_CALLER_ERROR;
    }

    pMessageQ = xQueueCreate((UBaseType_t)nMaxMsgs, FREERTOS_MESSAGESIZE);

    if(NULL != pMessageQ)
    {
        *phMsgQ = (ADI_OSAL_QUEUE_HANDLE) pMessageQ;
        eRetStatus = ADI_OSAL_SUCCESS;
    }
    else
    {
        *phMsgQ = ADI_OSAL_INVALID_QUEUE;
        eRetStatus = ADI_OSAL_FAILED;
    }

    return eRetStatus;
}


/*!
  ****************************************************************************
    @brief Deletes the specified message queue

    @param[in] hMsgQ   -  handle of the message queue to be deleted

    @return ADI_OSAL_SUCCESS        - If message queue is deleted successfully
    @return ADI_OSAL_BAD_HANDLE     - If the specified message queue ID is
                                      invalid
    @return ADI_OSAL_CALLER_ERROR   - If function is invoked from an invalid
                                      location (i.e an ISR)
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueueDestroy(ADI_OSAL_QUEUE_HANDLE const hMsgQ)
{
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_SUCCESS;

    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((NULL == hMsgQ) || (ADI_OSAL_INVALID_QUEUE == hMsgQ))
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    vQueueDelete((QueueHandle_t) hMsgQ);

    return eRetStatus;
}

#endif /* configSUPPORT_DYNAMIC_ALLOCATION == 1 */

/*!
  ****************************************************************************
    @brief Sends a message to the specified message queue.

    @param[in] hMsgQ     - Handle of the message queue to use.
    @param[in] pMsg      - Pointer to the message to send

    @return ADI_OSAL_SUCCESS    - If message queued successfully
    @return ADI_OSAL_FAILED     - If failed to queue the message
    @return ADI_OSAL_BAD_HANDLE - If the specified message queue handle is
                                  invalid
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_MsgQueuePost(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void *pMsg)
{
    BaseType_t  nRetValue;
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;

#ifdef OSAL_DEBUG
    if ((ADI_OSAL_INVALID_QUEUE == hMsgQ) || (NULL == hMsgQ))
    {
       return ADI_OSAL_BAD_HANDLE;
    }
#endif /* OSAL_DEBUG */

    if (CALLED_FROM_AN_ISR)
    {
        BaseType_t nHigherPriorityTaskWoken = pdFALSE;

        nRetValue = xQueueSendToBackFromISR((QueueHandle_t) hMsgQ, pMsg, &nHigherPriorityTaskWoken);

        if(nRetValue != errQUEUE_FULL)
        {
            /* If nHigherPriorityTaskWoken was set to true you we should yield */
            portYIELD_FROM_ISR( nHigherPriorityTaskWoken );
        }
    }
    else
    {
        nRetValue = xQueueSendToBack((QueueHandle_t) hMsgQ, pMsg, FREERTOS_NONBLOCKING_CALL);
    }

    if (pdTRUE == nRetValue)
    {
        eRetStatus = ADI_OSAL_SUCCESS;
    }

    return eRetStatus;
}

/*!
  ****************************************************************************
    @brief Receives a message from the specified message queue.

    @param[in]  hMsgQ             -  handle of the Message queue to retrieve
                                     the message from
    @param[out] ppMsg             -  Pointer to a location to store the message
    @param[in]  nTimeoutInTicks   -  Timeout in system ticks for retrieving the
                                     message.

      Valid timeouts are:

         ADI_OSAL_TIMEOUT_NONE     -   No wait. Results in an immediate return
                                       from this service regardless of whether
                                       or not it was successful

         ADI_OSAL_TIMEOUT_FOREVER  -   suspends the calling thread indefinitely
                                       until a message is obtained

         1 ... 0xFFFFFFFE          -   Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining a message from the queue

    @return ADI_OSAL_SUCCESS      - If message is received and copied to ppMsg
                                    buffer and removed from queue.
    @return ADI_OSAL_FAILED       - If failed to get a message.
    @return ADI_OSAL_BAD_HANDLE   - If the specified message queue is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location (i.e an ISR)
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_MsgQueuePend(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void **ppMsg, ADI_OSAL_TICKS nTimeoutInTicks)
{
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;
    TickType_t nTimeTicks;

#ifdef OSAL_DEBUG
    if((nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) &&
       (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
         return ADI_OSAL_BAD_TIME;
    }
#endif /* OSAL_DEBUG */

    if (CALLED_IN_SCHED_LOCK_REGION)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((NULL == hMsgQ) || (ADI_OSAL_INVALID_QUEUE == hMsgQ))
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    switch (nTimeoutInTicks)
    {
        case ADI_OSAL_TIMEOUT_NONE:
            nTimeTicks = FREERTOS_NONBLOCKING_CALL;
            break;
        case ADI_OSAL_TIMEOUT_FOREVER:
            nTimeTicks = portMAX_DELAY;
            break;
        default:
            nTimeTicks = (TickType_t)nTimeoutInTicks;
            break;
    } /* end of switch */

    if(pdTRUE == xQueueReceive((QueueHandle_t) hMsgQ, *ppMsg, nTimeTicks))
    {
        eRetStatus = ADI_OSAL_SUCCESS;
    }

    return eRetStatus;
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm114,Pm140,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
