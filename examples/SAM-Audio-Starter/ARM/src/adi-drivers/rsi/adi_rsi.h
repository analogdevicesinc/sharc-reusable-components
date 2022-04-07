/*********************************************************************************

Copyright(c) 2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

*********************************************************************************/

/** @addtogroup RSI_Driver RSI Device Driver
 *  @{
 */

/*!
 * @file    adi_rsi.h
 * @brief   RSI device driver definitions
 * @version $Revision: 62804 $
 * @date    $Date: 2019-11-11 00:47:59 -0500 (Mon, 11 Nov 2019) $
 *
 * @details
 *            This is the primary header file for the RSI driver, which contains the
 *            API declarations, data and constant definitions used in the APIs
 */

#ifndef __ADI_RSI_H__
#define __ADI_RSI_H__

#include <stdint.h>
#include <stdbool.h>
#include <limits.h> /* for USHRT_MAX */
#include <services/int/adi_int.h> /* for ADI_CALLBACK */
#include <sys/platform.h>

#if !defined(__BYTE_ADDRESSING__) && defined(__ADSPSHARC__)
	#error "Only Byte addressing mode is supported"
#endif 

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! A device handle used in all API functions to identify the RSI device. */
typedef void *ADI_RSI_HANDLE;

#if defined(__ADSPBF707_FAMILY__) || defined(__ADSPSC589_FAMILY__) || defined (__ADSPSC573_FAMILY__)

/*! Maximum number of bytes that the RSI can transfer in a single operation. */
#define ADI_RSI_MAX_TRANSFER_BYTES 65536u

/**
 * Maximum SD/MMC bus width supported by the RSI hardware.
 */
#define ADI_RSI_MAX_BUS_WIDTH 8u

#define ADI_RSI_CMDFLAG_CRCDIS        (BITM_MSI_CMD_CHKRESPCRC) /* check response CRC */
#define ADI_RSI_CMDFLAG_CHKBUSY       (BITM_MSI_CMD_WTPRIVDATA) /* wait for completion before sending cmd */

#define ADI_RSI_ENABLE_CLKSEN         (1u)    /*!< Clocks Enable */
#define ADI_RSI_ENABLE_DATPUP         (2u)    /*!< Data Pin Pull-Ups */
#define ADI_RSI_ENABLE_DAT3PUP        (4u)   /*!< Data Pin 3 Pull-Up */
#define ADI_RSI_ENABLE_PUPDATALL      (6u) /*!< Data Pin Pull-Ups inc. Pin 3*/
#define ADI_RSI_ENABLE_OPENDRAIN      (8u) /*!< MC_CMD Output Control */

#elif defined(__ADSPBF609_FAMILY__)

/*! Maximum number of bytes that the RSI can transfer in a single operation. */
#define ADI_RSI_MAX_TRANSFER_BYTES 65535u

/**
 * Maximum SD/MMC bus width supported by the RSI hardware.
 */
#define ADI_RSI_MAX_BUS_WIDTH 8u

/*
 * Optional flags for adi_rsi_SendCommand().
 */
#define ADI_RSI_CMDFLAG_IEN           (BITM_RSI_CMD_IEN)     /*!< Command Interrupt Enable */
#define ADI_RSI_CMDFLAG_PNDEN         (BITM_RSI_CMD_PNDEN)   /*!< Command Pending Enable */
#define ADI_RSI_CMDFLAG_CRCDIS        (BITM_RSI_CMD_CRCDIS)  /*!< Disable CRC check */
#define ADI_RSI_CMDFLAG_CHKBUSY       (BITM_RSI_CMD_CHKBUSY) /*!< Check Busy Condition */

/*
 * Optional flags for adi_rsi_Enable().
 */
#define ADI_RSI_ENABLE_CLKSEN         (BITM_RSI_CFG_CLKSEN)    /*!< Clocks Enable */
#define ADI_RSI_ENABLE_SD4EN          (BITM_RSI_CFG_SD4EN)     /*!< SDIO 4-Bit Enable */
#define ADI_RSI_ENABLE_MWINEN         (BITM_RSI_CFG_MWINEN)    /*!< Moving Window Enable */
#define ADI_RSI_ENABLE_RST            (BITM_RSI_CFG_RST)       /*!< SDMMC Reset */
#define ADI_RSI_ENABLE_DATPUP         (BITM_RSI_CFG_DATPUP)    /*!< Data Pin Pull-Ups */
#define ADI_RSI_ENABLE_DAT3PUP        (BITM_RSI_CFG_DAT3PUP)   /*!< Data Pin 3 Pull-Up */
#define ADI_RSI_ENABLE_PUPDATALL      (BITM_RSI_CFG_DATPUP | BITM_RSI_CFG_DAT3PUP) /*!< Data Pin Pull-Ups inc. Pin 3*/
#define ADI_RSI_ENABLE_IEBYPDIS       (BITM_RSI_CFG_IEBYPDIS)  /*!< Input Enable Bypass Disable */
#define ADI_RSI_ENABLE_OPENDRAIN      (BITM_RSI_CFG_OPENDRAIN) /*!< MC_CMD Output Control */
#define ADI_RSI_ENABLE_MMCBEN         (BITM_RSI_CFG_MMCBEN)    /*!< MMC Boot Enable */
#define ADI_RSI_ENABLE_MMCBMODE       (BITM_RSI_CFG_MMCBMODE)  /*!< MMC Boot Mode Select */
#define ADI_RSI_ENABLE_BACKEN         (BITM_RSI_CFG_BACKEN)    /*!< Boot ACK Enable */

