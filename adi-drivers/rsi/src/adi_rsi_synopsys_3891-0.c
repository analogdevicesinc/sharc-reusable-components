/******************************************************************************

Copyright (c) 2014-2019 Analog Devices.  All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.
 *******************************************************************************
 *
 * @file:    adi_rsi_synopsys_3891-0.c
 * @brief:   RSI device driver implementation for Synopsys MSI product 3891-0
 * @version: $Revision: 62473 $
 * @date:    $Date: 2019-08-26 22:55:23 -0400 (Mon, 26 Aug 2019) $
 *
 ******************************************************************************/

#ifndef _ADI_RSI_SYNOPSYS_3891_0_C_
#define _ADI_RSI_SYNOPSYS_3891_0_C_

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_5_6:"Allow reuse of names in different namespaces")
#pragma diag(suppress:misra_rule_5_7:"Allow reuse of identifiers")
#pragma diag(suppress:misra_rule_8_5:"Allow the definition of objects in the source file. MISRA treats this as include file as it is included by other sources")
#pragma diag(suppress:misra_rule_11_3:"Casts between pointers and integral types are needed for use with certain MMR definitions")
#pragma diag(suppress:misra_rule_14_7:"A function shall have a single point of exit at the end of the function.")
#endif

/* Disable assertion checking unless one of these private symbols is defined */
#if !defined(_ADI_RSI_ASSERTIONS) && !defined(_ADI_RUNTIME_ASSERTIONS) && !defined(NDEBUG)
#define NDEBUG
#endif

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1:"Naming issues in adi_dma")
#pragma diag(suppress:misra_rule_5_3:"Name length issues in adi_dma")
#pragma diag(suppress:misra_rule_5_6:"Naming issues in adi_dma")
#pragma diag(suppress:misra_rule_5_7:"Name length issues in adi_dma")
#pragma diag(suppress:misra_rule_19_7:"function-like macro in adi_dma")
#pragma diag(suppress:misra_rule_19_13:"function-like macro in adi_dma")
#pragma diag(suppress:misra_rule_11_3:"function-like macro in adi_dma")
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/platform.h>

#include <services/pwr/adi_pwr.h> /* for adi_pwr_GetSystemFreq() */
#include <services/gpio/adi_gpio.h>
#include <services/int/adi_int.h>

/* for flush_data_buffer() */
#if defined(__ADSPBLACKFIN__)
#include <cplbtab.h>
#elif defined(__ADSP215xx__)

#if defined(__ADSPARM__)
#include <runtime/cache/adi_cache.h>
#else
#include <sys/cache.h>
#endif

#else
#error Unsupported platform - no flush_data_buffer()
#endif

#if defined(ADI_CODE_IN_ROM)
#include "../rom/adi_rom_def.h"
#else
#define CONST const
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

#include <adi_osal.h>

#include "adi_rsi.h"
#include "adi_rsi_synopsys_3891-0_def.h"

/* Disable MISRA diagnostics as necessary */

/* The static "s_" arrays are only referenced by adi_rsi_Open(), which MISRA doesn't like,
 * but making them function statics would be counterproductive wrt. readability, and goes
 * against the usual way of declaring such data is SSDD code.
 */
#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_8_7:"Function statics don't match usual SSDD module style.")
#pragma diag(suppress:misra_rule_16_7:"Argument usage may change in future so we don't want to make pointers to const.")
#endif

/******************************************************
 Macros:
 *******************************************************/


/* This will be 1 until such time as ADI has a processor with more than
 * one RSI unit.
 */
#define NUM_RSI_DEVICES 1u

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_19_7:"Token pasting is essential to this macro.")
#pragma diag(suppress:misra_rule_19_13:"Although nominally function-like, this macro cannot be replaced by a function.")
#endif

#if (1u == NUM_RSI_DEVICES)
#define DEV_REG(pDev, reg) (*pREG_MSI0_ ## reg)
#else
#error Only a single RSI unit is currently supported.
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

#define pREG_MSI0_FIFO     ((volatile uint32_t *)(REG_MSI0_CTL + 0x200))            /* MSI0 FIFO register */

#define XFRSTAT_ERR_MSK (BITM_MSI_IMSK_EBE | BITM_MSI_IMSK_ACD | BITM_MSI_IMSK_SBEBCI | BITM_MSI_IMSK_HLE | BITM_MSI_IMSK_FRUN | BITM_MSI_IMSK_HTO | BITM_MSI_IMSK_DRTO | BITM_MSI_IMSK_RTO | BITM_MSI_IMSK_DCRC | BITM_MSI_IMSK_RCRC | BITM_MSI_IMSK_RE)

/* The ADSP_BF7xx and ADSP-SC58x MSI peripherals are configured with a 1024-byte FIFO,
 * which operates in 4-byte words (i.e. transfers must be a multiple of 4 bytes).
 */
#define FIFO_DEPTH 256u  /* FIFO depth (in FIFO words) */
#define F_DATA_WIDTH 32u /* FIFO word width (in bits) */
#define H_DATA_WIDTH 32u /* width of host interface (in bits) */

#define DEBOUNCE_MS 25u


/******************************************************
 Internal function prototypes:
 *******************************************************/

static void flushDataCache(
	uint8_t *pBuffer,
	uint32_t bufferSize,
	bool invalidate
);

static void *translateLocalToGlobal(void *ptr);

static ADI_RSI_RESULT submitTxRxBuffer(
	struct ADI_RSI_DEVICE       *pDev,
    uint8_t                     *pBuffer,
    uint32_t               const nBytes
);
static ADI_RSI_RESULT isTxRxBufferAvailable(
	struct ADI_RSI_DEVICE  *pDev,
	bool                   *pbAvailable);
static ADI_RSI_RESULT getTxRxBuffer (
	struct ADI_RSI_DEVICE *pDev,
	uint8_t              **ppBuffer,
	uint32_t              *pBufferSize);
static void interruptHandler(
    uint32_t            iid,
    void*               handlerArg);
static ADI_RSI_RESULT checkXfrStatus(
	struct ADI_RSI_DEVICE *pDev,
	uint32_t successBits);
static ADI_RSI_RESULT updateClockRegs(void);
#if defined (ADI_DEBUG)
static ADI_RSI_RESULT ValidateRsiHandle(ADI_RSI_HANDLE * const hDevice);
#endif
/******************************************************
 Internal data structures:
 *******************************************************/

#if !defined(ADI_CODE_IN_ROM)
static struct ADI_RSI_DEVICE s_rsiDriver[NUM_RSI_DEVICES];
static uint8_t s_dmaSemMemory[NUM_RSI_DEVICES][ADI_OSAL_MAX_SEM_SIZE_CHAR];
#endif /* ADI_CODE_IN_ROM */

/******************************************************
 Internal functions:
 *******************************************************/

static void
flushDataCache(uint8_t *pBuffer, uint32_t bufferSize, bool invalidate)
{
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_5:"Can't change the prototype of flush_data_buffer() to take const *.")
#pragma diag(suppress:misra_rule_17_4:"We can't get the end address without pointer arithmetic.")
#endif /* _MISRA_RULES */
	flush_data_buffer(pBuffer, &pBuffer[bufferSize - 1u], (int)invalidate);
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */
}

#if (defined(__ADSPSC589_FAMILY__) || defined(__ADSPSC573_FAMILY__)) && !defined(__ADSPARM__)
static void *translateLocalToGlobal(void *ptr)
{
	uint32_t addr = (uint32_t)ptr;

	/* It is only valid to call this function with an address in L1 local space */
	if ((addr >= 0x00240000u) && (addr <= 0x0039FFFFu))
	{
		if (ADI_CORE_SHARC0 == adi_core_id())
		{
			addr += 0x28000000u;
		}
		else
		{
			addr += 0x28800000u;
		}
	}

	return (void *)addr;
}
#else
static void *translateLocalToGlobal(void *ptr)
{
	return ptr;
}
#endif

static ADI_RSI_RESULT
checkXfrStatus(struct ADI_RSI_DEVICE *pDev, uint32_t successBits)
{
	ADI_RSI_RESULT result;
	uint32_t msi_RIntSts = DEV_REG(pDev, ISTAT);

	result = ADI_RSI_NOT_FINISHED;

	/* Check for success */
	if (0u != (msi_RIntSts & successBits))
	{
		/* Data Transfer over */
		result = ADI_RSI_SUCCESS;
		DEV_REG(pDev, ISTAT) = (msi_RIntSts & successBits);
	}

	/* Check for errors */
	msi_RIntSts = DEV_REG(pDev, ISTAT); /* reload ISTAT */
	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_EBE))
	{
		/* The data transfer timed out */
		result = ADI_RSI_TIMED_OUT;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_EBE;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_SBEBCI))
	{
		/* There was a startbit error */
		result = ADI_RSI_STARTBIT_ERROR;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_SBEBCI;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_FRUN))
	{
		/* There was a receive fifo overrun error */
		result = ADI_RSI_FIFO_ERROR;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_FRUN;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_HTO))
	{
		/* The data transfer timed out */
		result = ADI_RSI_TIMED_OUT;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_HTO;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_DRTO))
	{
		/* The data transfer timed out */
		result = ADI_RSI_TIMED_OUT;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_DRTO;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_RTO))
	{
		/* The command send timed out */
		result = ADI_RSI_TIMED_OUT;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_RTO;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_DCRC))
	{
		/* There was a data CRC error */
		result = ADI_RSI_CRC_ERROR;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_DCRC;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_RCRC))
	{
		/* There was a command CRC error */
		result = ADI_RSI_CRC_ERROR;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_RCRC;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_HLE))
	{
		/* There was a transmit fifo underrun error */
		result = ADI_RSI_FAILURE;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_HLE;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_RE))
	{
		/* There was a transmit fifo underrun error */
		result = ADI_RSI_FAILURE;
		DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_RE;
	}
	return result;
}

