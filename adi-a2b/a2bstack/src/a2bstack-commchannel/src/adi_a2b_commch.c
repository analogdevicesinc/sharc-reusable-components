/*******************************************************************************
Copyright (c) 2018 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
 * @file:    adi_a2b_commch.c
 * @brief:   This is the implementation Communication Channel on the slave side
 * @version: $Revision: 8359 $
 * @date:    $Date: 2018-10-09 15:38:29 +0530 (Tue, 09 Oct 2018) $
 * Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/*! \addtogroup Communication_Channel Communication Channel
 *  @{
 */

/** @defgroup Communication_Channel_Slave Communication Channel Slave
 *
 * This module handles the Communication Channel
 *
 */

/*! \addtogroup Communication_Channel_Slave
 *  @{
 */

/*============= I N C L U D E S =============*/
#include <string.h>
#include "adi_a2b_commch.h"

/*============= D E F I N E S =============*/


/*============= D A T A =============*/

/*============= L O C A L  P R O T O T Y P E S =============*/
static A2B_COMMCH_RET 	a2b_CommChTxmitNxtFrame(a2b_CommChInfo *pCommChInfo, a2b_CommChMsg *pCommChTxMsg);
static void 			a2b_CommChChkTxmitTimeOut(a2b_CommChInfo *pCommChInfo, a2b_CommChMsg *pCommChTxMsg);
static A2B_COMMCH_RET 	a2b_CommChMboxRead(a2b_CommChInfo *pCommChInfo, a2b_UInt8 nMboxNo, a2b_UInt8 nNoOfBytes, a2b_Byte *pReadBuf, a2b_Int8 nNodeAddr);
static A2B_COMMCH_RET 	a2b_CommChSlvMboxIntrQuery(a2b_CommChInfo *pCommChInfo, a2b_UInt8 nMboxNo );
static A2B_COMMCH_RET 	a2b_CommChSlvProcessMboxIntr(a2b_CommChInfo *pCommChInfo);
static A2B_COMMCH_RET 	a2b_CommChIntrQuery(a2b_Handle *hCommChSlv);
static A2B_COMMCH_RET 	a2b_CommChEngMboxWriteCbk(void* hCommCh, a2b_UInt8 nMboxNo, a2b_UInt16 nNoOfBytes, a2b_UInt8 *pWriteBuf, a2b_Int8 nNodeAddr);

/*============= C O D E =============*/
/*****************************************************************************/
/*!
@brief			This function opens the slave communication channel and does some housekeeping work such as initializing the I2C driver

@param [in]     pCommChPal		Pointer to comm ch PAL functions
@param [in]     pMem			Pointer to a block of memory
@param [in]     nSizeInBytes	Size of memory which is passed to this function in bytes

@return			a2b_CommPalCtx type
                - NULL	: If failed
                - Valid	: Upon success
*/
/*****************************************************************************/
a2b_CommPalCtx*	adi_a2b_CommChPalInit(a2b_CommChPal *pCommChPal, a2b_UInt8 *pMem, a2b_UInt8 nSizeInBytes)
{
	a2b_CommPalCtx	*pCommPalCtx = A2B_NULL;

	if((pCommChPal != A2B_NULL) && (pMem != A2B_NULL))
	{
		if(nSizeInBytes >= sizeof(a2b_CommPalCtx))
		{
			pCommPalCtx 			= (a2b_CommPalCtx*)pMem;
			pCommPalCtx->pCommChPal = pCommChPal;
			pCommPalCtx->hI2c 		= pCommPalCtx->pCommChPal->i2cOpen();

			if((pCommPalCtx->hI2c == A2B_NULL) || (pCommPalCtx->pCommChPal->timerInit() != 0u))
			{
				pCommPalCtx = A2B_NULL;
			}
		}
	}

	return (pCommPalCtx);
}

