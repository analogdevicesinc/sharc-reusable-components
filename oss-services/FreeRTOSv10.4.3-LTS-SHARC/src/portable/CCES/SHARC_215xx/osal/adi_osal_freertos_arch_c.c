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



/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")


/*=============  I N C L U D E S   =============*/


#include <adi_osal.h>
#include <sys/platform.h>
#include "osal_common.h"
#include "adi_osal_arch_internal.h"

/*=============  D E F I N E S  =============*/


#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")


/*!
  ****************************************************************************
    @brief  Installs a high-level interrupt handler
    .

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

    /* In the SHARC-XI port of FreeRTOS, no special wrapping of interrupts is required as
     * there are no RTOS-specific ISR prologs or epilogs.
     */
    index = adi_rtl_register_dispatched_handler (iid,
    		                                     _adi_osal_stdWrapper,
                                                 handlerArg);

    if (index < 0)
    {
        /* error */
        return ADI_OSAL_FAILED;
    }

    if (index >= (int)_adi_osal_gHandlerTableSize)
    {
        /* Register succeeded but OSAL's dispatch table is
         * too small for the returned index.
         */
        adi_rtl_unregister_dispatched_handler (iid);
        return ADI_OSAL_FAILED;
    }

    _adi_osal_gHandlerTable[index] = highLevelHandler;
    return ADI_OSAL_SUCCESS;
}

/*
 * This is the environment-specific interrupt wrapper,
 * i.e. the wrapper that is used for interrupts that
 * may call operating system APIs, and hence must support
 * rescheduling. In the FreeRTOS OSAL they have essentially the
 * same implementation as the plain wrapper, no special wrapping
 * of interrupts is required as there are no RTOS-specific ISR
 * prologs or epilogs. We let the compiler generate the appropriate
 * code for saving and restoring registers, and for setting up the C runtime.
 */
EX_DISPATCHED_HANDLER_NESTED(_adi_osal_stdWrapper, iid,  index, arg)
{
	(_adi_osal_gHandlerTable[index])(iid, (void*) arg);
}
