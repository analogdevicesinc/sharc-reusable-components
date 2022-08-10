/*******************************************************************************
 *
 * Copyright(c) 2011-2016 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.  By using this software you
 * agree to the terms of the associated Analog Devices License Agreement.
 *
 ******************************************************************************/

/*!
* @file      adi_ether.h
*
* @brief     Ethernet Device Driver Interface
*
* @details
*            Ethernet device driver interface (EDDI) defines the critical
*            functions that are required to be supported by all ethernet
*            device drivers. These primtive EDDI functions are used by the
*            stack interface, all other features supported by the ethernet
*            controllers has to be supplied as controller specific interface
*            functions.
*/

/** @addtogroup  Ethernet_Driver Ethernet Driver Interface
 *  @{
 */

#ifndef __ADI_ETHER_H__
#define __ADI_ETHER_H__

#ifdef _MISRA_RULES
#pragma diag(push)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <adi_types.h>
#include "adi_ether_misra.h"


#if defined(__ADSP215xx__)
#define ADI_ETHER_SUPPORT_AV
#define ADI_ETHER_SUPPORT_PTP
#define ADI_ETHER_SUPPORT_PPS
#define ADI_ETHER_SUPPORT_ALARM
#endif


/*!  Ethenet driver handle */
typedef void* ADI_ETHER_HANDLE;

/*! Ethernet callback function */
typedef void (*ADI_ETHER_CALLBACK_FN) (void*, uint32_t, void*, void*);

/*  \enum ADI_ETHER_RESULT
 *
 *  Generic result codes returned by the ethernet drivers
 */
typedef enum ADI_ETHER_RESULT
{
    ADI_ETHER_RESULT_SUCCESS =  0,          /*!< Ethernet API is successful.       */
    ADI_ETHER_RESULT_NO_MEMORY,             /*!< No memory in the driver.          */
    ADI_ETHER_RESULT_INVALID_SEQUENCE,      /*!< Invalid API execution sequence.   */
    ADI_ETHER_RESULT_INVALID_PARAM,         /*!< Invalid Input Parameter.          */
    ADI_ETHER_RESULT_RESET_FAILED,          /*!< Reset of the controller failed.   */
    ADI_ETHER_RESULT_NULL_BUFFER,           /*!< NULL buffer passed.               */
    ADI_ETHER_RESULT_PHYINIT_FAILED,        /*!< PHY initialization failed.        */
    ADI_ETHER_RESULT_RETRY,                 /*!< Re-try send / receive             */
    ADI_ETHER_RESULT_INVALID_DEVICE_ENTRY,  /*!< Invalid Device Entry              */
    ADI_ETHER_RESULT_FAILED,                /*!< Generic API failure.              */
    ADI_ETHER_RESULT_NOT_SUPPORTED,         /*!< Not supported by the hardware     */
    ADI_ETHER_RESULT_PARAM_NOT_SUPPORTED,
    ADI_ETHER_RESULT_DEVICE_IN_USE


} ADI_ETHER_RESULT;

/*  \enum ADI_ETHER_EVENT
 *
 *   Events codes returned by the ethernet driver
 */
typedef enum  ADI_ETHER_EVENT
{
    ADI_ETHER_EVENT_START     =  0x124,  /*!< First value of ethernet events.   */  /* FIXIT */
    ADI_ETHER_EVENT_FRAME_RCVD,          /*!< One or more frames received.      */
    ADI_ETHER_EVENT_FRAME_XMIT,          /*!< One or more frames tramsmitted.   */
    ADI_ETHER_EVENT_INTERRUPT,           /*!< Ethernet event has occured.       */
    ADI_ETHER_EVENT_PHY_INTERRUPT,       /*!< PHY interrupt has occured.        */
    ADI_ETHER_EVENT_PPS_INTERRUPT,       /*!< PPS interrupt                     */
    ADI_ETHER_EVENT_PPS_ERROR,           /*!< PPS Error occurred                */
    ADI_ETHER_EVENT_ALARM_INTERRUPT,     /*!< Alarm interrupt                   */
    ADI_ETHER_EVENT_ALARM_ERROR          /*!< Alarm error occurred              */
} ADI_ETHER_EVENT;

/*  \enum ADI_ETHER_PHY_INTERRUPT
 *
 *   For PHY interrupt event the argument specifies the actual event, one or
 *   more events could be encoded in the status word.
 */
