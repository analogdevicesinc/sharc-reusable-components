/*!
*********************************************************************************
 *
 * @file:    adi_dp83848_phy.c

 *
 * @brief:   DP83848VYB phy driver source
 *
 * @version: $Revision: 66140 $
 *
 * @date:    $Date: 2021-04-27 13:02:37 -0400 (Tue, 27 Apr 2021) $
 * ------------------------------------------------------------------------------
 *
 * Copyright (c) 2011-2019 Analog Devices, Inc.
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
#if !defined(__ADSPSC584__)

#include <sys/platform.h>
#include "adi_dp83848_phy_int.h"
#include "adi_phy_int.h"
#include <stdlib.h>
#include <services/int/adi_int.h>
#include "adi_gemac_proc_int.h"


#ifdef   GEMAC_SUPPORT_EMAC0

#if EMAC0_PHY_NUM_DEV > 0
#	if (EMAC0_PHY_DEV == 0)
#		if (EMAC0_PHY0_DEV == PHY_DEV_DP83848)
#			define __DP83848_EMAC0_PHY0__
#		endif
#	endif
#endif

#endif

#ifdef   GEMAC_SUPPORT_EMAC1

#if EMAC1_PHY_NUM_DEV > 0
#	if (EMAC1_PHY_DEV == 0)
#		if (EMAC1_PHY0_DEV == PHY_DEV_DP83848)
#			define __DP83848_EMAC1_PHY0__
#		endif
#	endif
#endif

#endif


#if !defined(__DP83848_EMAC0_PHY0__) &&  !defined(__DP83848_EMAC1_PHY0__)
#error "Phy Config Error"
#endif

static void wait_millisec(void);
static void sleep(uint32_t msec);
static int32_t poll_mii_done(ADI_PHY_DEVICE* pPhyDevice);
static uint32_t raw_phy_write   (
                                 ADI_PHY_DEVICE* pPhyDevice,
                                 const uint16_t  RegAddr,
                                 const uint32_t  Data
                                 );
static uint16_t dp83848_phy_read (
                                  ADI_PHY_DEVICE*   pPhyDevice,
                                  const uint16_t    RegAddr
                                  );
static uint32_t dp83848_phy_write (
                                   ADI_PHY_DEVICE* pPhyDevice,
                                   const uint16_t  RegAddr,
                                   const uint32_t  Data
                                   );

static void dp83848_ack_phyint (ADI_PHY_DEVICE* pPhyDevice);

static uint32_t dp83848_phy_pwrdown (
                                     ADI_PHY_DEVICE* pPhyDevice,
                                     const bool      bPowerDown
                                     );

static bool dp83848_phy_get_pwrdown (ADI_PHY_DEVICE* pPhyDevice);

/**
 * @brief     Waits for a milli second
 */
static void wait_millisec(void)
{
    WAIT_MILLISECOND_CODE;
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


/** \defgroup PHY Driver DP83848VYB PHY Driver
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
       if (!(pEmacRegs->EMAC_SMI_ADDR &  BITM_EMAC_SMI_ADDR_SMIB) )
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
                                 const uint16_t  RegAddr,
                                 const uint32_t  Data
                                 )
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    uint32_t result;

    pEmacRegs->EMAC_SMI_DATA = Data;

    pEmacRegs->EMAC_SMI_ADDR = SET_PHYAD(pPhyDevice->PhyAddress)      |
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
static uint16_t dp83848_phy_read (
                                  ADI_PHY_DEVICE*   pPhyDevice,
                                  const uint16_t    RegAddr
                                  )
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;

    poll_mii_done(pPhyDevice);

    pEmacRegs->EMAC_SMI_ADDR = SET_PHYAD(pPhyDevice->PhyAddress)     |
                                SET_REGAD(RegAddr)                    |
                                BITM_EMAC_SMI_ADDR_SMIB               |
                                SET_CR(pDev->MdcClk);

    poll_mii_done(pPhyDevice);
    return (uint16_t)pEmacRegs->EMAC_SMI_DATA;
}

/**
 * @brief       Writes data to the phy register after checking the busy state
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
static uint32_t dp83848_phy_write (
                                   ADI_PHY_DEVICE* pPhyDevice,
                                   const uint16_t  RegAddr,
                                   const uint32_t  Data
                                   )
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    uint32_t result;

    poll_mii_done(pPhyDevice);

    raw_phy_write(pPhyDevice,RegAddr,Data);

    pEmacRegs->EMAC_SMI_DATA = Data;

    pEmacRegs->EMAC_SMI_ADDR = SET_PHYAD(pPhyDevice->PhyAddress)  |
                                SET_REGAD(RegAddr)                 |
                                BITM_EMAC_SMI_ADDR_SMIB            |
                                BITM_EMAC_SMI_ADDR_SMIW            |
                                SET_CR(pDev->MdcClk);
    result = poll_mii_done(pPhyDevice);

    return result;
}

/**
 * @brief       Acknowledge phy interrupt
 *
 * @param [in]  hDevice    Device driver handle
 *
 * @param [in]  PhyAddr     Physical address of the ethernet PHY
 *
 * FIXIT: use the GPIO/INT service
 *
 */
static void dp83848_ack_phyint (ADI_PHY_DEVICE* pPhyDevice)
{
    uint16_t VAR_UNUSED_DECR(phy_data);
    
    
    
    phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MISR);
    phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MICR);
    VAR_UNUSED(phy_data);
    

