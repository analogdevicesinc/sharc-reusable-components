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

#ifndef _sae_cfg_h
#define _sae_cfg_h

/*
 * Overrides for standard C functions
 */
#include <string.h>
#define SAE_MEMSET  memset
#define SAE_MEMCPY  memcpy
#define SAE_STRLEN  strlen
#define SAE_STRCPY  strcpy

/*
 * WARNING: This module utilizes the entire memory space reserved
 *          in ADI's default LD/LDR files for MCAPI which means it
 *          CANNOT be used together with the MCAPI add-in.
 */

/* Set to the number of SAE participating cores.  This is not the same
 * as the number of cores on the SoC.
 */
#define IPC_MAX_CORES            (3)

/* This sets the maximum number of outstanding messages that
 * can be queued up on any particular core.  This number must be a
 * power of 2.
 */
#define IPC_MAX_MSG_QUEUE_SIZE   (16)

/* Minimal ARM and SHARC global interrupt disable/enable. */
#if defined (__ADSPARM__)
    #define SAE_ENTER_CRITICAL()  __builtin_disable_interrupts()
    #define SAE_EXIT_CRITICAL()   __builtin_enable_interrupts()
#elif defined(__ADSP21000__)
    #include "interrupt.h"
    #define SAE_ENTER_CRITICAL()  adi_rtl_disable_interrupts()
    #define SAE_EXIT_CRITICAL()   adi_rtl_reenable_interrupts()
#else
    #define SAE_ENTER_CRITICAL()
    #define SAE_EXIT_CRITICAL()
#endif

#endif
