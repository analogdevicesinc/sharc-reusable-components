/*=============================================================================
 *
 * Project: a2bstack
 *
 * Copyright (c) 2015 - Analog Devices Inc. All Rights Reserved.
 * This software is subject to the terms and conditions of the license set 
 * forth in the project LICENSE file. Downloading, reproducing, distributing or 
 * otherwise using the software constitutes acceptance of the license. The 
 * software may not be used except as expressly authorized under the license.
 *
 *=============================================================================
 *
 * \file:   ecb.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Basic definitions for the stack environment control block (ECB).
 *
 *=============================================================================
 */

/** 
 * \defgroup a2bstack_ecb           Environment Control Block (ECB) Definitions
 *  
 * Basic definitions for the stack environment control block (ECB).
 *
 * \{
 */

#ifndef A2B_ECB_H_
#define A2B_ECB_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/palecb.h"
#include "a2b/defs.h"
#include "a2b/i2c.h"

/*======================= D E F I N E S ===========================*/

#ifndef A2B_ECB
#ifndef A2B_CUSTOM_ECB
#define A2B_ECB  struct a2b_Ecb
#else
#define A2B_ECB  A2B_CUSTOM_ECB
#endif
#endif


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/**
 * Base environment properties that are considered part of the generic
 * A2B stack.
 */
typedef struct a2b_BaseEcb
{
    /** A URL indicating where traces should be delivered. This can
     * be tailored and interpreted by the platform implementation of
     * the "generic" stack.
     */
    const a2b_Char* traceUrl;

    /** A pointer to memory where the A2B stack can manage it's
     * heap. This may be A2B_NULL if the application implements it's own
     * memory management routines (perhaps relying on malloc and friends).
     * Likewise, it could be set by the application to indicate where the
     * heap actually begins. This heap pointer is passed into the PAL
     * #a2b_MemMgrOpenFunc() function.
     */
    a2b_Byte*     heap;

    /** The size of the A2B stack heap (in bytes). Again this may be
     * ignored if the application manages it's own heap. It will be
     * passed into the PAL #a2b_MemMgrOpenFunc(). The application could set
     * this as a "hint" for a custom application heap.
     */
    a2b_UInt32    heapSize;

#ifdef A2B_FEATURE_TRACE
    /** This is the default trace level used while initializing
     *  the A2B stack.  This makes it possible to either view or
     *  suppress tracing during #a2b_stackAlloc().  This level is:
     *  A2B_TRC_DOM_* | A2B_TRC_LVL_*.
     */
    a2b_UInt32      traceLvl;
#endif

    /** The I2C address of the A2B master node */
    a2b_UInt16      i2cMasterAddr;

    /** The I2C address format (7 or 10 bit address) */
    a2b_I2cAddrFmt  i2cAddrFmt;

    /** The I2C bus speed (100KHz or 400KHz) */
    a2b_I2cBusSpeed i2cBusSpeed;

    /** Master nodes BDD node info passed to the
     *  master init/open calls for validation.
     */
    a2b_NodeInfo    masterNodeInfo;

} a2b_BaseEcb;


/** 
 * Environmental Control Block (ECB)
 *  
 * \attention  The #a2b_BaseEcb and #a2b_PalEcb **MUST** be the first 
 * two fields of the #a2b_Ecb structure in case the client wishes to 
 * extend and create a custom ECB.
 */
typedef struct a2b_Ecb
{
    /** The basic environmental parameters irrespective of the target platform.
     */
    a2b_BaseEcb     baseEcb;

    /** These are the platform specific environmental properties to be defined
     * by the platform implementation. The non-platform specific stack has no
     * such properties.
     */
    a2b_PalEcb      palEcb;

} a2b_Ecb;

/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_ecb */

#endif /* A2B_ECB_H_ */
