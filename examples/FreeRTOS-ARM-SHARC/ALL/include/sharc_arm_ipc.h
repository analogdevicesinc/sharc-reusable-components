#ifndef _sharc_arm_ipc_h
#define _sharc_arm_ipc_h

#include <stdint.h>

typedef struct _SHARC_ARM_IPC {
    uint32_t armInitComplete;
} SHARC_ARM_IPC;

extern SHARC_ARM_IPC sharcArmIPC;

#endif
