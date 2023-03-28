/*******************************************************************************
Copyright (c) 2017 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
*******************************************************************************

   Name       : adi_a2b_bert.c

   Description: This file contains functions that process messages from SigmaStudio GUI

    Functions  :
                adi_a2b_BertIntiation()
                adi_a2b_BertUpdate()
                adi_a2b_BertStop()



   Prepared &
   Reviewed by: Automotive Software and Systems team,
                IPDC, Analog Devices,  Bangalore, India

   $Date: 2015-10-14 15:17:23 +0530 (Wed, 14 Oct 2015) $

******************************************************************************/
/*! \addtogroup BERT BERT
 *  @{
 */

/** @defgroup BERT
 *
 * This module initiates BERT, calculates BERT at regular intervals and terminates BERT on request.
 *
 */

/*============= I N C L U D E S =============*/
/*
*/
#include "string.h"
#include "a2b/defs.h"
#include "a2b/regdefs.h"
#include "a2b/error.h"
#include "a2b/i2c.h"
#include "a2b/timer.h"
#include "bdd_pb2.pb.h"
#include "a2b/pluginapi.h"
#include "plugin_priv.h"
/*============= D E F I N E S =============*/

/*============= D A T A =============*/


/*============= C O D E =============*/
/*
** Function Prototype section
*/

#ifdef A2B_RUN_BIT_ERROR_TEST

void adi_a2b_BertUpdate(a2b_Plugin*  pPlugin);

/****************************************************************************/
/*!
    @brief          This function performs initializations in order to
                    start BERT calculations.


    @param [in]     aBertConfigBuffer      BERT configuration buffer

                    aBertConfigBuffer[0] Master node ID
                    aBertConfigBuffer[1] HDCNTERR (0-Off, 1-On) //PRBS Enable(0-Off, 1-On) in case of PRBS mode
                    aBertConfigBuffer[2] DDERR (0-Off, 1-On) //PRBS mode(Node to Node)  in case of PRBS mode
                    aBertConfigBuffer[3] BECOVF (0-Off, 1-On)
                    aBertConfigBuffer[4] DPERR (0-Off, 1-On)
                    aBertConfigBuffer[5] DCRCERR (0-Off, 1-On)
                    aBertConfigBuffer[6] ICRCERR (0-Off, 1-On)


    @param [in]     pPlugin       Pointer to Plugin Structure

    @return         none

    Note: This function updates BERT count if the initilization is already done
*/
/********************************************************************************/
void adi_a2b_BertIntiation(a2b_UInt8 *pBertConfigBuffer, a2b_Plugin*  pPlugin)
{
	a2b_UInt8 nIndex = 0u;
	a2b_UInt32 nVal;
	a2b_UInt32 nResult = 0u;
    a2b_UInt8 nBECTLReg = 0,nDATCTLValue, nTESTMODEReg = 0;
    const bdd_Network* pBdd = pPlugin->bdd;
    ADI_A2B_BERT_HANDLER *pBert = pPlugin->pBertHandler;
    a2b_UInt8 wBuf[4];
    a2b_UInt8 rBuf[4];
    a2b_HResult status = A2B_RESULT_SUCCESS;

	/* Updating fields  */
	if (pBert->nBERTMode == AUDIO_TEST)
	{
		nBECTLReg = (a2b_UInt8)( *(pBertConfigBuffer + 1) | (*(pBertConfigBuffer + 2) << 1) | (*(pBertConfigBuffer + 3) << 2) |(*(pBertConfigBuffer + 4) << 3) |(*(pBertConfigBuffer + 5) << 4)| (7u << 5)) ;
	}
	else
	{
		nTESTMODEReg = (a2b_UInt8)( (*(pBertConfigBuffer + 2) << 2) | (*(pBertConfigBuffer + 1) << 1)|(*(pBertConfigBuffer + 1)) );
	}

	/* Ensure NODEADR 0 */
	wBuf[0u] = A2B_REG_NODEADR;
	wBuf[1u] = 0x00u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	/* Read the DATCTL value */
	wBuf[0u] = A2B_REG_DATCTL;
	status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, (a2b_UInt8*)&nDATCTLValue);

	/* Disabling DATCTL */
	wBuf[0u] = A2B_REG_DATCTL;
	wBuf[1u] = 0x00u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	wBuf[0u] = A2B_REG_CONTROL;
	wBuf[1u] = 0x01u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	/* Enable broadcast write */
	wBuf[1u] = 0x80u;
	wBuf[0u] = A2B_REG_NODEADR;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    /* Resetting test mode and BECTL register to 0 */
	wBuf[0u] = A2B_REG_TESTMODE;
    wBuf[1u] = 0x0u;
    status = a2b_i2cSlaveBroadcastWrite( pPlugin->ctx, 2, wBuf );

    wBuf[0u] = A2B_REG_BECCTL;
    wBuf[1u] = 0x0u;
    status = a2b_i2cSlaveBroadcastWrite( pPlugin->ctx, 2, wBuf );

	if (pBert->nBERTMode == AUDIO_TEST)
	{
		/* Resetting test mode and BECTL register to 0 */

		/* Configuring bit error register */
		wBuf[1u] = nBECTLReg;
		wBuf[0u] = A2B_REG_BECCTL;
		status = a2b_i2cSlaveBroadcastWrite(pPlugin->ctx, 2, wBuf);
	}
	else
	{

		/* Disable I2S Pin  - To avoid noise in the speaker  */
		wBuf[1u] = 0x0u;
		wBuf[0u] = A2B_REG_I2SCFG;
		status = a2b_i2cSlaveBroadcastWrite(pPlugin->ctx, 2, wBuf);

		/* Writing test mode register in Slave(s) */
		wBuf[1u] = nTESTMODEReg;
		wBuf[0u] = A2B_REG_TESTMODE;
		status = a2b_i2cSlaveBroadcastWrite(pPlugin->ctx, 2, wBuf);
	}

	/* Broadcast disable */
	wBuf[1u] = 0x0u;
	wBuf[0u] = A2B_REG_NODEADR;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	/* Re-enable DATCTL */
	/* Enable only downstream */
	wBuf[0u] = A2B_REG_DATCTL;
	wBuf[1u] = 0x01;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	wBuf[0u] = A2B_REG_CONTROL;
	wBuf[1u] = 0x00u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );
	/* Delay for new structure to take effect */
	a2b_ActiveDelay(pPlugin->ctx, 5u);

	/* Re-enable Both up and down-stream */
	wBuf[0u] = A2B_REG_DATCTL;
	wBuf[1u] = nDATCTLValue;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	wBuf[0u] = A2B_REG_CONTROL;
	wBuf[1u] = 0x01u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	/* Initialization done.. now read the values */
    adi_a2b_BertUpdate(pPlugin);

}