/*****************************************************************************/
/*!
@brief			This function creates and initializes an instance of slave communication channel

@param [in]     pCommChConfig		Comm ch slave configuration pointer

@return			a2b_Handle type
                - NULL	: If failed
                - Valid	: Upon success
*/
/*****************************************************************************/
a2b_Handle adi_a2b_CommChCreate(a2b_CommChConfig *pCommChConfig)
{
	a2b_CommChInfo	*pCommChInfo = A2B_INVALID_HANDLE;
	a2b_UInt8			nIdx;

	if(pCommChConfig != A2B_NULL)
	{
		pCommChInfo 		 									= (a2b_CommChInfo*)pCommChConfig->pMem;
		pCommChInfo->eCommChMsgTxState						= A2B_COMMCH_TX_IDLE;
		for(nIdx=0; nIdx<ADI_A2B_COMMCH_ENG_MAX_NO_OF_MAILBOX; nIdx++)
		{
			pCommChInfo->abReadComplete[nIdx]				= 0u;
		}
		pCommChInfo->nTxTimeout								= 0u;
		pCommChInfo->nCurrTimeInMsec	 						= 0u;
		pCommChInfo->nFrameStartTime 						= 0u;
		pCommChInfo->nIntrPollStartTime						= 0u;
		pCommChInfo->nIntrPollPeriod							= 0u;
		pCommChInfo->pCommPalCtx 							= pCommChConfig->pCommPalCtx;
		pCommChInfo->nI2cAddr								= pCommChConfig->nI2cAddr;

		(void)adi_a2b_CommChEngRstRxInfo(&pCommChInfo->oCommChEngInfo);
		(void)adi_a2b_CommChEngRstTxInfo(&pCommChInfo->oCommChEngInfo);

		pCommChInfo->oCommChEngInfo.nRxMbox					= pCommChConfig->nRxMbox;
		pCommChInfo->oCommChEngInfo.nTxMbox					= pCommChConfig->nTxMbox;
		pCommChInfo->oCommChEngInfo.pfStatCb			  	= pCommChConfig->pfStatCb;
		pCommChInfo->oCommChEngInfo.pCbParam			  	= pCommChConfig->pCbParam;
		pCommChInfo->oCommChEngInfo.pfMboxWriteCb			= &a2b_CommChEngMboxWriteCbk;
		pCommChInfo->oCommChEngInfo.hCommCh				  	= pCommChInfo;
	}

	return ((a2b_Handle)(pCommChInfo));
}

/*****************************************************************************/
/*!
@brief			This API is used to transmit a message using communication channel

@param [in]     hCommChSlv	Comm ch slave instance pointer
@param [in]     pMsg		Pointer to comm ch msg
@param [in]     nNodeAddr	Node address for which the msg is being transmitted

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
A2B_COMMCH_RET adi_a2b_CommChTxMsg(a2b_Handle hCommChSlv, a2b_CommChMsg *pMsg , a2b_Int8 nNodeAddr)
{
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;
	a2b_CommChInfo	*pCommChInfo = (a2b_CommChInfo*)hCommChSlv;
	a2b_CommChMsg		*pCommChTxMsg = &pCommChInfo->oCommChEngInfo.oA2bTxMsg;
	a2b_UInt8			nIdx;

	if (pCommChInfo->eCommChMsgTxState == A2B_COMMCH_TX_IDLE)
	{
		if((pMsg->pMsgPayload != A2B_NULL) && (pMsg->nMsgLenInBytes != 0u))
		{
			(void)memcpy(pCommChTxMsg->pMsgPayload, pMsg->pMsgPayload, pMsg->nMsgLenInBytes);
		}

		pCommChTxMsg->nMsgLenInBytes				= pMsg->nMsgLenInBytes;
		pCommChTxMsg->nMsgId						= pMsg->nMsgId;
		pCommChInfo->oCommChEngInfo.nTxNodeAddr  = nNodeAddr;
		pCommChInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_BUSY;

		eRet = adi_a2b_CommChEngTxProcess(&pCommChInfo->oCommChEngInfo);
		if(eRet == A2B_COMMCH_FAILED)
		{
			/* Reset Tx in progress flag , state and timeout count */
			pCommChInfo->eCommChMsgTxState 			= A2B_COMMCH_TX_IDLE;
			for(nIdx=0; nIdx<ADI_A2B_COMMCH_ENG_MAX_NO_OF_MAILBOX; nIdx++)
			{
				pCommChInfo->abReadComplete[nIdx]	= 0u;
			}
			pCommChInfo->nTxTimeout					= 0u;
			pCommChInfo->nFrameStartTime 			= 0u;
			(void)adi_a2b_CommChEngRstTxInfo(&pCommChInfo->oCommChEngInfo);
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
@brief			This function should be called periodically and checks for Tx done or a timeout occurred.

