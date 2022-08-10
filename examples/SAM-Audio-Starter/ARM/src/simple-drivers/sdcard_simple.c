/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

/*!
 * @brief     Simple, efficient, RTOS or bare metal master mode SDCARD driver
 *
 *   This SDCARD driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Fully protected multi-threaded device transfers
 *
 * @file      sdcard_simple.h
 * @version   1.0.1
 * @copyright 2021 Analog Devices, Inc.  All rights reserved.
 *
*/
#include <string.h>
#include <drivers/rsi/adi_rsi.h>
#include <sys/platform.h>

#include "sdcard_simple.h"

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define SDCARD_ENTER_CRITICAL()  taskENTER_CRITICAL()
    #define SDCARD_EXIT_CRITICAL()   taskEXIT_CRITICAL()
    #define SDCARD_LOCK()            xSemaphoreTake(sdcard->portLock, portMAX_DELAY);
    #define SDCARD_UNLOCK()          xSemaphoreGive(sdcard->portLock);
#else
    #define SDCARD_ENTER_CRITICAL()
    #define SDCARD_EXIT_CRITICAL()
    #define SDCARD_LOCK()
    #define SDCARD_UNLOCK()
#endif


/* define shorter forms of the ADI transfer types */
#define TRANS_READ   (ADI_RSI_TRANSFER_DMA_BLCK_READ)
#define TRANS_WRITE  (ADI_RSI_TRANSFER_DMA_BLCK_WRITE)
#define TRANS_NONE   (ADI_RSI_TRANSFER_NONE)

/* define shorter forms of the ADI response types */
#define RESPONSE_NONE  (ADI_RSI_RESPONSE_TYPE_NONE)
#define RESPONSE_LONG  (ADI_RSI_RESPONSE_TYPE_LONG)
#define RESPONSE_SHORT (ADI_RSI_RESPONSE_TYPE_SHORT)

/* define shorter forms of the ADI flags */
#define CRCDIS  (ADI_RSI_CMDFLAG_CRCDIS)
#define CHKBUSY (ADI_RSI_CMDFLAG_CHKBUSY)

#define NO_RESPONSE    0x00
#define R1_RESPONSE    0x40
#define R1B_RESPONSE   0x40
#define R2_RESPONSE    0xC0
#define R3_RESPONSE    0x40
#define R6_RESPONSE    0x40
#define R7_RESPONSE    0x40

#define SD_MMC_CMD0    0
#define SD_MMC_CMD2    2
#define SD_MMC_CMD3    3
#define SD_MMC_CMD7    7
#define SD_MMC_CMD8    8
#define SD_MMC_CMD9    9
#define SD_MMC_CMD12   12
#define SD_MMC_CMD17   17
#define SD_MMC_CMD18   18
#define SD_MMC_CMD24   24
#define SD_MMC_CMD25   25
#define SD_MMC_CMD55   55
#define SD_ACMD6       6
#define SD_ACMD41      41
#define SD_ACMD42      42
#define SD_ACMD13      13

#define SD_MMC_CMD_GO_IDLE_STATE        (SD_MMC_CMD0 | NO_RESPONSE)
#define SD_MMC_CMD_ALL_SEND_CID         (SD_MMC_CMD2 | R2_RESPONSE)
#define SD_CMD_SEND_RELATIVE_ADDR       (SD_MMC_CMD3 | R6_RESPONSE)
#define SD_MMC_CMD_SELECT_DESELECT_CARD (SD_MMC_CMD7 | R1B_RESPONSE)
#define SD_CMD_SEND_IF_COND             (SD_MMC_CMD8 | R7_RESPONSE)
#define SD_MMC_CMD_SEND_CSD             (SD_MMC_CMD9 | R2_RESPONSE)
#define SD_MMC_CMD_STOP_TRANSMISSION    (SD_MMC_CMD12 | R1B_RESPONSE)
#define SD_MMC_CMD_READ_BLOCK           (SD_MMC_CMD17 | R1_RESPONSE)
#define SD_MMC_CMD_READ_MULTIPLE_BLOCK  (SD_MMC_CMD18 | R1_RESPONSE)
#define SD_MMC_CMD_WRITE_BLOCK          (SD_MMC_CMD24 | R1_RESPONSE)
#define SD_MMC_CMD_WRITE_MULTIPLE_BLOCK (SD_MMC_CMD25 | R1_RESPONSE)
#define SD_MMC_CMD_APP_CMD              (SD_MMC_CMD55 | R1_RESPONSE)
#define SD_CMD_SET_BUS_WIDTH            (SD_ACMD6 | R1_RESPONSE)
#define SD_CMD_DISCONNECT_DAT3_PULLUP   (SD_ACMD42 | R1_RESPONSE)
#define SD_CMD_GET_OCR_VALUE            (SD_ACMD41 | R3_RESPONSE)
#define SD_CMD_GET_MEMORY_STATUS        (SD_ACMD13 | R1_RESPONSE)

