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
    @file adi_osal_freertos_event.c

    Operating System Abstraction Layer - OSAL for FreeRTOS - Events
    functions




*/
/** @addtogroup ADI_OSAL_Events ADI OSAL Events
 *  @{
 *  This module contains the Events APIs for the FreeRTOS implementation of
 *  OSAL
 *
 *  OSAL requires the events groups to be 32 bits, FreeRTOS allows only two
 *  options: 8 or 24 bits.  Users should have chosen the right option (32
 *  bits) but we cannot check because the information is only available when
 *  FreeRTOS debug is selected.
 */



/*=============  I N C L U D E S   =============*/

#include <string.h>

#include "osal_misra.h"                                                        /* for strncpy */
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
 *
 * Error[Pm128]: illegal implicit conversion from underlying MISRA type
 *                (MISRA C 2004 rule 10.1)
 *
 * Error[Pm140]: a cast should not be performed between a pointer type and an
 *                integral type (MISRA C 2004 rule 11.3)
 *
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm114,Pm127,Pm128,Pm140,Pm143")
#endif
/*! @endcond */
/*=============  D A T A  =============*/




/*=============  C O D E  =============*/



#if( configSUPPORT_STATIC_ALLOCATION == 1 )

/*!
  ****************************************************************************
    @brief Creates an event group with the memory which has already been
            provided.

    @param[in] nEventObjSize - Size of the memory passed for the creation of
                               the event group
    @param[in] pEventObject  - Area of memory provided to us for the event group

    This is typically used to synchronize threads with events that happen in
    the system.

    @param[out] phEventGroup - Pointer to a location to write the returned
                               event group handle

    @return ADI_OSAL_SUCCESS      - If event group is created successfully
    @return ADI_OSAL_FAILED       - If failed to create event group
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location
    @return ADI_OSAL_BAD_MEMORY  - If the memory provided by the us is NULL

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_EventGroupCreateStatic(void* const pEventObject, uint32_t nEventObjSize, ADI_OSAL_EVENT_HANDLE *phEventGroup)
{
    EventGroupHandle_t pEventNative;
    ADI_OSAL_STATUS eRetStatus;

#ifdef OSAL_DEBUG
    if ((false == _adi_osal_IsMemoryAligned(pEventObject)) || (NULL == pEventObject) || (nEventObjSize < sizeof(StaticEventGroup_t)))
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        return (ADI_OSAL_BAD_MEMORY);
    }
#endif /* OSAL_DEBUG */

    pEventNative =  xEventGroupCreateStatic(pEventObject);

	if(NULL == pEventNative)
	{
		*phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
		eRetStatus = ADI_OSAL_FAILED;
	}
	else
	{
		*phEventGroup = (ADI_OSAL_EVENT_HANDLE)pEventNative;
		eRetStatus = ADI_OSAL_SUCCESS;
	}

    return (eRetStatus);
}

/*!
  ****************************************************************************
    @brief Destroys the specified Event group without freeing memory

    @param[in] hEventGroup - handle of the event group to be destroyed

    @return ADI_OSAL_SUCCESS         - If event group is destroyed
                                       successfully
    @return ADI_OSAL_CALLER_ERROR    - If function is invoked from an
                                       invalid location
    @return ADI_OSAL_BAD_HANDLE      - Invalid event flag group ID

*****************************************************************************/
ADI_OSAL_STATUS adi_osal_EventGroupDestroyStatic(ADI_OSAL_EVENT_HANDLE const hEventGroup)
{
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    vEventGroupDelete((EventGroupHandle_t) hEventGroup);

    return ADI_OSAL_SUCCESS;
}

#endif /* ( configSUPPORT_STATIC_ALLOCATION == 1 ) */


#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

/*!
  ****************************************************************************
    @brief Creates a event group.

    @param[out] phEventGroup    - Pointer to a location to write the returned
                             event group ID

    @return ADI_OSAL_SUCCESS      - If event is created successfully
    @return ADI_OSAL_FAILED       - If failed to create event
    @return ADI_OSAL_CALLER_ERROR - If the call is made from an invalid location
                                    (i.e an ISR)

  Note:
      phEventGroup set to "ADI_OSAL_INVALID_EVENT_GROUP" if event creation is failed.
 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_EventGroupCreate(ADI_OSAL_EVENT_HANDLE *phEventGroup)
{
    EventGroupHandle_t pEventNative;
    ADI_OSAL_STATUS eRetStatus;

    if (NULL == phEventGroup)
    {
        return ADI_OSAL_FAILED;
    }

    if (CALLED_FROM_AN_ISR)
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        return ADI_OSAL_CALLER_ERROR;
    }

    /* Flags are initially all created as unset (0x0) */
    pEventNative =  xEventGroupCreate();
    if(NULL == pEventNative)
    {
        *phEventGroup = ADI_OSAL_INVALID_EVENT_GROUP;
        eRetStatus = ADI_OSAL_FAILED;
    }
    else
    {
        *phEventGroup = (ADI_OSAL_EVENT_HANDLE)pEventNative;
        eRetStatus = ADI_OSAL_SUCCESS;
    }

    return eRetStatus;
}

