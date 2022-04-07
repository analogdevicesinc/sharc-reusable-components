/*!
*********************************************************************************
 *
 * @file:    adi_gemac.c
 *
 * @brief:   Ethernet GEMAC driver source file
 *
 * @version: $Revision: 62469 $
 *
 * @date:    $Date: 2019-08-26 13:37:40 -0400 (Mon, 26 Aug 2019) $
 * ------------------------------------------------------------------------------
 *
 * Copyright (c) 2011-2019 Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Modified versions of the software must be conspicuously marked as such.
 * - This software is licensed solely and exclusively for use with processors
 *   manufactured by or for Analog Devices, Inc.
 * - This software may not be combined or merged with other code in any manner
 *   that would cause the software to become subject to terms and conditions
 *   which differ from those listed here.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights of one
 *   or more patent holders.  This license does not release you from the
 *   requirement that you obtain separate licenses from these patent holders
 *   to use this software.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
 * PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/** \defgroup GEMAC_Driver BF60x On-Chip EMAC Driver
 *  @{
 */
#include <string.h>
#include <sys/platform.h>
#include "adi_ether.h"
#include "adi_ether_gemac.h"
#include <services/int/adi_int.h>
#include "adi_gemac_int.h"
#include "adi_gemac_module.h"
#include "adi_phy_int.h"
#include <adi_osal.h>
#include <stdio.h>
#include <assert.h>

#ifdef ADI_DEBUG
#define ASSERT(X)     assert(X)
#else
#define ASSERT(X)
#endif

#ifdef _MISRA_RULES
/* Rule 14.5(Req) : The continue statement shall not be used. */
#pragma diag (suppress: misra_rule_14_5: "Continue statement required")
/* Rule 14.6(Req) : For any iteration statement there shall be at most one break statement used for loop termination. */
#pragma diag (suppress: misra_rule_14_6: "Multiple break required")
#endif

#ifdef _MISRA_RULES
/* Rule 14.5(Req) : The continue statement shall not be used. */
#pragma diag (suppress: misra_rule_14_5: "Continue statement required")
/* Rule 14.6(Req) : For any iteration statement there shall be at most one break statement used for loop termination. */
#pragma diag (suppress: misra_rule_14_6: "Multiple break required")
#endif



/* function prototypes */
static ADI_ETHER_RESULT activate_channel(ADI_ETHER_HANDLE hDevice,
                                         ADI_EMAC_CHANNEL *pChannel,
                                         int32_t nDMADeviceNum
                                         );

static ADI_ETHER_RESULT insert_queue(ADI_EMAC_FRAME_Q *pQueue,
                                      ADI_ETHER_BUFFER *pBuffer);

static ADI_ETHER_RESULT bind_buf_with_desc(ADI_ETHER_HANDLE hDevice,ADI_EMAC_CHANNEL *pChannel, int32_t nDMADeviceNum);

static ADI_ETHER_RESULT init_descriptor_memory(
											   ADI_ETHER_HANDLE phDevice,
											   const uint8_t *pMemory,
											   const uint32_t Length,
											   ADI_EMAC_CHANNEL *pChannel
											   );

static ADI_ETHER_RESULT init_descriptor (
										 ADI_ETHER_HANDLE phDevice,
										 ADI_EMAC_CHANNEL *pChannel
										 );

static uint32_t gemac_init(ADI_ETHER_HANDLE * const phDevice);


ADI_ETHER_RESULT add_multicastmac_filter(ADI_ETHER_HANDLE * const phDevice,
                                         uint32_t GroupIpAddress,
                                         bool bAddAddress);

static void flush_area(void *start, uint32_t bytes);
static void flushinv_area(void *start, uint32_t bytes);

/*! Enters critical region */
#define ENTER_CRITICAL_REGION()  (adi_osal_EnterCriticalRegion())
/*! Exit critical region */
#define EXIT_CRITICAL_REGION()   (adi_osal_ExitCriticalRegion())

/*! DMA Channel Number check */
#ifdef ADI_DEBUG
#define CHECK_DMA_CHANNEL_NUM(X)   do {                         \
    ASSERT((X) < GEMAC_SUPPORT_NUM_DMA_DEVICES);                \
    CHECK_DMA_CHANNEL_NUM_1((X));                               \
    CHECK_DMA_CHANNEL_NUM_2((X));                               \
    } while (0);
#else
#define CHECK_DMA_CHANNEL_NUM(X)
#endif /* ADI_DEBUG */

#ifdef GEMAC_SUPPORT_DMA1
#define CHECK_DMA_CHANNEL_NUM_1(X)    if ((X) == 1u) { ASSERT((pDev->Capability & ADI_EMAC_CAPABILITY_AV_DMA1) == ADI_EMAC_CAPABILITY_AV_DMA1); }
#else
#define CHECK_DMA_CHANNEL_NUM_1(X)
#endif /* GEMAC_SUPPORT_DMA1 */

#ifdef GEMAC_SUPPORT_DMA2
#define CHECK_DMA_CHANNEL_NUM_2(X)    if ((X) == 1u) { ASSERT((pDev->Capability & ADI_EMAC_CAPABILITY_AV_DMA2) == ADI_EMAC_CAPABILITY_AV_DMA2); }
#else
#define CHECK_DMA_CHANNEL_NUM_2(X)
#endif /* GEMAC_SUPPORT_DMA2 */


/*!  EMAC0 and EMAC1 Driver instances */
static ADI_EMAC_DEVICE gEMAC0 = {0};
#ifdef GEMAC_SUPPORT_EMAC1
static ADI_EMAC_DEVICE gEMAC1 = {0};
#endif
/*!  EMAC0 Driver Entrypoints */
extern ADI_ETHER_DRIVER_ENTRY  GEMAC0DriverEntry;
#ifdef GEMAC_SUPPORT_EMAC1
/*!  EMAC1 Driver Entrypoints */
extern ADI_ETHER_DRIVER_ENTRY  GEMAC1DriverEntry;
#endif

/* returns the pointer to gemac registers */
ALWAYS_INLINE
ADI_EMAC_REGISTERS* get_gemac_regptr(ADI_ETHER_HANDLE *const phDevice)
{
    return ((ADI_EMAC_DEVICE*)phDevice)->pEMAC_REGS;
}

/* return gemac version */
ALWAYS_INLINE
uint32_t gemac_version(ADI_ETHER_HANDLE phDevice)
{
#ifndef GEMAC_VERSION
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
    return (pEmacRegs->EMAC_VER);
#else
    return GEMAC_VERSION;
#endif
}

/* set media clock range */
ALWAYS_INLINE
void gemac_set_mdcclk(ADI_ETHER_HANDLE phDevice,
                     const uint32_t mdcClkRange)
{
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
    pDev->MDCClockRange   = mdcClkRange;
}

/* set dma operation mode */
ALWAYS_INLINE
void gemac_set_dmaopmode(
						 ADI_ETHER_HANDLE phDevice,
						 const uint32_t nDMAChannel,
						 const uint32_t opMode
						 )
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister[nDMAChannel] = opMode;
}

/* clear gemac interrupts */
ALWAYS_INLINE
uint32_t gemac_clr_interrupts(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif
    uint32_t status;

    volatile uint32_t *pDMARegister[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister[0] = &pEmacRegs->EMAC_DMA_STAT;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister[1] = &pEmacRegs->EMAC_DMA1_STAT;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister[2] = &pEmacRegs->EMAC_DMA2_STAT;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    status = *pDMARegister[nDMAChannel];
    *pDMARegister[nDMAChannel] = status;

    return(status);
}

ALWAYS_INLINE
void* gemac_get_cur_rxdesc_addr(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister[0] = &pEmacRegs->EMAC_DMA_RXDSC_ADDR;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister[1] = &pEmacRegs->EMAC_DMA1_RXDSC_ADDR;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister[2] = &pEmacRegs->EMAC_DMA2_RXDSC_ADDR;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    return ((void*)*pDMARegister[nDMAChannel]);
}

ALWAYS_INLINE
void* gemac_get_cur_txdesc_addr(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister[0] = &pEmacRegs->EMAC_DMA_TXDSC_ADDR;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister[1] = &pEmacRegs->EMAC_DMA1_TXDSC_ADDR;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister[2] = &pEmacRegs->EMAC_DMA2_TXDSC_ADDR;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    return ((void*)*pDMARegister[nDMAChannel]);
}

/* returns false if finished receive descriptor has any errors */
ALWAYS_INLINE
bool gemac_rx_desc_valid(const uint32_t status)
{
    return ((status & ENUM_DS_DESC_ERR) == 0) 						 &&
    	   ((status & ENUM_DS_RXFIRST_DESC) == ENUM_DS_RXFIRST_DESC) &&
           ((status & ENUM_DS_RXLAST_DESC) == ENUM_DS_RXLAST_DESC) ;
}

/* returns if descriptor is valid */
ALWAYS_INLINE
bool gemac_is_desc_valid(const uint32_t status)
{
    return ((status & ENUM_DS_DESC_ERR) == 0);
}

/* returns if tx is suspended */
ALWAYS_INLINE
bool gemac_is_txbuf_unavail(const uint32_t status)
{
    return ((status & BITM_EMAC_DMA_STAT_TU));
}

/* returns if rx is suspended */
ALWAYS_INLINE
bool gemac_is_rxbuf_unavail(const uint32_t status)
{
    return ((status & BITM_EMAC_DMA_STAT_RU));
}

/* set mac configuration */
ALWAYS_INLINE
void gemac_set_maccfg(ADI_ETHER_HANDLE phDevice,const uint32_t macCfg)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
    pEmacRegs->EMAC_MACCFG = macCfg;
}

/* set mac interrupt mask */
ALWAYS_INLINE
void gemac_set_macimask(ADI_ETHER_HANDLE phDevice,const uint32_t iMask)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
    pEmacRegs->EMAC_IMSK |= iMask;
}

/* reset given queue */
ALWAYS_INLINE
void reset_queue(ADI_EMAC_FRAME_Q *pQueue)
{
    pQueue->pQueueHead = NULL;
    pQueue->pQueueTail = NULL;
    pQueue->ElementCount = 0;
}

/* reset DMA Channel */
ALWAYS_INLINE
void reset_dma_channel(ADI_EMAC_DMA_CHANNEL* const pDmaChannel) {
	reset_queue(&pDmaChannel->Active);
	reset_queue(&pDmaChannel->Pending);
	reset_queue(&pDmaChannel->Queued);
	reset_queue(&pDmaChannel->Completed);

	pDmaChannel->pDmaDescHead = NULL;
	pDmaChannel->pDmaDescTail = NULL;
	pDmaChannel->NumAvailDmaDesc = 0u;
}



/* reset all queues in the ethernet device */
ALWAYS_INLINE
void reset_all_queues(ADI_EMAC_DEVICE * const pDev)
{
	pDev->Rx.Recv    = true;

    /* rest all queue structures for DMA0 */
	reset_dma_channel(&pDev->Rx.DMAChan[0]);
	reset_dma_channel(&pDev->Tx.DMAChan[0]);

	/* If DMA1 is supported */
	if (pDev->Capability & ADI_EMAC_CAPABILITY_AV_DMA1) {
	    /* rest all queue structures for DMA1 */
		reset_dma_channel(&pDev->Rx.DMAChan[1]);
		reset_dma_channel(&pDev->Tx.DMAChan[1]);
	}

	/* If DMA2 is supported */
	if (pDev->Capability & ADI_EMAC_CAPABILITY_AV_DMA2) {
	    /* rest all queue structures for DMA2 */
		reset_dma_channel(&pDev->Rx.DMAChan[2]);
		reset_dma_channel(&pDev->Tx.DMAChan[2]);
	}
}


/* copy queue elements from source to destination */
ALWAYS_INLINE
void copy_queue_elements(ADI_EMAC_FRAME_Q *pDstQ,
                                       ADI_EMAC_FRAME_Q *pSrcQ)
{
    pDstQ->pQueueHead = pSrcQ->pQueueHead;
    pDstQ->pQueueTail = pSrcQ->pQueueTail;
    pDstQ->ElementCount = pSrcQ->ElementCount;
}

/* reset dma lists of a channel */
ALWAYS_INLINE
void reset_dma_lists(ADI_EMAC_CHANNEL *pChannel)
{
    pChannel->nDMADescNum = 0u;
    pChannel->pDMADescMem = NULL;
}