/****************************************************
 *  SD OCR Register Bit Masks                       *
 ****************************************************/
#define SD_OCR_CARD_CAPACITY_STATUS (1<<30)
#define SD_OCR_CARD_POWER_UP_STATUS (1<<31)

#define CARD_STATUS_READY_FOR_DATA  (1 << 8)

typedef enum _SDCARD_CSD_STRUCTURE {
    SDCARD_CSD_STRUCTURE_VERSION_1_0          = 0,
    SDCARD_CSD_STRUCTURE_VERSION_2_0          = 1,
    SDCARD_CSD_STRUCTURE_VERSION_RESERVED0    = 2,
    SDCARD_CSD_STRUCTURE_VERSION_RESERVED1    = 3
} SDCARD_CSD_STRUCTURE;

/* SD transfer units and multipliers */
const unsigned long SDCARD_TRANSFER_UNIT[7] = {
    10, 100, 1000, 10000, 0, 0, 0
};
const unsigned long SDCARD_TRANSFER_MULTIPLIER[16] = {
    0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};

struct sSDCARD {
    uint8_t alignedData[512];

    bool open;

    ADI_RSI_HANDLE rsiHandle;

#ifdef FREE_RTOS
    SemaphoreHandle_t portLock;
    SemaphoreHandle_t cmdBlock;
#else
    volatile bool sdcardDone;
#endif

    SDCARD_SIMPLE_TYPE type;
    uint32_t rca;
    uint64_t capacity;

    SDCARD_CSD_STRUCTURE csdStruct;

} __attribute__((aligned(4)));


/* SDCARD port context containers */
static sSDCARD sdcardContext[SDCARD_END];

SDCARD_SIMPLE_RESULT sdcard_init(void)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    uint8_t port;
    sSDCARD *sdcard;

    memset(sdcardContext, 0, sizeof(sdcardContext));

    for (port = SDCARD0; port < SDCARD_END; port++) {

        sdcard = &sdcardContext[port];

#ifdef FREE_RTOS
        sdcard->portLock = xSemaphoreCreateMutex();
        if (sdcard->portLock == NULL) {
            result = SDCARD_SIMPLE_ERROR;
        }
        sdcard->cmdBlock = xSemaphoreCreateCounting(1, 0);
        if (sdcard->cmdBlock == NULL) {
            result = SDCARD_SIMPLE_ERROR;
        }
#endif

        sdcard->open = false;

    }

    return(result);
}

SDCARD_SIMPLE_RESULT sdcard_deinit(void)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    uint8_t port;
    sSDCARD *sdcard;

    for (port = SDCARD0; port < SDCARD_END; port++) {

        sdcard = &sdcardContext[port];

#ifdef FREE_RTOS
        if (sdcard->cmdBlock) {
            vSemaphoreDelete(sdcard->cmdBlock);
            sdcard->cmdBlock = NULL;
        }
        if (sdcard->portLock) {
            vSemaphoreDelete(sdcard->portLock);
            sdcard->portLock = NULL;
        }
#endif

    }

    return(result);
}

