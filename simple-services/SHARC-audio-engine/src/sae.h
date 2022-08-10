/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

/*!
 * @brief  The SHARC Audio Engine, a Simple, efficient, multi-core
 *         Inter Processor Communication (IPC) and Audio Transfer Engine
 *
 *   The SHARC Audio Engine (SAE) supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Highly efficient core-to-core IPC
 *     - Fully protected multi-threaded IPC
 *     - Zero copy core-to-core DMA possible
 *
 * @file      sae.h
 * @version   0.0.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/
#ifndef _sae_h
#define _sae_h

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sae_cfg.h"

/* Notes:
 *   1) Must build SHARC+ project with -char-size-8 to insure endian
 *      compatability with ARM core.
 */

/*!****************************************************************
 * @brief Opaque SHARC Audio Engine context object
 ******************************************************************/
typedef struct _SAE_CONTEXT SAE_CONTEXT;

/*!****************************************************************
 * @brief Opaque SHARC Audio Engine message buffer object
 ******************************************************************/
typedef struct _SAE_MSG_BUFFER SAE_MSG_BUFFER;

/*!****************************************************************
 * @brief Opaque SHARC Audio Engine message buffer object
 ******************************************************************/
typedef struct _SAE_HEAP_INFO {
    unsigned int totalBlocks;
    unsigned int allocBlocks;
    unsigned int freeBlocks;
    size_t allocSize;
    size_t freeSize;
    size_t maxContigFreeSize;
} SAE_HEAP_INFO;

/*!****************************************************************
 * @brief SHARC Audio Engine result codes
 ******************************************************************/
typedef enum _SAE_RESULT {
    SAE_RESULT_UNKNOWN = 0,     /**< Unknown/Invalid error */
    SAE_RESULT_OK,              /**< No error */
    SAE_RESULT_ERROR,           /**< Generic error */
    SAE_RESULT_NO_MEM,          /**< Memory allocation failure */
    SAE_RESULT_QUEUE_FULL,      /**< Destination core work queue full */
    SAE_RESULT_QUEUE_EMPTY,     /**< Source core work queue empty */
    SAE_RESULT_CORE_NOT_READY,  /**< Destination core not initialized */
    SAE_RESULT_STREAM_EXISTS,   /**< Stream exists */
    SAE_RESULT_BAD_STREAM_NAME, /**< No stream name given */
    SAE_RESULT_BAD_STREAM,      /**< Bad STREAM object given */
    SAE_RESULT_REFERENCE_ERROR, /**< Reference / unreference error */
    SAE_RESULT_CORRUPT_HEAP     /**< Corrupt heap found during check */
} SAE_RESULT;

/*!****************************************************************
 * @brief SHARC Audio Engine events
 ******************************************************************/
typedef enum _SAE_EVENT {
    SAE_EVENT_NONE = 0,         /**< NOP event */
    SAE_EVENT_ADD_STREAM,       /**< New stream added */
    SAE_EVENT_REMOVE_STREAM     /**< Existing stream removed */
} SAE_EVENT;

/*!****************************************************************
 * @brief SAE core indexes
 *
 * These indexes are sequentially assigned during initialization
 * to cores utilizing the SAE. This index is not the same as a
 * core's Core ID and is not required to match.
 ******************************************************************/
typedef enum _SAE_CORE_IDX {
    SAE_CORE_IDX_NONE = -1,     /**< Invalid core index */
    SAE_CORE_IDX_0 = 0,         /**< Core index 0 */
    SAE_CORE_IDX_1,             /**< Core index 1 */
    SAE_CORE_IDX_2              /**< Core index 2 */
} SAE_CORE_IDX;


#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************
 * @brief SHARC Audio Engine stream event notification callback
 ******************************************************************/
typedef void (*SAE_EVENT_CALLBACK)(SAE_CONTEXT *context, SAE_EVENT event,
    void *eventInfo, void *usrPtr);

/*!****************************************************************
 * @brief SHARC Audio Engine message received callback
 ******************************************************************/
typedef void (*SAE_MSG_RECEIVED_CALLBACK)(SAE_CONTEXT *context,
    SAE_MSG_BUFFER *msg, void *payload, void *usrPtr);

/*!****************************************************************
 * @brief SHARC Audio Engine initialization
 *
 * This function allocates and initializes the SHARC Audio Engine
 * on a core.
 *
 * This function must be called by each participating core.  Each
 * core must be assigned a sequential core index. One core must be
 * delegated as the master core.
 *
 * The master core is responsible for global SoC wide initialization.
 * The master core must run this function to completion before the
 * initialization of any other core.
 *
 * This function is not thread safe.
 *
 * @param [out] context    A pointer to an SAE context pointer
 * @param [in]  saeIdx     Index assigned by the application to this core
 * @param [in]  saeMaster  Set to true only on the SAE master core
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_initialize(SAE_CONTEXT **context, SAE_CORE_IDX saeIdx,
    bool saeMaster);


/*!****************************************************************
 * @brief SHARC Audio Engine uninitialization
 *
 * This function frees the SHARC Audio Engine on a core.
 *
 * This function will uninitialize the SAE context pointed to
 * by the 'context' parameter and set the context pointer
 * to NULL.
 *
 * This function is not thread safe.
 *
 * @param [in]  contextPtr    A pointer to an SAE context pointer
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_unInitialize(SAE_CONTEXT **contextPtr);


/*!****************************************************************
 * @brief Check the status of the SAE heap
 *
 * This function gathers statistics about the SAE heap.  It can 
 * take some time to execute and may result in excessive latency 
 * in time critical systems.  Use with caution.
 *
 * This function is thread safe.
 *
 * @param [in]  context   A pointer to an SAE context
 * @param [in]  heapInfo  A pointer to a SAE_HEAP_INFO struct
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_heapInfo(SAE_CONTEXT *context, SAE_HEAP_INFO *heapInfo);


/*!****************************************************************
 * @brief Create a new message buffer
 *
 * This function allocates and initializes a new message buffer of
 * the given size.  The returned message buffer is pre-initialized with
 * a reference count of 1.
 *
 * This function is thread safe.
 *
 * @param [in]  context    A pointer to an SAE context
 * @param [in]  size       The size of the buffer payload to allocate
 * @param [out] payload    Returns a pointer to the payload area.  Can be
 *                         NULL if no return value is needed.
 *
 * @return Returns NULL if error, otherwise a pointer to the newly
 *         created SAE_MSG_BUFFER.
 ******************************************************************/