#ifdef __DP83848_EMAC0_PHY0__
    if ((pPhyDevice->nEMACDevNum == 0) && (pPhyDevice->nPHYDevNum == 0)) {
    	EMAC0_PHY0_DP83848_ACK_PHY_INT;
    }
#endif
#ifdef __DP83848_EMAC1_PHY0__
    if ((pPhyDevice->nEMACDevNum == 1) && (pPhyDevice->nPHYDevNum == 0)) {
    	EMAC1_PHY0_DP83848_ACK_PHY_INT;
    }
#endif
}

/**
 * @brief       Power down PHY
 *
 * @param [in]  hDevice    Device driver handle
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 * @param [in]  bPowerDown   If true power down the PHY else power up the PHY
 *
 * @return      true if operation is successful
 *
 */
FUN_UNUSED static uint32_t dp83848_phy_pwrdown (
                                     ADI_PHY_DEVICE* pPhyDevice,
                                     const bool      bPowerDown
                                     )
{
    uint16_t phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MODECTL);
    uint32_t result;

    if(bPowerDown)
        phy_data |= PHY_MODECTL_POWER_DOWN;
    else
        phy_data &= ~PHY_MODECTL_POWER_DOWN;

    result = dp83848_phy_write(pPhyDevice, REG_PHY_MODECTL,phy_data);

    return result;
}

/**
 * @brief       Returns PHY powerdown status
 *
 * @param [in]  hDevice    Device driver handle
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 * return       True if phy is in power down state. False if it is powered.
 */
FUN_UNUSED static bool dp83848_phy_get_pwrdown (ADI_PHY_DEVICE* pPhyDevice)
{
    uint16_t phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MODECTL);

    return(! (phy_data & PHY_MODECTL_POWER_DOWN));
}


static void PHYInterruptHandler(uint32_t IID, void *pCBParm)
{
    ADI_PHY_DEVICE*   pPhyDevice = (ADI_PHY_DEVICE*)pCBParm;
    ADI_EMAC_DEVICE*  const  pDev      = (ADI_EMAC_DEVICE*)pPhyDevice->hEtherDevice;
    volatile uint32_t  phyReg;

    /* read the phy status */
    phyReg = dp83848_phy_read(pPhyDevice,REG_PHY_STS);

    /* check whether the link is up or down */
    if (phyReg & PHY_STS_LINK_STATUS)
    {
        /*
         * configure GEMAC with the auto-negotiation results
         */
        if (phyReg & PHY_STS_AUTO_NEG_COMPLETE)
        {
            uint32_t nAutoNegStatus = 0u;

            nAutoNegStatus |= (phyReg & PHY_STS_SPEED_STATUS)  ? ADI_PHY_AN_10Mbps : ADI_PHY_AN_100Mbps;
            if (phyReg & PHY_STS_DUPLEX_STATUS) {
                nAutoNegStatus |= ADI_PHY_AN_FULL_DUPLEX;
            }
            pPhyDevice->pfCallback((void*)pDev, ADI_PHY_EVENT_AUTO_NEGOTIATION_COMP, (void*)nAutoNegStatus);
        }

    }
    else /* link down */
    {
        pPhyDevice->pfCallback((void*)pDev, ADI_PHY_EVENT_LINK_DOWN, NULL);
    }

    dp83848_ack_phyint(pPhyDevice);
}