static void sdcard_event(void *usr, uint32_t event, void *arg)
{
    sSDCARD *sdcard = (sSDCARD *)usr;
    switch (event) {
        case ADI_RSI_EVENT_INTERRUPT:
        {
            BaseType_t contextSwitch = pdFALSE;
            uint32_t interrupts = *((uint32_t *)arg);
            if (interrupts & BITM_MSI_MSKISTAT_CMDDONE) {
                xSemaphoreGiveFromISR(sdcard->cmdBlock, &contextSwitch);
                if (contextSwitch == pdTRUE) {
                    portYIELD_FROM_ISR(contextSwitch);
                }
            }
            break;
        }
        case ADI_RSI_EVENT_CARD_CHANGE:
        {
            int event = *((int *)arg);
            if (event == 1) {
                /* Inserted */
            } else if (event == 0) {
                /* Removed */
            } else {
                /* Unknown */
            }
            break;
        }
        default:
            break;
    }
}

SDCARD_SIMPLE_RESULT sdcard_open(SDCARD_SIMPLE_PORT port, sSDCARD **sdcardHandle)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_RESULT rsiResult;
    sSDCARD *sdcard;

    if (port >= SDCARD_END) {
        return(SDCARD_SIMPLE_INVALID_PORT);
    }

    sdcard = &sdcardContext[port];

    SDCARD_LOCK();

    if (sdcard->open == true) {
        result = SDCARD_SIMPLE_PORT_BUSY;
    }

    rsiResult = adi_rsi_Open(port, &sdcard->rsiHandle);
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    if (result == SDCARD_SIMPLE_SUCCESS) {
        rsiResult = adi_rsi_RegisterCallback(sdcard->rsiHandle,
            sdcard_event, sdcard);
        rsiResult = adi_rsi_UnmaskInterrupts(sdcard->rsiHandle,
            BITM_MSI_MSKISTAT_CMDDONE);
        rsiResult = adi_rsi_UnmaskCardEvents (sdcard->rsiHandle,
            ADI_RSI_CARD_INSERTION | ADI_RSI_CARD_REMOVAL);
        *sdcardHandle = sdcard;
        sdcard->open = true;
    } else {
        *sdcardHandle = NULL;
    }

    SDCARD_UNLOCK();

    return(result);
}

SDCARD_SIMPLE_RESULT sdcard_close(sSDCARD **sdcardHandle)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_RESULT rsiResult;
    sSDCARD *sdcard = *sdcardHandle;

    if (*sdcardHandle == NULL) {
        return (SDCARD_SIMPLE_ERROR);
    }

    SDCARD_LOCK();

    rsiResult = adi_rsi_Close(sdcard->rsiHandle);

    sdcard->open = false;
    *sdcardHandle = NULL;

    SDCARD_UNLOCK();

    return(result);
}

SDCARD_SIMPLE_RESULT sdcard_present(sSDCARD *sdcard)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_RESULT rsiResult;

    SDCARD_LOCK();

    rsiResult = adi_rsi_IsCardPresent(sdcard->rsiHandle);

    SDCARD_UNLOCK();

    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);
}

/***********************************************************************
 * Send Command
 ***********************************************************************/
static ADI_RSI_RESULT sdcard_SendCommand(sSDCARD *sdcard,
    uint16_t command, uint32_t argument,
    ADI_RSI_RESPONSE_TYPE resp, ADI_RSI_TRANSFER transfer, uint32_t flags)
{
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT result;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    /* Comment out, the RSI driver does this on all commands */
#if 0
    /* Check busy on all commands but status and stop commands */
    if ( (command != SD_CMD_GET_MEMORY_STATUS) &&
         (command != SD_MMC_CMD_STOP_TRANSMISSION) ) {
        flags |= ADI_RSI_CMDFLAG_CHKBUSY;
    }
#endif

    result = adi_rsi_SetDataMode(rsiHandle, transfer, ADI_RSI_CEATA_MODE_NONE);
    result = adi_rsi_SendCommand(rsiHandle, command, argument, flags, resp);

    /* Wait up to 1 second for the command to complete */
    rtosResult = xSemaphoreTake(sdcard->cmdBlock, pdMS_TO_TICKS(1000));
    if (rtosResult != pdTRUE) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return result;
}

/***********************************************************************
 * Ready for data
 ***********************************************************************/
