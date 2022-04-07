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

#ifndef _a2b_stack_h
#define _a2b_stack_h

#include "a2b/a2b.h"
#include "a2b_bdd_helper.h"

#ifdef ADI_SIGMASTUDIO_BCF
#include "adi_a2b_busconfig.h"
#endif

typedef struct a2b_helperContext a2b_helperContext;

struct a2b_helperContext {

    /* Application allocated TWI device handle */
    void *twiHandle;

    /* Stack related context containers */
    A2B_ECB ecb;
    a2b_StackPal pal;
    struct a2b_StackContext *ctx;

    /* Network container */
    bdd_Network bdd;

#ifdef ADI_SIGMASTUDIO_BCF
    /* ADI Network BCF struct */
    ADI_A2B_BCD *sBusDescription;
#endif
    /* Mentor Network "BDD" binary buffer */
    const a2b_Byte *network;
    a2b_UInt32 networkLen;

    /* Network slave I2C peripheral init package */
    const a2b_Byte *periphPkg;
    a2b_UInt32 periphPkgLen;

    /* Discovery state variables */
    int discoveryDone;
    int discoverySuccessful;
    const char *discoveryStatus;
    int nodesDiscovered;

    /* Line fault state variables */
    int faultDone;
    int faultSuccessful;
    const char *faultStatus;
    int faultNode;
    struct a2b_MsgNotifier *notifyPowerFault;

    /* Bit error rate monitoring variables.  If 'berStatus' is not NULL,
     * it is an array of size 'nodesDiscovered'
     */
    unsigned *berStatus;

    /* Misc state variables */
    int networkLoaded;
    int pollTime;
    int started;

};

/*
 * a2b_stack_discover()
 *    Initiates a discovery and polls until complete.
 *
 * a2b_stack_discover_start()
 *    Initiates a discovery and returns
 *
 * a2b_stack_discover_complete()
 *    Ticks the stack and returns the discovery complete status
 */

a2b_Bool a2b_stack_init(a2b_helperContext *context);
a2b_Bool a2b_stack_load(a2b_helperContext *context);
a2b_Bool a2b_stack_start(a2b_helperContext *context, char *seqFile);
a2b_Bool a2b_stack_discover(a2b_helperContext *context);
a2b_Bool a2b_stack_tick(a2b_helperContext *context);
a2b_Bool a2b_stack_stop(a2b_helperContext *context);
a2b_Bool a2b_stack_check_network(a2b_helperContext *context);
a2b_Bool a2b_stack_check_ber(a2b_helperContext *context);
a2b_Bool a2b_stack_discover_start(a2b_helperContext *context);
a2b_Bool a2b_stack_discover_complete(a2b_helperContext *context);
a2b_Bool a2b_stack_clear_ber(a2b_helperContext *context);
a2b_Bool a2b_stack_gen_ber(a2b_helperContext *context, int node, char error);

#endif
