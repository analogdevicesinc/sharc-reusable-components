/*!
*********************************************************************************
 *
 * @file:    adi_dp83867_phy_int.h
 *
 * @brief:   DP83867VYB phy driver header
 *
 * @version: $Revision: 61638 $
 *
 * @date:    $Date: 2019-03-11 04:38:57 -0400 (Mon, 11 Mar 2019) $
 * ------------------------------------------------------------------------------
 *
 * Copyright (c) 2011-2016 Analog Devices, Inc.
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
 ******************************************************************************/
#ifndef _ADI_PHYDP83867_H_
#define _ADI_PHYDP83867_H_

#include "adi_dp8386x_phy.h"

//#define REG_PHY_1KSTSR            (10)          /*!<  1000Base-T Status Register   */
//#define REG_PHY_1KSCR             (15)          /*!<  1000Base-T Extended Status Register   */
//#define REG_PHY_STRAP_OPTION      (16)          /*!<  Strap Option Register   */
#define REG_PHY_LINK_AN_STATUS    (17)          /*!<  Link & Autonegotiation Status Register   */
 #define PHY_LINK_STATUS_UP             	(0x0400)
 #define PHY_LINK_STATUS_FULL_DUPLEX		(0x2000)
 #define BITM_PHY_LINK_STATUS_SPEED    		(0xC000)
 #define ENUM_PHY_LINK_STATUS_SPEED_1000 	(0x8000)
 #define ENUM_PHY_LINK_STATUS_SPEED_100  	(0x4000)
 #define ENUM_PHY_LINK_STATUS_SPEED_10  	(0x0000)


// 867 interrupts
#define REG_PHY_INT_MASK			(0x12)
 #define PHY_INT_MASK_SPEED			(0x4000)
 #define PHY_INT_MASK_DUPLEX		(0x2000)
 #define PHY_INT_MASK_AUTO_NEGO_COM	(0x0800)
 #define PHY_INT_MASK_LINK			(0x0400)
 #define PHY_INT_MASK_MDIX			(0x0040)
 #define PHY_INT_MASK_POL			(0x0002)

#define REG_PHY_INT_READ_CLEAR      (0x13)


#endif /* _ADI_PHYDP83867_H_ */