SDCARD_SIMPLE_RESULT sdcard_readyForData(sSDCARD *sdcard)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult;
    uint32_t response;

    if ((sdcard == NULL) || (sdcard->type == SDCARD_UNUSABLE_CARD)) {
        return(SDCARD_SIMPLE_ERROR);
    }

    /* TODO: 1 second timeout */
    while (1) {
        rsiResult = sdcard_SendCommand(sdcard,
            SD_CMD_GET_MEMORY_STATUS, sdcard->rca, RESPONSE_SHORT, TRANS_NONE, CRCDIS
        );
        if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };
        adi_rsi_GetShortResponse(rsiHandle, &response);
        if (response & CARD_STATUS_READY_FOR_DATA) {
            break;
        }
    }

abort:
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);
}

/***********************************************************************
 * Write
 ***********************************************************************/
static SDCARD_SIMPLE_RESULT
sdcard_submitDataTransfer(sSDCARD *sdcard,
    uint16_t cmd, bool read, void *data, uint32_t sector, uint32_t count)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult = ADI_RSI_SUCCESS;
    ADI_RSI_TRANSFER transfer;

    result = sdcard_readyForData(sdcard);
    if (result != SDCARD_SIMPLE_SUCCESS) {
        goto abort;
    }

    rsiResult = adi_rsi_SetBlockCntAndLen(rsiHandle, count, 512);
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };
    if (read) {
        rsiResult = adi_rsi_SubmitRxBuffer(rsiHandle, data, 512, count);
        transfer = TRANS_READ;
    } else {
        rsiResult = adi_rsi_SubmitTxBuffer(rsiHandle, data, 512, count);
        transfer = TRANS_WRITE;
    }
    rsiResult = sdcard_SendCommand(sdcard,
        cmd, sector, RESPONSE_SHORT, transfer, CRCDIS
    );

abort:
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);
}

static SDCARD_SIMPLE_RESULT
sdcard_writeUnaligned(sSDCARD *sdcard, void *data, uint32_t sector, uint32_t count)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult = ADI_RSI_SUCCESS;
    void *txBuf = NULL;
    uint8_t *inData;
    unsigned i;

    i = 0;
    inData = (uint8_t *)data;

    SDCARD_LOCK();

    do {

        memcpy(sdcard->alignedData, inData, 512);
        result = sdcard_submitDataTransfer(sdcard,
            SD_MMC_CMD_WRITE_BLOCK, false, sdcard->alignedData, sector, 1
        );
        if (result != SDCARD_SIMPLE_SUCCESS) { goto abort; };

        /* This call blocks until complete */
        rsiResult = adi_rsi_GetTxBuffer(rsiHandle, &txBuf);
        if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };

        count--; sector++; i++; inData += 512;

    } while (count);

abort:
    SDCARD_UNLOCK();
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);

}

#define MAX_RSI_TRANSFER_COUNT (ADI_RSI_MAX_TRANSFER_BYTES / 512)

SDCARD_SIMPLE_RESULT sdcard_write(sSDCARD *sdcard, void *data, uint32_t sector, uint32_t count)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult = ADI_RSI_SUCCESS;
    uint32_t transferCount;
    void *txBuf = NULL;
    uint16_t cmd;

    if ((sdcard == NULL) || (sdcard->type == SDCARD_UNUSABLE_CARD)) {
        return(SDCARD_SIMPLE_ERROR);
    }

    if (sdcard->type != SDCARD_TYPE_SD_V2X_HIGH_CAPACITY) {
        sector *= 512;
    }

    /* Must send unaligned data through an aligned DMA buffer */
    if ((uintptr_t)data & 0x3) {
        result = sdcard_writeUnaligned(sdcard, data, sector, count);
        return(result);
    }

    SDCARD_LOCK();

    do {

        transferCount = (count > MAX_RSI_TRANSFER_COUNT) ?
            MAX_RSI_TRANSFER_COUNT : count;

        cmd = (transferCount > 1) ?
            SD_MMC_CMD_WRITE_MULTIPLE_BLOCK : SD_MMC_CMD_WRITE_BLOCK;

        result = sdcard_submitDataTransfer(sdcard,
            cmd, false, data, sector, transferCount
        );
        if (result != SDCARD_SIMPLE_SUCCESS) { goto abort; };

        /* This call blocks until complete */
        rsiResult = adi_rsi_GetTxBuffer(rsiHandle, &txBuf);
        if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };

        if (transferCount > 1) {
            rsiResult = sdcard_SendCommand(sdcard,
                SD_MMC_CMD_STOP_TRANSMISSION, 0, RESPONSE_SHORT, TRANS_NONE, CRCDIS
            );
            if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };
        }

        count -= transferCount; sector += transferCount;

    } while (count);