/*!
  ****************************************************************************
    @brief Deletes a specified event group.

    @param[in]  hEventGroup           - The handle of the event group which need
                                        to be deleted
    @return ADI_OSAL_SUCCESS          - If event group is deleted successfully
    @return ADI_OSAL_BAD_HANDLE       - If the specified event group handle is
                                        invalid
    @return ADI_OSAL_CALLER_ERROR     - If the call is made from an invalid
                                        location (i.e an ISR).

 *****************************************************************************/

ADI_OSAL_STATUS adi_osal_EventGroupDestroy(ADI_OSAL_EVENT_HANDLE const hEventGroup)
{
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }

    if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
    {
        return ADI_OSAL_BAD_HANDLE;
    }

    vEventGroupDelete((EventGroupHandle_t) hEventGroup);

    return ADI_OSAL_SUCCESS;
}

#endif /* ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) */

/*!
  ****************************************************************************
  @brief Returns the size of a event group object.

  This function can be used by the adi_osal_EventGroupCreateStatic function to
  determine what the object size should be for this particular RTOS
  implementation.

  Parameters:
      None

    @return size of a event group object in bytes.

    @see adi_osal_EventGroupCreateStatic

*****************************************************************************/
uint32_t adi_osal_EventGroupGetObjSize(void)
{
    return ( FREERTOS_EVENTGROUP_OBJ_SIZE );
}

/*!
  ****************************************************************************
    @brief Waits for event flags

    @param[in] hEventGroup      - handle of the event group to use
    @param[in] nRequestedEvents - Specifies requested event flags.
    @param[in] eGetOption       - Specifies whether all bits need to be
                                  set/cleared OR any of the bits to be set.
    @param[in] nTimeoutInTicks  - Timeout for the event flag in system ticks.
    @param[in] pnReceivedEvents - Pointer to destination of where the retrieved
                                  event flags are placed.


         The following options are valid for setting flag eGetOption.

            ADI_OSAL_EVENT_FLAG_SET_ANY  - check any of the bits specified by
                                           the nRequestedEvents is set
            ADI_OSAL_EVENT_FLAG_SET_ALL  - check all the bits specified by the
                                           nRequestedEvents are set.

         Valid options for nTimeoutInTicks  are:

           ADI_OSAL_TIMEOUT_NONE     -  No wait. Results in an immediate return
                                        from this service  regardless of whether
                                        or not it was successful
           ADI_OSAL_TIMEOUT_FOREVER  -  Wait option for calling task to suspend
                                        indefinitely until the required flags are
                                        set.
           1 ... 0XFFFFFFFE          -  Selecting a numeric value specifies the
                                        maximum time limit (in system ticks) for
                                        set required event flags

    @return ADI_OSAL_SUCCESS      -  If there is no error while retrieving the
                                     event flags. This does not indicate event
                                     flag condition - the user must read the
                                     flags separately.
    @return ADI_OSAL_BAD_HANDLE   -  If the specified event group is invalid.
    @return ADI_OSAL_BAD_TIME      -  If the timeout value specified is not
                                      within the limit
    @return ADI_OSAL_CALLER_ERROR -  If the function is invoked from an invalid
                                     location (i.e an ISR)
    @return ADI_OSAL_BAD_OPTION   -  If "eGetOption" specifies a wrong option.
    @return ADI_OSAL_BAD_EVENT    -  If the event group size is invalid
    @return ADI_OSAL_TIMEOUT      -  If Timeout happens before the event occurs.
*****************************************************************************/