@param [in]     hCommChSlv	Comm ch slave instance pointer

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
A2B_COMMCH_RET adi_a2b_CommChTick(a2b_Handle *hCommChSlv)
{
	a2b_CommChInfo	*pCommChInfo = (a2b_CommChInfo*)hCommChSlv;
	a2b_CommChMsg		*pCommChTxMsg = &pCommChInfo->oCommChEngInfo.oA2bTxMsg;
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;
	a2b_UInt8			nIdx;

	/* Get current time tick */
	pCommChInfo->nCurrTimeInMsec = pCommChInfo->pCommPalCtx->pCommChPal->timerGetSysTime();

	/* If Slave node then query for mailbox interrupts once every polling period */
	pCommChInfo->nIntrPollPeriod =  pCommChInfo->nCurrTimeInMsec -  pCommChInfo->nIntrPollStartTime ;
	if( pCommChInfo->nIntrPollPeriod >= ADI_A2B_COMMCH_INTR_POLLING_PERIOD)
	{
		pCommChInfo->nIntrPollStartTime =  pCommChInfo->nCurrTimeInMsec;
		/* Check interrupt status */
		eRet = a2b_CommChIntrQuery(hCommChSlv);
	}

	/* If transmission ongoing */
	if(pCommChInfo->eCommChMsgTxState == A2B_COMMCH_TX_BUSY)
	{
		/* If Tx response received for current frame */
		if (pCommChInfo->abReadComplete[pCommChInfo->oCommChEngInfo.nTxMbox] == A2B_TRUE)
		{
			eRet = a2b_CommChTxmitNxtFrame(pCommChInfo, pCommChTxMsg);
		}
		/* If no response yet for current frame check for timeout */
		else
		{
			a2b_CommChChkTxmitTimeOut(pCommChInfo, pCommChTxMsg);
		}
	}

	return (eRet);
}

/*****************************************************************************/
/*!
@brief			This function checks if any bytes to be transmitted. If all the payload bytes
				are transmitted then it triggers a transmission finished callback.

@param [in]     pCommChInfo	Comm Channel slave instance pointer
@param [in]		pCommChTxMsg	Pointer to Buffer which holds the message being transmitted

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChTxmitNxtFrame(a2b_CommChInfo *pCommChInfo, a2b_CommChMsg *pCommChTxMsg)
{
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;

	/* Reset confirmation */
	pCommChInfo->abReadComplete[pCommChInfo->oCommChEngInfo.nTxMbox] = A2B_FALSE;

	/* If all payload bytes transmitted and acknowledged */
	if((pCommChInfo->oCommChEngInfo.nWriteIdx == pCommChTxMsg->nMsgLenInBytes)  || (pCommChTxMsg->nMsgLenInBytes == 0u))
	{
		/* Trigger an transmission finished callback */
		pCommChInfo->oCommChEngInfo.pfStatCb(pCommChInfo->oCommChEngInfo.pCbParam, pCommChTxMsg, A2B_COMMCH_EVENT_TX_DONE, pCommChInfo->oCommChEngInfo.nTxNodeAddr);

		/* Reset Tx in progress flag , state and timeout count */
		pCommChInfo->eCommChMsgTxState 											= A2B_COMMCH_TX_IDLE;
		pCommChInfo->abReadComplete[pCommChInfo->oCommChEngInfo.nTxMbox]		= 0u;
		pCommChInfo->nTxTimeout													= 0u;
		pCommChInfo->nFrameStartTime 											= 0u;
		(void)adi_a2b_CommChEngRstTxInfo(&pCommChInfo->oCommChEngInfo);
	}
	else	/* if bytes pending */
	{
		/* reset timeout for next frame */
		pCommChInfo->nTxTimeout		= 0u;
		/* Transmit next frame */
		eRet = adi_a2b_CommChEngTxProcess(&pCommChInfo->oCommChEngInfo);
		if(eRet == A2B_COMMCH_FAILED)
		{
			/* Reset Tx in progress flag , state and timeout count */
			pCommChInfo->eCommChMsgTxState 										= A2B_COMMCH_TX_IDLE;
			pCommChInfo->abReadComplete[pCommChInfo->oCommChEngInfo.nTxMbox]	= 0u;
			pCommChInfo->nTxTimeout												= 0u;
			pCommChInfo->nFrameStartTime 										= 0u;
			(void)adi_a2b_CommChEngRstTxInfo(&pCommChInfo->oCommChEngInfo);
		}
		else
		{
			/* Get the start tick for next frame */
			pCommChInfo->nFrameStartTime = pCommChInfo->nCurrTimeInMsec;
		}
	}

	return (eRet);
}

