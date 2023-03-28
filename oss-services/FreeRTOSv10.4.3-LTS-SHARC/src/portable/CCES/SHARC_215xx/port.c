/*
 * FreeRTOS Kernel V10.4.3 LTS Patch 2
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the SHARC+ port.
 *----------------------------------------------------------*/

#include <string.h> /* for memset() */

#include <sys/platform.h>
#include <platform_include.h>
#include <sys/anomaly_macros_rtl.h>
#include <interrupt.h>
#include <builtins.h>
#include <services/int/adi_sec.h> /* only needed for WA_20000081 */

#include "portASM.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

#define TASK_MODE1_CLR_BITS ((uint32_t)( \
	BITM_REGF_MODE1_BR0                | \
    BITM_REGF_MODE1_BR8                | \
    BITM_REGF_MODE1_SRCU               | \
	BITM_REGF_MODE1_SRD1L              | \
	BITM_REGF_MODE1_SRD2H              | \
	BITM_REGF_MODE1_SRD2L              | \
	BITM_REGF_MODE1_SRRFH              | \
	BITM_REGF_MODE1_SRRFL              | \
	BITM_REGF_MODE1_ALUSAT             | \
	BITM_REGF_MODE1_SSE                | \
	BITM_REGF_MODE1_TRUNCATE           | \
	BITM_REGF_MODE1_PEYEN              | \
	BITM_REGF_MODE1_BDCST9             | \
	BITM_REGF_MODE1_BDCST1             ) \
)

 /* tasks run in the secondary DAG and data regs */
#define TASK_MODE1_SET_BITS ((uint32_t)( \
	BITM_REGF_MODE1_CBUFEN             | \
	BITM_REGF_MODE1_SRRFL              | \
	BITM_REGF_MODE1_SRRFH              | \
	BITM_REGF_MODE1_SRD2L              | \
	BITM_REGF_MODE1_SRD2H              | \
	BITM_REGF_MODE1_SRD1L              | \
	BITM_REGF_MODE1_SRD1H              | \
	BITM_REGF_MODE1_IRPTEN             | \
	BITM_REGF_MODE1_RND32              ) \
)

/* interrupts run in the primary DAG and data regs */
#define FRTOS_MMASK_SET_BITS ((uint32_t)( \
		BITM_REGF_MMASK_SRRFL           | \
		BITM_REGF_MMASK_SRRFH           | \
		BITM_REGF_MMASK_SRD2L           | \
		BITM_REGF_MMASK_SRD2H           | \
		BITM_REGF_MMASK_SRD1L           | \
		BITM_REGF_MMASK_SRD1H           ) \
)

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
#endif


/* Let the user override the pre-loading of the initial LR with the address of
prvTaskExitError() in case it messes up unwinding of the stack in the
debugger. */
#ifdef configTASK_RETURN_ADDRESS
	#define portTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define portTASK_RETURN_ADDRESS	prvTaskExitError
#endif

#ifndef configTIMER_INTERRUPT
	#define configTIMER_INTERRUPT   (ADI_CID_TMZLI)
#endif

#if WA_20000081
/* Global to hold the core-specific system interrupt ID for rescheduling */
uint32_t _adi_OSRescheduleIntID;
/* This volaile flag variable is used by tasks, to block their progress after raising the reschedule
 * interrupt until the interrupt has been serviced. This is to compensate for the much greater latency
 * of an SEC interrupt compared with a core interrupt.
 */
volatile uint32_t _adi_OSWaitingForSched;
#endif /* WA_20000081 */


/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void vPortSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
EX_DISPATCHED_HANDLER_NON_NESTED(xPortSysTickHandler, a, b, c);
#if WA_20000081
/* This is an entrypoint for the reschedule interrupt, which is used as a "dispatched handler" by the
 * adi_rtl low-level interrupt APIs.
 */
EX_DISPATCHED_HANDLER_NESTED(_adi_SoftIntTaskSw, a, b, c);
#endif /* WA_20000081 */

/*
 * Start first task is implemented in portASM.asm
 */
extern void prvPortStartFirstTask( void );

/*
 * This is an entrypoint for the reschedule interrupt
 */
