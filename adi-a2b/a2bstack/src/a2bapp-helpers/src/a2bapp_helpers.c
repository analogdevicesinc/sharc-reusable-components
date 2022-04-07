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

#include <stdlib.h>

#include "a2bapp_helpers_cfg.h"
#include "a2bapp_helpers.h"

#ifdef ADI_SIGMASTUDIO_BCF
#include "a2b_bdd_helper.h"
#endif

#ifndef A2B_DEFAULT_POLL_TIME
#define A2B_DEFAULT_POLL_TIME  5
#endif

#ifndef A2B_HELPERS_MALLOC
#define A2B_HELPERS_MALLOC    malloc
#endif

#ifndef A2B_HELPERS_CALLOC
#define A2B_HELPERS_CALLOC    calloc
#endif

#ifndef A2B_HELPERS_FREE
#define A2B_HELPERS_FREE      free
#endif

//#define A2B_HELPERS_SEQUENCE_CHART

static void a2b_stack_clear_discovery_status(a2b_helperContext *context)
{
    context->discoveryDone = A2B_FALSE;
    context->discoverySuccessful = A2B_FALSE;
    context->discoveryStatus = "";
    context->nodesDiscovered = -1;
}

static void a2b_stack_clear_fault_status(a2b_helperContext *context)
{
    context->faultDone = A2B_FALSE;
    context->faultSuccessful = A2B_FALSE;
    context->faultStatus = "";
    context->faultNode = -1;
}

static void a2b_stack_clear_ber_status(a2b_helperContext *context)
{
    if (context->berStatus) {
        A2B_HELPERS_FREE(context->berStatus);
    }
    context->berStatus = NULL;
}

static void a2b_stack_clear_status(a2b_helperContext *context)
{
    context->started = A2B_FALSE;
    context->networkLoaded = A2B_FALSE;
    a2b_stack_clear_discovery_status(context);
    a2b_stack_clear_fault_status(context);
    a2b_stack_clear_ber_status(context);
}

static void a2b_stack_onDiscoveryComplete(struct a2b_Msg *msg, a2b_Bool isCancelled)
{
    a2b_helperContext *context = (a2b_helperContext *)a2b_msgGetUserData(msg);
    a2b_NetDiscovery *results;

    if ( isCancelled ) {
        context->discoveryStatus = "Discovery request was cancelled.";
    } else {
        results = (a2b_NetDiscovery *)a2b_msgGetPayload(msg);
        if ( A2B_SUCCEEDED(results->resp.status) ) {
            context->nodesDiscovered = results->resp.numNodes;
            context->discoveryStatus = "Discovery succeeded.";
            context->discoverySuccessful = A2B_TRUE;
            context->berStatus = A2B_HELPERS_CALLOC(
                context->nodesDiscovered, sizeof(unsigned)
            );
        } else {
            context->discoveryStatus = "Discovery failed!";
            context->discoverySuccessful = A2B_FALSE;
        }
    }

    context->discoveryDone = A2B_TRUE;
}

static const char *a2b_stack_fault_to_string(a2b_UInt8 intrType)
{
   const char *faultString;

   switch (intrType) {
      case A2B_ENUM_INTTYPE_PWRERR_CS_GND:
         faultString = "Cable Shorted to GND";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_CS_VBAT:
         faultString = "Cable Shorted to VBat";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_CS:
         faultString = "Cable Shorted Together";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_CDISC:
         faultString = "Cable Disconnected or Open Circuit";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_CREV:
         faultString = "Cable Reverse Connected";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_FAULT:
         faultString = "Indeterminate Fault";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_NLS_GND:
         faultString = "Non-Localized Short to GND";
         break;
      case A2B_ENUM_INTTYPE_PWRERR_NLS_VBAT:
         faultString = "Non-Localized Short to VBat";
         break;
      default:
         faultString = "Unknown";
         break;
   }
   return(faultString);
}