static ADI_RSI_RESULT submitTxRxBuffer(
	struct ADI_RSI_DEVICE       *pDev,
    uint8_t                     *pBuffer,
    uint32_t               const nBytes
)
{
	uint32_t bytesRemaining = nBytes;
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;
	uint32_t descIndex = 0u;
	uint8_t *pByteBuffer = (uint8_t*)translateLocalToGlobal(pBuffer);

	/* Flush the buffer from the data cache(s). Invalidation is not required
	 * for the Tx case, and in the Rx case it will be performed after the DMA
	 * has completed.
	 */
	flushDataCache(pBuffer, nBytes, false);

	/* If more than one descriptor is needed in the chain then all
	 * but the last will be set up by this loop.
	 */
	while ((bytesRemaining > TRANSFER_LIMIT) && (descIndex < NUM_DMA_DESCRIPTORS))
	{
		pDev->vDmaDescriptors[descIndex].DES0 = 0x80000010u;                /* chained descriptor with 'own' bit set */
		pDev->vDmaDescriptors[descIndex].DES1 = TRANSFER_LIMIT;             /* number of bytes for Buffer1, no bytes for Buffer2 */
		pDev->vDmaDescriptors[descIndex].DES2 = pByteBuffer;                /* Tx/Rx buffer address */
		pDev->vDmaDescriptors[descIndex].DES3 = (struct IDMADescriptor*)translateLocalToGlobal(&pDev->vDmaDescriptors[descIndex + 1u]);  /* next descriptor */

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_17_4:"Pointer arithmetic is needed here")
#endif /* _MISRA_RULES */
		pByteBuffer    += TRANSFER_LIMIT;
		bytesRemaining -= TRANSFER_LIMIT;
		++descIndex;
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */
	}

	/* Check that we haven't hit the limit of the number of
	 * descriptors. There must still be at least one free descriptor
	 * left at this point.
	 */
	if (descIndex >= NUM_DMA_DESCRIPTORS)
	{
		result = ADI_RSI_DATA_LENGTH;
	}
	else
	{
		/* Store the original buffer adddress and size */
		pDev->pCurrBuffer = pBuffer;
		pDev->currBufferSize = nBytes;

		/* Set up the last descriptor in the chain */
		pDev->vDmaDescriptors[descIndex].DES0 = 0x80000014u;                /* last descriptor with own bit set */
		pDev->vDmaDescriptors[descIndex].DES1 = bytesRemaining;             /* number of bytes for Buffer1, no bytes for Buffer2 */
		pDev->vDmaDescriptors[descIndex].DES2 = pByteBuffer;                /* Tx/Rx buffer address */
		pDev->vDmaDescriptors[descIndex].DES3 = (struct IDMADescriptor*)translateLocalToGlobal(&pDev->vDmaDescriptors[0]);  /* next descriptor: irrelevant here */

		/* Mark the first descriptor (which may also be the last)*/
		pDev->vDmaDescriptors[0].DES0 |= 0x00000008u;

		/* Flush the descriptors from the cache so that the DMA engine can see them */
		flushDataCache((uint8_t*)pDev->vDmaDescriptors, sizeof(pDev->vDmaDescriptors), false);

		/* Clear the Data Transfer Over bit in the interrupt status */
		DEV_REG(pDev, ISTAT) = BITM_MSI_ISTAT_DTO;

		/* Enable the Integrated DMA Controller */
		DEV_REG(pDev, BUSMODE) = BITM_MSI_BUSMODE_DE;
		DEV_REG(pDev, CTL) |= BITM_MSI_CTL_INTDMAC;

		/* Write the start of the descriptor list to the IDMA controller to initate the transfer */
		DEV_REG(pDev, DBADDR) = (uint32_t)translateLocalToGlobal(&pDev->vDmaDescriptors);
	}

	return result;
}

/*
 * Checks if the buffer is available for processing.
 */
static ADI_RSI_RESULT
isTxRxBufferAvailable(
		struct ADI_RSI_DEVICE  *pDev,
	    bool                   *pbAvailable
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

    if (0u != (DEV_REG(hDevice, ISTAT) & BITM_MSI_ISTAT_DTO))
	{
		*pbAvailable = true;
	}
	else
	{
		*pbAvailable = false;
	}

	return result;
}

/*
 * This will return the buffer if a filled buffer is available,
 * otherwise waits until a buffer is filled.
 */
static ADI_RSI_RESULT
getTxRxBuffer (
	struct ADI_RSI_DEVICE *pDev,
	uint8_t              **ppBuffer,
	uint32_t              *pBufferSize
)
{
	ADI_RSI_RESULT result = ADI_RSI_SEMAPHORE_FAILED;

    /* If Data Transfer Over is set then we don't need to wait */
    if (0u != (DEV_REG(pDev, ISTAT) & BITM_MSI_ISTAT_DTO))
    {
    	/* Clear the DTO bit in the interrupt status */
        DEV_REG(pDev, ISTAT) = BITM_MSI_ISTAT_DTO;
		result = ADI_RSI_SUCCESS;
    }
    else
    {
    	/* DTO isn't set yet, so we enable the interrupt and wait for the handler
    	 * to post the semaphore. Enabling the interrupt at this point should ensure
    	 * there is only one semaphore post for each DMA operation.
    	 */
    	DEV_REG(pDev, IMSK) |= BITM_MSI_IMSK_DTO; /* unmask the DTO interrupt */

		if (ADI_OSAL_SUCCESS == adi_osal_SemPend(pDev->hDmaCompleteSem, ADI_OSAL_TIMEOUT_FOREVER))
		{
			result = pDev->transferResult;

		}
		else
		{
			result = ADI_RSI_SEMAPHORE_FAILED;
		}

		DEV_REG(pDev, IMSK) &= ~BITM_MSI_IMSK_DTO; /* mask the DTO interrupt */
     }

	DEV_REG(pDev, CTL) &= ~BITM_MSI_CTL_INTDMAC;
	DEV_REG(pDev, BUSMODE) &= ~BITM_MSI_BUSMODE_DE;						/* Disable IDMAC */

	*ppBuffer = pDev->pCurrBuffer;
	*pBufferSize = pDev->currBufferSize;
	pDev->pCurrBuffer = NULL;
	pDev->currBufferSize = 0u;

	return result;
}

static void interruptHandler(
    uint32_t            iid,
    void*               handlerArg
)
{
    /* Pointer to the RSI unit instance data */
	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)handlerArg;
	uint32_t interrupts;

	adi_osal_EnterCriticalRegion();

	/* Get the flags from the masked interrupt status register
	 * and then clear them. Only unmasked interrupts will be cleared.
	 * This is to avoid the interrupt handler
	 * being re-entered on return, due to an interrupt not being
	 * cleared. The validity of doing this relies on interrupts not
	 * being re-asserted until some action has been taken, either
	 * by the driver or by the app callback.
	 */
	interrupts = DEV_REG(pDev, MSKISTAT);
	DEV_REG(pDev, ISTAT) = interrupts;

	adi_osal_ExitCriticalRegion();


	/* Check for the Data Transfer Over and Card Detect interrupts.
	 * These are handled locally. All other interrupts are
	 * passed through to the app callback if one
	 * is registered.
	 */
	if (0u != (interrupts & BITM_MSI_MSKISTAT_DTO))
		{
			/* Data Transfer Over */
			adi_osal_SemPost(pDev->hDmaCompleteSem);
		}
	else if (0u != (interrupts & BITM_MSI_MSKISTAT_CD))
	{
		uint32_t cardDetect = (0u == (0x1u & DEV_REG(pDev, CDETECT)));

		if (((0u != cardDetect) && (0u != (pDev->cardEventMask & (ADI_RSI_CARD_INSERTION)))) ||
			((0u == cardDetect) && (0u != (pDev->cardEventMask & (ADI_RSI_CARD_REMOVAL)))))
		{
			/* Dispatch to the user's callback */
			(*pDev->pfAppCallback)(
					pDev->CBparam,
					(uint32_t)ADI_RSI_EVENT_CARD_CHANGE,
					&cardDetect);
		}
	}
	else if ((NULL != pDev->pfAppCallback) && (0u != interrupts))
	{
		/* Dispatch to the user's callback */
		(*pDev->pfAppCallback)(
				pDev->CBparam,
				(uint32_t)ADI_RSI_EVENT_INTERRUPT,
				&interrupts);
	}
	else if((interrupts & (uint32_t)XFRSTAT_ERR_MSK) != 0u)
	{
		/* accumulate status events, these events are passed to application following a GetBuffer */
		pDev->rsiStatusEvent |= interrupts;
	}
	else
	{
		/* This block intentionaly empty */
	}
}


