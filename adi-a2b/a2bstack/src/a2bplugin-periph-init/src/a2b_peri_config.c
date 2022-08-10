/*******************************************************************************
Copyright (c) 2014 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
*******************************************************************************

   Name       : a2b_peri_config.c

   Description: This file contains functions required to configure
                the peripherals connected to A2B transceiver(AD2410)

   Functions  :  adi_a2b_PeriheralConfig()
                 adi_a2b_DeviceConfig()
                 adi_a2b_RemoteDeviceConfig()

   Prepared &
   Reviewed by: Automotive Software and Systems team,
                IPDC, Analog Devices,  Bangalore, India

   $Date: 2015-07-29 08:44:17 +0530 (Wed, 29 Jul 2015) $

******************************************************************************/
/*! \addtogroup Network_Configuration
 *  @{
 */

/** @defgroup Remote_Peripheral_Configuration Remote Peripheral Configuration
 *
 * This module configures the peripherals on the Slave A2B nodes, through remote I2C
 *
 */

/*! \addtogroup Remote_Peripheral_Configuration
 *  @{
 */

/*============= I N C L U D E S =============*/
/*
*/

#include "adi_a2b_periconfig.h"
#include "plugin-periph-init.h"
#include "plugin_priv.h"
#include "a2b/regdefs.h"
#include "a2b/stack.h"
#include "a2b/i2c.h"
#include "a2b/error.h"
#include "adi_a2b_datatypes.h"
#include "a2b/timer.h"
#include "a2b/util.h"
#include "a2b/trace.h"
#include "a2b/interrupt.h"
#include "a2b/i2c.h"
#include "a2b/timer.h"
#include "a2b/regdefs.h"
#include "a2b/seqchart.h"

/*============= D E F I N E S =============*/

/*============= D A T A =============*/

/*============= C O D E =============*/
/*
** Function Prototype section
*/
#ifdef ENABLE_PERI_CONFIG_BCF
static a2b_UInt32 adi_a2b_RemoteDeviceConfig(a2b_Plugin* plugin, ADI_A2B_PERI_DEVICE_CONFIG* psDeviceConfig);
/*
** Function Definition section
*/

/****************************************************************************/
/*!
    @brief          This function configures/programs peripherals connected
                    to the slave node.(remote I2C)

    @param [in]     plugin                 Pointer to A2B Slave Plugin
    @param [in]     pPeriConfig            Pointer to Node Peripheral Config Table


    @return          Return code
                    - 0: Success
                    - 1: Failure
*/
/********************************************************************************/
a2b_HResult adi_a2b_PeriheralConfig(struct a2b_Plugin* plugin, ADI_A2B_NODE_PERICONFIG *pPeriConfig)
{
    a2b_UInt32 nResult = 0u;
    a2b_UInt8 i;
    a2b_Int16 nodeAddr;
#ifndef  A2B_BCF_FROM_SOC_EEPROM
    nodeAddr = plugin->nodeSig.nodeAddr;
    A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                                 "a2b_PeriheralConfig: Starting peripheral configuration "
                                 "nodeAddr = %hd", &nodeAddr));


    for(i = 0u; i < (a2b_UInt8)pPeriConfig->nNumConfig;i++)
    {
        nResult = (a2b_UInt32)adi_a2b_RemoteDeviceConfig(plugin,&pPeriConfig->aDeviceConfig[i]);
    }

    A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                                 "a2b_PeriheralConfig: Ending peripheral configuration "
                                 "nodeAddr = %hd", &nodeAddr));
#endif
    return nResult;
}