static void a2b_stack_onDiscovery_PowerFault(struct a2b_Msg *msg, a2b_Handle userData)
{
    a2b_helperContext *context = (a2b_helperContext *)userData;
    a2b_PowerFault *fault;

    fault = (a2b_PowerFault *)a2b_msgGetPayload(msg);
    if ( fault ) {
        if ( A2B_SUCCEEDED(fault->status) ) {
            context->faultSuccessful = A2B_TRUE;
            context->faultStatus = a2b_stack_fault_to_string(fault->intrType);
            context->faultNode = fault->faultNode + 1;
        } else {
            context->faultSuccessful = A2B_FALSE;
        }
    } else {
        context->faultSuccessful = A2B_FALSE;
    }

    context->faultDone = A2B_TRUE;
}

a2b_Bool a2b_stack_discover_start(a2b_helperContext *context)
{
    a2b_NetDiscovery *discReq;
    struct a2b_Msg *msg;
    a2b_HResult result;

    if (context->started == A2B_FALSE) {
        A2B_HELPERS_SYSLOG_PRINT("A2B network not started!");
        return A2B_FALSE;
    }

    /* Reset the discover status */
    a2b_stack_clear_discovery_status(context);

    /* Reset the fault status */
    a2b_stack_clear_fault_status(context);

    /* Reset the bit error rate status */
    a2b_stack_clear_ber_status(context);

    /* Create a network discovery request message */
    msg = a2b_msgAlloc(context->ctx, A2B_MSG_REQUEST, A2B_MSGREQ_NET_DISCOVERY);

    /* Attach the BDD information to the message */
    discReq = (a2b_NetDiscovery*)a2b_msgGetPayload(msg);
    discReq->req.bdd = &context->bdd;
    discReq->req.periphPkg = context->periphPkg;
    discReq->req.pkgLen = context->periphPkgLen;
    a2b_msgSetUserData(msg, (a2b_Handle)context, A2B_NULL);

    /* Send the discover message to the Stack */
    result = a2b_msgRtrSendRequest(msg, A2B_NODEADDR_MASTER,
        a2b_stack_onDiscoveryComplete);

    /* The message router adds it's own reference to the submitted message. */
    a2b_msgUnref(msg);

    if (A2B_FAILED(result)) {
        A2B_HELPERS_SYSLOG_PRINT("Could not start discovery!");
        return false;
    }

    return(true);
}

a2b_Bool a2b_stack_discover(a2b_helperContext *context)
{
    a2b_Bool ret;

    if (context->started == A2B_FALSE) {
        A2B_HELPERS_SYSLOG_PRINT("A2B network not started!");
        return A2B_FALSE;
    }

    ret = a2b_stack_discover_start(context);
    if (ret) {
        while (context->discoveryDone == A2B_FALSE) {
            a2b_stackTick( context->ctx );
        }
    }

    return(ret);
}

a2b_Bool a2b_stack_discover_complete(a2b_helperContext *context)
{
    if (context->discoveryDone == A2B_FALSE) {
        a2b_stackTick( context->ctx );
    }
    return(context->discoveryDone);
}