extern void xPortSFT31Handler( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * The number of SysTick increments that make up one tick period.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context
	 * switch interrupt.
	 */
	pxTopOfStack -= 4; // space for arguments
    StackType_t *stkI6 = pxTopOfStack;
	pxTopOfStack -=2;
	
    /* Allocate a context record on the task's stack */
    ContextRecord *pRec = (ContextRecord*)(pxTopOfStack);
    pRec -= 1;

    StackRecord *pStk = (StackRecord*)pRec;
    pStk -= 1;

    /* Initialize the context record */
	memset(pRec, 0, sizeof(ContextRecord));
	memset(pStk, 0, sizeof(StackRecord));

    /* Create the stack registers first */
    pRec->B6 = (UINT32)pxTopOfStack;
    pRec->B7 = (UINT32)pxTopOfStack;

	/* Set up the fixed registers */
	pRec->M6  =  1;
	pRec->M7  = (UINT32)-1;
	pRec->M14 =  1;
	pRec->M15 = (UINT32)-1;

    /* Setup the parameter for the task's main function */
    pRec->R4    = (UINT32)pvParameters;

    /* .. and the Run function. */
    pStk->PC_STACK = (UINT32)pxCode;
    pStk->PC_STACK_COUNT = 1u;

    /* MODE1 is configured for a C runtime environment for new tasks.  */
    pRec->MODE1 = ((sysreg_read(sysreg_MODE1) & ~TASK_MODE1_CLR_BITS) |  TASK_MODE1_SET_BITS);

	/* Set the frame pointer to the top of stack */
	pRec->I6 = (UINT32)stkI6;

	/* Return the new stacktop */
	return (StackType_t*)pStk;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call vTaskDelete( NULL ).

	Artificially force an assert() to be triggered if configASSERT() is
	defined, then stop here so application writers can catch the error. */
	configASSERT( 0UL );
	portDISABLE_INTERRUPTS(); 
	for( ;; );
}
/*-----------------------------------------------------------*/



/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler( void )
{
	/* Set bit in the MMASK register so that the foreground (system) data and
	 * DAG registers will be selected on interrupt entry.
	 */
	sysreg_bit_set(sysreg_MMASK, FRTOS_MMASK_SET_BITS);

#if WA_20000081
    /* Anomaly 36-10-0101 : SEC interrupts fails to latch in IRPTL if incoming SECI interrupt aligns with explicit write to IRPTL register
     *
     * The workaround for this anomaly involves using a system interrupt rather than a core interrupt to
     * trigger rescheduling from task level, so there is extra initialization required to configure the
     * interrupt on the System Event Controller (SEC).
     */

  /* Use SEC interrupts SOFT6 for SHARC0 (core 1) and SOFT7 for SHARC1 (core 2) */
  if (ADI_CORE_SHARC1 == adi_core_id())
    {
      _adi_OSRescheduleIntID = INTR_SYS_SOFT7_INT;
    }
  else
    {
      _adi_OSRescheduleIntID = INTR_SYS_SOFT6_INT;
    }

    adi_rtl_register_dispatched_handler (_adi_OSRescheduleIntID, _adi_SoftIntTaskSw, 0u);
    adi_sec_SetCoreID(_adi_OSRescheduleIntID, (ADI_SEC_CORE_ID)adi_core_id()); /* route the interrupt to this core */
    adi_sec_EnableSource(_adi_OSRescheduleIntID, true);                        /* enable the interrupt source */
    adi_sec_EnableInterrupt(_adi_OSRescheduleIntID, true);                     /* enable the interrupt */
    adi_rtl_activate_dispatched_handler (_adi_OSRescheduleIntID);
#else /* !WA_20000081 */
    /* We do not need to use the iid or the argument for this interrupt. */
    adi_rtl_register_dispatched_handler (OS_RESCHEDULE_CID,				 				 /* User software interrupt 3 */
     									(adi_dispatched_handler_t) xPortSFT31Handler,	 /* OS IRQ handler  */
 										(adi_dispatched_callback_t) NULL);	             /* Without argument */
    adi_rtl_activate_dispatched_handler (OS_RESCHEDULE_CID);							 /* Enable the Interrupt */
#endif /* WA_20000081 */

	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	vPortSetupTimerInterrupt();

	/* Start the first task. */
	prvPortStartFirstTask();

	/* Should never get here as the tasks will now be executing!  Call the task
	exit error function to prevent compiler warnings about a static function
	not being called in the case that the application writer overrides this
	functionality by defining configTASK_RETURN_ADDRESS. */
	prvTaskExitError();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* Not implemented in ports where there is nothing to return to.
	Artificially force an assert. */
	configASSERT( 0UL );
}
/*-----------------------------------------------------------*/

static uint32_t s_SavedIntMask;

