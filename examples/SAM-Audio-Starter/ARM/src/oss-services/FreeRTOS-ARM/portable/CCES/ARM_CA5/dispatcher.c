/*****************************************************************************
    Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
*****************************************************************************/

/*
 * dispatcher.c
 *
 * Contains support function(s) to connect the FreeRTOS GIC interrupt dispatcher (in portASM.S) to the CCES
 * C runtime dispatched interrupt vector table.
 *
 *  Created on: Feb 14, 2017
 */

#include <runtime/int/interrupt.h>
#include <sys/platform.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"

/* Dispatched interrupt vector table, defined by CCES runtime */
extern adi_dispatched_data_t adi_dispatched_int_vector_table[ADI_DISPATCHED_VECTOR_TABLE_SIZE];

/* vApplicationIRQHandler() is just a normal C function.
 * - the argument ulICCIAR is the value which was read from the interrupt acknowledge register
 *
 * Assume that if the user has configured any type of FPU support then they'd also like
 * to have FPU context saved during ISR handling too.  This assumption can be overridden
 * by defining configNO_ISR_FPU_SUPPORT.
 */
#if defined(configUSE_TASK_FPU_SUPPORT) && (configUSE_TASK_FPU_SUPPORT > 0) && \
    !defined(configNO_ISR_FPU_SUPPORT)
void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR )
#else
void vApplicationIRQHandler( uint32_t ulICCIAR )
#endif
{
    /* Re-enable interrupts, which were disabled on entry to the dispatcher (FreeRTOS_IRQ_Handler) */
    __asm volatile( "CPSIE I" );

    /* Convert the hardware-supplied interrupt number to an IID by masking out everything except the interrupt ID.
     * Since the only other field in the INT_ACK register is the CPU ID, this step is optional on platforms where
     * the CPU ID will always be zero.
     *
     * (runtime/int/interrupt.h should really have an ADI_RTL_INTR_IID() macro to do this, but it doesn't as yet.)
     */
    const uint32_t iid = ulICCIAR & ADI_RTL_INTID_MASK;

	/* Check for the "spurious interrupt" IDs (0x3FF and 0x3FE). These are returned by the GIC if there is nothing
	 * to be done, e.g. if the interrupt has already been acknowledged by another core (0x3FE used in TrustZone).
	 * We can check for both spurious interrupts *and* for overrun of the interrupt table, simple by checking
	 * against the interrupt table size since this will always be <= 0x3FE.
	 */
	if (iid < ADI_DISPATCHED_VECTOR_TABLE_SIZE)
	{
		/* Convert the interrupt ID into a index into the ADI_RTL interrupt vector table */
		const uint32_t idx = ADI_RTL_IID_TO_INDEX(iid);

		/* Call the interrupt handler, as a plain C function, passing the interrupt ID and the
		 * user-provided parameter as arguments.
		 */
		(*adi_dispatched_int_vector_table[idx].handler)(iid, (adi_dispatched_callback_arg_t)(adi_dispatched_int_vector_table[idx].callback_arg));
	}
}