a2b_Bool a2b_stack_start(a2b_helperContext *context, char *seqFile)
{
    a2b_HResult result;

    if (context->networkLoaded == A2B_FALSE) {
        A2B_HELPERS_SYSLOG_PRINT("A2B network not loaded!");
        return A2B_FALSE;
    }

    a2b_bddPalInit(&context->ecb, &context->bdd);

    context->ecb.baseEcb.heap = A2B_HELPERS_MALLOC(context->ecb.baseEcb.heapSize);
    context->ctx = a2b_stackAlloc(&context->pal, &context->ecb);

    if (context->ctx == A2B_NULL) {
        A2B_HELPERS_SYSLOG_PRINT("Could not allocate A2B stack context!");
        return A2B_FALSE;
    }

    result = a2b_intrStartIrqPoll( context->ctx, context->pollTime );
    if (A2B_FAILED(result)) {
        A2B_HELPERS_SYSLOG_PRINT("Failed to start IRQ polling!");
        return A2B_FALSE;
    }

#if defined(A2B_FEATURE_SEQ_CHART) && defined(A2B_HELPERS_SEQUENCE_CHART)
    /* Enable sequence charts */
    if (seqFile) {
        a2b_seqChartStart(context->ctx, seqFile,
        A2B_SEQ_CHART_LEVEL_I2C | A2B_SEQ_CHART_LEVEL_MSGS,
        A2B_SEQ_CHART_OPT_ALL, "Sequence Chart");
    }
#endif

    /* Register for notifications on power faults */
    context->notifyPowerFault = a2b_msgRtrRegisterNotify(context->ctx,
                                            A2B_MSGNOTIFY_POWER_FAULT,
                                            a2b_stack_onDiscovery_PowerFault,
                                            context,
                                            A2B_NULL);

   context->started = A2B_TRUE;

   return A2B_TRUE;
}


a2b_Bool a2b_stack_stop(a2b_helperContext *context)
{
    if (context->started == A2B_TRUE) {
        a2b_msgRtrUnregisterNotify(context->notifyPowerFault);
        a2b_intrStopIrqPoll(context->ctx);
#if defined(A2B_FEATURE_SEQ_CHART) && defined(A2B_HELPERS_SEQUENCE_CHART)
        a2b_seqChartStop(context->ctx);
#endif
        a2b_stackFree(context->ctx);
        if (context->ecb.baseEcb.heap != NULL) {
            A2B_HELPERS_FREE(context->ecb.baseEcb.heap);
        }
        a2b_systemShutdown(A2B_NULL, &context->ecb);
        a2b_stack_clear_status(context);
    }

    return A2B_TRUE;
}


int a2b_stack_loadBdd(a2b_helperContext *context)
{
    a2b_Bool ok;

    /* Default to OK */
    ok = A2B_TRUE;

    /* Decode the config and store it into the bdd struct. */
    if (context->network && context->networkLen) {
        if ( !a2b_bddDecode(context->network, context->networkLen, &context->bdd) ) {
            ok = A2B_FALSE;
        }
#ifdef ADI_SIGMASTUDIO_BCF
    } else if (context->sBusDescription) {
        a2b_bcfParse_bdd(context->sBusDescription, &context->bdd, 0);
#endif
    } else {
        ok = A2B_FALSE;
    }

    return(ok);
}

a2b_Bool a2b_stack_load(a2b_helperContext *context)
{
    a2b_Bool bddOk;

    if (context->networkLoaded == A2B_TRUE) {
        return(A2B_TRUE);
    }

    bddOk = a2b_stack_loadBdd(context);

    if (bddOk == A2B_FALSE) {
        A2B_HELPERS_SYSLOG_PRINT("Error loading the network configuration!");
        return A2B_FALSE;
    }

    context->networkLoaded = A2B_TRUE;

   return A2B_TRUE;
}

a2b_Bool a2b_stack_tick(a2b_helperContext *context)
{
    a2b_stackTick( context->ctx );
    return A2B_TRUE;
}

a2b_Bool a2b_stack_check_network(a2b_helperContext *context)
{
    a2b_HResult result;
    char wBuf[1];
    char rBuf[1];
    a2b_Bool ok;

    if (!context->discoverySuccessful) {
        return(A2B_FALSE);
    }

    wBuf[0] = A2B_REG_VENDOR;

    result = a2b_i2cSlaveWriteRead(
        context->ctx, context->nodesDiscovered - 1,
        sizeof(wBuf), wBuf,
        sizeof(rBuf), rBuf
    );

    ok = A2B_SUCCEEDED(result) && (rBuf[0] == 0xAD);

    return(ok);
}

