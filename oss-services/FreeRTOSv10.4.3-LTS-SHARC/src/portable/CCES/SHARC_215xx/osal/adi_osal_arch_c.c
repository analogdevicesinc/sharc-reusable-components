/*
 * Copyright (C) 2018 Analog Devices Inc. All Rights Reserved.
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


/*=============  I N C L U D E S   =============*/


/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-16.2 indicates that  Functions shall not call themselves,either directly or indirectly */
#pragma diag(suppress:misra_rule_16_2 : "Since the OSAL is reentrant by nature (several thread could call the API) the compiler MISRA checker mistakes sometimes the reentrancy for recurrence")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
/* Rule-20.4 Dynamic heap memory allocation shall not be used. */
#pragma diag(suppress:misra_rule_20_4:"This source needs to use malloc")
/* Rule 17.4(Req): Array indexing shall be the only allowed form of pointer arithmetic. */
#pragma diag(suppress:misra_rule_17_4:"This source needs to use pointer indexing")

#include <stdlib.h>           /* for NULL definition */
#include <limits.h>           /* for INT_MAX */

#include "adi_osal.h"
#include "adi_osal_arch.h"
#include "adi_osal_arch_internal.h"
#include "osal_common.h"

/*=============  D E F I N E S  =============*/


#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")

#define ADI_OSAL_INVALID_HEAP_INDEX (-1)



/*!
    @internal
    @var osal_snHeapIndex
         Heap index that is used for memory operations within the OSAL.
         This variable is only valid after the call to adi_osal_Init
    @endinternal
*/
static int32_t osal_snHeapIndex = ADI_OSAL_INVALID_HEAP_INDEX;

/*=============  C O D E  =============*/



/*!
  ****************************************************************************
   @internal

   @brief This function abstracts the creation of a heap with the information
    provided.

   @details Creates a heap with the information provided. The user ID might be
            in use by application code. For this reason, if heap_install fails
            OSAL will change the user ID 10 times before giving up and
            returning an error

   Parameters:
   @param[in] pHeapMemory - Pointer to the allocated memory
   @param[in] nHeapMemorySize  - Size of memory to be allocated

   @return ADI_OSAL_SUCCESS    - if the heap was created successfully
   @return ADI_OSAL_MEM_ALLOC_FAILED  - if the heap could not be created

   @endinternal

*****************************************************************************/

ADI_OSAL_STATUS _adi_osal_HeapInstall(uint32_t *pHeapMemory, uint32_t nHeapMemorySize)
{
    int32_t nHeapUserId = INT_MAX;

    /* If the application is not defining where to place the heap, OSAL will
       use the default one
    */
    if ( NULL == pHeapMemory )
    {
        osal_snHeapIndex = 0;
        return (ADI_OSAL_SUCCESS);
    }

    /* The least likely heap ID to be used by an application is INT_MAX because
       they normally start 0,1,2 ... This code tries to look for an available
       heap ID of INT_MAX and if that one is unavailable it tries, INT_MAX-1,
       INT_MAX-2,... until INT_MAX -9. If all of those are unavailable, it
       returns an error
     */

    do
    {
        int32_t pmdm = -1;             /* We look for DM memory */
        osal_snHeapIndex = heap_install(pHeapMemory, nHeapMemorySize, nHeapUserId);
        nHeapUserId --;
    } while ( (ADI_OSAL_INVALID_HEAP_INDEX == osal_snHeapIndex)  && ( (INT_MAX -10 )< nHeapUserId ));

    if (ADI_OSAL_INVALID_HEAP_INDEX == osal_snHeapIndex ) {
            return (ADI_OSAL_MEM_ALLOC_FAILED);
    } else  {
            return (ADI_OSAL_SUCCESS);
    }
}

/*!
  ****************************************************************************
   @internal

   @brief Abstracts which heap to use for dynamic memory allocations

   @param[out] ppData - Pointer that will store the allocated memory pointer
   @param[in] nSize  - Size of memory to be allocated

   @return ADI_OSAL_SUCCESS - if the memory was allocated correctly
   @return ADI_OSAL_MEM_ALLOC_FAILED  - if the memory could not be allocated

   @endinternal

*****************************************************************************/

ADI_OSAL_STATUS _adi_osal_MemAlloc(void** ppData, uint32_t nSize)
{
    void* pMemory;

    /* pass it directly to heap_malloc if a heap was created or malloc
     * otherwise */
    if (osal_snHeapIndex == ADI_OSAL_INVALID_HEAP_INDEX)
    {
        return (ADI_OSAL_FAILED); /* the heap number was not set yet */
    }

    pMemory = heap_malloc(osal_snHeapIndex, nSize);

    if (pMemory != NULL)
    {
        *ppData = pMemory;
        return (ADI_OSAL_SUCCESS);
    }
    else
    {
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }
}



/*!
  ****************************************************************************
   @internal

   @brief Abstracts which heap to use to free dynamic memory

   @param[in] pData - Memory area to be freed (same argument as free)

   @endinternal

*****************************************************************************/

void  _adi_osal_MemFree(void* pData)
{
    heap_free(osal_snHeapIndex, pData);
}


/*!
  ****************************************************************************
    @brief  Activates a high-level interrupt handler
    .

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully activated
    @return ADI_OSAL_FAILED       - If failed to activate handler

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ActivateHandler (uint32_t iid)
{
    if (0 != adi_rtl_activate_dispatched_handler(iid))
    {
        /* Error - the IID doesn't correspond to a registered handler */
        return ADI_OSAL_FAILED;
    }
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief  Deactivates a high-level interrupt handler

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully deactivated
    @return ADI_OSAL_FAILED       - If failed to deactivate handler

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_DeactivateHandler (uint32_t iid)
{
    if (0 != adi_rtl_deactivate_dispatched_handler(iid))
    {
        /* Error - the IID doesn't correspond to a registered handler */
        return ADI_OSAL_FAILED;
    }
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief  Uninstalls a high-level interrupt handler
    .

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully uninstalled
    @return ADI_OSAL_FAILED       - If failed to uninstall handler
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_UninstallHandler (uint32_t iid)
{
    int32_t index;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }
#endif /* OSAL_DEBUG */

    index = adi_rtl_unregister_dispatched_handler (iid);

    if (index < 0)
    {
        /* Error - the IID doesn't correspond to a registered handler */
        return ADI_OSAL_FAILED;
    }

#ifdef OSAL_DEBUG
    if (index >= (int)_adi_osal_gHandlerTableSize)
    {
        /* OSAL's dispatch table is too small for the returned index.
         * Something would have to go badly wrong for this to happen
         * (e.g. memory corruption) as the index should always be the
         * same as when the handler was registered.
         */
        return ADI_OSAL_FAILED;
    }
#endif /* OSAL_DEBUG */

    _adi_osal_gHandlerTable[index] = NULL;
    return ADI_OSAL_SUCCESS;
}


/*
**
** EOF:
**
*/
