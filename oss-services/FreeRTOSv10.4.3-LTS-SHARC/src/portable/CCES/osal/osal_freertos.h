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

/*!    @file osal_freertos.h */
#ifndef __OSAL_FREERTOS_H__
#define __OSAL_FREERTOS_H__



/*=============  I N C L U D E S   =============*/
#include "osal_misra.h"



/* Suppression for the FreeRTOS files */
ADI_OSAL_MISRA_SUPPRESS_ALL

#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <event_groups.h>

ADI_OSAL_MISRA_RESTORE_ALL

#include <adi_types.h>
#include "osal_common.h"
#include "adi_osal_arch_internal.h"

/*! @addtogroup ADI_OSAL_FreeRTOS ADI OSAL FreeRTOS
 *  @{
 */

/* disable misra diagnostics as necessary
 *
 * Error[Pm120]: the # and ## preprocessor operators should not be used
 *                (MISRA C 2004 rule 19.13)
 *
 * Error[Pm123]: there shall be no definition of objects or functions in a
 *                  header file (MISRA C 2004 rule 8.5)
 *
 * Error[Pm127]: a 'U' suffix shall be applied to all constants of 'unsigned' type
 *                (MISRA C 2004 rule 10.6)
 *
 * Error[Pm128]: illegal implicit conversion from underlying MISRA type
 *                (MISRA C 2004 rule 10.1)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm120,Pm123,Pm127,Pm128")
#endif
/*! @endcond */

/*==============  D E F I N E S  ===============*/
#if defined(__STDC__)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
  @def ADI_OSAL_PATCH_VER
  @details Patch version of  OSAL for FreeRTOS.  Please refer "adi_osal.h" for
  ADI_OSAL_MAJOR_VER and ADI_OSAL_MINOR_VER since these fields are
  also used for the code-base version
*/
#define ADI_OSAL_PATCH_VER              0u


#ifdef ADI_DEBUG
/* in case we ever want to turn on the checks in 'release' mode */
#ifndef OSAL_DEBUG
#define OSAL_DEBUG
#endif /* OSAL_DEBUG */
#endif /* ADI_DEBUG */

/*!
  @def OSAL_INITIALIZED
  @details This preprocessor constant is used to indicate is the OSAL has been initialized.
   This is used instead of boolean flag in order to minimize the risk of match when
   the memory is uninitialized after processor boot
*/
#define OSAL_INITIALIZED                (0xAD10ul)

/*!
  @def INIT_COUNT_FOR_EMULATED_MUTEX
  @details Semaphores are used in place of Mutexes in this implementation
 */
#define INIT_COUNT_FOR_EMULATED_MUTEX   1u

/*!
  @def OSAL_MINIMUM_HEAP_SIZE
  @details Defines the minimum number of bytes that the user has to pass during OSAL
    Initialization to create the OSAL heap.  For now in FreeRTOS, there are no real
    requirements, so we just check that it is not 0
*/
#define OSAL_MINIMUM_HEAP_SIZE          ((uint32_t) 0u)


/*!
  @def CALLED_IN_SCHED_LOCK_REGION
  @details The scheduler and critical region locks counters can be used to verify if
  the user calls a function that can potentially create a deadlock situation
 */

#define CALLED_IN_SCHED_LOCK_REGION     (_adi_osal_IsSchedLocked())

/*!
  @def CALLED_BEFORE_OS_RUNNING
  @details The scheduler and critical region locks counters can be used to verify if
   the user calls a function that can potentially create a hang situation
 */

#define CALLED_BEFORE_OS_RUNNING       (_adi_osal_OsNotStarted())


/*!
  @def ADI_OSAL_FREERTOS_BASE_PRIO
  @details FreeRTOS doesn't reserve any priorities and the
    Idle thread priority in FreeRTOS is 0
*/
#define ADI_OSAL_FREERTOS_BASE_PRIO         tskIDLE_PRIORITY


/*!
  @def COMPATIBLE_OS_VERSION
  @details The current code has been developed and tested with FreeRTOS version 8.1.2
*/

#define COMPATIBLE_OS_VERSION           (0x812u)

/*!
  @def FREERTOS_NONBLOCKING_CALL
  @details For non blocking calls in FreeRTOS, No waiting
*/
#define FREERTOS_NONBLOCKING_CALL (0u)

/*!
  @def FREERTOS_MESSAGESIZE
  @details Message size in FreeRTOS is fixed as size of UBaseType_t
*/
#define  FREERTOS_MESSAGESIZE  (sizeof( UBaseType_t))