abort:
    SDCARD_UNLOCK();
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);
}

/***********************************************************************
 * Read
 ***********************************************************************/
static SDCARD_SIMPLE_RESULT
sdcard_readUnaligned(sSDCARD *sdcard, void *data, uint32_t sector, uint32_t count)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult = ADI_RSI_SUCCESS;
    void *txBuf = NULL;
    uint8_t *inData;
    unsigned i;

    i = 0;
    inData = (uint8_t *)data;

    SDCARD_LOCK();

    do {

        result = sdcard_submitDataTransfer(sdcard,
            SD_MMC_CMD_READ_BLOCK, true, sdcard->alignedData, sector, 1
        );
        if (result != SDCARD_SIMPLE_SUCCESS) { goto abort; };

        /* This call blocks until complete */
        rsiResult = adi_rsi_GetRxBuffer(rsiHandle, &txBuf);
        if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };

        memcpy(inData, sdcard->alignedData, 512);

        count--; sector++; i++; inData += 512;

    } while (count);

abort:
    SDCARD_UNLOCK();
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);

}

SDCARD_SIMPLE_RESULT sdcard_read(sSDCARD *sdcard, void *data, uint32_t sector, uint32_t count)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult = ADI_RSI_SUCCESS;
    uint32_t transferCount;
    void *rxBuf = NULL;
    uint16_t cmd;

    if ((sdcard == NULL) || (sdcard->type == SDCARD_UNUSABLE_CARD)) {
        return(SDCARD_SIMPLE_ERROR);
    }

    if (sdcard->type != SDCARD_TYPE_SD_V2X_HIGH_CAPACITY) {
        sector *= 512;
    }

    /* Must send unaligned data through an aligned buffer */
    if ((uintptr_t)data & 0x3) {
        result = sdcard_readUnaligned(sdcard, data, sector, count);
        return(result);
    }

    SDCARD_LOCK();

    do {

        transferCount = (count > MAX_RSI_TRANSFER_COUNT) ?
            MAX_RSI_TRANSFER_COUNT : count;

        cmd = (transferCount > 1) ?
            SD_MMC_CMD_READ_MULTIPLE_BLOCK : SD_MMC_CMD_READ_BLOCK;

        result = sdcard_submitDataTransfer(sdcard,
            cmd, true, data, sector, transferCount
        );
        if (result != SDCARD_SIMPLE_SUCCESS) { goto abort; };

        /* This call blocks until complete */
        rsiResult = adi_rsi_GetRxBuffer(rsiHandle, &rxBuf);

        if (transferCount > 1) {
            rsiResult = sdcard_SendCommand(sdcard,
                SD_MMC_CMD_STOP_TRANSMISSION, 0, RESPONSE_SHORT, TRANS_NONE, CRCDIS
            );
            if (rsiResult != ADI_RSI_SUCCESS) { goto abort; };
        }

        count -= transferCount; sector += transferCount;

    } while (count);

abort:
    SDCARD_UNLOCK();

    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
    }

    return(result);
}

/***********************************************************************
 * Identify / Init
 ***********************************************************************/
static void sdcard_SetSpeed(sSDCARD *sdcard,
    uint32_t sclk, uint32_t cardSpeed)
{
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult;
    uint32_t clkDiv = 0;
    uint32_t rsiFreq;

    rsiFreq = sclk;
    while(rsiFreq > cardSpeed) {
        clkDiv++;
        rsiFreq = sclk / (clkDiv * 2);
    }
    rsiResult = adi_rsi_SetClock(rsiHandle, clkDiv * 2, ADI_RSI_CLK_MODE_ENABLE);
}

