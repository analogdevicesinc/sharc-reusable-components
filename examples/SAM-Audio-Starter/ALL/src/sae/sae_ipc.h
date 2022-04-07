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

#ifndef _sharc_arm_ipc_h
#define _sharc_arm_ipc_h

#include <stdint.h>

#include "sae_cfg.h"
#include "sae_priv.h"

typedef struct _SAE_IPC_MSG_QUEUE {
    uint8_t head;
    uint8_t tail;
    uint8_t size;
    uint8_t align;
    void *queue[IPC_MAX_MSG_QUEUE_SIZE];
} SAE_IPC_MSG_QUEUE;

#pragma pack(1)
typedef struct _SAE_SHARC_ARM_IPC {
    uint32_t lock;
    int32_t idx2trigger[IPC_MAX_CORES];
    SAE_IPC_MSG_QUEUE msgQueues[IPC_MAX_CORES];
    SAE_STREAM *streamList;
    uint8_t heap[1];
} SAE_SHARC_ARM_IPC;
#pragma pack()

extern SAE_SHARC_ARM_IPC *saeSharcArmIPC;

#endif