/**
 * @brief       Initializes the PHY
 *
 * @param [in]  hDevice     EMAC device handle
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 */
ADI_PHY_RESULT dp83848_phy_init (
                                 ADI_PHY_DEVICE*               pPhyDevice,
                                 ADI_ETHER_HANDLE        const hDevice,
                                 ADI_PHY_DEV_INIT      * const pInitParams
                                 )
{
    uint16_t phy_data;
    uint16_t VAR_UNUSED_DECR(phy_id1);
    uint16_t VAR_UNUSED_DECR(phy_id2);
    

    /* Set the callback function and ether device handle */
    pPhyDevice->pfCallback = pInitParams->pfCallback;
    pPhyDevice->hEtherDevice    = hDevice;

    /* reset the phy */
    raw_phy_write(pPhyDevice, REG_PHY_MODECTL, PHY_MODECTL_RESET);

    /* wait 1 second, DP83848 spec recommends 3us wait time after reset */
    sleep(1000);

    /* read mode control register */
    phy_data = dp83848_phy_read(pPhyDevice,REG_PHY_MODECTL);

    if (phy_data & PHY_MODECTL_RESET)
    {
        ETHER_PRINT("Phy reset failed\n");
        return ADI_PHY_RESULT_FAILED;
    }

    if (pInitParams->bResetOnly) {
    	return ADI_PHY_RESULT_SUCCESS;
    }

    /* read phy identifier registers */
    phy_id1 = dp83848_phy_read(pPhyDevice,REG_PHY_PHYID1);
    phy_id2 = dp83848_phy_read(pPhyDevice, REG_PHY_PHYID2);
    
    VAR_UNUSED(phy_id1);
    VAR_UNUSED(phy_id2);

    /* advertise flow control supported */
    phy_data = dp83848_phy_read(pPhyDevice,REG_PHY_ANAR);
    phy_data |= PHY_ANAR_PAUSE_OPERATION;
    dp83848_phy_write(pPhyDevice,REG_PHY_ANAR, phy_data );
    phy_data = dp83848_phy_read(pPhyDevice,REG_PHY_ANAR);

    phy_data = dp83848_phy_read(pPhyDevice,REG_PHY_MODECTL);
    phy_data &= ~PHY_MODECTL_AUTO_NEGO_ENA;
    dp83848_phy_write(pPhyDevice,REG_PHY_MODECTL, phy_data );

    phy_data  = 0;

    /* loopback mode - enable rx <-> tx loopback */
    if(pInitParams->nConfig & ADI_PHY_CFG_LOOPBACK_EN)
        phy_data |= PHY_MODECTL_LOOPBACK;

    /* check if auto-negotiation is enabled */
    if(pInitParams->nConfig & ADI_PHY_CFG_AUTO_NEGOTIATE_EN)
    {
        phy_data |= PHY_MODECTL_AUTO_NEGO_ENA | PHY_MODECTL_RESTART_A_NEGO;
    }
    else /* configure as specified by the driver */
    {
        /* configure duplex mode */
        if(pInitParams->nConfig & ADI_PHY_CFG_FULL_DUPLEX_EN)
            phy_data |= PHY_MODECTL_DUPLEX_MODE;
        else
            phy_data &= ~PHY_MODECTL_DUPLEX_MODE;

        /* configure port speed  */
        if(pInitParams->nConfig & ADI_PHY_CFG_10Mbps_EN)
            phy_data &= ~PHY_MODECTL_SPEED_SELECT;
        else
            phy_data |= PHY_MODECTL_SPEED_SELECT;
    }

    /* acknowledge any latched interrupts */
    dp83848_ack_phyint(pPhyDevice);
    

#ifdef __DP83848_EMAC0_PHY0__
    if ((pPhyDevice->nEMACDevNum == 0) && (pPhyDevice->nPHYDevNum == 0)) {
    	EMAC0_PHY0_DP83848_PORT_CFG;
    	adi_int_InstallHandler(EMAC0_PHY0_DP83848_INT,PHYInterruptHandler,(void*)pPhyDevice,true);
    }
#endif
#ifdef __DP83848_EMAC1_PHY0__
    if ((pPhyDevice->nEMACDevNum == 1) && (pPhyDevice->nPHYDevNum == 0)) {
    	EMAC1_PHY0_DP83848_PORT_CFG;
    	adi_int_InstallHandler(EMAC1_PHY0_DP83848_INT,PHYInterruptHandler,(void*)pPhyDevice,true);
    }
#endif

    
    dp83848_phy_write(pPhyDevice, REG_PHY_MICR, PHY_MICR_INTEN | PHY_MICR_INT_OE);

    /* enable phy interrupts  */
    dp83848_phy_write(pPhyDevice, REG_PHY_MISR,
                                        PHY_MISR_LINKINT_EN |  /* link established */
                                        PHY_MISR_ANCINT_EN);   /* auto-negotiation completed */
    /* start the auto-negotiation */
    dp83848_phy_write(pPhyDevice, REG_PHY_MODECTL, phy_data);
    /*
     * By default PHY interrupt is active. By enabling ADI_ETHER_POLLING this routine
     * will not return until auto-negotiation is complete. So if a cable is not plugged-in
     * the code may block infinitely.
     */
    //sleep(3000);
#if defined(ADI_ETHER_POLLING)
    /* read the mode status register */
    do {
       phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MODESTAT);
    } while(!(phy_data & 0x0020));

    phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_CR);

    /* clear  pending PHY interrupts */
    phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MISR);
    phy_data = dp83848_phy_read(pPhyDevice, REG_PHY_MICR);
