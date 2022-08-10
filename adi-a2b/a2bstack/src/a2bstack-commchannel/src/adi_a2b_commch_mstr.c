/*******************************************************************************
Copyright (c) 2018 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
 * @file:    adi_a2b_commch_mstr.c
 * @brief:   This is the implementation Communication Channel on the master side
 * @version: $Revision: 7970 $
 * @date:    $Date: 2018-04-27 09:52:07 +0530 (Fri, 27 Apr 2018) $
 * Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/*! \addtogroup Communication_Channel Communication Channel
 *  @{
 */

/** @defgroup Communication_Channel_Master    Communication Channel Master
 *
 * This module handles the Communication Channel on the master side which runs the ADI A2B stack
 *
 */

/*! \addtogroup Communication_Channel_Master
 *  @{
 */

/*============= I N C L U D E S =============*/
#include "adi_a2b_commch_mstr.h"
#include "a2b/msg.h"
#include "a2b/msgrtr.h"
#include <string.h>
#ifdef A2B_FEATURE_COMM_CH
/*============= D E F I N E S =============*/

/*============= D A T A =============*/
/*!< Message notify interrupt */
static struct a2b_MsgNotifier *gpNotifyMboxIntr;

/*============= L O C A L  P R O T O T Y P E S =============*/
static void a2b_CommChMstrOnMboxEvent(struct a2b_Msg* msg, a2b_Handle userData);
static A2B_COMMCH_RET a2b_CommChEngMboxWriteCbk(void* commChHnd, a2b_UInt8 nMboxNo, a2b_UInt16 nNoOfBytes, a2b_UInt8 *pWriteBuf, a2b_Int8 nNodeAddr);
/*============= C O D E =============*/
/*****************************************************************************/
/*!
@brief			This function opens the master communication channel by registering for the mailbox event

@param [in]     ctx		Stack context pointer

@return			None
*/
/*****************************************************************************/
void adi_a2b_CommChMstrOpen(struct a2b_StackContext* ctx)
{
	/* Register for notifications on mailbox interrupts for every frame transfer */
	gpNotifyMboxIntr = a2b_msgRtrRegisterNotify(ctx, A2B_MSGNOTIFY_MAILBOX_EVENT, &a2b_CommChMstrOnMboxEvent, A2B_NULL, A2B_NULL);
}


/*****************************************************************************/
/*!
@brief			This function closes the master communication channel by
				unregistering the mailbox notification event

@param [in]     ctx		Stack context pointer

@return			None
*/
/*****************************************************************************/
void adi_a2b_CommChMstrClose(struct a2b_StackContext* ctx)
{
	/* UnRegister notifications on mailbox interrupts */
	a2b_msgRtrUnregisterNotify(gpNotifyMboxIntr);
}

