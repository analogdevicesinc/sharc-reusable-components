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
 * \file:   verinfo.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of routines to retrieve the version and build
 *          information for a plugin.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "verinfo.h"
#include "a2b/msgtypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
 *
 * \b       a2b_getVerInfo
 *
 * Returns the version and build information for the plugin.
 *
 * \param   [in,out]    verInfo     A pointer to a version structure that
 *                                  will filled in with the plugin's
 *                                  version/build information.
 *
 * \pre     None
 *
 * \post    None
 *
 * \return  None
 *
 *****************************************************************************/
void
a2b_mstr_getVerInfo
    (
    struct a2b_PluginVerInfo* verInfo
    )
{
    if ( A2B_NULL != verInfo )
    {
        verInfo->resp.majorVer = 17u;
        verInfo->resp.minorVer = 0u;
        verInfo->resp.relVer = 0u;
        verInfo->resp.buildInfo =
                    "date: Tue 12 Sep 2017 11:40:00 AM EDT,"
                    "rev: 217e5ad87cf68de7d9c1004ea1e6913dcd8f79b6,"
                    "build: 15,"
                    "owner: ADI,"
                    "host: ADI";
    }
}