/*
 * RSI exception bitmasks.
 */
#define ADI_RSI_EXC_SDIOINT     (BITM_RSI_STAT0_SDIOINT)    /*!< SDIO Interrupt */
#define ADI_RSI_EXC_SDCARD      (BITM_RSI_STAT0_SDCARD)     /*!< SD Card Detected */
#define ADI_RSI_EXC_BSETUPEXP   (BITM_RSI_STAT0_BSETUPEXP)  /*!< Boot Setup Expired */
#define ADI_RSI_EXC_BHOLDEXP    (BITM_RSI_STAT0_BHOLDEXP)   /*!< Boot Hold Expired */
#define ADI_RSI_EXC_BDATTO      (BITM_RSI_STAT0_BDATTO)     /*!< Boot data Timeout */
#define ADI_RSI_EXC_BACKTO      (BITM_RSI_STAT0_BACKTO)     /*!< Boot ACK Timeout */
#define ADI_RSI_EXC_BACKBAD     (BITM_RSI_STAT0_BACKBAD)    /*!< Boot ACK Bad */
#define ADI_RSI_EXC_BACKDONE    (BITM_RSI_STAT0_BACKDONE)   /*!< Boot ACK Done */
#define ADI_RSI_EXC_SLPDONE     (BITM_RSI_STAT0_SLPDONE)    /*!< Sleep Done */
#define ADI_RSI_EXC_WKPDONE     (BITM_RSI_STAT0_WKPDONE)    /*!< Wakeup Done */
#define ADI_RSI_EXC_SLPWKPTOUT  (BITM_RSI_STAT0_SLPWKPTOUT) /*!< Sleep Wakeup Timeout */
#define ADI_RSI_EXC_CARDRDY     (BITM_RSI_STAT0_CARDRDY)    /*!< Card Ready */
#define ADI_RSI_EXC_SLPMODE     (BITM_RSI_STAT0_SLPMODE)    /*!< Sleep Mode */
#define ADI_RSI_EXC_BUSYMODE    (BITM_RSI_STAT0_BUSYMODE)   /*!< Busy Mode */

/*
 * RSI interrupt and transfer status bitmasks.
 */
#define ADI_RSI_INT_CMDCRCFAIL  (BITM_RSI_XFR_IMSK1_CMDCRCFAIL) /*!< CMD CRC Fail */
#define ADI_RSI_INT_DATCRCFAIL  (BITM_RSI_XFR_IMSK1_DATCRCFAIL) /*!< Data CRC Fail */
#define ADI_RSI_INT_CMDTO       (BITM_RSI_XFR_IMSK1_CMDTO)      /*!< CMD Timeout */
#define ADI_RSI_INT_DATTO       (BITM_RSI_XFR_IMSK1_DATTO)      /*!< Data Timeout */
#define ADI_RSI_INT_TXUNDR      (BITM_RSI_XFR_IMSK1_TXUNDR)     /*!< Transmit Under Run */
#define ADI_RSI_INT_RXOVER      (BITM_RSI_XFR_IMSK1_RXOVER)     /*!< Receive Over Run */
#define ADI_RSI_INT_RESPEND     (BITM_RSI_XFR_IMSK1_RESPEND)    /*!< Command Response End */
#define ADI_RSI_INT_CMDSENT     (BITM_RSI_XFR_IMSK1_CMDSENT)    /*!< Command Sent */
#define ADI_RSI_INT_DATEND      (BITM_RSI_XFR_IMSK1_DATEND)     /*!< Data End */
#define ADI_RSI_INT_STRTBITERR  (BITM_RSI_XFR_IMSK1_STRTBITERR) /*!< Start Bit Error */
#define ADI_RSI_INT_DATBLKEND   (BITM_RSI_XFR_IMSK1_DATBLKEND)  /*!< Data Block End */

