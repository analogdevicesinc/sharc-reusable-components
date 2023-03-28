/*******************************************************************************
Copyright (c) 2014 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
* @file: adi_a2b_busconfig.h
* @brief: This file contains macros and structure definitions used for bus configuration 
* @version: $Revision$
* @date: $Date$
* BCF Version - 1.0.0
* Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/

#ifndef _ADI_A2B_BUSCONFIG_H_
#define _ADI_A2B_BUSCONFIG_H_

/*! \addtogroup Network_Configuration Network Configuration
* @{
*/

/** @defgroup Bus_Configuration
*
* This module has structure definitions to describe SigamStudio exported bus configuration/description file.
* The exported bus configuration is interpreted using the structure \ref ADI_A2B_BCD.
*
*/

/*! \addtogroup Bus_Configuration  Bus Configuration
* @{
*/

/*
	Terms Usage
	BCF - Bus Configuration File
	NCF - Node Configuration File

	BCD  - Bus Configuration Data
	NCD  - Node Configuration Data

*/
/*======================= I N C L U D E S =========================*/
#include "a2b/ctypes.h"
/*============= D E F I N E S =============*/

#define ADI_A2B_MAX_DEVICES_PER_NODE				(16)

#define ADI_A2B_MAX_PERI_CONFIG_UNIT_SIZE			(6000)

#define ADI_A2B_TOTAL_PERI_CONFIGS					(4)

#define ADI_A2B_MAX_GPIO_PINS						(0x08)
#define ADI_A2B_MAX_CUST_NODE_ID_LEN				(0x32)


#define A2B_CONF_MAX_NUM_BCD						(10)			/*TODO:Decide on this */

#define A2B_ENABLE_AD242X_BCFSUPPORT				(0x01)
#define A2B_ENABLE_AD241X_BCFSUPPORT				(0x01)
/* I2S Settings  */
#define A2B_ALTERNATE_SYNC							(0x20u)        /*  Drive SYNC Pin Alternating */
#define A2B_PULSE_SYNC								(0x00u)        /*  Drive SYNC Pin for 1 Cycle */

#define A2B_16BIT_TDM								(0x10u)        /*  16-Bit */
#define A2B_32BIT_TDM								(0x00u)        /*  32-Bit */

#define A2B_TDM2									(0x00u)        /*  TDM2 */
#define A2B_TDM4									(0x01u)        /*  TDM4 */
#define A2B_TDM8									(0x02u)        /*  TDM8 */
#define A2B_TDM12									(0x03u)        /*  No slave node support */
#define A2B_TDM16									(0x04u)        /*  TDM16 */
#define A2B_TDM20									(0x05u)        /*  No slave node support */
#define A2B_TDM24									(0x06u)        /*  No slave node support */
#define A2B_TDM32									(0x07u)        /*  TDM32 */

#define A2B_SAMPLERATE_192kHz						(0x06u)        /*  SFF x 4 */
#define A2B_SAMPLERATE_96kHz						(0x05u)        /*  SFF x 2 */
#define A2B_SAMPLERATE_48kHz						(0x00u)        /*  1x SFF */
#define A2B_SAMPLERATE_24kHz						(0x01u)        /*  0.5 x SFF */
#define A2B_SAMPLERATE_12kHz						(0x02u)        /*  0.25x SFF */
#define A2B_SAMPLERATE_RRDIV						(0x03u)        /*  1x SFF */

#define A2B_BCLKRATE_I2SGFG							(0x00)        /*  SFF x 4 */
#define A2B_BCLKRATE_SYNCx2048						(0x01)        /*  SFF x 2 */
#define A2B_BCLKRATE_SYNCx4096						(0x02)        /*  1x SFF */
#define A2B_BCLKRATE_SFFx64							(0x04)        /*  0.5 x SFF */
#define A2B_BCLKRATE_SFFx128						(0x05)        /*  0.25x SFF */
#define A2B_BCLKRATE_SFFx256						(0x06)        /*  1x SFF */

/* Clock rate definition  */
#define A2B_CODEC_CLK_12_288MHz						(0x00u)
#define A2B_CODEC_CLK_24_512MHz						(0x01u)

/* PDM Settings */
#define A2B_0_93									(0xC0u)        /*  0.93 Hz */
#define A2B_3_73									(0x80u)        /*  3.73 Hz */
#define A2B_29_8									(0x20u)        /*  29.8 Hz */
#define A2B_7_46									(0x60u)        /*  7.46 Hz */
#define A2B_59_9									(0x00u)        /*  59.9 Hz */
#define A2B_14_9									(0x40u)        /*  14.9 Hz */
#define A2B_1_86									(0xA0u)        /*  1.86 Hz */

#define A2B_PDM1SLOTS_2								(0x08u)        /*  2 Slots */
#define A2B_PDM1SLOTS_1								(0x00u)        /*  1 Slot */

#define A2B_PDM0SLOTS_2								(0x02u)        /*  2 Slots */
#define A2B_PDM0SLOTS_1								(0x00u)        /*  1 Slot */


#define A2B_PDM_RATE_SFF							(0x00u)        /*  SFF  */ 
#define A2B_PDM_RATE_SFF_DIV_2							(0x01u)        /*  SFF/2  */
#define A2B_PDM_RATE_SFF_DIV_4							(0x02u)        /*  SFF/4  */

/* A2b Slot formats */
#define A2B_UPSLOT_SIZE_8							(0x00u)	              /*  8 Bits */
#define A2B_UPSLOT_SIZE_12							(0x10u)	            /*  12 Bits */
#define A2B_UPSLOT_SIZE_16							(0x20u)	            /*  16 Bits */
#define A2B_UPSLOT_SIZE_20							(0x30u)	            /*  20 Bits */
#define A2B_UPSLOT_SIZE_28							(0x50u)	            /*  28 Bits */
#define A2B_UPSLOT_SIZE_24							(0x40u)	            /*  24 Bits */
#define A2B_UPSLOT_SIZE_32							(0x60u)	            /*  32 Bits */

#define A2B_DNSLOT_SIZE_8							(0x00u)	            /*  8 Bits */
#define A2B_DNSLOT_SIZE_12							(0x01u)	            /*  12 Bits */
#define A2B_DNSLOT_SIZE_16							(0x02u)	            /*  16 Bits */
#define A2B_DNSLOT_SIZE_20							(0x03u)	            /*  20 Bits */
#define A2B_DNSLOT_SIZE_24							(0x04u)	            /*  24 Bits */
#define A2B_DNSLOT_SIZE_28							(0x05u)	            /*  28 Bits */
#define A2B_DNSLOT_SIZE_32							(0x06u)	            /*  32 Bits */


