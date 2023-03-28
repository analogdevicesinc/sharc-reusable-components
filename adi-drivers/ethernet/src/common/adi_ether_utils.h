/*!
 ******************************************************************************
 *
 * @file:    adi_ether_utils.h
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
#ifndef __ADI_ETHER_UTILS_H__
#define __ADI_ETHER_UTILS_H__

/*=============  D E F I N E S  =============*/
/*
** Cache Utilities
*/

/*!
 * If the cached data line is dirty then this macro writes the line out
 * and marks the cache line a clean. The macro increments the supplied
 * address by cache line size.
 */
#define FLUSH(P)                                   \
    do {                                           \
        asm volatile("FLUSH[%0++];" : "+p"(P));    \
    }while (0)

/*!
 * This macro flushes and invalidates the cache line. So the cache line
 * will be loaded again from next level of memory.The macro increments
 * the supplied cache line pointer by the cache size.
 */
#define FLUSHINV(P)                                \
    do {                                           \
        asm volatile("FLUSHINV[%0++];" : "+p"(P)); \
    } while (0)

/*!
 * This macro also performs flush and invalidate operation for single cache
 * line.
 */
#define SIMPLEFLUSHINV(P)                          \
    do {                                           \
        ssync();                                   \
        asm volatile("FLUSHINV[%0];"::"#p"(P));    \
        ssync();                                   \
    }while (0)


/*! expressions returning usually true are wrapped under this macro */
#define ether_exp_true(X)          (expected_true(X))
/*! expressions returning usually false are wrapped under this macro */
#define ether_exp_false(X)         (expected_false(X))

#endif /* __ADI_ETHER_UTILS_H__ */