/****************************************************************************/
/*!
    @brief          This function reads bit error counts(PRBS & BECNT) from all the configured nodes

    @param [in]     pPlugin    Pointer to Plugin

    @return         none
*/
/********************************************************************************/
void adi_a2b_BertUpdate(a2b_Plugin*  pPlugin)
{
	a2b_UInt8 nIndex = 0u, nSlaveID =0u;
    const bdd_Network* pBdd = pPlugin->bdd;
    ADI_A2B_BERT_HANDLER *pBert = pPlugin->pBertHandler;
    a2b_UInt8 wBuf[4];
    a2b_UInt8 rBuf[4];
    a2b_HResult status = A2B_RESULT_SUCCESS;


    if (pBert->nBERTMode == AUDIO_TEST)
    {
    	pBert->nErrorCount[0u] = 0u;

        /* Read Error Count */
        wBuf[0u] = A2B_REG_BECNT;
        rBuf[0u] = 0u;
        status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, rBuf);
        pBert->nErrorCount[0u] = rBuf[0u];
    }
    else
    {
    	pBert->nPRBSCount[0u] = 0u;

		/* Get PRBS count for master node */
		wBuf[0u] = A2B_REG_ERRCNT0;
		rBuf[0u] = 0u;
		status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, rBuf);
		pBert->nPRBSCount[0u] =  rBuf[0u];

		wBuf[0u] = A2B_REG_ERRCNT1;
		rBuf[0u] = 0u;
		status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, rBuf);
		pBert->nPRBSCount[0u] += ((a2b_UInt32)rBuf[0u]<<8);

		wBuf[0u] = A2B_REG_ERRCNT2;
		rBuf[0u] = 0u;
		status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, rBuf);
		pBert->nPRBSCount[0u] += ((a2b_UInt32)rBuf[0u]<<16);

		wBuf[0u] = A2B_REG_ERRCNT3;
		rBuf[0u] = 0u;
		status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, rBuf);
		pBert->nPRBSCount[0u] += ((a2b_UInt32)rBuf[0u]<<24);
    }


    /* Get PRBS as well as bit error count for Slave nodes */
    for(nIndex  = 0u ; nIndex < (a2b_UInt8)(pPlugin->discovery.dscNumNodes) ;nIndex++)
    {
    	nSlaveID = nIndex+1u;
		wBuf[0u] = A2B_REG_NODEADR;
		wBuf[1u] = nIndex;
		status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	    if (pBert->nBERTMode == AUDIO_TEST)
	    {
	    	pBert->nErrorCount[nSlaveID] = 0u;

	    	wBuf[0u] = A2B_REG_BECNT;
	    	rBuf[0u] = 0u;
	    	status = a2b_i2cSlaveWriteRead( pPlugin->ctx, (a2b_Int16)nIndex, 1, wBuf, 1, rBuf);
	    	pBert->nErrorCount[nSlaveID] = rBuf[0u];
	    }
	    else
	    {
			/* Slave/Bus preset */
			pBert->nPRBSCount[nSlaveID] = 0u;

	    	wBuf[0u] = A2B_REG_ERRCNT0;
		    rBuf[0u] = 0u;
			status = a2b_i2cSlaveWriteRead( pPlugin->ctx, (a2b_Int16)nIndex, 1, wBuf, 1, rBuf);

			pBert->nPRBSCount[nSlaveID] += rBuf[0];

			wBuf[0u] = A2B_REG_ERRCNT1;
		    rBuf[0u] = 0u;
		    status = a2b_i2cSlaveWriteRead( pPlugin->ctx, (a2b_Int16)nIndex, 1, wBuf, 1, rBuf);
			pBert->nPRBSCount[nSlaveID] += ((a2b_UInt32)rBuf[0u]<<8);

			wBuf[0u] = A2B_REG_ERRCNT2;
		    rBuf[0u] = 0u;
		    status = a2b_i2cSlaveWriteRead( pPlugin->ctx, (a2b_Int16)nIndex, 1, wBuf, 1, rBuf);
			pBert->nPRBSCount[nSlaveID] += ((a2b_UInt32)rBuf[0u]<<16);

			wBuf[0u] = A2B_REG_ERRCNT3;
			rBuf[0u] = 0u;
			status = a2b_i2cSlaveWriteRead( pPlugin->ctx, (a2b_Int16)nIndex, 1, wBuf, 1, rBuf);
			pBert->nPRBSCount[nSlaveID] += ((a2b_UInt32)rBuf[0u]<<24);
	    }

    }

    /* Reset  node address */
	wBuf[0u] = A2B_REG_NODEADR;
	wBuf[1u] = 0u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	/* just indicate that read is complete now */
    pBert->bReadFlag = 1u;

}

