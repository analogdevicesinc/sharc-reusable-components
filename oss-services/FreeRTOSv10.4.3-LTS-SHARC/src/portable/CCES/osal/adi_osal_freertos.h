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

/*****************************************************************************
    FreeRTOS implementation specific version of OSAL definitions.
    This header file should be included automatically by the main adi_osal.h
    header file that is located within CCES.
*****************************************************************************/

#ifndef __ADI_OSAL_H__
#error The header file adi_osal_freertos.h should be included by including the file adi_osal.h from CrossCore Embedded Studio in your compilation.
#endif

#ifndef __ADI_OSAL_FREERTOS_H__
#define __ADI_OSAL_FREERTOS_H__

/*! @file adi_osal_freertos.h */

/* disable misra diagnostics as necessary */
#include "osal_misra.h"
/* Suppression for the ADSP-CM40x device.h file */
ADI_OSAL_MISRA_SUPPRESS_ALL
ADI_OSAL_MISRA_RESTORE_ALL
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm088,Pm100,Pm114,Pm120,Pm123,Pm127,Pm128,Pm138,Pm139,Pm140,Pm141,Pm143")
#endif
/*! @endcond */
/*=============  I N C L U D E S   =============*/


#include <adi_osal_arch.h>

#ifdef __STDC__
#include <stdint.h>
#include <stdbool.h>
#include <adi_types.h>
#include <FreeRTOS.h>
#endif
/*==============  D E F I N E S  ===============*/

/*
 *  Generic Constants usable in both C and ASM
 */


/*! @name ADI_OSAL_VERSION
 *  @brief Current API version (major.minor)
 * @{
 */
#define     ADI_OSAL_MAJOR_VER    1u
#define     ADI_OSAL_MINOR_VER    2u
/*@}*/

/*-------------------------------------------------------------------*
     Return/Status codes - these correspond to the codes
     in the enum ADI_OSAL_STATUS
 *-------------------------------------------------------------------*/
/*! @name ADI_OSAL_RET_STATUS
 *  @brief Return/Status codes
 *  @details  these correspond to the codes in the enum ADI_OSAL_STATUS
 * @{
 */
#define     E_ADI_OSAL_SUCCESS                  0x00       /*!< Generic Success */

#define     E_ADI_OSAL_FAILED                   0x01       /*!<  Generic Failure */

#define     E_ADI_OSAL_BAD_COUNT                0x02       /*!<  The value specified for the initial semaphore count exceeds
                                                              the underlying operating systems count range*/
#define     E_ADI_OSAL_BAD_EVENT                0x03       /*!<  An invalid event flag was specified (an attempt was made to
                                                              utilize bit 31). */
#define     E_ADI_OSAL_BAD_HANDLE               0x04       /*!<  The supplied handle is invalid */

#define     E_ADI_OSAL_BAD_HEAP                 0x05       /*!<  Heap pointer is not 4-bytes aligned */

#define     E_ADI_OSAL_BAD_IVG_LEVEL            0x06       /*!<  An attempt was made to install an ISR into a level that cannot
                                                              be modified */
#define     E_ADI_OSAL_BAD_MEMORY               0x07       /*!<  The pointer to memory is invalid (may not be aligned) */

#define     E_ADI_OSAL_BAD_OPTION               0x08       /*!<  An invalid option was specified for the event pend operation */

#define     E_ADI_OSAL_BAD_PRIO_INHERIT         0x09       /*!<  Priority inheritance is not supported by the underlying OS */

#define     E_ADI_OSAL_BAD_PRIORITY             0x0a       /*!<  The priority level indicated is invalid for the underlying OS */

#define     E_ADI_OSAL_BAD_SLOT_KEY             0x0b       /*!<  The specified thread local storage slot number exceeds the
                                                              available slots */
#define     E_ADI_OSAL_BAD_STACK_ADDR           0x0c       /*!<  The thread stack address supplied as part of the ADI_OSAL_THREAD_ATTR
                                                              structure is not aligned correctly */
#define     E_ADI_OSAL_BAD_STACK_SIZE           0x0d       /*!<  The thread stack size supplied as part of the ADI_OSAL_THREAD_ATTR
                                                              structure does not meet the underlying operating system
                                                              minimum requirements */
#define     E_ADI_OSAL_BAD_THREAD_FUNC          0x0e       /*!<  The thread function supplied as part of the ADI_OSAL_THREAD_ATTR
                                                              structure is invalid */
#define     E_ADI_OSAL_BAD_THREAD_NAME          0x0f       /*!<  The string specifying the thread name as part of the ADI_OSAL_THREAD_ATTR
                                                              structure exceeds the size specified by ADI_OSAL_MAX_THREAD_NAME */
#define     E_ADI_OSAL_BAD_TIME                 0x10       /*!<  The time for sleep or timeout exceeds the range allowed by the
                                                              underlying operating system */