#define ADI_RSI_INT_CMDACT      (BITM_RSI_XFR_IMSK1_CMDACT)     /*!< Command Active */
#define ADI_RSI_INT_TXACT       (BITM_RSI_XFR_IMSK1_TXACT)      /*!< Transmit Active */
#define ADI_RSI_INT_RXACT       (BITM_RSI_XFR_IMSK1_RXACT)      /*!< Receive Active */
#define ADI_RSI_INT_TXFIFOSTAT  (BITM_RSI_XFR_IMSK1_TXFIFOSTAT) /*!< Tx FIFO Status */
#define ADI_RSI_INT_RXFIFOSTA   (BITM_RSI_XFR_IMSK1_RXFIFOSTAT) /*!< Rx FIFO Status */
#define ADI_RSI_INT_TXFIFOFULL  (BITM_RSI_XFR_IMSK1_TXFIFOFULL) /*!< Tx FIFO Full */
#define ADI_RSI_INT_RXFIFOFULL  (BITM_RSI_XFR_IMSK1_RXFIFOFULL) /*!< Rx FIFO Full */
#define ADI_RSI_INT_TXFIFOZERO  (BITM_RSI_XFR_IMSK1_TXFIFOZERO) /*!< Tx FIFO Empty */
#define ADI_RSI_INT_RXFIFOZERO  (BITM_RSI_XFR_IMSK1_RXFIFOZERO) /*!< Rx FIFO Empty */
#define ADI_RSI_INT_TXFIFORDY   (BITM_RSI_XFR_IMSK1_TXFIFORDY)  /*!< Tx FIFO Available */
#define ADI_RSI_INT_RXFIFORDY   (BITM_RSI_XFR_IMSK1_RXFIFORDY)  /*!< Rx FIFO Available */

/**
 * Enumeration of conditions on which an RSI device can issue an outgoing Trigger
 */
typedef enum
{
    ADI_RSI_TRIGGER_OUT_NONE        = ENUM_DMA_CFG_NO_TRIG,    /*!< No trigger (default) */
    ADI_RSI_TRIGGER_OUT_XCNT_EXPIRY = ENUM_DMA_CFG_XCNT_TRIG,  /*!< Generate trigger when X Count reaches 0 */
    ADI_RSI_TRIGGER_OUT_YCNT_EXPIRY = ENUM_DMA_CFG_YCNT_TRIG   /*!< Generate trigger when Y Count reaches 0 */
} ADI_RSI_TRIGGER_OUT;

#elif defined (__ADSPBF518_FAMILY__) || defined(__ADSPBF506F_FAMILY__)

/*! Maximum number of bytes that the RSI can transfer in a single operation. */
#define ADI_RSI_MAX_TRANSFER_BYTES 65535u

/**
 * Maximum SD/MMC bus width supported by the RSI hardware.
 */
#define ADI_RSI_MAX_BUS_WIDTH 4u

/*
 * Optional flags for adi_rsi_SendCommand().
 */
#define ADI_RSI_CMDFLAG_IEN           (CMD_INT_EN)     /*!< Command Interrupt Enable */
#define ADI_RSI_CMDFLAG_PNDEN         (CMD_PEND_EN)    /*!< Command Pending Enable */
#define ADI_RSI_CMDFLAG_CRCDIS        (0u)             /* Unsupported by hardware */
#define ADI_RSI_CMDFLAG_CHKBUSY       (0u)             /* Unsupported by hardware */

/*
 * Optional flags for adi_rsi_Enable().
 */
#define ADI_RSI_ENABLE_CLKSEN         (RSI_CLK_EN)       /*!< Clocks Enable */
#define ADI_RSI_ENABLE_SD4EN          (SDIO4_EN)         /*!< SDIO 4-Bit Enable */
#define ADI_RSI_ENABLE_MWINEN         (MW_EN)            /*!< Moving Window Enable */
#define ADI_RSI_ENABLE_RST            (RSI_RST)          /*!< SDMMC Reset */
#define ADI_RSI_ENABLE_DATPUP         (PU_DAT)           /*!< Data Pin Pull-Ups */
#define ADI_RSI_ENABLE_DAT3PUP        (PU_DAT3)          /*!< Data Pin 3 Pull-Up */
#define ADI_RSI_ENABLE_PUPDATALL      (PU_DAT | PU_DAT3) /*!< Data Pin Pull-Ups inc. Pin 3*/
#define ADI_RSI_ENABLE_OPENDRAIN      (0x00000800u)

/*
 * RSI exception bitmasks.
 */
#define ADI_RSI_EXC_SDIOINT     ((uint16_t)SDIO_INT_DET)    /*!< SDIO Interrupt */
#define ADI_RSI_EXC_SDCARD      ((uint16_t)SD_CARD_DET)     /*!< SD Card Detected */
#define ADI_RSI_EXC_CEATAINT    ((uint16_t)CEATA_INT_DET)   /*!< CE-ATA Command Completion */

/*
 * RSI interrupt and transfer status bitmasks.
 */