/* resume tx dma channel */
ALWAYS_INLINE
void resume_tx(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
    volatile uint32_t *pDMARegister_STAT[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    volatile uint32_t *pDMARegister_TXPOLL[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    volatile uint32_t *pDMARegister_OPMODE[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    uint32_t TxDmaStatus;
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    pDMARegister_STAT[0] = &pEmacRegs->EMAC_DMA_STAT;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_STAT[1] = &pEmacRegs->EMAC_DMA1_STAT;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_STAT[2] = &pEmacRegs->EMAC_DMA2_STAT;
#endif

    pDMARegister_TXPOLL[0] = &pEmacRegs->EMAC_DMA_TXPOLL;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_TXPOLL[1] = &pEmacRegs->EMAC_DMA1_TXPOLL;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_TXPOLL[2] = &pEmacRegs->EMAC_DMA2_TXPOLL;
#endif

    pDMARegister_OPMODE[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_OPMODE[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_OPMODE[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    TxDmaStatus = *pDMARegister_STAT[nDMAChannel] & BITM_EMAC_DMA_STAT_TS;

    if (TxDmaStatus == ENUM_EMAC_DMA_STAT_TS_SUSPENDED) {
    	*pDMARegister_TXPOLL[nDMAChannel] = 0x1;
    }
    else if (TxDmaStatus == ENUM_EMAC_DMA_STAT_TS_STOPPED) {
    	*pDMARegister_OPMODE[nDMAChannel] |= BITM_EMAC_DMA_OPMODE_ST;
    }
}

/* stop receive dma */
ALWAYS_INLINE
void gemac_stop_rx(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
    volatile uint32_t *pDMARegister_OPMODE[GEMAC_SUPPORT_NUM_DMA_DEVICES];
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    pDMARegister_OPMODE[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_OPMODE[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_OPMODE[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister_OPMODE[nDMAChannel] &= ~(BITM_EMAC_DMA_OPMODE_SR);
}

/* stop transmit dma */
ALWAYS_INLINE
void gemac_stop_tx(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
    volatile uint32_t *pDMARegister_OPMODE[GEMAC_SUPPORT_NUM_DMA_DEVICES];
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    pDMARegister_OPMODE[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_OPMODE[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_OPMODE[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister_OPMODE[nDMAChannel] &= ~(BITM_EMAC_DMA_OPMODE_ST);
}

/* resume receive dma */
ALWAYS_INLINE
void resume_rx(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
	ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister_STAT[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    volatile uint32_t *pDMARegister_RXPOLL[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    volatile uint32_t *pDMARegister_OPMODE[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    uint32_t RxDmaStatus;

    pDMARegister_STAT[0] = &pEmacRegs->EMAC_DMA_STAT;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_STAT[1] = &pEmacRegs->EMAC_DMA1_STAT;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_STAT[2] = &pEmacRegs->EMAC_DMA2_STAT;
#endif

    pDMARegister_RXPOLL[0] = &pEmacRegs->EMAC_DMA_RXPOLL;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_RXPOLL[1] = &pEmacRegs->EMAC_DMA1_RXPOLL;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_RXPOLL[2] = &pEmacRegs->EMAC_DMA2_RXPOLL;
#endif

    pDMARegister_OPMODE[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_OPMODE[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_OPMODE[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    RxDmaStatus = *pDMARegister_STAT[nDMAChannel] & BITM_EMAC_DMA_STAT_RS;

    if( RxDmaStatus == ENUM_EMAC_DMA_STAT_RS_SUSPENDED) {
    	*pDMARegister_RXPOLL[nDMAChannel] = 0x1;
    }
    else if (RxDmaStatus == ENUM_EMAC_DMA_STAT_RS_STOPPED) {
    	*pDMARegister_OPMODE[nDMAChannel] |= BITM_EMAC_DMA_OPMODE_SR; // stopped
    }
}

/* enable rx dma */
ALWAYS_INLINE
void enable_rx(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister_OPMODE[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister_OPMODE[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_OPMODE[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_OPMODE[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister_OPMODE[nDMAChannel] |= BITM_EMAC_DMA_OPMODE_SR;
}

/* enable tx dma */
ALWAYS_INLINE
void enable_tx(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister_OPMODE[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister_OPMODE[0] = &pEmacRegs->EMAC_DMA_OPMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_OPMODE[1] = &pEmacRegs->EMAC_DMA1_OPMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_OPMODE[2] = &pEmacRegs->EMAC_DMA2_OPMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister_OPMODE[nDMAChannel] |= BITM_EMAC_DMA_OPMODE_ST;
}

/* set dma bus mode register */
ALWAYS_INLINE
void gemac_set_dmabusmode(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel, const uint32_t BusModeValue)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister[0] = &pEmacRegs->EMAC_DMA_BUSMODE;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister[1] = &pEmacRegs->EMAC_DMA1_BUSMODE;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister[2] = &pEmacRegs->EMAC_DMA2_BUSMODE;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister[nDMAChannel] = BusModeValue;
}


/* mask ethernet interrupts */
ALWAYS_INLINE
void mask_gemac_ints(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister_DMA_IEN[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister_DMA_IEN[0] = &pEmacRegs->EMAC_DMA_IEN;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_DMA_IEN[1] = &pEmacRegs->EMAC_DMA1_IEN;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_DMA_IEN[2] = &pEmacRegs->EMAC_DMA2_IEN;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister_DMA_IEN[nDMAChannel] = 0;
}

/* umask ethernet interrupts */
ALWAYS_INLINE
void unmask_gemac_ints(ADI_ETHER_HANDLE phDevice, const uint32_t nDMAChannel)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = get_gemac_regptr(phDevice);
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

    volatile uint32_t *pDMARegister_DMA_IEN[GEMAC_SUPPORT_NUM_DMA_DEVICES];
    pDMARegister_DMA_IEN[0] = &pEmacRegs->EMAC_DMA_IEN;
#ifdef GEMAC_SUPPORT_DMA1
    pDMARegister_DMA_IEN[1] = &pEmacRegs->EMAC_DMA1_IEN;
#endif
#ifdef GEMAC_SUPPORT_DMA2
    pDMARegister_DMA_IEN[2] = &pEmacRegs->EMAC_DMA2_IEN;
#endif

    CHECK_DMA_CHANNEL_NUM(nDMAChannel);

    *pDMARegister_DMA_IEN[nDMAChannel] = ADI_EMAC_AIS_NIS_INTERRUPTS;
}

static ADI_ETHER_RESULT bind_desc_and_activate (ADI_ETHER_HANDLE hDevice,ADI_EMAC_CHANNEL *pChannel, int32_t nDMADeviceNum)
{
	ADI_ETHER_RESULT eResult = ADI_ETHER_RESULT_SUCCESS;
	ADI_EMAC_DEVICE*  const  pDev = (ADI_EMAC_DEVICE*)hDevice;

	if (
		   (nDMADeviceNum == 0)
#ifdef ADI_ETHER_SUPPORT_AV
        || (   (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
            && (
		           ((nDMADeviceNum == 1) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_RX_EN) && (pChannel->Recv))
		        || ((nDMADeviceNum == 1) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_TX_EN) && (!pChannel->Recv))
		        || ((nDMADeviceNum == 2) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_RX_EN) && (pChannel->Recv))
		        || ((nDMADeviceNum == 2) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_TX_EN) && (!pChannel->Recv))
            ))
#endif
		)
	{
		bind_buf_with_desc (hDevice, pChannel, nDMADeviceNum);
		activate_channel (hDevice, pChannel, nDMADeviceNum);
    }

    return eResult;
}

/**
 * @brief       Configure EMAC for the given autonegotiation values
 *
 * @details     gemac_autonego_config function is responsible for configuring the MAC for
 *              duplex and speed configuration based on the autonegotiation status.
 *              Typically this function gets called from the PHYInterruptHandler once
 *              auto-negotiation is successfully completed.
 *              MAC configuration is setup in configuration register (MACCFG) frame
 *              filter register (MACFRMFILT) and flow control register (MACFLOWCTL)
 *
 *              This routine sets the MACCFG register with the following bits
 *
 *              # enable jabber
 *              # burst enable
 *              # disable jumbo frame
 *              # enable multicast hash function
 *              # full/half duplex mode
 *
 *              The routine sets the following frame filter register bits
 *
 *              # multicast hash bits
 *              # unicast hash bits
 *
 *              The routine sets the following flow control register bits
 *
 *              # receive flow control enable/disable
 *              # transmit flow control enable/disable
 *
 * @param [in]  hDevice  Device Handle.
 *
 * @param [in]  nAutoNegoStatus Autonegotiation status. Ored values of ADI_PHY_AUTONEGOTIATE_STATUS.
 */
static void gemac_autonego_config(ADI_ETHER_HANDLE hDevice, uint32_t nAutoNegoStatus)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)hDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    volatile uint32_t phyRegData;

    /* MAC configuration */
    phyRegData =  pEmacRegs->EMAC_MACCFG;

    /* jabber enable */
    phyRegData |=  BITM_EMAC_MACCFG_JB;

    phyRegData &=  ~( (1UL << BITP_EMAC_MACCFG_DO)  |   /* disable receive own */
                      (1UL << BITP_EMAC_MACCFG_LM)  |   /* disable loopback    */
                      (1UL << BITP_EMAC_MACCFG_DR)  |   /* enable retry        */
                      (1UL << BITP_EMAC_MACCFG_ACS) |   /* automatic pad stripping */
                      (1UL << BITP_EMAC_MACCFG_DC)      /* disable defferal check */
                    );
    /* set the duplex mode */
    if (nAutoNegoStatus & ADI_PHY_AN_FULL_DUPLEX) {
    	phyRegData |= BITM_EMAC_MACCFG_DM;
    } else {
    	phyRegData &= (~BITM_EMAC_MACCFG_DM);
    }

    /* Set the configuration required for 10/100/1000Mbps speed */
    if (nAutoNegoStatus & ADI_PHY_AN_1000Mbps) {
    	phyRegData &= (~0x00008000);
    } else {
        phyRegData |= 0x00008000;
        if (nAutoNegoStatus & ADI_PHY_AN_100Mbps) {
        	phyRegData |= BITM_EMAC_MACCFG_FES;
        } else {
        	phyRegData &= ~BITM_EMAC_MACCFG_FES;
        }
    }

    /* Update MACCFG with the new value */
    pEmacRegs->EMAC_MACCFG = phyRegData;

    /* MAC Frame filter configuration */
    phyRegData = pEmacRegs->EMAC_MACFRMFILT;

    phyRegData  |=  ( BITM_EMAC_MACFRMFILT_HMC  | /* enable multicast hash filter */
                      BITM_EMAC_MACFRMFILT_HUC    /* enable unicast hash filter   */
                    );

    pEmacRegs->EMAC_MACFRMFILT = phyRegData;

    if (nAutoNegoStatus & ADI_PHY_AN_FULL_DUPLEX) {
    	/* rx,tx flow control */
    	pEmacRegs->EMAC_FLOWCTL |= ( BITM_EMAC_FLOWCTL_RFE | BITM_EMAC_FLOWCTL_TFE);
    } else {
        /* rx,tx flow control - disable */
        pEmacRegs->EMAC_FLOWCTL &= ~( BITM_EMAC_FLOWCTL_RFE | BITM_EMAC_FLOWCTL_TFE);
    }
}

/**
 * @brief       PHY interrupt handler
 *
 * @details     PHY Interrupt handler gets invoked because of a PHY event. PHY events includes
 *              link up, link down, auto-negotiation completion etc.
 *
 *              Upon enabling EMAC (adi_Ether_EnableMAC) the EMAC and phy are by default
 *              configured for auto negotiation mode. Once auto negotiation is complete PHY
 *              interrupt gets generated which will invoke this handler.
 *
 *              In this handler the operational mode (speed, duplex) connectivity is
 *              determined and the EMAC block is configured accordingly.
 *
 *              Callback is invoked with ADI_ETHER_EVENT_PHY_INTERRUPT event along with
 *              the PHY status.The status information indicates the speed and the duplex
 *              nature of the connections.Potential bit enumerations for the status are given below
 *
 *              - ADI_ETHER_PHY_LINK_DOWN
 *              - ADI_ETHER_PHY_LINK_UP
 *              - ADI_ETHER_PHY_10T_FULL_DUPLEX
 *              - ADI_ETHER_PHY_10T_HALF_DUPLEX
 *              - ADI_ETHER_PHY_100T_FULL_DUPLEX
 *              - ADI_ETHER_PHY_100T_HALF_DUPLEX
 *              - ADI_ETHER_PHY_AN_COMPLETE
 *              - ADI_ETHER_PHY_LOOPBACK
 *
 * @param [in]  IID       Interrupt Identifier.
 *
 * @param [in]  pCBParm   Callback parameter which contains the EMAC device pointer (&gEMACx)
 */
void PHYCallbackHandler(void* pHandle, uint32_t nEvent, void *Arg)
{
	ADI_EMAC_DEVICE*    const  pDev = (ADI_EMAC_DEVICE*)pHandle;
	ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;

    switch(nEvent)
    {
    case (uint32_t)ADI_PHY_EVENT_AUTO_NEGOTIATION_COMP:
        {
            uint32_t nAutoNegStatus = (uint32_t)Arg;
            gemac_autonego_config(pDev, nAutoNegStatus);
            pDev->pEtherCallback(pDev,ADI_ETHER_EVENT_PHY_INTERRUPT,(void*)ADI_ETHER_PHY_AN_COMPLETE,
                pDev->pUsrPtr);
        }
        break;

    case (uint32_t)ADI_PHY_EVENT_LINK_DOWN:
#ifdef GEMAC_SUPPORT_LPI
    	if (pDev->Capability & ADI_EMAC_CAPABILITY_LPI)
    	{
    		pEmacRegs->EMAC_LPI_CTLSTAT &= (~BITM_EMAC_LPI_CTLSTAT_PLS);
    	}
#endif
    	pDev->pEtherCallback(pDev,ADI_ETHER_EVENT_PHY_INTERRUPT,(void*)ADI_ETHER_PHY_LINK_DOWN,
            pDev->pUsrPtr);
    	break;

    case (uint32_t)ADI_PHY_EVENT_LINK_UP:
#ifdef GEMAC_SUPPORT_LPI
    	if (pDev->Capability & ADI_EMAC_CAPABILITY_LPI)
    	{
    		pEmacRegs->EMAC_LPI_CTLSTAT |= BITM_EMAC_LPI_CTLSTAT_PLS;
    	}
#endif
    	pDev->pEtherCallback(pDev,ADI_ETHER_EVENT_PHY_INTERRUPT,(void*)ADI_ETHER_PHY_LINK_UP,
            pDev->pUsrPtr);
    	break;

    default:
        break;
    }
}

/**
 * @brief       Processes incoming or outgoing frames.
 *
 * @details     This function gets invoked from the Ethernet interrupt service routine in case
 *              of receive or transmit complete interrupt occurs. This function checks the
 *              associated active queue for receive or transmit channels and processes it.
 *              processing include removing the association of dma descriptor from the processed
 *              buffer and placing the descriptor in the free list. Also the buffer that is
 *              processed is returned via the callback handler. The dma descriptors are now
 *              avaialble for the resepective channels.
 *
 *              Once buffers are returned to the application, the frame completed queue is reset
 *              to zero.
 *
 * @param [in]  hDevice    Device Handle.
 *
 * @param [in]  pChannel   Receive or transmit channel
 *
 * @note                   This function may return more than one buffer in the callback.
 */
static void process_int(ADI_EMAC_DEVICE *pDev,ADI_EMAC_CHANNEL *pChannel, int32_t nDMADeviceNum)
{
	ADI_EMAC_DMA_CHANNEL *pDMAChannel = &pChannel->DMAChan[nDMADeviceNum];
    ADI_ETHER_BUFFER *pProcessedBuffer = pDMAChannel->Active.pQueueHead;
    ADI_EMAC_DMADESC *pCurDmaDesc = NULL;
    uint32_t          nNumWaitChecks = 0;
    ADI_ETHER_EVENT   Event;

    short *pLength;

    CHECK_DMA_CHANNEL_NUM(nDMADeviceNum);

    while (pProcessedBuffer)
    {
        pCurDmaDesc  = ( (ADI_EMAC_BUFINFO*)pProcessedBuffer)->pDmaDesc;

        /* data cache is enabled then flush and invalidate the descriptor */
        if (pDev->Cache) { SIMPLEFLUSHINV(pCurDmaDesc); }

        /* if any descriptor is owned by host we will break */
        if(pCurDmaDesc->Status & ADI_EMAC_DMAOWN)
        {
        	/* There can be delay in the OWN bit being updated in the DDR memory and hence the core would have read the wrong value.
        	 * If the current DMA descriptor in process in the device is different from the pCurDmaDesc, then it means that
        	 * the device has moved to the next descriptor.
        	 */

        	/* IF (The device current DMA descriptor matches) */
        	if (pChannel->Recv) {

        		if (gemac_get_cur_rxdesc_addr(pDev, nDMADeviceNum) == (void*)pCurDmaDesc) {
        			break;
        		}
        	} else {
        		if (gemac_get_cur_txdesc_addr(pDev, nDMADeviceNum) == (void*)pCurDmaDesc) {
        			break;
        		}
        	}

        	/* Prevent lock up */
            if (nNumWaitChecks++ > 10) {
            	break;
            }
        	continue;
        }

        /* we have atleast one finished buffer */
        pDMAChannel->Active.pQueueHead = pProcessedBuffer->pNext;
        pDMAChannel->Active.ElementCount--;
        pProcessedBuffer->pNext = NULL;
        pCurDmaDesc->pNextDesc  = NULL;

        /* Clear the frame status frag */
        pProcessedBuffer->Status = 0u;

        /* pProcessedBuffer is the currently processed buffer and the pCurDmaDesc is
         * current descriptor. The length of the frame is stored in DESC0 status area.
         */
        if (pChannel->Recv)
        {
            pProcessedBuffer->ProcessedElementCount = ((pCurDmaDesc->Status >> 16) & 0x3FFF);
            pLength = (short*)pProcessedBuffer->Data;
            *pLength = pProcessedBuffer->ProcessedElementCount + 6;
            pProcessedBuffer->ProcessedElementCount += 6;
            pProcessedBuffer->ProcessedFlag = (uint32_t)true;
        }

#ifdef ADI_ETHER_SUPPORT_PTP
        /* Get the timestamp is available */
        if (pDev->Capability & ADI_EMAC_CAPABILITY_PTP)
        {

            if (   (    (pChannel->Recv)
            		&& (pCurDmaDesc->Status & (1u << 7))    /*      Timestamp available bit is set */
            		&& (pCurDmaDesc->Status & (1u << 8)))   /* AND  Last Descriptor bit is set     */
                || (    (!pChannel->Recv)
                	&& (pCurDmaDesc->Status & (1u << 17))   /*      Timestamp available bit is set */
                	&& (pCurDmaDesc->Status & (1u << 29)))   /* AND  Last Segment bit is set     */
                	)
            {
            	/* Mark the status as timestamp available */
            	pProcessedBuffer->Status |= ADI_ETHER_BUFFER_STATUS_TIMESTAMP_AVAIL;

            	pProcessedBuffer->TimeStamp.HSecond = pDev->pEMAC_REGS->EMAC_TM_HISEC;
            	pProcessedBuffer->TimeStamp.LSecond = pCurDmaDesc->TimeStampHi;
            	pProcessedBuffer->TimeStamp.NanoSecond = pCurDmaDesc->TimeStampLo;
            }
        }
#endif

         /* put the buffer in the completed queue */
         insert_queue(&pDMAChannel->Completed,pProcessedBuffer);

         /* see if next buffer in the active list is also done */
         pProcessedBuffer = pDMAChannel->Active.pQueueHead;

         /* place the finished dma descriptor in the available list for the channel */

         if (pDMAChannel->pDmaDescTail != NULL)
         {
        	 pDMAChannel->pDmaDescTail->pNextDesc =  pCurDmaDesc;
        	 pDMAChannel->pDmaDescTail = pCurDmaDesc;
         }
         else
         {
        	 pDMAChannel->pDmaDescHead = pDMAChannel->pDmaDescTail =  pCurDmaDesc;
         }
         pDMAChannel->NumAvailDmaDesc += 1;
    }

    /* determine the event */
    Event = pChannel->Recv ? ADI_ETHER_EVENT_FRAME_RCVD : ADI_ETHER_EVENT_FRAME_XMIT;

    /* Call only if anything is there in there in the completed queue */
    if (pDMAChannel->Completed.pQueueHead)
    {
		/* return the processed buffers to the application */
		pDev->pEtherCallback(pDev,Event,pDMAChannel->Completed.pQueueHead, pDev->pUsrPtr);

		/* reset the completed queue */
		reset_queue(&pDMAChannel->Completed);
    }

    /* if no more active elements in the queue reset the queue */
    if(pDMAChannel->Active.pQueueHead == NULL) {
        reset_queue(&pDMAChannel->Active);
    }

     return;
}

/**
 * @brief       Transfers buffers on either pending on queued
 *
 * @details     This routine gets called from the EMACInterruptHandler once transmission
 *              or reception completes. Because of completion of transmit or receive packet
 *              associated descriptors gets available for the next transfer. This routine
 *              checks for any queued buffers or buffers that are waiting for descriptors
 *              to be available, binds them and schedules for transfer.
 *
 * @param [in]  phDevice   Device handle
 *
 * @param [in]  pChannel   Device channel
 */
static void transfer_queued_bufs(ADI_ETHER_HANDLE phDevice,ADI_EMAC_CHANNEL *pChannel, int32_t nDMADeviceNum)
{
	ADI_EMAC_DMA_CHANNEL* pDMAChannel = &pChannel->DMAChan[nDMADeviceNum];
#ifdef ADI_DEBUG
    ADI_EMAC_DEVICE *pDev = (ADI_EMAC_DEVICE*)phDevice;
#endif

	CHECK_DMA_CHANNEL_NUM(nDMADeviceNum);

    /* if buffers are queued and descriptors are available or
     * if buffers are pending - already binded with descriptors
     * we activate the respective channel
     */
    if (((pDMAChannel->Queued.pQueueHead != NULL) && pDMAChannel->NumAvailDmaDesc > 1 ) ||
        (pDMAChannel->Pending.pQueueHead != NULL))
    {
       bind_buf_with_desc(phDevice,pChannel, nDMADeviceNum);
       activate_channel(phDevice,pChannel, nDMADeviceNum);
    }
}
/**
 * @brief       Handles abnomral ethernet interrupts or events
 *
 * @details     This function gets called only from the etherent interrupt handler because of an
 *              abnormal event. Appropriate action is taken dependeing on the status.
 *
 *              Handles the following errr conditions
 *
 *              # transmit process stopped
 *              # transmit jabber timeout
 *              # receive fifo overflow
 *              # transmit underflow
 *              # receive buffer unavailable
 *              # receive buffer stopped
 *              # receive watchdog timeout
 *              # early transmit interrupt
 *              # fatel bus error
 *
 * @param [in]  pDev       Device Handle.
 *
 * @param [in]  DmaStatus  DMA status
 */
static void handle_abnormal_interrupts(ADI_EMAC_DEVICE *pDev,const uint32_t DmaStatus, int32_t nDMADeviceNum)
{
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    volatile short VAR_UNUSED_DECR(value);

    CHECK_DMA_CHANNEL_NUM(nDMADeviceNum);

    /* transmit process stopped */
    if (DmaStatus & BITM_EMAC_DMA_STAT_TPS)
    {
        STATS_INC(pDev->Stats.TxProcessStopCnt);
    }

    /* transmit jabber timeout */
    if (DmaStatus & BITM_EMAC_DMA_STAT_TJT)
    {
        STATS_INC(pDev->Stats.TxJabberTimeOutCnt);
    }

    /* receive buffer unavailable */
    if (DmaStatus & BITM_EMAC_DMA_STAT_RU)
    {
        STATS_INC(pDev->Stats.RxOvfCnt);
    }

    /* receiver process stopped */
    if (DmaStatus & BITM_EMAC_DMA_STAT_RPS)
    {
        STATS_INC(pDev->Stats.RxProcessStopCnt);
    }

    /* transmit buffer unavailable */
    if (DmaStatus & BITM_EMAC_DMA_STAT_UNF)
    {
        STATS_INC(pDev->Stats.TxUnfCnt);
        transfer_queued_bufs(pDev,&pDev->Tx, nDMADeviceNum);
    }

    /* received watchdog timeout */
    if (DmaStatus & BITM_EMAC_DMA_STAT_RWT)
    {
        STATS_INC(pDev->Stats.RxWDTimeoutCnt);
    }

    /* early transmit interrupt */
    if (DmaStatus & BITM_EMAC_DMA_STAT_ETI)
    {
        STATS_INC(pDev->Stats.EarlyTxIntCnt);
    }

    /* early transmit interrupt */
    if (DmaStatus & BITM_EMAC_DMA_STAT_ERI)
    {
        STATS_INC(pDev->Stats.EarlyRxIntCnt);
    }

    /* fatal buss error */
    if (DmaStatus & BITM_EMAC_DMA_STAT_FBI)
    {
        STATS_INC(pDev->Stats.BusErrorCnt);
    }

    /* mmc interrupt */
    if (DmaStatus & BITM_EMAC_DMA_STAT_MCI)
    {
          STATS_INC(pDev->Stats.MMCIntCnt);
          value = pEmacRegs->EMAC_ISTAT;
          pEmacRegs->EMAC_ISTAT = 0;
          pEmacRegs->EMAC_MMC_CTL = 0x1;
          value = pEmacRegs->EMAC_MMC_RXINT;
          value = pEmacRegs->EMAC_MMC_TXINT;
          value = pEmacRegs->EMAC_IPC_RXINT;
    }
}

#if defined(ADI_ETHER_SUPPORT_PPS) && defined(ADI_ETHER_SUPPORT_ALARM)
void PpsInterruptHandler (ADI_EMAC_DEVICE* pDev)
{
	uint32_t nTMStatus = pDev->pEMAC_REGS->EMAC_TM_STMPSTAT;
	uint32_t nEvent;
	static uint32_t BITM_Err_Int[] = {
			BITM_EMAC_TM_STMPSTAT_TSTRGTERR0,
			BITM_EMAC_TM_STMPSTAT_TSTRGTERR1,
			BITM_EMAC_TM_STMPSTAT_TSTRGTERR2,
			BITM_EMAC_TM_STMPSTAT_TSTRGTERR3
	};
	static uint32_t BITM_TM_Int[] = {
			BITM_EMAC_TM_STMPSTAT_TSTARGT0,
			BITM_EMAC_TM_STMPSTAT_TSTARGT1,
			BITM_EMAC_TM_STMPSTAT_TSTARGT2,
			BITM_EMAC_TM_STMPSTAT_TSTARGT3
	};

	/* Handle all the interrupts */
	{
		uint32_t nDeviceID;

		int x;
		for (x = 0; x < (sizeof(BITM_Err_Int)/sizeof(uint32_t)); x++) {
			if (nTMStatus & (BITM_Err_Int[x] | BITM_TM_Int[x]))
			{
				nDeviceID = x;

                /* IF (Interrupt is generated for enabled devices) */
                if (pDev->PPS_Alarm_Devices[nDeviceID].bEnabled) {
    				if (nTMStatus & BITM_Err_Int[x]) {
    					nEvent = (pDev->PPS_Alarm_Devices[x].bAlarm) ? ADI_ETHER_EVENT_ALARM_ERROR : ADI_ETHER_EVENT_PPS_ERROR;
    					pDev->PPS_Alarm_Devices[x].bEnabled = false;
    					if (pDev->PPS_Alarm_Devices[x].pfCallback != NULL) {
    						pDev->PPS_Alarm_Devices[x].pfCallback(pDev, nEvent, (void*)nDeviceID, pDev->pUsrPtr);
    					}
    				}

    				if (nTMStatus & BITM_TM_Int[x])
                    {
    					nEvent = (pDev->PPS_Alarm_Devices[x].bAlarm)
                                    ? ADI_ETHER_EVENT_ALARM_INTERRUPT : ADI_ETHER_EVENT_PPS_INTERRUPT;
    					if ((pDev->PPS_Alarm_Devices[x].bAlarm) || (pDev->PPS_Alarm_Devices[x].ePulseMode == ADI_ETHER_GEMAC_PPS_PULSE_MODE_SINGLE)) {
    						pDev->PPS_Alarm_Devices[x].bEnabled = false;
    					}
                        pps_set_trigger_mode(pDev, x, PPS_TRIG_MODE_PULSE_ONLY);
    					if (pDev->PPS_Alarm_Devices[x].pfCallback != NULL) {
    						if (pDev->PPS_Alarm_Devices[x].bTriggerPending) {
    							pDev->PPS_Alarm_Devices[x].pfCallback(pDev, nEvent, (void*)nDeviceID, pDev->pUsrPtr);
    						}
    					}
    					pDev->PPS_Alarm_Devices[x].bTriggerPending = false;
    				}
                } else {
                    pps_set_trigger_mode(pDev, nDeviceID, PPS_TRIG_MODE_PULSE_ONLY);
                }
			}
		}

        for (x = 0; x < (sizeof(BITM_Err_Int)/sizeof(uint32_t)); x++) {
            if (    (pDev->PPS_Alarm_Devices[x].bTriggerPending == true)
                 && (pDev->PPS_Alarm_Devices[x].bEnabled == true))
            {
                pDev->pEMAC_REGS->EMAC_TM_CTL |= BITM_EMAC_TM_CTL_TSTRIG;
                break;
            }
        }



	}
}
#endif


/**
 * @brief       EMAC Interrupt handler
 *
 * @details     EMAC interrupt handler is responsible for processing the all EMAC interrupts.
 *              EMAC interrupts include DMA receive, transmit complete interrupts, DMA error
 *              interrupts, Overflow, underflow interrupts, MMC counter and timestamp interrupts.
 *
 *              After processing EMAC interrupt handler acknowledgs the interrupt at the EMAC
 *              source, by writing to the W1C bits of status registers.
 *
 *              By the time this interrupt handler is called all required actions at the SEC
 *              are taken care of. Upon returning from this handler the dispatcher acknowledgs
 *              the interrupt via writing to the SEC_END register.
 *
 *              If rx and tx dma is suspended or stopped and there is a queued buffer that is
 *              ready then the interrupt handler will trigger the DMA by writing to the poll
 *              registers.
 *
 *
 * @param [in]  IID      Interrupt Identifier.
 *
 * @param [in]  pCBParm  Callback parameter which contains the EMAC device pointer (&gEMACx)
 *
 * @note                 This function may return more than one buffer in the callback.
 */
void EMACInterruptHandler(uint32_t IID, void *pCBParm)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)pCBParm;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    uint32_t dma_status[3] = {0};
    uint32_t mashed_status;
    int x;


    SYS_SSYNC;

    dma_status[0] =  pEmacRegs->EMAC_DMA_STAT;
    /* acknowledge dma interrupts except RI and TI */
    pEmacRegs->EMAC_DMA_STAT = dma_status[0] & 0x1FFFF;

#ifdef ADI_ETHER_SUPPORT_AV
    if (   (pDev->Capability      &  ADI_EMAC_CAPABILITY_AV_DMA1)
    	&& (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN))
    {
    	dma_status[1] =  pEmacRegs->EMAC_DMA1_STAT;
    	/* acknowledge dma interrupts */
    	pEmacRegs->EMAC_DMA1_STAT = dma_status[1] & 0x1FFFF;
    }

    if (   (pDev->Capability      &  ADI_EMAC_CAPABILITY_AV_DMA2)
    	&& (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN))
    {
    	dma_status[2] =  pEmacRegs->EMAC_DMA2_STAT;
    	/* acknowledge dma interrupts */
    	pEmacRegs->EMAC_DMA2_STAT = dma_status[2] & 0x1FFFF;
    }
#endif

    mashed_status = dma_status[0] | dma_status[1] | dma_status[2];

    /* check if we got any interrupt */
    if (mashed_status == 0) {
        return;
    }

#ifdef GEMAC_SUPPORT_RGMII
    if (pDev->Capability & ADI_EMAC_CAPABILITY_RGMII)
    {
		/* GMAC line interface interrupt */
		if (mashed_status & BITM_EMAC_DMA0_STAT_GLI)
		{
			uint32_t IStat = pEmacRegs->EMAC_ISTAT;

			/* SMII or RGMII : Link change event occurred */
			if (IStat & BITM_EMAC_ISTAT_RGMIIIS)
			{
				uint32_t VAR_UNUSED_DECR(nCtrlStatusReg);

				/* Read the RGMII Control and Status Register to clear the interrupt */
				nCtrlStatusReg = pEmacRegs->EMAC_GIGE_CTLSTAT;
			}

			/* General Purpose Input Status event occurred */
			/* PCS (TVI, RTBI, SGMII) Interrupt occurred */
		}
    }
#endif

    /* IEEE-1588 time stamp trigger interrupt */
    if (mashed_status & BITM_EMAC_DMA_STAT_TTI)
    {
#ifdef ADI_ETHER_SUPPORT_PPS
        if (pDev->Capability & ADI_EMAC_CAPABILITY_PPS) {
    	       PpsInterruptHandler(pDev);
        }
#endif
    }

    /* memory management counter interrupts */
    if (mashed_status & BITM_EMAC_DMA_STAT_MCI)
    {

    }


    for (x = (pDev->nNumDmaChannels - 1); x >= 0; x--)
    {
		/* receive frame interrupt */
		if (dma_status[x] & BITM_EMAC_DMA_STAT_RI)
		{
		   STATS_INC(pDev->Stats.RxIntCnt);
		   process_int(pDev,&pDev->Rx, x);
		}

		/* transmit complete interrupt */
		if (dma_status[x] &  BITM_EMAC_DMA_STAT_TI)
		{
		   STATS_INC(pDev->Stats.TxIntCnt);
		   process_int(pDev,&pDev->Tx, x);
		}

		/* abnormal interrupts - errors */
		if (dma_status[x] & BITM_EMAC_DMA_STAT_AIS)
		{
			handle_abnormal_interrupts(pDev,dma_status[x], x);
		}

		/* no buffer to transmit - tx in suspended state check queued buffers */
		if (gemac_is_txbuf_unavail(dma_status[x]))
		{
		   transfer_queued_bufs(pDev,&pDev->Tx, x);
		}

		/* no receive buffer available -rx suspeneded */
		if (gemac_is_rxbuf_unavail(dma_status[x]))
		{
		   transfer_queued_bufs(pDev,&pDev->Rx, x);
		}
	}
}

/**
 * @brief      Sets GEMAC handle
 *
 * @details    Depending on the device id this function will set the GEMAC handle
 *             and associated interrupt
 *
 * @param [in] phDevice   pointer to device handle
 *
 * @param [in] devID      Device ID
 *
 * @note       This function may required to change with different hardware platforms.
 */
static void set_gemac_handle(const ADI_ETHER_DRIVER_ENTRY *pEntryPoint, ADI_ETHER_HANDLE*  const phDevice)
{
	gEMAC0.Interrupt = (uint32_t)INTR_EMAC0_STAT;
	gEMAC0.pPhyDevice = &PhyDevice[0];
	gEMAC0.Capability = GEMAC0_CAPABILITY;
	gEMAC0.nNumDmaChannels = GEMAC0_NUM_DMA_DEVICES;

#ifdef GEMAC_SUPPORT_EMAC1
	gEMAC1.Interrupt = (uint32_t)INTR_EMAC1_STAT;
	gEMAC1.pPhyDevice = &PhyDevice[1];
	gEMAC1.Capability = GEMAC1_CAPABILITY;
	gEMAC1.nNumDmaChannels = GEMAC1_NUM_DMA_DEVICES;
#endif

	if (pEntryPoint == &GEMAC0DriverEntry)
	{
		*phDevice = &gEMAC0;
	}
#ifdef GEMAC_SUPPORT_EMAC1
	else if (pEntryPoint == &GEMAC1DriverEntry)
	{
		*phDevice = &gEMAC1;
	}
#endif
	else
	{
		*phDevice = NULL;
	}
}

/**
 * @brief       Sets the default init parameters
 *
 * @details     Sets up the driver with default initialization parameters
 *
 * @param [in]  phDevice    pointer to device handle
 *
 * @param [in]  pDeviceInit pointer to device initialization structure
 *
 * @note        This function may required to change with different hardware platforms.
 *
 */
ALWAYS_INLINE
void set_init_params(ADI_ETHER_HANDLE *phDevice, ADI_ETHER_DEV_INIT* const pDeviceInit)
{
    ADI_EMAC_DEVICE* const  pDev = (ADI_EMAC_DEVICE*)phDevice;

    /* populate with default parameters */
    pDev->nPhyConfig    = ADI_GEMAC_PHY_CFG_AUTO_NEGOTIATE_EN;
    pDev->Cache         = pDeviceInit->Cache;
    pDev->TxIntPeriod   = 1;

    /* Check the commands passed to the driver */
    if (pDeviceInit->pCmdArgArray)
    {
    	int index = 0;
    	while (pDeviceInit->pCmdArgArray[index].cmd != ADI_ETHER_CMD_END_OF_ARRAY) {
    		switch (pDeviceInit->pCmdArgArray[index].cmd)
    		{
    		case (uint32_t)ADI_ETHER_CMD_SET_PHY_SPEED:
    			{
    				uint32_t arg = (uint32_t)pDeviceInit->pCmdArgArray[index].arg;

    				switch (arg)
    				{
    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_AUTO_NEGOTIATE:
    					pDev->nPhyConfig = ADI_GEMAC_PHY_CFG_AUTO_NEGOTIATE_EN;
    					break;

    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_10T_HALF_DUPLEX:
    					pDev->nPhyConfig = ADI_GEMAC_PHY_CFG_10Mbps_EN;
    					break;

    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_10T_FULL_DUPLEX:
    					pDev->nPhyConfig =   ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN
    					                   | ADI_GEMAC_PHY_CFG_10Mbps_EN;
    					break;

    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_100T_HALF_DUPLEX:
    					pDev->nPhyConfig = ADI_GEMAC_PHY_CFG_100Mbps_EN;
    					break;

    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_100T_FULL_DUPLEX:
    					pDev->nPhyConfig =   ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN
    					                   | ADI_GEMAC_PHY_CFG_100Mbps_EN;
    					break;

    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_1000T_HALF_DUPLEX:
    					pDev->nPhyConfig = ADI_GEMAC_PHY_CFG_1000Mbps_EN;
    					break;

    				case (uint32_t)ADI_ETHER_ARG_PHY_SPEED_1000T_FULL_DUPLEX:
    					pDev->nPhyConfig =   ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN
    					                   | ADI_GEMAC_PHY_CFG_1000Mbps_EN;
    					break;

    				default:
    					break;
    				}
    			}
    			break;
    		case (uint32_t)ADI_GEMAC_CMD_TX_INTERRUPT_MAX_PERIOD:
    			{
    				uint32_t arg = (uint32_t)pDeviceInit->pCmdArgArray[index].arg;
					pDev->TxIntPeriod = arg;
    			}
    			break;

    		default:
    			break;
    		}

    		/* Increment the index */
    		index++;
    	}
    }
}

/**
 * @brief       Initializes EMAC
 *
 * @details     This function configures the EMAC Configuration register, filter register
 *              and flow control register using the default configuration supplied by the
 *              user.
 *
 * @param [in]  phDevice   Device Handle.
 *
 * @return      Status
 *                         - ADI_ETHER_RESULT_SUCCESS  successfully opened the device
 *
 * @note        Internal Driver function.
 */
static uint32_t gemac_init(ADI_ETHER_HANDLE * const phDevice)
{
    uint32_t reg_data;
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;

    reg_data = pEmacRegs->EMAC_MACCFG;

    /* initialize mac configuration register */
    if (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_LOOPBACK_EN) {
        reg_data = BITM_EMAC_MACCFG_LM;
    }

    if (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN) {
        reg_data |= BITM_EMAC_MACCFG_DM;
    }

	/* Set the configuration required for 10/100/1000Mbps speed */
	if (pDev->nPhyConfig &  ADI_GEMAC_PHY_CFG_1000Mbps_EN) {
		reg_data &= (~0x00008000);
	} else {
		reg_data |= 0x00008000;
		if (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_100Mbps_EN) {
			reg_data |= BITM_EMAC_MACCFG_FES;
		} else {
			reg_data &= ~BITM_EMAC_MACCFG_FES;
		}
	}

//    reg_data  |= (BITM_EMAC_MACCFG_PS | BITM_EMAC_MACCFG_TE |
    reg_data  |=  (BITM_EMAC_MACCFG_TE | BITM_EMAC_MACCFG_RE | BITM_EMAC_MACCFG_CST);

    gemac_set_maccfg(phDevice,reg_data);

    /* initialize frame filter configuration */

    pEmacRegs->EMAC_MACFRMFILT = BITM_EMAC_MACFRMFILT_RA  | 0x1 |
                                 BITM_EMAC_MACFRMFILT_PM  |
                                 BITM_EMAC_MACFRMFILT_HMC |
                                 BITM_EMAC_MACFRMFILT_HUC;

    /* setup flow control options */
    if(pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN) {
        pEmacRegs->EMAC_FLOWCTL = BITM_EMAC_FLOWCTL_TFE |
                                  BITM_EMAC_FLOWCTL_RFE;
    }

#ifdef GEMAC_SUPPORT_RGMII
    /* Set the Interrupt Mask register to mask RGMII Interrupt */
    if (pDev->Capability & ADI_EMAC_CAPABILITY_RGMII)
    {
    	gemac_set_macimask(phDevice, BITM_EMAC_ISTAT_RGMIIIS);
    }
#endif /* GEMAC_SUPPORT_RGMII */

    return (ADI_ETHER_RESULT_SUCCESS);
}
/**
 * @brief       Opens the given ethernet device
 *
 * @details     Every ethernet device driver exports its functionality using an entrypoint.
 *              Entrypoint consists of set of driver functions like, open, read, write, close etc.
 *
 *              adi_ether_GemacOpen function is responsible for initializing the ethernet device.
 *              This routine sets up EMAC and platform specific configuration.adi_ether_GemacOpen
 *              is called by routing layer function adi_ether_Open() using the supplied entrypoint.
 *              Applications open the ethernet drivers and supply the driver handle to the stack.
 *
 *              pDeviceInit parameter supplies memory for the driver. It uses the memory
 *              to construct receive and transmit dma descriptor chains. These descriptors
 *              will be used in conjuction with transmit and receive buffers to schedule DMA
 *              transfers.
 *
 *              It is adviced to increase the number of descriptors for applications having
 *              large data transfers.
 *
 *              This routine merely configures the MAC with default settings. Applications
 *              can change any defaults before actually enabling the MAC.
 *
 * @param [in]  pEntryPoint     Device driver entry point. For GEMAC driver the entry point
 *                              can be GEMAC0DriverEntry or GEMAC1DriverEntry.
 *
 *
 * @param[in]    pDeviceInit     Pointer to Initialization Data Structure. The init
 *                               structure is used to supply memory to the driver as
 *                               well operation of the cache. The supplied memory is
 *                               used for storing the transmit and receive descriptors.
 *                               Cache element is used to specifify whether data cache
 *                               is enable or disabled. If data cache is enabled driver
 *                               performs additional functions for the cache-coherency.
 *
 * @param[in]    pfCallback      Pointer to Ethernet callback Function. This callback
 *                               is used by the driver asynchronously to return the
 *                               received packets as well as transmitted packets. With
 *                               lwip tcp/ip stack this callback has to be adi_lwip_StackcallBack.
 *                               adi_lwip_StackcallBack is defined in lwip. Applications
 *                               that directly use the driver can hookup their own callback
 *                               routines.
 *
 * @param [out] phDevice        Pointer to a location where the handle to the
 *                              opened device is written.
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  successfully opened the device
 *                              - ADI_ETHER_RESULT_INVALID_PARAM invalid input parameter
 *                              - ADI_ETHER_RESULT_INVALID_DEVICE_ENTRY invalid entry point
 *
 * @sa          adi_Ether_Close()
 * @sa          adi_Ether_EnableMAC()
 */
ADI_ETHER_RESULT adi_ether_GemacOpen(
									 ADI_ETHER_DRIVER_ENTRY* const pEntryPoint,
									 ADI_ETHER_DEV_INIT*     const pDeviceInit,
									 ADI_ETHER_CALLBACK_FN   const pfCallback,
									 ADI_ETHER_HANDLE*       const phDevice,
                                     void*                   const pUsrPtr
                                     )
{
    ADI_EMAC_DEVICE   *pDev;
    ADI_ETHER_RESULT  Result =  ADI_ETHER_RESULT_FAILED;
    int nDMADeviceNum;

    gEMAC0.pEMAC_REGS = ((ADI_EMAC_REGISTERS *)ADI_EMAC0_BASE_ADDRESS);
#ifdef GEMAC_SUPPORT_EMAC1
    gEMAC1.pEMAC_REGS = ((ADI_EMAC_REGISTERS *)ADI_EMAC1_BASE_ADDRESS);
#endif

#if defined(ADI_DEBUG)
    if ((pfCallback == NULL) || (phDevice == NULL)  || (pDeviceInit == NULL))
    {
        return (ADI_ETHER_RESULT_INVALID_PARAM);
    }

    if ((pEntryPoint != &GEMAC0DriverEntry)
#ifdef GEMAC_SUPPORT_EMAC1
         && (pEntryPoint != &GEMAC1DriverEntry)
#endif
       )
    {
        return ADI_ETHER_RESULT_INVALID_DEVICE_ENTRY;
    }

    if((pDeviceInit->pEtherMemory->RecvMemLen < ADI_EMAC_MIN_DMEM_SZ)  ||
       (pDeviceInit->pEtherMemory->TransmitMemLen < ADI_EMAC_MIN_DMEM_SZ))
    {
        return ADI_ETHER_RESULT_INVALID_PARAM;
    }
#endif
    /* set GEMAC handle */
    set_gemac_handle(pEntryPoint,phDevice);

    pDev = (ADI_EMAC_DEVICE *)*phDevice;

    ENTER_CRITICAL_REGION();

    /* check if the device is already opened */
    if (!pDev->Opened)
    {
        pDev->Opened = true;

        pDev->pEtherCallback = pfCallback;
        pDev->pUsrPtr = pUsrPtr;
        pDev->MdcClk  =  GEMAC_MDC_CLK;

        /* stop transmit and receive (DMA 0,1,2) and mask interrupts */
        for (nDMADeviceNum = 0; nDMADeviceNum < pDev->nNumDmaChannels; nDMADeviceNum++) {
        	gemac_stop_rx(pDev,   nDMADeviceNum);
        	gemac_stop_tx(pDev,   nDMADeviceNum);
        	mask_gemac_ints(pDev, nDMADeviceNum);
        }

        /* reset the dma descriptor lists */
        reset_all_queues(*phDevice);
        reset_dma_lists(&pDev->Rx);
        reset_dma_lists(&pDev->Tx);

        /* initialize device from supplied parameters */
        set_init_params(*phDevice,pDeviceInit);

        /* GEMAC version */
        pDev->Version = gemac_version(*phDevice);

        /* setup rx descriptor list */
        Result = init_descriptor_memory(
        								pDev,
        								(uint8_t*)pDeviceInit->pEtherMemory->pRecvMem,
                             	 	 	pDeviceInit->pEtherMemory->RecvMemLen,
                             	 	 	&pDev->Rx
                             	 	 	);

        /* setup tx descriptor list */
        if (Result == ADI_ETHER_RESULT_SUCCESS)
        {
            Result = init_descriptor_memory (
            								 pDev,
            								 (uint8_t*)pDeviceInit->pEtherMemory->pTransmitMem,
            								 pDeviceInit->pEtherMemory->TransmitMemLen,
            								 &pDev->Tx
            								 );
        }

        /* Register Ehernet Interrupt Handler and enable it */
        if(adi_int_InstallHandler(
                                  pDev->Interrupt,
                                  EMACInterruptHandler,
                                  (void *)pDev,
                                  true
                                  ) != ADI_INT_SUCCESS)
         {
        	pDev->Opened = false;
             return (ADI_ETHER_RESULT_FAILED);
         }
    }
    else
    {
        *phDevice = NULL;
    }

    EXIT_CRITICAL_REGION();

    return(Result);
}

/**
 * @brief      Wait until gemac reset is complete
 *
 * @details    Waits for the Soft reset bit in DMA busmode register to get cleared.
 *             Upon successful reset this bit gets cleared automatically. If this bit is
 *             not cleared with in MAC_LOOPCOUNT number of times then the function result
 *             false or else it returns true.
 *
 * @param [in] hDevice   Device Handle.
 *
 * @note       This function may required to change with different hardware platforms.
 *
 */
static bool gemac_reset_complete(ADI_ETHER_HANDLE * const phDevice)
{
    int32_t loopcount = MAC_LOOPCOUNT;
    ADI_EMAC_REGISTERS*    const  pEmacRegs      = ((ADI_EMAC_DEVICE*)phDevice)->pEMAC_REGS;

    /* wait for the bus mode register to reset */
    do
    {
       if(!(pEmacRegs->EMAC_DMA_BUSMODE & BITM_EMAC_DMA_BUSMODE_SWR))
               return (true);
    } while (--loopcount > 0);

    return(false);
}

/**
 * @brief       Supply buffers to the ethernet driver to receive packets
 *
 * @details     adi_ether_GemacRead is responsible for reading the ethernet frames from the
 *              network. Buffers supplied via this API are queued and used one for each
 *              received frame. Only frames without errors will be received.
 *
 *              This routine can take single or linked list of buffers, it is adviced to
 *              supply list of buffers for optimal operation.
 *
 *              The supplied buffers are associated with a dma descriptor and scheduled for
 *              dma transfer. As number of buffers can vary independent of number of descriptors
 *              if descriptors are not available it will keep the buffers in queued list and
 *              schedules for DMA only when descriptors are available.Depending on the application
 *              requirements configure the number of descriptors.
 *
 *              If device is already started the routine will activate the channel so that
 *              the receive dma channel continue to operate. Once a buffer or buffer list is
 *              supplied to the driver upper layers should not attempt to access these buffers.
 *              Once buffer processing is completed driver will return the buffer via the callback
 *              handler supplied in the adi_ether_GemacOpen function.
 *
 *              All read and write operations are DMA driven. Programmed I/O is not supported.
 *
 *
 * @param [in]  phDevice        Handle to the ethernet device
 *
 * @param [in]  pBuffer         Pointer to a single buffer or list of buffers
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  successfully supplied the buffers
 *
 * @sa          adi_Ether_Write()
 */
ADI_ETHER_RESULT adi_ether_GemacRead( ADI_ETHER_HANDLE const phDevice,
                                             ADI_ETHER_BUFFER *pBuffer
                                           )
{
	ADI_ETHER_RESULT eResult = ADI_ETHER_RESULT_SUCCESS;
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_ETHER_BUFFER* pIntBuffer;

#ifdef ADI_DEBUG
#ifdef ADI_ETHER_SUPPORT_AV
    if ((pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_AV_EN) && (pDev->Capability & ADI_EMAC_CAPABILITY_AV))
    {
		pIntBuffer = pBuffer;
		while (pIntBuffer)
		{
			if (
					(pIntBuffer->nChannel >= GEMAC_SUPPORT_NUM_DMA_DEVICES)
				||  ((pIntBuffer->nChannel == 1) && (!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_RX_EN)))
				||  ((pIntBuffer->nChannel == 2) && (!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_RX_EN)))
				)
			{
				return ADI_ETHER_RESULT_INVALID_PARAM;
			}

			/* Get the next buffer in the list */
			pIntBuffer = pIntBuffer->pNext;
		}
    }
#endif
#endif

#ifdef ADI_ETHER_SUPPORT_AV
    if ((pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_AV_EN) && (pDev->Capability & ADI_EMAC_CAPABILITY_AV))
    {
		while (pBuffer) {

			/* Extract the first buffer */
			pIntBuffer = pBuffer;
			pBuffer = pBuffer->pNext;
			pIntBuffer->pNext = NULL;

			/* Insert the buffer into the queued list */
			eResult = insert_queue(&pDev->Rx.DMAChan[pIntBuffer->nChannel].Queued, pIntBuffer);

			if (eResult != ADI_ETHER_RESULT_SUCCESS) { return eResult; }
		}
    }
    else
#endif
    {

		/* Insert the buffer into the queued list */
		eResult = insert_queue(&pDev->Rx.DMAChan[0].Queued, pBuffer);

    }

    if(pDev->Started)
	{
    	int nDMADeviceNum;
    	for (nDMADeviceNum = (pDev->nNumDmaChannels - 1); nDMADeviceNum >= 0; nDMADeviceNum--) {
			/* Bind buffers with descriptors and if successful activate for all DMA Channel */
			if (eResult == ADI_ETHER_RESULT_SUCCESS) {
				eResult = bind_desc_and_activate(phDevice, &pDev->Rx, nDMADeviceNum);
			}
    	}
	}


    return eResult;
}

/**
 * @brief       Transmits packet over the network
 *
 * @details     adi_ether_GemacWrite is responsible for transmitting ethernet frames over
 *              network. Single or list of frames can be supplied by the upper layers.
 *
 *              The write operation immediately return by queuing up the transfer, this does
 *              not indicate successful transfer of the supplied frame. Actual tranfer of
 *              frame is indicated by returning the buffer via callback.
 *
 *              This routine will bind the incoming buffer with the avaiable transmit channel
 *              descriptor and schedules for DMA transfer. If descriptors are not avaiable
 *              for the transmit channel the buffer gets queued for transmission. As soon as
 *              more descriptors are available buffers will be associated with descriptors and
 *              scheduled for DMA transfer.
 *
 *              Once buffer is supplied to the driver applications should not access the
 *              buffer or buffer contents. With data cache enabled systems any such action
 *              will lead to cache coherency problems. For applications performing large transfers
 *              it is adviced to increase the number of DMA descriptors.
 *
 *              All read and write operations are DMA driven. Programmed I/O is not available.
 *
 * @param [in]  phDevice        Handle to the ethernet device
 *
 * @param [in]  pBuffer         Pointer to a single buffer or list of buffers
 *
 * @return      Status
 *                              -ADI_ETHER_RESULT_SUCCESS  successfully transmitted the packet
 *
 * @sa          adi_Ether_Read()
 */
ADI_ETHER_RESULT adi_ether_GemacWrite( ADI_ETHER_HANDLE const phDevice,
                                       ADI_ETHER_BUFFER *pBuffer )
{
	ADI_ETHER_RESULT  eResult = ADI_ETHER_RESULT_SUCCESS;
    ADI_EMAC_DEVICE*  const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_ETHER_BUFFER* pIntBuffer;

#ifdef ADI_DEBUG
#ifdef ADI_ETHER_SUPPORT_AV
    if ((pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_AV_EN) && (pDev->Capability & ADI_EMAC_CAPABILITY_AV))
    {
		pIntBuffer = pBuffer;
		while (pIntBuffer)
		{
			if (
					(pIntBuffer->nChannel >= GEMAC_SUPPORT_NUM_DMA_DEVICES)
				||  ((pIntBuffer->nChannel == 1) && (!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_TX_EN)))
				||  ((pIntBuffer->nChannel == 2) && (!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_TX_EN)))
				||  ((pIntBuffer->nChannel == 1) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_SLOT_EN) && (pIntBuffer->nSlot >= 16u))
				||  ((pIntBuffer->nChannel == 2) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_SLOT_EN) && (pIntBuffer->nSlot >= 16u))
				)
			{
				return ADI_ETHER_RESULT_INVALID_PARAM;
			}

			/* Get the next buffer in the list */
			pIntBuffer = pIntBuffer->pNext;
		}
    }
#endif
#endif

#ifdef ADI_ETHER_SUPPORT_AV
    if ((pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_AV_EN) && (pDev->Capability & ADI_EMAC_CAPABILITY_AV))
    {
		while (pBuffer) {

			/* Extract the first buffer */
			pIntBuffer = pBuffer;
			pBuffer = pBuffer->pNext;
			pIntBuffer->pNext = NULL;

			/* Insert the buffer into the queued list */
			eResult = insert_queue(&pDev->Tx.DMAChan[pIntBuffer->nChannel].Queued, pIntBuffer);

			if (eResult != ADI_ETHER_RESULT_SUCCESS) { return eResult; }
		}
    }
    else
#endif
    {
		/* Insert the buffer into the queued list */
		eResult = insert_queue(&pDev->Tx.DMAChan[0].Queued, pBuffer);

    }

    if(pDev->Started)
	{
    	int nDMADeviceNum;
    	for (nDMADeviceNum = (pDev->nNumDmaChannels - 1); nDMADeviceNum >= 0; nDMADeviceNum--) {
			/* Bind buffers with descriptors and if successful activate for all DMA Channel */
			if (eResult == ADI_ETHER_RESULT_SUCCESS) {
				eResult = bind_desc_and_activate(phDevice, &pDev->Tx, nDMADeviceNum);
			}
    	}

	}

    return(eResult);
}


/**
 * @brief       Returns the link status
 *
 * @details     Returns the link status, if link is up it returns true else false. This
 *              API can be used by the applications to check whether network link is present
 *              or not.
 *
 * @param [in]  phDevice        Handle to the ethernet device
 *
 * @return      Boolean
 *                              - True if the link is up, False if the link is down
 */
bool adi_ether_GemacGetLinkStatus(ADI_ETHER_HANDLE phDevice)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    uint32_t nStatus;

    ENTER_CRITICAL_REGION();
    pDev->pPhyDevice->getStatus(pDev->pPhyDevice, &nStatus);
    EXIT_CRITICAL_REGION();

    return (nStatus & ADI_ETHER_PHY_LINK_UP) ? true : false;
}

/**
 * @brief       Enables the Ethernet.
 *
 * @details     adi_ether_GemacEnableMAC enables the MAC unit. One MAC is enabled driver
 *              configuration can not be changed. Once enable is issued reception and
 *              transmission of frames starts with active ethernet and PHY interrupts.
 *
 *              Any previously sumbitted transmit and receive buffers will also be processed.
 *
 * @param [in]  phDevice        Handle to the ethernet device
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  if successfully enables the EMAC
 *                              - ADI_ETHER_RESULT_FAILED [D]  in case of failure
 *                              - ADI_ETHER_RESULT_RESET_FAILED Unable to reset MAC
 */
ADI_ETHER_RESULT adi_ether_GemacEnableMAC(ADI_ETHER_HANDLE phDevice)
{
	ADI_ETHER_RESULT eResult;
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = ((ADI_EMAC_DEVICE*)phDevice)->pEMAC_REGS;
    uint32_t            Status;
    uint32_t            nDMA_IEN, nDMA_BUSMODE, nDMA_OPMODE;
    ADI_PHY_DEV_INIT    nPhyConfig;

    nPhyConfig.pfCallback = PHYCallbackHandler;

#if defined(ADI_DEBUG)
      /* If device is already enabled then we throw an error in debug build */
      if (pDev->Started)
      {
          return (ADI_ETHER_RESULT_FAILED);
      }
#endif

    /* Reset the PHY if GEMAC-1000Mbps */
    if (pDev->Capability & ADI_EMAC_CAPABILITY_RGMII)
    {
        /* Reset the phy */
    	nPhyConfig.bResetOnly = true;

        if (pDev->pPhyDevice->init(pDev->pPhyDevice, phDevice, &nPhyConfig) != ADI_PHY_RESULT_SUCCESS)
        {
        	return ADI_ETHER_RESULT_PHYINIT_FAILED;
        }
    }

    /* Initialize the DMA descriptors */
    if ((eResult = init_descriptor(phDevice, &pDev->Rx)) != ADI_ETHER_RESULT_SUCCESS)
    {
    	return eResult;
    }
    if ((eResult = init_descriptor(phDevice, &pDev->Tx)) != ADI_ETHER_RESULT_SUCCESS)
    {
    	return eResult;
    }


    /* reset emac bus mode register */
    gemac_set_dmabusmode(phDevice, 0, BITM_EMAC_DMA_BUSMODE_SWR);

    /* wait for rest to finsih */
    if (!gemac_reset_complete(phDevice))
    {
         //ETHER_DEBUG("rest failed \n",*pREG_EMAC0_DMA_BUSMODE);
         return (ADI_ETHER_RESULT_RESET_FAILED);
    }

    /* clear any pending interrupts - write to clear bits in status reg */
    Status = pEmacRegs->EMAC_DMA_STAT;
    pEmacRegs->EMAC_DMA_STAT = Status & 0x1FFFF;  /* only lower 16-bits has interrupts and w1c */

#ifdef ADI_ETHER_SUPPORT_AV
    if (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
    {
        if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN) {
            /* clear any pending interrupts - write to clear bits in status reg for DMA1 */
            Status = pEmacRegs->EMAC_DMA1_STAT;
            pEmacRegs->EMAC_DMA1_STAT = Status & 0x1FFFF;  /* only lower 16-bits has interrupts and w1c */
        }

        if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN) {
            /* clear any pending interrupts - write to clear bits in status reg for DMA2 */
            Status = pEmacRegs->EMAC_DMA2_STAT;
            pEmacRegs->EMAC_DMA2_STAT = Status & 0x1FFFF;  /* only lower 16-bits has interrupts and w1c */
        }
    }
#endif

    pEmacRegs->EMAC_MMC_RXIMSK = 0xffffffff;
    pEmacRegs->EMAC_MMC_TXIMSK = 0xffffffff;
    pEmacRegs->EMAC_IPC_RXIMSK = 0xffffffff;

#ifdef GEMAC_REG_CFG_BMMODE
    /* Set the BMMODE register */
    pEmacRegs->EMAC_DMA_BMMODE = GEMAC_REG_CFG_BMMODE;
#endif


    /* Set the DMA bus mode and DMA IEN */

    /* initialize dma interrupt mask register
     * normal mode enables the following interrupts
     * - Transmit interrupt
     * - Transmit buffer unavailable
     * - Receive interrupt
     * - Early receive interrupt
     */
    nDMA_IEN = (1UL << BITP_EMAC_DMA_IEN_NIS) |
    	 	   (1UL << BITP_EMAC_DMA_IEN_AIS) |
    	 	   (1UL << BITP_EMAC_DMA_IEN_RU)  |
    	 	   (1UL << BITP_EMAC_DMA_IEN_RI)  |
    	 	   (1UL << BITP_EMAC_DMA_IEN_UNF) |
    	 	   (1UL << BITP_EMAC_DMA_IEN_TU)  |
    	 	   (1UL << BITP_EMAC_DMA_IEN_TI);

#ifdef GEMAC_SUPPORT_EMAC1
    if((ADI_EMAC_DEVICE*)phDevice == &gEMAC1)
    	nDMA_BUSMODE = EMAC_REG_CFG_BUSMODE;
    else
    	nDMA_BUSMODE = GEMAC_REG_CFG_BUSMODE;
#else
	nDMA_BUSMODE = GEMAC_REG_CFG_BUSMODE;
#endif

     gemac_set_dmabusmode(phDevice, 0, nDMA_BUSMODE);
     pEmacRegs->EMAC_DMA_IEN = nDMA_IEN;

#ifdef ADI_ETHER_SUPPORT_AV
    if (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
    {
         if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_AV_EN) {

        	 pEmacRegs->EMAC_MAC_AVCTL = pDev->AVDevice.nAVMACCtlReg;

    		 if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN) {
    			 gemac_set_dmabusmode(phDevice, 1, nDMA_BUSMODE);
    			 pEmacRegs->EMAC_DMA1_IEN = nDMA_IEN;

    			 if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_CBS_EN) {
    				 pEmacRegs->EMAC_DMA1_CHCBSCTL = pDev->AVDevice.Chan1.nCBSCtlReg;
    				 pEmacRegs->EMAC_DMA1_CHISC    = pDev->AVDevice.Chan1.nIdleSlopeReg;
    				 pEmacRegs->EMAC_DMA1_CHSSC    = pDev->AVDevice.Chan1.nSendSlopeReg;
    				 pEmacRegs->EMAC_DMA1_CHHIC    = pDev->AVDevice.Chan1.nHiCreditReg;
    				 pEmacRegs->EMAC_DMA1_CHLOC    = pDev->AVDevice.Chan1.nLowCreditReg;
    			 }
    			 else
    			 {
    				 pEmacRegs->EMAC_DMA1_CHCBSCTL = BITM_EMAC_DMA1_CHCBSCTL_CBSD;
    			 }

    			 if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_SLOT_EN) {
    				 pEmacRegs->EMAC_DMA1_CHSFCS   = pDev->AVDevice.Chan1.nSlotCtlStatReg;
    			 }
    		 }

    		 if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN) {
    			 gemac_set_dmabusmode(phDevice, 2, nDMA_BUSMODE);
    			 pEmacRegs->EMAC_DMA2_IEN = nDMA_IEN;

    			 if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_CBS_EN) {
    				 pEmacRegs->EMAC_DMA2_CHCBSCTL = pDev->AVDevice.Chan2.nCBSCtlReg;
    				 pEmacRegs->EMAC_DMA2_CHISC    = pDev->AVDevice.Chan2.nIdleSlopeReg;
    				 pEmacRegs->EMAC_DMA2_CHSSC    = pDev->AVDevice.Chan2.nSendSlopeReg;
    				 pEmacRegs->EMAC_DMA2_CHHIC    = pDev->AVDevice.Chan2.nHiCreditReg;
    				 pEmacRegs->EMAC_DMA2_CHLOC    = pDev->AVDevice.Chan2.nLowCreditReg;
    			 }
    			 else
    			 {
    				 pEmacRegs->EMAC_DMA2_CHCBSCTL = BITM_EMAC_DMA2_CHCBSCTL_CBSD;
    			 }


    			 if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_SLOT_EN) {
    				 pEmacRegs->EMAC_DMA2_CHSFCS   = pDev->AVDevice.Chan2.nSlotCtlStatReg;
    			 }

    		 }
         }

         if (	(!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_AV_EN))
        	 || (   (!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_RX_EN))
        		 && (!(pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_RX_EN))
        		 ))
         {
        	 pEmacRegs->EMAC_MAC_AVCTL = BITM_EMAC_MAC_AVCTL_AVCD;
         }
     }
