/*****************************************************************************
    Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
*****************************************************************************/


/*****************************************************************************
Description:
              Operating System Abstraction Layer for the FreeRTOS on
              there is no OS(Operating System) on ARM Cortex-A (ADSP-SC5xx).

*****************************************************************************/

/*=============  I N C L U D E S   =============*/


#include <stdlib.h>           /* for NULL definition */
#include <limits.h>           /* for INT_MAX */
#include <FreeRTOS.h>

#include <services/int/adi_gic.h>
#include "adi_osal.h"
#include "adi_osal_arch.h"
#include "adi_osal_arch_internal.h"
#include "osal_common.h"

/*=============  D E F I N E S  =============*/

#define ADI_OSAL_INVALID_HEAP_INDEX (-1)
static int32_t osal_snHeapIndex = ADI_OSAL_INVALID_HEAP_INDEX;

/*!
    @internal
    @var osal_snHeapIndex
         Heap index that is used for memory operations within the OSAL.
         This variable is only valid after the call to adi_osal_Init
    @endinternal
*/

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
    ADI_OSAL_STATUS eRetStatus = ADI_OSAL_FAILED;

    /* If the application is not defining where to place the heap, OSAL will
       use the default one. This is the only thing supported in the Cortex-A5
    */
    if ( NULL == pHeapMemory )
    {
        osal_snHeapIndex = 0;
        eRetStatus = ADI_OSAL_SUCCESS;
    }
    return (eRetStatus);
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

    pMemory = malloc(nSize);

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
    free(pData);
}


/*!
  ****************************************************************************
    @brief  Installs a high-level interrupt handler.

    @param[in] iid - ID of the interrupt to be handled
    @param[in] highLevelHandler - Function address of the handler
    @param[in] handlerArg - Generic argument to be passed to the handler

    @return ADI_OSAL_SUCCESS      - If handler is successfully installed
    @return ADI_OSAL_FAILED       - If failed to install handler
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_InstallHandler (
   uint32_t iid,
   ADI_OSAL_HANDLER_PTR highLevelHandler,
   void* handlerArg
)
{
    int32_t index;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }
#endif /* OSAL_DEBUG */

    index = adi_rtl_register_dispatched_handler (iid,
                                                 (adi_dispatched_handler_t) highLevelHandler,
                                                 handlerArg);

    if (index < 0)
    {
        /* ADI_RTL 	error */
        return ADI_OSAL_FAILED;
    }

    /* If an interrupt handler is being installed via OSAL then it is safe to assume that the handler
     * expects to interact with the RTOS, i.e. to call RTOS API functions. FreeRTOS only allows interrupts
     * below a certain priority level to call its APIs, and the default priority for GIC interrupts is above
     * level, so it is necessary to change the interrupt's priority here so that FreeRTOS API function called
     * from the handler will not fail.
     *
     * Note that if an application (or a device driver) needs to set a specific priority for an interrupt it
     * will need to do this *after* registering the handler, otherwise its setting will be overwritten here. There
     * is currently no adi_gic_SetIntPriority() API so we can't query the current priority of the interrupt to see
     * if it's already within the required range.
     */
    if (ADI_GIC_SUCCESS != adi_gic_SetIntPriority(iid, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT))
    {
        /* ADI_GIC error */
    	adi_rtl_unregister_dispatched_handler (iid);
        return ADI_OSAL_FAILED;
    }

    return ADI_OSAL_SUCCESS;

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
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief  Deactivates a high-level interrupt handler
    .

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully deactivated
    @return ADI_OSAL_FAILED       - If failed to deactivate handler

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_DeactivateHandler (uint32_t iid)
{
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

    return ADI_OSAL_SUCCESS;
}


/*
**
** EOF:
**
*/
