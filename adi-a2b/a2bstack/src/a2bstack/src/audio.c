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
 * \file:   audio.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of interfaces supporting A2B stack audio access.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/error.h"
#include "a2b/audio.h"
#include "stack_priv.h"
#include "stackctx.h"


/*======================= D E F I N E S ===========================*/


/*======================= L O C A L  P R O T O T Y P E S  =========*/


/*======================= D A T A  ================================*/


/*======================= M A C R O S =============================*/


/*======================= C O D E =================================*/


/*!****************************************************************************
*
*  \b              a2b_audioConfig
*
*  Public interface to call the PAL audioConfig. This interface is used to
*  pass master node TDM settings to the application so it can properly
*  intialize audio.
* 
*  \note This API is restricted to only the Master plugin.
*
*  \param          [in]    ctx              A2B Stack Context
*  \param          [in]    tdmSettings      TDM setting for the master node
*
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_audioConfig
    (
    struct a2b_StackContext*   ctx,
    a2b_TdmSettings*    tdmSettings
    )
{
    a2b_HResult status = A2B_RESULT_SUCCESS;

    /* Only the master plugin is allowed to call this API */
    if ( (ctx->domain != A2B_DOMAIN_PLUGIN) ||
         (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) )
    {
        return A2B_MAKE_HRESULT( A2B_SEV_FAILURE,
                                 A2B_FAC_STACK,
                                 A2B_EC_PERMISSION );
    }

    if (( A2B_NULL != ctx->stk->pal.audioConfig ) && 
        ( A2B_NULL != ctx->stk->audioHnd ))
    {
        status = ctx->stk->pal.audioConfig( ctx->stk->audioHnd,
                                            tdmSettings );
    }

    return status;

} /* a2b_audioConfig */
