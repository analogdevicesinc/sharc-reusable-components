/*!
*********************************************************************************
 *
 * @file:    adi_ether_gemac.h
 *
 * @brief:   GEMAC header file
 *
 * @version: $Revision: 25625 $
 *
 * @date:    $Date: 2016-03-18 07:26:22 -0400 (Fri, 18 Mar 2016) $
 * ------------------------------------------------------------------------------
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
 ******************************************************************************/
#ifndef _ADI_ETHER_GEMAC_H_
#define _ADI_ETHER_GEMAC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "adi_ether.h"
#include <adi_types.h>


#ifdef _MISRA_RULES
#pragma diag(push)
/* Rule 5.1(Req) : Identifiers (internal and external) shall not rely on the significance of more than 31 characters. */
#pragma diag (suppress: misra_rule_5_1: "Identifies of size more than 31 characters required")
#endif

extern ADI_ETHER_DRIVER_ENTRY  GEMAC0DriverEntry;
#if defined(__ADSPBF6xx__) || defined(__ADSPSC587__) || defined(__ADSPSC589__)
extern ADI_ETHER_DRIVER_ENTRY  GEMAC1DriverEntry;
#endif /* defined(__ADSPBF6xx__) || defined(__ADSPSC587__) || defined(__ADSPSC589__) */

#define ADI_GEMAC_NUM_AV_DMA                (2)

/** @addtogroup  Ethernet_Driver Ethernet Driver Interface
 *  @{
 */

/* \enum ADI_GEMAC_CMD
 *
 * The various command supported by the gemac ethernet driver
 */
enum ADI_GEMAC_CMD
{
	/*! This command set the Tx Interrupt Period
	 *
	 *  Argument:
	 *     0          - invalid
	 *     N (N >= 0) - Tx Interrupt can be spaced out to a maximum of N frames
	 *                  Example:
	 *                  	N = 1 means Tx Interrupts are generated for every Tx Frames
	 *                  	N = 2 means Tx interrupts can be configured to be generated
	 *                  	            every alternate frames.
	 */
	ADI_GEMAC_CMD_TX_INTERRUPT_MAX_PERIOD   = 0x05890000u
};


#if defined(ADI_ETHER_SUPPORT_PPS)
/*  \enum ADI_ETHER_GEMAC_PPS_PULSE_MODE
 *
 *  PPS pulse modes in GEMAC controller
 */
typedef enum {
    ADI_ETHER_GEMAC_PPS_PULSE_MODE_SINGLE,         /*!< Single PPS pulse          */
    ADI_ETHER_GEMAC_PPS_PULSE_MODE_FIXED_LEN,      /*!< PPS Pulse of fixed length */
    ADI_ETHER_GEMAC_PPS_PULSE_MODE_CONTINUOUS      /*!< Continuous PPS pulse      */
} ADI_ETHER_GEMAC_PPS_PULSE_MODE;
#endif /* ADI_ETHER_SUPPORT_PPS */


#if defined(ADI_ETHER_SUPPORT_AV)
/*  \enum ADI_ETHER_GEMAC_DMA_CHANNEL
 *
 *  Enum for the DMA Channels in the EMAC controller
 */
typedef enum {
	ADI_ETHER_GEMAC_DMA_CHANNEL_0 = 0u,   /*!< GEMAC DMA Channel 0 */
	ADI_ETHER_GEMAC_DMA_CHANNEL_1 = 1u,   /*!< GEMAC DMA Channel 1 */
	ADI_ETHER_GEMAC_DMA_CHANNEL_2 = 2u    /*!< GEMAC DMA Channel 2 */
} ADI_ETHER_GEMAC_DMA_CHANNEL;


/** \struct __ADI_ETHER_GEMAC_AV_DMA_TX_CONFIG
 *
 *  Configuration to configure Tx AV DMA
 */
typedef struct __ADI_ETHER_GEMAC_AV_DMA_TX_CONFIG
{

	bool      bEnabled;            /*!< true if DMA is enabled else false */
	uint32_t  nNumReservedDesc;    /*!< Number of descriptors to be reserved for this DMA */

} ADI_ETHER_GEMAC_AV_DMA_TX_CONFIG;

/** \struct __ADI_ETHER_GEMAC_AV_DMA_RX_CONFIG
 *
 *  Configuration to configure Rx AV DMA
 */