/*
 * API implementation:
 */

/** @addtogroup RSI_Driver RSI Device Driver
 *  @{
 *
 * The RSI (Removable Storage Interface) allows interfacing with
 * various types of storage and I/O devices, including:
 *
 * - Secure Digital (SD) and Secure Digital High Capacity (SDHC) memory cards
 * - Multimedia (MMC) and Embedded Multimedia (eMMC) memory cards
 * - Secure Digital Input/Output (SDIO) peripheral cards
 * - CE-ATA hard disk drives
 *
 * The RSI supports 1-bit, 4-bit, and 8-bit (MMC and CE_ATA only) devices.
 *
 * The adi_rsi driver provides functions to configure the interface (e.g. for
 * bus width and clock frequency), to issue commands to the removable device,
 * and to read or write data blocks as required by those commands. Asynchronous
 * events generated by the RSI hardware can also be handled, such as status
 * interrupts, exceptions, and card insertion/removal notifications.
 */

/**
 * @brief       Open a RSI device instance.
 *
 *  If successful the returned ADI_RSI_HANDLE parameter will hold a device handle
 *  to be used as an input to other RSI API functions.  The RSI device must be
 *  opened before using any other RSI API functions. There may be more than one
 *  concurrent opening of the RSI device.
 *
 *
 * @param [in]  DeviceNum           The RSI device instance number to be opened.
 * @param [out] phDevice            A pointer to a location where the handle to the
 *                                  opened device is written.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                 Successfully opened the given instance.
 *              - #ADI_RSI_BAD_DEVICE_NUMBER       The device number is invalid.
 *              - #ADI_RSI_GPIO_ERR                GPIO initialization failed.
 *              - #ADI_RSI_SEMAPHORE_FAILED        Semaphore initialization failed.
 *              - #ADI_RSI_DMA_FAILED              DMA initialization failed.
 *              - #ADI_RSI_INTERRUPT_FAILURE       Interrupt initialization failed.
 *
 *
 * @sa          adi_rsi_Close()
 */
ADI_RSI_RESULT adi_rsi_Open(
	uint32_t               const  DeviceNum,
	ADI_RSI_HANDLE       * const  phDevice
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	if (DeviceNum >= NUM_RSI_DEVICES)
	{
		result = ADI_RSI_BAD_DEVICE_NUMBER;
	}
	else
	{

#if !defined(ADI_CODE_IN_ROM)
		struct ADI_RSI_DEVICE *pDev = &s_rsiDriver[DeviceNum];
#else
		struct ADI_RSI_DEVICE *pDev = &pram_for_rom_Code->s_rsiDriver[DeviceNum];
#endif /* ADI_CODE_IN_ROM */

		++pDev->openCount;

		/* Only initialize the hardware and data structures on the first open
		 */
		if (1u == pDev->openCount)
		{
			ADI_OSAL_STATUS status;

			pDev->hDmaCompleteSem = NULL;
			pDev->transferResult = ADI_RSI_SUCCESS;
			pDev->pCurrBuffer = NULL;
			pDev->cardDetected = true;
			pDev->pfAppCallback = NULL;
			pDev->CBparam = NULL;
			pDev->cardEventMask = 0u;

			/* Create the semaphore */
#if !defined(ADI_CODE_IN_ROM)
			status = adi_osal_SemCreateStatic(&s_dmaSemMemory[DeviceNum],
					ADI_OSAL_MAX_SEM_SIZE_CHAR, &pDev->hDmaCompleteSem, 0u);
#else
			status = adi_osal_SemCreateStatic(&pram_for_rom_Code->s_dmaSemMemory[DeviceNum],
					ADI_OSAL_MAX_SEM_SIZE_CHAR, &pDev->hDmaCompleteSem, 0u);
#endif /* ADI_CODE_IN_ROM */

			if (ADI_OSAL_SUCCESS != status)
			{
				result = ADI_RSI_SEMAPHORE_FAILED;
			}

			/* Clear any pending interrupts in the raw interrupt register */
			DEV_REG(pDev, ISTAT) = 0xFFFFFFFFu;
			/* Mask out all interrupts */
			DEV_REG(pDev, IMSK) = 0u;
			/* Set the MSI's global interrupt enable */
			DEV_REG(pDev, CTL) |= BITM_MSI_CTL_INTEN | BITM_MSI_CTL_INTDMAC;

			/* Installing the MSI0 status interrupt handler */
			if (ADI_INT_SUCCESS != adi_int_InstallHandler(
					(uint32_t)INTR_MSI0_STAT,
					interruptHandler,
					pDev,
					true
					))
			{
				result = ADI_RSI_INTERRUPT_FAILURE;
			}
		}

		if (ADI_RSI_SUCCESS == result)
		{

			DEV_REG(pDev, ISTAT)    = 0xFFFFFFFFu;

			/* Set the Rx/Tx FIFO thresholds and the DMA multiple transaction size.
			 *
			 * The Synopsys documentation (p.160) says that the number
			 * of words in a burst transfer should be sub-multiple of
			 * (RX_WMark + 1)* (F_DATA_WIDTH/H_DATA_WIDTH) and
			 * (FIFO_DEPTH - TX_WMark)*(F_DATA_WIDTH/H_DATA_WIDTH)
			 *
			 * For ADI's configuration this simplifies to mean that
			 * the number of words in a burst transfer should be a
			 * sub-multiple of FIFO_DEPTH/2 (which is 128 in 32-bit
			 * words = 512 bytes). The 3-bit DMAMASZ field value which
			 * corresponds to 128 words is 0b110 = 6. Setting DMAMASZ to
			 * this value should allow an entire 512-byte SD/MMC card block
			 * to be read or written in a single DMA burst transfer.
			 */
			{
				/* Set the Rx and Tx watermarks to the Synopsys recommended values */
				const uint32_t RX_WMark = (FIFO_DEPTH/2u) - 1u;
				const uint32_t TX_WMark = FIFO_DEPTH/2u;
				const uint32_t DMAMASZ  = 6u;  /* specifies 128-word (512-byte) DMA bursts */

				DEV_REG(pDev, FIFOTH) =
					((DMAMASZ  << BITP_MSI_FIFOTH_DMAMSZ) & BITM_MSI_FIFOTH_DMAMSZ) |
					((RX_WMark << BITP_MSI_FIFOTH_RXWM)   & BITM_MSI_FIFOTH_RXWM)   |
					((TX_WMark << BITP_MSI_FIFOTH_TXWM)   & BITM_MSI_FIFOTH_TXWM);
			}

			/* Reset the default for the next command to be a data write, any
			 * read or non-data operation will override this before the next
			 * adi_rsi_SendCommand() call by calling adi_rsi_SetDataMode().
			 * (For data writes, the uC/FS BSP does not currently call
			 * adi_rsi_SetDataMode() until after the command response has been
			 * received.)
			 */
			pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_RDWR;

			/* Query the Power Service to find the current S0CLK
			 * frequency, as this is what the RSI clock is derived
			 * from. We don't care about SYSCLK or S1CLK.
			 */
			{
				uint32_t sysclk, s0clk, s1clk;

				if (ADI_PWR_SUCCESS == adi_pwr_GetSystemFreq(0u, &sysclk, &s0clk, &s1clk))
				{
					/* The debounce period should be around 25Ms */
					uint32_t debounce = s0clk / (1000u / DEBOUNCE_MS);
					DEV_REG(pDev, DEBNCE) = debounce;
				}
			}

#if defined(__ADSPSC589_FAMILY__)
			/* The MSI device operates as a bus master for DMA, so it needs to be
			 * a secure master, since the rest of the system is also secure (i.e.
			 * the memories).
			 */
			*pREG_SPU0_SECUREP58 = 0x2u; /* make msi secure peripheral */
#elif defined(__ADSPSC573_FAMILY__)
			*pREG_SPU0_SECUREP41 = 0x3u;
#endif

			/* Return the device handle */
			*phDevice = (ADI_RSI_HANDLE *)(void *)pDev;
		}
		else
		{
			/* If the open has been unsuccessful then decrement the open count again */
			--pDev->openCount;
		}
	}

	return result;
}

/**
 * @brief       Close the given RSI device.
 *
 * @param [in]  hDevice             The handle to the RSI device.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS               Successfully closed the given instance.
 *              - #ADI_RSI_SEMAPHORE_FAILED      A semaphore error occurred.
 *              - #ADI_RSI_GPIO_ERR              A GPIO error occurred.
 *              - #ADI_RSI_INTERRUPT_FAILURE     An interrupt error occurred.
 *              - #ADI_RSI_DMA_FAILED            A DMA error occurred.
 *
 * @sa          adi_rsi_Open()
 */