/* Slot Enhance for AD242x  only  */
#define SLOT_0_DISABLED							(0x0)
#define SLOT_1_DISABLED							(0x0)
#define SLOT_2_DISABLED							(0x0)
#define SLOT_3_DISABLED							(0x0)
#define SLOT_4_DISABLED							(0x0)
#define SLOT_5_DISABLED							(0x0)
#define SLOT_6_DISABLED							(0x0)
#define SLOT_7_DISABLED							(0x0)
#define SLOT_8_DISABLED							(0x0)
#define SLOT_9_DISABLED							(0x0)
#define SLOT_10_DISABLED							(0x0)
#define SLOT_11_DISABLED							(0x0)
#define SLOT_12_DISABLED							(0x0)
#define SLOT_13_DISABLED							(0x0)
#define SLOT_14_DISABLED							(0x0)
#define SLOT_15_DISABLED							(0x0)
#define SLOT_16_DISABLED							(0x0)
#define SLOT_17_DISABLED							(0x0)
#define SLOT_18_DISABLED							(0x0)
#define SLOT_19_DISABLED							(0x0)
#define SLOT_20_DISABLED							(0x0)
#define SLOT_21_DISABLED							(0x0)
#define SLOT_22_DISABLED							(0x0)
#define SLOT_23_DISABLED							(0x0)
#define SLOT_24_DISABLED							(0x0)
#define SLOT_25_DISABLED							(0x0)
#define SLOT_26_DISABLED							(0x0)
#define SLOT_27_DISABLED							(0x0)
#define SLOT_28_DISABLED							(0x0)
#define SLOT_29_DISABLED							(0x0)
#define SLOT_30_DISABLED							(0x0)
#define SLOT_31_DISABLED							(0x0)
#define SLOT_0_ENABLED							(0x1)
#define SLOT_1_ENABLED							(0x1)
#define SLOT_2_ENABLED							(0x1)
#define SLOT_3_ENABLED							(0x1)
#define SLOT_4_ENABLED							(0x1)
#define SLOT_5_ENABLED							(0x1)
#define SLOT_6_ENABLED							(0x1)
#define SLOT_7_ENABLED							(0x1)
#define SLOT_8_ENABLED							(0x1)
#define SLOT_9_ENABLED							(0x1)
#define SLOT_10_ENABLED							(0x1)
#define SLOT_11_ENABLED							(0x1)
#define SLOT_12_ENABLED							(0x1)
#define SLOT_13_ENABLED							(0x1)
#define SLOT_14_ENABLED							(0x1)
#define SLOT_15_ENABLED							(0x1)
#define SLOT_16_ENABLED							(0x1)
#define SLOT_17_ENABLED							(0x1)
#define SLOT_18_ENABLED							(0x1)
#define SLOT_19_ENABLED							(0x1)
#define SLOT_20_ENABLED							(0x1)
#define SLOT_21_ENABLED							(0x1)
#define SLOT_22_ENABLED							(0x1)
#define SLOT_23_ENABLED							(0x1)
#define SLOT_24_ENABLED							(0x1)
#define SLOT_25_ENABLED							(0x1)
#define SLOT_26_ENABLED							(0x1)
#define SLOT_27_ENABLED							(0x1)
#define SLOT_28_ENABLED							(0x1)
#define SLOT_29_ENABLED							(0x1)
#define SLOT_30_ENABLED							(0x1)
#define SLOT_31_ENABLED							(0x1)
/* GPIO Mux Options  */
#define A2B_GPIO_0_INPUT							(0x00u)
#define A2B_GPIO_0_OUTPUT							(0x01u)
#define A2B_GPIO_0_DISABLE							(0x02u)

#define A2B_GPIO_1_INPUT							(0x00u)
#define A2B_GPIO_1_OUTPUT							(0x01u)
#define A2B_GPIO_1_DISABLE							(0x02u)
#define A2B_GPIO_1_AS_CLKOUT						(0x03u)

#define A2B_GPIO_2_INPUT							(0x00u)
#define A2B_GPIO_2_OUTPUT							(0x01u)
#define A2B_GPIO_2_DISABLE							(0x02u)
#define A2B_GPIO_2_AS_CLKOUT						(0x03u)

#define A2B_GPIO_3_INPUT							(0x00u)
#define A2B_GPIO_3_OUTPUT							(0x01u)
#define A2B_GPIO_3_DISABLE							(0x02u)
#define A2B_GPIO_3_AS_DTX0							(0x03u)

#define A2B_GPIO_4_INPUT							(0x00u)
#define A2B_GPIO_4_OUTPUT							(0x01u)
#define A2B_GPIO_4_DISABLE							(0x02u)
#define A2B_GPIO_4_AS_DTX1							(0x03u)

#define A2B_GPIO_5_INPUT							(0x00u)
#define A2B_GPIO_5_OUTPUT							(0x01u)
#define A2B_GPIO_5_DISABLE							(0x02u)
#define A2B_GPIO_5_AS_DRX0							(0x03u)
#define A2B_GPIO_5_AS_PDM0							(0x04u)

#define A2B_GPIO_6_INPUT							(0x00u)
#define A2B_GPIO_6_OUTPUT							(0x01u)
#define A2B_GPIO_6_DISABLE							(0x02u)
#define A2B_GPIO_6_AS_DRX1							(0x03u)
#define A2B_GPIO_6_AS_PDM1							(0x04u)

#define A2B_GPIO_7_INPUT							(0x00u)
#define A2B_GPIO_7_OUTPUT							(0x01u)
#define A2B_GPIO_7_DISABLE							(0x02u)
#define A2B_GPIO_7_PDMCLK							(0x03u)

/* GPIOD settings for AD242x only */
#define A2B_MASK_BUSFLAG_0							(0x0)
#define A2B_MASK_BUSFLAG_1							(0x0)
#define A2B_MASK_BUSFLAG_2							(0x0)
#define A2B_MASK_BUSFLAG_3							(0x0)
#define A2B_MASK_BUSFLAG_4							(0x0)
#define A2B_MASK_BUSFLAG_5							(0x0)
#define A2B_MASK_BUSFLAG_6							(0x0)
#define A2B_MASK_BUSFLAG_7							(0x0)

#define A2B_MAP_BUSFLAG_0							(0x1)
#define A2B_MAP_BUSFLAG_1							(0x1)
#define A2B_MAP_BUSFLAG_2							(0x1)
#define A2B_MAP_BUSFLAG_3							(0x1)
#define A2B_MAP_BUSFLAG_4							(0x1)
#define A2B_MAP_BUSFLAG_5							(0x1)
#define A2B_MAP_BUSFLAG_6							(0x1)
#define A2B_MAP_BUSFLAG_7							(0x1)

/* Clkout For AD242x only */
#define A2B_CLKOUT_DIV_2								(0u)
#define A2B_CLKOUT_DIV_4								(1u)
#define A2B_CLKOUT_DIV_6								(2u)
#define A2B_CLKOUT_DIV_8								(3u)
#define A2B_CLKOUT_DIV_10								(4u)
#define A2B_CLKOUT_DIV_12								(5u)
#define A2B_CLKOUT_DIV_14								(6u)
#define A2B_CLKOUT_DIV_16								(7u)
#define A2B_CLKOUT_DIV_18								(8u)
#define A2B_CLKOUT_DIV_20								(9u)
#define A2B_CLKOUT_DIV_22								(10u)
#define A2B_CLKOUT_DIV_24								(11u)
#define A2B_CLKOUT_DIV_26								(12u)
#define A2B_CLKOUT_DIV_28								(13u)
#define A2B_CLKOUT_DIV_30								(14u)
#define A2B_CLKOUT_DIV_32								(15u)
#define A2B_CLKOUT_PREDIV_02							(0x00)
#define A2B_CLKOUT_PREDIV_32							(0x01)

/* I2C rate settings */
#define A2B_I2C_100kHz								(0x00u)
#define A2B_I2C_400kHz								(0x01u)

#define A2B_SFF_RATE_44_1kHz						(0x04u)
#define A2B_SFF_RATE_48_0kHz						(0x00u)

/* PLL Settings */
#define A2B_PLL_SYNC								(0x00u)
#define A2B_PLL_BCLK								(0x04u)

#define A2B_PLL_BCLK_12_288MHz						(0x00u)            /*  1 x  (BCLK=12.288 MHz in TDM8) */
#define A2B_PLL_BCLK_24_576MHZ						(0x01u)            /*  2  x (BCLK=24.576 MHz in TDM16) */
#define A2B_PLL_BCLK_49_152MHZ						(0x02u)            /*  4  x  (BCLK=49.152 MHz for TDM32) */

/* Different discovery types */
#define A2B_SIMPLE_DISCOVERY						(0u)
#define A2B_MODIFIED_DISCOVERY						(1u)
#define A2B_OPTIMIZED_DISCOVERY						(2u)
#define A2B_ADVANCED_DISCOVERY						(3u)

