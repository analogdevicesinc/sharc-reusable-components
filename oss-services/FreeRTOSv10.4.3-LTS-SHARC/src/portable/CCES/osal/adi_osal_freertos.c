/*
 * Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
 */


/*!
    @file adi_osal_freertos.c

    Operating System Abstraction Layer - OSAL for FreeRTOS


*/
/** @addtogroup ADI_OSAL_FreeRTOS ADI OSAL FreeRTOS
 *  @{
 *
 *  This module contains the APIs designed to abstract FreeRTOS the operating
 *  system from the user.
 *
 */

/*=============  I N C L U D E S   =============*/


#include <string.h>                                                             /* for strncpy */
#include <stdlib.h>

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

/*  disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm143")
#endif
/*! @endcond */

/* "version.h" contain macro "ADI_BUILD_VER"  which gives the
    build version for OSAL for Free RTOS. This is a generated file.
*/


/*=============  D E F I N E S  =============*/





#include "version.h"


/*=============  D A T A  =============*/

/*=============  C O D E  =============*/


/*!
   ****************************************************************************
     @brief Returns the code-base version information.

     The code-base version differs for each target operating system of the OSAL
     although the major and minor revs are the same for all OS variants.

     @param[out] pVersion - the location to store the retrieved version
                            information.

     @return ADI_OSAL_SUCCESS - if able to successfully return the version
     @return ADI_OSAL_FAILED  - in the unlikely event that the version
                               information could not be obtained.

    @note
    Version number is mentioned in the format major.minor.patch.build.
    For example,Version "1.0.2.2022" means

    => major  = 1.
    => minor  = 0.
    => patch  = 2.
    => build  = 2022.

    Members of structure ADI_OSAL_VERSION_PTR are also declared in above order.
*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetVersion(ADI_OSAL_VERSION *pVersion)
{
    pVersion->nMajor = ADI_OSAL_MAJOR_VER;
    pVersion->nMinor = ADI_OSAL_MINOR_VER;
    pVersion->nPatch = ADI_OSAL_PATCH_VER;
    pVersion->nBuild = ADI_BUILD_VER;
    return(ADI_OSAL_SUCCESS);
}



/*!
  ****************************************************************************
    @brief Configures OSAL.

    This function configures the internal OSAL data structure.

    @param[in] pConfig - pointer to a ADI_OSAL_CONFIG data structure that
                         contains the OSAL configuration options.

    @return ADI_OSAL_SUCCESS          - Configuration is done successfully.
    @return ADI_OSAL_FAILED           - OSAL was already configured
    @return ADI_OSAL_OS_ERROR         - The version of OSAL is not compatible
                                        with the FreeRTOS version
    @return ADI_OSAL_BAD_SLOT_KEY     - Number of thread local storage slots
                                        specified greater than the maximum
                                        allowed.
    @return ADI_OSAL_BAD_PRIO_INHERIT - Priority inheritance specified when it
                                        is not supported or vice versa.
    @return ADI_OSAL_MEM_ALLOC_FAILED - Error initializing dynamic memory heap
    @return ADI_OSAL_INVALID_ARGS     - The arguments do not describe a viable
                                        configuration

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_Config( const ADI_OSAL_CONFIG *pConfig)
{
    uint32_t* pHeapMemory = NULL; /*ptr to the memory to use for the heap */
    uint32_t nHeapMemorySize = 0u;/*size of memory pointed by pHeapMemory */

#ifdef OSAL_DEBUG
    /*!
        @internal
        @var snOsalConfigurationState
             Static variable to record if OSAL has already been configured
        @endinternal
    */
    static uint32_t snOsalConfigurationState = 0u ;
#endif

    /* Checks that the version of Free RTOS is compatible with this version of OSAL */
    if (GetOSVersion() < COMPATIBLE_OS_VERSION)
    {
        return (ADI_OSAL_OS_ERROR);
    }

#ifdef OSAL_DEBUG
    /* Check if already configured. If the parameters are the same then the
     * call succeeds. Otherwise the call fails. The priority inheritance
     * setting is not recorded so we cannot check it
     */
    if (OSAL_INITIALIZED == snOsalConfigurationState)
    {
        if (NULL == pConfig)
        {
            return (ADI_OSAL_SUCCESS);
        }
        else
        {
            if ( (pConfig->nNumTLSSlots != _adi_osal_gnNumSlots) ||
                (pConfig->nSysTimerPeriodInUsec != _adi_osal_gnTickPeriod) ||
                (pConfig->pHeap != pHeapMemory) ||
                (pConfig->nHeapSizeBytes != nHeapMemorySize))
            {
                return (ADI_OSAL_FAILED);
            }
            else
            {
                return (ADI_OSAL_SUCCESS);
            }
        }
    }
#endif

    /* checks that arguments are all valid */
    if (NULL != pConfig)
    {
#ifdef OSAL_DEBUG
        if ( (ADI_OSAL_PRIO_INHERIT_ENABLED != pConfig->eEnablePrioInherit) &&
             (ADI_OSAL_PRIO_INHERIT_AUTO    != pConfig->eEnablePrioInherit))
        {
            /* incorrect value for priority inheritance */
            return(ADI_OSAL_BAD_PRIO_INHERIT);
        }

        if (pConfig->nNumTLSSlots > (uint32_t) ADI_OSAL_MAX_NUM_TLS_SLOTS)
        {
            return (ADI_OSAL_BAD_SLOT_KEY);
        }
#endif

        if (pConfig->nSysTimerPeriodInUsec == (uint32_t) 0)
        {
            /* automatically sets the tick period based on the FreeRTOS settings */
            _adi_osal_gnTickPeriod = USEC_PER_SEC / configTICK_RATE_HZ;
        }
        else
        {
            _adi_osal_gnTickPeriod = pConfig->nSysTimerPeriodInUsec;
        }
        _adi_osal_gnNumSlots = pConfig->nNumTLSSlots;

        pHeapMemory = pConfig->pHeap;
        nHeapMemorySize = pConfig->nHeapSizeBytes;

    }

    /* Create a heap with the information provided. If pHeapMemory was NULL
       then _adi_osal_HeapInstall will set its heap to the default
     */

    if ((NULL != pHeapMemory) && (0u == nHeapMemorySize))
    {
        return (ADI_OSAL_INVALID_ARGS);
    }

    if (ADI_OSAL_SUCCESS != _adi_osal_HeapInstall(pHeapMemory,nHeapMemorySize))
    {
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }


#ifdef OSAL_DEBUG
   snOsalConfigurationState = OSAL_INITIALIZED;
#endif

    return( ADI_OSAL_SUCCESS);
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/
