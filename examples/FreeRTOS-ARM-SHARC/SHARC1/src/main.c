/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#include "sharc_arm_ipc.h"

/* SHARC / ARM IPC struct for communication */
SHARC_ARM_IPC sharcArmIPC __attribute__ ((section("seg_l2_uncached")));

int main(int argc, char **argv)
{
    /* Keep from constantly hitting the automatic main() breakpoint in
     * the while() loop below */
    asm("nop;");

    /* Wait for the ARM core to complete the system init */
    while (sharcArmIPC.armInitComplete == 0);

    while (1) {
        asm("nop;");
    }
}