/*
 *  Verifies if the device is an SD Version 2.0 or later device based on the response
 */
static SDCARD_SIMPLE_TYPE sdcard_IdentifyV2(sSDCARD *sdcard, uint32_t response)
{
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT result;
    uint32_t ocr = 0x00000000;

    SDCARD_SIMPLE_TYPE type = SDCARD_UNUSABLE_CARD;

    if ((response & 0x000001FF) == 0x000001AA) {
        /* TODO: 1 second timeout */
        do {
            result = sdcard_SendCommand(sdcard,
                SD_MMC_CMD_APP_CMD, 0, RESPONSE_SHORT, TRANS_NONE, CRCDIS
            );
            adi_rsi_GetShortResponse(rsiHandle, &response);

            result = sdcard_SendCommand(sdcard,
                SD_CMD_GET_OCR_VALUE, 0x40FF8000, RESPONSE_SHORT, TRANS_NONE, CRCDIS
            );
            adi_rsi_GetShortResponse(rsiHandle, &ocr);

        } while ((ocr & SD_OCR_CARD_POWER_UP_STATUS) == 0);

        if (ocr & SD_OCR_CARD_CAPACITY_STATUS) {
            type = SDCARD_TYPE_SD_V2X_HIGH_CAPACITY;
        } else {
            type = SDCARD_TYPE_SD_V2X;
        }
    }

    return type;
}