/*****************************************************************************/
/*!
@brief			This function creates and initializes an instance of master communication channel

@param [in]     pCommChMstrConfig		Comm ch master configuration pointer

@return			a2b_Handle type
                - NULL	: If failed
                - Valid	: Upon success
*/
/*****************************************************************************/
a2b_Handle adi_a2b_CommChMstrCreate(a2b_CommChMstrConfig *pCommChMstrConfig)
{
	a2b_CommChMstrInfo	*pCommChMstrInfo = A2B_INVALID_HANDLE;
	a2b_UInt8			nIdx;

	if(pCommChMstrConfig != A2B_NULL)
	{
		pCommChMstrInfo 		 								= (a2b_CommChMstrInfo*)pCommChMstrConfig->pMem;
		pCommChMstrInfo->eCommChMsgTxState						= A2B_COMMCH_TX_IDLE;
		for(nIdx=0; nIdx<ADI_A2B_COMMCH_ENG_MAX_NO_OF_MAILBOX; nIdx++)
		{
			pCommChMstrInfo->abReadComplete[nIdx]				= 0u;
		}
		pCommChMstrInfo->bTimeout								= 0u;
		pCommChMstrInfo->ctx	 								= pCommChMstrConfig->ctx;
		pCommChMstrInfo->mboxHnd 								= pCommChMstrConfig->mboxHnd;

		adi_a2b_CommChEngRstRxInfo(&pCommChMstrInfo->oCommChEngInfo);
		adi_a2b_CommChEngRstTxInfo(&pCommChMstrInfo->oCommChEngInfo);

		pCommChMstrInfo->oCommChEngInfo.nRxMbox					= A2B_COMMCH_MSTR_RX_MAILBOX_NO;
		pCommChMstrInfo->oCommChEngInfo.nTxMbox					= A2B_COMMCH_MSTR_TX_MAILBOX_NO;
		pCommChMstrInfo->oCommChEngInfo.pfStatCb			  	= pCommChMstrConfig->pfStatCb;
		pCommChMstrInfo->oCommChEngInfo.pCbParam			  	= pCommChMstrConfig->pCbParam;
		pCommChMstrInfo->oCommChEngInfo.pfMboxWriteCb			= &a2b_CommChEngMboxWriteCbk;
		pCommChMstrInfo->oCommChEngInfo.hCommCh				  	= pCommChMstrInfo;
	}

	return ((a2b_Handle)(pCommChMstrInfo));
}

/*****************************************************************************/
/*!
@brief			This API is used to transmit a message using communication channel

@param [in]     hCommChMstr	Comm ch Master instance pointer
@param [in]     pMsg		Pointer to comm ch msg
@param [in]     nNodeAddr	Node address for which the msg is being transmitted

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
A2B_COMMCH_RET adi_a2b_CommChMstrTxMsg(a2b_Handle hCommChMstr, a2b_CommChMsg *pMsg , a2b_Int8 nNodeAddr)
{
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;
	a2b_CommChMstrInfo	*pCommChMstrInfo = (a2b_CommChMstrInfo*)hCommChMstr;
	a2b_CommChMsg		*pCommChTxMsg = &pCommChMstrInfo->oCommChEngInfo.oA2bTxMsg;

	if (pCommChMstrInfo->eCommChMsgTxState == A2B_COMMCH_TX_IDLE)
	{
		if((pMsg->pMsgPayload != A2B_NULL) && (pMsg->nMsgLenInBytes != 0u))
		{
			(void)memcpy(pCommChTxMsg->pMsgPayload, pMsg->pMsgPayload, pMsg->nMsgLenInBytes);
		}

		pCommChTxMsg->nMsgLenInBytes				= pMsg->nMsgLenInBytes;
		pCommChTxMsg->nMsgId						= pMsg->nMsgId;
		pCommChMstrInfo->oCommChEngInfo.nTxNodeAddr = nNodeAddr;
		pCommChMstrInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_BUSY;

		eRet = adi_a2b_CommChEngTxProcess(&pCommChMstrInfo->oCommChEngInfo);
		if(eRet == A2B_COMMCH_FAILED)
		{
			/* Reset Tx in progress flag and state parameters  */
			pCommChMstrInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_IDLE;
			adi_a2b_CommChEngRstTxInfo(&pCommChMstrInfo->oCommChEngInfo);
		}
	}
	else
	{
		/* return busy */
		eRet = A2B_COMMCH_FAILED;
	}

	return(eRet);
}