#define     E_ADI_OSAL_BAD_TIMER_PERIOD         0x11       /*!<  The value specified for nSysTimerPerioduSec is set to zero,
                                                              yet the underlying operating system does not supply timing information */
#define     E_ADI_OSAL_CALLBACK_FAILED          0x12       /*!<  Startup callback (during adi_osal_OSStart) failed (or something
                                                              called in it failed) */
#define     E_ADI_OSAL_CALLER_ERROR             0x13       /*!<  The function was called from an invalid location (such as in
                                                              an ISR, or during startup) */
#define     E_ADI_OSAL_COUNT_OVERFLOW           0x14       /*!<  An internal counter overflowed (e.g. the semaphore count, the
                                                              mutex ownership count, etc) */
#define     E_ADI_OSAL_DESTROY_SELF             0x15       /*!<  A thread is attempting to destroy itself (not supported) */

#define     E_ADI_OSAL_IVG_LEVEL_IN_USE         0x16       /*!<  Specified IVG level already has an ISR installed */

#define     E_ADI_OSAL_MEM_ALLOC_FAILED         0x17       /*!<  No memory available for allocation of the specified object */

#define     E_ADI_OSAL_NOT_MUTEX_OWNER          0x18       /*!<  The current thread does not own the mutex being destroyed */

#define     E_ADI_OSAL_OS_ERROR                 0x19       /*!<  The underlying OS is not setup in a compatible way or has returned
                                                             an error during initialization */
#define     E_ADI_OSAL_PRIORITY_IN_USE          0x1a       /*!<  Specified priority is already in use (occurs for operating systems
                                                              that do not support more than one thread per priority level)*/
#define     E_ADI_OSAL_QUEUE_EMPTY              0x1b       /*!<  Message queue is empty when fetching a message */

#define     E_ADI_OSAL_QUEUE_FULL               0x1c       /*!<  Message queue is full when sending a message */

#define     E_ADI_OSAL_START_FAILED             0x1d       /*!<  Underlying RTOS failed to start */

#define     E_ADI_OSAL_THREAD_PENDING           0x1e       /*!<  An attempt was made to destroy a semaphore, mutex or message queue
                                                              when a thread is pending on the object*/
#define     E_ADI_OSAL_TIMEOUT                  0x1f       /*!<  Timeout occurred (Pend functions) */

#define     E_ADI_OSAL_SLOT_NOT_ALLOCATED       0x20       /*!<  An attempt was made to use a thread slot before allocating it */

#define     E_ADI_OSAL_MEM_TOO_SMALL            0x21       /*!<  The memory size given is not sufficient for its purpose */

#define     E_ADI_OSAL_INVALID_ARGS             0x22       /*!<  The API arguments are not valid */
/*@}*/


/*! @name EVENT_FLAGS
 *  @brief Event Flags (only bits 0 - 30 can be used)
 *  @{
 */
#define     ADI_OSAL_EVENT_FLAG0                0x00000001
#define     ADI_OSAL_EVENT_FLAG1                0x00000002
#define     ADI_OSAL_EVENT_FLAG2                0x00000004
#define     ADI_OSAL_EVENT_FLAG3                0x00000008
#define     ADI_OSAL_EVENT_FLAG4                0x00000010
#define     ADI_OSAL_EVENT_FLAG5                0x00000020
#define     ADI_OSAL_EVENT_FLAG6                0x00000040
#define     ADI_OSAL_EVENT_FLAG7                0x00000080
#define     ADI_OSAL_EVENT_FLAG8                0x00000100
#define     ADI_OSAL_EVENT_FLAG9                0x00000200
#define     ADI_OSAL_EVENT_FLAG10               0x00000400
#define     ADI_OSAL_EVENT_FLAG11               0x00000800
#define     ADI_OSAL_EVENT_FLAG12               0x00001000
#define     ADI_OSAL_EVENT_FLAG13               0x00002000
#define     ADI_OSAL_EVENT_FLAG14               0x00004000
#define     ADI_OSAL_EVENT_FLAG15               0x00008000
#define     ADI_OSAL_EVENT_FLAG16               0x00010000
#define     ADI_OSAL_EVENT_FLAG17               0x00020000
#define     ADI_OSAL_EVENT_FLAG18               0x00040000
#define     ADI_OSAL_EVENT_FLAG19               0x00080000
#define     ADI_OSAL_EVENT_FLAG20               0x00100000
#define     ADI_OSAL_EVENT_FLAG21               0x00200000
#define     ADI_OSAL_EVENT_FLAG22               0x00400000
#define     ADI_OSAL_EVENT_FLAG23               0x00800000
#define     ADI_OSAL_EVENT_FLAG24               0x01000000
#define     ADI_OSAL_EVENT_FLAG25               0x02000000
#define     ADI_OSAL_EVENT_FLAG26               0x04000000
#define     ADI_OSAL_EVENT_FLAG27               0x08000000
#define     ADI_OSAL_EVENT_FLAG28               0x10000000
#define     ADI_OSAL_EVENT_FLAG29               0x20000000
#define     ADI_OSAL_EVENT_FLAG30               0x40000000
/* bit 31, 0x80000000 is reserved */
/*@}*/

