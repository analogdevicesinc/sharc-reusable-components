/*******************************************************************************
Copyright (c) 2018 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
 * @file:    adi_a2b_commch_mstr.h
 * @brief:   This  header file contains structure definitions for Communication channel on the master side
 * @version: $Revision: 7702 $
 * @date:    $Date: 2018-02-23 15:32:54 +0530 (Fri, 23 Feb 2018) $
 * Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/** \addtogroup Communication_Channel_Master
 *  @{
 */

#ifndef ADI_A2B_COMMCH_MSTR_H_
#define	ADI_A2B_COMMCH_MSTR_H_

#include "adi_a2b_commch_engine.h"
#include "a2b/ctypes.h"

#define A2B_COMMCH_MSTR_RX_MAILBOX_NO			(1u) 	/*!< Default Mailbox No on which Slave transmits and master receives */
#define A2B_COMMCH_MSTR_TX_MAILBOX_NO			(0u) 	/*!< Default Mailbox No on which Master transmits and slave receives */
#define A2B_MAILBOX_TX_RESPONSE_TIMEOUT_IN_MS	(10u)	/*!< Timeout for mailbox read response for a transmission specified in milliseconds */

/*! \struct a2b_CommChMstrConfig
    Communication channel master configuration
*/
typedef struct a2b_CommChMstrConfig
{
	/*!< Pointer to master plugin call back function */
	pfStatusCb					pfStatCb;

	/*!< Pointer to master callback parameter */
	a2b_Handle					pCbParam;

	/*!< Pointer to a block of memory */
	a2b_Handle					pMem;

	/*!< Stack ctx pointer */
	struct a2b_StackContext* 	ctx;

	/*!< This is the mailbox (job queue) used for communicating from communication channel to master plugin  */
	a2b_Handle					mboxHnd;

}a2b_CommChMstrConfig;

/*! \struct a2b_CommChMstrInfo
    Communication channel master state information
*/
typedef struct a2b_CommChMstrInfo
{
	/*!< A2B comm channel master Tx msg state. */
	A2B_COMMCH_TX_MSG_STATE		eCommChMsgTxState;

	/*!< Flag to indicate the acknowledgment of message read by the slave. This flag is set in comm channel master */
	volatile a2b_Bool 			abReadComplete[ADI_A2B_COMMCH_ENG_MAX_NO_OF_MAILBOX];

	/*!< Flag to indicate the timeout of 4 byte transmitted message. This flag is set on the timeout event from msater plugin */
	volatile a2b_Bool 			bTimeout;

	struct a2b_StackContext* 	ctx;

	/*!< This is the mailbox (job queue) used for communicating from communication channel to master plugin  */
	a2b_Handle					mboxHnd;

	/*!< Instance of Comm channel engine */
	a2b_CommChEngInfo			oCommChEngInfo;

}a2b_CommChMstrInfo;



void 	   		adi_a2b_CommChMstrOpen(struct a2b_StackContext* ctx);
void 			adi_a2b_CommChMstrClose(struct a2b_StackContext* ctx);
a2b_Handle 		adi_a2b_CommChMstrCreate(a2b_CommChMstrConfig *pCommChMstrConfig);
A2B_COMMCH_RET 	adi_a2b_CommChMstrTxMsg(a2b_Handle hCommChMstr, a2b_CommChMsg *pMsg , a2b_Int8 nNodeAddr);
A2B_COMMCH_RET 	adi_a2b_CommChMstrTick(a2b_Handle *hCommChMstr);



#endif /* ADI_A2B_COMMCH_MSTR_H_ */

/**@}*/