typedef enum  ADI_ETHER_PHY_STATUS
{
    ADI_ETHER_PHY_LINK_DOWN         = (1UL << 0),   /*!< Link is down.              */
    ADI_ETHER_PHY_LINK_UP           = (1UL << 1),   /*!< Link is up.                */
    ADI_ETHER_PHY_AN_COMPLETE       = (1UL << 2),   /*!< Completed autonegotiation  */
    ADI_ETHER_PHY_10T_HALF_DUPLEX   = (1UL << 3),   /*!< 10Base-T half duplex.      */
    ADI_ETHER_PHY_10T_FULL_DUPLEX   = (1UL << 4),   /*!< 10Base-T full duplex.      */
    ADI_ETHER_PHY_100T_HALF_DUPLEX  = (1UL << 5),   /*!< 100Base-T half duplex.     */
    ADI_ETHER_PHY_100T_FULL_DUPLEX  = (1UL << 6),   /*!< 100Base-T full duplex.     */
    ADI_ETHER_PHY_LOOPBACK          = (1UL << 7),   /*!< Loopback on.               */
    ADI_ETHER_PHY_1000T_HALF_DUPLEX = (1UL << 8),   /*!< 100Base-T full duplex.     */
    ADI_ETHER_PHY_1000T_FULL_DUPLEX = (1UL << 9)   /*!< 100Base-T full duplex.     */
} ADI_ETHER_PHY_STATUS;

/* \enum ADI_ETHER_ARG_PHY_SPEED
 *
 * Argument for the ADI_ETHER_CMD_SET_PHY_SPEED command
 */
typedef enum ADI_ETHER_ARG_PHY_SPEED
{
	ADI_ETHER_ARG_PHY_SPEED_AUTO_NEGOTIATE,      /*!< Auto-negotiate the speed */
	ADI_ETHER_ARG_PHY_SPEED_10T_HALF_DUPLEX,     /*!< 10Base-T   half duplex.      */
	ADI_ETHER_ARG_PHY_SPEED_10T_FULL_DUPLEX,     /*!< 10Base-T   full duplex.      */
	ADI_ETHER_ARG_PHY_SPEED_100T_HALF_DUPLEX,    /*!< 100Base-T  half duplex.      */
	ADI_ETHER_ARG_PHY_SPEED_100T_FULL_DUPLEX,    /*!< 100Base-T  full duplex.      */
	ADI_ETHER_ARG_PHY_SPEED_1000T_HALF_DUPLEX,   /*!< 1000Base-T half duplex.      */
	ADI_ETHER_ARG_PHY_SPEED_1000T_FULL_DUPLEX    /*!< 1000Base-T full duplex.      */
} ADI_ETHER_ARG_PHY_SPEED;


/* \enum ADI_ETHER_BUFFER_FLAG
 *
 * Flag used to set Flag member of the ADI_ETHER_BUFFER structure
 */
typedef enum
{
	ADI_ETHER_BUFFER_FLAG_TX_TIMESTAMP_EN    = 0x00000001u  /*!< Enable Timestamp for TX */

} ADI_ETHER_BUFFER_FLAG;

/* \enum ADI_ETHER_BUFFER_STATUS
 *
 * Flag used to set Status member of the ADI_ETHER_BUFFER structure
 */
typedef enum
{
	ADI_ETHER_BUFFER_STATUS_TIMESTAMP_AVAIL  = 0x00000001u  /*!< Timestamp member of the ADI_ETHER_BUFFER contains a valid timestamp */

} ADI_ETHER_BUFFER_STATUS;

/**
 * \struct __ADI_ETHER_TIME
 *
 * TIME structure used to define time in the ethernet module
 */
 typedef struct __ADI_ETHER_TIME {
     uint16_t HSecond;                 /*!< Upper-16bit of the 48bit-Second part of time */
     uint32_t LSecond;                 /*!< Lower-32bit of the 48bit-Second part of time */
     uint32_t NanoSecond;              /*!< Nano-Second part of time */
 } ADI_ETHER_TIME;


 /* \enum ADI_ETHER_CMD
 *
 * The various command supported by the ethernet driver
 */
enum ADI_ETHER_CMD
{
	/*! This command is used to mark the end of the command array */
	ADI_ETHER_CMD_END_OF_ARRAY    = 0x00000000u,

