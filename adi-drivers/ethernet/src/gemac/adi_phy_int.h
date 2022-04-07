/*!
 ******************************************************************************
 * @file:    adi_phy_int.h
 *
 * @brief:   Physical (PHY) Interface header file
 *
 * @version: $Revision: 61638 $
 *
 * @date:    $Date: 2019-03-11 04:38:57 -0400 (Mon, 11 Mar 2019) $
 * -----------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2016 Analog Devices, Inc.
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

/** \addtogroup PHY_Interface PHY Interface
 *  @{
 */

#ifndef __ADI_PHY_INT_H__
#define __ADI_PHY_INT_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============  I N C L U D E  ============*/
#include <drivers/ethernet/adi_ether_misra.h>

/*============  D E F I N E S  ============*/
/*!
 * \enum ADI_PHY_CFG
 *
 * The Configuration Options for PHY Initialization
 */
typedef enum ADI_PHY_CFG
{
    ADI_PHY_CFG_AUTO_NEGOTIATE_EN   = (1U << 0),   /*!< Enable Auto Negotiation */
    ADI_PHY_CFG_FULL_DUPLEX_EN      = (1U << 1),   /*!< Enable Full-Duplex (Ignored if Auto Negotiation is Enabled) */
    ADI_PHY_CFG_10Mbps_EN           = (1U << 2),   /*!< Enable 10 Mbps Speed (Ignored if Auto Negotiation is Enabled) */
    ADI_PHY_CFG_100Mbps_EN          = (1U << 3),   /*!< Enable 100 Mbps Speed (Ignored if Auto Negotiation is Enabled) */
    ADI_PHY_CFG_1000Mbps_EN         = (1U << 4),   /*!< Enable 1000 Mbps Speed (Ignored if Auto Negotiation is Enabled) */
    ADI_PHY_CFG_LOOPBACK_EN         = (1U << 5),   /*!< Enable Loopback */
    ADI_PHY_CFG_POWERDOWN_EN        = (1U << 6)    /*!< Enable Power Down */
} ADI_PHY_CFG;

/*! Autonegotiate Status */
typedef enum ADI_PHY_AUTONEGOTIATE_STATUS
{
    ADI_PHY_AN_1000Mbps     = (1U << 0),   /*!< Speed negotiated to 1000 Mbps */
    ADI_PHY_AN_100Mbps      = (1U << 1),   /*!< Speed negotiated to 100 Mbps */
    ADI_PHY_AN_10Mbps       = (1U << 2),   /*!< Speed negotiated to 10  Mbps */
    ADI_PHY_AN_FULL_DUPLEX  = (1U << 3),   /*!< Full Duplex Supported */
    ADI_PHY_AN_HALF_DUPLEX  = (1U << 4),   /*!< Half Duplex Supported */
    ADI_PHY_AN_FLOWCONTROL  = (1U << 5)    /*!< Flow Control Supported */
} ADI_PHY_AUTONEGOTIATE_STATUS;

/*!
 * Generic result codes returned by PHY
 */
typedef enum ADI_PHY_RESULT
{
    ADI_PHY_RESULT_SUCCESS = 0,      /*!< PHY API is successful */
    ADI_PHY_RESULT_FAILED            /*!< Generic API Failure */
} ADI_PHY_RESULT;

/*!
 * Events codes returned by the PHY Module
 */
typedef enum ADI_PHY_EVENT
{
    ADI_PHY_EVENT_LINK_DOWN = 0x24,         /* Link Down Event */
    ADI_PHY_EVENT_LINK_UP,                  /* Link Up Event */
    ADI_PHY_EVENT_AUTO_NEGOTIATION_COMP     /* Auto Negotiation Complete Event */
} ADI_PHY_EVENT;


/* PHY Call Back Function */
typedef void (*ADI_PHY_CALLBACK_FN) (void*,uint32_t, void*);

/*!
 *  \struct ADI_PHY_DEV_INIT
 *
 *  PHY Initialization structure
 */
typedef struct ADI_PHY_DEV_INIT
{
	bool                   bResetOnly;    /*!< Reset the device without config */
    uint32_t               nConfig;       /*!< Configuration (Or configurations from enum ADI_PHY_CFG) */
    ADI_PHY_CALLBACK_FN    pfCallback;
} ADI_PHY_DEV_INIT;

struct _ADI_PHY_DEVICE;
typedef struct _ADI_PHY_DEVICE ADI_PHY_DEVICE;

struct _ADI_PHY_DEVICE {
    uint32_t PhyAddress;
    ADI_PHY_RESULT (*init) (
                            ADI_PHY_DEVICE*           pPhyDevice,
                            ADI_ETHER_HANDLE  const   hDevice,
                            ADI_PHY_DEV_INIT* const   pInitParams
                            );
    ADI_PHY_RESULT (*uninit) (ADI_PHY_DEVICE* pPhyDevice);
    ADI_PHY_RESULT (*getStatus) (ADI_PHY_DEVICE* pPhyDevice, uint32_t* const nStatus);

    ADI_PHY_CALLBACK_FN  pfCallback;
    ADI_ETHER_HANDLE     hEtherDevice;
    uint16_t             nEMACDevNum;
    uint16_t             nPHYDevNum;
};

/*============  F U N C T I O N    D E F I N I T I O N S  ============*/
extern ADI_PHY_DEVICE PhyDevice[EMAC_NUM_DEV];

extern ADI_PHY_RESULT dp8386x_phy_init (
                                 ADI_PHY_DEVICE* pPhyDevice,
                                 ADI_ETHER_HANDLE        const hDevice,
                                 ADI_PHY_DEV_INIT *      const pInitParams
                                 );
extern ADI_PHY_RESULT dp8386x_phy_uninit (ADI_PHY_DEVICE* pPhyDevice);
extern ADI_PHY_RESULT dp8386x_phy_get_status(ADI_PHY_DEVICE* pPhyDevice, uint32_t* const nStatus);

#ifdef __cplusplus
}
#endif
#endif /* __ADI_PHY_INT_H__ */

/*@}*/