#endif

     /* setup descriptor lists */

     /* setup appropriate filtering options GEMAC 1,2,3 */
     /* enable GEMAC register 0 for trnamsit and receive modes */
     gemac_init(phDevice);


     /* Enable DMA for rx and tx if pakcets are present */
     nDMA_OPMODE = GEMAC_REG_CFG_OPMODE;

     gemac_set_dmaopmode(phDevice, 0, nDMA_OPMODE);
#ifdef ADI_ETHER_SUPPORT_AV
     if (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
     {
         if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN) {
        	 gemac_set_dmaopmode(phDevice, 1u, nDMA_OPMODE);
         }
         if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN) {
        	 gemac_set_dmaopmode(phDevice, 2u, nDMA_OPMODE);
         }
     }
#endif

      adi_ether_SetMACAddress(pDev,(const uint8_t *)pDev->MacAddress);

      pEmacRegs->EMAC_DMA_TXDSC_ADDR  = (uint32_t)pDev->Tx.DMAChan[0].pDmaDescHead;
      pEmacRegs->EMAC_DMA_RXDSC_ADDR  = (uint32_t)pDev->Rx.DMAChan[0].pDmaDescHead;

#ifdef ADI_ETHER_SUPPORT_AV
      if (pDev->Capability & ADI_EMAC_CAPABILITY_AV) {
          if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN) {
        	  pEmacRegs->EMAC_DMA1_TXDSC_ADDR = (uint32_t)pDev->Tx.DMAChan[1].pDmaDescHead;
        	  pEmacRegs->EMAC_DMA1_RXDSC_ADDR = (uint32_t)pDev->Rx.DMAChan[1].pDmaDescHead;
          }

          if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN) {
        	  pEmacRegs->EMAC_DMA2_TXDSC_ADDR = (uint32_t)pDev->Tx.DMAChan[2].pDmaDescHead;
        	  pEmacRegs->EMAC_DMA2_RXDSC_ADDR = (uint32_t)pDev->Rx.DMAChan[2].pDmaDescHead;
          }
      }