/*! @def ADI_OSAL_INVALID_PRIORITY
 *  @brief Invalid priority level
 *  @details (this is a priority level that cannot occur in normal usage)
 */
#define     ADI_OSAL_INVALID_PRIORITY           0xFFFFFFFF

/*! @def ADI_OSAL_INVALID_THREAD_SLOT
 *  @brief Invalid thread slot number
 *  @details (this is a thread slot number that cannot occur in normal usage)
 */
#define     ADI_OSAL_INVALID_THREAD_SLOT        0xFFFFFFFF



#if defined(__STDC__)
/*
 *  Constants only usable in C ...
 */


/*! @name TIMEOUT_CONSTANTS
 *  @brief Timeout constants
 *  @{
 */
#define     ADI_OSAL_TIMEOUT_NONE               0u
#define     ADI_OSAL_TIMEOUT_FOREVER            0xFFFFFFFFu
/*@}*/

/*! @def ADI_OSAL_TLS_UNALLOCATED
 *  @brief Unallocated TLS Slot - to be passed to adi_osal_ThreadSlotAcquire.
 */
#define ADI_OSAL_TLS_UNALLOCATED (0xF0F0F0F0)

/*! @name INVALID_HANDLES
 *  @details    Macros defined for invalid handles for mutex, thread, semaphore etc.
 *              (these are pointers to locations that are invalid for a normal handle
 *              i.e. the last address in memory is normally core MMR space)
 *  @{
*/
#define     ADI_OSAL_INVALID_THREAD             ((ADI_OSAL_THREAD_HANDLE) -1)
#define     ADI_OSAL_INVALID_MUTEX              ((ADI_OSAL_MUTEX_HANDLE)  -1)
#define     ADI_OSAL_INVALID_SEM                ((ADI_OSAL_SEM_HANDLE)    -1)
#define     ADI_OSAL_INVALID_QUEUE              ((ADI_OSAL_QUEUE_HANDLE)  -1)
#define     ADI_OSAL_INVALID_EVENT_GROUP        ((ADI_OSAL_EVENT_HANDLE)  -1)
/*@}*/

/*! @def  ADI_OSAL_MAX_MUTEX_SIZE_CHAR
 *  @details This is the OS-specific implementation of OSAL (for FreeRTOS) so we can
 *           use the correct sizes for static objects, as provided by the FreeRTOS.h header.
 */
#define     ADI_OSAL_MAX_MUTEX_SIZE_CHAR              (sizeof(StaticSemaphore_t))

/*! @def ADI_OSAL_MAX_SEM_SIZE_CHAR
 *  @details This is the OS-specific implementation of OSAL (for FreeRTOS) so we can
 *           use the correct sizes for static objects, as provided by the FreeRTOS.h header.
 */
#define     ADI_OSAL_MAX_SEM_SIZE_CHAR                (sizeof(StaticSemaphore_t))

/*! @def ADI_OSAL_MAX_EVENT_GROUP_SIZE_CHAR
 *  @details This is the OS-specific implementation of OSAL (for FreeRTOS) so we can
 *           use the correct sizes for static objects, as provided by the FreeRTOS.h header.
 */
#define     ADI_OSAL_MAX_EVENT_GROUP_SIZE_CHAR        (sizeof(StaticEventGroup_t))

/*! @def ADI_OSAL_SEM_MAX_COUNT
 *  @details Maximum count for a semaphore
 *           Although many RTOS may cope with UINT_MAX as the maximum size for a
 *           semaphore, uCOS-II has a 16-bit field. For semaphore creation OSAL limits
 *           the count to something that all supported RTOS can manage
 */
#define     ADI_OSAL_SEM_MAX_COUNT              (0xFFFFu)

/*! @def  ADI_OSAL_MAX_TIMEOUT
 * @details Maximum Timeout
 *          Although many RTOS may cope with UINT_MAX as the maximum timeout
 *          uCOS-II has a 16-bit field. For semaphore creation OSAL limits
 *          the timeout to something that all supported RTOS can manage
 */
#define    ADI_OSAL_MAX_TIMEOUT                (0xFFFFu)


/*! @name DEFAULT_SETTINGS
 *  @brief Default settings
 *  @{
 */
#define     ADI_OSAL_DEFAULT_PRIO_INHERIT       (ADI_OSAL_PRIO_INHERIT_AUTO)      /*!< use the underlying OS for priority inheritance setting */
#define     ADI_OSAL_MAX_NUM_TLS_SLOTS          (configNUM_THREAD_LOCAL_STORAGE_POINTERS)
#define     ADI_OSAL_DEFAULT_SYS_TIMER_PERIOD   (0u)                              /*!< use the underlying OS for period information */