#define ADI_RSI_INT_CMDCRCFAIL  ((uint32_t)CMD_CRC_FAIL)    /*!< CMD CRC Fail */
#define ADI_RSI_INT_DATCRCFAIL  ((uint32_t)DAT_CRC_FAIL)    /*!< Data CRC Fail */
#define ADI_RSI_INT_CMDTO       ((uint32_t)CMD_TIMEOUT)     /*!< CMD Timeout */
#define ADI_RSI_INT_DATTO       ((uint32_t)DAT_TIMEOUT)     /*!< Data Timeout */
#define ADI_RSI_INT_TXUNDR      ((uint32_t)TX_UNDERRUN)     /*!< Transmit Under Run */
#define ADI_RSI_INT_RXOVER      ((uint32_t)RX_OVERRUN)      /*!< Receive Over Run */
#define ADI_RSI_INT_RESPEND     ((uint32_t)CMD_RESP_END)    /*!< Command Response End */
#define ADI_RSI_INT_CMDSENT     ((uint32_t)CMD_SENT)        /*!< Command Sent */
#define ADI_RSI_INT_DATEND      ((uint32_t)DAT_END)         /*!< Data End */
#define ADI_RSI_INT_STRTBITERR  ((uint32_t)START_BIT_ERR)   /*!< Start Bit Error */
#define ADI_RSI_INT_DATBLKEND   ((uint32_t)DAT_BLK_END)     /*!< Data Block End */

#define ADI_RSI_INT_CMDACT      ((uint32_t)CMD_ACT)      /*!< Command Active */
#define ADI_RSI_INT_TXACT       ((uint32_t)TX_ACT)       /*!< Transmit Active */
#define ADI_RSI_INT_RXACT       ((uint32_t)RX_ACT)       /*!< Receive Active */
#define ADI_RSI_INT_TXFIFOSTAT  ((uint32_t)TX_FIFO_STAT) /*!< Tx FIFO Status */
#define ADI_RSI_INT_RXFIFOSTA   ((uint32_t)RX_FIFO_STAT) /*!< Rx FIFO Status */
#define ADI_RSI_INT_TXFIFOFULL  ((uint32_t)TX_FIFO_FULL) /*!< Tx FIFO Full */
#define ADI_RSI_INT_RXFIFOFULL  ((uint32_t)RX_FIFO_FULL) /*!< Rx FIFO Full */
#define ADI_RSI_INT_TXFIFOZERO  ((uint32_t)TX_FIFO_ZERO) /*!< Tx FIFO Empty */
#define ADI_RSI_INT_RXFIFOZERO  ((uint32_t)RX_DAT_ZERO)  /*!< Rx FIFO Empty */
#define ADI_RSI_INT_TXFIFORDY   ((uint32_t)TX_DAT_RDY)   /*!< Tx FIFO Available */
#define ADI_RSI_INT_RXFIFORDY   ((uint32_t)RX_FIFO_RDY)  /*!< Rx FIFO Available */

#elif (defined (__ADSPBF548_FAMILY__) || defined(__ADSPBF548M_FAMILY__))

/*! Maximum number of bytes that the RSI can transfer in a single operation. */
#define ADI_RSI_MAX_TRANSFER_BYTES 65535u

/**
 * Maximum SD/MMC bus width supported by the RSI hardware.
 */
#define ADI_RSI_MAX_BUS_WIDTH 4u

/*
 * Optional flags for adi_rsi_SendCommand().
 */
#define ADI_RSI_CMDFLAG_IEN           ((uint32_t)CMD_INT_E)     /*!< Command Interrupt Enable */
#define ADI_RSI_CMDFLAG_PNDEN         ((uint32_t)CMD_PEND_E)    /*!< Command Pending Enable */
#define ADI_RSI_CMDFLAG_CRCDIS        (0u)            /* Unsupported by hardware */
#define ADI_RSI_CMDFLAG_CHKBUSY       (0u)            /* Unsupported by hardware */

/*
 * Optional flags for adi_rsi_Enable().
 */
#define ADI_RSI_ENABLE_CLKSEN         ((uint32_t)CLKS_EN)       /*!< Clocks Enable */
#define ADI_RSI_ENABLE_SD4EN          ((uint32_t)SD4E)          /*!< SDIO 4-Bit Enable */
#define ADI_RSI_ENABLE_MWINEN         ((uint32_t)MWE)           /*!< Moving Window Enable */
#define ADI_RSI_ENABLE_RST            ((uint32_t)SD_RST)        /*!< SDMMC Reset */
#define ADI_RSI_ENABLE_DATPUP         ((uint32_t)PUP_SDDAT)     /*!< Data Pin Pull-Ups */
#define ADI_RSI_ENABLE_DAT3PUP        ((uint32_t)PUP_SDDAT3)    /*!< Data Pin 3 Pull-Up */
#define ADI_RSI_ENABLE_PUPDATALL      ((uint32_t)PUP_SDDAT | PUP_SDDAT3) /*!< Data Pin Pull-Ups inc. Pin 3*/
#define ADI_RSI_ENABLE_OPENDRAIN      (0x00000800u)