/*****************************************************************************/
/*!
@brief			This function should be called periodically and checks for Tx done or a timeout occurred

@param [in]     hCommChMstr	Comm ch Master instance pointer

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
A2B_COMMCH_RET 	   adi_a2b_CommChMstrTick(a2b_Handle *hCommChMstr)
{
	a2b_CommChMstrInfo	*pCommChMstrInfo = (a2b_CommChMstrInfo*)hCommChMstr;
	a2b_CommChMsg		*pCommChTxMsg = &pCommChMstrInfo->oCommChEngInfo.oA2bTxMsg;
	A2B_COMMCH_RET		 eRet = A2B_COMMCH_SUCCESS;

	/* If transmission ongoing */
	if(pCommChMstrInfo->eCommChMsgTxState == A2B_COMMCH_TX_BUSY)
	{
		/* If Tx response received for current frame */
		if (pCommChMstrInfo->abReadComplete[pCommChMstrInfo->oCommChEngInfo.nTxMbox] == A2B_TRUE)
		{
			/* Reset confirmation */
			pCommChMstrInfo->abReadComplete[pCommChMstrInfo->oCommChEngInfo.nTxMbox] = A2B_FALSE;

			/* If all payload bytes transmitted and acknowledged */
			if(pCommChMstrInfo->oCommChEngInfo.nWriteIdx >= pCommChTxMsg->nMsgLenInBytes)
			{
				/* Trigger an transmission finished callback */
				pCommChMstrInfo->oCommChEngInfo.pfStatCb(pCommChMstrInfo->oCommChEngInfo.pCbParam, pCommChTxMsg, A2B_COMMCH_EVENT_TX_DONE, pCommChMstrInfo->oCommChEngInfo.nTxNodeAddr);

				/* Reset Tx in progress flag and state parameters  */
				pCommChMstrInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_IDLE;
				adi_a2b_CommChEngRstTxInfo(&pCommChMstrInfo->oCommChEngInfo);
			}
			else	/* if bytes pending */
			{
				/* Transmit next frame */
				eRet = adi_a2b_CommChEngTxProcess(&pCommChMstrInfo->oCommChEngInfo);
				if(eRet == A2B_COMMCH_FAILED)
				{
					/* Reset Tx in progress flag and state parameters  */
					pCommChMstrInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_IDLE;
					adi_a2b_CommChEngRstTxInfo(&pCommChMstrInfo->oCommChEngInfo);
				}
			}
		}
		/* If no response yet for current frame check for timeout */
		else if(pCommChMstrInfo->bTimeout == A2B_TRUE)
		{
			/* Clear the timeout flag */
			pCommChMstrInfo->bTimeout = A2B_FALSE;

			/* Trigger a timeout callback */
			pCommChMstrInfo->oCommChEngInfo.pfStatCb(pCommChMstrInfo->oCommChEngInfo.pCbParam, pCommChTxMsg, A2B_COMMCH_EVENT_TX_TIMEOUT, pCommChMstrInfo->oCommChEngInfo.nTxNodeAddr);

			/* Clear the Tx state m/c */
			pCommChMstrInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_IDLE;
			adi_a2b_CommChEngRstTxInfo(&pCommChMstrInfo->oCommChEngInfo);
		}
		else
		{
			/* Do nothing wait for response or timeout */
		}

	}

	return (eRet);
}