#define     ADI_OSAL_MAX_THREAD_NAME            (80u)
/*@}*/

#endif /* Language C */





/*============= D A T A T Y P E S =============*/


/*
 *    C-Specific Definitions
 */
#if defined(__STDC__)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/*-------------------------------------------------------------------*
 *    Handles definitions                                            *
 *-------------------------------------------------------------------*/

/* NOTE: all handles are opaque pointers - these do NOT
   use void * to ensure valid parameter type checking */

/* Those types are incomplete because they are only used for type checking.
   They are always used as pointer to them (handles) so there isn't a need to define them
   most of the time they are typecasted to the underlying OS specific types. */

/*! @typedef *ADI_OSAL_THREAD_HANDLE
 *  @brief Typedef for a thread handle
 */
typedef struct _ADI_Thread_Handle *ADI_OSAL_THREAD_HANDLE;

/*! @typedef *ADI_OSAL_SEM_HANDLE
 *  @brief Typedef for a semaphore handle
 */
typedef struct _ADI_Sem_Handle *ADI_OSAL_SEM_HANDLE;

/*! @typedef *ADI_OSAL_MUTEX_HANDLE
 *  @brief Typedef for a mutex handle
 */
typedef struct _ADI_Mutex_Handle *ADI_OSAL_MUTEX_HANDLE;

/*! @typedef *ADI_OSAL_QUEUE_HANDLE
 *  @brief Typedef for a message queue handle
 */
typedef struct _ADI_Queue_Handle *ADI_OSAL_QUEUE_HANDLE;

/*! @typedef *ADI_OSAL_EVENT_HANDLE
 *  @brief Typedef for a event handle
 */
typedef struct _ADI_Event_Handle *ADI_OSAL_EVENT_HANDLE;


/*-------------------------------------------------------------------*
 *     Return/Status codes from OSAL API                             *
 *-------------------------------------------------------------------*/
/*! @enum ADI_OSAL_STATUS
 *  @brief Return/Status codes from OSAL API
 */