#endif

      /* Set the device as started */
      pDev->Started = true;

      /* setup the phy */
      nPhyConfig.bResetOnly = false;
      nPhyConfig.nConfig = 0u;
      nPhyConfig.nConfig |= (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_LOOPBACK_EN) ? ADI_PHY_CFG_LOOPBACK_EN : 0u;

      if (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_AUTO_NEGOTIATE_EN)
      {
          nPhyConfig.nConfig |= ADI_PHY_CFG_AUTO_NEGOTIATE_EN;
      }
      else
      {
          nPhyConfig.nConfig |= (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN)
        									? ADI_PHY_CFG_FULL_DUPLEX_EN : 0u;

          if (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_10Mbps_EN) {
        	  nPhyConfig.nConfig |= ADI_PHY_CFG_10Mbps_EN;
          } else if (pDev->nPhyConfig & ADI_GEMAC_PHY_CFG_100Mbps_EN) {
        	  nPhyConfig.nConfig |= ADI_PHY_CFG_100Mbps_EN;
          } else {
        	  nPhyConfig.nConfig |= ADI_PHY_CFG_1000Mbps_EN;
          }
      }

      if (pDev->pPhyDevice->init(pDev->pPhyDevice, phDevice, &nPhyConfig) != ADI_PHY_RESULT_SUCCESS)
      {
    	  return ADI_ETHER_RESULT_PHYINIT_FAILED;
      }

      ENTER_CRITICAL_REGION();
      /* activate rx channel (DMA 0) */
      bind_buf_with_desc(phDevice,&pDev->Rx, 0);
      activate_channel(pDev,&pDev->Rx, 0);
      enable_rx(pDev, 0);

      /* activate tx channel (DMA 0) */
      bind_buf_with_desc(phDevice,&pDev->Tx, 0);
      activate_channel(pDev,&pDev->Tx, 0);
      enable_tx(pDev, 0);