/*****************************************************************************/
/*!
@brief			This function checks for transmission timeout id there is no response

@param [in]     pCommChInfo	Comm Channel slave instance pointer
@param [in]		pCommChTxMsg	Pointer to Buffer which holds the message being transmitted

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static void a2b_CommChChkTxmitTimeOut(a2b_CommChInfo *pCommChInfo, a2b_CommChMsg *pCommChTxMsg)
{
	pCommChInfo->nTxTimeout = (a2b_UInt32)(pCommChInfo->nCurrTimeInMsec - pCommChInfo->nFrameStartTime);

	/* If timeout reached */
	if(pCommChInfo->nTxTimeout == ADI_A2B_COMMCH_TXMIT_MBOX_TIMEOUT_IN_TICKS )
	{
		/* Trigger a timeout callback */
		pCommChInfo->oCommChEngInfo.pfStatCb(pCommChInfo->oCommChEngInfo.pCbParam, pCommChTxMsg, A2B_COMMCH_EVENT_TX_TIMEOUT, pCommChInfo->oCommChEngInfo.nTxNodeAddr);

		/* Reset the Tx in progress flag, timeout and state */
		pCommChInfo->eCommChMsgTxState 										= A2B_COMMCH_TX_IDLE;
		pCommChInfo->abReadComplete[pCommChInfo->oCommChEngInfo.nTxMbox]	= 0u;
		pCommChInfo->nTxTimeout												= 0u;
		pCommChInfo->nFrameStartTime 										= 0u;
		(void)adi_a2b_CommChEngRstTxInfo(&pCommChInfo->oCommChEngInfo);
	}
}

/*****************************************************************************/
/*!
@brief			This function writes the message to a specific mailbox

@param [in]     hCommCh		Comm ch slave instance pointer
@param [in]     nMboxNo		Mailbox number
@param [in]     nNoOfBytes	Number of bytes to be transmitted
@param [in]     pWriteBuf	Write buffer pointer
@param [in]     nNodeAddr	Node address for which the msg is being transmitted

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChEngMboxWriteCbk(void* hCommCh, a2b_UInt8 nMboxNo, a2b_UInt16 nNoOfBytes, a2b_UInt8 *pWriteBuf, a2b_Int8 nNodeAddr)
{
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;
	a2b_HResult 		eI2CRet;
	a2b_UInt8       	regOffset,awBuf[5];
	a2b_UInt8       	awTmpBuf[2];
	a2b_CommChInfo	*pCommChInfo = (a2b_CommChInfo*)hCommCh;

	switch(nMboxNo)
	{
		case 0u:
				/* Mailbox 0 */
				awBuf[0] = A2B_REG_MBOX0B0;
				break;

		case 1u:
				/* Mailbox 1 */
				awBuf[0] = A2B_REG_MBOX1B0;
				break;

		default:
				break;
	}
	(void)memcpy(&awBuf[1],pWriteBuf,nNoOfBytes);

	/* Transmit only to  Master Node */
	if(nNodeAddr == -1)
	{
		/* Write data to mailbox register */
		eI2CRet = pCommChInfo->pCommPalCtx->pCommChPal->i2cWrite(pCommChInfo->pCommPalCtx->hI2c, pCommChInfo->nI2cAddr, nNoOfBytes+1u, &awBuf[0]);
		if(eI2CRet != 0u)
		{
			eRet = A2B_COMMCH_RET_MBOXWRITE_FAILED;
		}
	}
	else
	{
		eRet = A2B_COMMCH_RET_MBOXWRITE_FAILED;
	}

	return (eRet);
}