typedef enum
{
    ADI_OSAL_SUCCESS                  = E_ADI_OSAL_SUCCESS,                 /*!< Generic Success */
    ADI_OSAL_FAILED                   = E_ADI_OSAL_FAILED,                  /*!<  Generic Failure */
    ADI_OSAL_BAD_COUNT                = E_ADI_OSAL_BAD_COUNT,               /*!<  The value specified for the initial semaphore count exceeds the underlying operating systems count range*/
    ADI_OSAL_BAD_EVENT                = E_ADI_OSAL_BAD_EVENT,               /*!<  An invalid event flag was specified (an attempt was made to utilize bit 31). */
    ADI_OSAL_BAD_HANDLE               = E_ADI_OSAL_BAD_HANDLE,              /*!<  The supplied handle is invalid */
    ADI_OSAL_BAD_HEAP                 = E_ADI_OSAL_BAD_HEAP,                /*!<  Heap pointer is not 4-bytes aligned */
    ADI_OSAL_BAD_IVG_LEVEL            = E_ADI_OSAL_BAD_IVG_LEVEL,           /*!<  An attempt was made to install an ISR into a level that cannot be modified */
    ADI_OSAL_BAD_MEMORY               = E_ADI_OSAL_BAD_MEMORY,              /*!<  The pointer to memory is invalid (may not be aligned) */
    ADI_OSAL_BAD_OPTION               = E_ADI_OSAL_BAD_OPTION,              /*!<  An invalid option was specified for the event pend operation */
    ADI_OSAL_BAD_PRIO_INHERIT         = E_ADI_OSAL_BAD_PRIO_INHERIT,        /*!<  Priority inheritance is not supported by the underlying OS */
    ADI_OSAL_BAD_PRIORITY             = E_ADI_OSAL_BAD_PRIORITY,            /*!<  The priority level indicated is invalid for the underlying OS */
    ADI_OSAL_BAD_SLOT_KEY             = E_ADI_OSAL_BAD_SLOT_KEY,            /*!<  The specified thread local storage slot number exceeds the available slots */
    ADI_OSAL_BAD_STACK_ADDR           = E_ADI_OSAL_BAD_STACK_ADDR,          /*!<  The thread stack address supplied as part of the ADI_OSAL_THREAD_ATTR structure is not aligned correctly */
    ADI_OSAL_BAD_STACK_SIZE           = E_ADI_OSAL_BAD_STACK_SIZE,          /*!<  The thread stack size supplied as part of the ADI_OSAL_THREAD_ATTR structure does not meet the underlying operating system minimum requirements */
    ADI_OSAL_BAD_THREAD_FUNC          = E_ADI_OSAL_BAD_THREAD_FUNC,         /*!<  The thread function supplied as part of the ADI_OSAL_THREAD_ATTR structure is invalid */
    ADI_OSAL_BAD_THREAD_NAME          = E_ADI_OSAL_BAD_THREAD_NAME,         /*!<  The string specifying the thread name as part of the ADI_OSAL_THREAD_ATTR structure exceeds the size specified by ADI_OSAL_MAX_THREAD_NAME */
    ADI_OSAL_BAD_TIME                 = E_ADI_OSAL_BAD_TIME,                /*!<  The time for sleep or timeout exceeds the range allowed by the underlying operating system */
    ADI_OSAL_BAD_TIMER_PERIOD         = E_ADI_OSAL_BAD_TIMER_PERIOD,        /*!<  The value specified for nSysTimerPerioduSec is set to zero, yet the underlying operating system does not supply timing information */
    ADI_OSAL_CALLBACK_FAILED          = E_ADI_OSAL_CALLBACK_FAILED,         /*!<  Startup callback (during adi_osal_OSStart) failed (or something called in it failed) */
    ADI_OSAL_CALLER_ERROR             = E_ADI_OSAL_CALLER_ERROR,            /*!<  The function was called from an invalid location (such as in an ISR, or during startup) */
    ADI_OSAL_COUNT_OVERFLOW           = E_ADI_OSAL_COUNT_OVERFLOW,          /*!<  An internal counter overflowed (e.g. the semaphore count, the mutex ownership count, etc) */
    ADI_OSAL_DESTROY_SELF             = E_ADI_OSAL_DESTROY_SELF,            /*!<  A thread is attempting to destroy itself (not supported) */
    ADI_OSAL_IVG_LEVEL_IN_USE         = E_ADI_OSAL_IVG_LEVEL_IN_USE,        /*!<  Specified IVG level already has an ISR installed */
    ADI_OSAL_MEM_ALLOC_FAILED         = E_ADI_OSAL_MEM_ALLOC_FAILED,        /*!<  No memory available for allocation of the specified object */
    ADI_OSAL_NOT_MUTEX_OWNER          = E_ADI_OSAL_NOT_MUTEX_OWNER,         /*!<  The current thread does not own the mutex being destroyed */
    ADI_OSAL_OS_ERROR                 = E_ADI_OSAL_OS_ERROR,                /*!<  The underlying OS is not setup in a compatible way or has returned an error during initialization */
    ADI_OSAL_PRIORITY_IN_USE          = E_ADI_OSAL_PRIORITY_IN_USE,         /*!<  Specified priority is already in use (occurs for operating systems that do not support more than one thread per priority level)*/
    ADI_OSAL_QUEUE_EMPTY              = E_ADI_OSAL_QUEUE_EMPTY,             /*!<  Message queue is empty when fetching a message */
    ADI_OSAL_QUEUE_FULL               = E_ADI_OSAL_QUEUE_FULL,              /*!<  Message queue is full when sending a message */
    ADI_OSAL_START_FAILED             = E_ADI_OSAL_START_FAILED,            /*!<  Underlying RTOS failed to start */
    ADI_OSAL_THREAD_PENDING           = E_ADI_OSAL_THREAD_PENDING,          /*!<  An attempt was made to destroy a semaphore, mutex or message queue when a thread is pending on the object*/
    ADI_OSAL_TIMEOUT                  = E_ADI_OSAL_TIMEOUT,                 /*!<  Timeout occurred (Pend functions) */
    ADI_OSAL_SLOT_NOT_ALLOCATED       = E_ADI_OSAL_SLOT_NOT_ALLOCATED,      /*!<  An attempt was made to use a thread slot before allocating it */
    ADI_OSAL_MEM_TOO_SMALL            = E_ADI_OSAL_MEM_TOO_SMALL,           /*!<  The memory size given is not sufficient for its purpose */
    ADI_OSAL_INVALID_ARGS             = E_ADI_OSAL_INVALID_ARGS             /*!<  The API arguments are not valid */
} ADI_OSAL_STATUS;






/*-------------------------------------------------------------------*
    Pointer to functions definitions
 *-------------------------------------------------------------------*/

/*!
 *  @details ISR prototype definition - used for the pointer to an interrupt service routine
 */
typedef void (*ADI_OSAL_ISR_PTR)(void);

/*!
 *  @details thread function prototype definition - used for the pointer to a thread
 */
typedef void (*ADI_OSAL_THREAD_PTR) (void * thread);

/*!
 *  @details Callback function prototype definition - used for pointer to a callback function
 */
typedef ADI_OSAL_STATUS (*ADI_OSAL_CALLBACK_PTR) (void * status);

/*!
 *   @details Callback function prototype definition for Thread Local Storage Destruction
 *            it is different than regular callbacks because its input and outputs are predefined
 */
typedef void  *ADI_OSAL_SLOT_VALUE;             /*!<   Typedef for value to be stored in thread's local storage buffer. */
typedef void (*ADI_OSAL_TLS_CALLBACK_PTR) (ADI_OSAL_SLOT_VALUE Slot_value);




