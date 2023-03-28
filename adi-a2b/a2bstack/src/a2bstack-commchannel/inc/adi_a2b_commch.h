/*******************************************************************************
Copyright (c) 2018 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
 * @file:    adi_a2b_commch.h
 * @brief:   This  header file contains structure definitions for Communication channel on the slave side
 * @version: $Revision: 7705 $
 * @date:    $Date: 2018-02-23 16:25:59 +0530 (Fri, 23 Feb 2018) $
 * Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/** \addtogroup Communication_Channel_Slave
 *  @{
 */

#ifndef ADI_A2B_COMMCH_H_
#define	ADI_A2B_COMMCH_H_

#include "adi_a2b_commch_engine.h"
#include "a2b/regdefs.h"

#define ADI_A2B_COMMCH_TXMIT_MBOX_TIMEOUT_IN_TICKS		(1000u)	/*!< Timeout for mailbox read response for a transmission is specified in ticks at which the time base input to comm channel */
#define ADI_A2B_COMMCH_INTR_POLLING_PERIOD				(1u)	/*!< Polling period for interrupt query. Only applicable for slave nodes is specified in ticks at which the time base runs for comm channel */

#define ADI_A2B_COMMCH_SLVNODE_I2C_ADDR    		(0x6A)	/*!< A2B Slave node I2C address */

#define ADI_A2B_COMMCH_RX_MAILBOX_NO			(0u) 	/*!< Default Mailbox No on which Master transmits and slave receives */
#define ADI_A2B_COMMCH_TX_MAILBOX_NO			(1u) 	/*!< Default Mailbox No on which Slave transmits and master receives */

typedef a2b_Handle  (* pfCommChI2cOpenFunc)();
typedef a2b_HResult (* pfCommChI2cCloseFunc)(a2b_Handle hnd);
typedef a2b_HResult (* pfCommChI2cReadFunc)(a2b_Handle hnd, a2b_UInt16 addr, a2b_UInt16 nRead, a2b_UInt8* rBuf);
typedef a2b_HResult (* pfCommChI2cWriteFunc)(a2b_Handle hnd, a2b_UInt16 addr, a2b_UInt16 nWrite, const a2b_UInt8* wBuf);
typedef a2b_HResult (* pfCommChI2cWriteReadFunc)(a2b_Handle hnd, a2b_UInt16 addr, a2b_UInt16 nWrite, const a2b_UInt8* wBuf, a2b_UInt16 nRead, a2b_UInt8* rBuf);

typedef a2b_HResult (* pfCommChTimerInit)(void);
/* Must return time in milliseconds */
typedef a2b_UInt32  (* pfCommChTimerGetSysTimeFunc)(void);

/*! \struct a2b_CommChPal
    Communication Channel Platform Abstraction Layer (PAL) function pointers
*/
typedef struct a2b_CommChPal
{
    /** \name I2C Implementation
     *  I2C prototypes requiring platform implementation.
     *
     * \{ */
    pfCommChI2cOpenFunc         i2cOpen;		/*!< Function pointer for: i2cOpen */
    pfCommChI2cCloseFunc        i2cClose;		/*!< Function pointer for: i2cClose */
    pfCommChI2cReadFunc         i2cRead;		/*!< Function pointer for: i2cRead */
    pfCommChI2cWriteFunc        i2cWrite;		/*!< Function pointer for: i2cWrite */
    pfCommChI2cWriteReadFunc    i2cWriteRead;	/*!< Function pointer for: i2cWriteRead */
    /** \} */

    /** \name Timer Implementation
     *  Timer prototypes requiring platform implementation.
     *
     * \{ */
    pfCommChTimerInit			timerInit;		/*!< Function pointer for: timerInit */
    pfCommChTimerGetSysTimeFunc timerGetSysTime;/*!< Function pointer for: timerGetSysTime */
    /** \} */
}a2b_CommChPal;

/*! \struct a2b_CommPalCtx
    Communication Channel slave PAL context
*/
typedef struct a2b_CommPalCtx
{
    /*!< Pointer to comm channel Pal functions */
	a2b_CommChPal	*pCommChPal;

    /*!< I2C handle */
	void*			hI2c;
}a2b_CommPalCtx;

/*! \struct a2b_CommChSlvConfig
    Strcuture holding the various configurable parameters of a Communication channel instance
*/
typedef struct a2b_CommChConfig
{
	/*!< Pointer to status call back function used for indicating events to application*/
	pfStatusCb		pfStatCb;

	/*!< Pointer to callback parameter passed during the callback */
	void*			pCbParam;

	/*!< Pointer to  block of memory used for holding channel instance state information */
	void*			pMem;

	/*!< Comm channel Pal context pointer */
	a2b_CommPalCtx	*pCommPalCtx;

	/*!< Local I2C address which is used during I2C read/write  */
	a2b_UInt16		nI2cAddr;

	/*!< Mailbox number for this node configured as a receive mailbox */
	a2b_UInt8		nRxMbox;

	/*!< Mailbox number for this node configured as a transmit mailbox */
	a2b_UInt8		nTxMbox;


}a2b_CommChConfig;

/*! \struct a2b_CommChSlvInfo
    Structure holding the Communication Channel instance state information
*/
typedef struct a2b_CommChInfo
{
	/*!<  Indicates the current Message transmission state */
	A2B_COMMCH_TX_MSG_STATE	eCommChMsgTxState;

	/*!< Flag to indicate whether data transmitted via mailbox is read by receiver */
	volatile a2b_Bool 		abReadComplete[ADI_A2B_COMMCH_ENG_MAX_NO_OF_MAILBOX];

	/*!< Field to calculate the timeout on mailbox empty interrupt for data transmitted to remote node */
	volatile a2b_UInt32		nTxTimeout;

	/*!< Current time in msec */
	a2b_UInt64 				nCurrTimeInMsec;

	/*!< Start Time when a frame is transmitted over mailbox */
	a2b_UInt64 				nFrameStartTime;

	/*!< Interrupt polling start time */
	a2b_UInt64 				nIntrPollStartTime;

	/*!< Interrupt polling period */
	a2b_UInt64 				nIntrPollPeriod;

	/*!< Communication Channel Pal context pointer */
	a2b_CommPalCtx			*pCommPalCtx;

	/*!< Local A2B transceiver I2C address which is used during I2C read/write  */
	a2b_UInt16				nI2cAddr;

	/*!< Instance of Communication channel engine */
	a2b_CommChEngInfo		oCommChEngInfo;

}a2b_CommChInfo;

a2b_CommPalCtx*	adi_a2b_CommChPalInit(a2b_CommChPal *pCommChPal, a2b_UInt8 *pMem, a2b_UInt8 nSizeInBytes);
a2b_Handle 		adi_a2b_CommChCreate(a2b_CommChConfig *pCommChSlvConfig);
A2B_COMMCH_RET 	adi_a2b_CommChTxMsg(a2b_Handle hCommChSlv, a2b_CommChMsg *pMsg , a2b_Int8 nNodeAddr);
A2B_COMMCH_RET 	adi_a2b_CommChTick(a2b_Handle *hCommChSlv);


#endif /* ADI_A2B_COMMCH_H_ */

/**@}*/