#ifdef ADI_ETHER_SUPPORT_AV
      if (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
      {
          /* IF (DMA1 is enabled) */
          if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN) {

        	  if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_RX_EN) {
        		  /* activate rx channel (DMA 1) */
        		  bind_buf_with_desc(phDevice,&pDev->Rx, 1);
        		  activate_channel(pDev,&pDev->Rx, 1);
        		  enable_rx(pDev, 1);
        	  }

        	  if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_TX_EN) {
        		  /* activate tx channel (DMA 1) */
        		  bind_buf_with_desc(phDevice,&pDev->Tx, 1);
        		  activate_channel(pDev,&pDev->Tx, 1);
        		  enable_tx(pDev, 1);
        	  }
          }

          /* IF (DMA2 is enabled) */
          if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN) {

        	  if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_RX_EN) {
        		  /* activate rx channel (DMA 2) */
        		  bind_buf_with_desc(phDevice,&pDev->Rx, 2);
        		  activate_channel(pDev,&pDev->Rx, 2);
        		  enable_rx(pDev, 2);
        	  }

        	  if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_TX_EN) {
    			  /* activate tx channel (DMA 1) */
    			  bind_buf_with_desc(phDevice,&pDev->Tx, 2);
    			  activate_channel(pDev,&pDev->Tx, 2);
    			  enable_tx(pDev, 2);
        	  }

          }
      }
