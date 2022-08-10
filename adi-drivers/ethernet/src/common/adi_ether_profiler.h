/*!
 ******************************************************************************
 *
 * @file:    adi_ether_profiler.h
 *
 * @brief:   Internal Profiling Code for Ethernet. 
 *
 * @version: $Revision: 25625 $
 *
 * @date:    $Date: 2016-03-18 07:26:22 -0400 (Fri, 18 Mar 2016) $
 * -----------------------------------------------------------------------------
 *
 * Copyright (c) 2012-2016 Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Modified versions of the software must be conspicuously marked as such.
 * - This software is licensed solely and exclusively for use with processors
 *   manufactured by or for Analog Devices, Inc.
 * - This software may not be combined or merged with other code in any manner
 *   that would cause the software to become subject to terms and conditions
 *   which differ from those listed here.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights of one
 *   or more patent holders.  This license does not release you from the
 *   requirement that you obtain separate licenses from these patent holders
 *   to use this software.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
 * PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#ifndef __ADI_ETHER_PROFILER_H__
#define __ADI_ETHER_PROFILER_H__

/*! Store Profiler Informations */
typedef struct PROFILER_DATA
{
    unsigned int nCurrMark;               /*!< Current Cycle Count Mark */
    unsigned long long nTotalCycleCount;  /*!< The total number of cycle counts */
    unsigned long long nTotalCalls;       /*!< Total number of Measurements */
    unsigned int nAvgCycles;              /*!< Average Number of cycles */
    unsigned int nMaxCycles;              /*!< Maximum Number of cycles */
    unsigned int nMinCycles;              /*!< Minimum number of cycles */

} PROFILER_DATA;

/*! Custom Profile Structure for Ethernet */
typedef struct ETHER_PROFILER
{
    PROFILER_DATA  ProfRead;           /*!< Profiler Data for Driver Read */
    PROFILER_DATA  ProfWrite;          /*!< Profiler Data for Driver Write */
    PROFILER_DATA  ProfIntRx;          /*!< Profiler Data for Driver Rx Interrupt */
    PROFILER_DATA  ProfIntTx;          /*!< Profiler Data for Driver TX Interrupt */
} ETHER_PROFILER;

/*! Load the Cycles Register to the given variable */
#define PROF_LOAD(X)   do {asm( " R0 = cycles; [%0] = R0;" : :"a"(&(X)) :"R0");}while (0)


/***************************************************************************//*!
* @brief        Initialize the Profiler Structure
*
* @param[in]    pData           Pointer to Profiler Structure 
*
* @return       None
*
* @sa           ProfilerStart, ProfilerStop
*******************************************************************************/
#pragma inline
void ProfilerInit (PROFILER_DATA *pData)
{
    /* Initialize all the variables in the Profiler Structure */
    pData->nTotalCycleCount = 0U;
    pData->nTotalCalls = 0U;
    pData->nAvgCycles = 0U;
    pData->nMaxCycles = 0U;
    pData->nMinCycles = 0xffffffff;
}

/***************************************************************************//*!
* @brief        Start Profiler for a given profile structure
*
* @param[in]    pData           Pointer to Profiler Structure 
*
* @return       None
*
* @note         In case of Nested calls, only the inner case will be considered and 
*               all other cases will be discarded
*
* @sa           ProfilerInit, ProfilerStop
*******************************************************************************/
#pragma inline
void ProfilerStart (PROFILER_DATA *pData)
{
    unsigned int nCurrCycleCount;
    unsigned int IntMask;

    /* Disable Interrupt */
    IntMask = cli();
    
    /* Load the cycles */
    PROF_LOAD (nCurrCycleCount);

    /* Set the profiler mark to current cycles */
    pData->nCurrMark = nCurrCycleCount;

    /* Enable Interrupt */
    sti(IntMask);
}

/***************************************************************************//*!
* @brief        Stop Profiler and calculate the avg, min and max
*
* @param[in]    pData           Pointer to Profiler Structure 
*
* @return       None
*
* @note         In case of Nested calls, only the inner case will be considered and 
*               all other cases will be discarded.
*
* @sa           ProfilerInit, ProfilerStart
*******************************************************************************/
#pragma inline
void ProfilerStop (PROFILER_DATA *pData)
{
    unsigned int IntMask;
    unsigned int nCycleMark;
    unsigned int nCycleDiff;
    unsigned int nCurrCycleCount;

    /* Disable Interrupt */
    IntMask = cli();

    /* Load the cycles */
    PROF_LOAD (nCurrCycleCount);

    /* Get the mask */
    nCycleMark = pData->nCurrMark;

    /* Set the mark as null for identifying some special cases */
    pData->nCurrMark = 0U;

    /* Enable the Interrupt */
    sti(IntMask);

    /* if nCycleMark is 0, then there was multiple Starts and the case is discarded */
    /* Note : If there is nested calls, only the inner one will be taken            */
    if (nCycleMark)
    {
        /* Get the cycle difference */
        nCycleDiff = nCurrCycleCount - nCycleMark;
        
        /* Add the cycle Count and increments the calls */
        pData->nTotalCycleCount = pData->nTotalCycleCount + nCycleDiff;
        pData->nTotalCalls++;
        
        /* Get the Current Average */
        pData->nAvgCycles = (unsigned int)(pData->nTotalCycleCount / pData->nTotalCalls);
        
        /* Get the Max Cycles */
        if (pData->nMaxCycles < nCycleDiff)
        {
            pData->nMaxCycles = nCycleDiff;
        }

        /* Get the Min Cycle Counts */
        if (pData->nMinCycles > nCycleDiff)
        {
            pData->nMinCycles = nCycleDiff;
        }
    }
}
    
#endif  /* __ADI_ETHER_PROFILER_H__*/