#endif /* ADI_ETHER_POLLING */

    return ADI_PHY_RESULT_SUCCESS;
}

/**
 * @brief       Get the PHY status
 *
 * @param [in]  hDevice    Device driver handle
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 * return       Status
 */
ADI_PHY_RESULT dp83848_phy_get_status(ADI_PHY_DEVICE* pPhyDevice, uint32_t* const nStatus)
{
    uint16_t phy_data = dp83848_phy_read(pPhyDevice,REG_PHY_STS);
    uint32_t status = 0;

    if (phy_data & PHY_STS_LINK_STATUS)
          status |= (ADI_ETHER_PHY_LINK_UP);

    if ( phy_data & PHY_STS_SPEED_STATUS )
    {
        if (phy_data & PHY_STS_DUPLEX_STATUS)
            status |= ADI_ETHER_PHY_10T_FULL_DUPLEX;
        else
            status |= ADI_ETHER_PHY_10T_HALF_DUPLEX;
    }
    else
    {
        if (phy_data & PHY_STS_DUPLEX_STATUS)
            status |= ADI_ETHER_PHY_100T_FULL_DUPLEX;
        else
            status |= ADI_ETHER_PHY_100T_HALF_DUPLEX;
    }

    if (phy_data & PHY_STS_AUTO_NEG_COMPLETE)
          status |= (ADI_ETHER_PHY_AN_COMPLETE);

    if (phy_data & PHY_STS_LOOPBACK_STATUS)
          status |= (ADI_ETHER_PHY_LOOPBACK);

    *nStatus = status;
    return ADI_PHY_RESULT_SUCCESS;
}

ADI_PHY_RESULT dp83848_phy_uninit (ADI_PHY_DEVICE* pPhyDevice)
{
#ifdef __DP83848_EMAC0_PHY0__
    if ((pPhyDevice->nEMACDevNum == 0) && (pPhyDevice->nPHYDevNum == 0)) {
    	adi_int_EnableInt(EMAC0_PHY0_DP83848_INT, false);
    	adi_int_UninstallHandler(EMAC0_PHY0_DP83848_INT);
    }
#endif
#ifdef __DP83848_EMAC1_PHY0__
    if ((pPhyDevice->nEMACDevNum == 1) && (pPhyDevice->nPHYDevNum == 0)) {
    	adi_int_EnableInt(EMAC1_PHY0_DP83848_INT, false);
    	adi_int_UninstallHandler(EMAC1_PHY0_DP83848_INT);
    }
#endif

    return ADI_PHY_RESULT_SUCCESS;
}




#ifdef ADI_DEBUG
#if 0
#include <stdio.h>
/**
 * @brief       Returns all PHY register contents
 *
 * @param [in]  PhyAddr      Physical Address of the PHY
 *
 * @param [out] pRegister    Constant pointer to the memory where register
 *                           values will be saved
 *
 * @param [in]  numRegs      Number of registers
 *
 * @return      void  // FIXIT
 *
 * @note        valid only in debug build
 *
 */
static void  dp83848_phy_get_regs(ADI_ETHER_HANDLE* const hDevice,
                           const uint16_t PhyAddr,
                           int32_t* const pRegisters,
                           int32_t numRegs)
{
    int32_t register_indx;

    for(register_indx = 0; register_indx < NUM_PHY_REGS; register_indx++ )
    {
        pRegisters[register_indx] = dp83848_phy_read(hDevice,PhyAddr,register_indx);
    }
}
#endif
#endif /* ADI_DEBUG */

#endif /* Processor check */
/*@}*/