#endif



      EXIT_CRITICAL_REGION();

     return(ADI_ETHER_RESULT_SUCCESS);
}


/**
 * @brief       Add's supplied multicast group address in the MAC
 *
 * @details     Enables the relevant multicast bits for the supplied multicast group address.
 *              Multicast based applications typically use IGMP protocol and enable multicast
 *              functionality using BSD socket API call ioctlsocket().
 *
 * @param [in]  phDevice            Handle to the ethernet device
 *
 * @param [in]  MultiCastGroupAddr  Multicast group address
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  if successfully enables the EMAC
 */
ADI_ETHER_RESULT adi_ether_GemacAddMulticastFilter(ADI_ETHER_HANDLE phDevice,
                                                   const uint32_t MultiCastGroupAddr)
{
    return add_multicastmac_filter(phDevice,MultiCastGroupAddr,true);
}

/**
 * @brief       Deletes the supplied multicast group address from the EMAC
 *
 * @details     Disables the relevant multicast bits for the supplied multicast group address.
 *              Multicast based applications typically use IGMP protocol and disable multicast
 *              functionality using BSD socket API ioctlsocket() which will in-turn call the
 *              driver routine.
 *
 * @param [in]  phDevice             Handle to the ethernet device
 *
 * @param [in]  MultiCastGroupAddr   Multicast group address
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  if successfully enables the EMAC
 */
ADI_ETHER_RESULT adi_ether_GemacDelMulticastFilter(ADI_ETHER_HANDLE phDevice,
                                                   const uint32_t MultiCastGroupAddr)
{
    return add_multicastmac_filter(phDevice,MultiCastGroupAddr,false);
}

/**
 * @brief       Returns the current MAC address
 *
 * @details     This API returns the MAC address present in the EMAC.
 *
 * @param [in]  phDevice           Handle to the ethernet device
 *
 * @param [out] pMacAddress        Pointer to the memory area where MAC address will be stored. It
 *                                 has to be atleast 6 bytes long.
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  if successfully enables the EMAC
 */
ADI_ETHER_RESULT adi_ether_GemacGetMACAddress(ADI_ETHER_HANDLE phDevice, uint8_t *pMacAddress)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    uint32_t  mac;
    int32_t   i;

    mac = pEmacRegs->EMAC_ADDR0_LO;

    for (i = 0; i <= 3 ; i++)
    {
        pMacAddress[i] = mac & 0xFF;
        mac >>= 8;
    }

    mac = pEmacRegs->EMAC_ADDR0_HI;

    for (i = 4; i <= 5; i++)
    {
        pMacAddress[i] = mac & 0xFF;
        mac >>= 8;
    }
    return(ADI_ETHER_RESULT_SUCCESS);
}

/**
 * @brief       Sets the given MAC address in the EMAC
 *
 * @details     adi_ether_GemacSetMACAddress sets the MAC address in the EMAC. It will not
 *              update the non-volatile block where ez-kits store the MAC address.
 *
 * @param [in]  phDevice           Handle to the ethernet device
 *
 * @param [in] pMacAddress         Pointer to the memory where MAC address is present
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  if successfully enables the EMAC
 */
ADI_ETHER_RESULT adi_ether_GemacSetMACAddress(ADI_ETHER_HANDLE phDevice, const uint8_t *pMacAddress)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    uint32_t  mac;
    int32_t   i;

    mac = 0;

    /* store the mac address */
    memcpy(pDev->MacAddress,pMacAddress,6);

    for(i = 3; i >= 0; i-- )
    {
        mac = (mac << 8) | pMacAddress[i];
    }
    pEmacRegs->EMAC_ADDR0_LO = mac;

    mac = 0;

    for(i = 5; i >= 4; i-- )
    {
        mac = (mac << 8) | pMacAddress[i];
    }
    pEmacRegs->EMAC_ADDR0_HI = mac;

    return(ADI_ETHER_RESULT_SUCCESS);
}

/**
 * @brief       Manage the Module IO Operations
 *
 * @details     The ethernet drivers can have additional modules like PTP, AV etc depending upon the
 * 			    capabilities of the underlying controller. adi_ether_GemacModuleIO is responsible for managing
 * 			    the IO with those modules. The function passes the arguments to the functions corresponding to
 * 			    managing the modules.
 *
 * @param [in]  phDevice        Handle to the ethernet device
 *
 * @param [in]  ModuleID        ID of the module
 *
 * @param [in]  Func            Module function to be performed
 *
 * @param [in]  arg0            Argument 0 corresponding to the function
 *
 * @param [in]  arg1            Argument 1 corresponding to the function
 *
 * @param [in]  arg2            Argument 2 corresponding to the function
 *
 * @return      Status
 *                              - ADI_ETHER_RESULT_SUCCESS  successfully supplied the buffers
 *
 * @sa          adi_Ether_Write()
 */

ADI_ETHER_RESULT adi_ether_GemacModuleIO (
										  ADI_ETHER_HANDLE      const phDevice,
										  ADI_ETHER_MODULE      const ModuleID,
										  ADI_ETHER_MODULE_FUNC const Func,
										  void*                       arg0,
										  void*                       arg1,
										  void*                       arg2
        								  )
{
	ADI_ETHER_RESULT eResult     = ADI_ETHER_RESULT_NOT_SUPPORTED;
    ADI_EMAC_DEVICE* const  pDev = (ADI_EMAC_DEVICE*)phDevice;

    switch (ModuleID)
    {
    case ADI_ETHER_MODULE_PTP:
#ifdef ADI_ETHER_SUPPORT_PTP
    	eResult = gemac_PTPModuleIO(pDev, Func, arg0, arg1, arg2);
#endif
    	break;

    case ADI_ETHER_MODULE_PPS:
#ifdef ADI_ETHER_SUPPORT_PPS
    	eResult = gemac_PPS_AlarmModuleIO(pDev, Func, arg0, arg1, arg2);
#endif
    	break;

    case ADI_ETHER_MODULE_ALARM:
#ifdef ADI_ETHER_SUPPORT_ALARM
    	eResult = gemac_PPS_AlarmModuleIO(pDev, Func, arg0, arg1, arg2);
#endif
    	break;

    case ADI_ETHER_MODULE_AV:
#ifdef ADI_ETHER_SUPPORT_AV
    	eResult = gemac_AV_ModuleIO(pDev, Func, arg0, arg1, arg2);
#endif
    	break;

    default:
    	eResult = ADI_ETHER_RESULT_NOT_SUPPORTED;
        break;
    }

    return eResult;
}



static ADI_ETHER_RESULT init_descriptor_memory (
												ADI_ETHER_HANDLE phDevice,
												const uint8_t *pMemory,
												const uint32_t Length,
												ADI_EMAC_CHANNEL *pChannel
												)
{
    uint8_t *pCurBase,*pOriginalBase;
    int32_t ActualLength;

    pOriginalBase =  (uint8_t*)pMemory;

    /* align the start address to 32-byte */
    pCurBase = (uint8_t*)((((uint32_t)pMemory) + 31) & (~0x1F));

    /* Adjust length according to the alignment */
    ActualLength = Length - (pCurBase - pOriginalBase);

    /* Set the DMA descriptor memory */
    pChannel->pDMADescMem = (void*)pCurBase;

    /* Compute the number of descriptors */
    pChannel->nDMADescNum = ActualLength / sizeof(ADI_EMAC_DMADESC);

    return (ADI_ETHER_RESULT_SUCCESS);
}

static ADI_ETHER_RESULT init_descriptor (
										 ADI_ETHER_HANDLE phDevice,
										 ADI_EMAC_CHANNEL *pChannel
										 )
{
	ADI_EMAC_DEVICE* const  pDev = (ADI_EMAC_DEVICE*)phDevice;
	ADI_EMAC_DMADESC *pDMADescPool;
	int32_t i, nTotalNumDMADesc;


	/* Check the assumptions made in the code */
#ifdef ADI_DEBUG
#ifdef _MISRA_RULES
#pragma diag(push)
/* Rule 13.7(Req) : Boolean operations whose results are invariant shall not be permitted. */
#pragma diag (suppress: misra_rule_13_7: "Assumptions Validation")
#endif
    assert (sizeof(ADI_EMAC_DMADESC) == 32u);
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

#endif

	/* Set the start of the DMA Desc Pool */
    pDMADescPool = (ADI_EMAC_DMADESC*) pChannel->pDMADescMem;
    nTotalNumDMADesc = pChannel->nDMADescNum;

    /* Clear the descriptor memory to zero */
    memset(pDMADescPool, 0, sizeof(ADI_EMAC_DMADESC) * pChannel->nDMADescNum);

    /* Link all the DMA descriptors */
	for (i = 0; i < (pChannel->nDMADescNum - 1); i++)
	{
		pDMADescPool[i].pNextDesc = &pDMADescPool[i+1];
	}
	pDMADescPool[pChannel->nDMADescNum - 1].pNextDesc = NULL;

#ifdef ADI_ETHER_SUPPORT_AV
    if (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
    {
        /* IF (DMA2 is enabled) */
        if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_EN)
        {
        	uint32_t nDMAChannel = 2;
        	uint32_t nNumDesc = (pChannel->Recv)
        							?  pDev->AVDevice.Chan2.Rx.NumReservedDesc
        							 : pDev->AVDevice.Chan2.Tx.NumReservedDesc;

        	if (nNumDesc > 0) {
    			pChannel->DMAChan[nDMAChannel].pDmaDescHead = pDMADescPool;
    			pChannel->DMAChan[nDMAChannel].pDmaDescTail = &pDMADescPool[nNumDesc - 1];
    			pChannel->DMAChan[nDMAChannel].pDmaDescTail->pNextDesc = NULL;
    			pChannel->DMAChan[nDMAChannel].NumAvailDmaDesc = nNumDesc;

    			pDMADescPool = &pDMADescPool[nNumDesc];
    			nTotalNumDMADesc = nTotalNumDMADesc - nNumDesc;
        	}
        }

        /* IF (DMA1 is enabled) */
        if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN)
        {
        	uint32_t nDMAChannel = 1;
        	uint32_t nNumDesc = (pChannel->Recv)
        	    							?  pDev->AVDevice.Chan1.Rx.NumReservedDesc
        	    							 : pDev->AVDevice.Chan1.Tx.NumReservedDesc;

        	if (nNumDesc > 0) {
    			pChannel->DMAChan[nDMAChannel].pDmaDescHead = pDMADescPool;
    			pChannel->DMAChan[nDMAChannel].pDmaDescTail = &pDMADescPool[nNumDesc - 1];
    			pChannel->DMAChan[nDMAChannel].pDmaDescTail->pNextDesc = NULL;
    			pChannel->DMAChan[nDMAChannel].NumAvailDmaDesc = nNumDesc;

    			pDMADescPool = &pDMADescPool[nNumDesc];
    			nTotalNumDMADesc = nTotalNumDMADesc - nNumDesc;
        	}
        }
    }
#endif

    /* Set the rest of descriptors for DMA0 */
    {
    	uint32_t nNumDesc = nTotalNumDMADesc;
    	uint32_t nDMAChannel = 0;

    	pChannel->DMAChan[nDMAChannel].pDmaDescHead = pDMADescPool;
    	pChannel->DMAChan[nDMAChannel].pDmaDescTail = &pDMADescPool[nNumDesc - 1];
    	pChannel->DMAChan[nDMAChannel].pDmaDescTail->pNextDesc = NULL;
    	pChannel->DMAChan[nDMAChannel].NumAvailDmaDesc = nNumDesc;
    }

    return (ADI_ETHER_RESULT_SUCCESS);
}

/*
 * Inserts the buffer in the given queue
 */
static ADI_ETHER_RESULT insert_queue(ADI_EMAC_FRAME_Q *pQueue, ADI_ETHER_BUFFER *pBuffer)
{
    int32_t NumInputBuffers=0;
    ADI_ETHER_BUFFER *pTempBuffer = pBuffer,*pLastBuffer = NULL;

#ifdef ADI_DEBUG
       if (pBuffer == NULL) return ADI_ETHER_RESULT_NULL_BUFFER;
#endif

    /* typically the number of incoming buffers are small */
    do
    {
        NumInputBuffers++;
        pLastBuffer = pTempBuffer;
        pTempBuffer = pTempBuffer->pNext;

    } while (pTempBuffer != NULL);

    ENTER_CRITICAL_REGION();
    /* Now insert and update the queue */
    if ( (pQueue->pQueueHead == NULL) && (pQueue->pQueueTail == NULL) )
        pQueue->pQueueHead = pBuffer;
    else
        pQueue->pQueueTail->pNext = pBuffer;

    pQueue->pQueueTail    = pLastBuffer;
    pQueue->ElementCount += NumInputBuffers;

    EXIT_CRITICAL_REGION();

    return (ADI_ETHER_RESULT_SUCCESS);
}

/**
 * @brief       Sets descriptor values
 *
 * @details     Depending on the channel the decriptors were configured with default values.
 *
 * @param [in]  hDevice       Handle to the device
 * @param [in]  pDmaDesc      Pointer to the dma descriptor
 * @param [in]  pBindedBuf    Pointer to the buffer to be associated with the descriptor
 * @param [in]  pChannel      Pointer to the channel
 *
 * @return      void
 *
 * @note        used by bind_buf_with_desc()
 */
static void  set_descriptor(
							ADI_ETHER_HANDLE hDevice,
                     	 	ADI_EMAC_DMADESC *pDmaDesc,
                     	 	ADI_ETHER_BUFFER *pBindedBuf,
                     	 	ADI_EMAC_CHANNEL *pChannel,
                     	 	uint32_t          nDMADeviceNum
                     	 	)
{
    ADI_EMAC_DEVICE*    const  pDev = (ADI_EMAC_DEVICE*)hDevice;

    /* set the descriptor parameters */
    pDmaDesc->ControlDesc = pBindedBuf->ElementCount;
    pDmaDesc->Status      = 0;

    if(pChannel->Recv)
    {
        pDmaDesc->StartAddr   = (uint32_t)((uint8_t*)pBindedBuf->Data+2);
        pDmaDesc->ControlDesc |= (1UL << 14);

        /* data cache is enabled flush and invalidate the cache for entire data area */
        if(pDev->Cache)
        {
            flushinv_area((uint8_t*)pBindedBuf->Data,
                          (uint32_t)(pBindedBuf->ElementWidth *pBindedBuf->ElementCount));
        }
    }
    else
    {
        pDmaDesc->StartAddr   = (uint32_t)((uint8_t*)pBindedBuf->Data+2);
        pDmaDesc->ControlDesc -= 2;
        pDmaDesc->Status |= ( (1UL << 30)     /* Interrupt on Completion */
        					 | (1UL << 29)    /* Last Segment */
        					 | (1UL << 28)    /* First Segment */
        					 | (1UL << 20)    /* Second Address Chained */
        					 );

#ifdef ADI_ETHER_SUPPORT_PTP
        if (pDev->Capability & ADI_EMAC_CAPABILITY_PTP) {
            /* Timestamp is enabled for the TX packet */
            if (pBindedBuf->Flag & ADI_ETHER_BUFFER_FLAG_TX_TIMESTAMP_EN) {
        	       pDmaDesc->Status |= (1UL << 25);  /* Transmit Timestamp Enable */
               }
        }
#endif

#ifdef ADI_ETHER_SUPPORT_AV
        if (pDev->Capability & ADI_EMAC_CAPABILITY_PTP) {
            if (
            		((nDMADeviceNum == 1) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_SLOT_EN))
            	||  ((nDMADeviceNum == 2) && (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA2_SLOT_EN))
            	)
            {
            	pDmaDesc->Status |= ((pBindedBuf->nSlot & 0x000Fu) << 3);
            }
        }
#endif

        /* data cache is enabled flush the cache for entire data area */
        if(pDev->Cache)
        {
            flush_area((uint8_t*)pBindedBuf->Data,
                       (uint32_t)(pBindedBuf->ElementWidth *pBindedBuf->ElementCount));
        }
    }
}