/*
 * RSI exception bitmasks.
 */
#define ADI_RSI_EXC_SDIOINT     ((uint16_t)SDIO_INT_DET)    /*!< SDIO Interrupt */
#define ADI_RSI_EXC_SDCARD      ((uint16_t)SD_CARD_DET)     /*!< SD Card Detected */

/*
 * RSI interrupt and transfer status bitmasks.
 */
#define ADI_RSI_INT_CMDCRCFAIL  ((uint32_t)CMD_CRC_FAIL)    /*!< CMD CRC Fail */
#define ADI_RSI_INT_DATCRCFAIL  ((uint32_t)DAT_CRC_FAIL)    /*!< Data CRC Fail */
#define ADI_RSI_INT_CMDTO       ((uint32_t)CMD_TIMEOUT)     /*!< CMD Timeout */
#define ADI_RSI_INT_DATTO       ((uint32_t)DAT_TIMEOUT)     /*!< Data Timeout */
#define ADI_RSI_INT_TXUNDR      ((uint32_t)TX_UNDERRUN)     /*!< Transmit Under Run */
#define ADI_RSI_INT_RXOVER      ((uint32_t)RX_OVERRUN)      /*!< Receive Over Run */
#define ADI_RSI_INT_RESPEND     ((uint32_t)CMD_RESP_END)    /*!< Command Response End */
#define ADI_RSI_INT_CMDSENT     ((uint32_t)CMD_SENT)        /*!< Command Sent */
#define ADI_RSI_INT_DATEND      ((uint32_t)DAT_END)         /*!< Data End */
#define ADI_RSI_INT_STRTBITERR  ((uint32_t)START_BIT_ERR)   /*!< Start Bit Error */
#define ADI_RSI_INT_DATBLKEND   ((uint32_t)DAT_BLK_END)     /*!< Data Block End */

#define ADI_RSI_INT_CMDACT      ((uint32_t)CMD_ACT)      /*!< Command Active */
#define ADI_RSI_INT_TXACT       ((uint32_t)TX_ACT)       /*!< Transmit Active */
#define ADI_RSI_INT_RXACT       ((uint32_t)RX_ACT)       /*!< Receive Active */
#define ADI_RSI_INT_TXFIFOSTAT  ((uint32_t)TX_FIFO_STAT) /*!< Tx FIFO Status */
#define ADI_RSI_INT_RXFIFOSTA   ((uint32_t)RX_FIFO_STAT) /*!< Rx FIFO Status */
#define ADI_RSI_INT_TXFIFOFULL  ((uint32_t)TX_FIFO_FULL) /*!< Tx FIFO Full */
#define ADI_RSI_INT_RXFIFOFULL  ((uint32_t)RX_FIFO_FULL) /*!< Rx FIFO Full */
#define ADI_RSI_INT_TXFIFOZERO  ((uint32_t)TX_FIFO_ZERO) /*!< Tx FIFO Empty */
#define ADI_RSI_INT_RXFIFOZERO  ((uint32_t)RX_DAT_ZERO)  /*!< Rx FIFO Empty */
#define ADI_RSI_INT_TXFIFORDY   ((uint32_t)TX_DAT_RDY)   /*!< Tx FIFO Available */
#define ADI_RSI_INT_RXFIFORDY   ((uint32_t)RX_FIFO_RDY)  /*!< Rx FIFO Available */
#else
#error processor not supported by adi_rsi
#endif

/*
 * Card Event bitmasks
 */
#define ADI_RSI_CARD_INSERTION   0x00000001u  /*!< Card Inserted */
#define ADI_RSI_CARD_REMOVAL     0x00000002u  /*!< Card Removed */

/**
 * Result codes generated by the RSI driver.
 */
typedef enum
{
    ADI_RSI_SUCCESS = 0,           /*!< The API call succeeded. */
    ADI_RSI_FAILURE,               /*!< The API call failed. */
    ADI_RSI_BAD_DEVICE_NUMBER,     /*!< Invalid DeviceNum argument. */
    ADI_RSI_NOT_FINISHED,          /*!< Operation currently incomplete. */
    ADI_RSI_TIMED_OUT,             /*!< An operation did not complete in the expected time. */
    ADI_RSI_INVALID_CLK_DIV,       /*!< Invalid clock divisor */
    ADI_RSI_INVALID_CLK_MODE,      /*!< Invalid clock mode */
    ADI_RSI_INVALID_BOOT_TIME,     /*!< Invalid boot time */
    ADI_RSI_CRC_ERROR,             /*!< CRC (Cyclic Redundancy Check) error */
    ADI_RSI_STARTBIT_ERROR,        /*!< Start bit error */
    ADI_RSI_FIFO_ERROR,            /*!< FIFO overflow/underflow */
    ADI_RSI_DATA_LENGTH,           /*!< Invalid data length */
    ADI_RSI_BUS_WIDTH,             /*!< Invalid bus width */
    ADI_RSI_GPIO_ERR,              /*!< A GPIO service operation failed */
    ADI_RSI_PWR_ERROR,             /*!< A power service operation failed */
    ADI_RSI_NO_CARD,               /*!< No card present */
    ADI_RSI_CARD_PROTECTED,        /*!< Card is write-protected */
    ADI_RSI_INTERRUPT_FAILURE,     /*!< Interrupt operation failed. */
    ADI_RSI_DMA_FAILED,            /*!< DMA operation failed. */
    ADI_RSI_SEMAPHORE_FAILED,      /*!< Semaphore operation failed. */
    ADI_RSI_CMD_RESPONSE_ERR,      /*!< Command response error. */
    ADI_RSI_INVALID_ARGUMENT,      /*!< Invalid API argument. */
    ADI_RSI_INVALID_HANDLE         /*!< Invalid RSI handle. */
} ADI_RSI_RESULT;