ADI_RSI_RESULT adi_rsi_Close(
    ADI_RSI_HANDLE         const hDevice
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	if (1u == pDev->openCount)
	{
		/* destroy semaphore */
		if (ADI_OSAL_SUCCESS != adi_osal_SemDestroyStatic(pDev->hDmaCompleteSem))
		{
			result = ADI_RSI_SEMAPHORE_FAILED;
		}

		/* Unregister the interrupt handler */
		if (ADI_INT_SUCCESS != adi_int_UninstallHandler((uint32_t)INTR_MSI0_STAT))
		{
			result = ADI_RSI_INTERRUPT_FAILURE;
		}

		/* Clear the MSI's global interrupt enable */
		DEV_REG(pDev, CTL) &= ~BITM_MSI_CTL_INTEN;
		/* Clear any pending interrupts in the raw interrupt register */
		DEV_REG(pDev, ISTAT) = 0xFFFFFFFFu;
		/* Mask out all interrupts */
		DEV_REG(pDev, IMSK) = 0u;
	}

	--pDev->openCount;

	return result;
}


/**
 * @brief       Register an application-defined callback function.
 *
 * An application defined callback function is used to notify the application of
 * device related events. The kinds of events supported are: RSI interrupts, RSI
 * exceptions, and card events (insertion and removal).
 *
 * @param [in]  hDevice             Handle to the RSI device.
 * @param [in]  pfCallback          Application supplied callback function which is
 *                                  called to notify device related events.  A value of
 *                                  NULL disables application callbacks.
 * @param [in]  pCBParam            Application supplied callback parameter which
 *                                  will be passed back in the callback function.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS               Successfully set the application defined callback function.
 *
 */
ADI_RSI_RESULT adi_rsi_RegisterCallback(
    ADI_RSI_HANDLE         const hDevice,
    ADI_CALLBACK           const pfCallback,
    void                        *pCBParam)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	pDev->pfAppCallback = pfCallback;
	pDev->CBparam    = pCBParam;

	return result;
}

/**
 * @brief       Write data to a RSI device (non-blocking).
 *
 * This function returns immediately (does not wait for the write to complete).
 * Completion of the transfer may be queried or waited for via the functions
 * adi_rsi_IsTxBufferAvailable() and adi_rsi_GetTxBuffer(), respectively.
 * The supplied buffer is owned by the driver during the data transfer process.
 * This buffer should not be modified or deleted by the application until the
 * data transfer is complete.  It is recommended that the buffer not be a
 * local variable.
 *
 * @param [in]  hDevice             Handle to the RSI device.
 * @param [in]  pBuffer            The buffer that contains the data to transmit.
 * @param [in]  nBlkSize            The number of bytes per block.
 * @param [in]  nBlkCnt             The number of blocks to write.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully submitted the buffer for writing.
 *              - #ADI_RSI_DMA_FAILED             Failed to submit the buffer for writing.
 *
 *
 * @sa          adi_rsi_IsTxBufferAvailable(), adi_rsi_GetTxBuffer(), adi_rsi_SubmitRxBuffer()
 */
ADI_RSI_RESULT adi_rsi_SubmitTxBuffer(
    ADI_RSI_HANDLE         const hDevice,
    void                        *pBuffer,
    uint32_t               const nBlkSize,
    uint32_t               const nBlkCnt
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	result = submitTxRxBuffer(hDevice, pBuffer, nBlkSize * nBlkCnt);

	return result;
}

/**
 * @brief       Read data from a RSI device (non-blocking).
 *
 * This function returns immediately (does not wait for the read to complete).
 * Completion of the transfer may be queried or waited for via the functions
 * adi_rsi_IsRxBufferAvailable() and adi_rsi_GetRxBuffer(), respectively.
 * The supplied buffer is owned by the driver during the data transfer process.
 * This buffer should not be modified or deleted by the application until the
 * data transfer is complete.  It is recommended that the buffer not be a
 * local variable.
 *
 * @param [in]  hDevice             The handle to the RSI device.
 * @param [out] pBuffer             The data buffer that will contain the received data.
 * @param [in]  nBlkSize            The number of bytes per block.
 * @param [in]  nBlkCnt             The number of blocks to read.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully submitted the buffer for reading.
 *              - #ADI_RSI_DMA_FAILED             The device handle is invalid.
 *
 * @sa          adi_rsi_IsRxBufferAvailable(), adi_rsi_GetRxBuffer(), adi_rsi_SubmitTxBuffer()
 */
ADI_RSI_RESULT adi_rsi_SubmitRxBuffer(
    ADI_RSI_HANDLE         const hDevice,
    void                        *pBuffer,
    uint32_t               const nBlkSize,
    uint32_t               const nBlkCnt
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	result = submitTxRxBuffer(hDevice, pBuffer, nBlkSize * nBlkCnt);

	return result;
}

/**
 * @brief Checks if a Rx Buffer is available for processing.
 *
 * The Rx buffer is available if no Rx buffer transfer is in progress.
 *
 * @param [in]  hDevice             Handle to the RSI device.
 * @param [out] pbAvailable         True if a Rx buffer transfer is not in progress, false otherwise.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully checked if the Rx Buffer is available for processing.
 *              - #ADI_RSI_SEMAPHORE_FAILED       A semaphore operation failed.
 *              - #ADI_RSI_DMA_FAILED             A DMA failure occurred.
 *
 * @sa          adi_rsi_SubmitRxBuffer(), adi_rsi_GetRxBuffer(), adi_rsi_IsTxBufferAvailable()
 */
ADI_RSI_RESULT adi_rsi_IsRxBufferAvailable(
	    ADI_RSI_HANDLE         const hDevice,
	    bool                   *pbAvailable
)
{
	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	return isTxRxBufferAvailable(hDevice, pbAvailable);
}

/**
 * @brief Checks if a Tx Buffer is available for processing.
 *
 * The Tx buffer is available if no Tx buffer transfer is in progress.
 *
 * @param [in]  hDevice             Handle to the RSI device.
 * @param [out] pbAvailable         True if a Tx buffer transfer is not in progress, false otherwise.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully checked if the Tx Buffer is available for processing.
 *              - #ADI_RSI_SEMAPHORE_FAILED       A semaphore operation failed.
 *              - #ADI_RSI_DMA_FAILED             A DMA failure occurred.
 *
 * @sa          adi_rsi_SubmitTxBuffer(), adi_rsi_GetTxBuffer(), adi_rsi_IsRxBufferAvailable()
 */
ADI_RSI_RESULT adi_rsi_IsTxBufferAvailable(
	    ADI_RSI_HANDLE         const hDevice,
	    bool                   *pbAvailable
)
{
	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	return isTxRxBufferAvailable(hDevice, pbAvailable);
}

/**
 * @brief Return the Rx buffer if a filled buffer is available, otherwise
 *        wait until a buffer is filled.
 *
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [out] ppBuffer      The available Rx buffer.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully returned a Rx buffer.
 *              - #ADI_RSI_SEMAPHORE_FAILED       A semaphore operation failed.
 *              - #ADI_RSI_DMA_FAILED             A DMA failure occurred.
 *
 * @sa          adi_rsi_SubmitRxBuffer(), adi_rsi_IsRxBufferAvailable(), adi_rsi_GetTxBuffer()
 */
ADI_RSI_RESULT
adi_rsi_GetRxBuffer (
	ADI_RSI_HANDLE   const   hDevice,
	void                   **ppBuffer
)
{

	ADI_RSI_RESULT result;
	uint8_t *pBuffer;
	uint32_t bufferSize;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	result = getTxRxBuffer(hDevice, &pBuffer, &bufferSize);

	/* Flush and invalidate the buffer. This is only necessary for Rx buffers,
	 * which is why we do it here and not in getTxRxBuffer().
	 */
	flushDataCache(pBuffer, bufferSize, true);

	*ppBuffer = pBuffer;

	return result;
}

/**
 * @brief Return the Tx buffer if a filled buffer is available, otherwise
 *        wait until a buffer is filled.
 *
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [out] ppBuffer      The available Tx buffer.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully returned a Tx buffer.
 *              - #ADI_RSI_SEMAPHORE_FAILED       A semaphore operation failed.
 *              - #ADI_RSI_DMA_FAILED             A DMA failure occurred.
 *
 * @sa          adi_rsi_SubmitTxBuffer(), adi_rsi_IsTxBufferAvailable(), adi_rsi_GetRxBuffer()
 */
ADI_RSI_RESULT
adi_rsi_GetTxBuffer (
	ADI_RSI_HANDLE   const   hDevice,
	void                   **ppBuffer
)
{
	ADI_RSI_RESULT result;
	uint8_t *pBuffer;
	uint32_t bufferSize;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	result = getTxRxBuffer(hDevice, &pBuffer, &bufferSize);

	*ppBuffer = pBuffer;

	return result;
}

/**
 * @brief Set the RSI to the power-on state, and optionally set other configuration flags.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  flags         Optional flags for controlling RSI operation.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully enabled RSI.
 *
 * @sa adi_rsi_Disable()
 */
