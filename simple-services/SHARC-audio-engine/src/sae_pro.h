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
 * @brief  The SHARC Audio Engine, PRO Interface
 *
 *   Lower-level "Professional" interface methods to the SHARC Audio Engine.
 *
 *   These methods should normally never be used.
 *
 * @file      sae_pro.h
 * @version   0.0.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/
#ifndef _sae_pro_h
#define _sae_pro_h

#include <stdint.h>

#include "sae.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************
 * @brief Returns the source core index of a message buffer.
 *
 * This function returns the source core index of a message buffer.
 * This index is not the same as a core's Core ID and is not
 * required to match
 *
 * This function is thread safe.  The caller is responsible for
 * insuring that 'msg' remains valid until the function returns.
 *
 * @param [in]  msg       A pointer to an SAE_MSG_BUFFER
 *
 * @return  Returns the source core index of the message.
 ******************************************************************/
SAE_CORE_IDX sae_getMsgBufferSrcCoreIdx(SAE_MSG_BUFFER *msg);

/*!****************************************************************
 * @brief Sets type of message.
 *
 * This function sets the type of message.  See sae_priv.h for
 * possible message types.
 *
 * This function is thread safe.  The caller is responsible for
 * insuring that 'msg' remains valid until the function returns.
 *
 * @param [in]  msg       A pointer to an SAE_MSG_BUFFER
 * @param [in]  type      The type of message
 *
 * @return  None
 ******************************************************************/
void sae_setMsgBufferType(SAE_MSG_BUFFER *msg, uint8_t type);

/*!****************************************************************
 * @brief Gets the current reference count of a message.
 *
 * This function safely gets the current reference count of a
 * message.
 *
 * This function is thread safe.  The caller is responsible for
 * insuring that 'msg' remains valid until the function returns.
 *
 * @param [in]  msg       A pointer to an SAE_MSG_BUFFER
 *
 * @return  None
 ******************************************************************/
uint8_t sae_getMsgBufferRefCount(SAE_MSG_BUFFER *msg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
