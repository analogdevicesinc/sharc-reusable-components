/*
 *  Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
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
    @file adi_osal_freertos_tls.c

    Operating System Abstraction Layer - OSAL for FreeRTOS - TLS
    functions

*/
/** @addtogroup ADI_OSAL_TLS ADI OSAL TLS
 *  @{
 *
 *  This module contains the Thread Local Storage APIs for the FreeRTOS
 *  implementation of OSAL. Since FreeRTOS does not support TLS feature
 *  the OSAL TLS APIs in this module return ADI_OSAL_OS_ERROR
 */
/*=============  I N C L U D E S   =============*/

#include <stdlib.h>
#include "adi_osal.h"
#include "osal_common.h"
#include "osal_freertos.h"

/* disable misra diagnostics as necessary
 *
 * Error[Pm008]: sections of code should not be 'commented out'
 *               (MISRA C 2004 rule 2.4)
 *
 * Error[Pm128]: illegal implicit conversion from underlying MISRA type
 *                (MISRA C 2004 rule 10.1)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm008,Pm128")
#endif
/*! @endcond */



/* Thread local storage numbers/keys consist of two values:
 *  - The signature, indicating that the key refers to a TLS slot 0xNNNNNNXX
 *  - The index of the slot, from 0 to Max Slots 0xXXXXXXNN
 */
#define TLS_SIGNATURE ((uint32_t)(0x544C5300u))

/* Masks used to extract either the signature component, or the slot index component
 * of a TLS Key.
 */
#define TLS_MASK_SIG ((uint32_t)(0xFFFFFF00u))
#define TLS_MASK_NUM ((uint32_t)(0x000000FFu))

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

/*=============  D A T A  =============*/

/*!
    @internal
    @var _adi_osal_gnNumSlots
         stores the number of TLS slots requested by the user during
         initialization.
    @endinternal
*/
uint32_t _adi_osal_gnNumSlots = ADI_OSAL_MAX_NUM_TLS_SLOTS;

/*!
    @internal
    @var _adi_osal_gTLSUsedSlots
         Word used in the management of allocated TLS slots.
         Bits are used to represent the status of the slot. Bit 0 corresponds
         to slot number 0 and slot number 30 corresponds bit number 30.
         A slot is free if the corresponding bit is clear and a
         slot is acquired if the corresponding bit is set. Initially all
         the slot bits are clear.
    @endinternal
*/
static uint32_t _adi_osal_gTLSUsedSlots = 0u;

/*!
    @internal
    @var  _adi_osal_gaTLSCallbacks
          Hold the callback structure for each Thread Local Storage Slot. The
          Callback are per slot, all threads that are using that slot are using
          the same callback.  Make the array the maximum supported size
          (ADI_OSAL_MAX_THREAD_SLOTS)
    @endinternal
*/
/*
static ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbacks[ADI_OSAL_MAX_NUM_TLS_SLOTS];
*/

bool IsValidTLSKey(ADI_OSAL_TLS_SLOT_KEY key);

/*=============  Prototypes ===========*/

void adi_osal_RegisterLocalStorageCallback(void*);

/*=============  C O D E  =============*/


#if defined (__ECC__)
#pragma always_inline
#elif defined (__ICCARM__)
#pragma inline=forced
#endif
inline bool IsValidTLSKey(ADI_OSAL_TLS_SLOT_KEY key)
{
    return (TLS_SIGNATURE == (key & TLS_MASK_SIG));
}

/*!
  ****************************************************************************
    @brief Allocates a thread slot and returns the slot number

    Dummy function retuns ADI_OSAL_OS_ERROR.

    @param[out] pnThreadSlotKey       - Pointer to return the slot number if a
                                        free slot is found.  Must be populated
                                        with ADI_OSAL_TLS_UNALLOCATED.  If a
                                        valid slot is already present in the
                                        supplied address this API returns
                                        success.

    @param[in] pTerminateCallbackFunc - Pointer to a function that gets called
                                        when the slot is freed.  Can be NULL
                                        if the callback function is not
                                        required.

    @return ADI_OSAL_OS_ERROR        - FreeRTOS does not support TLS feature.

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ThreadSlotAcquire(ADI_OSAL_TLS_SLOT_KEY     *pnThreadSlotKey,
                           ADI_OSAL_TLS_CALLBACK_PTR pTerminateCallbackFunc)
{
#ifdef INCLUDE_vTaskDelete
	/* Register callback if provided */
	adi_osal_RegisterLocalStorageCallback (pTerminateCallbackFunc);