/*!
  @def EVENTFLAG_MAX_SIZE
  @details Event flag size is configured to 24 bit in FreeRTOS
*/
  #define EVENTFLAG_MAX_SIZE 0xFFFFFFu


/*!
  @def FREERTOS_SEMAPHORE_OBJ_SIZE
  @details Semaphore object size in FreeRTOS V8.1.2 is 76bytes.
  Queue_t is typedfed in tasks.c file and not the header
  file hence sizeof(Queue_t) cannot be used in the OSAL code.
  Thus defining the below macro
*/
#define  FREERTOS_SEMAPHORE_OBJ_SIZE   (sizeof(StaticSemaphore_t))

/*!
  @def FREERTOS_EVENTGROUP_OBJ_SIZE
  @details Event group object size in FreeRTOS V8.1.2 is 28bytes.
  EventGroup_t is typedfed in event_groups.c file and hence
  sizeof(EventGroup_t) cannot be used in the OSAL code. Thus
  defining the below macro
*/
#define  FREERTOS_EVENTGROUP_OBJ_SIZE   (sizeof(StaticEventGroup_t))


/*!
  @def FREERTOS_KERNEL_VERSION_NUM
  @details FreeRTOS Kernel Version Number
*/
#define FREERTOS_KERNEL_VERSION_NUMBER        tskKERNEL_VERSION_NUMBER

/*!
  @def FREERTOS_KERNEL_VERSION_MAJ
  @details FreeRTOS Kernel Version Major Number
*/
#define FREERTOS_KERNEL_VERSION_MAJOR         tskKERNEL_VERSION_MAJOR

/*!
  @def FREERTOS_KERNEL_VERSION_MIN
  @details FreeRTOS Kernel Version Minor Number
*/
#define FREERTOS_KERNEL_VERSION_MINOR         tskKERNEL_VERSION_MINOR

/*!
  @def FREERTOS_KERNEL_VERSION_BUI
  @details FreeRTOS Kernel Version Build Number
*/
#define FREERTOS_KERNEL_VERSION_BUILD         tskKERNEL_VERSION_BUILD

/*!
  @def FREERTOS_CRITICAL_ENABLE
  @details FreeRTOS OSAL Critical Region implementation
           when set to 1 it uses FreeRTOS method of enabling/disabling interrupts in Critical Region
           when set to 0 it uses native method of enabling/disabling interrupts in Critical Region
*/
#define FREERTOS_CRITICAL_ENABLE        0
#if 0 // BW
static uint32_t _adi_osal_InterruptsDisable(void);
static void _adi_osal_InterruptsEnable(uint32_t previousState);

/*!
  ****************************************************************************
   @internal

   @fn _adi_osal_InterruptsDisable(void)

   @brief Interrupt Disable function

   @details This Function disables the interrupts from the OSAL

   Parameters:
   @return state - State of the interrupt mask register
   @endinternal

   @note In FreeRTOS, the critical region disables only those interrupts
   whose priority is less than the configMAX_SYSCALL_INTERRUPT_PRIORITY setting
   in FreeRTOSConfig.h file
*****************************************************************************/
#pragma inline
static uint32_t _adi_osal_InterruptsDisable(void)
{

    /* FreeRTOS way of implementing Critical Region */
#if FREERTOS_CRITICAL_ENABLE
    uint32_t state = ulPortSetInterruptMask();
#else
    uint32_t state = __get_PRIMASK();
    if(state == 0u)
    {
        __disable_irq();
    }
#endif

    return state;
}

/*!
  ****************************************************************************
   @internal

   @fn _adi_osal_InterruptsEnable(uint32_t previousState)

   @brief Interrupt Enable function

   @details This Function enables the interrupts from the OSAL

   Parameters:
   @param[in] previousState - Previous state of the interrupt mask register
   @endinternal

   @note In FreeRTOS, the critical region disables only those interrupts
   whose priority is less than the configMAX_SYSCALL_INTERRUPT_PRIORITY setting
   in FreeRTOSConfig.h file
*****************************************************************************/
#pragma inline
static void _adi_osal_InterruptsEnable(uint32_t previousState)
{
     /* FreeRTOS way of implementing Critical Region */
#if FREERTOS_CRITICAL_ENABLE
    vPortClearInterruptMask(previousState);
#else
    if(previousState == 0u)
    {
        __enable_irq();
    }
#endif

}
#endif
/*! @details Structure:  ADI_OSAL_THREAD_INFO
    This structure stores the thread information such as thread signature
    and thread handle
*/
#if 1 /* BW */
typedef struct _ADI_Thread_Handle *ADI_OSAL_THREAD_INFO;