	/*! Set the phy speed. Argument should of type ADI_ETHER_ARG_PHY_SPEED */
	ADI_ETHER_CMD_SET_PHY_SPEED
};

#define ADI_ETHER_DRIVER_MEM  (20)            /*!< Driver memory - primarily used for DMA */

/**
 * \struct __ADI_ETHER_BUFFER
 *
 * Ethernet buffer structure
 */
typedef struct __ADI_ETHER_BUFFER
{
    char_t   Reserved[ADI_ETHER_DRIVER_MEM];  /*!< Reserved for physical device */
    void     *Data;                           /*!< Pointer to data.             */
    uint32_t ElementCount;                    /*!< Data element count.          */
    uint32_t ElementWidth;                    /*!< Data element width in bytes. */  /* BC */
    void*    CallbackParameter;               /*!< Callback flag/pArg value.    */
    uint32_t ProcessedFlag;                   /*!< Buffer processed flag.       */
    uint32_t ProcessedElementCount;           /*!< Actual bytes read or sent.   */
    struct   __ADI_ETHER_BUFFER  *pNext;      /*!< Next buffer pointer.         */
    void     *PayLoad;                        /*!< Pointer to IP Payload.       */  /* BC */
    uint16_t IPHdrChksum;                     /*!< IP header checksum.          */
    uint16_t IPPayloadChksum;                 /*!< IP header & payload checksum.*/
    uint16_t StatusWord;                      /*!< The frame status word.       */  /* BC */
    void*    x;                               /*!< Network interface.           */  /* BC */

#ifdef ADI_ETHER_SUPPORT_PTP
    ADI_ETHER_TIME TimeStamp;                 /*!< Timestamp of the packet */
#endif

#ifdef ADI_ETHER_SUPPORT_AV
    uint16_t nSlot;                           /*!< Slot number for AV transmission */
    uint16_t nChannel;						  /*!< Channel number for AV transmission */
#endif
    uint32_t Status;                          /*!< Status for the Buffer - ORed value of ADI_ETHER_BUFFER_STATUS */
    uint32_t Flag;                            /*!< Flag for the Buffer  - ORed value of ADI_ETHER_BUFFER_FLAG */

} ADI_ETHER_BUFFER;

/*
 * \struct ADI_ETHER_STATISTICS_COUNTS
 *
 *  Ethernet statistical counters
 */