/**
 * Specifies the kind of response from the card which is expected for a command.
 */
typedef enum
{
	ADI_RSI_RESPONSE_TYPE_NONE = 0, /*!< No response. */
	ADI_RSI_RESPONSE_TYPE_SHORT,    /*!< 32-bit response. */
	ADI_RSI_RESPONSE_TYPE_LONG      /*!< 128-bit response. */
} ADI_RSI_RESPONSE_TYPE;

/**
 * Specifies the type of data transfer.
 */
typedef enum
{
	ADI_RSI_TRANSFER_NONE = 0,
	ADI_RSI_TRANSFER_PIO_BLCK_READ,
	ADI_RSI_TRANSFER_PIO_BLCK_WRITE,
	ADI_RSI_TRANSFER_DMA_BLCK_READ,
	ADI_RSI_TRANSFER_DMA_BLCK_WRITE,
	ADI_RSI_TRANSFER_PIO_STRM_READ,
	ADI_RSI_TRANSFER_PIO_STRM_WRITE,
	ADI_RSI_TRANSFER_DMA_STRM_READ,
	ADI_RSI_TRANSFER_DMA_STRM_WRITE
} ADI_RSI_TRANSFER;

/**
 * Specifies the mode of operation for CE-ATA.
 */
typedef enum
{
	ADI_RSI_CEATA_MODE_NONE = 0  /*!< CE-ATA mode disabled */
	/* CE-ATA supported only on BF518 and BF506 */
#if defined (__ADSPBF518_FAMILY__) || defined(__ADSPBF506F_FAMILY__)
	, ADI_RSI_CEATA_MODE_EN,        /*!< CE-ATA mode enabled */
	ADI_RSI_CEATA_MODE_EN_INT     /*!< CE-ATA mode enabled with command completion signal. */
#endif
} ADI_RSI_CEATA_MODE;

/**
 * Specifies which timeout is to be set.
 */
typedef enum
{
	ADI_RSI_TIMEOUT_DATA = 0,    /*!< Set the data timeout */
	ADI_RSI_TIMEOUT_BOOT_ACK,    /*!< Set the Boot ACK timeout */
	ADI_RSI_TIMEOUT_SLEEP_WAKEUP,/*!< Set the sleep timeout */
	ADI_RSI_TIMEOUT_RESPONSE     /*!< Set the sleep timeout */
} ADI_RSI_TIMEOUT;

/**
 * Specifies why the user-supplied RSI callback has been invoked
 */
typedef enum
{
	ADI_RSI_EVENT_CARD_CHANGE = 0,  /*!< Card has been inserted or removed. */
	ADI_RSI_EVENT_INTERRUPT,        /*!< An RSI interrupt has occurred. */
	ADI_RSI_EVENT_EXCEPTION         /*!< An RSI exception has occurred. */
} ADI_RSI_EVENT;

/**
 * Specifies the kind of card which is in the RSI slot.
 */
typedef enum
{
	ADI_RSI_CARD_TYPE_SDIO   = 0,   /*!< SD-IO card. */
	ADI_RSI_CARD_TYPE_EMMC   = 1,   /*!< Embedded MMc (eMMC) card. */
	ADI_RSI_CARD_TYPE_SDCARD = 2   /*!< SD card. */
	/* CE-ATA supported only on BF518 and BF506 */
#if defined (__ADSPBF518_FAMILY__) || defined(__ADSPBF506F_FAMILY__)
	, ADI_RSI_CARD_TYPE_CEATA  = 3    /*!< CE-ATA disk drive. */
#endif
} ADI_RSI_CARD_TYPE;

/**
 * Specifies the operating mode of the RSI clock.
 */
typedef enum
{
	ADI_RSI_CLK_MODE_DISABLE = 0,  /*!< Clock is off. */
	ADI_RSI_CLK_MODE_ENABLE,       /*!< Clock is always on. */
	ADI_RSI_CLK_MODE_PWRSAVE       /*!< Clock is on when bus is active, off when idle. */
} ADI_RSI_CLK_MODE;