ADI_RSI_RESULT
adi_rsi_Enable(
	ADI_RSI_HANDLE const hDevice,
	uint32_t flags
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	uint32_t clken = 0u;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	if (0u != (flags & ADI_RSI_ENABLE_CLKSEN))
	{
		clken = BITM_MSI_CLKEN_EN0;
	}

	if (0u != (flags & ADI_RSI_ENABLE_PUPDATALL))
	{
	}

	if (0u != (flags & ADI_RSI_ENABLE_OPENDRAIN))
	{
		/* Should set enable_OD_pullup (bit 24) in REG_MSI0_CTL,
		 * but no symbol for this bit is currently defined.
		 *
		 * This is because open-drain pullup is an option that is not
		 * implemented in ADSP-BF707. If required, customer boards
		 * would need to implement it externally, using GPIO control,
		 * and the driver would need to be customized here. Given that
		 * open-drain is only useful where multiple MMC vards are sharing
		 * the same bus, it is unlikely that such funtionality will
		 * ever be required (SD cards do not support bus-sharing).
		 */
	}

	/* Start the card clock, if required */
	DEV_REG(hDevice, CLKEN) = clken;
	result = updateClockRegs();

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	return result;
}

/**
 * @brief Set the RSI to the power-off state, restoring the default configuration.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully disabled RSI.
 *
 * @sa adi_rsi_Enable()
 */
ADI_RSI_RESULT
adi_rsi_Disable(ADI_RSI_HANDLE const hDevice)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	DEV_REG(hDevice, CLKEN) = 0u;

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	return result;
}

/**
 * @brief Sets the block size for an upcoming RSI transfer, and the number of blocks
 *        to be transferred.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  blkCnt        The number of blocks to read.
 * @param [in]  blkSize       The number of bytes per block.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully set the transfer size.
 *              - #ADI_RSI_DATA_LENGTH            Invalid total transfer size.
 *
 */
ADI_RSI_RESULT
adi_rsi_SetBlockCntAndLen(
		ADI_RSI_HANDLE const hDevice,
		uint32_t       const blkCnt,
		uint32_t       const blkSize
		)
{
	ADI_RSI_RESULT result = ADI_RSI_DATA_LENGTH;

	uint32_t dataLen = blkCnt * blkSize;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	/* Check that the block size and the overall transfer size
	 * are within the required limits.
	 */
	if ((((blkSize << BITP_MSI_BLKSIZ_VALUE) & BITM_MSI_BLKSIZ_VALUE) == (blkSize << BITP_MSI_BLKSIZ_VALUE)) &&
		(dataLen <= ADI_RSI_MAX_TRANSFER_BYTES))
	{
		DEV_REG(hDevice, BLKSIZ) = (blkSize << BITP_MSI_BLKSIZ_VALUE) & BITM_MSI_BLKSIZ_VALUE;
		DEV_REG(hDevice, BYTCNT) = dataLen;	/* length of data transfer */
		result = ADI_RSI_SUCCESS;

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
		ssync();
#endif
	}

	return result;
}

/**
 * @brief Set one of the communication timeouts for the RSI.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  kind          Which timeout to set (data, boot ack, sleep-wakeup).
 * @param [in]  timeout       The timeout period in RSI clocks.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully set the timeout.
 *
 */
ADI_RSI_RESULT
adi_rsi_SetTimeout(
		ADI_RSI_HANDLE const hDevice,
		ADI_RSI_TIMEOUT const kind,
		uint32_t const timeout)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	const uint32_t tmOutReg = DEV_REG(hDevice, TMOUT);
	uint32_t dataTimeout = (tmOutReg & BITM_MSI_TMOUT_DATA)     >> BITP_MSI_TMOUT_DATA;
	uint32_t respTimeout = (tmOutReg & BITM_MSI_TMOUT_RESPONSE) >> BITP_MSI_TMOUT_RESPONSE;



	switch(kind)
	{
	case ADI_RSI_TIMEOUT_DATA:
		dataTimeout = timeout;    	/* data timeout value */
		respTimeout = 255u;
		break;
	case ADI_RSI_TIMEOUT_RESPONSE:
		respTimeout = timeout;
		break;
	default:
		assert(0);
		break;
	}

	DEV_REG(pDev, TMOUT) =
			((dataTimeout << BITP_MSI_TMOUT_DATA)     & BITM_MSI_TMOUT_DATA) |   	/* data timeout value */
			((respTimeout << BITP_MSI_TMOUT_RESPONSE) & BITM_MSI_TMOUT_RESPONSE);   /* response timeout value */

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	return result;
}

#define CTRL_MASK (BITM_MSI_CTL_INTDMAC)

/**
 * @brief Set the mode of operation for RSI data transfer.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  transfer      Which type of transfer to enable.
 * @param [in]  ceata         The CE-ATA mode setting.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully set the data mode.
 *
 */
