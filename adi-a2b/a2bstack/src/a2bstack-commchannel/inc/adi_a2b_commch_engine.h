/*******************************************************************************
Copyright (c) 2018 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
 * @file:    adi_a2b_commch_engine.h
 * @brief:   This  header file contains structure definitions for Tx and Rx state machines
 * @version: $Revision: 8410 $
 * @date:    $Date: 2018-10-15 16:22:08 +0530 (Mon, 15 Oct 2018) $
 * Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/** \addtogroup Communication_Channel_Engine
 *  @{
 */

#ifndef ADI_A2B_COMMCH_ENGINE_H_
#define	ADI_A2B_COMMCH_ENGINE_H_

#include "a2b/ctypes.h"

/*============= D E F I N E S =============*/
#define ADI_A2B_COMMCH_ENG_MSG_ID_BITMASK							(0x3Fu)	/*!< Bit Mask for Message Id */
#define ADI_A2B_COMMCH_ENG_FF_ID									(0u)	/*!< Id for First Frame */
#define ADI_A2B_COMMCH_ENG_CF_ID									(1u)	/*!< Id for Consecutive Frame */
#define ADI_A2B_COMMCH_ENG_SF_ID									(2u)	/*!< Id for Single Frame */
#define ADI_A2B_COMMCH_ENG_FRAME_ID_BIT_SHIFT						(6u)	/*!< Frame ID bit shift */
#define ADI_A2B_COMMCH_ENG_MAX_SN									(0x3Fu)	/*!< Maximum sequence number which can be represented by 6-bit */

/* A2B maximum mailbox message size */
#define ADI_A2B_COMMCH_ENG_MAX_NO_OF_MAILBOX	(2u)	/*!< Maximum number of mailboxes */
#define ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE	(4u)    /*!< Maximum no of bytes that can be transmitted/received over Mailbox at a time */
#define ADI_A2B_COMMCH_ENG_MAX_RX_MSG_SIZE		(1500u)	/*!< Maximum message reception capacity at once in bytes */
#define ADI_A2B_COMMCH_ENG_MAX_TX_MSG_SIZE		(1500u)	/*!< Maximum message transmission capacity at once in bytes */
#define ADI_A2B_COMMCH_ENG_NODEADDR_MASTER    	(-1)	/*!< The node address for the A2B master node */


/*! \enum A2B_COMMCH_RET
Enumeration for A2B Comm Channel API return values
*/
typedef enum A2B_COMMCH_RET
{
	A2B_COMMCH_SUCCESS,      			/*!< API execution success	*/
	A2B_COMMCH_FAILED, 					/*!< General failure		*/
	A2B_COMMCH_RET_MBOXWRITE_FAILED,	/*!< Error writing to Mailbox */
	A2B_COMMCH_RET_MBOXREAD_FAILED,		/*!< Error reading from Mailbox */
	A2B_COMMCH_RET_EXCEDDED_TXCAPACITY  /*!< Transmission size greater than maximum permissible */
} A2B_COMMCH_RET;

/*! \enum A2B_COMMCH_TX_MSG_STATE
Enumeration for A2B communication channel transmit state.
*/
typedef enum A2B_COMMCH_TX_MSG_STATE
{
	A2B_COMMCH_TX_IDLE,         /*!<  Tx state machine idle. Tx message can be accepted by the state machine	*/
	A2B_COMMCH_TX_BUSY          /*!<  Tx state machine busy. No more tx messages can be accepted in this state	*/
}A2B_COMMCH_TX_MSG_STATE;

/*! \enum A2B_COMMCH_RX_MSG_STATE
Enumeration for A2B communication channel engine receive msg state.
*/
typedef enum A2B_COMMCH_RX_MSG_STATE
{
	A2B_COMMCH_RX_IDLE,         /*!< Rx state machine idle. Rx message can be accepted by the state machine	*/
	A2B_COMMCH_RX_BUSY          /*!< Rx state machine busy. No more Rx messages can be accepted in this state	*/
}A2B_COMMCH_RX_MSG_STATE;

/*! \enum A2B_COMMCH_ENG_STATE
Enumeration for A2B communication channel engine state.
*/
typedef enum A2B_COMMCH_ENG_STATE
{
	A2B_COMMCH_MSG_INIT,        /*!< Engine state machine Init. Ready to receive/transmit a First Frame or Single Frame	*/
	A2B_COMMCH_MSG_CTS          /*!< Engine state machine continue to send (CTS). Consecutive frame is received or transmitted in this state */
}A2B_COMMCH_ENG_STATE;