/*-------------------------------------------------------------------*
 *    OSAL Objects definitions                                       *
 *-------------------------------------------------------------------*/

/*! @typedef ADI_OSAL_TLS_SLOT_KEY
 *  @brief Type definition for the thread's local storage slot key
 */
typedef uint32_t ADI_OSAL_TLS_SLOT_KEY;

/*! @typedef ADI_OSAL_PRIORITY
 *  @brief Typedef representing a thread priority level (0 is highest priority)
 */
typedef uint32_t ADI_OSAL_PRIORITY;

/*! @typedef ADI_OSAL_TICKS
 *  @brief Typedef for a time represented in system ticks
 *
    valid values:
        0               - no wait/timeout (ADI_OSAL_TIMEOUT_NONE)
        1 .. 0xFFFFFFFE - delay/timeout in system ticks
        0xFFFFFFFF      - wait forever (ADI_OSAL_TIMEOUT_FOREVER)
*/
typedef uint32_t ADI_OSAL_TICKS;

/*! @typedef ADI_OSAL_EVENT_FLAGS
 *  @brief Typedef for event flags (31 flags available)
 *    (See also ADI_OSAL_EVENT_FLAGxx definitions)
 */
typedef uint32_t ADI_OSAL_EVENT_FLAGS;

/*! @enum ADI_OSAL_EVENT_FLAG_OPTION
 *  @brief Enumerations for Event Flag Options  (used by adi_osal_EventPend() function)
 */
typedef enum
{
    ADI_OSAL_EVENT_FLAG_ANY = 0,                        /*!< Option to check whether any one of specified flag(s) is set. */
    ADI_OSAL_EVENT_FLAG_ALL                             /*!< Option to check whether all flags specified are set */
} ADI_OSAL_EVENT_FLAG_OPTION;

/*! @enum ADI_OSAL_PRIO_INHERIT_OPTION
 *  @brief Defines the initialization options for priority inheritance support
 */
typedef enum {
    ADI_OSAL_PRIO_INHERIT_ENABLED,                      /*!< OSAL priority inheritance support is enable */
    ADI_OSAL_PRIO_INHERIT_DISABLED,                     /*!< OSAL priority inheritance support is disable */
    ADI_OSAL_PRIO_INHERIT_AUTO                          /*!< OSAL use the underlying RTOS settings for prio inheritance */
} ADI_OSAL_PRIO_INHERIT_OPTION;

/*!
 *  @details Structure defining the argument passed during OSAL configuration (adi_osal_Config)
 */
typedef struct __AdiOsalConfig
{
    ADI_OSAL_PRIO_INHERIT_OPTION    eEnablePrioInherit;             /*!< set priority inheritance initialization option */
    uint32_t                        nNumTLSSlots;                   /*!< number of TLS allocated (for each thread) */
    uint32_t                        nSysTimerPeriodInUsec;          /*!< indicate the system tick period in microseconds */
    void                            *pHeap;                         /*!< pointer to a memory location to use as a heap (4-bytes aligned) */
    uint32_t                        nHeapSizeBytes;                 /*!< size of the Heap in bytes */
} ADI_OSAL_CONFIG, *ADI_OSAL_CONFIG_PTR;

/*!
 *  @details Structure used to define a Callback
 */
typedef struct __AdiOsalCallbackAttr
{
    ADI_OSAL_CALLBACK_PTR       pCallbackFunc;                      /*!< pointer to a function to execute by this callback */
    void                        *pCallBackParam;                    /*!< argument to a callback function */
} ADI_OSAL_CALLBACK_ATTR, *ADI_OSAL_CALLBACK_ATTR_PTR;

/*!
 *  @details Structure containing the attributes of a thread (for thread creation)
 */
typedef struct __AdiOsalTaskAttr
{
    ADI_OSAL_THREAD_PTR pThreadFunc;                                /*!< address of function to invoke for this thread */
    uint32_t            nPriority;                                  /*!< priority level of this thread */
    void                *pStackBase;                                /*!< pointer to base address of thread stack */
    uint32_t            nStackSize;                                 /*!< stack size in bytes */
    void                *pTaskAttrParam;                            /*!< app-specific parameter to pass to the thread function */
    const char_t *      szThreadName;                               /*!< string defining the thread name */
} ADI_OSAL_THREAD_ATTR, *ADI_OSAL_THREAD_ATTR_PTR;

/*!
 *  @details OSAL Code-base version information (OS-dependent)
 */
typedef struct __AdiOsalVersion
{
    uint32_t   nMajor;                      /*!< Major revision number - Changes when API incompatibilities are introduced */
    uint32_t   nMinor;                      /*!< Minor revision number - Changes if features are added */
    uint32_t   nPatch;                      /*!< Patch number - Changes when bug fixes are made */
    uint32_t   nBuild;                      /*!< Build revision - Contains the subversion revision of the code */
} ADI_OSAL_VERSION, *ADI_OSAL_VERSION_PTR;