#ifndef  A2B_BCF_FROM_SOC_EEPROM
/****************************************************************************/
/*!
    @brief          This function configures devices connected to slave node
                    through remote I2C

    @param [in]     plugin                  Pointer to A2B slave Plugin
    @param [in]     psDeviceConfig          Pointer to peripheral device configuration structure

    @return          Return code
                    - 0: Success
                    - 1: Failure
*/
/********************************************************************************/
static a2b_UInt32 adi_a2b_RemoteDeviceConfig(a2b_Plugin* plugin, ADI_A2B_PERI_DEVICE_CONFIG* psDeviceConfig)
{
    ADI_A2B_MEM_PERI_CONFIG_DATA
    static a2b_UInt8 aDataBuffer[ADI_A2B_MAX_PERI_CONFIG_UNIT_SIZE];
    static a2b_UInt8 aDataWriteReadBuf[4u];

    a2b_UInt32 nReturn = 0u;
    ADI_A2B_PERI_CONFIG_UNIT* pOPUnit;
    a2b_UInt8 nIndex, nIndex1;
    a2b_UInt32 nNumOpUnits;
    a2b_UInt32 nDelayVal;
    a2b_Int16 nodeAddr;
    a2b_HResult status = A2B_RESULT_SUCCESS;

    nNumOpUnits = psDeviceConfig->nNumPeriConfigUnit;
    nodeAddr = plugin->nodeSig.nodeAddr;
    for(nIndex= 0u ; nIndex < nNumOpUnits ; nIndex++ )
    {
        pOPUnit = &psDeviceConfig->paPeriConfigUnit[nIndex];
        /* Operation code*/
        switch(pOPUnit->eOpCode)
        {
           /* write */
            case 0u:
                    adi_a2b_Concat_Addr_Data(&aDataBuffer[0u], pOPUnit->nAddrWidth, pOPUnit->nAddr);
                    (void)a2b_memcpy(&aDataBuffer[pOPUnit->nAddrWidth], pOPUnit->paConfigData, pOPUnit->nDataCount);
                    status = a2b_i2cPeriphWrite(plugin->ctx, nodeAddr, (a2b_UInt16)psDeviceConfig->nDeviceAddress,
                            (a2b_UInt16)(pOPUnit->nAddrWidth + pOPUnit->nDataCount), &aDataBuffer[0u]);
                    break;
            /* read */
            case 1u:
                   (void)a2b_memset(&aDataBuffer[0u], (a2b_Int32)0u, (size_t)pOPUnit->nDataCount);
                    adi_a2b_Concat_Addr_Data(&aDataWriteReadBuf[0u], pOPUnit->nAddrWidth, pOPUnit->nAddr);
                    status = a2b_i2cPeriphWriteRead(plugin->ctx, nodeAddr, (a2b_UInt16)psDeviceConfig->nDeviceAddress,
                            (a2b_UInt16)pOPUnit->nAddrWidth, &aDataWriteReadBuf[0u],
                            (a2b_UInt16)pOPUnit->nDataCount, &aDataBuffer[0u]);

                    break;
            /* delay */
            case 2u: nDelayVal = 0u;
                    for(nIndex1 = 0u; nIndex1 < pOPUnit->nDataCount; nIndex1++)
                    {
                        nDelayVal = (a2b_UInt32)((a2b_UInt32)pOPUnit->paConfigData[nIndex1] << (a2b_UInt32)((a2b_UInt32)8u * nIndex1)) | nDelayVal;
                    }
                    (void)a2b_ActiveDelay(plugin->ctx, nDelayVal);
                    break;

            default: break;

        }

        if(status != A2B_RESULT_SUCCESS)
        {
            nReturn = 1u;
            break;
        }
    }

    return(nReturn);
}
#endif
#endif

/****************************************************************************/
/*!
    @brief          This function calculates reg value based on width and adds
                    it to the data array

    @param [in]     pDstBuf               Pointer to destination array
    @param [in]     nAddrwidth            Data unpacking boundary(1 byte / 2 byte /4 byte )
    @param [in]     nAddr            Number of words to be copied

    @return          Return code
                    - 0: Success
                    - 1: Failure
*/
/********************************************************************************/
void adi_a2b_Concat_Addr_Data(a2b_UInt8 pDstBuf[] ,a2b_UInt32 nAddrwidth, a2b_UInt32 nAddr)
{
    /* Store the read values in the place holder */
    switch(nAddrwidth)
    {   /* Byte */
        case 1u:
                pDstBuf[0u]  =  (a2b_UInt8)nAddr;
                break;
         /* 16 bit word*/
        case 2u:

                pDstBuf[0u]  =  (a2b_UInt8)(nAddr >> 8u);
                pDstBuf[1u]  =  (a2b_UInt8)(nAddr & 0xFFu);

                break;
        /* 24 bit word */
        case 3u:
                pDstBuf[0u]  =  (a2b_UInt8)((nAddr & 0xFF0000u) >> 16u);
                pDstBuf[1u]  =  (a2b_UInt8)((nAddr & 0xFF00u) >> 8u);
                pDstBuf[2u]  =  (a2b_UInt8)(nAddr & 0xFFu);
                break;

        /* 32 bit word */
        case 4u:
                pDstBuf[0u]  =  (a2b_UInt8)(nAddr >> 24u);
                pDstBuf[1u]  =  (a2b_UInt8)((nAddr & 0xFF0000u) >> 16u);
                pDstBuf[2u]  =  (a2b_UInt8)((nAddr & 0xFF00u) >> 8u);
                pDstBuf[3u]  =  (a2b_UInt8)(nAddr & 0xFFu);
                break;

        default:
        break;

    }
}


/**
 @}
*/


/**
 @}
*/

/*
**
** EOF: $URL$
**
*/







