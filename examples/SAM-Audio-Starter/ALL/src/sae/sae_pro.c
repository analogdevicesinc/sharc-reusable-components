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

/* Module includes */
#include "sae_priv.h"
#include "sae_util.h"

SAE_CORE_IDX sae_getMsgBufferSrcCoreIdx(SAE_MSG_BUFFER *msg)
{
    return((SAE_CORE_IDX)msg->srcCoreIdx);
}

void sae_setMsgBufferType(SAE_MSG_BUFFER *msg, uint8_t type)
{
    msg->msgType = type;
}

uint8_t sae_getMsgBufferRefCount(SAE_MSG_BUFFER *msg)
{
    uint8_t ref;

    /* Get an exclusive lock on the IPC area */
    sae_lockIpc();

    ref = msg->ref;

    /* Unlock the IPC area */
    sae_unLockIpc();

    return(ref);
}