/*-------------------------------------------------------------------*
          Prototypes for OSAL functions
 *-------------------------------------------------------------------*/
/* (globally-scoped functions) */


ADI_OSAL_STATUS adi_osal_GetVersion(ADI_OSAL_VERSION* pVersion);
bool            adi_osal_IsSchedulerActive(void);

/*-------  Prototypes for OS Initialization  function  -------*/

ADI_OSAL_STATUS adi_osal_Init(void);
ADI_OSAL_STATUS adi_osal_Config(const ADI_OSAL_CONFIG *pConfig);


/*-------  Prototypes for Timer APIs  -------*/

ADI_OSAL_STATUS adi_osal_TickPeriodInMicroSec(uint32_t *pnTickPeriod);
void            adi_osal_TimeTick(void);
ADI_OSAL_STATUS adi_osal_GetCurrentTick(uint32_t *pnTicks);


/*-------  Prototypes for Thread management APIs  -------*/

ADI_OSAL_STATUS adi_osal_ThreadCreate(ADI_OSAL_THREAD_HANDLE *phThread, const ADI_OSAL_THREAD_ATTR *pThreadAttr);
ADI_OSAL_STATUS adi_osal_ThreadDestroy(ADI_OSAL_THREAD_HANDLE const hThread);
ADI_OSAL_STATUS adi_osal_ThreadGetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY *pnThreadPrio);
ADI_OSAL_STATUS adi_osal_ThreadSetPrio(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_PRIORITY nNewPriority);
ADI_OSAL_STATUS adi_osal_ThreadSleep(ADI_OSAL_TICKS nTimeInTicks);
ADI_OSAL_STATUS adi_osal_ThreadGetHandle(ADI_OSAL_THREAD_HANDLE *phThread);
ADI_OSAL_STATUS  adi_osal_ThreadGetNativeHandle(void **phThread);
ADI_OSAL_STATUS adi_osal_ThreadGetName(char_t *pszTaskName, uint32_t    nNumBytesToCopy);


/*-------  Prototypes for Thread Local Storage APIs  -------*/

ADI_OSAL_STATUS adi_osal_ThreadSlotAcquire(ADI_OSAL_TLS_SLOT_KEY *pnThreadSlotKey,
                                           ADI_OSAL_TLS_CALLBACK_PTR pTerminateCallbackFunc);
ADI_OSAL_STATUS adi_osal_ThreadSlotRelease(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey);
ADI_OSAL_STATUS adi_osal_ThreadSlotSetValue(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey, ADI_OSAL_SLOT_VALUE slotValue);
ADI_OSAL_STATUS adi_osal_ThreadSlotGetValue(ADI_OSAL_TLS_SLOT_KEY nThreadSlotKey, ADI_OSAL_SLOT_VALUE *pSlotValue);


/*-------  Prototypes for semaphore APIs  -------*/

uint32_t        adi_osal_SemGetObjSize(void);
ADI_OSAL_STATUS adi_osal_SemCreateStatic(void* const pSemObject, uint32_t nSemObjSize, ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount);
ADI_OSAL_STATUS adi_osal_SemCreate(ADI_OSAL_SEM_HANDLE *phSem, uint32_t nInitCount);
ADI_OSAL_STATUS adi_osal_SemDestroy(ADI_OSAL_SEM_HANDLE const hSem);
ADI_OSAL_STATUS adi_osal_SemDestroyStatic(ADI_OSAL_SEM_HANDLE const hSem);
ADI_OSAL_STATUS adi_osal_SemPend(ADI_OSAL_SEM_HANDLE const hSem, ADI_OSAL_TICKS nTimeoutInTicks);
ADI_OSAL_STATUS adi_osal_SemPost(ADI_OSAL_SEM_HANDLE const hSem);


/*-------  Prototypes for mutex APIs  -------*/

ADI_OSAL_STATUS adi_osal_MutexCreate(ADI_OSAL_MUTEX_HANDLE *phMutex);
ADI_OSAL_STATUS adi_osal_MutexCreateStatic(void* const pMutexObject, uint32_t nMutexObjSize, ADI_OSAL_MUTEX_HANDLE *phMutex);
uint32_t        adi_osal_MutexGetObjSize(void);
ADI_OSAL_STATUS adi_osal_MutexDestroy(ADI_OSAL_MUTEX_HANDLE const hMutex);
ADI_OSAL_STATUS adi_osal_MutexDestroyStatic(ADI_OSAL_MUTEX_HANDLE const hMutex);
ADI_OSAL_STATUS adi_osal_MutexPend(ADI_OSAL_MUTEX_HANDLE const hMutex, ADI_OSAL_TICKS nTimeoutInTicks);
ADI_OSAL_STATUS adi_osal_MutexPost(ADI_OSAL_MUTEX_HANDLE const hMutex);