/*! \enum A2B_COMMCH_EVENT
communication channel event type
*/
typedef enum A2B_COMMCH_EVENT
{
	A2B_COMMCH_EVENT_RX_MSG,        		/*!< Message received event indication */
	A2B_COMMCH_EVENT_RX_SN_ERROR,			/*!< Reception Sequence Count error event indication	*/
	A2B_COMMCH_EVENT_RX_EXCEDDED_CAPACITY,  /*!< Received message length greater than permissible event indication	*/
	A2B_COMMCH_EVENT_RX_INVALID_FRAME,      /*!< Invalid frame received event indication	*/
	A2B_COMMCH_EVENT_TX_DONE,       		/*!< Transmission finished event indication	*/
	A2B_COMMCH_EVENT_TX_TIMEOUT,    		/*!< Transmission timeout event indication	*/
	A2B_COMMCH_EVENT_FAILURE        		/*!< Generic failure indication	*/
}A2B_COMMCH_EVENT;

/*! \struct a2b_CommChMsg
   Structure holding the elements of a Communication Channel message
*/
typedef struct a2b_CommChMsg
{
    /** Message id */
    a2b_UInt8 			nMsgId;

    /** Message data size: Should be in this format: Total Length = (LEN_MSB_BYTE << 8) | LEN_LSB_BYTE */
    a2b_UInt16 			nMsgLenInBytes;

    /** Pointer to the Message payload */
    a2b_UInt8* 			pMsgPayload;
}a2b_CommChMsg;

/* Status call back  to indicate events to higher layers */
typedef void (*pfStatusCb)(void* pCbParam, a2b_CommChMsg *pMsg, A2B_COMMCH_EVENT eEventType, a2b_Int8 nNodeAddr);

/* Callback to request a write over mailbox  */
typedef A2B_COMMCH_RET (*pfMailboxboxWriteCb)(void* hCommCh, a2b_UInt8 nMboxNo, a2b_UInt16 nNoOfBytes, a2b_UInt8 *pWriteBuf, a2b_Int8 nNodeAddr);

/*! \struct a2b_CommChEngInfo
    Communication Channel Engine State Information
*/
typedef struct a2b_CommChEngInfo
{
	/*!< State of the Reception State Machine */
	volatile A2B_COMMCH_ENG_STATE	eRxState;

	/*!< Number of bytes received in the current message */
	volatile a2b_UInt16		nReadIdx;

	/*!< Buffer to hold  message received */
	struct a2b_CommChMsg	oA2bRxMsg;

	/*!< Node from which the message is received */
	a2b_Int8 				nRxNodeAddr;

	/*!< Comm channel Rx message state. It indicates that whether Rx state machine is busy processing a message or idle */
	volatile A2B_COMMCH_RX_MSG_STATE eCommChRxMsgState;

	/*!< Buffer to hold receive message payload */
	a2b_UInt8				oA2bRxBuf[ADI_A2B_COMMCH_ENG_MAX_RX_MSG_SIZE];

	/*!< Receive mailbox number */
	a2b_UInt8				nRxMbox;

	/*!< Received sequence count number */
	a2b_UInt8				nRxSeqCnt;

	/*!< Status call back function pointer to indicate asynchonrous events to the higher layer */
	pfStatusCb				pfStatCb;

	/*!< Call back function pointer to request a mailbox write from the communication channel */
	pfMailboxboxWriteCb		pfMboxWriteCb;

	/*!< Pointer to callback parameter */
	void*					pCbParam;

	/*!< State of the Transmit State Machine */
	volatile A2B_COMMCH_ENG_STATE	eTxState;

	/*!< Number of bytes transmitted during a message transmission */
	volatile a2b_UInt16		nWriteIdx;

	/*!< Buffer to hold  message being transmitted */
	struct a2b_CommChMsg	oA2bTxMsg;

	/*!< Node to which the message will be transmitted */
	a2b_Int8 				nTxNodeAddr;

	/*!< Buffer to hold payload of message being transmitted */
	a2b_UInt8				oA2bTxBuf[ADI_A2B_COMMCH_ENG_MAX_TX_MSG_SIZE];

	/*!< Transmit mailbox number */
	a2b_UInt8				nTxMbox;

	/*!< Transmit sequence count number */
	a2b_UInt8				nTxSeqCnt;

	/*!< Communication Channel instance handle */
	void*					hCommCh;

	/*!< Buffer which is used to store the frame to be transmitted */
	a2b_UInt8 				aMboxWbuf[ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE];
}a2b_CommChEngInfo;

/*============= G L O B A L  P R O T O T Y P E S =============*/

A2B_COMMCH_RET 	adi_a2b_CommChEngTxProcess(a2b_CommChEngInfo *pCommChEngInfo);
void 			adi_a2b_CommChEngRxProcess(a2b_CommChEngInfo *pCommChEngInfo, a2b_UInt8 *pMsg);
void 			adi_a2b_CommChEngRstRxInfo(a2b_CommChEngInfo *pCommChEngInfo);
void 			adi_a2b_CommChEngRstTxInfo(a2b_CommChEngInfo *pCommChEngInfo);
#endif /* ADI_A2B_COMMCH_ENGINE_H_ */

/**@}*/