/* Diffrent types of peripheral device  */
#define A2B_AUDIO_SOURCE							(0u)
#define A2B_AUDIO_SINK								(1u)
#define A2B_AUDIO_UNKNOWN							(2u)
#define A2B_AUDIO_SOURCE_SINK						(3u)
#define A2B_AUDIO_HOST								(4u)
#define A2B_GENERIC_I2C_DEVICE						(5u)

/* I2S operational codes */
#define A2B_WRITE_OP								(0u)
#define A2B_READ_OP									(1u)
#define A2B_DEALY_OP								(2u)

#define ENABLED										(1u)
#define DISABLED									(0u)

#define A2B_HIGH									(1u)
#define A2B_LOW										(0u)
#define A2B_IGNORE									(2u)

#define RAISING_EDGE								(0u)
#define FALLING_EDGE								(1u)

#define A2B_READ_FROM_MEM							(0x01)
#define A2B_READ_FROM_GPIO							(0x02)
#define A2B_READ_FROM_COMM_CH						(0x03)

#define ADI_A2B_MASTER								(0u)
#define ADI_A2B_SLAVE								(1u)

#define A2B_BUS_ONLY                				(0u)
#define A2B_LOCAL_TX_ONLY           				(1u)
#define A2B_BUS_AND_LOCAL_TX        				(2u)

/**************************************************** STRUCTURE DEFINITION *****************************/

/*! \struct A2B_CUSTOM_NODE_AUTHENTICATION
	A2B Custom Node identification structure
*/
typedef struct A2B_CUSTOM_NODE_AUTHENTICATION
{
	/*! Enable/Disable Custom Node ID settings */
	a2b_UInt8	bCustomNodeIdAuth;

	/*! Custom node authorization settings to be read from Memory/GPIO */
	a2b_UInt8	nReadFrm;

	/*! Address of device to read from */
	a2b_UInt8	nDeviceAddr;

	/*! Custom Node ID */
	a2b_UInt8	nNodeId[ADI_A2B_MAX_CUST_NODE_ID_LEN];

	/*! Custom Node ID Length */
	a2b_UInt8	nNodeIdLength;

	/*! Width of the memory address to read */
	a2b_UInt8	nReadMemAddrWidth;

	/*! Memory address to read */
	a2b_UInt32	nReadMemAddr;

	/*! GPIO pin values */
	a2b_UInt8	aGpio[ADI_A2B_MAX_GPIO_PINS];

	/*! Timeout in case of authorization via Communication Channel  */
	a2b_UInt16	nTimeOut;
}A2B_CUSTOM_NODE_AUTHENTICATION;

/*! \enum ADI_A2B_PARTNUM
    Possible A2B part numbers
 */
typedef enum
{
 /*!  Enum forAD2410  */
    ADI_A2B_AD2410,
  /*!  Enum forAD2401  */
    ADI_A2B_AD2401,
/*!  Enum forAD2402 */
    ADI_A2B_AD2402 ,
 /*!  Enum for AD2403 */
    ADI_A2B_AD2403,
 /*!  Enum for AD2425 */
    ADI_A2B_AD2425,
 /*!  Enum forAD2421 */
    ADI_A2B_AD2421 ,
 /*!  Enum for AD2422 */
    ADI_A2B_AD2422,
 /*!  Enum for AD2428 */
    ADI_A2B_AD2428,
 /*!  Enum for AD2426 */
    ADI_A2B_AD2426,
 /*!  Enum for AD2427 */
    ADI_A2B_AD2427,
 /*!  Enum for AD2429 */
	ADI_A2B_AD2429,
 /*!  Enum for AD2420 */
	ADI_A2B_AD2420

}ADI_A2B_PARTNUM;

/*! \struct ADI_A2B_PERI_CONFIG_UNIT
	A2B peripheral config unit structure
 */
typedef struct
 {
	/*!  Operational code. Write - 0, Read - 1, Delay - 2*/
	a2b_UInt32 eOpCode;

	/*!  Sub address width */
	a2b_UInt32 nAddrWidth;

	/*!  Sub address */
	a2b_UInt32 nAddr;

	/*!  Data width */
	a2b_UInt32 nDataWidth;

	/*!  Data count */
	a2b_UInt32 nDataCount;

	/*!  Pointer to config data */
	a2b_UInt8* paConfigData;


} ADI_A2B_PERI_CONFIG_UNIT;

/*! \struct ADI_A2B_PERI_DEVICE_CONFIG
	A2B peripheral device config structure
 */typedef struct
 {
	/*!  I2C address of the device to be configured  */
	a2b_UInt32 nDeviceAddress;

	/*!  I2C address of the device to be configured  */
	a2b_UInt8 bActive;

	/*!  ID of the node to which the peripheral is connected  */
	a2b_UInt32 nConnectedNodeID;

	/*!  Number of peripheral config units to be programmed to the device  */
	a2b_UInt32 nNumPeriConfigUnit;

	/*!  Pointer to peripheral configuration unit */
	ADI_A2B_PERI_CONFIG_UNIT* paPeriConfigUnit;


} ADI_A2B_PERI_DEVICE_CONFIG, *ADI_A2B_PERI_DEVICE_CONFIG_PTR;

/*! \struct ADI_A2B_NODE_PERICONFIG
    Peripheral configuration information structure
*/
typedef struct
{
   /*! Array of device configurations  */
   ADI_A2B_PERI_DEVICE_CONFIG  aDeviceConfig[ADI_A2B_MAX_DEVICES_PER_NODE];

   /*! Number of valid configurations */
   a2b_UInt8  nNumConfig;

}ADI_A2B_NODE_PERICONFIG;