ADI_RSI_RESULT
adi_rsi_SetDataMode(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_TRANSFER const transfer,
	ADI_RSI_CEATA_MODE const ceata
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;
	uint32_t ctrl;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	ctrl = DEV_REG(pDev, CTL);
#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	switch (transfer)
	{
	case ADI_RSI_TRANSFER_NONE:
		pDev->cmdDataModeFlags = 0u;
		ctrl &= ~(BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_PIO_BLCK_READ:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT;
		ctrl &= ~(BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_PIO_BLCK_WRITE:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_RDWR;
		break;
	case ADI_RSI_TRANSFER_DMA_BLCK_READ:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT;
		ctrl |= (BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_DMA_BLCK_WRITE:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_RDWR;
		ctrl |= (BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_PIO_STRM_READ:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_XFRMODE;
		ctrl &= ~(BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_PIO_STRM_WRITE:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_RDWR | BITM_MSI_CMD_XFRMODE;
		ctrl &= ~(BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_DMA_STRM_READ:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_XFRMODE;
		ctrl |= (BITM_MSI_CTL_INTDMAC);
		break;
	case ADI_RSI_TRANSFER_DMA_STRM_WRITE:
		pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_RDWR | BITM_MSI_CMD_XFRMODE;
		ctrl |= (BITM_MSI_CTL_INTDMAC);
		break;
	default:
		result = ADI_RSI_FAILURE;
		break;
	}

	if (ADI_RSI_SUCCESS == result)
	{
		DEV_REG(pDev, CTL) = ((ctrl & CTRL_MASK) | (DEV_REG(pDev, CTL) & ~CTRL_MASK));

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif
	}

	return result;
}

/**
 * @brief Set the mode of operation for RSI data transfer.
 *
 * @param [in]  hDevice                Handle to the RSI device.
 * @param [in]  cmd                    The SD/MMC command to send.
 * @param [in]  arg                    The argument to send with the command.
 * @param [in]  flags                  Optional flags for the command.
 * @param [in]  expectedResponseType   The type of response expected (long, short, or none).
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Successfully sent the command.
 *
 */
ADI_RSI_RESULT
adi_rsi_SendCommand(
	ADI_RSI_HANDLE const hDevice,
	uint32_t const cmd,
	uint32_t const arg,
	uint32_t const flags,
	ADI_RSI_RESPONSE_TYPE const expectedResponseType
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;
	uint32_t command = ((cmd << BITP_MSI_CMD_INDX) & BITM_MSI_CMD_INDX);

	DEV_REG(pDev, ISTAT) = 0xFFFFFFFEu; 	/* Clear the interrupt status */

	command |= pDev->cmdDataModeFlags;      /* OR in the saved data mode flags */

	/* Set the flags to indicate the expected response */
	switch (expectedResponseType)
	{
	case ADI_RSI_RESPONSE_TYPE_LONG:
		/* BITM_MSI_CMD_RLEN is a "don't care" unless BITM_MSI_CMD_RXPECT is also set */
		command |= (BITM_MSI_CMD_RXPECT | BITM_MSI_CMD_RLEN);
		break;
	case ADI_RSI_RESPONSE_TYPE_SHORT:
		command |= BITM_MSI_CMD_RXPECT;
		break;
	case ADI_RSI_RESPONSE_TYPE_NONE:
		break;
	default:
		assert(0);
		break;
	}

	if (0u != (flags & ADI_RSI_CMDFLAG_CHKBUSY))
	{
		/* Wait for current command transfer to complete */
		command |= BITM_MSI_CMD_WTPRIVDATA;
	}

	/* For MSI we invert the sense of the CRC flag, as it's a disable
	 * in the API but an enable on the hardware.
	 */
	if (0u == (flags & ADI_RSI_CMDFLAG_CRCDIS))
	{
		/* Enable the checking of the response CRC */
		command |= BITM_MSI_CMD_CHKRESPCRC;
	}

	/* For data read operations we must configure the card read threshold.
	 */
	if ((0u != (command & BITM_MSI_CMD_DXPECT)) && (0u == (command & BITM_MSI_CMD_RDWR)))
	{
		/* It's a data read.
		 *
		 * "When an application needs to perform a Single or Multiple Block Read
		 * command, the application must program the CardThrCtl register with
		 * the appropriate Card Read Threshold size (CardRdThreshold) and set
		 * the Card Read Threshold Enable (CardRdThrEnable) bit to 1'b1. This
		 * additional programming ensures that the Host controller sends a Read
		 * Command only if there is space equal to the CardRDThreshold available
		 * in the Rx FIFO. This in turn ensures that the card clock is not
		 * stopped in the middle a block of data being transmitted from the
		 * card. The Card Read Threshold can be set to the block size of the
		 * transfer, which guarantees that there is a minimum of one block size
		 * of space in the RxFIFO before the controller enables the card clock."
		 * -- Programming the DWC_mobile_storage, v2.70a, p.223
		 */
		const uint32_t fifoSizeInBytes = (FIFO_DEPTH*F_DATA_WIDTH)/8u; /* F_DATA_WIDTH is in bits */
		const uint32_t blockSize = (DEV_REG(hDevice, BLKSIZ) & BITM_MSI_BLKSIZ_VALUE) >> BITP_MSI_BLKSIZ_VALUE;

		uint32_t cardThrCtl = DEV_REG(hDevice, CDTHRCTL); /* get the card read threshold MMR contents */

		if (blockSize <= fifoSizeInBytes)
		{
			cardThrCtl &= ~BITM_MSI_CDTHRCTL_RDTHR;               /* clear the read threshold field */
			cardThrCtl |= (blockSize << BITP_MSI_CDTHRCTL_RDTHR); /* merge in the new threshold value */
			cardThrCtl |= BITM_MSI_CDTHRCTL_RDTHREN;              /* set the card read threshold enable */
		}
		else
		{
			cardThrCtl &= ~BITM_MSI_CDTHRCTL_RDTHREN; /* clear the card read threshold enable */
		}

		DEV_REG(hDevice, CDTHRCTL) = cardThrCtl; /* update the card read threshold MMR contents */
	}

	/* Send the command and its argument to the card
	 */
	DEV_REG(pDev, CMDARG) = arg;
	DEV_REG(pDev, CMD) = (command| BITM_MSI_CMD_USEHOLDREG | BITM_MSI_CMD_STARTCMD | BITM_MSI_CMD_WTPRIVDATA);

	/* Reset the default for the next command to be a data write, any
	 * read or non-data operation will override this before the next
	 * adi_rsi_SendCommand() call by calling adi_rsi_SetDataMode().
	 * (For data writes, the uC/FS BSP does not currently call
	 * adi_rsi_SetDataMode() until after the command response has been
	 * received.)
	 */
	pDev->cmdDataModeFlags = BITM_MSI_CMD_DXPECT | BITM_MSI_CMD_RDWR;

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	return result;
}

/**
 * @brief Check for successful completion of an SD/MMC command.
 *
 * @param [in]  hDevice                Handle to the RSI device.
 * @param [in]  expectedResponseType   The type of response expected (long, short, or none).
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Command was successful.
 *              - #ADI_RSI_NOT_FINISHED           Command has not completed yet.
 *              - #ADI_RSI_FAILURE                The expected response was not received.
 *              - #ADI_RSI_TIMED_OUT              The command timed out.
 *              - #ADI_RSI_CRC_ERROR              Cyclic Reduncancy Check error.
 *              - #ADI_RSI_STARTBIT_ERROR         Start bit error.
 *              - #ADI_RSI_FIFO_ERROR             Fifo overrun or underrun.
 *
 */
ADI_RSI_RESULT
adi_rsi_CheckCommand(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_RESPONSE_TYPE const expectedResponseType
	)
{
	ADI_RSI_RESULT result = ADI_RSI_NOT_FINISHED;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	/* First check for success */
	if (0u != (DEV_REG(hDevice, ISTAT) & BITM_MSI_MSKISTAT_CMDDONE))
	{
		/* Check the MSI raw interrupt status register for completion flags */
		uint32_t msi_RIntSts = DEV_REG(hDevice, ISTAT);

		/* Command done */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_CMDDONE;
		msi_RIntSts &= ~BITM_MSI_MSKISTAT_CMDDONE;

		result = ADI_RSI_SUCCESS;

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_DTO))
	{
		/* Data transfer over */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_DTO;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_DTO;
		result = ADI_RSI_SUCCESS;
	}

	/* Check for errors */
	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_EBE))
	{
		/* The data transfer timed out */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_EBE;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_EBE;
		result = ADI_RSI_TIMED_OUT;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_SBEBCI))
	{
		/* There was a startbit error */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_SBEBCI;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_SBEBCI;
		result = ADI_RSI_STARTBIT_ERROR;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_FRUN))
	{
		/* There was a receive fifo overrun error */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_FRUN;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_FRUN;
		result = ADI_RSI_FIFO_ERROR;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_HTO))
	{
		/* The data transfer timed out */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_HTO;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_HTO;
		result = ADI_RSI_TIMED_OUT;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_DRTO))
	{
		/* The data transfer timed out */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_DRTO;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_DRTO;
		result = ADI_RSI_TIMED_OUT;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_RTO))
	{
		/* The command send timed out */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_RTO;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_RTO;
		result = ADI_RSI_TIMED_OUT;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_DCRC))
	{
		/* There was a data CRC error */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_DCRC;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_DCRC;
		result = ADI_RSI_CRC_ERROR;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_HLE))
	{
		/* Hardware locked error */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_HLE;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_HLE;
		result = ADI_RSI_FAILURE;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_RE))
	{
		/* Response error */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_RE;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_RE;
		result = ADI_RSI_FAILURE;
	}

	if (0u != (msi_RIntSts & BITM_MSI_MSKISTAT_RCRC))
	{
		/* There was a response CRC error */
		DEV_REG(hDevice, ISTAT) = BITM_MSI_MSKISTAT_RCRC;
			msi_RIntSts &= ~BITM_MSI_MSKISTAT_RCRC;
		result = ADI_RSI_CRC_ERROR;
	}
#if defined (ADI_DEBUG)
		assert(msi_RIntSts == DEV_REG(hDevice, ISTAT));
#endif
	}

	return result;
}

/**
 * @brief Test whether the returned command is the one expected.
 *
 * @param [in]  hDevice                Handle to the RSI device.
 * @param [in]  cmd                    The expected command code.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Command was successful.
 *              - #ADI_RSI_CMD_RESPONSE_ERR       Command has not completed yet.
 */
ADI_RSI_RESULT adi_rsi_CheckResponseCommand(
	ADI_RSI_HANDLE const hDevice,
	uint32_t const cmd
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	const uint32_t respIndex = ((DEV_REG(hDevice, STAT) & BITM_MSI_STAT_RSPINDX) >> BITP_MSI_STAT_RSPINDX);

	if (cmd != respIndex)
	{
		result = ADI_RSI_CMD_RESPONSE_ERR;
	}

	return result;
}

/**
 * @brief Retrieves a short (32-bit) command response from the RSI.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [out] pResp         The 32-bit response.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Retrieved the response.
 *
 */
ADI_RSI_RESULT
adi_rsi_GetShortResponse(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pResp)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	*pResp = DEV_REG(hDevice, RESP0);

	return result;
}

/**
 * @brief Retrieves a long (128-bit) command response from the RSI.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [out] vResp         The 128-bit response.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Retrieved the response.
 *
 */
ADI_RSI_RESULT
adi_rsi_GetLongResponse(
	ADI_RSI_HANDLE const hDevice,
	uint32_t vResp[4]
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	/* For long responses, the MSI registers are in the opposite order
	 * to what might be expected, and to the order on the earlier RSI
	 * hardware.
	 */
	vResp[0] = DEV_REG(hDevice, RESP3);
	vResp[1] = DEV_REG(hDevice, RESP2);
	vResp[2] = DEV_REG(hDevice, RESP1);
	vResp[3] = DEV_REG(hDevice, RESP0);

	return result;
}

/**
 * @brief Set the SD/MMC bus width for RSI operations.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  width         The bus width to use.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Retrieved the response.
 *
 */
ADI_RSI_RESULT
adi_rsi_SetBusWidth(
	ADI_RSI_HANDLE const hDevice,
	uint32_t const width
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	switch(width)
	{
	case 1u:
		DEV_REG(hDevice, CTYPE) = 0u;
		break;
	case 4u:
		DEV_REG(hDevice, CTYPE) = BITM_MSI_CTYPE_WIDNIB0;
		break;
	case 8u:
		DEV_REG(hDevice, CTYPE) = BITM_MSI_CTYPE_WIDBYTE0;
		break;
	default:
		result = ADI_RSI_BUS_WIDTH;
		break;
	}

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	return result;
}

/**
 * @brief Set the card type for RSI operations.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  cardType      The type of card (SDIO, eMMC, SD/MMC, CE-ATA).
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Set the card type.
 *
 */
ADI_RSI_RESULT
adi_rsi_SetCardType(
	ADI_RSI_HANDLE const hDevice,
	ADI_RSI_CARD_TYPE cardType
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

    struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	pDev->cardType = cardType;

	return result;
}

/**
 * @brief Query the RSI input frequency.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [out] pInClk        The frequency in Hz.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Returned the frequency.
 *              - #ADI_RSI_PWR_ERROR              An error occurred in the adi_pwr service.
 *
 * @sa adi_rsi_SetClock()
 */
ADI_RSI_RESULT
adi_rsi_GetInputFreq(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pInClk
	)
{
	ADI_RSI_RESULT result = ADI_RSI_PWR_ERROR;

#if defined(__ADSPSC589_FAMILY__) || defined (__ADSPSC573_FAMILY__)
	/* On the ADSP-SC589 family we assume that CLKO9 (the input to the MSI module)
	 * is configured as 50MHz, since adi_pwr does not currently provide any way to
	 * query the frequency of this clock.
	 */
	*pInClk = 50000000u;
	result = ADI_RSI_SUCCESS;
#else
	/* Query the Power Service to find the current S0CLK frequency, as this
	 * is what the RSI clock is derived from (divided by 2).
	 * We don't care about SYSCLK or S1CLK.
	 */
	uint32_t sysclk, s0clk, s1clk;

	if (ADI_PWR_SUCCESS == adi_pwr_GetSystemFreq(0u, &sysclk, &s0clk, &s1clk))
	{
		*pInClk = s0clk/2u;
		result = ADI_RSI_SUCCESS;
	}
#endif

	return result;
}

