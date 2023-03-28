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
 * \file:   pwrdiag.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the API for conducting power diagnostics.
 *
 *=============================================================================
 */

/*! \addtogroup Network_Configuration
 *  @{
 */

/*! \addtogroup Power_Diagnostics
 *  @{
 */
#ifndef A2B_PWRDIAG_H_
#define A2B_PWRDIAG_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Plugin;


typedef enum {
    /** State of the power diagnosis is unknown */
    A2B_PWR_DIAG_STATE_UNKNOWN = 0u,

    /** The power diagnosis context has been initialized. */
    A2B_PWR_DIAG_STATE_INIT,

    /** The power diagnosis is in the process of executing the localization
     * of concealed faults.
     */
    A2B_PWR_DIAG_STATE_IN_PROGRESS,

    /** The power diagnosis has completed either successfully or with
     * an error.
     */
    A2B_PWR_DIAG_STATE_COMPLETE
} a2b_PwrDiagState;


typedef struct a2b_PwrDiagResults
{
    /**
     * These fields are used to store the results of a power diagnostic
     */

    /** Whether or not the power diagnosis was successful (or not) */
    a2b_HResult                 diagResult;

    /** Which node is causing the power fault */
    a2b_Int16                   faultNode;

    /** As per A2B_ENUM_INTTYPE_PWRERR_X values defined in regdefs.h */
    a2b_Int32                   intrType;

} a2b_PwrDiagResults;

typedef struct a2b_PwrDiagCtx
{
    a2b_PwrDiagState            state;

    /** State information needed for localization of concealed faults */
    a2b_Int16                   nextNode;   /* e.g. 'n' in the diag flow */
    a2b_Int16                   curNode;    /* e.g. 'Node' in the diag flow */
    a2b_Int16                   goodNode;
    a2b_Bool                    priorFault;
    a2b_Bool                    discComplete;
    a2b_Bool                    hasFault;

    /** Results of power diagnosis are stored here */
    a2b_PwrDiagResults          results;
} a2b_PwrDiagCtx;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT void a2b_pwrDiagInit(struct a2b_Plugin* plugin);
A2B_EXPORT a2b_HResult a2b_pwrDiagStart(struct a2b_Plugin* plugin,
                                            a2b_UInt8 intrSrc,
                                            a2b_UInt8 intrType);

A2B_EXPORT void a2b_pwrDiagDiagnose(struct a2b_Plugin* plugin,
                                            a2b_UInt8 intrSrc,
                                            a2b_UInt8 intrType);
A2B_EXPORT a2b_Bool a2b_pwrDiagIsActive(struct a2b_Plugin* plugin);

A2B_END_DECLS

/*======================= D A T A =================================*/
/**
 @}
*/


/**
 @}
*/

#endif /* A2B_PWRDIAG_H_ */