typedef struct ADI_ETHER_STATISTICS_COUNTS
{
    uint64_t cEMAC_RX_CNT_OK;      /*!< RX frame successful count.             */
    uint64_t cEMAC_RX_CNT_FCS;     /*!< RX frame FCS failure count.            */
    uint64_t cEMAC_RX_CNT_ALIGN;   /*!< RX alignment error count.              */
    uint64_t cEMAC_RX_CNT_OCTET;   /*!< RX octets successfully received count. */
    uint64_t cEMAC_RX_CNT_LOST;    /*!< MAC sublayer error RX frame count.     */
    uint64_t cEMAC_RX_CNT_UNI;     /*!< Unicast RX frame count.                */
    uint64_t cEMAC_RX_CNT_MULTI;   /*!< Multicast RX frame count.              */
    uint64_t cEMAC_RX_CNT_BROAD;   /*!< Broadcast RX frame count.              */
    uint64_t cEMAC_RX_CNT_IRL;     /*!< RX frame in range error count.         */
    uint64_t cEMAC_RX_CNT_ORL;     /*!< RX frame out of range error count.     */
    uint64_t cEMAC_RX_CNT_LONG;    /*!< RX frame too long count.               */
    uint64_t cEMAC_RX_CNT_MACCTL;  /*!< MAC control RX frame count.            */
    uint64_t cEMAC_RX_CNT_OPCODE;  /*!< Unsupported op-code RX frame count.    */
    uint64_t cEMAC_RX_CNT_PAUSE;   /*!< MAC control pause RX frame count.      */
    uint64_t cEMAC_RX_CNT_ALLF;    /*!< Overall RX frame count.                */
    uint64_t cEMAC_RX_CNT_ALLO;    /*!< Overall RX octet count.                */
    uint64_t cEMAC_RX_CNTD;        /*!< Type/Length consistent RX frame count. */
    uint64_t cEMAC_RX_CNT_SHORT;   /*!< RX frame fragment count,count x < 64.  */
    uint64_t cEMAC_RX_CNT_EQ64;    /*!< Good RX frames,count x = 64.           */
    uint64_t cEMAC_RX_CNT_LT128;   /*!< Good RX frames,count x 64 <= x < 128.  */
    uint64_t cEMAC_RX_CNT_LT256;   /*!< Good RX frames,count x 128 <= x < 256. */
    uint64_t cEMAC_RX_CNT_LT512;   /*!< Good RX frames,count x 256 <= x < 512. */
    uint64_t cEMAC_RX_CNT_LT1024;  /*!< Good RX frames,count x 512 <= x < 1024 */
    uint64_t cEMAC_RX_CNT_EQ1024;  /*!< Good RX frames,count x >= 1024.        */
    uint64_t cEMAC_TX_CNT_OK;      /*!< TX frame successful count.             */
    uint64_t cEMAC_TX_CNT_SCOLL;   /*!< Successful tx frames after single collision.    */
    uint64_t cEMAC_TX_CNT_MCOLL;   /*!< TX frames successful after multiple collisions. */
    uint64_t cEMAC_TX_CNT_OCTET;   /*!< TX octets successfully received count.*/
    uint64_t cEMAC_TX_CNT_DEFER;   /*!< TX frame delayed due to busy count.   */
    uint64_t cEMAC_TX_CNT_LATE;    /*!< Late TX collisions count.             */
    uint64_t cEMAC_TX_CNT_ABORTC;  /*!< TX frame failed due to Excessive collisions.    */
    uint64_t cEMAC_TX_CNT_LOST;    /*!< Internal MAC sublayer error TX frame count.     */
    uint64_t cEMAC_TX_CNT_CRS;     /*!< Carrier sense deasserted during TX frame count. */
    uint64_t cEMAC_TX_CNT_UNI;     /*!< Unicast TX frame count.                */
    uint64_t cEMAC_TX_CNT_MULTI;   /*!< Multicast TX frame count.              */
    uint64_t cEMAC_TX_CNT_BROAD;   /*!< Broadcast TX frame count.              */
    uint64_t cEMAC_TX_CNT_EXDEF;   /*!< TX frames with excessive deferral count.       */
    uint64_t cEMAC_TX_CNT_MACCTL;  /*!< MAC control TX frame count.            */
    uint64_t cEMAC_TX_CNT_ALLF;    /*!< Overall TX frame count.                */
    uint64_t cEMAC_TX_CNT_ALLO;    /*!< Overall TX octet count.                */
    uint64_t cEMAC_TX_CNT_EQ64;    /*!< Good TX frames,count x = 64.           */
    uint64_t cEMAC_TX_CNT_LT128;   /*!< Good TX frames,count x  64 <= x < 128. */
    uint64_t cEMAC_TX_CNT_LT256;   /*!< Good TX frames,count x 128 <= x < 256. */
    uint64_t cEMAC_TX_CNT_LT512;   /*!< Good TX frames,count x 256 <= x < 512. */
    uint64_t cEMAC_TX_CNT_LT1024;  /*!< Good TX frames,count x 512 <= x < 1024 */
    uint64_t cEMAC_TX_CNT_EQ1024;  /*!< Good TX frames,count x >= 1024.        */
    uint64_t cEMAC_TX_CNT_ABORT;   /*!< Total TX frames aborted count.         */
} ADI_ETHER_STATISTICS_COUNTS;



/*
 * \struct ADI_ETHER_MEM
 *
 * Ethernet driver initialization structure
 */
typedef struct ADI_ETHER_MEM
{
    uint8_t   *pRecvMem;       /*!< Driver memory used for rx operations.     */
    uint32_t   RecvMemLen;     /*!< Supplied memory in bytes for receive.     */
    uint8_t   *pTransmitMem;   /*!< Driver memory used for tx operations.     */
    uint32_t   TransmitMemLen; /*!< Supplied memory in bytes for transmit.    */
    uint8_t   *pBaseMem;       /*!< Driver base memory used for statistics.   */
    uint32_t   BaseMemLen;     /*!< Supplied base memory in bytes.            */

} ADI_ETHER_MEM;

/*
 * \struct ADI_ETHER_CMD_ARG
 *
 * Ethernet Generic command-arg structure. The arg will depend on the command
 *
 */
typedef struct ADI_ETHER_CMD_ARG
{
	uint32_t          cmd;
	void*             arg;
} ADI_ETHER_CMD_ARG;