#else
typedef struct __AdiOsalThreadInfo
{
    /*! @var nThreadSignature
     * @details Stores the thread signature to determine if a pointer is an OSAL thread
     */
    uint32_t nThreadSignature;

    /*! @var pNativeThread
     * @details Stores the task  handle  of the created task with which it can be
     *  referenced
     */
    TaskHandle_t pNativeThread;
} ADI_OSAL_THREAD_INFO, *ADI_OSAL_THREAD_INFO_PTR;


/*!
  @def ADI_OSAL_THREAD_SIGNATURE
  @brief signature added to OSAL threads on creation
*/
#define ADI_OSAL_THREAD_SIGNATURE (0x41444954u)

/*!
  @def ADI_OSAL_INVALID_THREAD_SIGNATURE
  @brief signature added to OSAL threads on destruction
*/
#define ADI_OSAL_INVALID_THREAD_SIGNATURE (0x6e746872u)
#endif
/*!
  @def GetOSVersion()
  @brief Checks the Version of the RTOS used.
*/
#define GetOSVersion() (check_osversion())

static inline uint32_t check_osversion( void );
static inline bool _adi_osal_IsSchedLocked(void);
static inline bool _adi_osal_OsNotStarted(void);
static inline bool _adi_osal_IsOSALThread(ADI_OSAL_THREAD_INFO const *hThread);

/*! @fn check_osversion(void)
    @details This is function returns the FreeRTOS build version
*/
inline
uint32_t check_osversion( void )
{
    return ((FREERTOS_KERNEL_VERSION_MAJOR << (8))|(FREERTOS_KERNEL_VERSION_MINOR << (4))|(FREERTOS_KERNEL_VERSION_BUILD));
}


/*! @def ADI_OSAL_FREERTOS_MAX_TCB_SIZE
    @details This is the worst case TCB structure size (based on a single thread
    register) The 51 words have been computed using the worst case definition
    based on FreeRTOS v8.1.2 - Doesn't include configGENERATE_RUN_TIME_STATS
    and configUSE_NEWLIB_REENTRANT. Task Name length is ADI_OSAL_MAX_THREAD_NAME
*/
#define ADI_OSAL_FREERTOS_MAX_TCB_SIZE           ((uint32_t) (37u*4u ))


/*! @def COMPILE_TIME_ASSERT(condition,name)
 *  @brief Utility Macro to do compile time assertion
 */
#define COMPILE_TIME_ASSERT(condition, name) typedef uint8_t name##_failed[(condition)?1:-1]



/*=============  E X T E R N A L S  =============*/

/* Local global variables that are shared across files */
extern uint32_t _adi_osal_gnNumSlots;
extern uint32_t _adi_osal_gTLSAvailableSlots;
extern ADI_OSAL_TLS_CALLBACK_PTR _adi_osal_gaTLSCallbackTable[ADI_OSAL_MAX_NUM_TLS_SLOTS];
extern uint32_t _adi_osal_gnTickPeriod;
extern uint32_t _adi_osal_gkLowestPrio;
extern ADI_OSAL_THREAD_INFO _adi_osal_oStartupVirtualThread;
extern uint32_t _adi_osal_gnSchedulerLockCnt;
extern int32_t _adi_osal_gnCriticalRegionNestingCnt;

/*! @fn _adi_osal_IsSchedLocked(void)
 *  @details Scheduler Lock Status checks for taskSCHEDULER_SUSPENDED
 */
static inline
bool _adi_osal_IsSchedLocked(void)
{
    return (taskSCHEDULER_SUSPENDED == xTaskGetSchedulerState());
}
/*! @fn _adi_osal_OsNotStarted(void)
 *  @details Scheduler Started Status checks for taskSCHEDULER_NOT_STARTED
 */
static inline
bool _adi_osal_OsNotStarted(void)
{
    return (taskSCHEDULER_RUNNING != xTaskGetSchedulerState());
}
/*! @fn _adi_osal_IsOSALThread(ADI_OSAL_THREAD_INFO const *hThread)
 *  @param[in] hThread - Thread Info
 *  @details Checks if Thread Signature is equal to ADI_OSAL_THREAD_SIGNATURE
 */
static inline
bool _adi_osal_IsOSALThread(ADI_OSAL_THREAD_INFO const *hThread)
{
    return 1; //( hThread->nThreadSignature == ADI_OSAL_THREAD_SIGNATURE);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#else /* assembly language specific macros and declarations*/


#endif  /* if !defined(_LANGUAGE_C) */

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm120,Pm123,Pm127,Pm128")
#endif
/*! @endcond */
#endif /*__OSAL_FREERTOS_H__ */

/*
**
** EOF:
**
*/
/*@}*/