#if defined(__ADSPBF707_FAMILY__) || defined(__ADSPSC589_FAMILY__) || defined (__ADSPSC573_FAMILY__)
/**
 * Selects which byte counter value to return.
 */
typedef enum
{
	ADI_RSI_BYTE_COUNTER_CIU_CARD = 0,
	ADI_RSI_BYTE_COUNTER_BIU_FIFO
} ADI_RSI_BYTE_COUNTER;
#endif

/**
 * Prepares the RSI device for use.
 */
ADI_RSI_RESULT adi_rsi_Open(
	uint32_t               const  DeviceNum,
	ADI_RSI_HANDLE       * const  phDevice
);

/**
 * Releases the RSI device after use.
 */
ADI_RSI_RESULT adi_rsi_Close(
    ADI_RSI_HANDLE         const hDevice
);

/**
 * Register an application-defined callback function.
 */
ADI_RSI_RESULT adi_rsi_RegisterCallback(
    ADI_RSI_HANDLE         const hDevice,
    ADI_CALLBACK           const pfCallback,
    void                        *pCBParam);

/**
 * Write data to an RSI device (non-blocking).
 */
ADI_RSI_RESULT adi_rsi_SubmitTxBuffer(
    ADI_RSI_HANDLE         const hDevice,
    void                        *pBuffer,
    uint32_t               const nBlkSize,
    uint32_t               const nBlkCnt
);

/**
 * Read data from an RSI device (non-blocking).
 */
ADI_RSI_RESULT adi_rsi_SubmitRxBuffer(
    ADI_RSI_HANDLE         const hDevice,
    void                        *pBuffer,
    uint32_t               const nBlkSize,
    uint32_t               const nBlkCnt
);

/**
 * Checks if the Rx Buffer is available for processing.
 */
ADI_RSI_RESULT adi_rsi_IsRxBufferAvailable(
	    ADI_RSI_HANDLE         const hDevice,
	    bool                   *pbAvailable
);

/**
 * Checks if the Tx Buffer is available for processing.
 */
ADI_RSI_RESULT adi_rsi_IsTxBufferAvailable(
	    ADI_RSI_HANDLE         const hDevice,
	    bool                   *pbAvailable
);

/**
 * This will return the Rx buffer if a filled buffer is available,
 * otherwise waits until a buffer is filled.
 */
ADI_RSI_RESULT adi_rsi_GetRxBuffer (
                    ADI_RSI_HANDLE   const   hDevice,
                    void                   **ppBuffer
);

/**
 * This will return the Tx buffer if a filled buffer is available,
 * otherwise waits until a buffer is filled.
 */
ADI_RSI_RESULT adi_rsi_GetTxBuffer (
                    ADI_RSI_HANDLE   const   hDevice,
                    void                   **ppBuffer
);

/**
 * Write data to an RSI device (blocking).
 */
ADI_RSI_RESULT adi_rsi_Write(
    ADI_RSI_HANDLE         const hDevice,
    void                        *pBuffer,
    uint32_t               const nBlkSize,
    uint32_t               const nBlkCnt
);

/**
 * Read data from an RSI device (blocking).
 */
ADI_RSI_RESULT adi_rsi_Read(
    ADI_RSI_HANDLE         const hDevice,
    void                        *pBuffer,
    uint32_t               const nBlkSize,
    uint32_t               const nBlkCnt
);

/**
 * Turns on the RSI, optionally enabling the internal clocks.
 */
ADI_RSI_RESULT adi_rsi_Enable(
	ADI_RSI_HANDLE const hDevice,
	uint32_t flags
);

/**
 * Turns off the RSI.
 */
ADI_RSI_RESULT adi_rsi_Disable(
	ADI_RSI_HANDLE const hDevice
);

/**
 * Set the block length and number of blocks for upcoming transfer(s).
 */
ADI_RSI_RESULT adi_rsi_SetBlockCntAndLen(
	ADI_RSI_HANDLE const hDevice,
	uint32_t const blkCnt,
	uint32_t const blkSize
);

/**
 * Set an RSI timeout. This is specified in RSI clock periods.
 */
ADI_RSI_RESULT adi_rsi_SetTimeout(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_TIMEOUT const kind,
	uint32_t const timeout
);

/**
 * Set up RSI and DMA for data transfer prior to a command.
 */
ADI_RSI_RESULT adi_rsi_SetDataMode(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_TRANSFER const transfer,
	ADI_RSI_CEATA_MODE const ceata
);

/**
 * Send a command to the card.
 */
ADI_RSI_RESULT adi_rsi_SendCommand(
	ADI_RSI_HANDLE const hDevice,
	uint32_t const cmd,
	uint32_t const arg,
	uint32_t const flags,
	ADI_RSI_RESPONSE_TYPE const expectedResponseType
);