void vPortEnterCritical( void )
{
	adi_rtl_disable_interrupts();
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	adi_rtl_reenable_interrupts();
}
/*-----------------------------------------------------------*/

uint32_t ulPortSetInterruptMask( void )
{
       uint32_t state = sysreg_bit_tst(sysreg_MODE1, BITM_REGF_MODE1_IRPTEN );   \

       asm volatile("JUMP (PC, .SH_INT_DISABLED) (DB);  \n\
                                   BIT CLR MODE1 0x1000;     \n\
                                   NOP;                                      \n\
                                   .SH_INT_DISABLED: \n");

       return state;
 }
 /*-----------------------------------------------------------*/

void vPortClearInterruptMask( uint32_t ulNewMaskValue )
{
       if (0u != ulNewMaskValue)
       {
               sysreg_bit_set(sysreg_MODE1, BITM_REGF_MODE1_IRPTEN);
       }
}
/*-----------------------------------------------------------*/

EX_DISPATCHED_HANDLER_NON_NESTED(xPortSysTickHandler, a, b, c)
{
	/* Increment the RTOS tick. */
	if( xTaskIncrementTick() != pdFALSE )
	{
		/* A context switch is required.  */
		portYIELD();
	}
}
/*-----------------------------------------------------------*/

void vPortFixupStack(volatile StackType_t *stackTop, StackType_t *stackBase)
{
    StackRecord *pStk = (StackRecord*)(stackTop);
	pStk += 1;
    ContextRecord *pRec = (ContextRecord*)(pStk);

    /* The original top-of-stack address (before allocation of the context record) was
	 * stashed in the B6 and B7 fields of the record by pxPortInitialiseStack(). Since
	 * this address was calculated (in prvInitialiseNewTask()) from the stack base and
	 * length we can, given the stack base, recover the stack length here by reversing
	 * the calculation. (This is only possible for a downwards-growing stack.)
	 */
    StackType_t *pxTopOfStack = (StackType_t*)pRec->B6;
	uint32_t ulStackDepth = (pxTopOfStack - stackBase) + 1ul;

#if (8 == portBYTE_ALIGNMENT)
	/* If the stacks are double-word aligned then we assume that stack depths are even
	 * numbers and we round up here. Double-word alignment is recommended for SHARC but
	 * is not required or assumed by the current compiler.
	 */
	ulStackDepth = (ulStackDepth + 1ul) & ~1ul;
#endif

	/* Knowing the stack length we can now set up the stack bounds correctly in the
	 * L6/7 & B6/7 register fields of the context record.
	 */
	pRec->B6 = (UINT32)stackBase;
	pRec->B7 = (UINT32)stackBase;
	pRec->L6 = ulStackDepth;
	pRec->L7 = ulStackDepth;
}
/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1

#error Tickless IDLE not currently implemented for SHARC

	__attribute__((weak)) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
	{
		/* Not supported */
	}

#endif /* #if configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
__attribute__(( weak )) void vPortSetupTimerInterrupt( void )
{
	/* Configure SysTick to interrupt at the requested rate. */
	const uint32_t ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );

	/* Calculate the constants required to configure the tick interrupt. */
	#if configUSE_TICKLESS_IDLE == 1
	{
		xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
		ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
	}
	#endif /* configUSE_TICKLESS_IDLE */

	sysreg_bit_clr(sysreg_MODE2, BITM_REGF_MODE2_TIMEN);         /* make sure the core timer is off */
	asm volatile ("TPERIOD=%0;\n"                                  /* The period and current count = ulTimerCountsForOneTick */
		"TCOUNT=%1;\n" : :"d" (ulTimerCountsForOneTick), "d" (ulTimerCountsForOneTick) );

	adi_rtl_register_dispatched_handler(configTIMER_INTERRUPT, xPortSysTickHandler,	NULL);
	adi_rtl_activate_dispatched_handler(configTIMER_INTERRUPT);

	sysreg_bit_set(sysreg_MODE2, BITM_REGF_MODE2_TIMEN); /* start the core timer */
}

/*-----------------------------------------------------------*/

#if( configASSERT_DEFINED == 1 )

	void vPortValidateInterruptPriority( void )
	{
		/* On SHARC we don't currently distinguish between interrupt priorities which are valid
		 * for use with FreeRTOS and those which are not - all interrupt priorities can call
		 * FreeRTOS APIs. For this reason this function performs no checking.
		 */
	}

#endif /* configASSERT_DEFINED */