/*****************************************************************************/
/*!
@brief			This is a callback function called on mailbox event, This function handles mailbox
				events such as Tx done, Tx time out, Rx data, etc.

@param [in]     msg			message pointer
@param [in]     userData	User specified data

@return			None
*/
/*****************************************************************************/
static void a2b_CommChMstrOnMboxEvent(struct a2b_Msg* msg, a2b_Handle userData)
{
	a2b_MailboxEventInfo *pMboxEventInfo;
	a2b_CommChMstrInfo	 *pCommChMstrInfo;
	A2B_UNUSED(userData);

	if (msg)
	{
		pMboxEventInfo = a2b_msgGetPayload(msg);

		if (pMboxEventInfo)
		{
			pCommChMstrInfo = pMboxEventInfo->commChHnd;

			switch(pMboxEventInfo->eEvent)
			{
			case A2B_MBOX_TX_DONE:
				/* Set the read complete flag */
				pCommChMstrInfo->abReadComplete[pCommChMstrInfo->oCommChEngInfo.nTxMbox] = A2B_TRUE;
				break;

			case A2B_MBOX_TX_TIMEOUT:
				/* Set the timeout flag */
				pCommChMstrInfo->bTimeout = A2B_TRUE;
				break;

			case A2B_MBOX_TX_IO_ERROR:
				pCommChMstrInfo->oCommChEngInfo.pfStatCb(pCommChMstrInfo->oCommChEngInfo.pCbParam, &pCommChMstrInfo->oCommChEngInfo.oA2bTxMsg, A2B_COMMCH_EVENT_FAILURE, pCommChMstrInfo->oCommChEngInfo.nTxNodeAddr);

				/* Clear the Tx state m/c and give a Tx failure cb */
				pCommChMstrInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_IDLE;
				adi_a2b_CommChEngRstTxInfo(&pCommChMstrInfo->oCommChEngInfo);
				break;

			case A2B_MBOX_RX_DATA:
				/* Call the Rx process for the next 4 bytes */
				pCommChMstrInfo->oCommChEngInfo.nRxNodeAddr = (a2b_Int8) pMboxEventInfo->nNodeId;
				adi_a2b_CommChEngRxProcess(&pCommChMstrInfo->oCommChEngInfo, pMboxEventInfo->prBuf);
				break;

			case A2B_MBOX_RX_IO_ERROR:
				/*pCommChMstrInfo->oCommChEngInfo.pfStatCb(pCommChMstrInfo->oCommChEngInfo.pCbParam, &pCommChMstrInfo->oCommChEngInfo.oA2bTxMsg, A2B_COMMCH_EVENT_FAILURE, pCommChMstrInfo->oCommChEngInfo.nTxNodeAddr);*/

				/* Clear the Rx state m/c and give a Rx failure cb */
				adi_a2b_CommChEngRstRxInfo(&pCommChMstrInfo->oCommChEngInfo);
				break;

			default:
				break;
			}
		}
	}
}

/*****************************************************************************/
/*!
@brief			This function allocates a mailbox message and sends a request to
				a specific mailbox

@param [in]     commChHnd		Comm ch Master instance pointer
@param [in]     nMboxNo		Mailbox number
@param [in]     nNoOfBytes	Number of bytes to be transmitted
@param [in]     pWriteBuf	Write buffer pointer
@param [in]     nNodeAddr	Node address for which the msg is being transmitted

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChEngMboxWriteCbk(void* commChHnd, a2b_UInt8 nMboxNo, a2b_UInt16 nNoOfBytes, a2b_UInt8 *pWriteBuf, a2b_Int8 nNodeAddr)
{
	A2B_COMMCH_RET			eRet = A2B_COMMCH_SUCCESS;
	a2b_CommChMstrInfo	 	*pCommChMstrInfo = (a2b_CommChMstrInfo*)commChHnd;
	struct a2b_Msg* 		pCommChMboxWriteMsg;
	a2b_MailboxTxInfo		*pMailboxTxInfo;
	a2b_HResult				nStatus;

	/* Allocate the mailbox send data request */
	pCommChMboxWriteMsg =  a2b_msgAlloc(pCommChMstrInfo->ctx, A2B_MSG_REQUEST, A2B_MSGREQ_SEND_MBOX_DATA);

	pMailboxTxInfo = (a2b_MailboxTxInfo*)a2b_msgGetPayload(pCommChMboxWriteMsg);

	pMailboxTxInfo->req.nDataSz 		= (a2b_UInt8)nNoOfBytes;
	pMailboxTxInfo->req.nSlvNodeAddr	= nNodeAddr;
	pMailboxTxInfo->req.pwBuf			= pWriteBuf;
	pMailboxTxInfo->req.nMbox			= pCommChMstrInfo->oCommChEngInfo.nTxMbox;

	/* Send the request to the specific mailbox */
	nStatus = a2b_msgRtrSendRequestToMailbox(pCommChMboxWriteMsg, pCommChMstrInfo->mboxHnd, A2B_NULL);
	if(nStatus != 0u)
	{
		eRet = A2B_COMMCH_FAILED;
	}
	(void)a2b_msgUnref(pCommChMboxWriteMsg);

	return (eRet);
}
#endif /* A2B_FEATURE_COMM_CH */

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