a2b_Bool a2b_stack_check_ber(a2b_helperContext *context)
{
    a2b_HResult result;
    char wBuf[3];
    char rBuf[2];
    a2b_Bool ok;
    int node;

    if (!context->discoverySuccessful) {
        return(A2B_FALSE);
    }

    /* A2B_REG_BECCTL is at address 0x1E and A2B_REG_BECNT is immediately
     * behind it at address 0x1F.  If BECCTL is at its default state of
     * 0x00, then set it to all bit errors enabled with an interrupt
     * at 256 errors (0xFF).  If it's not 0x00, then do not change it
     * assuming that it was configured to monitor a specific set of
     * bit errors.
     */
    wBuf[0] = A2B_REG_BECCTL;
    wBuf[1] = 0x00;
    wBuf[2] = 0x00;
    node = 0;
    ok = A2B_TRUE;

    while ((node < context->nodesDiscovered) && ok) {
        /* Read the bit error registers */
        result = a2b_i2cSlaveWriteRead(
            context->ctx, node, 1, wBuf, 2, rBuf
        );
        ok = A2B_SUCCEEDED(result);
        if (ok) {
            /* Enable all BERs if BECCTL has not been set */
            if (rBuf[0] == A2B_REG_BECCTL_RESET) {
                wBuf[1] = A2B_ENUM_BECCTL_THRESHLD_256 |
                    A2B_ENUM_BECCTL_ENICRC_EN |
                    A2B_ENUM_BECCTL_ENDP_EN |
                    A2B_ENUM_BECCTL_ENCRC_EN|
                    A2B_ENUM_BECCTL_ENDD_EN |
                    A2B_ENUM_BECCTL_ENHDCNT_EN;
            } else {
                wBuf[1] = rBuf[0];
            }
            /* Accumulate bit errors from rBuf[0] if any are enabled */
            if ((rBuf[0] & ~(A2B_BITM_BECCTL_THRESHLD)) && context->berStatus) {
                context->berStatus[node] += (unsigned char)rBuf[1];
            }
            /* Clear slave errors */
            result = a2b_i2cSlaveWrite(context->ctx, node, 3, wBuf);
            ok = A2B_SUCCEEDED(result);
        }
        node++;
    }

    return(ok);
}

a2b_Bool a2b_stack_clear_ber(a2b_helperContext *context)
{
    int node;

    if (!context->discoverySuccessful) {
        return(A2B_FALSE);
    }

    for (node = 0; node < context->nodesDiscovered; node++) {
        context->berStatus[node] = 0;
    }

    return(A2B_TRUE);
}

a2b_Bool a2b_stack_gen_ber(a2b_helperContext *context, int node, char error)
{
    a2b_HResult result;
    char wBuf[2];
    a2b_Bool ok;

    if (!context->discoverySuccessful || (node >= context->nodesDiscovered) ) {
        return(A2B_FALSE);
    }

    wBuf[0] = A2B_REG_GENERR;
    wBuf[1] = error & (
            A2B_BITM_GENERR_GENICRCERR |
            A2B_BITM_GENERR_GENDPERR |
            A2B_BITM_GENERR_GENCRCERR |
            A2B_BITM_GENERR_GENDDERR |
            A2B_BITM_GENERR_GENHCERR
        );

    if (node < 0) {
        result = a2b_i2cMasterWrite(context->ctx, 2, wBuf);
    } else {
        result = a2b_i2cSlaveWrite(context->ctx, node, 2, wBuf);
    }
    ok = A2B_SUCCEEDED(result);

    return(ok);
}


a2b_Bool a2b_stack_init(a2b_helperContext *context)
{
    memset(context, 0, sizeof(*context));

    /* Initialize the A2B Stack */
    a2b_systemInitialize(A2B_NULL, &context->ecb);

    /* Initialize the A2B Stack SAM board PAL */
    a2b_palInit(&context->pal, &context->ecb);

    /* Add the app context to the PAL ecb */
    context->ecb.palEcb.usrPtr = (void *)context;

    /* Init some state variables */
    context->pollTime = A2B_DEFAULT_POLL_TIME;

    return A2B_TRUE;
}