#endif

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }
#endif /* OSAL_DEBUG */

    ADI_OSAL_STATUS  eRetStatus = ADI_OSAL_FAILED;

    /* Lock the scheduler - as a task may be deleted while a callback is being
     * installed.  */
    vTaskSuspendAll();

    /* If the passed-in slot number has already been allocated, then we return
     * successfully.  We check that -
     *  - It has the correct TLS signature
     *  - The slot has been (and is still) allocated.
     */
    if (IsValidTLSKey(*pnThreadSlotKey))
    {
        /* Extract the slot number from the TLS key, and convert to a bit
         * position */
        uint32_t nSlotBit = 1ul << (*pnThreadSlotKey & TLS_MASK_NUM);

        if (_adi_osal_gTLSUsedSlots & nSlotBit)
        {
			eRetStatus = ADI_OSAL_SUCCESS;
        }
        else
        {
        	eRetStatus = ADI_OSAL_SLOT_NOT_ALLOCATED;
        }
    }
    else
    {
        /* Before we allocate a slot, the address to be written to must have
         * an "unallocated" key.
         */
		if (*pnThreadSlotKey != ADI_OSAL_TLS_UNALLOCATED)
		{
			*pnThreadSlotKey = ADI_OSAL_INVALID_THREAD_SLOT;
		}
		else
		{
		    BaseType_t nSlotIndex;

		    for (nSlotIndex = 0; nSlotIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS; ++nSlotIndex)
		    {
		    	const uint32_t slotBit = (1u << nSlotIndex);

		    	if (0u == (_adi_osal_gTLSUsedSlots & slotBit))
		    	{
		    		/* We've found an unused slot */
					_adi_osal_gTLSUsedSlots |= slotBit; /* mark the slot as in use */
					*pnThreadSlotKey = (nSlotIndex | TLS_SIGNATURE);
					eRetStatus = ADI_OSAL_SUCCESS;
					break;
		    	}
		    }
		}
    }

    xTaskResumeAll();

    return eRetStatus;
}



/*!
  ****************************************************************************
    @brief Frees the specified slot in the local storage buffer.

    @param[in] nThreadSlotKey     - slot which needs to be freed.

    @return ADI_OSAL_OS_ERROR        - FreeRTOS does not support TLS feature.

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ThreadSlotRelease(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey)
{
	const uint32_t slotIndex = nThreadSlotKey & TLS_MASK_NUM;

#ifdef OSAL_DEBUG
	/* Range-check the key */
	if (slotIndex >= configNUM_THREAD_LOCAL_STORAGE_POINTERS)
	{

	    return ADI_OSAL_BAD_SLOT_KEY;
	}
#endif /* OSAL_DEBUG */

	const uint32_t slotBit = (1u << slotIndex);

	/* Mark the slot as free */
	_adi_osal_gTLSUsedSlots &= ~slotBit;

    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief Stores the given value in the specified TLS slot.

    @param[out] nThreadSlotKey     - Slot key for the Thread Local Buffer in
                                     which "SlotValue" to be stored.
    @param[in] slotValue           - Value to be stored.

    @return ADI_OSAL_OS_ERROR      - FreeRTOS does not support TLS feature.
*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ThreadSlotSetValue(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey,
                            ADI_OSAL_SLOT_VALUE   slotValue)
{
	const uint32_t slotIndex = nThreadSlotKey & TLS_MASK_NUM;

#ifdef OSAL_DEBUG
	/* Range-check the key */
	if (slotIndex >= configNUM_THREAD_LOCAL_STORAGE_POINTERS)
	{

	    return ADI_OSAL_BAD_SLOT_KEY;
	}
#endif /* OSAL_DEBUG */

	vTaskSetThreadLocalStoragePointer(NULL, slotIndex, slotValue);

    return ADI_OSAL_SUCCESS;
}



/*!
  ****************************************************************************

    @brief Gets a value for the specified TLS slot from the current thread.

    @param[in] nThreadSlotKey     - Slot key, from which the data needs
                                    to be retrieved.
    @param[out] pSlotValue        - Pointer to store the retrieved value from
                                    TLS.

    @return ADI_OSAL_OS_ERROR        - FreeRTOS does not support TLS feature.
*****************************************************************************/

ADI_OSAL_STATUS
adi_osal_ThreadSlotGetValue(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey,
                            ADI_OSAL_SLOT_VALUE   *pSlotValue)
{
	const uint32_t slotIndex = nThreadSlotKey & TLS_MASK_NUM;

#ifdef OSAL_DEBUG
	/* Range-check the key */
	if (slotIndex >= configNUM_THREAD_LOCAL_STORAGE_POINTERS)
	{

	    return ADI_OSAL_BAD_SLOT_KEY;
	}
#endif /* OSAL_DEBUG */

	*pSlotValue = pvTaskGetThreadLocalStoragePointer(NULL, slotIndex);

    return ADI_OSAL_SUCCESS;
}

#endif /* configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 */


/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm008,Pm128")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
