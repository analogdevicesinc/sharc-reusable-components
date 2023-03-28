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

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdint.h>            /* for uint32_t */
#include <sysreg.h>            /* for sysreg_IRPTL */
#include <builtins.h>          /* for leftz() and sysreg_bit_set() */
#include <sys/platform.h>      /* for BITM_REGF_IMASK_SFT3I */
#include <sys/anomaly_macros_rtl.h>
#include <interrupt.h>         /* for ADI_CID_SFT3I */

#if WA_20000081
#if defined(_LANGUAGE_ASM)
.extern VAR_NAME(_adi_OSRescheduleIntID); /* global var to store the per-core interrupt ID */
#else
extern uint32_t _adi_OSRescheduleIntID; /* global var to store the per-core interrupt ID */
#endif /* !defined(_LANGUAGE_ASM) */
#else /* !WA_20000081 */
#define OS_RESCHEDULE_INT (BITM_REGF_IMASK_SFT3I)
#define OS_RESCHEDULE_CID (ADI_CID_SFT3I)
#endif /* WA_20000081 */

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the given hardware
 * and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef portBASE_TYPE BaseType_t;
typedef unsigned portBASE_TYPE UBaseType_t;

typedef uint32_t TickType_t;
#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

/* 32-bit tick type on a 32-bit architecture, so reads of the tick count do
not need to be guarded with a critical section. */
#define portTICK_TYPE_IS_ATOMIC 1

/*-----------------------------------------------------------*/

/* Hardware specifics
 *
 * The SHARC-XI stack grows downwards and is 8-byte aligned for preference, though
 * the current compiler only requires 4-byte alignment.
 */
#define portSTACK_GROWTH			( -1 ) /* Stack grows downwards (i.e. towards lower memory addresses */
#define portBYTE_ALIGNMENT			8 /* 8-byte (double-word) stack alignment is recommended for SHARC, but not mandatory */

#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )

/*-----------------------------------------------------------*/

/* Task utilities
 *
 * The SHARC-XI port of FreeRTOS uses the lowest-priority core software interrupt (SFT3)
 * to trigger all context-switching, both from interrupt level and from task level.
 */
#if WA_20000081
/* This volaile flag variable is used by tasks, to block their progress after raising the reschedule
 * interrupt until the interrupt has been serviced. This is to compensate for the much greater latency
 * of an SEC interrupt compared with a core interrupt.
 */
extern volatile uint32_t _adi_OSWaitingForSched;
/* Raise the reschedule interrupt, without waiting for it to be serviced */
#define portYIELD() { *pREG_SEC0_RAISE = _adi_OSRescheduleIntID;}
/* Raise the reschedule interrupt and wait for it to be serviced *unless* interrupts are globally disabled */
#define portYIELD_WITHIN_API() { _adi_OSWaitingForSched = 1u; *pREG_SEC0_RAISE = _adi_OSRescheduleIntID; if (__builtin_sysreg_bit_tst(sysreg_MODE1, 0x1000)) { while(_adi_OSWaitingForSched) {}; } }
#else /* !WA_20000081 */
#define portYIELD() 			sysreg_bit_set(sysreg_IRPTL, OS_RESCHEDULE_INT)
#define portYIELD_WITHIN_API()	sysreg_bit_set(sysreg_IRPTL, OS_RESCHEDULE_INT)
#endif /* WA_20000081 */

#define portEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != pdFALSE ) portYIELD()
#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )


/*-----------------------------------------------------------
 * Critical section control
 *----------------------------------------------------------*/

/* These macros and function *do* globally disable/enable interrupts.
 * ADI processors which feature the System Event Controller (SEC) do not
 * currently support selective interrupt masking, (i.e. using
 * configMAX_API_CALL_INTERRUPT_PRIORITY).
 */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern uint32_t ulPortSetInterruptMask( void );
extern void vPortClearInterruptMask( uint32_t ulNewMaskValue );

#define portENTER_CRITICAL()		vPortEnterCritical();
#define portEXIT_CRITICAL()			vPortExitCritical();
#define portDISABLE_INTERRUPTS()	   ulPortSetInterruptMask()
#define portENABLE_INTERRUPTS()		vPortClearInterruptMask( 0 )
#define portSET_INTERRUPT_MASK_FROM_ISR()		ulPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortClearInterruptMask(x)

/* In order to set up the stack limits for a newly-created task we define
 * portSETUP_TCB(). This gives us access to the Task Control Block, which
 * is not defined outside of tasks.c.
 * vPortFixupStack() uses the stack information from the TCB to set up
 * the stack base and length register values in the task's context record.
 */
extern void vPortFixupStack(volatile StackType_t *stackTop, StackType_t *stackBase);
#define portSETUP_TCB( pxTCB ) { \
	vPortFixupStack((pxTCB)->pxTopOfStack, (pxTCB)->pxStack); \
}

/*-----------------------------------------------------------*/

/* Task function macros as described on the FreeRTOS.org WEB site.  These are
not required for this port but included in case common demo code that uses these
macros is used. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )	void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )	void vFunction( void *pvParameters )


#define portLOWEST_INTERRUPT_PRIORITY ( ( ( uint32_t ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define portLOWEST_USABLE_INTERRUPT_PRIORITY ( portLOWEST_INTERRUPT_PRIORITY - 1UL )

/* Architecture specific optimisations. */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

	/* Check the configuration. */
	#if( configMAX_PRIORITIES > 31 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 31.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif

	/* Store/clear the ready priorities in a bit map. */
	#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

	/*-----------------------------------------------------------*/

	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - leftz( uxReadyPriorities ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

#ifdef configASSERT
	void vPortValidateInterruptPriority( void );
	#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 	vPortValidateInterruptPriority()
#endif /* configASSERT */

#define portNOP() __asm volatile( "NOP;" )
#define portINLINE __inline

#ifdef __cplusplus
	} /* extern C */
#endif

#endif /* PORTMACRO_H */