/*
 * \struct ADI_ETHER_DEV_INIT
 *
 * Ethernet driver initialization structure
 */
typedef struct ADI_ETHER_DEV_INIT
{
    bool               Cache;          /*!< Supplied Ethernet Frame is cached or not */
    ADI_ETHER_MEM     *pEtherMemory;   /*!< Supply memory to the driver              */
    ADI_ETHER_CMD_ARG *pCmdArgArray;   /*!< Pointer to an array of CMD-ARG structure
    								        (Can be Null if none need to be provided) */

} ADI_ETHER_DEV_INIT;

/*
 * Enum for Modules to be used for adi_ether_ModuleIO
 */
typedef enum ADI_ETHER_MODULE
{
    ADI_ETHER_MODULE_PTP,       /* PTP   Module       */
    ADI_ETHER_MODULE_PPS,       /* PPS   Module       */
    ADI_ETHER_MODULE_ALARM,     /* Alarm Module       */
    ADI_ETHER_MODULE_AV         /* Audio-Video Module */

} ADI_ETHER_MODULE;

/* Enum for the functions provided by the modules */
typedef enum ADI_ETHER_MODULE_FUNC
{
    ADI_ETHER_MODULE_FUNC_PTP_CFG                = ((ADI_ETHER_MODULE_PTP   << 16u) + 0),
    ADI_ETHER_MODULE_FUNC_PTP_EN                 = ((ADI_ETHER_MODULE_PTP   << 16u) + 1),
    ADI_ETHER_MODULE_FUNC_PTP_GET_CUR_TIME       = ((ADI_ETHER_MODULE_PTP   << 16u) + 2),
    ADI_ETHER_MODULE_FUNC_PTP_UPDATE_FREQ        = ((ADI_ETHER_MODULE_PTP   << 16u) + 3),
    ADI_ETHER_MODULE_FUNC_PTP_APPLY_TIME_OFFSET  = ((ADI_ETHER_MODULE_PTP   << 16u) + 4),

    ADI_ETHER_MODULE_FUNC_PPS_CFG                = ((ADI_ETHER_MODULE_PPS   << 16u) + 0),
    ADI_ETHER_MODULE_FUNC_PPS_EN                 = ((ADI_ETHER_MODULE_PPS   << 16u) + 1),

    ADI_ETHER_MODULE_FUNC_ALARM_CFG              = ((ADI_ETHER_MODULE_ALARM << 16u) + 0),
    ADI_ETHER_MODULE_FUNC_ALARM_EN               = ((ADI_ETHER_MODULE_ALARM << 16u) + 1),

    ADI_ETHER_MODULE_FUNC_AV_SET_PROFILE         = ((ADI_ETHER_MODULE_AV    << 16u) + 0)

} ADI_ETHER_MODULE_FUNC;

/**
 * \struct driver entry point
 */