ADI_OSAL_STATUS adi_osal_EventPend (ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                    ADI_OSAL_EVENT_FLAGS        nRequestedEvents,
                                    ADI_OSAL_EVENT_FLAG_OPTION  eGetOption,
                                    ADI_OSAL_TICKS              nTimeoutInTicks,
                                    ADI_OSAL_EVENT_FLAGS        *pnReceivedEvents)
{
    BaseType_t  nWaitOption = (BaseType_t)pdTRUE;
    ADI_OSAL_EVENT_FLAGS        nRetValue;
    ADI_OSAL_STATUS eRetStatus;
    TickType_t nTimeTicks;

    EventGroupHandle_t hEventNative = (EventGroupHandle_t) hEventGroup;

#ifdef OSAL_DEBUG
    if ( (nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) &&
         (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER) )
    {
         return (ADI_OSAL_BAD_TIME);
    }
#endif /* OSAL_DEBUG */

    if (CALLED_IN_SCHED_LOCK_REGION)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if (CALLED_FROM_AN_ISR)
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

    if ( (eGetOption != ADI_OSAL_EVENT_FLAG_ANY) &&
         (eGetOption != ADI_OSAL_EVENT_FLAG_ALL) )
    {
        return (ADI_OSAL_BAD_OPTION);
    }

    if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }

    if(eGetOption == ADI_OSAL_EVENT_FLAG_ANY)
    {
        nWaitOption = (BaseType_t)pdFALSE;
    }

    if(nRequestedEvents > EVENTFLAG_MAX_SIZE)
    {
        return (ADI_OSAL_BAD_EVENT);
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
    }

    nRetValue = xEventGroupWaitBits(hEventNative,(BaseType_t)nRequestedEvents,pdFALSE,nWaitOption,nTimeTicks);
    if(eGetOption == ADI_OSAL_EVENT_FLAG_ALL)
    {
        if((nRetValue & nRequestedEvents) == nRequestedEvents)
        {
            eRetStatus = ADI_OSAL_SUCCESS;
            *pnReceivedEvents = (uint32_t) nRetValue;
        }
        else
        {
            eRetStatus = ADI_OSAL_TIMEOUT;
            *pnReceivedEvents = (uint32_t) 0; /*Make the received events as 0 in case of timeout*/
        }
    }
    else
    {
        if((nRetValue & nRequestedEvents) != 0u)
        {
            eRetStatus = ADI_OSAL_SUCCESS;
            *pnReceivedEvents = (uint32_t) nRetValue;
        }
        else
        {
            eRetStatus = ADI_OSAL_TIMEOUT;
            *pnReceivedEvents = (uint32_t) 0; /*Make the received events as 0 in case of timeout*/
        }
    }

    return( eRetStatus );
}

/*!
  ****************************************************************************
    @brief Sets one or more event flags.

    @param[in] hEventGroup      - handle of the event group to use
    @param[in] nEventFlags      - Specifies the event flags to set.
                                  'ORed' into the current event flags.

    @return ADI_OSAL_SUCCESS    - If the event flag(s) are posted successfully.
    @return ADI_OSAL_BAD_HANDLE - If the specified event group is invalid
    @return ADI_OSAL_BAD_EVENT  - If the event group size is invalid
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_EventSet( ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                     ADI_OSAL_EVENT_FLAGS nEventFlags)
{
    EventBits_t nRetValue;
    BaseType_t nHigherPriorityTaskWoken = pdFALSE;
    EventGroupHandle_t hEventNative = (EventGroupHandle_t) hEventGroup;

#ifdef OSAL_DEBUG
  if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
   {
       return(ADI_OSAL_BAD_HANDLE);
   }
#endif /* OSAL_DEBUG */

    if(nEventFlags > EVENTFLAG_MAX_SIZE)
    {
        return (ADI_OSAL_BAD_EVENT);
    }

    if(CALLED_FROM_AN_ISR)
    {
        nRetValue = xEventGroupSetBitsFromISR(hEventNative,(EventBits_t)nEventFlags,\
                              &nHigherPriorityTaskWoken);

        if(nRetValue != pdFAIL)
        {
            /* If nHigherPriorityTaskWoken was set to true you we should yield */
            portYIELD_FROM_ISR( nHigherPriorityTaskWoken );
        }
    }
    else
    {
        xEventGroupSetBits(hEventNative,(EventBits_t)nEventFlags);
    }

    return ADI_OSAL_SUCCESS;
}

/*!
  ****************************************************************************
    @brief Clears one or more event flags.

    @param[in] hEventGroup      - Handle of the event group to use
    @param[in] nEventFlags      - Specifies the event flags to cleared.

    @return ADI_OSAL_SUCCESS    - If the event flag(s) are cleared successfully.
    @return ADI_OSAL_BAD_HANDLE - If the specified event group is invalid
    @return ADI_OSAL_BAD_EVENT  - If the event group size is invalid
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_EventClear(ADI_OSAL_EVENT_HANDLE const hEventGroup, ADI_OSAL_EVENT_FLAGS nEventFlags)
{

    EventGroupHandle_t hEventNative = (EventGroupHandle_t) hEventGroup;

 #ifdef OSAL_DEBUG
    if ((NULL == hEventGroup) || (ADI_OSAL_INVALID_EVENT_GROUP == hEventGroup))
    {
        return(ADI_OSAL_BAD_HANDLE);
    }
#endif

    if(nEventFlags > EVENTFLAG_MAX_SIZE)
    {
        return (ADI_OSAL_BAD_EVENT);
    }
    if(CALLED_FROM_AN_ISR)
    {
        xEventGroupClearBitsFromISR(hEventNative,(EventBits_t)nEventFlags);
    }
    else
    {
        xEventGroupClearBits(hEventNative,(EventBits_t)nEventFlags);
    }

    return ADI_OSAL_SUCCESS;
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm114,Pm128,Pm140,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