/****************************************************************************/
/*!
    @brief          This function terminates BERT calculation session

    @param [in]     pPlugin    Pointer to Plugin

    @return         none
*/
/********************************************************************************/
void adi_a2b_BertStop(a2b_Plugin*  pPlugin)
{
	a2b_UInt8 nIndex = 0u, nSlaveID =0u, nI2SCFG=0u;
    const bdd_Network* pBdd = pPlugin->bdd;
    ADI_A2B_BERT_HANDLER *pBert = pPlugin->pBertHandler;
    a2b_UInt8 wBuf[4];
    a2b_UInt8 rBuf[4];
    a2b_HResult status = A2B_RESULT_SUCCESS;
    const bdd_Node *pMasterNode ,*pSlaveNode;
    a2b_UInt8 nBECTLReg,nDATCTLValue, nTESTMODEReg;

    pMasterNode = (const bdd_Node *)&pBdd->nodes[(a2b_UInt32)A2B_NODEADDR_MASTER+1u];

    /* Master Pre-set */

    /* Broad cast write enable */
	wBuf[0u] = A2B_REG_NODEADR;
	wBuf[1u] = 0x80u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    /* Read the DATCTL value */
    wBuf[0u] = A2B_REG_DATCTL;
    status = a2b_i2cMasterWriteRead( pPlugin->ctx, 1, wBuf, 1, (a2b_UInt8*)&nDATCTLValue);

    /* Disabling DATCTL */
	wBuf[0u] = A2B_REG_DATCTL;
	wBuf[1u] = 0x0u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	wBuf[0u] = A2B_REG_CONTROL;
	wBuf[1u] = 0x1u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    /* Clear PRBS and BECNT registers- Master  */
	wBuf[0u] = A2B_REG_TESTMODE;
	wBuf[1u] = 0x0u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	if(pMasterNode->intRegs.has_becctl)
	{
		wBuf[0u] = A2B_REG_BECCTL;
		wBuf[1u] = (a2b_UInt8)pMasterNode->intRegs.becctl;
		status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );
	}

    /* Write 1 to clear */
	wBuf[0u] = A2B_REG_BECNT;
	wBuf[1u] = 0xFFu;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    /* Clear PRBS and BECNT registers - Slave */
    /* Slave/Bus preset */
    wBuf[0u] = A2B_REG_TESTMODE;
    wBuf[1u] = 0x0u;
    status = a2b_i2cSlaveBroadcastWrite(pPlugin->ctx, 2, wBuf);

    /* Write 1 to clear */
    wBuf[0u] = A2B_REG_BECNT;
    wBuf[1u] = 0xFFu;
    status = a2b_i2cSlaveBroadcastWrite(pPlugin->ctx, 2, wBuf);

    /* Master Pre-set */
    /* Broad cast disable */
	wBuf[0u] = A2B_REG_NODEADR;
	wBuf[1u] = 0x0u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    for (nIndex = 0u; nIndex < (a2b_UInt8)(a2b_UInt8)(pPlugin->discovery.dscNumNodes); nIndex++)
    {
        nSlaveID = nIndex+1u ;

        pSlaveNode = (const bdd_Node *)&pBdd->nodes[nSlaveID];

	   /* Get I2S configuration */
	   nI2SCFG = (a2b_UInt8)(pSlaveNode->i2cI2sRegs.i2scfg);

	   /* Restore the configuration */
		if(pMasterNode->intRegs.has_becctl)
		{
			wBuf[0u] = A2B_REG_BECCTL;
			wBuf[1u] =(a2b_UInt8) pMasterNode->intRegs.becctl;
			status = a2b_i2cSlaveWrite( pPlugin->ctx,(a2b_Int16)nIndex, 2, wBuf );
		}
		wBuf[0u] = A2B_REG_I2SCFG;
		wBuf[1u] =(a2b_UInt8) pSlaveNode->i2cI2sRegs.i2scfg;
		status = a2b_i2cSlaveWrite( pPlugin->ctx,(a2b_Int16)nIndex, 2, wBuf );
    }

    /* Master  pre-set */
    /* Restore master configuration */
	wBuf[0u] = A2B_REG_I2SCFG;
	wBuf[1u] = (a2b_UInt8)pMasterNode->i2cI2sRegs.i2scfg;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

     /* Set node address to zero */
	wBuf[0u] = A2B_REG_NODEADR;
	wBuf[1u] = 0x0u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    /* Enable only downstream */
	wBuf[0u] = A2B_REG_DATCTL;
	wBuf[1u] = 0x1u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	wBuf[0u] = A2B_REG_CONTROL;
	wBuf[1u] = 0x1u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

    /* Delay for new structure to take effect - minimum of 5 super frames */
	a2b_ActiveDelay(pPlugin->ctx, 5u);

    /* Re-enable Both up and down-stream */
	wBuf[0u] = A2B_REG_DATCTL;
	wBuf[1u] = nDATCTLValue;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );

	wBuf[0u] = A2B_REG_CONTROL;
	wBuf[1u] = 0x1u;
	status = a2b_i2cMasterWrite( pPlugin->ctx, 2, wBuf );
}
#endif
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