static ADI_RSI_RESULT updateClockRegs(void)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;
	bool hardwareLockedError;

	/* Write the "update clock registers" command to the interface, with STARTCMD set */
	DEV_REG(pDev, CMD) = (BITM_MSI_CMD_STARTCMD  | BITM_MSI_CMD_UCLKREGS);

#if !defined(__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
	ssync();
#endif

	/* Loop retrying on hardware locked errors */
	do
	{
		hardwareLockedError = false;

		/* Wait for the hardware to clear the STARTCMD bit */
		while (0u != (BITM_MSI_CMD_STARTCMD & DEV_REG(pDev, CMD)))
		{
			/* Check for hardware locked error */
			/* We should check for errors here */
			if (0u != (DEV_REG(pDev, ISTAT) & BITM_MSI_MSKISTAT_HLE))
			{
				hardwareLockedError = true;
				DEV_REG(pDev, ISTAT)  = BITM_MSI_MSKISTAT_HLE;
				break;
			}
		}
	} while (hardwareLockedError);

	DEV_REG(pDev, ISTAT) = BITM_MSI_MSKISTAT_CMDDONE;

	return result;
}

/**
 * @brief Set the RSI bus clock frequency.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  clkDivisor    The divisor to apply to the RSI input clock.
 * @param [in]  rsiClkMode    The RSI clock operating mode.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                Returned the frequency.
 *              - #ADI_RSI_INVALID_CLK_DIV        The specified clock divisor is invalid.
 *              - #ADI_RSI_INVALID_CLK_MODE       The specified clock mode is invalid.
 *
 * @sa adi_rsi_GetInputFreq()
 */
ADI_RSI_RESULT
adi_rsi_SetClock(
	ADI_RSI_HANDLE   const hDevice,
	uint32_t         const clkDivisor,
	ADI_RSI_CLK_MODE const rsiClkMode
	)
{
	ADI_RSI_RESULT result = ADI_RSI_INVALID_CLK_DIV;

	uint32_t clkMode = 0u;
	uint32_t divisor;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	switch(clkDivisor)
	{
	case 0u: /* a divisor of zero is invalid */
		break;
	case 1u: /* a divisor of 1 means bypass the clock divider */
		divisor = 0u; /* zero bypasses the divider on the Synopsys hardware */
		result = ADI_RSI_SUCCESS;
		break;
	default: /* divisor must be greater than 1, less than 513, and even */
		/* Compute the hardware divisor */
		divisor = (clkDivisor / 2u);      /* divide the divisor by 2 */
		divisor <<= (uint32_t)BITP_MSI_CLKDIV_DIV0; /* shift to the register's field position */
		divisor &= BITM_MSI_CLKDIV_DIV0;  /* and mask to the register's field width */

		/* Validate the divisor by reversing the calculation and checking
		 * that we get the value that was originally requested.
		 */
		if (((divisor >> BITP_MSI_CLKDIV_DIV0) * 2u) == clkDivisor)
		{
			result = ADI_RSI_SUCCESS;
		}
		break;
	}

	if (ADI_RSI_SUCCESS == result)
	{
		switch (rsiClkMode)
		{
		case ADI_RSI_CLK_MODE_DISABLE:
			clkMode = 0u;
			break;
		case ADI_RSI_CLK_MODE_ENABLE:
			clkMode = BITM_MSI_CLKEN_EN0;
			break;
		case ADI_RSI_CLK_MODE_PWRSAVE:
			clkMode = (BITM_MSI_CLKEN_EN0 | BITM_MSI_CLKEN_LP0);
			break;
		default:
			result = ADI_RSI_INVALID_CLK_MODE;
			break;
		}
	}

	if (ADI_RSI_SUCCESS == result)
	{
		/* Stop the card clock */
		DEV_REG(hDevice, CLKEN) = 0u;
		result = updateClockRegs();

		/* Set the new clock divisor */
		DEV_REG(hDevice, CLKDIV) = ((divisor << BITP_MSI_CLKDIV_DIV0) & BITM_MSI_CLKDIV_DIV0);
		result = updateClockRegs();

		/* Restart the card clock in the new mode, if required */
		DEV_REG(hDevice, CLKEN) = clkMode;
		result = updateClockRegs();
	}

	return result;
}

/**
 * @brief Query the state of the card slot.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS                A card is present and not locked.
 *              - #ADI_RSI_CARD_PROTECTED         The card's "lock" switch is set.
 *              - #ADI_RSI_NO_CARD                There is no card in the slot.
 *              - #ADI_RSI_GPIO_ERR               An adi_gpio error occurred.
 *
 */
ADI_RSI_RESULT adi_rsi_IsCardPresent(
	ADI_RSI_HANDLE const hDevice)
{
	ADI_RSI_RESULT result = ADI_RSI_GPIO_ERR;

	const uint32_t cardDetect = DEV_REG(hDevice, CDETECT);

	/* Note: the MSI peripheral, as configured in ADSP-BF70x, does not
	 * have any dedicated support for detection of the write-protect (WP) setting
	 * of memory cards (unlike the card-detection support, which is built-in).
	 * Any customer board requiring WP depection must do so using GPIO, and the
	 * coe below can then be customized to return ADI_RSI_CARD_PROTECTED in the
	 * write-protected case.
	 */
	if (0u != (BITM_MSI_CDETECT_CD0 & cardDetect))
	{
		/* card_detect_0 is 1 - no card in slot
		assert(!pDev->cardDetected);*/
		result = ADI_RSI_NO_CARD;
	}
	else
	{
		/* card_detect_0 is 0 - card present */
#if defined (ADI_DEBUG)
		assert(pDev->cardDetected);
#endif
		result = ADI_RSI_SUCCESS;
	}

	return result;
}

/**
 * @brief Perform a blocking data read from the RSI.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [out] pBuffer       The buffer to receive the data.
 * @param [in]  nBlkSize      The block size for the transfer.
 * @param [in]  nBlkCnt       The number of blocks to transfer.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS         A card is present and not locked.
 *              - #ADI_RSI_FAILURE         An error occurred.
 *              - #ADI_RSI_CRC_ERROR       A CRC error occurred.
 *              - #ADI_RSI_TIMED_OUT       The operation timed out.
 *              - #ADI_RSI_NOT_FINISHED    The operation is still running.
 *
 * @sa adi_rsi_Write()
 */
ADI_RSI_RESULT
adi_rsi_Read(
	ADI_RSI_HANDLE         const hDevice,
	void                        *pBuffer,
	uint32_t               const nBlkSize,
	uint32_t               const nBlkCnt
)
{
	ADI_RSI_RESULT result = ADI_RSI_NOT_FINISHED;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;
	uint32_t *pBufferWord = pBuffer;
	uint32_t blockCount;

	uint32_t rawIntStatus = 0u;
	uint32_t status = 0u;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	/* For each block */
	for(blockCount = 0u;
		(blockCount < nBlkCnt) && (0u == (rawIntStatus & XFRSTAT_ERR_MSK));
		++blockCount)
	{
		uint32_t byteCount;

		/* For each 4-byte word in the block */
		for(byteCount = 0u;
			(byteCount < nBlkSize) && (0u == (rawIntStatus & XFRSTAT_ERR_MSK));
			byteCount += 4u)
		{
			/* Spin while the FIFO is empty and no errors are flagged */
			do {
				rawIntStatus = DEV_REG(pDev, ISTAT);
				status = DEV_REG(pDev, STAT);
			} while((0u != (BITM_MSI_STAT_FIFOEMPTY & status)) && (0u == (rawIntStatus & XFRSTAT_ERR_MSK)));

			/* Copy the next word from the FIFO */
			*pBufferWord = DEV_REG(pDev, FIFO);
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_17_4:"We need address arithmetic here")
#endif
			++pBufferWord;
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif
		}
	}

	while (ADI_RSI_NOT_FINISHED == result)
	{
		result = checkXfrStatus(pDev, BITM_MSI_MSKISTAT_DTO);
	}

	return result;
}

/**
 * @brief Perform a blocking data write to the RSI.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  pBuffer       The buffer to provide the data.
 * @param [in]  nBlkSize      The block size for the transfer.
 * @param [in]  nBlkCnt       The number of blocks to transfer.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS         A card is present and not locked.
 *              - #ADI_RSI_FAILURE         An error occurred.
 *              - #ADI_RSI_CRC_ERROR       A CRC error occurred.
 *              - #ADI_RSI_TIMED_OUT       The operation timed out.
 *              - #ADI_RSI_NOT_FINISHED    The operation is still running.
 *
 * @sa adi_rsi_Read()
 */
