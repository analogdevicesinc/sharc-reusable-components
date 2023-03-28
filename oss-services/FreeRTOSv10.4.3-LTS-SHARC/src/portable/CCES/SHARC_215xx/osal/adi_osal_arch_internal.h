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

#ifndef __ADI_OSAL_ARCH_INTERNAL__
#define __ADI_OSAL_ARCH_INTERNAL__

#if !defined (__ADSP21000__)
    #error "Unknown processor family"
#endif

#include <platform_include.h>
#include <sysreg.h>
#include <interrupt.h>

#if defined (__ADI_GENERATED_DEF_HEADERS__)
#define ADI_OSAL_IRPTEN (BITM_REGF_MODE1_IRPTEN)
#else
#define ADI_OSAL_IRPTEN ((uint32_t)IRPTEN)
#endif

#pragma diag(push)
#pragma diag(suppress:misra_rule_8_12:"OSAL declares external global arrays without indicating their size")

/* external data (shared across OSAL environments) */

/*!
    @internal
    @var _adi_osal_gHandlerTable
	The OSAL dispatch table. It is an array of function pointers, of the
	type corresponding to OSAL's plain C interrupt handlers (i.e. the high-level
	handlers). The table needs to be large enough for any index that we can get
	back from adi_rtl_register_dispatched_handler().
    @endinternal
 */
extern ADI_OSAL_HANDLER_PTR _adi_osal_gHandlerTable[];
#pragma diag(pop)
/*!
    @internal
    @var _adi_osal_gHandlerTableSize
	The size of the OSAL dispatch table. The size needs to be large enough for
    any index that we can get back from adi_rtl_register_dispatched_handler().
    @endinternal
 */
extern uint32_t _adi_osal_gHandlerTableSize;

/* external code (shared across OSAL environments) */
extern ADI_OSAL_STATUS _adi_osal_HeapInstall(uint32_t *pHeapMemory, uint32_t nHeapMemorySize);
extern ADI_OSAL_STATUS _adi_osal_MemAlloc(void** ppData, uint32_t nSize);
extern void            _adi_osal_MemFree(void* pData);
extern bool            _adi_osal_IsCurrentLevelISR( void );
extern bool            _adi_osal_IsMemoryAligned(const void *pMemory);

EX_DISPATCHED_HANDLER_NESTED( _adi_osal_stdWrapper, iid, index, arg);

#pragma inline
static uint32_t _adi_osal_InterruptsDisable(void)
{
    uint32_t state = (uint32_t)__builtin_sysreg_bit_tst(sysreg_MODE1, ADI_OSAL_IRPTEN );
    
    asm volatile("#include <platform_include.h>");
#if defined (__ADI_GENERATED_DEF_HEADERS__)
    asm volatile("#define SH_INT_DISABLE JUMP (PC, .SH_INT_DISABLED?) (DB);  \
                                   BIT CLR MODE1 BITM_REGF_MODE1_IRPTEN;                     \
                                   NOP;                                      \
                                   .SH_INT_DISABLED?: "                      \
                                   "\nSH_INT_DISABLE");
#else
    asm volatile("#define SH_INT_DISABLE JUMP (PC, .SH_INT_DISABLED?) (DB);  \
                                   BIT CLR MODE1 IRPTEN;                     \
                                   NOP;                                      \
                                   .SH_INT_DISABLED?: "                      \
                                   "\nSH_INT_DISABLE");
#endif
    return state;
}

#pragma inline
static void _adi_osal_InterruptsEnable(uint32_t previousState)
{
    if(previousState > 0u)
    {
        sysreg_bit_set(sysreg_MODE1, ADI_OSAL_IRPTEN);
    }
}

/*!
  ****************************************************************************

   @internal

   @brief Describes whether the API is called at interrupt level or not

   @return true  - if the current execution is at interrupt level
   @return false - if the current execution is not at interrupt level

   @endinternal
*****************************************************************************/
#pragma inline
bool _adi_osal_IsCurrentLevelISR( void )
{
#if defined(__2116x__)
    return (sysreg_read(sysreg_IMASKP) & (uint32_t)(~LPISUMI)) > 0u;
#else
    return (sysreg_read(sysreg_IMASKP) > 0u);
#endif
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

#pragma diag(suppress:misra_rule_11_3 : "We need to convert the pointer to see if it is aligned")

#pragma inline
bool _adi_osal_IsMemoryAligned(const void *pMemory)
{
    return(true);
}




#endif /* __ADI_OSAL_ARCH_INTERNAL__ */