/**
 * @brief       Bind buffers with descriptors.
 *
 * @details     This function fetches an available dma descriptor from the respective
 *              channel and associates with a buffer in Queued list then moves the buffer to
 *              the Pending queue. The buffers in Pending queue are ready to be go to the
 *              active dma chain. Number of dma descriptors are independent of the number
 *              of buffers in the system. So if there is no available dma descriptor to
 *              associate a buffer then buffer is left in Queued list.
 *
 *              Depending on the channel the decriptors were configured with default values.
 *
 * @param [in]  hDevice       Handle to the ethernet device
 * @param [in]  pChannel      Pointer to the channel
 *
 * @return      Status
 *                            - ADI_ETHER_RESULT_SUCCESS  if successfully enables the EMAC
 */
static ADI_ETHER_RESULT bind_buf_with_desc(ADI_ETHER_HANDLE hDevice,ADI_EMAC_CHANNEL *pChannel, int32_t nDMADeviceNum)
{
	ADI_EMAC_DMADESC *pAvailDesc;
    ADI_ETHER_BUFFER *pQueuedBuf;
    ADI_EMAC_DMA_CHANNEL* pDMAChannel = &pChannel->DMAChan[nDMADeviceNum];

    /* Mask off etherent interrupt alone */
    ENTER_CRITICAL_REGION();

    pAvailDesc = pDMAChannel->pDmaDescHead;

    pQueuedBuf = pDMAChannel->Queued.pQueueHead;

#ifdef ADI_DEBUG
    if ( (pAvailDesc == NULL) || (pQueuedBuf == NULL) )
    {
         EXIT_CRITICAL_REGION();
         return ADI_ETHER_RESULT_NULL_BUFFER;
    }
#endif /* ADI_DEBUG */


    /* We will leave the last descriptor without associating with a buffer. This is
     * because the DMA will fetch the descriptor and enter into suspend state. We do
     * not want to stop and restart DMA but rather use the last descriptor to continue
     * from the suspended state.
     */
    while (pAvailDesc && (pDMAChannel->NumAvailDmaDesc > 1) && pQueuedBuf)
    {
        /* remove the queued buffer from the queue */
        pDMAChannel->Queued.pQueueHead = pDMAChannel->Queued.pQueueHead->pNext;
        pDMAChannel->Queued.ElementCount--;
        pQueuedBuf->pNext = NULL;

        /* remove the descriptor from the avail list */
        pDMAChannel->pDmaDescHead = pDMAChannel->pDmaDescHead->pNextDesc;
        pDMAChannel->NumAvailDmaDesc--;

        /* now bind the descriptor with the buffer */
        ((ADI_EMAC_BUFINFO*)pQueuedBuf)->pDmaDesc = pAvailDesc;

        set_descriptor(hDevice,pAvailDesc,pQueuedBuf,pChannel, nDMADeviceNum);

        /* now put the buffer in the pending list */
        if (pDMAChannel->Pending.pQueueHead == NULL)
        {
            pDMAChannel->Pending.pQueueHead = pQueuedBuf;
            pDMAChannel->Pending.pQueueTail = pQueuedBuf;
            pDMAChannel->Pending.ElementCount = 1;
        }
        else
        {
           /* link the descriptors */
            ((ADI_EMAC_BUFINFO*)pDMAChannel->Pending.pQueueTail)->pDmaDesc->pNextDesc = pAvailDesc;

            pDMAChannel->Pending.pQueueTail->pNext = pQueuedBuf;
            pDMAChannel->Pending.pQueueTail = pQueuedBuf;
            pDMAChannel->Pending.ElementCount++;
        }

        /* next available descriptor will be always at the end */
        pAvailDesc->pNextDesc = pDMAChannel->pDmaDescHead;

        pAvailDesc = pDMAChannel->pDmaDescHead;
        pQueuedBuf = pDMAChannel->Queued.pQueueHead;

        if(pQueuedBuf == NULL)
        {
            pDMAChannel->Queued.pQueueTail = NULL;
        }
    }
    EXIT_CRITICAL_REGION();
    return (ADI_ETHER_RESULT_SUCCESS);
}

/*
 * Moves binded buffers from the pending queue and places them in the
 * active queue. It also enables the appropriate config and dma opmode bits.
 */

static ADI_ETHER_RESULT activate_channel(ADI_ETHER_HANDLE hDevice,ADI_EMAC_CHANNEL *pChannel, int32_t nDMADeviceNum)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)hDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    ADI_ETHER_BUFFER *pPendQFirstBuf, *pActiveQLastBuf, *pBuffer;
    ADI_EMAC_DMADESC *pLastDmaDesc, *pNextDmaDesc, *pDmaDesc;
    ADI_EMAC_DMA_CHANNEL* pDMAChannel = &pChannel->DMAChan[nDMADeviceNum];
    uint32_t index;

    ENTER_CRITICAL_REGION();


    /* if no buffers to be processed return */
    if (pDMAChannel->Pending.pQueueHead == NULL)
    {
        EXIT_CRITICAL_REGION();
        return ADI_ETHER_RESULT_SUCCESS;
    }

    /* check if there are any buffers in the active list */
    if ((pDMAChannel->Active.pQueueHead == NULL) ||  (pDMAChannel->Active.ElementCount >= ADI_EMAC_THRESHOLD ))
    {
    	if (pDMAChannel->Active.pQueueHead == NULL) {
    		copy_queue_elements(&pDMAChannel->Active, &pDMAChannel->Pending);

    		pNextDmaDesc = ((ADI_EMAC_BUFINFO*)pDMAChannel->Active.pQueueHead)->pDmaDesc;
    		pLastDmaDesc = ((ADI_EMAC_BUFINFO*)pDMAChannel->Active.pQueueTail)->pDmaDesc;
    		if(pChannel->Recv) {
    			if ((pEmacRegs->EMAC_DMA_STAT & BITM_EMAC_DMA_STAT_RS) == ENUM_EMAC_DMA_STAT_RS_STOPPED)
    			{
    				pEmacRegs->EMAC_DMA_RXDSC_ADDR = (uint32_t)pNextDmaDesc;
    			}
    		} else {
                if ((pEmacRegs->EMAC_DMA_STAT & BITM_EMAC_DMA_STAT_TS) == ENUM_EMAC_DMA_STAT_TS_STOPPED)
                {
                    pEmacRegs->EMAC_DMA_TXDSC_ADDR = (uint32_t)pNextDmaDesc;
                }
    		}
    	} else {
			 pActiveQLastBuf = pDMAChannel->Active.pQueueTail;
			 pPendQFirstBuf  = pDMAChannel->Pending.pQueueHead;

			 pLastDmaDesc = ((ADI_EMAC_BUFINFO*)pActiveQLastBuf)->pDmaDesc;
			 pNextDmaDesc = ((ADI_EMAC_BUFINFO*)pPendQFirstBuf)->pDmaDesc;

			 /* now link the descriptors */
			 pLastDmaDesc->pNextDesc = pNextDmaDesc;

			 if(pDev->Cache) { SIMPLEFLUSHINV(pLastDmaDesc); }

			 /* link the buffers */
			 pActiveQLastBuf->pNext = pPendQFirstBuf;
			 pDMAChannel->Active.pQueueTail = pDMAChannel->Pending.pQueueTail;
			 pDMAChannel->Active.ElementCount += pDMAChannel->Pending.ElementCount;
    	}


    	/* Set the OWN bit of DMA descriptor of the attached buffers */
    	index = 0;
    	pBuffer = pDMAChannel->Pending.pQueueHead;
    	while(pBuffer) {
    		pDmaDesc = ((ADI_EMAC_BUFINFO*)pBuffer)->pDmaDesc;
    		if (((index++ % pDev->TxIntPeriod) != 0) && (pBuffer->pNext != NULL)) {
    			/* Clear interrupt status */
    			pDmaDesc->Status &= (~(1u << 30));
    		}

    	    pDmaDesc->Status     |= ADI_EMAC_DMAOWN;
    	    if(pDev->Cache) { SIMPLEFLUSHINV(pDmaDesc); }

    		pBuffer = pBuffer->pNext;
    	}

  	    /* adjust the last descriptor */
		reset_queue(&pDMAChannel->Pending);
    }


	if(pChannel->Recv) {
	   resume_rx(hDevice, nDMADeviceNum);
	} else {
	   resume_tx(hDevice, nDMADeviceNum);
	}

    EXIT_CRITICAL_REGION();

    return ADI_ETHER_RESULT_SUCCESS;
}


/**
* @brief        Closes the driver and releases any memory
*
* @details      Closes the MAC driver. Buffers that are pending will not be returned.
*
* @param[in]    hEtherDevice    Ethernet Device Handler
*
* @return       Status
*                   - ADI_ETHER_RESULT_SUCCESS on Success
*                   - ADI_ETHER_RESULT_INVALID_PARAM on Parameter Error
*                   - ADI_ETHER_RESULT_FAILED on Error
*
* @details
*               Function closes the driver and releases any memory.
*               If there is any current frames then it will wait for it
*               to complete.
*
* @sa           adi_ether_GemacOpen
*/

ADI_ETHER_RESULT adi_ether_GemacClose(ADI_ETHER_HANDLE hEtherDevice)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)hEtherDevice;
    ADI_ETHER_RESULT    Result=ADI_ETHER_RESULT_FAILED;

    ENTER_CRITICAL_REGION();

    if(pDev->Opened == true)
    {
        /* stop transmit and receive (DMA 0)*/
        gemac_stop_rx(hEtherDevice, 0);
        gemac_stop_tx(hEtherDevice, 0);
        mask_gemac_ints(hEtherDevice, 0);

#ifdef ADI_ETHER_SUPPORT_AV
        if (pDev->Capability & ADI_EMAC_CAPABILITY_AV)
        {
            /* IF(DMA1 is enabled) */
            if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN)
            {
                /* stop transmit and receive (DMA 1)*/
                gemac_stop_rx(hEtherDevice, 1);
                gemac_stop_tx(hEtherDevice, 1);
                mask_gemac_ints(hEtherDevice, 1);

            }

            /* IF(DMA2 is enabled) */
            if (pDev->AVDevice.config & ADI_EMAC_AV_CONFIG_DMA1_EN)
            {
                /* stop transmit and receive (DMA 0)*/
                gemac_stop_rx(hEtherDevice, 2);
                gemac_stop_tx(hEtherDevice, 2);
                mask_gemac_ints(hEtherDevice, 2);
            }
        }
#endif

        /* reset driver internal variables */
        reset_all_queues(hEtherDevice);
        reset_dma_lists(&pDev->Rx);
        reset_dma_lists(&pDev->Tx);

        pDev->Opened = false;
        Result = ADI_ETHER_RESULT_SUCCESS;
    }

    pDev->pPhyDevice->uninit(pDev->pPhyDevice);

    adi_int_UninstallHandler(pDev->Interrupt);

    EXIT_CRITICAL_REGION();

    /* Clear the gemac structure */
    memset(pDev, 0, sizeof(ADI_EMAC_DEVICE));

    return (Result);
}

/**
* @brief        Return the Buffer Prefix for the Driver
*
* @param[in]    hEtherDevice   Ethernet Device Handle
*
* @param[out]   pBufferPrefix  Pointer to Buffer Prefix
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS on Success
*                  - ADI_ETHER_RESULT_INVALID_PARAM on Parameter Error
*                  - ADI_ETHER_RESULT_FAILED on Error
*
*/
ADI_ETHER_RESULT adi_ether_GemacGetBufferPrefix (
                                                 ADI_ETHER_HANDLE    hEtherDevice,
                                                 uint32_t* const     pBufferPrefix
                                                 )
{
    /* The Buffer prefix used is 2 for GEMAC driver */
    (*pBufferPrefix) = 2;

    /* Return Success */
    return (ADI_ETHER_RESULT_SUCCESS);
}

/***************************************************************************//*!
* @brief        Flush a Memory Area
*
* @param[in]    start   Pointer to the start of the memory Segment
* @param[in]    bytes   Number of bytes to flush
*
* @return       None
*
* @details
*               Function flush the given memory segment by flushing out each
*               memory element. If the memory is cached and dirty, the data will
*               be written back to the next higher memory else it act as NOP.
*
* @sa           flushinv_area
*******************************************************************************/
static void flush_area(void *start, uint32_t bytes)
{
    FLUSH_AREA_CODE;
}

/***************************************************************************//*!
* @brief        Flush & Invalidate a Memory Area
*
* @param[in]    start   Pointer to the start of the memory Segment
* @param[in]    bytes   Number of bytes to flush
*
* @return       None
*
* @details
*               Function flush the given memory segment by flushing out each
*               memory element and it also invalidate the cache. If the memory
*               is cached and dirty, the data will be written back to the next
*               higher memory else it act as NOP
*
* @sa           flush_area
*******************************************************************************/
static void flushinv_area(void *start, uint32_t bytes)
{
    FLUSHINV_AREA_CODE;
}