ADI_RSI_RESULT
adi_rsi_Write(
	ADI_RSI_HANDLE         const hDevice,
	void                        *pBuffer,
	uint32_t               const nBlkSize,
	uint32_t               const nBlkCnt
)
{
	ADI_RSI_RESULT result = ADI_RSI_NOT_FINISHED;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;
	uint32_t *pBufferWord = pBuffer;
    uint32_t blockCount;

	uint32_t rawIntStatus = 0u;
	uint32_t status = 0u;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	/* For each block */
	for(blockCount = 0u;
		(blockCount < nBlkCnt) && (0u == (rawIntStatus & XFRSTAT_ERR_MSK));
		++blockCount)
	{
		uint32_t byteCount;

		/* For each 4-byte word in the block */
		for(byteCount = 0u;
			(byteCount < nBlkSize) && (0u == (rawIntStatus & XFRSTAT_ERR_MSK));
			byteCount += 4u)
		{
			/* Spin while the FIFO is full and no errors are flagged */
			do {
				rawIntStatus = DEV_REG(pDev, ISTAT);
				status = DEV_REG(pDev, STAT);
			} while((0u != (BITM_MSI_STAT_FIFOFULL & status)) && (0u == (rawIntStatus & XFRSTAT_ERR_MSK)));

			/* Copy the next word to the FIFO */
			DEV_REG(pDev, FIFO) = *pBufferWord;
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_17_4:"We need address arithmetic here")
#endif
			++pBufferWord;
#ifdef _MISRA_RULES
#pragma diag(pop)
#endif
		}
	}

	while (ADI_RSI_NOT_FINISHED == result)
	{
		result = checkXfrStatus(pDev, BITM_MSI_MSKISTAT_DTO);
	}

	return result;
}

/**
 * @brief Returns counts of transferred byted from RSI device.
 *
 * @param [in]  hDevice          RSI device handle to configure.
 * @param [in]  eCounter         Selects which byte counter value to return.
 * @param [out] pCount           Returns the selected byte count.
 *
 * @return  Status
 *          - #ADI_RSI_SUCCESS: Successfully returned the requested byte count.
 *          - Other error codes
 *
 */
ADI_RSI_RESULT  adi_rsi_GetByteCount(
    ADI_RSI_HANDLE const hDevice,
    ADI_RSI_BYTE_COUNTER eCounter,
    uint32_t            *pCount)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

    switch(eCounter)
    {
    case ADI_RSI_BYTE_COUNTER_CIU_CARD:
    	*pCount = DEV_REG(hDevice, TCBCNT);
    	break;
    case ADI_RSI_BYTE_COUNTER_BIU_FIFO:
    	*pCount = DEV_REG(hDevice, TCBCNT);
    	break;
    default:
    	result = ADI_RSI_INVALID_ARGUMENT;
    	break;
    }

    return result;
}

/**
 * @brief Enable selected RSI interrupts.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  mask          The set of interrupts to enable.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS           The specified interrupts were enabled.
 *
 * @sa adi_rsi_MaskInterrupts()
 */
ADI_RSI_RESULT adi_rsi_UnmaskInterrupts(
		ADI_RSI_HANDLE const hDevice,
		uint32_t       const mask
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	adi_osal_EnterCriticalRegion();

	DEV_REG(hDevice, IMSK) |= mask;

	adi_osal_ExitCriticalRegion();

	return result;
}

/**
 * @brief Disable selected RSI interrupts.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  mask          The set of interrupts to disable.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS           The specified interrupts were disabled.
 *
 * @sa adi_rsi_UnmaskInterrupts()
 */
ADI_RSI_RESULT adi_rsi_MaskInterrupts(
		ADI_RSI_HANDLE const hDevice,
		uint32_t       const mask
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	adi_osal_EnterCriticalRegion();

	DEV_REG(hDevice, IMSK) &= ~mask;

	adi_osal_ExitCriticalRegion();

	return result;
}


/**
 * @brief Enable selected RSI card events.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  mask          The set of card events to enable.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS           The specified card events were enabled.
 *
 * @sa adi_rsi_MaskCardEvents()
 */
ADI_RSI_RESULT adi_rsi_UnmaskCardEvents(
		ADI_RSI_HANDLE const hDevice,
		uint32_t       const mask
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

    struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;
	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */



	adi_osal_EnterCriticalRegion();

	pDev->cardEventMask |= mask;

	if (0u != (ADI_RSI_CARD_INSERTION & mask))
	{
		DEV_REG(pDev, IMSK) |= BITM_MSI_MSKISTAT_CD;
	}

	adi_osal_ExitCriticalRegion();

	return result;
}

/**
 * @brief Disable selected RSI card events.
 *
 * @param [in]  hDevice       Handle to the RSI device.
 * @param [in]  mask          The set of card events to disable.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS           The specified card events were disabled.
 *
 * @sa adi_rsi_UnmaskCardEvents()
 */
ADI_RSI_RESULT adi_rsi_MaskCardEvents(
		ADI_RSI_HANDLE const hDevice,
		uint32_t       const mask
	)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	struct ADI_RSI_DEVICE *pDev = (struct ADI_RSI_DEVICE *)hDevice;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */



	adi_osal_EnterCriticalRegion();

	pDev->cardEventMask &= ~mask;

	adi_osal_ExitCriticalRegion();

	return result;
}

/**
 * @brief Query the RSI interrupt status.
 *
 * @param [in]   hDevice       Handle to the RSI device.
 * @param [out]  pIntStatus    The returned interrupt status.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS     The interrupt status was returned.
 *
 * @sa adi_rsi_ClrInterruptStatus()
 */
ADI_RSI_RESULT adi_rsi_GetInterruptStatus(
	ADI_RSI_HANDLE const hDevice,
	uint32_t *pIntStatus
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	*pIntStatus = DEV_REG(hDevice, ISTAT);
	return result;
}

/**
 * @brief Clear specified RSI interrupt status flags.
 *
 * @param [in]   hDevice       Handle to the RSI device.
 * @param [in]   clrMask       The set of interrupts to clear.
 *
 * @return      Status
 *              - #ADI_RSI_SUCCESS      The specified interrupts were cleared.
 *
 * @sa adi_rsi_GetInterruptStatus()
 */
ADI_RSI_RESULT adi_rsi_ClrInterruptStatus(
	ADI_RSI_HANDLE const hDevice,
	uint32_t       const clrMask
)
{
	ADI_RSI_RESULT result = ADI_RSI_SUCCESS;

	/* debug build only */
#if defined (ADI_DEBUG)
    /* validate the given handle */
	if (ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
	{
	    return ADI_RSI_INVALID_HANDLE;
	}
#endif /* ADI_DEBUG */

	DEV_REG(hDevice, ISTAT) = clrMask;
	return result;
}

/**
 * @brief   This function gets the RSI's accumulated hardware error(s) (effectively the  MSI status register).
 *          pHwError contains the field bits of the MSI status register
 *
 * @param   [in]  hDevice   Handle of the RSI device
 * @param   [in]  pError    Pointer to a variable containing the RSI HW error.
 *
 * @return        Status
 *                - #ADI_RSI_SUCCESS          The device is successfully opened for the given instance.
 *                - #ADI_RSI_INVALID_HANDLE   An invalid device handle.
 *
 * @note    This API will only work if application has enabled HW Error interrupts using adi_rsi_UnmaskInterrupts() API. Otherwise, adi_rsi_Open()
 *          and adi_rsi_Write() will return the HW error status errors, if there are any.
 *
 */
ADI_RSI_RESULT adi_rsi_GetHWErrorStatus (ADI_RSI_HANDLE const hDevice,
                                         uint32_t *pHwError)
{

    /* Pointer to device instance */
    struct ADI_RSI_DEVICE *pDev = hDevice;

/* debug build only */
#if defined (ADI_DEBUG)

    /* validate the given handle */
    if(ValidateRsiHandle(hDevice) != ADI_RSI_SUCCESS)
    {
        return ADI_RSI_INVALID_HANDLE;
    }

	 /* Check if the given pointer is valid */
    if(pHwError == NULL)
    {
        return ADI_RSI_INVALID_ARGUMENT;
    }

#endif

    *pHwError = pDev->rsiStatusEvent;
     adi_osal_EnterCriticalRegion();
     pDev->rsiStatusEvent  = ADI_RSI_HW_ERR_NONE;
     adi_osal_ExitCriticalRegion();
     return ADI_RSI_SUCCESS;
}


/**
 Function:       ValidateRsiHandle

 Description:    This function validates the RSI handle.
 *
 */

#ifdef ADI_DEBUG
static ADI_RSI_RESULT ValidateRsiHandle(ADI_RSI_HANDLE * const hDevice)
{
    /* index counter */
    uint8_t i;
    /* return code */
    ADI_RSI_RESULT Result = ADI_RSI_INVALID_HANDLE;

    /* all RSI device instances */
    for (i = 0u; i < (uint8_t)NUM_RSI_DEVICES; i++)
    {
#if !defined(ADI_CODE_IN_ROM)
    	if (hDevice == (ADI_RSI_HANDLE) &s_rsiDriver[i])
#else
      	if (hDevice == (ADI_RSI_HANDLE) &pram_for_rom_Code->s_rsiDriver[i])
#endif

        {
            Result = ADI_RSI_SUCCESS; /* Found a valid device */
            break;
        }
    }
    /* Return */
    return Result;
}
#endif
/*@}*/
#endif /* _ADI_RSI_SYNOPSYS_3891_0_C_ */
