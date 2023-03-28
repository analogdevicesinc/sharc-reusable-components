/*****************************************************************************
    Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
*****************************************************************************/

#ifndef __ADI_OSAL_ARCH_INTERNAL__
#define __ADI_OSAL_ARCH_INTERNAL__

#if !defined (__ADSPCORTEXA5__)
    #error "Unknown processor family"
#endif

#include <sys/platform.h>
#include <runtime/int/interrupt.h>

/* external data (shared across OSAL environments) */

/* external code (shared across OSAL environments) */
extern ADI_OSAL_STATUS _adi_osal_HeapInstall(uint32_t *pHeapMemory, uint32_t nHeapMemorySize);
extern ADI_OSAL_STATUS _adi_osal_MemAlloc(void** ppData, uint32_t nSize);
extern void            _adi_osal_MemFree(void* pData);
static inline bool     _adi_osal_IsCurrentLevelISR( void );
static inline bool     _adi_osal_IsMemoryAligned(const void *pMemory);

#if 0 /* Not currently used in the FreeRTOS version of OSAL */
static inline uint32_t _adi_osal_InterruptsDisable(void)
{
    uint32_t local_cpsr;
    uint32_t state;

    asm ("MRS %0, CPSR" :"=r"(local_cpsr): :);

    /* Get the state of the interrupts */
    state = (local_cpsr & (ADI_RTL_ARM_CONTROL_FIQ_DIS | ADI_RTL_ARM_CONTROL_IRQ_DIS));

    /* If not already disabled disable them */
    if(state != 0u)
    {
        __builtin_disable_interrupts();
    }

     return state;
}


static inline void _adi_osal_InterruptsEnable(uint32_t previousState)
{
    if(previousState > 0u)
    {
        __builtin_enable_interrupts();
    }
}
#endif
/*!
  ****************************************************************************

   @internal

   @brief Describes whether the API is called at interrupt level or not

   @return true  - if the current execution is at interrupt level
   @return false - if the current execution is not at interrupt level

   @endinternal
*****************************************************************************/
static inline bool _adi_osal_IsCurrentLevelISR( void )
{
  	uint32_t local_cpsr;
  	uint32_t mode;

	asm ("MRS %0, CPSR" :"=r"(local_cpsr): :);

	mode = (local_cpsr & ADI_RTL_ARM_MODE_MASK);

	/* Under FreeRTOS all GIC interrupts are dispatched in SVC mode, but
	 * tasks run in SYSTEM mode (or possibly USER mode if protection was enabled).
	 * Therefore, any mode other than USR or SVC indicated that we're in an interrupt.
	 *
	 */
    if ((mode == ADI_RTL_ARM_MODE_USR) || (mode == ADI_RTL_ARM_MODE_SYS))
        return false;
    else
        return true;

}

/*!
  ****************************************************************************
   @internal

   @brief This function indicates whether a pointer is aligned and can be used
          to store variables in the particular architecture

   @param[in] pMemory - Pointer to the allocated memory

   @return true    - if the memory was aligned
   @return false   - if the memory was not aligned

   @endinternal

*****************************************************************************/

static inline bool _adi_osal_IsMemoryAligned(const void *pMemory)
{
    return( 0u == ((uint32_t)(pMemory) & 0x3u));
}

#endif /* __ADI_OSAL_ARCH_INTERNAL__ */