/*! \struct ADI_A2B_NODE_PERICONFIG
    Peripheral configuration information structure
*/
typedef struct
{
   /*! Array of nodes and devices attached to it  */
	ADI_A2B_NODE_PERICONFIG  aPeriDownloadTable[A2B_CONF_MAX_NUM_MASTER_NODES][A2B_CONF_MAX_NUM_SLAVE_NODES];

}ADI_A2B_NETWORK_PERICONFIG_TABLE;




	/*! \struct ADI_A2B_COMMON_CONFIG
	   Common configurations for master as well as slave node
    */
	typedef struct
	{
		/*! Down slot size */
		a2b_UInt8  nDwnSlotSize;

		/*! Up slot size  */
		a2b_UInt8  nUpSlotSize;

		/*! Floating point compression for upstream  */
		a2b_UInt8	bUpstreamCompression;

		/*! Floating point compression for downstream  */
		a2b_UInt8	bDwnstreamCompression;

		/*! Enable Upstream */
		a2b_UInt8   bEnableUpstream;

		/*! Enable Downstream */
		a2b_UInt8   bEnableDwnstream;

		/*! Reduce Data Rate on A2B Bus*/
		a2b_UInt8   bEnableReduceRate;

		/*! System level reduced rate factor */
		a2b_UInt8   nSysRateDivFactor;

		/*! Master I2C address - 7 bit */
		a2b_UInt8   nMasterI2CAddr;

		/*! Bus I2C address - 7bit */
		a2b_UInt8   nBusI2CAddr;


    }ADI_A2B_COMMON_CONFIG;

	/*! \struct PERIPHERAL_DEVICE_CONFIG
	   Peripheral device configuration
    */
	typedef struct
	{
		/*! I2C interface status  */
		a2b_UInt8  bI2CInterfaceUse;

		/*! 7 bit I2C address */
		a2b_UInt8  nI2Caddr;

		/*! Device type -audio source/sink/host  */
		a2b_UInt8  eDeviceType;

		/*! Tx0 Pin in use */
		a2b_UInt8  bUseTx0;

		/*! Rx0 Pin in use */
		a2b_UInt8  bUseRx0;

		/*! Tx1 Pin in use */
		a2b_UInt8  bUseTx1;

		/*! Rx1 Pin in use */
		a2b_UInt8  bUseRx1;

		/*! No of Tx0 channels  */
		a2b_UInt8  nChTx0;

		/*! No of Rx0 channels  */
		a2b_UInt8  nChRx0;

		/*! No of Tx1 channels  */
		a2b_UInt8  nChTx1;

		/*! No of Rx1 channels  */
		a2b_UInt8  nChRx1;

		/*! Enable Upstream */
		a2b_UInt8  nNumPeriConfigUnit;

		/*! Pointer to peripheral configuration unit */
		ADI_A2B_PERI_CONFIG_UNIT* paPeriConfigUnit;

    }A2B_PERIPHERAL_DEVICE_CONFIG;



	/*! \struct ADI_A2B_NODE_PERICONFIG_DATA
	    Peripheral configuration data for a node on a chain
	*/
	typedef struct
	{
		/*! Pointer to node device peripheral configurations */
		A2B_PERIPHERAL_DEVICE_CONFIG *apNodePeriDeviceConfig[ADI_A2B_MAX_DEVICES_PER_NODE];

		/*! Number of peripheral devices */
		a2b_UInt8  nNumPeriDevice;

	}ADI_A2B_NODE_PERICONFIG_DATA;


	/*! \struct ADI_A2B_CHAIN_PERICONFIG_DATA
	    Peripheral configuration data for a chain on the network
	*/
	typedef struct
	{
		/*! Pointer to node peripheral configurations */
		ADI_A2B_NODE_PERICONFIG_DATA *apNodePericonfig[A2B_CONF_MAX_NUM_SLAVE_NODES];

		/*! Number of nodes */
		a2b_UInt8  nNumNodes;

	}ADI_A2B_CHAIN_PERICONFIG_DATA;

	/*! \struct ADI_A2B_NETWORK_PERICONFIG
	   Peripheral Configuration Data for entire network including all chains
	*/
	typedef struct
	{
		/*! Array of chain peripheral configuration pointers */
		ADI_A2B_CHAIN_PERICONFIG_DATA *apChainPericonfig[A2B_CONF_MAX_NUM_MASTER_NODES];

		/*! Number of chains */
		a2b_UInt8  nNumChain;

	}ADI_A2B_NETWORK_PERICONFIG;

    /*! \struct ADI_A2B_NETWORK_CONFIG
	   Network configuration & monitoring guidance
    */
	typedef struct
	{
		/*! Discovery mode  */
		a2b_UInt8  eDiscoveryMode;

		/*! Enable/disable Line diagnostics */
 		a2b_UInt8  bLineDiagnostics;

		/*! Auto rediscovery on critical line fault during discovery */
 		a2b_UInt8  bAutoDiscCriticalFault;

		/*! Number of redisocvery attempts on critical faults */
 		a2b_UInt8  nAttemptsCriticalFault;

		/*! Auto rediscovery upon post-discovery line fault */
		a2b_UInt8  bAutoRediscOnFault;

		/*! Number of Peripheral device(s) connected to Target  */
		a2b_UInt8  nNumPeriDevice;

		/*! Interval (in milliseconds) between re-discovery attempt */
		a2b_UInt16  nRediscInterval;
		
		/*! Interval (in milliseconds) between Soft reset and discovery */
		a2b_UInt16  nDiscoveryStartDelay;

		/*! Array of pointers to peripheral configuration unit */
		A2B_PERIPHERAL_DEVICE_CONFIG  *apPeriConfig[ADI_A2B_MAX_DEVICES_PER_NODE];

	}ADI_A2B_NETWORK_CONFIG;


  /*! \struct A2B_NODE_AUTHENTICATION
	Node authentication settings (common for master & slave )
	*/
	typedef struct
	{
		/*! AD2410 manufacturer ID */
		a2b_UInt8  nVendorID;

		/*! AD2410 Silicon version */
		a2b_UInt8  nVersionID;

		/*! AD2410 product ID */
		a2b_UInt8  nProductID;

		/*! AD2410 capability */
		a2b_UInt8  nCapability;

	}A2B_NODE_AUTHENTICATION;

	/*! \struct A2B_CONFIG_CONTROL
	Basic control settings for slave node
	*/
	typedef struct
	{
		/*! I2C interface frequency */
		a2b_UInt8  nI2CFrequency;

		/*! Response cycles  */
		a2b_UInt8  nRespCycle;

		/*! Expected super/audio frame rate */
		a2b_UInt8 nSuperFrameRate;

		/*! Number of broadcast slots */
		a2b_UInt8 nBroadCastSlots;

		/*! Local down slots */
		a2b_UInt8 nLocalDwnSlotsConsume ;

		/*! Local Up slots */
		a2b_UInt8 nLocalUpSlotsContribute ;

		/*! Pass up slots */
		a2b_UInt8 nPassUpSlots ;

		/*! Pass down slots */
		a2b_UInt8 nPassDwnSlots ;

		/*! Enable Down slot consume through mask */
		a2b_UInt8 bUseDwnslotConsumeMasks ;

		/*! Number of slots for contribution */
		a2b_UInt8 nSlotsforDwnstrmContribute ;

		/*! Number of Upslots consumed  */
		a2b_UInt8 nLocalUpSlotsConsume ;

		/*! Offset from the RX Buffer for downstream contribution */
		a2b_UInt8 nOffsetDwnstrmContribute ;

		/*! Offset from the RX Buffer for Upstream contribution */
		a2b_UInt8 nOffsetUpstrmContribute ;

		/*! Downstream slots consume mask */
		a2b_UInt8 anDwnstreamConsumeSlots[32] ;

		/*! Upstream slots consume mask */
		a2b_UInt8 anUpstreamConsumeSlots[32] ;


	}A2B_SLAVE_CONFIG_CONTROL;


	/*! \struct A2B_MASTER_CONFIG_CONTROL
	Basic control settings for master
	*/
	typedef struct 
	{
		/*! Early acknowledge for I2C read/write */
		a2b_UInt8  bI2CEarlyAck;

		/*! Response cycles  */
		a2b_UInt8  nRespCycle;

		/*! PLL time base -SYNC or BCLK */
		a2b_UInt8 nPLLTimeBase;

		/*! BCLK rate - 12.288MHz; 24.576MHz and 49.152MHz  */
		a2b_UInt8 nBCLKRate;

		/*! Pass up slots */
		a2b_UInt8 nPassUpSlots ;

		/*! Upstream slots consume mask */
		a2b_UInt8 nPassDwnSlots ;

		/*! Data control for Master */
		a2b_UInt8 nDatctrl ;

	}A2B_MASTER_CONFIG_CONTROL;

	    /*! \struct A2B_SLAVE_I2S_RATE_CONFIG
	I2S rate settings for AD24xx
	*/
	 typedef struct  
	{
		/*! I2S sampling rate  */
		a2b_UInt8 nSamplingRate;

		/*! Reduce / re-transmit higher frequency samples  */
		a2b_UInt8 bReduce;

		/*! Share A2B bus slots for reduced sampling */
		a2b_UInt8 bShareBusSlot;

		/*! BCLK as a factor of SYNC/SFF for reduced rates */
		a2b_UInt8 nRBCLKRate;

		/*! Reduced rate sync offset */
		a2b_UInt8 nRROffset;

		/*! Enable RR valid bit in LSB */
		a2b_UInt8 bRRValidBitLSB;

		/*! Enable Valid RR bit in Extra bit */
		a2b_UInt8 bRRValidBitExtraBit;

		/*! Enable Valid RR bit in Extra Channel */
		a2b_UInt8 bRRValidBitExtraCh;

		/*! Enable Reduced rate strobe in ADR1/IO1 */
		a2b_UInt8 bRRStrobe;

		/*! Strobe direction High or Low  */
		a2b_UInt8  bRRStrobeDirection;

	}A2B_SLAVE_I2S_RATE_CONFIG;

	/*! \struct A2B_SLAVE_I2S_RATE_CONFIG
	I2S rate settings for AD24xx
	*/
	typedef struct 
	{
		/*! Enable RR valid bit in LSB read/write */
		a2b_UInt8  bRRValidBitLSB;

		/*! Enable Valid RR bit in Extra bit  */
		a2b_UInt8  bRRValidBitExtraBit;

		/*! Enable Valid RR bit in Extra Channel */
		a2b_UInt8 bRRValidBitExtraCh;

		/*! Enable Reduced rate strobe in ADR1/IO1  */
		a2b_UInt8 bRRStrobe;

		/*! Strobe direction High or Low */
		a2b_UInt8 bRRStrobeDirection;

	}A2B_MASTER_I2S_RATE_CONFIG;

	/*! \struct A2B_SLAVE_I2S_SETTINGS
	I2S interface settings for slave AD2410
	*/
	 typedef struct  
	{
		/*! TDM mode  */
		a2b_UInt8 nTDMMode;

		/*! TDM channel size  */
		a2b_UInt8 nTDMChSize;

		/*! SYNC mode- Pulse/50% duty cycle */
		a2b_UInt8 nSyncMode;

		/*! SYNC Polarity- Rising/Falling edge */
		a2b_UInt8 nSyncPolarity;

		/*! Early frame sync status */
		a2b_UInt8 bEarlySync;

		/*! Serial RX on DTX1 Pin */
		a2b_UInt8 bSerialRxOnDTx1;

		/*! SYNC offset with Super frame */
		a2b_UInt8 nSyncOffset;

		/*! DTXn change BCLK edge */
		a2b_UInt8 nBclkTxPolarity;

		/*! DRXn sampling BCLK edge */
		a2b_UInt8 nBclkRxPolarity;

		/*! Interleave slots between TX pins  */
		a2b_UInt8 bTXInterleave;

		/*! Interleave slots between RX pins  */
		a2b_UInt8 bRXInterleave;

		/*! I2S Rate Configuration structure for slave  */
		A2B_SLAVE_I2S_RATE_CONFIG sI2SRateConfig;

		/*! Codec clock rate - applicable only for AD241x  */
		a2b_UInt8  nCodecClkRate;

	}A2B_SLAVE_I2S_SETTINGS;

	/*! \struct A2B_MASTER_I2S_SETTINGS
	I2S interface Settings for master AD2410
	*/
	 typedef struct
	{
		/*! TDM mode  */
		a2b_UInt8 nTDMMode;

		/*! TDM channel size  */
		a2b_UInt8 nTDMChSize;

		/*! SYNC mode - Pulse/50% duty cycle   */
		a2b_UInt8 nSyncMode;

		/*! SYNC Polarity- Rising/Falling edge */
		a2b_UInt8 nSyncPolarity;

		/*! Early frame sync status */
		a2b_UInt8 bEarlySync;

		/*! Serial RX on DTX1 Pin */
		a2b_UInt8 bSerialRxOnDTx1;

		/*! DTXn change BCLK edge */
		a2b_UInt8 nBclkTxPolarity;

		/*! DRXn sampling BCLK edge */
		a2b_UInt8 nBclkRxPolarity;

		/*! Interleave slots between TX pins  */
		a2b_UInt8 bTXInterleave;

		/*! Interleave slots between RX pins  */
		a2b_UInt8 bRXInterleave;

		/*! Transmit data offset in TDM - 0 to 63 */
		a2b_UInt8 nTxOffset;

		/*! Receive data offset in TDM - 0 to 63 */
		a2b_UInt8 nRxOffset;

		/*! TxPin TriState before driving TDM slots */
		a2b_UInt8 bTriStateBeforeTx;

		/*! TxPin Tristate after driving TDM slots */
		a2b_UInt8 bTriStateAfterTx;

		/*! I2S Rate Configuration structure for master  */
		A2B_MASTER_I2S_RATE_CONFIG sI2SRateConfig;

	}A2B_MASTER_I2S_SETTINGS;


	/*! \struct A2B_PDM_SETTINGS
	PDM Settings( Only for slaves )
	*/
	typedef struct
	{
		/*! Number of PDM0 slots */
		a2b_UInt8 nNumSlotsPDM0;

		/*! Number of PDM1 slots */
		a2b_UInt8 nNumSlotsPDM1;

		/*! Use High Pass Filter    */
		a2b_UInt8 bHPFUse;

		/*! PDM rate for AD242x */
		a2b_UInt8 nPDMRate;

		/*! Filter Cut-off frequency */
		a2b_UInt8 nHPFCutOff;

		/*! PDM Inverted Version of Alternate Clock */
		a2b_UInt8 bPDMInvClk;

		/*! PDM Alternate Clock */
		a2b_UInt8 bPDMAltClk;

		/*! PDM0 Falling Edge First */
		a2b_UInt8 bPDM0FallingEdgeFrst;

		/*! PDM1 Falling Edge First */
		a2b_UInt8 bPDM1FallingEdgeFrst;

		/*! PDM Destination */
		a2b_UInt8 ePDMDestination;

	}A2B_SLAVE_PDM_SETTINGS;

	/*! \struct A2B_SLAVE_PIN_MUX_SETTINGS
	GPIO pin multiplication status
	*/
	typedef struct
	{
		/*! GPIO 0 Pin multiplexing */
		a2b_UInt8 bGPIO0PinUsage;

		/*! GPIO 1 Pin multiplexing */
		a2b_UInt8 bGPIO1PinUsage;

		/*! GPIO 2 Pin multiplexing */
		a2b_UInt8 bGPIO2PinUsage;

		/*! GPIO 3 Pin multiplexing */
		a2b_UInt8 bGPIO3PinUsage;

		/*! GPIO 4 Pin multiplexing */
		a2b_UInt8 bGPIO4PinUsage;

		/*! GPIO 5 Pin multiplexing */
		a2b_UInt8 bGPIO5PinUsage;

		/*! GPIO 6 Pin multiplexing */
		a2b_UInt8 bGPIO6PinUsage;

		/*! GPIO 7 Pin multiplexing */
		a2b_UInt8 bGPIO7PinUsage;

	}A2B_SLAVE_PIN_MUX_SETTINGS;


	/*! \struct A2B_MASTER_PIN_MUX_SETTINGS
	GPIO pin multiplication settings  
	*/
	typedef struct 
	{
		/*! GPIO 1 Pin multiplexing */
		a2b_UInt8 bGPIO1PinUsage;

		/*! GPIO 2 Pin multiplexing */
		a2b_UInt8 bGPIO2PinUsage;

		/*! GPIO 3 Pin multiplexing */
		a2b_UInt8 bGPIO3PinUsage;

		/*! GPIO 4 Pin multiplexing */
		a2b_UInt8 bGPIO4PinUsage;

		/*! GPIO 5 Pin multiplexing */
		a2b_UInt8 bGPIO5PinUsage;

		/*! GPIO 6 Pin multiplexing */
		a2b_UInt8 bGPIO6PinUsage;

		/*! GPIO 7 Pin multiplexing */
		a2b_UInt8 bGPIO7PinUsage;

	}A2B_MASTER_PIN_MUX_SETTINGS;



	/*! \struct A2B_SLAVE_OUTPIN_CONFIG
	GPIO output pin configuration
	*/
	typedef struct 
	{
		/*! Data value for GPIO 0 output pin  */
		a2b_UInt8 bGPIO0Val;

		/*! Data value for GPIO 1 output pin  */
		a2b_UInt8 bGPIO1Val;

		/*! Data value for GPIO 2 output pin  */
		a2b_UInt8 bGPIO2Val;

		/*! Data value for GPIO 3 output pin  */
		a2b_UInt8 bGPIO3Val;

		/*! Data value for GPIO 4 output pin  */
		a2b_UInt8 bGPIO4Val;

		/*! Data value for GPIO 5 output pin  */
		a2b_UInt8 bGPIO5Val;

		/*! Data value for GPIO 6 output pin  */
		a2b_UInt8 bGPIO6Val;

		/*! Data value for GPIO 7 output pin  */
		a2b_UInt8 bGPIO7Val;

	    }A2B_SLAVE_OUTPIN_CONFIG;


	/*! \struct A2B_SLAVE_INPUTPIN_INTERRUPT_CONFIG
	GPIO input pin settings for slave node
	*/
	typedef struct
	{
		/*! Enable GPIO 0 Input pin interrupt  */
		a2b_UInt8 bGPIO0Interrupt;

		/*! Interrupt polarity - GPIO 0 Input pin  */
		a2b_UInt8 bGPIO0IntPolarity;

		/*! Enable GPIO 1 Input pin interrupt  */
		a2b_UInt8 bGPIO1Interrupt;

		/*! Interrupt polarity -  GPIO 1 Input pin  */
		a2b_UInt8 bGPIO1IntPolarity;

		/*! Enable GPIO 2 Input pin interrupt  */
		a2b_UInt8 bGPIO2Interrupt;

		/*! Interrupt polarity - GPIO 2 Input pin  */
		a2b_UInt8 bGPIO2IntPolarity;

		/*! Enable GPIO 3 Input pin interrupt  */
		a2b_UInt8 bGPIO3Interrupt;

		/*! Interrupt polarity - GPIO 3 Input pin  */
		a2b_UInt8 bGPIO3IntPolarity;

		/*! Enable GPIO 4 Input pin interrupt  */
		a2b_UInt8 bGPIO4Interrupt;

		/*! Interrupt polarity - GPIO 4 Input pin  */
		a2b_UInt8 bGPIO4IntPolarity;

		/*! Enable GPIO 5 Input pin interrupt  */
		a2b_UInt8 bGPIO5Interrupt;

		/*! Interrupt polarity -  GPIO 5 Input pin  */
		a2b_UInt8 bGPIO5IntPolarity;

		/*! Enable GPIO 6 Input pin interrupt  */
		a2b_UInt8 bGPIO6Interrupt;

		/*! Enable GPIO 6 Input pin interrupt  */
		a2b_UInt8 bGPIO6IntPolarity;

		/*! Enable GPIO 7 Input pin interrupt  */
		a2b_UInt8 bGPIO7Interrupt;

		/*! Interrupt polarity - GPIO 7 Input pin  */
		a2b_UInt8 bGPIO7IntPolarity;

	}A2B_SLAVE_INPUTPIN_INTERRUPT_CONFIG;



	/*! \struct A2B_OUTPIN_VALUE
	GPIO output pin configuration for master 
	*/
	typedef struct 
	{
		/*! Data value for GPIO 1 output pin  */
		a2b_UInt8 bGPIO1Val;

		/*! Data value for GPIO 2 output pin  */
		a2b_UInt8 bGPIO2Val;

		/*! Data value for GPIO 3 output pin  */
		a2b_UInt8 bGPIO3Val;

		/*! Data value for GPIO 4 output pin  */
		a2b_UInt8 bGPIO4Val;

		/*! Data value for GPIO 5 output pin  */
		a2b_UInt8 bGPIO5Val;

		/*! Data value for GPIO 6 output pin  */
		a2b_UInt8 bGPIO6Val;

		/*! Data value for GPIO 7 output pin  */
		a2b_UInt8 bGPIO7Val;

		}A2B_MASTER_OUTPIN_CONFIG;


	/*! \struct A2B_MASTER_INPUTPIN_INTERRUPT_CONFIG
	GPIO input pin interrupt configurations 
	*/
	typedef struct 
	{
		/*! Enable GPIO 1 Input pin interrupt  */
		a2b_UInt8 bGPIO1Interrupt;

		/*! Interrupt polarity - GPIO 1 Input pin  */
		a2b_UInt8 bGPIO1IntPolarity;

		/*! Enable GPIO 2 Input pin interrupt  */
		a2b_UInt8 bGPIO2Interrupt;

		/*! Interrupt polarity - GPIO 2 Input pin  */
		a2b_UInt8 bGPIO2IntPolarity;

		/*! Enable GPIO 3 Input pin interrupt  */
		a2b_UInt8 bGPIO3Interrupt;

		/*! Interrupt polarity - GPIO 3 Input pin  */
		a2b_UInt8 bGPIO3IntPolarity;

		/*! Enable GPIO 4 Input pin interrupt  */
		a2b_UInt8 bGPIO4Interrupt;

		/*! Interrupt polarity - GPIO 4 Input pin  */
		a2b_UInt8 bGPIO4IntPolarity;

		/*! Enable GPIO 5 Input pin interrupt  */
		a2b_UInt8 bGPIO5Interrupt;

		/*! Interrupt polarity -  GPIO 5 Input pin  */
		a2b_UInt8 bGPIO5IntPolarity;

		/*! Enable GPIO 6 Input pin interrupt  */
		a2b_UInt8 bGPIO6Interrupt;

		/*! Interrupt polarity - GPIO 6 Input pin  */
		a2b_UInt8 bGPIO6IntPolarity;

		/*! Enable GPIO 7 Input pin interrupt  */
		a2b_UInt8 bGPIO7Interrupt;

		/*! Interrupt polarity - GPIO 7 Input pin  */
		a2b_UInt8 bGPIO7IntPolarity;

	}A2B_MASTER_INPUTPIN_INTERRUPT_CONFIG;


	/*! \struct A2B_SLAVE_GPIO_SETTINGS
	Slave GPIO Configuration
	*/
	typedef struct
	{
		/*! GPIO Pin multiplex Settings */
		A2B_SLAVE_PIN_MUX_SETTINGS  sPinMuxSettings;

		/*! Input Pin interrupt configuration */
		A2B_SLAVE_INPUTPIN_INTERRUPT_CONFIG   	sPinIntConfig;

		/*! Input Pin interrupt Settings */
		A2B_SLAVE_OUTPIN_CONFIG					sOutPinVal;

		/*! Digital Pin drive strength */
		a2b_UInt8 bHighDriveStrength;

		/*! IRQ Pin Invert */
		a2b_UInt8 bIRQInv;

		/*! Enable tristate when inactive */
		a2b_UInt8 bIRQTriState;

	}A2B_SLAVE_GPIO_SETTINGS;


	/*! \struct A2B_MASTER_GPIO_SETTINGS
	Master GPIO Configuration
	*/
	typedef struct 
	{
		/*! GPIO Pin multiplex Settings */
		A2B_MASTER_PIN_MUX_SETTINGS  sPinMuxSettings;

		/*! Input Pin interrupt configuration */
		A2B_MASTER_INPUTPIN_INTERRUPT_CONFIG   	    sPinIntConfig;

		/*! Input Pin interrupt Settings */
		A2B_MASTER_OUTPIN_CONFIG					sOutPinVal;

		/*! Digital Pin drive strength */
		a2b_UInt8 bHighDriveStrength;

		/*! IRQ Pin Invert */
		a2b_UInt8 bIRQInv;

		/*! Enable tristate when inactive */
		a2b_UInt8 bIRQTriState;

	}A2B_MASTER_GPIO_SETTINGS;


	/*! \struct A2B_SLAVE_INTERRUPT_SETTINGS
	AD2410 Interrupt configuration
	*/
	typedef struct
	{
	    /*! Report Header count error  */
		a2b_UInt8 bReportHDCNTErr;

		/*! Report Data decoding error  */
		a2b_UInt8 bReportDDErr;

		/*! Report Data CRC error  */
		a2b_UInt8 bReportCRCErr;

		/*! Report Data Parity error  */
		a2b_UInt8 bReportDataParityErr;

		/*! Report Data Bus Power error  */
		a2b_UInt8 bReportPwrErr;

		/*! Report bit error count overflow error  */
		a2b_UInt8 bReportErrCntOverFlow;

		/*! Report SRF miss error  */
		a2b_UInt8 bReportSRFMissErr;

		/*! Report SRF crc error  */
		a2b_UInt8 bReportSRFCrcErr;

		/*! Report GPIO  0 Interrupt */
		a2b_UInt8 bReportGPIO0;

		/*! Report GPIO  1 Interrupt */
		a2b_UInt8 bReportGPIO1;

		/*! Report GPIO  2 Interrupt */
		a2b_UInt8 bReportGPIO2;

		/*! Report GPIO  3 Interrupt */
		a2b_UInt8 bReportGPIO3;

		/*! Report GPIO  4 Interrupt */
		a2b_UInt8 bReportGPIO4;

		/*! Report GPIO  5 Interrupt */
		a2b_UInt8 bReportGPIO5;

		/*! Report GPIO  6 Interrupt */
		a2b_UInt8 bReportGPIO6;

		/*! Report GPIO  7 Interrupt */
		a2b_UInt8 bReportGPIO7;

	}A2B_SLAVE_INTERRUPT_SETTINGS;

	/*! \struct A2B_MASTER_INTERRUPT_CONFIG
	AD2410 Interrupt configuration
	*/
	typedef struct
	{
		/*! Report Header count error  */
		a2b_UInt8 bReportHDCNTErr;

		/*! Report Data decoding error  */
		a2b_UInt8 bReportDDErr;

		/*! Report Data CRC error  */
		a2b_UInt8 bReportCRCErr;

		/*! Report Data Parity error  */
		a2b_UInt8 bReportDataParityErr;

		/*! Report Data Bus Power error  */
		a2b_UInt8 bReportPwrErr;

		/*! Report bit error count overflow error  */
		a2b_UInt8 bReportErrCntOverFlow;

		/*! Report SRF miss error  */
		a2b_UInt8 bReportSRFMissErr;

		/*! Report GPIO  1 Interrupt */
		a2b_UInt8 bReportGPIO1;

		/*! Report GPIO  2 Interrupt */
		a2b_UInt8 bReportGPIO2;

		/*! Report GPIO  3 Interrupt */
		a2b_UInt8 bReportGPIO3;

		/*! Report GPIO  4 Interrupt */
		a2b_UInt8 bReportGPIO4;

		/*! Report GPIO  5 Interrupt */
		a2b_UInt8 bReportGPIO5;

		/*! Report GPIO  6 Interrupt */
		a2b_UInt8 bReportGPIO6;

		/*! Report GPIO  7 Interrupt */
		a2b_UInt8 bReportGPIO7;

		/*! Report I2C failure error  */
		a2b_UInt8 bReportI2CErr;

		/*! Report Discovery Completion */
		a2b_UInt8 bDiscComplete;

		/*! Report Interrupt frame error */
		a2b_UInt8 bIntFrameCRCErr;

		/*! Report Interrupt requests  */
		a2b_UInt8 bSlaveIntReq;

	}A2B_MASTER_INTERRUPT_SETTINGS;


	/*! \struct A2B_GPIOD_PIN_CONFIG 
	AD242x GPIOD settings
	*/
	typedef struct
	{
		/*! Enable/Disable GPIO over distance */
		a2b_UInt8 	bGPIODistance;

		/*! Enable/Disable  */
		a2b_UInt8 	bGPIOSignalInv;

		/*! Bus port masks  */
		a2b_UInt8   abBusPortMask[8];

	}A2B_GPIOD_PIN_CONFIG;


	/*! \struct A2B_GPIOD_SETTINGS
	AD242x GPIOD settings
	*/
	typedef struct
	{
		/*! Slave GPIOD0 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD0Config;

		/*! Slave GPIOD1 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD1Config;

		/*! Slave GPIOD2 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD2Config;

		/*! Slave GPIOD3 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD3Config;

		/*! Slave GPIOD4 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD4Config;

		/*! Slave GPIOD5 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD5Config;

		/*! Slave GPIOD6 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD6Config;

		/*! Slave GPIOD7 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD7Config;

	}A2B_SLAVE_GPIOD_SETTINGS;


	/*! \struct A2B_GPIOD_SETTINGS
	AD242x GPIOD settings
	*/
	typedef struct
	{
		/*! Master GPIOD1 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD1Config;

		/*! Master GPIOD2 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD2Config;

		/*! Master GPIOD3 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD3Config;

		/*! Master GPIOD4 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD4Config;

		/*! Master GPIOD5 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD5Config;

		/*! Master GPIOD6 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD6Config;

		/*! Master GPIOD7 configuration structure */
		A2B_GPIOD_PIN_CONFIG sGPIOD7Config;

	}A2B_MASTER_GPIOD_SETTINGS;


	/*! \struct A2B_AD242x_CLKOUT_CONFIG 
	Clock out config for AD242x
	*/
	typedef struct
	{
		/*! Enable Clock1 inversion  */
		a2b_UInt8 	bClk1Inv;

		/*! Clk1 pre-division */
		a2b_UInt8 	bClk1PreDiv;

		/*! CLK1 division factor */
		a2b_UInt8 	bClk1Div;

		/*! Enable Clock 2 inversion  */
		a2b_UInt8 	bClk2Inv;

		/*! Clk2 pre-division */
		a2b_UInt8 	bClk2PreDiv;

		/*! CLK2 division factor */
		a2b_UInt8   bClk2Div;

	}A2B_AD242x_CLKOUT_CONFIG;


	/*! \struct A2B_SLAVE_REGISTER_SETTINGS
	A2B Slave register configuration
	*/
	typedef struct
	{
		/*! Switch control register */
		a2b_UInt8 	nSWCTL;

		/*! Test mode register */
		a2b_UInt8 	nTESTMODE;

		/*! Error count register */
		a2b_UInt8 	nBECCTL;

		/*! Error management register  */
		a2b_UInt8 	nERRMGMT;

		/*! PLL control register  */
		a2b_UInt8   nPLLCTL;

		/*! I2S test register  */
		a2b_UInt8 	nI2STEST;

		/*! Generate error register  */
		a2b_UInt8 	nGENERR;

		/*! Raise interrupt register  */
		a2b_UInt8 	nRAISE;

		/*! Bus monitor configuration */
		a2b_UInt8 	nBMMCFG;

		/*! Clock sustain configuration */
		a2b_UInt8 	nSUSCFG;

		/*! Mailbox 0 control */
		a2b_UInt8 	nMBOX0CTL;

		/*! Mailbox 1 control */
		a2b_UInt8   nMBOX1CTL;
	
        /*! LVDSA TX Control Register */
		a2b_UInt8   nTXACTL;

        /*! LVDSB TX Control Register */
		a2b_UInt8   nTXBCTL;

		/*! Control register */
		a2b_UInt8 	nCONTROL;

	}A2B_SLAVE_REGISTER_SETTINGS;


	/*! \struct A2B_MASTER_REGISTER_SETTINGS
	A2B Master register configuration
	*/
	typedef struct
	{
		/*! Switch control register */
		a2b_UInt8 	nSWCTL;

		/*! PDM control register  */
		a2b_UInt8 	nPDMCTL;

		/*! Test mode register */
		a2b_UInt8 	nTESTMODE;

		/*! Error count register */
		a2b_UInt8 	nBECCTL;

		/*! Error management register  */
		a2b_UInt8 	nERRMGMT;

		/*! I2S test register  */
		a2b_UInt8 	nI2STEST;

		/*! Generate error  */
		a2b_UInt8 	nGENERR;

		/*! Raise interrupt register  */
		a2b_UInt8 	nRAISE;

		/*! Bus monitor configuration */
		a2b_UInt8   nBMMCFG;
		
		/*! PDM control 2 register  */
		a2b_UInt8 	nPDMCTL2;

		/*! PLL control register  */
		a2b_UInt8 	nPLLCTL;

		/*! LVDSA TX Control Register  */
		a2b_UInt8 	nTXACTL;

		/*! LVDSB TX Control Register  */
		a2b_UInt8 	nTXBCTL;

		/*! Control register */
		a2b_UInt8 	nCONTROL;

	}A2B_MASTER_REGISTER_SETTINGS;


/************************************************* NCD DEFINITION **********************************/
	/*! \struct ADI_A2B_SLAVE_NCD
	   Slave configuration
    */
	typedef struct
	{
		/*! Slave node ID */
		a2b_UInt16  					nNodeID;

		/*! Connected Node ID - upstream node */
		a2b_UInt16  					nSrcNodeID;

     /*! Transceiver part number   */
 		ADI_A2B_PARTNUM                     ePartNum;

		/*! Auto-Configuration Enabled */
		a2b_UInt16  					bEnableAutoConfig;

		/*! Authentication settings  */
		A2B_NODE_AUTHENTICATION 	sAuthSettings;

        /*! Custom Node ID Authentication settings  */
		A2B_CUSTOM_NODE_AUTHENTICATION		sCustomNodeAuthSettings;

		/*! Basic configuration & control  */
		A2B_SLAVE_CONFIG_CONTROL	sConfigCtrlSettings;

		/*! A2B I2S Settings */
		A2B_SLAVE_I2S_SETTINGS  	sI2SSettings;

		/*! PDM settings  */
		A2B_SLAVE_PDM_SETTINGS		sPDMSettings;

		/*! GPIO settings  */
		A2B_SLAVE_GPIO_SETTINGS    	sGPIOSettings;

		/*! Interrupt configuration */
		A2B_SLAVE_INTERRUPT_SETTINGS  sInterruptSettings;

		/*! AD242x clock out config */
		A2B_AD242x_CLKOUT_CONFIG  	sClkOutSettings;

		/*! AD242x clock out config */
		A2B_SLAVE_GPIOD_SETTINGS  	sGPIODSettings;

		/*! AD2410 Register configuration - for advanced use */
		A2B_SLAVE_REGISTER_SETTINGS sRegSettings;

		/*! Number of Peripheral device  */
		a2b_UInt8  						 nNumPeriDevice;

		/*! Array of pointers  */
		A2B_PERIPHERAL_DEVICE_CONFIG  *apPeriConfig[ADI_A2B_MAX_DEVICES_PER_NODE];

    }ADI_A2B_SLAVE_NCD;



	/*! \struct ADI_A2B_MASTER_NCD
	   Master configuration 
    */
	typedef struct
	{
		/*! Slave node ID */
		a2b_UInt16  						nNodeID;

		/*! Connected Node ID - upstream node */
		a2b_UInt16  						nSrcNodeID;

		/*! Transceiver part number   */
 		ADI_A2B_PARTNUM                     ePartNum;

		/*! Expected authentication settings  */
		A2B_NODE_AUTHENTICATION 			sAuthSettings;

         /*! Custom Node ID Authentication settings  */
		A2B_CUSTOM_NODE_AUTHENTICATION		sCustomNodeAuthSettings;

		/*! Basic configuration & control  */
		A2B_MASTER_CONFIG_CONTROL			sConfigCtrlSettings;

		/*! A2B I2S Settings */
		A2B_MASTER_I2S_SETTINGS  			sI2SSettings;

		/*! GPIO settings  */
		A2B_MASTER_GPIO_SETTINGS    		sGPIOSettings;

		/*! Interrupt configuration */
		A2B_MASTER_INTERRUPT_SETTINGS  		sInterruptSettings;

		/*! AD242x clock out config */
		A2B_AD242x_CLKOUT_CONFIG  		sClkOutSettings;

		/*! AD242x clock out config */
		A2B_MASTER_GPIOD_SETTINGS  		sGPIODSettings;

		/*! AD2410 Register configuration - for advanced use */
		A2B_MASTER_REGISTER_SETTINGS 	sRegSettings;

    }ADI_A2B_MASTER_NCD;

/***************************************** BCD & CHIAN DEFINITION ************************************/

    /*! \struct ADI_A2B_MASTER_SLAVE_CHAIN_CONFIG  
	   Configuration for one master and associated slaves
    */
	typedef struct
	{
		/*! Slave node ID */
		a2b_UInt8  nNumSlaveNode;

		/*! Pointer to master node configuration  */
		ADI_A2B_MASTER_NCD* pMasterConfig;

		/*! Slave node configuration array */
		ADI_A2B_SLAVE_NCD *apSlaveConfig[A2B_CONF_MAX_NUM_SLAVE_NODES];

	    /*! Common network configuration for one daisy chain */
		ADI_A2B_COMMON_CONFIG sCommonSetting;

	}ADI_A2B_MASTER_SLAVE_CONFIG;


	/*! \struct ADI_A2B_BCD  
	   Bus configuration Data for entire network
    */
	typedef struct
	{
		/*! Number of master node in network */
		a2b_UInt8  nNumMasterNode;

		/*! Pointer to master-slave chains */
		ADI_A2B_MASTER_SLAVE_CONFIG *apNetworkconfig[A2B_CONF_MAX_NUM_MASTER_NODES];

	    /*! Network Configuration */
		ADI_A2B_NETWORK_CONFIG sTargetProperties; 

	}ADI_A2B_BCD;


	/*! \struct ADI_A2B_BCD
		   Super Bus Configuration Structure
	*/
	typedef struct
	{
		/*! Number of Bus Configuration Data */
		a2b_UInt8  nNumBCD;

		/*! Pointer to Bus Configuration array */
		ADI_A2B_BCD *apBusDescription[A2B_CONF_MAX_NUM_BCD];

		/*! Default BCF Index */
		a2b_UInt8 nDefaultBCDIndex ;

	}ADI_A2B_SUPERBCD;


    /*! \struct ADI_A2B_MASTER_SLAVE_CHAIN_CONFIG
	   Configuration for one master and associated slaves
    */
	typedef struct
	{
		/*! Number of slave nodes */
		a2b_UInt8   nNumSlaveNode;

		/*! Compressed info of a2b nodes */
		a2b_UInt8*  pgA2bNetwork;

		/*! Number of bytes  */
		a2b_UInt32  gA2bNetworkLen;

		/*! Pointer to node peripheral configurations */
		ADI_A2B_NODE_PERICONFIG_DATA *apNodePericonfig[A2B_CONF_MAX_NUM_SLAVE_NODES];

	}ADI_A2B_COMPR_MASTER_SLAVE_CONFIG;


	/*! \struct ADI_A2B_COMPR_BCD
	  Compressed Bus configuration Data for entire network
    */
	typedef struct
	{

		/*! Number of master node in network */
		a2b_UInt8  nNumMasterNode;

		/*! Pointer to master-slave chains */
		ADI_A2B_COMPR_MASTER_SLAVE_CONFIG *apNetworkconfig[A2B_CONF_MAX_NUM_MASTER_NODES];

	    /*! Network Configuration */
		ADI_A2B_NETWORK_CONFIG sTargetProperties;

	}ADI_A2B_COMPR_BCD;


	/*! \struct ADI_A2B_BCD
		   Super Bus Configuration Structure
	*/
	typedef struct
	{
		/*! Number of Bus Configuration Data */
		a2b_UInt8  nNumBCD;

		/*! Pointer to Bus Configuration array */
		ADI_A2B_COMPR_BCD *apBusDescription[A2B_CONF_MAX_NUM_BCD];

		/*! Default BCF Index */
		a2b_UInt8 nDefaultBCDIndex ;

	}ADI_A2B_COMPR_SUPERBCD;



/*============= D A T A T Y P E S=============*/


/*============= E X T E R N A L S ============*/

#endif /*_ADI_A2B_BUSCONFIG_H_*/

extern ADI_A2B_COMPR_BCD sCmprBusDescription;
extern ADI_A2B_BCD sBusDescription;
extern ADI_A2B_SUPERBCD sSuperBCD;
extern ADI_A2B_COMPR_SUPERBCD sCmprSuperBCD;

/**
 @}
*/
/**
 @}
*/


/*
**
** EOF: adi_a2b_busconfig.h
**
*/