/**
 * Test whether the command send was successful.
 */
ADI_RSI_RESULT adi_rsi_CheckCommand(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_RESPONSE_TYPE const expectedResponseType
);

/**
 * Test whether the returned command send is the one expected.
 */
ADI_RSI_RESULT adi_rsi_CheckResponseCommand(
	ADI_RSI_HANDLE const hDevice,
	uint32_t const cmd
);

/**
 * Read a 32-bit response from the RSI.
 */
ADI_RSI_RESULT adi_rsi_GetShortResponse(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pResp
);

/**
 * Read a 128-bit response from the RSI.
 */
ADI_RSI_RESULT adi_rsi_GetLongResponse(
	ADI_RSI_HANDLE const hDevice,
	uint32_t vResp[4]
);

/**
 * Set the RSI bus width.
 */
ADI_RSI_RESULT adi_rsi_SetBusWidth(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const width
);

/**
 * Get the frequency of the input clock to the RSI.
 */
ADI_RSI_RESULT adi_rsi_GetInputFreq(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pInClk
);

/**
 * Set the clock divisor that derives the RSI (card) clock
 * from the input clock.
 */
ADI_RSI_RESULT adi_rsi_SetClock(
	ADI_RSI_HANDLE   const hDevice,
	uint32_t         const clkDivisor,
	ADI_RSI_CLK_MODE const rsiClkMode
);

/**
 * Set the type of card that is in the RSI slot.
 */
ADI_RSI_RESULT
adi_rsi_SetCardType(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_CARD_TYPE cardType
);

/**
 * Query whether a card is physically present in the RSI slot.
 */
ADI_RSI_RESULT adi_rsi_IsCardPresent(
	ADI_RSI_HANDLE const hDevice
);

#if defined(__ADSPBF609_FAMILY__)
/**
 * Set the boot timing register.
 */
ADI_RSI_RESULT adi_rsi_SetBootTiming(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const setupTime,
	uint32_t       const holdTime
);

/**
 * Configure Trigger Input settings for an RSI device
 */
ADI_RSI_RESULT  adi_rsi_ConfigTriggerIn (
    ADI_RSI_HANDLE const       hDevice,
    bool                        bTriggerWait,
    bool                        bTriggerOvError);

/*
 * Configures Trigger Output settings for an RSI device
 */
ADI_RSI_RESULT  adi_rsi_ConfigTriggerOut (
    ADI_RSI_HANDLE const       hDevice,
    ADI_RSI_TRIGGER_OUT        eRsiTrigger);
#endif

#if defined(__ADSPBF707_FAMILY__) || defined(__ADSPSC589_FAMILY__) || defined (__ADSPSC573_FAMILY__)
/*
 * Returns counts of transferred byted from RSI device
 */
ADI_RSI_RESULT  adi_rsi_GetByteCount(
    ADI_RSI_HANDLE const hDevice,
    ADI_RSI_BYTE_COUNTER eCounter,
    uint32_t            *pCount);
#endif

/**
 * Enable selected RSI interrupts for callback.
 */
ADI_RSI_RESULT adi_rsi_UnmaskInterrupts(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const mask
);

/**
 * Disable selected RSI interrupts for callback.
 */
ADI_RSI_RESULT adi_rsi_MaskInterrupts(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const mask
);

/**
 * Enable selected RSI exceptions for callback.
 */
ADI_RSI_RESULT adi_rsi_UnmaskExceptions(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const mask
);

/**
 * Disable selected RSI interrupts for callback.
 */
ADI_RSI_RESULT adi_rsi_MaskExceptions(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const mask
);

/**
 * Enable selected card events for callback.
 */
ADI_RSI_RESULT adi_rsi_UnmaskCardEvents(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const mask
);

/**
 * Disable selected card events for callback.
 */
ADI_RSI_RESULT adi_rsi_MaskCardEvents(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const mask
);

/**
 * Read the RSI interrupt status.
 */
ADI_RSI_RESULT adi_rsi_GetInterruptStatus(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pIntStatus
);

/**
 * Clear RSI interrupts.
 */
ADI_RSI_RESULT adi_rsi_ClrInterruptStatus(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const clrMask
);

/**
 * Return the RSI's accumulated hardware error(s).
 */
ADI_RSI_RESULT adi_rsi_GetHWErrorStatus (
	ADI_RSI_HANDLE const hDevice,
    uint32_t *pHwError
);

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPBF707_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
/**
 * Read the RSI exception status.
 */
ADI_RSI_RESULT adi_rsi_GetExceptionStatus(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pExcStatus
);

/**
 * Clear RSI exceptions.
 */
ADI_RSI_RESULT adi_rsi_ClrExceptionStatus(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const clrMask
);
#endif



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ADI_RSI_H__ */
/**@}*/