static SDCARD_SIMPLE_RESULT sdcard_Start(sSDCARD *sdcard)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;
    ADI_RSI_HANDLE rsiHandle = sdcard->rsiHandle;
    ADI_RSI_RESULT rsiResult;
    uint32_t cardResp;
    uint32_t cid[4];
    uint32_t csd[4];
    uint32_t rca;

    /* Reset card info */
    sdcard->type = SDCARD_UNUSABLE_CARD;
    sdcard->capacity = 0;

    /* Set speed to 400KHz for identification */
    sdcard_SetSpeed(sdcard, SDCLK, 400000);

    /* Set 1 bit bus width transfer for identification */
    adi_rsi_SetBusWidth(rsiHandle, 1);

    /* Put card in idle state (reset) */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_MMC_CMD_GO_IDLE_STATE, 0, RESPONSE_NONE, TRANS_NONE, 0
    );

    /* Check SDv2 voltage range */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_CMD_SEND_IF_COND, 0x000001AA, RESPONSE_SHORT, TRANS_NONE, CRCDIS
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }

    /* SDv2 card */
    adi_rsi_GetShortResponse(rsiHandle, &cardResp);
    sdcard->type = sdcard_IdentifyV2(sdcard, cardResp);
    if (sdcard->type == SDCARD_UNUSABLE_CARD) {
        rsiResult = ADI_RSI_FAILURE;
        goto abort;
    }

    /* Get CID */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_MMC_CMD_ALL_SEND_CID, 0, RESPONSE_LONG, TRANS_NONE, 0
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    adi_rsi_GetLongResponse(rsiHandle, cid);

    /* Get RCA */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_CMD_SEND_RELATIVE_ADDR, 0, RESPONSE_SHORT, TRANS_NONE, 0
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    adi_rsi_GetShortResponse(rsiHandle, &rca);
    rca &= 0xFFFF0000;
    sdcard->rca = rca;

    /* Get CSD */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_MMC_CMD_SEND_CSD, rca, RESPONSE_LONG, TRANS_NONE, CRCDIS
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    adi_rsi_GetLongResponse(rsiHandle, csd);

    /* Calculate the max clock speed */
    uint8_t tran_speed = csd[0] & 0x000000FF;
    uint32_t unit, mul, maxClock;
    unit = SDCARD_TRANSFER_UNIT[tran_speed & 0x7];
    mul = SDCARD_TRANSFER_MULTIPLIER[(tran_speed >> 3) & 0xF];
    maxClock = unit * mul * 1000;

    /* Calculate the size */
    sdcard->csdStruct = (csd[0] & 0xC0000000) >> 30;
    if (sdcard->csdStruct == SDCARD_CSD_STRUCTURE_VERSION_1_0) {
        uint32_t read_bl_len = (csd[1] & 0x000F0000) >> 16;
        uint32_t c_size = ((csd[1] & 0x000003FF) << 2) | ((csd[2] & 0xC0000000) >> 30);
        uint32_t c_size_mult = (csd[2] & 0x00038000) >> 15;
        uint64_t mult = c_size_mult << 8;
        uint64_t block_nr = (c_size + 1) * mult;
        uint64_t block_len = read_bl_len << 12;
        sdcard->capacity = mult * block_nr * block_len;
    } else if (sdcard->csdStruct == SDCARD_CSD_STRUCTURE_VERSION_2_0) {
        uint64_t c_size;
        c_size = ((csd[1] & 0x0000003F) << 16) | ((csd[2] & 0xFFFF0000) >> 16);
        sdcard->capacity = (c_size + 1) * 512 * 1024;
    } else {
        sdcard->type = SDCARD_UNUSABLE_CARD;
        goto abort;
    }

    /* Select the card */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_MMC_CMD_SELECT_DESELECT_CARD, rca, RESPONSE_SHORT, TRANS_NONE, CRCDIS
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }

    /* Increase speed */
    sdcard_SetSpeed(sdcard, SDCLK, maxClock);

    /* Set 4-bit bus width */
    rsiResult = sdcard_SendCommand(sdcard,
        SD_MMC_CMD_APP_CMD, rca, RESPONSE_SHORT, TRANS_NONE, CRCDIS
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    rsiResult = sdcard_SendCommand(sdcard,
        SD_CMD_DISCONNECT_DAT3_PULLUP, 0, RESPONSE_SHORT, TRANS_NONE, 0
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    rsiResult = sdcard_SendCommand(sdcard,
        SD_MMC_CMD_APP_CMD, rca, RESPONSE_SHORT, TRANS_NONE, CRCDIS
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    rsiResult = sdcard_SendCommand(sdcard,
        SD_CMD_SET_BUS_WIDTH, 2, RESPONSE_SHORT, TRANS_NONE, CRCDIS
    );
    if (rsiResult != ADI_RSI_SUCCESS) { goto abort; }
    rsiResult = adi_rsi_SetBusWidth(rsiHandle, 4);

abort:
    if (rsiResult != ADI_RSI_SUCCESS) {
        result = SDCARD_SIMPLE_ERROR;
        sdcard->type = SDCARD_UNUSABLE_CARD;
        sdcard->capacity = 0;
    }
    return(result);
}

/***********************************************************************
 * Start/Stop Functions
 ***********************************************************************/
SDCARD_SIMPLE_RESULT sdcard_start(sSDCARD *sdcard)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;

    if (sdcard == NULL) {
        return(SDCARD_SIMPLE_ERROR);
    }

    SDCARD_LOCK();
    result = sdcard_Start(sdcard);
    SDCARD_UNLOCK();

    return(result);
}

SDCARD_SIMPLE_RESULT sdcard_stop(sSDCARD *sdcard)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;

    if (sdcard == NULL) {
        return(SDCARD_SIMPLE_ERROR);
    }

    SDCARD_LOCK();
    sdcard->type = SDCARD_UNUSABLE_CARD;
    sdcard->capacity = 0;
    SDCARD_UNLOCK();

    return(result);
}

/***********************************************************************
 * Info Function
 ***********************************************************************/
SDCARD_SIMPLE_RESULT sdcard_info(sSDCARD *sdcard, SDCARD_SIMPLE_INFO *info)
{
    SDCARD_SIMPLE_RESULT result = SDCARD_SIMPLE_SUCCESS;

    if (sdcard == NULL) {
        return(SDCARD_SIMPLE_ERROR);
    }

    SDCARD_LOCK();
    if (info) {
        info->type = sdcard->type;
        info->capacity = sdcard->capacity;
    }
    SDCARD_UNLOCK();
    return(result);
}