#if 0
#include <stdio.h>
/* Print the addresses of the EMAC registers */
void PrintEMACRegisterAddresses(ADI_ETHER_HANDLE phDevice)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    ETHER_DEBUG("MACCONFIG = 0x%08X\n",&pEmacRegs->EMAC_MACCFG);
    ETHER_DEBUG("EMAC_MACFRMFILT = %08X\n",&pEmacRegs->EMAC_MACFRMFILT);
    ETHER_DEBUG("EMAC_HASHTBL_HI = %08X\n",&pEmacRegs->EMAC_HASHTBL_HI);
    ETHER_DEBUG("EMAC_HASHTBL_LO = %08X\n",&pEmacRegs->EMAC_HASHTBL_LO);
    ETHER_DEBUG("EMAC_GMII_ADDR = %08X\n",&pEmacRegs->EMAC_GMII_ADDR);
    ETHER_DEBUG("EMAC_GMII_DATA = %08X\n",&pEmacRegs->EMAC_GMII_DATA);
    ETHER_DEBUG("EMAC_FLOWCTL = %08X\n",&pEmacRegs->EMAC_FLOWCTL);
    ETHER_DEBUG("EMAC_VLANTAG = %08X\n",&pEmacRegs->EMAC_VLANTAG);
    ETHER_DEBUG("EMAC_VER = %08X\n",&pEmacRegs->EMAC_VER);
    ETHER_DEBUG("EMAC_DBG = %08X\n",&pEmacRegs->EMAC_DBG);
    ETHER_DEBUG("EMAC_RMTWKUP = %08X\n",&pEmacRegs->EMAC_RMTWKUP);
    ETHER_DEBUG("EMAC_PMT_CTLSTAT = %08X\n",&pEmacRegs->EMAC_PMT_CTLSTAT);
    ETHER_DEBUG("EMAC_ISTAT = %08X\n",&pEmacRegs->EMAC_ISTAT);
    ETHER_DEBUG("EMAC_IMSK = %08X\n",&pEmacRegs->EMAC_IMSK);
    ETHER_DEBUG("EMAC_ADDR0_HI = %08X\n",&pEmacRegs->EMAC_ADDR0_HI);
    ETHER_DEBUG("EMAC_ADDR0_LO = %08X\n",&pEmacRegs->EMAC_ADDR0_LO);
    ETHER_DEBUG("EMAC_MMC_CTL = %08X\n",&pEmacRegs->EMAC_MMC_CTL);
    ETHER_DEBUG("EMAC_MMC_RXINT = %08X\n",&pEmacRegs->EMAC_MMC_RXINT);
    ETHER_DEBUG("EMAC_MMC_TXINT = %08X\n",&pEmacRegs->EMAC_MMC_TXINT);
    ETHER_DEBUG("EMAC_MMC_RXIMSK = %08X\n",&pEmacRegs->EMAC_MMC_RXIMSK);
    ETHER_DEBUG("EMAC_MMC_TXIMSK = %08X\n",&pEmacRegs->EMAC_MMC_TXIMSK);
    ETHER_DEBUG("EMAC_TXOCTCNT_GB = %08X\n",&pEmacRegs->EMAC_TXOCTCNT_GB);
    ETHER_DEBUG("EMAC_TXFRMCNT_GB = %08X\n",&pEmacRegs->EMAC_TXFRMCNT_GB);
    ETHER_DEBUG("EMAC_TXBCASTFRM_G = %08X\n",&pEmacRegs->EMAC_TXBCASTFRM_G);
    ETHER_DEBUG("EMAC_TXMCASTFRM_G = %08X\n",&pEmacRegs->EMAC_TXMCASTFRM_G);
    ETHER_DEBUG("EMAC_TX64_GB = %08X\n",&pEmacRegs->EMAC_TX64_GB);
    ETHER_DEBUG("EMAC_TX65TO127_GB = %08X\n",&pEmacRegs->EMAC_TX65TO127_GB);
    ETHER_DEBUG("EMAC_TX128TO255_GB = %08X\n",&pEmacRegs->EMAC_TX128TO255_GB);
    ETHER_DEBUG("EMAC_TX256TO511_GB = %08X\n",&pEmacRegs->EMAC_TX256TO511_GB);
    ETHER_DEBUG("EMAC_TX512TO1023_GB = %08X\n",&pEmacRegs->EMAC_TX512TO1023_GB);
    ETHER_DEBUG("EMAC_TX1024TOMAX_GB = %08X\n",&pEmacRegs->EMAC_TX1024TOMAX_GB);
    ETHER_DEBUG("EMAC_TXUCASTFRM_GB = %08X\n",&pEmacRegs->EMAC_TXUCASTFRM_GB);
    ETHER_DEBUG("EMAC_TXMCASTFRM_GB = %08X\n",&pEmacRegs->EMAC_TXMCASTFRM_GB);
    ETHER_DEBUG("EMAC_TXBCASTFRM_GB = %08X\n",&pEmacRegs->EMAC_TXBCASTFRM_GB);
    ETHER_DEBUG("EMAC_TXUNDR_ERR = %08X\n",&pEmacRegs->EMAC_TXUNDR_ERR);
    ETHER_DEBUG("EMAC_TXSNGCOL_G = %08X\n",&pEmacRegs->EMAC_TXSNGCOL_G);
    ETHER_DEBUG("EMAC_TXMULTCOL_G = %08X\n",&pEmacRegs->EMAC_TXMULTCOL_G);
    ETHER_DEBUG("EMAC_TXDEFERRED = %08X\n",&pEmacRegs->EMAC_TXDEFERRED);
    ETHER_DEBUG("EMAC_TXLATECOL = %08X\n",&pEmacRegs->EMAC_TXLATECOL);
    ETHER_DEBUG("EMAC_TXEXCESSCOL = %08X\n",&pEmacRegs->EMAC_TXEXCESSCOL);
    ETHER_DEBUG("EMAC_TXCARR_ERR = %08X\n",&pEmacRegs->EMAC_TXCARR_ERR);
    ETHER_DEBUG("EMAC_TXOCTCNT_G = %08X\n",&pEmacRegs->EMAC_TXOCTCNT_G);
    ETHER_DEBUG("EMAC_TXFRMCNT_G = %08X\n",&pEmacRegs->EMAC_TXFRMCNT_G);
    ETHER_DEBUG("EMAC_TXEXCESSDEF = %08X\n",&pEmacRegs->EMAC_TXEXCESSDEF);
    ETHER_DEBUG("EMAC_TXPAUSEFRM = %08X\n",&pEmacRegs->EMAC_TXPAUSEFRM);
    ETHER_DEBUG("EMAC_TXVLANFRM_G = %08X\n",&pEmacRegs->EMAC_TXVLANFRM_G);
    ETHER_DEBUG("EMAC_RXFRMCNT_GB = %08X\n",&pEmacRegs->EMAC_RXFRMCNT_GB);
    ETHER_DEBUG("EMAC_RXOCTCNT_GB = %08X\n",&pEmacRegs->EMAC_RXOCTCNT_GB);
    ETHER_DEBUG("EMAC_RXOCTCNT_G = %08X\n",&pEmacRegs->EMAC_RXOCTCNT_G);
    ETHER_DEBUG("EMAC_RXBCASTFRM_G = %08X\n",&pEmacRegs->EMAC_RXBCASTFRM_G);
    ETHER_DEBUG("EMAC_RXMCASTFRM_G = %08X\n",&pEmacRegs->EMAC_RXMCASTFRM_G);
    ETHER_DEBUG("EMAC_RXCRC_ERR = %08X\n",&pEmacRegs->EMAC_RXCRC_ERR);
    ETHER_DEBUG("EMAC_RXALIGN_ERR = %08X\n",&pEmacRegs->EMAC_RXALIGN_ERR);
    ETHER_DEBUG("EMAC_RXRUNT_ERR = %08X\n",&pEmacRegs->EMAC_RXRUNT_ERR);
    ETHER_DEBUG("EMAC_RXJAB_ERR = %08X\n",&pEmacRegs->EMAC_RXJAB_ERR);
    ETHER_DEBUG("EMAC_RXUSIZE_G = %08X\n",&pEmacRegs->EMAC_RXUSIZE_G);
    ETHER_DEBUG("EMAC_RXOSIZE_G = %08X\n",&pEmacRegs->EMAC_RXOSIZE_G);
    ETHER_DEBUG("EMAC_RX64_GB = %08X\n",&pEmacRegs->EMAC_RX64_GB);
    ETHER_DEBUG("EMAC_RX65TO127_GB = %08X\n",&pEmacRegs->EMAC_RX65TO127_GB);
    ETHER_DEBUG("EMAC_RX128TO255_GB = %08X\n",&pEmacRegs->EMAC_RX128TO255_GB);
    ETHER_DEBUG("EMAC_RX256TO511_GB = %08X\n",&pEmacRegs->EMAC_RX256TO511_GB);
    ETHER_DEBUG("EMAC_RX512TO1023_GB = %08X\n",&pEmacRegs->EMAC_RX512TO1023_GB);
    ETHER_DEBUG("EMAC_RX1024TOMAX_GB = %08X\n",&pEmacRegs->EMAC_RX1024TOMAX_GB);
    ETHER_DEBUG("EMAC_RXUCASTFRM_G = %08X\n",&pEmacRegs->EMAC_RXUCASTFRM_G);
    ETHER_DEBUG("EMAC_RXLEN_ERR = %08X\n",&pEmacRegs->EMAC_RXLEN_ERR);
    ETHER_DEBUG("EMAC_RXOORTYPE = %08X\n",&pEmacRegs->EMAC_RXOORTYPE);
    ETHER_DEBUG("EMAC_RXPAUSEFRM = %08X\n",&pEmacRegs->EMAC_RXPAUSEFRM);
    ETHER_DEBUG("EMAC_RXFIFO_OVF = %08X\n",&pEmacRegs->EMAC_RXFIFO_OVF);
    ETHER_DEBUG("EMAC_RXVLANFRM_GB = %08X\n",&pEmacRegs->EMAC_RXVLANFRM_GB);
    ETHER_DEBUG("EMAC_RXWDOG_ERR = %08X\n",&pEmacRegs->EMAC_RXWDOG_ERR);
    ETHER_DEBUG("EMAC_IPC_RXIMSK = %08X\n",&pEmacRegs->EMAC_IPC_RXIMSK);
    ETHER_DEBUG("EMAC_IPC_RXINT = %08X\n",&pEmacRegs->EMAC_IPC_RXINT);
    ETHER_DEBUG("EMAC_RXIPV4_GD_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV4_GD_FRM);
    ETHER_DEBUG("EMAC_RXIPV4_HDR_ERR_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV4_HDR_ERR_FRM);
    ETHER_DEBUG("EMAC_RXIPV4_NOPAY_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV4_NOPAY_FRM);
    ETHER_DEBUG("EMAC_RXIPV4_FRAG_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV4_FRAG_FRM);
    ETHER_DEBUG("EMAC_RXIPV4_UDSBL_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV4_UDSBL_FRM);
    ETHER_DEBUG("EMAC_RXIPV6_GD_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV6_GD_FRM);
    ETHER_DEBUG("EMAC_RXIPV6_HDR_ERR_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV6_HDR_ERR_FRM);
    ETHER_DEBUG("EMAC_RXIPV6_NOPAY_FRM = %08X\n",&pEmacRegs->EMAC_RXIPV6_NOPAY_FRM);
    ETHER_DEBUG("EMAC_RXUDP_GD_FRM = %08X\n",&pEmacRegs->EMAC_RXUDP_GD_FRM);
    ETHER_DEBUG("EMAC_RXUDP_ERR_FRM = %08X\n",&pEmacRegs->EMAC_RXUDP_ERR_FRM);
    ETHER_DEBUG("EMAC_RXTCP_GD_FRM = %08X\n",&pEmacRegs->EMAC_RXTCP_GD_FRM);
    ETHER_DEBUG("EMAC_RXTCP_ERR_FRM = %08X\n",&pEmacRegs->EMAC_RXTCP_ERR_FRM);
    ETHER_DEBUG("EMAC_RXICMP_GD_FRM = %08X\n",&pEmacRegs->EMAC_RXICMP_GD_FRM);
    ETHER_DEBUG("EMAC_RXICMP_ERR_FRM = %08X\n",&pEmacRegs->EMAC_RXICMP_ERR_FRM);
    ETHER_DEBUG("EMAC_RXIPV4_GD_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV4_GD_OCT);
    ETHER_DEBUG("EMAC_RXIPV4_HDR_ERR_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV4_HDR_ERR_OCT);
    ETHER_DEBUG("EMAC_RXIPV4_NOPAY_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV4_NOPAY_OCT);
    ETHER_DEBUG("EMAC_RXIPV4_FRAG_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV4_FRAG_OCT);
    ETHER_DEBUG("EMAC_RXIPV4_UDSBL_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV4_UDSBL_OCT);
    ETHER_DEBUG("EMAC_RXIPV6_GD_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV6_GD_OCT);
    ETHER_DEBUG("EMAC_RXIPV6_HDR_ERR_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV6_HDR_ERR_OCT);
    ETHER_DEBUG("EMAC_RXIPV6_NOPAY_OCT = %08X\n",&pEmacRegs->EMAC_RXIPV6_NOPAY_OCT);
    ETHER_DEBUG("EMAC_RXUDP_GD_OCT = %08X\n",&pEmacRegs->EMAC_RXUDP_GD_OCT);
    ETHER_DEBUG("EMAC_RXUDP_ERR_OCT = %08X\n",&pEmacRegs->EMAC_RXUDP_ERR_OCT);
    ETHER_DEBUG("EMAC_RXTCP_GD_OCT = %08X\n",&pEmacRegs->EMAC_RXTCP_GD_OCT);
    ETHER_DEBUG("EMAC_RXTCP_ERR_OCT = %08X\n",&pEmacRegs->EMAC_RXTCP_ERR_OCT);
    ETHER_DEBUG("EMAC_RXICMP_GD_OCT = %08X\n",&pEmacRegs->EMAC_RXICMP_GD_OCT);
    ETHER_DEBUG("EMAC_RXICMP_ERR_OCT = %08X\n",&pEmacRegs->EMAC_RXICMP_ERR_OCT);
    ETHER_DEBUG("EMAC_TM_CTL = %08X\n",&pEmacRegs->EMAC_TM_CTL);
    ETHER_DEBUG("EMAC_TM_SUBSEC = %08X\n",&pEmacRegs->EMAC_TM_SUBSEC);
    ETHER_DEBUG("EMAC_TM_SEC = %08X\n",&pEmacRegs->EMAC_TM_SEC);
    ETHER_DEBUG("EMAC_TM_NSEC = %08X\n",&pEmacRegs->EMAC_TM_NSEC);
    ETHER_DEBUG("EMAC_TM_SECUPDT = %08X\n",&pEmacRegs->EMAC_TM_SECUPDT);
    ETHER_DEBUG("EMAC_TM_NSECUPDT = %08X\n",&pEmacRegs->EMAC_TM_NSECUPDT);
    ETHER_DEBUG("EMAC_TM_ADDEND = %08X\n",&pEmacRegs->EMAC_TM_ADDEND);
    ETHER_DEBUG("EMAC_TM_TGTM = %08X\n",&pEmacRegs->EMAC_TM_TGTM);
    ETHER_DEBUG("EMAC_TM_NTGTM = %08X\n",&pEmacRegs->EMAC_TM_NTGTM);
    ETHER_DEBUG("EMAC_TM_HISEC = %08X\n",&pEmacRegs->EMAC_TM_HISEC);
    ETHER_DEBUG("EMAC_TM_STMPSTAT = %08X\n",&pEmacRegs->EMAC_TM_STMPSTAT);
    ETHER_DEBUG("EMAC_TM_PPSCTL = %08X\n",&pEmacRegs->EMAC_TM_PPSCTL);
    ETHER_DEBUG("EMAC_TM_AUXSTMP_NSEC = %08X\n",&pEmacRegs->EMAC_TM_AUXSTMP_NSEC);
    ETHER_DEBUG("EMAC_TM_AUXSTMP_SEC = %08X\n",&pEmacRegs->EMAC_TM_AUXSTMP_SEC);
    ETHER_DEBUG("EMAC_DMA_BUSMODE = %08X\n",&pEmacRegs->EMAC_DMA_BUSMODE);
    ETHER_DEBUG("EMAC_DMA_TXPOLL = %08X\n",&pEmacRegs->EMAC_DMA_TXPOLL);
    ETHER_DEBUG("EMAC_DMA_RXPOLL = %08X\n",&pEmacRegs->EMAC_DMA_RXPOLL);
    ETHER_DEBUG("EMAC_DMA_RXDSC_ADDR = %08X\n",&pEmacRegs->EMAC_DMA_RXDSC_ADDR);
    ETHER_DEBUG("EMAC_DMA_TXDSC_ADDR = %08X\n",&pEmacRegs->EMAC_DMA_TXDSC_ADDR);
    ETHER_DEBUG("EMAC_DMA_STAT = %08X\n",&pEmacRegs->EMAC_DMA_STAT);
    ETHER_DEBUG("EMAC_DMA_OPMODE = %08X\n",&pEmacRegs->EMAC_DMA_OPMODE);
    ETHER_DEBUG("EMAC_DMA_IEN = %08X\n",&pEmacRegs->EMAC_DMA_IEN);
    ETHER_DEBUG("EMAC_DMA_MISS_FRM = %08X\n",&pEmacRegs->EMAC_DMA_MISS_FRM);
    ETHER_DEBUG("EMAC_DMA_RXIWDOG = %08X\n",&pEmacRegs->EMAC_DMA_RXIWDOG);
    ETHER_DEBUG("EMAC_DMA_BMMODE = %08X\n",&pEmacRegs->EMAC_DMA_BMMODE);
    ETHER_DEBUG("EMAC_DMA_BMSTAT = %08X\n",&pEmacRegs->EMAC_DMA_BMSTAT);
    ETHER_DEBUG("EMAC_DMA_TXDSC_CUR = %08X\n",&pEmacRegs->EMAC_DMA_TXDSC_CUR);
    ETHER_DEBUG("EMAC_DMA_RXDSC_CUR = %08X\n",&pEmacRegs->EMAC_DMA_RXDSC_CUR);
    ETHER_DEBUG("EMAC_DMA_TXBUF_CUR = %08X\n",&pEmacRegs->EMAC_DMA_TXBUF_CUR);
    ETHER_DEBUG("EMAC_DMA_RXBUF_CUR = %08X\n",&pEmacRegs->EMAC_DMA_RXBUF_CUR);
    ETHER_DEBUG("EMAC_HWFEAT = %08X\n",&pEmacRegs->EMAC_HWFEAT);
}
#endif

/* The Entry Point Structure to the GEMAC Driver */
ADI_ETHER_DRIVER_ENTRY  GEMAC0DriverEntry =
{
    adi_ether_GemacOpen,
    adi_ether_GemacRead,
    adi_ether_GemacWrite,
    adi_ether_GemacClose,
    adi_ether_GemacGetLinkStatus,
    adi_ether_GemacAddMulticastFilter,
    adi_ether_GemacDelMulticastFilter,
    adi_ether_GemacGetBufferPrefix,
    adi_ether_GemacGetMACAddress,
    adi_ether_GemacSetMACAddress,
    adi_ether_GemacEnableMAC,
    adi_ether_GemacModuleIO
};

#ifdef GEMAC_SUPPORT_EMAC1
ADI_ETHER_DRIVER_ENTRY  GEMAC1DriverEntry =
{
    adi_ether_GemacOpen,
    adi_ether_GemacRead,
    adi_ether_GemacWrite,
    adi_ether_GemacClose,
    adi_ether_GemacGetLinkStatus,
    adi_ether_GemacAddMulticastFilter,
    adi_ether_GemacDelMulticastFilter,
    adi_ether_GemacGetBufferPrefix,
    adi_ether_GemacGetMACAddress,
    adi_ether_GemacSetMACAddress,
    adi_ether_GemacEnableMAC,
    adi_ether_GemacModuleIO
};
#endif

/*@}*/