/*-------  Prototypes for message queue APIs  -------*/

ADI_OSAL_STATUS adi_osal_MsgQueueCreate(ADI_OSAL_QUEUE_HANDLE *phMsgQ, void* aMsgQ[],
                                        uint32_t nMaxMsgs);
ADI_OSAL_STATUS adi_osal_MsgQueueDestroy(ADI_OSAL_QUEUE_HANDLE const hMsgQ);
ADI_OSAL_STATUS adi_osal_MsgQueuePost(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void  *pMsg);
ADI_OSAL_STATUS adi_osal_MsgQueuePend(ADI_OSAL_QUEUE_HANDLE const hMsgQ, void **ppMsg,
                                      ADI_OSAL_TICKS nTimeoutInTicks);

/*-------  Prototypes for events APIs  -------*/

ADI_OSAL_STATUS adi_osal_EventGroupCreate(ADI_OSAL_EVENT_HANDLE *phEventGroup);
ADI_OSAL_STATUS adi_osal_EventGroupCreateStatic(void* const pEventObject, uint32_t nEventObjSize,ADI_OSAL_EVENT_HANDLE *phEventGroup);
ADI_OSAL_STATUS adi_osal_EventGroupDestroy(ADI_OSAL_EVENT_HANDLE const hEventGroup);
ADI_OSAL_STATUS adi_osal_EventGroupDestroyStatic(ADI_OSAL_EVENT_HANDLE const hEventGroup);
uint32_t        adi_osal_EventGroupGetObjSize(void);
ADI_OSAL_STATUS adi_osal_EventPend(ADI_OSAL_EVENT_HANDLE const hEventGroup,
                                   ADI_OSAL_EVENT_FLAGS nRequestedEvents,
                                   ADI_OSAL_EVENT_FLAG_OPTION eGetOption,
                                   ADI_OSAL_TICKS nTimeoutInTicks,
                                   ADI_OSAL_EVENT_FLAGS *pnReceivedEvents);
ADI_OSAL_STATUS adi_osal_EventSet(ADI_OSAL_EVENT_HANDLE const hEventGroup, ADI_OSAL_EVENT_FLAGS nEventFlags);
ADI_OSAL_STATUS adi_osal_EventClear(ADI_OSAL_EVENT_HANDLE const hEventGroup, ADI_OSAL_EVENT_FLAGS nEventFlags);

/*-------  Prototypes for Critical section code protection APIs  -------*/

ADI_OSAL_STATUS adi_osal_RTLGlobalsLock(void);
ADI_OSAL_STATUS adi_osal_RTLGlobalsUnlock(void);
void adi_osal_SchedulerLock(void);
ADI_OSAL_STATUS adi_osal_SchedulerUnlock(void);
void adi_osal_EnterCriticalRegion(void);
void adi_osal_ExitCriticalRegion(void);


/*-------  Prototypes for ISR control APIs  -------*/

typedef void(*ADI_OSAL_HANDLER_PTR)(uint32_t iid, void* handlerArg);

/*!
 *  @details Handler entry to store the handler and its application-supplied argument
 */
typedef struct _AdiOsalHandlerEntry
{
    ADI_OSAL_HANDLER_PTR    pfOSALHandler;
    void                    *pOSALArg;
} ADI_OSAL_HANDLER_ENTRY;

/*! @ingroup ADI_OSAL_Arch
 *  @{
 */
ADI_OSAL_STATUS  adi_osal_InstallHandler (uint32_t iid,
   ADI_OSAL_HANDLER_PTR highLevelHandler,
   void* handlerArg);
ADI_OSAL_STATUS    adi_osal_ActivateHandler (uint32_t iid);
ADI_OSAL_STATUS    adi_osal_DeactivateHandler (uint32_t iid);
ADI_OSAL_STATUS  adi_osal_UninstallHandler (uint32_t iid);
/*@}*/

/*-------  Prototypes for Profiling APIs  -------*/
ADI_OSAL_STATUS adi_osal_ThreadEnableCycleCounting(void);
ADI_OSAL_STATUS adi_osal_ThreadGetCycleCount(uint64_t *pnCycles);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#else
/*!
  @name ASM_MACROS
  @details assembly language specific macros and extern declarations.
         These functions may be called from an ISR (assembly)
  @{
 */

.extern _adi_osal_SemPost;
.extern _adi_osal_TimeTick;
.extern _adi_osal_MsgQueuePost;
.extern _adi_osal_EventSet;
.extern _adi_osal_EventClear;
/**@}*/

#endif  /* if !defined(__STDC__) */

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm088,Pm100,Pm114,Pm120,Pm123,Pm127,Pm128,Pm138,Pm139,Pm140,Pm141,Pm143")
#endif
/*! @endcond */
#endif /* __ADI_OSAL_FREERTOS_H__ */
