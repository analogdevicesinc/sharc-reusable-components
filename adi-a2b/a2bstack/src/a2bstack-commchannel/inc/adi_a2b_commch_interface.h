/*******************************************************************************
Copyright (c) 2018 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
 * @file:    adi_a2b_commch_interface.h
 * @brief:   This  header file contains interfaces and messages to be used by comm channel
 * @version: $Revision: 7702 $
 * @date:    $Date: 2018-02-23 15:32:54 +0530 (Fri, 23 Feb 2018) $
 * Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/** \addtogroup Communication_Channel
 *  @{
 */

#ifndef ADI_A2B_COMMCH_INTERFACE_H_
#define	ADI_A2B_COMMCH_INTERFACE_H_

#include "adi_a2b_commch_engine.h"

/*============= D E F I N E S =============*/
/* Message ID's upto 0x0A are reserved to be used within master plugin */
/* !! DO NOT CHANGE THE BELOW MACRO DEFINITIONS !! */
#define A2B_COMMCH_MSG_REQ_SLV_NODE_SIGNATURE	(0x01u)	/*!< Communication channel message: To request a slave node authentication sequence */
#define A2B_COMMCH_MSG_RSP_SLV_NODE_SIGNATURE	(0x06u)	/*!< Communication channel message: Slave node Response for  A2B_COMMCH_MSG_REQ_SLV_NODE_SIGNATURE */
#define A2B_COMMCH_MSG_CUSTOM_START            	(0x0Bu)	/*!< Communication channel custom message start. Anything beyond this value is considered a custom communication channel message */
#define A2B_COMMCH_MSG_MAX						(0x3Fu)	/*!< Communication channel message: Maximum message possible */

/*============= G L O B A L  P R O T O T Y P E S =============*/

A2B_COMMCH_RET 	adi_a2b_app_CommChInit(void);
void			adi_a2b_app_CommChTick(void);
A2B_COMMCH_RET 	adi_a2b_app_CommChSendMsg(a2b_UInt8 nMsgId, a2b_UInt16 nMsgLenInBytes, a2b_UInt8* pMsgPayload, a2b_Int16 nNodeAddr);
void 			adi_a2b_app_CommChCallBk(a2b_UInt8 nMsgId, a2b_UInt16 nMsgLenInBytes, a2b_UInt8* pMsgPayload, a2b_Int16 nNodeAddr, A2B_COMMCH_EVENT eEventType);

#endif /* ADI_A2B_COMMCH_INTERFACE_H_ */

/**@}*/