typedef struct ADI_ETHER_DRIVER_ENTRY
{
    ADI_ETHER_RESULT (*adi_ether_Open)( struct ADI_ETHER_DRIVER_ENTRY* const pEntryPoint,
                                        ADI_ETHER_DEV_INIT  *   const pDeviceInit,
                                        ADI_ETHER_CALLBACK_FN   const pfCallback,
                                        ADI_ETHER_HANDLE*       const phDevice,
                                        void*                   const pUsrPtr );

    ADI_ETHER_RESULT (*adi_ether_Read)(ADI_ETHER_HANDLE phDevice,
                                       ADI_ETHER_BUFFER *pBuffer);

    ADI_ETHER_RESULT (*adi_ether_Write)(ADI_ETHER_HANDLE phDevice,
                                        ADI_ETHER_BUFFER *pBuffer);

    ADI_ETHER_RESULT (*adi_ether_Close)(ADI_ETHER_HANDLE phDevice);

    bool             (*adi_ether_GetLinkStatus)(ADI_ETHER_HANDLE pEtherHandle);

    ADI_ETHER_RESULT (*adi_ether_AddMulticastFilter)(ADI_ETHER_HANDLE pEtherHandle,
                                                     const uint32_t MultiCastGroupAddr);

    ADI_ETHER_RESULT (*adi_ether_DelMulticastFilter)(ADI_ETHER_HANDLE pEtherHandle,
                                                     const uint32_t MultiCastGroupAddr);

    ADI_ETHER_RESULT (*adi_ether_GetBufferPrefix)(ADI_ETHER_HANDLE pEtherHandle,
                                                  uint32_t* const  pBufferPrefix);

    ADI_ETHER_RESULT (*adi_ether_GetMACAddress)(ADI_ETHER_HANDLE pEtherHandle,
                                                uint8_t *pMacAddress);

    ADI_ETHER_RESULT (*adi_ether_SetMACAddress)(ADI_ETHER_HANDLE pEtherHandle,
                                                const uint8_t *pMacAddress);

    ADI_ETHER_RESULT (*adi_ether_EnableMAC)(ADI_ETHER_HANDLE phDevice);

    ADI_ETHER_RESULT (*adi_ether_ModuleIO)(ADI_ETHER_HANDLE      const phDevice,
    									   ADI_ETHER_MODULE      const ModuleID,
    									   ADI_ETHER_MODULE_FUNC const Func,
    									   void*                       arg0,
                                           void*                       arg1,
                                           void*                       arg2
                                           );

    ADI_ETHER_RESULT (*adi_ether_SetSrcAddrFilt)(ADI_ETHER_HANDLE phDevice,
                                                 const uint32_t IpAddr,
                                                 const uint8_t IpMaskBits,
                                                 const bool invert);

    ADI_ETHER_RESULT (*adi_ether_SetSrcAddrFiltEnable)(ADI_ETHER_HANDLE phDevice,
                                                       const bool enable);

    ADI_ETHER_RESULT (*adi_ether_SetDstPortFilt)(ADI_ETHER_HANDLE phDevice,
                                                 const uint16_t Port,
                                                 const bool udp,
                                                 const bool invert);

    ADI_ETHER_RESULT (*adi_ether_SetDstPortFiltEnable)(ADI_ETHER_HANDLE phDevice,
                                                       const bool enable);

} ADI_ETHER_DRIVER_ENTRY;

/* Opens the Ethernet Device Driver */
ADI_ETHER_RESULT adi_ether_Open(
                                ADI_ETHER_DRIVER_ENTRY* const pEntryPoint,
                                ADI_ETHER_DEV_INIT*     const pDeviceInit,
                                ADI_ETHER_CALLBACK_FN   const pfCallback,
                                ADI_ETHER_HANDLE*       const phDevice,
                                void *                  const pUsrPtr
                                );

/* Submits single or list of buffers for receiving ethernet packets */
ADI_ETHER_RESULT adi_ether_Read (
                                 ADI_ETHER_HANDLE const hDevice,
                                 ADI_ETHER_BUFFER *pBuffer
                                 );

/* Submits single or list of buffers for transmission over ethernet */
ADI_ETHER_RESULT adi_ether_Write (
                                  ADI_ETHER_HANDLE const hDevice,
                                  ADI_ETHER_BUFFER *pBuffer
                                  );

/* Close the ethernet device driver */
ADI_ETHER_RESULT adi_ether_Close(ADI_ETHER_HANDLE hDevice);


/* Return the Link Status */
bool adi_ether_GetLinkStatus(ADI_ETHER_HANDLE hDevice);

/* Configures and Enables MAC and PHY */
ADI_ETHER_RESULT adi_ether_EnableMAC(ADI_ETHER_HANDLE hDevice);


/* Add multicast group */
ADI_ETHER_RESULT adi_ether_AddMulticastFilter (
                                               ADI_ETHER_HANDLE hDevice,
                                               const uint32_t MultiCastGroupAddr
                                               );

/* Delete multicast group */
ADI_ETHER_RESULT adi_ether_DelMulticastFilter (
                                               ADI_ETHER_HANDLE hDevice,
                                               const uint32_t MultiCastGroupAddr
                                               );

/* Get the MAC address of the interface */
ADI_ETHER_RESULT adi_ether_GetMACAddress (
                                          ADI_ETHER_HANDLE hDevice,
                                          uint8_t *pMacAddress
                                          );

/* Set the MAC address of the interface */
ADI_ETHER_RESULT adi_ether_SetMACAddress (
                                          ADI_ETHER_HANDLE hDevice,
                                          const uint8_t *pMacAddress
                                          );

/* Get the Buffer Prefix */
ADI_ETHER_RESULT adi_ether_GetBufferPrefix (
                                            ADI_ETHER_HANDLE hDevice,
                                            uint32_t* const  pBufferPrefix
                                            );

