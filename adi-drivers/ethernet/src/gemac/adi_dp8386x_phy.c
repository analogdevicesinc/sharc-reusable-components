/*!
*********************************************************************************
 *
 * @file:    adi_dp8386x_phy.c

 *
 * @brief:   PHY virtual driver
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
#include "adi_dp8386x_phy.h"
#include <stdlib.h>

#if defined (__ADSP215xx__)
/* Mapping required for SC589 processor */
#define EMAC_GMII_ADDR         EMAC_SMI_ADDR
#define EMAC_GMII_DATA         EMAC_SMI_DATA
#endif

/**
 * @brief     Waits for a milli second
 */
static void wait_millisec(void)
{
	WAIT_MILLISECOND_CODE
}
/**
 * @brief     Sleeps for specified number of milli-seconds
 */
static void sleep(uint32_t msec)
{
    while (msec != 0) {
           wait_millisec();
           msec--;
    }
    return;
}


/** \defgroup PHY Driver DP83865VYB PHY Driver
 *  @{
 */

/**
 * @brief       Wait until busy bit is cleared
 *
 * @param [in]  hDevice    Device driver handle
 *
 * @return      void
 */
static int32_t poll_mii_done(ADI_PHY_DEVICE* pPhyDevice)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;

    int32_t  loop_count = PHY_LOOP_COUNT;

    /* poll for the GMII busy bit */
    do
    {
       if (!(pEmacRegs->EMAC_GMII_ADDR &  BITM_EMAC_SMI_ADDR_SMIB) )
               break;
    } while (--loop_count > 0);

    return ( (loop_count == 0) ? -1 : 0);
}

/**
 * @brief       Writes data to the phy register without checking the busy state
 *
 * @param [in]  hDevice     Device driver handle
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 * @param [in]  RegAddr      Register Address in the PHY
 *
 * @param [in]  Data         Data to be written
 *
 * @return      void
 *
 */
static uint32_t raw_phy_write   (
                                 ADI_PHY_DEVICE* pPhyDevice,
								 const uint16_t  PhyAddress,
                                 const uint16_t  RegAddr,
                                 const uint32_t  Data
                                 )
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    uint32_t result;

    pEmacRegs->EMAC_GMII_DATA = Data;

    pEmacRegs->EMAC_GMII_ADDR = SET_PHYAD(PhyAddress)      |
                                SET_REGAD(RegAddr)                     |
                                BITM_EMAC_SMI_ADDR_SMIB                |
                                BITM_EMAC_SMI_ADDR_SMIW                |
                                SET_CR(pDev->MdcClk);
    result = poll_mii_done(pPhyDevice);
    return result;
}

/**
 * @brief       Reads data from a PHY register
 *
 * @param [in]  hDevice     Device driver handle
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 * @param [in]  RegAddr      Register Address in the PHY
 *
 * @return      16-bit register data
 *
 */
static uint16_t raw_phy_read (
                                  ADI_PHY_DEVICE*   pPhyDevice,
								  const uint16_t    PhyAddress,
                                  const uint16_t    RegAddr
                                  )
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;

    poll_mii_done(pPhyDevice);

    pEmacRegs->EMAC_GMII_ADDR = SET_PHYAD(PhyAddress)     |
                                SET_REGAD(RegAddr)                    |
                                BITM_EMAC_SMI_ADDR_SMIB               |
                                SET_CR(pDev->MdcClk);

    poll_mii_done(pPhyDevice);
    return (uint16_t)pEmacRegs->EMAC_GMII_DATA;
}

ADI_PHY_RESULT dp8386x_phy_init (
                                 ADI_PHY_DEVICE* 		 pPhyDevice,
                                 ADI_ETHER_HANDLE        const hDevice,
                                 ADI_PHY_DEV_INIT *      const pInitParams
                                 )
{
    uint16_t 		phy_vendor0;
    uint16_t 		phy_vendor1;
    uint16_t 		phy_model;

    /* Set the ether device handle */
    pPhyDevice->hEtherDevice = hDevice;

	// Try to reset PHY's at address 0 and 1
    raw_phy_write(pPhyDevice, 0, REG_PHY_MODECTL, PHY_MODECTL_RESET);
    raw_phy_write(pPhyDevice, 1, REG_PHY_MODECTL, PHY_MODECTL_RESET);

    // wait 1 second
    sleep(1000);

    phy_vendor0 = raw_phy_read(pPhyDevice,0, REG_PHY_PHYID1);
    phy_vendor1 = raw_phy_read(pPhyDevice,1, REG_PHY_PHYID1);

    if(phy_vendor0 == DP83865_OUI_VENDOR)
    	pPhyDevice->PhyAddress = 0;
    else if(phy_vendor1 == DP83865_OUI_VENDOR)
    	pPhyDevice->PhyAddress = 1;
    else
    {
        ETHER_PRINT("Phy identification failed vendor ID\n");
        return ADI_PHY_RESULT_FAILED;
    }

    // Read PHY ID to determine PHY
    /* read PHY identifier registers */
    phy_model = raw_phy_read(pPhyDevice,pPhyDevice->PhyAddress, REG_PHY_PHYID2);

    // Setup for appropriate PHY
    if (phy_model == DP83865_OUT_MODEL)
    {
    	pPhyDevice->uninit = dp83865_phy_uninit;
    	pPhyDevice->getStatus = dp83865_phy_get_status;
    	return dp83865_phy_init( pPhyDevice, hDevice, pInitParams );
    }
    else if (phy_model == DP83867_OUT_MODEL)
    {
    	pPhyDevice->uninit = dp83867_phy_uninit;
    	pPhyDevice->getStatus = dp83867_phy_get_status;
    	return dp83867_phy_init( pPhyDevice, hDevice, pInitParams );
    }
    else
    {
        ETHER_PRINT("Phy identification failed model ID\n");
        return ADI_PHY_RESULT_FAILED;
    }

}

ADI_PHY_RESULT dp8386x_phy_uninit (ADI_PHY_DEVICE* pPhyDevice)
{
	// If we get here, report runtime error
	return ADI_PHY_RESULT_FAILED;
}

ADI_PHY_RESULT dp8386x_phy_get_status(ADI_PHY_DEVICE* pPhyDevice, uint32_t* const nStatus)
{
 // If we get here, report runtime error
	return ADI_PHY_RESULT_FAILED;
}
