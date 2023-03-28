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
 * \file:   system.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the system-level API.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/system.h"
#include "a2b/error.h"
#include "a2b/util.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/** Flag used to track whether or not the system has been initialized */
static a2b_Bool gSystemInitialized = A2B_FALSE;

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_systemInitialize
*
*  Executes platform agnostic initialization. If a platform specific
*  initializer is provided then this function will be called as well. This
*  function should *only* be called once on a given platform. It must be
*  called *before* any other A2B stack function.
*
*  \param          [in]    f        A platform specific initialization function.
*                                   If A2B_NULL then such a function will not
*                                   be called.
* 
*  \param          [in]    ecb      Pointer to the platform environment control 
*                                   block.  Treated opaquely.
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns status indicating whether or not initialization
*                  succeeded or failed. The #A2B_SUCCEEDED() and #A2B_FAILED()
*                  macros can be used to test for success/failure respectively.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_systemInitialize
    (
    a2b_PlatformInitFunc    f,
    A2B_ECB*                ecb
    )
{
    a2b_HResult status = A2B_RESULT_SUCCESS;

    if ( !gSystemInitialized )
    {
        if ( A2B_NULL != f )
        {
            status = f(ecb);
        }

        if ( A2B_SUCCEEDED(status) )
        {
            gSystemInitialized =  A2B_TRUE;
        }
    }

    return status;

} /* a2b_systemInitialize */


/*!****************************************************************************
*
*  \b              a2b_systemShutdown
*
*  Executes platform agnostic shutdown logic. After this is called no
*  other A2B stack functions should be called. If a platform specific
*  shutdown function is provided it will be called before any final
*  platform generic shutdown logic.
*
*  \param          [in]    f        A platform specific shutdown function.
*                                   If A2B_NULL then such a function will not
*                                   be called.
* 
*  \param          [in]    ecb      Pointer to the platform environment control 
*                                   block.  Treated opaquely.
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns status indicating whether or not the shutdown
*                  succeeded or failed. The #A2B_SUCCEEDED() and #A2B_FAILED()
*                  macros can be used to test for success/failure respectively.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_systemShutdown
    (
    a2b_PlatformShutdownFunc    f,
    A2B_ECB*                    ecb
    )
{
    a2b_HResult status = A2B_RESULT_SUCCESS;
    if ( gSystemInitialized )
    {
        if ( A2B_NULL != f )
        {
            status = f(ecb);
        }

        if ( A2B_SUCCEEDED(status) )
        {
            gSystemInitialized = A2B_FALSE;
        }
    }

    return status;

} /* a2b_systemShutdown */