/*****************************************************************************/
/*!
@brief			This function process the mailbox interrupts

@param [in]     pCommChInfo	Comm ch slave instance pointer

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChSlvProcessMboxIntr(a2b_CommChInfo *pCommChInfo)
{
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;
	a2b_Byte			anMboxData[ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE];
	a2b_UInt8           regOffset,intrType;
	a2b_HResult 		retTemp;

	/* Read interrupt type */
	regOffset = A2B_REG_LINTTYPE;
	retTemp = pCommChInfo->pCommPalCtx->pCommChPal->i2cWriteRead(pCommChInfo->pCommPalCtx->hI2c,pCommChInfo->nI2cAddr, 1u,&regOffset, 1u, &intrType);
	if ( retTemp != 0u )
	{
		eRet = A2B_COMMCH_RET_MBOXREAD_FAILED;
	}

	if(eRet == A2B_COMMCH_SUCCESS)
	{
		switch(intrType)
		{
			case A2B_ENUM_LINTTYPE_MBOX0_FULL:
				eRet = a2b_CommChMboxRead(pCommChInfo, pCommChInfo->oCommChEngInfo.nRxMbox, ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE, &anMboxData[0], -1);
				if(eRet == A2B_COMMCH_SUCCESS)
				{
					/* Call process function */
					adi_a2b_CommChEngRxProcess(&pCommChInfo->oCommChEngInfo, &anMboxData[0]);
				}

				break;

			case A2B_ENUM_LINTTYPE_MBOX0_EMPTY:
				pCommChInfo->abReadComplete[0] = A2B_TRUE;
				break;

			case A2B_ENUM_LINTTYPE_MBOX1_FULL:
				eRet = a2b_CommChMboxRead(pCommChInfo, pCommChInfo->oCommChEngInfo.nTxMbox, ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE, &anMboxData[0], -1);
				if(eRet == A2B_COMMCH_SUCCESS)
				{
					/* Call process function */
					(void)adi_a2b_CommChEngRxProcess(&pCommChInfo->oCommChEngInfo, &anMboxData[0]);
				}
				break;

			case A2B_ENUM_LINTTYPE_MBOX1_EMPTY:
				pCommChInfo->abReadComplete[1] = A2B_TRUE;
				break;

			default:
				break;
		}
	}

	return eRet;
}

/*****************************************************************************/
/*!
@brief			This function queries for the mailbox interrupts

@param [in]     hCommChSlv	Comm ch slave instance pointer

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChIntrQuery(a2b_Handle *hCommChSlv)
{
	a2b_UInt16          idx;
	A2B_COMMCH_RET 		ret = A2B_COMMCH_SUCCESS;
	a2b_CommChInfo	*pCommChInfo = (a2b_CommChInfo*)hCommChSlv;


	/* Check for interrupts on receive mailbox (0) */
	ret = a2b_CommChSlvMboxIntrQuery(pCommChInfo, pCommChInfo->oCommChEngInfo.nRxMbox);

	/* Check for interrupts on transmit mailbox (1) */
	ret = a2b_CommChSlvMboxIntrQuery(pCommChInfo, pCommChInfo->oCommChEngInfo.nTxMbox);

	return (ret);
}