typedef struct __ADI_ETHER_GEMAC_AV_DMA_RX_CONFIG
{

	bool      bEnabled;            /*!< true if DMA is enabled else false */
	uint32_t  nNumReservedDesc;    /*!< Number of descriptors to be reserved for this DMA */

} ADI_ETHER_GEMAC_AV_DMA_RX_CONFIG;

/** \struct __ADI_ETHER_GEMAC_AV_CHANNEL
 *
 *  Configuration to configure AV Channel
 */
typedef struct __ADI_ETHER_GEMAC_AV_CHANNEL
{
	/*! Enable Credit Based Shaping */
	bool      bEnableCBS;

	/*! Reset the accumulated credit to zero when the credit is positive and there is no frame to transmit */
	bool      bResetCredit;

	/*!
	 * Idle slope credit (when increasing) in bits per cycle (40ns/8ns cycle for 100Mbps and 1000Mbps respectively).
	 *
	 * Max value is 8.0 and 4.0 for 1000Mbps and 100Mbps respectively
	 */
	float     idleSlopeCredit;

	/*!
	 * Send slope credit (when decreasing) in bits per cycle (40ns/8ns cycle for 100Mbps and 1000Mbps respectively.
	 *
	 * Max value is 8.0 and 4.0 for 1000M bps and 100Mbps respectively
	 */
	float     sendSlopeCredit;


	/*! Sets the maximum credit that can be accumulated (should be positive and max 131,072 bits) in bits */
	float     MaxCredit;

	/*! Sets the minimum credit that can be accumulated (should be negative and min -131,072 bits) in bits */
	float     MinCredit;

	/*! Enable/Disable Slot Comparison */
	bool bEnableSlotComparison;

	/*! Enable/Disable Advance Slot Check */
	bool bEnableAdvanceSlotCheck;

    /*! Tx AV DMA Configuration */
	ADI_ETHER_GEMAC_AV_DMA_TX_CONFIG TxDMA;

    /*! Rx AV DMA Configuration */
	ADI_ETHER_GEMAC_AV_DMA_RX_CONFIG RxDMA;

} ADI_ETHER_GEMAC_AV_CHANNEL;

/** \struct __ADI_ETHER_GEMAC_AV_PROFILE
 *
 *  Configuration to configure the AV in Gemac Controller
 */
typedef struct __ADI_ETHER_GEMAC_AV_PROFILE
{
	/*! Channel to which untagged PTP packets (Ethernet payload only) receive are queued */
	ADI_ETHER_GEMAC_DMA_CHANNEL PTPPktChannel;

	/*! Channel to which the received untagged AV control packets are queued */
	ADI_ETHER_GEMAC_DMA_CHANNEL AVControlPktChannel;

	/*! Enables/Disables the Tagged Non_AV Packets to be queued based on the VLAN priority. The channels to which it is queued depends on the AVPriority */
	bool                        bEnableTaggedNonAVQueueing;

	/*! AVPriority controls the Rx channels to which AV packets with give VLAN priority is queued.
	 *
	 *  If only Channel 1 Rx path is enabled then
	 *  	(VLAN Priority >= AVPriority) => Channel 1
	 *  	(Rest all) 					  => Channel 0
	 *
	 *  If both Channel 2 and 1 is enabled then
	 *  	(VLAN Priority >= AVPriority) => Channel 2
	 *  	(VLAN Priority < AVPriority)  => Channel 1
	 *  	(Rest all) 					  => Channel 0
	 */
	uint8_t 					AVPriority;

	/*! EtherType field of the incoming (tagged or untagged) Ethernet frame to detect an AV packet */
	uint16_t                    AVEtherType;


	ADI_ETHER_GEMAC_AV_CHANNEL  Chan1;
	ADI_ETHER_GEMAC_AV_CHANNEL  Chan2;

} ADI_ETHER_GEMAC_AV_PROFILE;

#endif /* ADI_ETHER_SUPPORT_AV */


#if defined(ADI_ETHER_SUPPORT_PTP)
/**
 * \enum ADI_ETHER_GEMAC_PTP_CLK_SRC
 *
 * Specified the Clock source to be used for PTP block
 */
typedef enum __ADI_ETHER_GEMAC_PTP_CLK_SRC
{
	ADI_ETHER_GEMAC_PTP_CLK_SRC_SCLK,					/*!< System Clock*/
	ADI_ETHER_GEMAC_PTP_CLK_SRC_PHY,                  /*!< PHY clock (RMII/RGMII clock) */
	ADI_ETHER_GEMAC_PTP_CLK_SRC_EXT				    /*!< External Clock               */
} ADI_ETHER_GEMAC_PTP_CLK_SRC;