ADI_ETHER_RESULT adi_ether_SetSrcAddrFilt(ADI_ETHER_HANDLE phDevice,
                                          const uint32_t IpAddr,
                                          const uint8_t IpMaskBits,
                                          const bool invert);

ADI_ETHER_RESULT adi_ether_SetSrcAddrFiltEnable(ADI_ETHER_HANDLE phDevice,
                                                const bool enable);

ADI_ETHER_RESULT adi_ether_SetDstPortFilt(ADI_ETHER_HANDLE phDevice,
                                          const uint16_t Port,
                                          const bool udp,
                                          const bool invert);

ADI_ETHER_RESULT adi_ether_SetDstPortFiltEnable(ADI_ETHER_HANDLE phDevice,
                                                const bool enable);

#if defined(ADI_ETHER_SUPPORT_PTP)

/* Configure the PTP (Precision Time Protocol) */
ADI_ETHER_RESULT adi_ether_ptp_Config (
                                        ADI_ETHER_HANDLE        const hDevice,
                                        void*                   const pConfig,
                                        ADI_ETHER_CALLBACK_FN   const pfCallback
                                        );

/* Enable/Disable PTP */
ADI_ETHER_RESULT adi_ether_ptp_Enable (
                                       ADI_ETHER_HANDLE const hDevice,
                                       bool             const bEnable
                                       );

/* Get the current time of the PTP module */
ADI_ETHER_RESULT adi_ether_ptp_GetCurrentTime (
                                               ADI_ETHER_HANDLE  const hDevice,
                                               ADI_ETHER_TIME*   const pTime
                                               );

/* Update the input clock frequency. */
#ifdef LEGACY_EMAC_PTP
ADI_ETHER_RESULT adi_ether_ptp_UpdateClkFreq (
                                              ADI_ETHER_HANDLE  const hDevice,
                                              uint32_t          const nClkFreq
                                              );
#else
ADI_ETHER_RESULT adi_ether_ptp_UpdateClkFreq (
                                              ADI_ETHER_HANDLE  const hDevice,
                                              float32_t *       const nClkFreq
                                              );
#endif

/* Apply an offset to the PTP time */
ADI_ETHER_RESULT adi_ether_ptp_ApplyTimeOffset (
                                                ADI_ETHER_HANDLE const hDevice,
                                                ADI_ETHER_TIME*  const pTime,
                                                bool             const bAddTime
                                                );
#endif /* ADI_ETHER_SUPPORT_PTP */

#if defined(ADI_ETHER_SUPPORT_AV)
/* Set AV Profile */
ADI_ETHER_RESULT adi_ether_av_SetProfile (
                                          ADI_ETHER_HANDLE const hDevice,
                                          void*            const pAVProfile
                                          );
#endif /* ADI_ETHER_SUPPORT_AV */

#if defined(ADI_ETHER_SUPPORT_PPS)

/* Configure the PPS */
ADI_ETHER_RESULT adi_ether_pps_Config (
                                       ADI_ETHER_HANDLE        const hDevice,
                                       uint32_t                const nDeviceID,
                                       void*                   const pPPSConfig,
                                       ADI_ETHER_CALLBACK_FN   const pfCallback
                                       );
/* Enable/Disable PPS */
ADI_ETHER_RESULT adi_ether_pps_Enable (
                                      ADI_ETHER_HANDLE const hDevice,
                                      uint32_t         const nDeviceID,
                                      bool             const bEnable
                                      );
#endif /* ADI_ETHER_SUPPORT_PPS */

#if defined(ADI_ETHER_SUPPORT_ALARM)

/* Configure the Alarm */
ADI_ETHER_RESULT adi_ether_alarm_Config (
                                         ADI_ETHER_HANDLE        const hDevice,
                                         uint32_t                const nDeviceID,
                                         ADI_ETHER_TIME*         const pTime,
                                         ADI_ETHER_CALLBACK_FN   const pfCallback
                                         );

/* Enable/Disable Alarm */
ADI_ETHER_RESULT adi_ether_alarm_Enable (
                                         ADI_ETHER_HANDLE const hDevice,
                                         uint32_t         const nDeviceID,
                                         bool             const bEnable
                                         );
#endif /* ADI_ETHER_SUPPORT_ALARM */



#ifdef __cplusplus
}
#endif

/*@}*/


#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

#endif  /* __ADI_ETHER_H__  */