/*****************************************************************************/
/*!
@brief			This function queries for the interrupts from a specific mailbox

@param [in]     pCommChInfo	Comm ch slave instance pointer
@param [in]		nMboxNo			Mailbox number

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChSlvMboxIntrQuery(a2b_CommChInfo *pCommChInfo, a2b_UInt8 nMboxNo )
{

	a2b_UInt8           regOffset;
	A2B_COMMCH_RET 		ret = A2B_COMMCH_SUCCESS;
	a2b_HResult 		retTemp;
	a2b_UInt8           intrStatMbox;
	a2b_UInt32			nIntrEmptyMask, nIntrFullMask;
	a2b_Bool			bIntrPending = A2B_FALSE;

	/* Select mailbox */
	switch(nMboxNo)
	{
		case 0u:
			/* Mailbox 0 */
			regOffset	  	= A2B_REG_MBOX0STAT;
			nIntrEmptyMask	= A2B_ENUM_MBOX0STAT_MB0EIRQ_ACT;
			nIntrFullMask	= A2B_ENUM_MBOX0STAT_MB0FIRQ_ACT;
			break;

		case 1u:
			/* Mailbox 1 */
			regOffset 	  	= A2B_REG_MBOX1STAT;
			nIntrEmptyMask	= A2B_ENUM_MBOX1STAT_MB1EIRQ_ACT;
			nIntrFullMask	= A2B_ENUM_MBOX1STAT_MB1FIRQ_ACT;
			break;

		default:
			break;
	}

	/* Read mailbox status */
	retTemp = pCommChInfo->pCommPalCtx->pCommChPal->i2cWriteRead( pCommChInfo->pCommPalCtx->hI2c,pCommChInfo->nI2cAddr, 1u,&regOffset, 1u, &intrStatMbox);
	if ( retTemp != 0u )
	{
		ret = A2B_COMMCH_RET_MBOXREAD_FAILED;
	}
	/* Check empty interrupt active */
	if(intrStatMbox & (a2b_UInt8)nIntrEmptyMask)
	{
		bIntrPending = A2B_TRUE;
	}
	/* Check full interrupt active  */
	else if(intrStatMbox & (a2b_UInt8)nIntrFullMask)
	{
		bIntrPending = A2B_TRUE;
	}
	else
	{
		/* Do nothing as Mailbox interrupts not pending */
	}

	/* If interrupt is active */
	if((ret == A2B_COMMCH_SUCCESS) && bIntrPending)
	{
		/* Process the interrupt */
		ret = a2b_CommChSlvProcessMboxIntr(pCommChInfo);

	}

	return (ret);
}

/*****************************************************************************/
/*!
@brief			This function reads the message from the specified mailbox

@param [in]     pCommChInfo	Comm ch slave instance pointer
@param [in]     nMboxNo			Mailbox number
@param [in]     nNoOfBytes		Number of bytes to be read
@param [in]     pReadBuf		Read buffer pointer
@param [in]     nNodeAddr		Node address

@return			A2B_COMMCH_RET type
                - 0: A2B_COMMCH_SUCCESS
                - 1: A2B_COMMCH_FAILED
*/
/*****************************************************************************/
static A2B_COMMCH_RET a2b_CommChMboxRead(a2b_CommChInfo *pCommChInfo, a2b_UInt8 nMboxNo, a2b_UInt8 nNoOfBytes, a2b_Byte *pReadBuf, a2b_Int8 nNodeAddr)
{
	A2B_COMMCH_RET		eRet = A2B_COMMCH_SUCCESS;
	a2b_HResult 		eI2CRet;
	a2b_UInt8       	regOffset;
	a2b_UInt8       	awTmpBuf[2];

	switch(nMboxNo)
	{
		case 0u:
				/* Mailbox 0 */
				regOffset = A2B_REG_MBOX0B0;
				break;

		case 1u:
				/* Mailbox 1 */
				regOffset = A2B_REG_MBOX1B0;
				break;

		default:
				break;
	}

	/* Read data from mailbox register */
	eI2CRet= pCommChInfo->pCommPalCtx->pCommChPal->i2cWriteRead(pCommChInfo->pCommPalCtx->hI2c, pCommChInfo->nI2cAddr, 1u, &regOffset, 4u, pReadBuf);
	if(eI2CRet != 0u)
	{
		eRet=A2B_COMMCH_RET_MBOXREAD_FAILED;
	}

	return (eRet);
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