/**
 * \enum ADI_ETHER_GEMAC_PKT_TYPE
 *
 * Enum for different possible Ethernet packets
 */
typedef enum __ADI_ETHER_GEMAC_PKT_TYPE
{
    ADI_ETHER_GEMAC_PKT_TYPE_NONE                      = 0x00000000u,   /*!< None of the packets                 */
    ADI_ETHER_GEMAC_PKT_TYPE_PTP_SYNC                  = 0x00000001u,   /*!< PTP SYNC Packet                     */
    ADI_ETHER_GEMAC_PKT_TYPE_PTP_FOLLOW_UP             = 0x00000002u,   /*!< PTP Follow Up Packet                */
	ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_REQ             = 0x00000004u,   /*!< PTP Delay Request                   */
	ADI_ETHER_GEMAC_PKT_TYPE_PTP_DELAY_RESP            = 0x00000008u,   /*!< PTP Delay Response Packet           */
	ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_REQ            = 0x00000010u,   /*!< PTP PDelay Request Packet           */
	ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP           = 0x00000020u,   /*!< PTP PDelay Responce Packet          */
	ADI_ETHER_GEMAC_PKT_TYPE_PTP_PDELAY_RESP_FOLLOW_UP = 0x00000040u,   /*!< PTP PDeay_Response_Follow_Up Packet */

    ADI_ETHER_GEMAC_PKT_TYPE_PTP_OVER_ETHERNET_FRAME   = 0x00010000u,   /*!< PTP over Ethernet Frames            */
    ADI_ETHER_GEMAC_PKT_TYPE_PTP_OVER_IPV4_UDP         = 0x00020000u,   /*!< PTP over IPv4 UDP                   */
    ADI_ETHER_GEMAC_PKT_TYPE_PTP_OVER_IPV6_UDP         = 0x00040000u,   /*!< PTP over IPv6 UDP                   */
    ADI_ETHER_GEMAC_PKT_TYPE_PTP_V2                    = 0x00080000u,   /*!< PTP v2 format                       */

    ADI_ETHER_GEMAC_PKT_TYPE_ALL                       = 0x10000000u    /*!< All Ehernet packets                 */
} ADI_ETHER_GEMAC_PKT_TYPE;

/**
 * \struct __ADI_ETHER_GEMAC_PTP_CONFIG
 *
 * PTP configuration structure
 */
 typedef struct __ADI_ETHER_GEMAC_PTP_CONFIG
 {
    ADI_ETHER_GEMAC_PTP_CLK_SRC eClkSrc;          /*!< Clock Source to the PTP module */
    uint32_t                    nClkFreq;         /*!< Frequency of the clock source */
    uint32_t                    nPTPClkFreq;      /*!< Frequency fo the PTP clock. The time will be upated with
                  *                                    the resolution of this clock. */
    uint32_t                    RxPktFilter;      /*!< Packet filter to be used for Rx timestamp. This should be
                                                       an ORed value of member of type ADI_ETHER_GEMAC_PKT_TYPE */
    ADI_ETHER_TIME              nInitTime;        /*!< The time to which the PTP clock will be initialized to */
 } ADI_ETHER_GEMAC_PTP_CONFIG;

#endif /* ADI_ETHER_SUPPORT_PTP */


#if defined(ADI_ETHER_SUPPORT_PPS)
/**
 * \struct __ADI_ETHER_GEMAC_PPS_CONFIG
 *
 * PPS configuration structure
 */
typedef struct __ADI_ETHER_GEMAC_PPS_CONFIG
{

    bool bGenerateInterrupt;                   /*!< Enables/Disable interrupt when the PPS starts */
    ADI_ETHER_GEMAC_PPS_PULSE_MODE ePulseMode; /*!< Pulse mode of the PPS signal */
    ADI_ETHER_TIME StartTime;                  /*!< Start time of the PPS signal */
    ADI_ETHER_TIME EndTime;                    /*!< End time of the PPS signal */
    ADI_ETHER_TIME Interval;                   /*!< Interval of the PPS signal */
    ADI_ETHER_TIME Width;                      /*!< Width of the PPS signal */

} ADI_ETHER_GEMAC_PPS_CONFIG;


#endif /* ADI_ETHER_SUPPORT_PPS */


#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /* _ADI_ETHER_GEMAC_H_ */