SAE_MSG_BUFFER *sae_createMsgBuffer(SAE_CONTEXT *context, size_t size,
    void **payload);

/*!****************************************************************
 * @brief Gets the size of a message buffer in bytes.
 *
 * This function returns the size of a message buffer in bytes.
 *
 * This function is thread safe.  The caller is responsible for
 * insuring that 'msg' remains valid until the function returns.
 *
 * @param [in]  msg       A pointer to an SAE_MSG_BUFFER
 *
 * @return  Returns the size of the message in bytes.
 ******************************************************************/
size_t sae_getMsgBufferSize(SAE_MSG_BUFFER *msg);

/*!****************************************************************
 * @brief Gets a pointer to the payload of a message buffer.
 *
 * This function returns a pointer to the payload of a message buffer.
 *
 * This function is thread safe.  The caller is responsible for
 * insuring that 'msg' remains valid until the function returns.
 *
 * @param [in]  msg       A pointer to an SAE_MSG_BUFFER
 *
 * @return  Returns a pointer to the payload.
 ******************************************************************/
void *sae_getMsgBufferPayload(SAE_MSG_BUFFER *msg);

/*!****************************************************************
 * @brief Increment a message buffer's reference count
 *
 * This function increments the reference count of a message buffer.
 *
 * This function is thread safe.
 *
 * @param [in]  context   A pointer to an SAE context
 * @param [in]  msg       A pointer to a SAE_MSG_BUFFER
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_refMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg);


/*!****************************************************************
 * @brief Decrements a message buffer's reference count
 *
 * This function decrements the reference count of a message buffer.
 * When the reference count reaches zero, the buffer, and it's
 * contents, are automatically freed.
 *
 * This function is thread safe.
 *
 * @param [in]  context   A pointer to an SAE context
 * @param [in]  msg       A pointer to a SAE_MSG_BUFFER
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_unRefMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg);

/*!****************************************************************
 * @brief Removes a message buffer from the core's message queue.
 *
 * This function returns the next message available in the
 * core's message queue.
 *
 * This function is thread safe and generally called from
 * a loop within the call-back registered in
 * sae_registerMsgReceivedCallback().
 *
 * @param [in]  context   A pointer to an SAE context
 * @param [in]  msg       A pointer to a SAE_MSG_BUFFER pointer
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_receiveMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER **msg);

/*!****************************************************************
 * @brief Sends a message to a core.
 *
 * This function appends the given message to the destination
 * core's message queue.  Multiple messages can be queued prior
 * to signalling the destination core by setting signalDstCore
 * to false.
 *
 * This function is thread safe.
 *
 * @param [in]  context        Pointer to an SAE context
 * @param [in]  msg            Pointer to a SAE_MSG_BUFFER pointer
 * @param [in]  dstCoreIdx     Destination core's index
 * @param [in]  signalDstCore  Signal the destination core
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_sendMsgBuffer(SAE_CONTEXT *context, SAE_MSG_BUFFER *msg,
    uint8_t dstCoreIdx, bool signalDstCore);

/*!****************************************************************
 * @brief Registers a callback to be called when a message is placed
 *        on a core's message queue.
 *
 * This function registers a callback to be called when a message
 * is placed on a core's message queue.
 *
 * This function is thread safe.
 *
 * @param [in]  context       Pointer to an SAE context
 * @param [in]  cb            Pointer to a message received callback
 *                            function
 * @param [in]  usrPtr        User pointer returned to the callback
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_registerMsgReceivedCallback(SAE_CONTEXT *context,
    SAE_MSG_RECEIVED_CALLBACK cb, void *usrPtr);

/*!****************************************************************
 * @brief Registers a callback to handle SAE events
 *
 * This function registers a callback to handle SAE events
 *
 * This function is thread safe.
 *
 * @param [in]  context       Pointer to an SAE context
 * @param [in]  cb            Pointer to a message received callback
 *                            function
 * @param [in]  usrPtr        User pointer returned to the callback
 *
 * @return Returns SAE_RESULT_OK if successful, otherwise
 *         an error.
 ******************************************************************/
SAE_RESULT sae_registerEventCallback(SAE_CONTEXT *context,
    SAE_EVENT_CALLBACK cb, void *usrPtr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
