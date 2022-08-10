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
 * \file:   stackinfo.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of version and build information for the
 *          A2B stack.
 *
 ******************************************************************************
 * This file has been created from stackinfo.in and should NOT be modified.
 ******************************************************************************
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/stack.h"
#include "a2b/stackversion.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
* 
*  \b       a2b_stackGetVersion
* 
*  Returns the A2B stack major, minor, and/or release version.
*  This is the version assigned to the stack when it's built and is
*  encoded in the binary library.
* 
*  \param   [in,out]    major   The major version of the stack. If you are not
*                               interested in the value pass in A2B_NULL.
* 
*  \param   [in,out]    minor   The minor version of the stack. If you are not
*                               interested in the value pass in A2B_NULL.
* 
*  \param   [in,out]    release The release version of the stack. If you are not
*                               interested in the value pass in A2B_NULL.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
*****************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackGetVersion
    (
    a2b_UInt32*  major,
    a2b_UInt32*  minor,
    a2b_UInt32*  release
    )
{
    if ( A2B_NULL != major )
    {
        *major = A2B_STACK_VER_MAJOR;
    }

    if ( A2B_NULL != minor )
    {
        *minor = A2B_STACK_VER_MINOR;
    }

    if ( A2B_NULL != release )
    {
        *release = A2B_STACK_VER_RELEASE;
    }

} /* a2b_stackGetVersion */


/*!****************************************************************************
* 
*  \b       a2b_stackGetBuild
* 
*  Returns information about the build including build number,
*  date, owner, source revision, and/or host machine that did the build.
* 
*  \param   [in,out]    buildNum    An incrementing build number. If you are not
*                                   interested in the value pass in A2B_NULL.
* 
*  \param   [in,out]    buildDate   The date the library was built. If you are 
*                                   not interested in the value pass in 
*                                   A2B_NULL.
* 
*  \param   [in,out]    buildOwner  The owner/user who built this software. If 
*                                   you are not interested in the value pass 
*                                   in A2B_NULL.
* 
*  \param   [in,out]    buildSrcRev The source control revision associated with
*                                   the stack being built. If you are not
*                                   interested in the value pass in A2B_NULL.
* 
*  \param   [in,out]    buildHost   The host machine where the A2B stack was
*                                   built. If you are not interested in the 
*                                   value pass in A2B_NULL.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stackGetBuild
    (
    a2b_UInt32*             buildNum,
    const a2b_Char** const  buildDate,
    const a2b_Char** const  buildOwner,
    const a2b_Char** const  buildSrcRev,
    const a2b_Char** const  buildHost
    )
{
    if ( A2B_NULL != buildNum )
    {
        *buildNum = 15u;
    }
    
    if ( A2B_NULL != buildDate )
    {
        *buildDate = "Fri 30 Sep 2016 11:39:40 AM EDT";
    }
    
    if ( A2B_NULL != buildOwner )
    {
        *buildOwner = "kfurge";
    }
    
    if ( A2B_NULL != buildSrcRev )
    {
        *buildSrcRev = "31d4d0b8ff9e5903b985ec3a40f68340d77d6947";
    }
    
    if ( A2B_NULL != buildHost )
    {
        *buildHost = "kfurge-HP-EliteBook-8460p";
    }

} /* a2b_stackGetBuild */

