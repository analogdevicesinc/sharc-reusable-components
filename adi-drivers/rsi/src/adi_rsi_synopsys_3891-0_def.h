/******************************************************************************

Copyright (c) 2019 Analog Devices.  All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.
 *******************************************************************************
 *
 * @file:    adi_rsi_synopsys_3891-0_def.h
 * @brief:   RSI device driver local definition file for Synopsys MSI product 3891-0
 * @version: $Revision: 62246 $
 * @date:    $Date: 2019-07-18 05:50:49 -0400 (Thu, 18 Jul 2019) $
 *
 ******************************************************************************/
 
#ifndef _ADI_RSI_SYNOPSYS_3891_0_DEF_H_
#define _ADI_RSI_SYNOPSYS_3891_0_DEF_H_

#include <drivers/rsi/adi_rsi.h>

/******************************************************
 Internal constants:
 *******************************************************/

/* The IDMA controller can transfer up to 8191 (0x1FFF) bytes per descriptor
 * (limited by the 13-bit size fields in the descriptor) but we limit our
 * transfers to a multiple of 512 so that descriptor chaining events happen
 * on block boundaries.
 */
#define TRANSFER_LIMIT 7680u

/* The number of DMA descriptors is calculated as the minumum number that will
 * support a transfer of ADI_RSI_MAX_TRANSFER_BYTES in a single (chained) DMA
 * operation. For the current ADI_RSI_MAX_TRANSFER_BYTES of 65536 (64KBytes)
 * this gives NUM_DMA_DESCRIPTORS of 9.
 */
#define NUM_DMA_DESCRIPTORS (((ADI_RSI_MAX_TRANSFER_BYTES - 1u) / TRANSFER_LIMIT) + 1u)

#define ADI_RSI_HW_ERR_NONE 0u

/******************************************************
 Internal structure types:
 *******************************************************/

struct IDMADescriptor
{
 	uint32_t DES0;
 	uint32_t DES1;
 	void * DES2;
 	struct IDMADescriptor * DES3;
};

struct ADI_RSI_DEVICE
{
	struct IDMADescriptor vDmaDescriptors[NUM_DMA_DESCRIPTORS];
	uint32_t              cmdDataModeFlags;
	ADI_RSI_CARD_TYPE     cardType;
	ADI_OSAL_SEM_HANDLE   hDmaCompleteSem;
	uint32_t              openCount;
	ADI_RSI_RESULT        transferResult;
	uint8_t              *pCurrBuffer;
	uint32_t              currBufferSize;
	volatile bool         cardDetected;
	ADI_CALLBACK          pfAppCallback;
	void                 *CBparam;
	uint32_t              cardEventMask;
	uint32_t              rsiStatusEvent;
};

#endif /* _ADI_RSI_SYNOPSYS_3891_0_DEF_H_ */
