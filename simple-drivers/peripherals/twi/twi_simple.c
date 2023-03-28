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

/*
 * TWI Simple provides a simple, efficient, RTOS or bare metal TWI driver.
 *
 * This driver supports:
 *  - FreeRTOS or no RTOS main-loop modes
 *  - Fully protected multi-threaded TWI transfers
 *  - Up to 64k read and write transfers
 *  - Blocking transfers
 *
 */
#include <string.h>

#include <sys/anomaly_macros_rtl.h>
#include <services/int/adi_int.h>

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define TWI_ENTER_CRITICAL()  taskENTER_CRITICAL()
    #define TWI_EXIT_CRITICAL()   taskEXIT_CRITICAL()
#else
    #define TWI_ENTER_CRITICAL()
    #define TWI_EXIT_CRITICAL()
#endif

#include "twi_simple.h"

#define TWI_SIMPLE_IRQ_ENABLE ( \
    ENUM_TWI_ISTAT_TXSERV_YES |  ENUM_TWI_ISTAT_RXSERV_YES |   \
    ENUM_TWI_ISTAT_MCOMP_YES )

#define TWI_SIMPLE_IRQ_MASK (    \
    BITM_TWI_ISTAT_TXSERV | BITM_TWI_ISTAT_RXSERV |     \
    BITM_TWI_ISTAT_MCOMP )

#define TWI_SIMPLE_FLUSH_FIFOS ( \
    ENUM_TWI_FIFOCTL_TXFLUSH | ENUM_TWI_FIFOCTL_RXFLUSH )

#define TWI_SIMPLE_ERRORS (               \
    BITM_TWI_MSTRSTAT_LOSTARB | BITM_TWI_MSTRSTAT_ANAK |       \
    BITM_TWI_MSTRSTAT_DNAK    | BITM_TWI_MSTRSTAT_BUFRDERR  |  \
    BITM_TWI_MSTRSTAT_BUFWRERR )

#define ENUM_TWI_FIFOSTAT_TX_FIFO_FULL  (3u << BITP_TWI_FIFOSTAT_TXSTAT)
#define TX_FIFO_FULL ((*twi->pREG_TWI_FIFOSTAT & BITM_TWI_FIFOSTAT_TXSTAT) == ENUM_TWI_FIFOSTAT_TX_FIFO_FULL)
#define ENUM_TWI_FIFOSTAT_RX_FIFO_EMPTY  (0u << BITP_TWI_FIFOSTAT_RXSTAT)
#define RX_FIFO_EMPTY ((*twi->pREG_TWI_FIFOSTAT & BITM_TWI_FIFOSTAT_RXSTAT) == ENUM_TWI_FIFOSTAT_RX_FIFO_EMPTY)

#ifndef TWI_SIMPLE_PRESCALE
#define TWI_SIMPLE_PRESCALE       (SCLK0 / 10000000u)
#endif

#ifndef TWI_SIMPLE_DUTYCYCLE
#define TWI_SIMPLE_DUTYCYCLE      (50u)
#endif

#define TWI_MAX_DCNT              (0xFF - 1)

typedef enum TWI_SIMPLE_XFER_TYPE {
   TWI_SIMPLE_READ,
   TWI_SIMPLE_WRITE,
   TWI_SIMPLE_WRITEREAD,
   TWI_SIMPLE_WRITEWRITE,
} TWI_SIMPLE_XFER_TYPE;

struct sTWI {

    // Memory-mapped control registers used to program TWI peripheral
    volatile uint16_t *pREG_TWI_CTL;
    volatile uint16_t *pREG_TWI_CLKDIV;
    volatile uint16_t *pREG_TWI_FIFOCTL;
    volatile uint16_t *pREG_TWI_FIFOSTAT;
    volatile uint16_t *pREG_TWI_MSTRADDR;
    volatile uint16_t *pREG_TWI_IMSK;
    volatile uint16_t *pREG_TWI_ISTAT;
    volatile uint16_t *pREG_TWI_MSTRSTAT;
    volatile uint16_t *pREG_TWI_MSTRCTL;

    volatile uint16_t *pREG_TWI_TXDATA8;
    volatile uint16_t *pREG_TWI_TXDATA16;
    volatile uint16_t *pREG_TWI_RXDATA8;
    volatile uint16_t *pREG_TWI_RXDATA16;

    uint16_t TWI_DATA_IRQ_ID;
    uint16_t DEFAULT_TWI_MSTRCTL;

    bool open;

    uint8_t address;
    const uint8_t *sdata;
    uint16_t slen;
    uint8_t *rdata;
    uint16_t rlen;
    uint8_t longXfer;
    TWI_SIMPLE_XFER_TYPE xferType;
    TWI_SIMPLE_XFER_TYPE xferState;

#ifdef FREE_RTOS
    SemaphoreHandle_t portLock;
    SemaphoreHandle_t portBlock;
#else
    volatile bool twiDone;
#endif

};

/* TWI port context containers */
static sTWI twiContext[TWI_END];

/**************************************************************************
 * TWI port configuration functions
 **************************************************************************/
static void twi_data_irq(uint32_t id, void *usrPtr);

static void _twi_setSpeed(sTWI *twi, TWI_SIMPLE_SPEED speed)
{
    uint16_t clkPeriod, clkHi, clkLo;

    /* Set the clock */
    clkPeriod = 10000u / speed;
    clkHi = ((clkPeriod * TWI_SIMPLE_DUTYCYCLE) + 50u)/100u;
    clkLo = clkPeriod - clkHi;
    *twi->pREG_TWI_CLKDIV = (clkHi << BITP_TWI_CLKDIV_CLKHI) | clkLo;

    /* Set fast mode if necessary */
    if (speed > TWI_SIMPLE_SPEED_100) {
        twi->DEFAULT_TWI_MSTRCTL |= ENUM_TWI_MSTRCTL_FAST;
    }

}

TWI_SIMPLE_RESULT twi_setSpeed(sTWI *twiHandle, TWI_SIMPLE_SPEED speed)
{
    sTWI *twi = twiHandle;
    TWI_SIMPLE_RESULT result;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    result = TWI_SIMPLE_SUCCESS;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(twi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
        return result;
    }
#endif

    if (result == TWI_SIMPLE_SUCCESS) {
        _twi_setSpeed(twi, speed);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(twi->portLock);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
    }
#endif

    return(result);
}

static uint16_t twi_begin(sTWI *twi)
{
    uint32_t mstrctl;
    uint32_t len;

    /* Flush the Rx and Tx FIFOs */
    *twi->pREG_TWI_FIFOCTL |= TWI_SIMPLE_FLUSH_FIFOS;
    *twi->pREG_TWI_FIFOCTL &= ~TWI_SIMPLE_FLUSH_FIFOS;

    /* Set the slave address */
    *twi->pREG_TWI_MSTRADDR = twi->address;

    /* Set up the control word */
    twi->longXfer = 0;
    mstrctl = twi->DEFAULT_TWI_MSTRCTL;
    if (twi->xferType == TWI_SIMPLE_READ) {
        mstrctl |= ENUM_TWI_MSTRCTL_RX;
        if (twi->rlen > TWI_MAX_DCNT) {
            twi->longXfer = 1;
            len = 0xFF;
        } else {
            len = twi->rlen;
        }
    } else if (twi->xferType == TWI_SIMPLE_WRITEREAD) {
        mstrctl |= BITM_TWI_MSTRCTL_RSTART | ENUM_TWI_MSTRCTL_TX;
        len = twi->slen;
    } else {
        mstrctl |= ENUM_TWI_MSTRCTL_TX;
        if (twi->xferType == TWI_SIMPLE_WRITE) {
            len = twi->slen;
        } else {
            len = twi->slen + twi->rlen;
        }
        if (len > TWI_MAX_DCNT) {
            twi->longXfer = 1;
            len = 0xFF;
        }
    }
    mstrctl |= ((len & 0xFF) << BITP_TWI_MSTRCTL_DCNT);

    return (mstrctl);
}

static uint16_t twi_end(sTWI *twi)
{
   uint16_t errors;

    /* Gather errors */
    errors = (*twi->pREG_TWI_MSTRSTAT & TWI_SIMPLE_ERRORS);

    /* Clear the status */
    *twi->pREG_TWI_MSTRSTAT = 0xFFFF;

   return errors;
}

static void twi_fill_fifo(sTWI *twi)
{
    if (twi->sdata != NULL) {
        while ((twi->slen > 0) && !TX_FIFO_FULL) {
            *twi->pREG_TWI_TXDATA8 = *twi->sdata;
            twi->sdata++; twi->slen--;
            if (twi->xferType == TWI_SIMPLE_WRITEWRITE) {
                if (twi->slen == 0) {
                    twi->sdata = twi->rdata;
                    twi->slen = twi->rlen;
                    twi->xferType = TWI_SIMPLE_WRITE;
                }
            }
        }
    }
}

static void twi_empty_fifo(sTWI *twi)
{
    if (twi->rdata != NULL) {
        while ((twi->rlen > 0) && !RX_FIFO_EMPTY) {
            *twi->rdata = *twi->pREG_TWI_RXDATA8;
            twi->rdata++; twi->rlen--;
        }
    }
}


static TWI_SIMPLE_RESULT twi_xfer(sTWI *twi, uint8_t address,
    TWI_SIMPLE_XFER_TYPE xferType,
    uint8_t *out, uint16_t outLen,
    uint8_t *in, uint16_t inLen )
{
    TWI_SIMPLE_RESULT result;
    uint16_t mstrctrl;
    uint16_t twiErrors;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (twi == NULL) {
        return(TWI_SIMPLE_ERROR);
    }

    result = TWI_SIMPLE_SUCCESS;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(twi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
        return(result);
    }
#endif

    /* Convert write/reads if necessary */
    if (xferType == TWI_SIMPLE_WRITEREAD) {
        if (outLen > TWI_MAX_DCNT) {
            result = TWI_SIMPLE_BAD_LENGTH;
        }
        if ((outLen == 0) && (inLen == 0)) {
            result = TWI_SIMPLE_BAD_LENGTH;
        } else if (outLen == 0) {
            xferType = TWI_SIMPLE_READ;
        } else if (inLen == 0) {
            xferType = TWI_SIMPLE_WRITE;
        }
    }

    /* Convert write/writes if necessary */
    if (xferType == TWI_SIMPLE_WRITEWRITE) {
        if ((outLen == 0) && (inLen == 0)) {
            result = TWI_SIMPLE_BAD_LENGTH;
        } else if ((in == NULL) || (inLen == 0)) {
            xferType = TWI_SIMPLE_WRITE;
        }
    }

    /* Don't allow zero byte reads.  Confuses some devices */
    if ((xferType == TWI_SIMPLE_READ) && (inLen == 0)) {
        result = TWI_SIMPLE_BAD_LENGTH;
    }

    if (result == TWI_SIMPLE_SUCCESS) {

        /* Save the transfer info */
        twi->address = address;
        twi->sdata = out;
        twi->slen = outLen;
        twi->xferType = xferType;
        twi->rdata = in;
        twi->rlen = inLen;

        /* Setup the transfer */
        mstrctrl = twi_begin(twi);

        /* Prefill the FIFO */
        if (twi->xferType != TWI_SIMPLE_READ) {
            twi_fill_fifo(twi);
        }

#ifndef FREE_RTOS
        twi->twiDone = false;
#endif

        /* Start the transfer */
        *twi->pREG_TWI_MSTRCTL = mstrctrl | ENUM_TWI_MSTRCTL_EN;

        /* Block until complete */
#ifdef FREE_RTOS
        rtosResult = xSemaphoreTake(twi->portBlock, portMAX_DELAY);
        if (rtosResult != pdTRUE) {
            result = TWI_SIMPLE_ERROR;
        }
#else
        while (twi->twiDone == false);
#endif

        /* Complete the transfer */
        twiErrors = twi_end(twi);
        if (twiErrors) {
            result = TWI_SIMPLE_ERROR;
        }

    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(twi->portLock);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
    }
#endif

    return(result);
}


TWI_SIMPLE_RESULT twi_write(sTWI *twiHandle, uint8_t address,
    uint8_t *out, uint16_t outLen)
{
    TWI_SIMPLE_RESULT result;

    result = twi_xfer(twiHandle, address, TWI_SIMPLE_WRITE,
        out, outLen, NULL, 0);

    return(result);
}

TWI_SIMPLE_RESULT twi_read(sTWI *twiHandle, uint8_t address,
    uint8_t *in, uint16_t inLen)
{
    TWI_SIMPLE_RESULT result;

    result = twi_xfer(twiHandle, address, TWI_SIMPLE_READ,
        NULL, 0, in, inLen);

    return(result);
}

TWI_SIMPLE_RESULT twi_writeRead(sTWI *twiHandle, uint8_t address,
    uint8_t *out, uint16_t outLen, uint8_t *in, uint16_t inLen)
{
    TWI_SIMPLE_RESULT result;

    result = twi_xfer(twiHandle, address, TWI_SIMPLE_WRITEREAD,
        out, outLen, in, inLen);

    return(result);
}

TWI_SIMPLE_RESULT twi_writeWrite(sTWI *twiHandle, uint8_t address,
    uint8_t *out, uint16_t outLen, uint8_t *out2, uint16_t out2Len)
{
    TWI_SIMPLE_RESULT result;

    result = twi_xfer(twiHandle, address, TWI_SIMPLE_WRITEWRITE,
        out, outLen, out2, out2Len);

    return(result);
}

static void twi_initRegs(sTWI *twi)
{
    twi->DEFAULT_TWI_MSTRCTL  = 0x0000u;

    *twi->pREG_TWI_CTL        = 0x0000u;
    *twi->pREG_TWI_CLKDIV     = 0x0000u;
    *twi->pREG_TWI_FIFOCTL    =
        ENUM_TWI_FIFOCTL_RXTWOBYTE | ENUM_TWI_FIFOCTL_TXTWOBYTE;
    *twi->pREG_TWI_MSTRADDR   = 0x0000u;
    *twi->pREG_TWI_MSTRCTL    = twi->DEFAULT_TWI_MSTRCTL;

    *twi->pREG_TWI_IMSK       = 0x0000u;
    *twi->pREG_TWI_ISTAT      = 0xFFFFu;
    *twi->pREG_TWI_MSTRSTAT   = 0xFFFFu;
}

TWI_SIMPLE_RESULT twi_init(void)
{
    TWI_SIMPLE_RESULT result = TWI_SIMPLE_SUCCESS;
    uint8_t port;
    sTWI *twi;

    memset(twiContext, 0, sizeof(twiContext));

    for (port = TWI0; port < TWI_END; port++) {

        twi = &twiContext[port];

        if (port == TWI0) {
            twi->pREG_TWI_CTL        = pREG_TWI0_CTL;
            twi->pREG_TWI_CLKDIV     = pREG_TWI0_CLKDIV;
            twi->pREG_TWI_FIFOCTL    = pREG_TWI0_FIFOCTL;
            twi->pREG_TWI_FIFOSTAT   = (volatile uint16_t *)pREG_TWI0_FIFOSTAT;
            twi->pREG_TWI_MSTRADDR   = pREG_TWI0_MSTRADDR;
            twi->pREG_TWI_IMSK       = pREG_TWI0_IMSK;
            twi->pREG_TWI_ISTAT      = pREG_TWI0_ISTAT;
            twi->pREG_TWI_MSTRSTAT   = pREG_TWI0_MSTRSTAT;
            twi->pREG_TWI_MSTRCTL    = pREG_TWI0_MSTRCTL;
            twi->pREG_TWI_TXDATA8    = pREG_TWI0_TXDATA8;
            twi->pREG_TWI_TXDATA16   = pREG_TWI0_TXDATA16;
            twi->pREG_TWI_RXDATA8    = pREG_TWI0_RXDATA8;
            twi->pREG_TWI_RXDATA16   = pREG_TWI0_RXDATA16;

            twi->TWI_DATA_IRQ_ID     = INTR_TWI0_DATA;
        }
        else if (port == TWI1) {
            twi->pREG_TWI_CTL        = pREG_TWI1_CTL;
            twi->pREG_TWI_CLKDIV     = pREG_TWI1_CLKDIV;
            twi->pREG_TWI_FIFOCTL    = pREG_TWI1_FIFOCTL;
            twi->pREG_TWI_FIFOSTAT   = (volatile uint16_t *)pREG_TWI1_FIFOSTAT;
            twi->pREG_TWI_MSTRADDR   = pREG_TWI1_MSTRADDR;
            twi->pREG_TWI_IMSK       = pREG_TWI1_IMSK;
            twi->pREG_TWI_ISTAT      = pREG_TWI1_ISTAT;
            twi->pREG_TWI_MSTRSTAT   = pREG_TWI1_MSTRSTAT;
            twi->pREG_TWI_MSTRCTL    = pREG_TWI1_MSTRCTL;
            twi->pREG_TWI_TXDATA8    = pREG_TWI1_TXDATA8;
            twi->pREG_TWI_TXDATA16   = pREG_TWI1_TXDATA16;
            twi->pREG_TWI_RXDATA8    = pREG_TWI1_RXDATA8;
            twi->pREG_TWI_RXDATA16   = pREG_TWI1_RXDATA16;

            twi->TWI_DATA_IRQ_ID     = INTR_TWI1_DATA;
        }
        else if (port == TWI2) {
            twi->pREG_TWI_CTL        = pREG_TWI2_CTL;
            twi->pREG_TWI_CLKDIV     = pREG_TWI2_CLKDIV;
            twi->pREG_TWI_FIFOCTL    = pREG_TWI2_FIFOCTL;
            twi->pREG_TWI_FIFOSTAT   = (volatile uint16_t *)pREG_TWI2_FIFOSTAT;
            twi->pREG_TWI_MSTRADDR   = pREG_TWI2_MSTRADDR;
            twi->pREG_TWI_IMSK       = pREG_TWI2_IMSK;
            twi->pREG_TWI_ISTAT      = pREG_TWI2_ISTAT;
            twi->pREG_TWI_MSTRSTAT   = pREG_TWI2_MSTRSTAT;
            twi->pREG_TWI_MSTRCTL    = pREG_TWI2_MSTRCTL;
            twi->pREG_TWI_TXDATA8    = pREG_TWI2_TXDATA8;
            twi->pREG_TWI_TXDATA16   = pREG_TWI2_TXDATA16;
            twi->pREG_TWI_RXDATA8    = pREG_TWI2_RXDATA8;
            twi->pREG_TWI_RXDATA16   = pREG_TWI2_RXDATA16;

            twi->TWI_DATA_IRQ_ID     = INTR_TWI2_DATA;
        }

#ifdef FREE_RTOS
#if 1
        twi->portLock = xSemaphoreCreateCounting(1, 1);
#else
        /*
         * There's an FreeRTOS assert at line 408 of tasks.c that is
         * triggered when the semaphore is created as a mutex.
         */
        twi->portLock = xSemaphoreCreateMutex();
#endif
        if (twi->portLock == NULL) {
            result = TWI_SIMPLE_ERROR;
        }

        twi->portBlock = xSemaphoreCreateCounting(1, 0);
        if (twi->portBlock == NULL) {
            result = TWI_SIMPLE_ERROR;
        }
#endif
        twi->open = false;

    }

    return(result);
}


TWI_SIMPLE_RESULT twi_deinit(void)
{
    TWI_SIMPLE_RESULT result = TWI_SIMPLE_SUCCESS;
    uint8_t port;
    sTWI *twi;

    for (port = TWI0; port < TWI_END; port++) {

        twi = &twiContext[port];

#ifdef FREE_RTOS
        if (twi->portBlock) {
            vSemaphoreDelete(twi->portBlock);
            twi->portBlock = NULL;
        }

        if (twi->portLock) {
            vSemaphoreDelete(twi->portLock);
            twi->portLock = NULL;
        }
#endif

    }

    return(result);
}


TWI_SIMPLE_RESULT twi_open(TWI_SIMPLE_PORT port, sTWI **twiHandle)
{
    TWI_SIMPLE_RESULT result = TWI_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sTWI *twi;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (port >= TWI_END) {
        return(TWI_SIMPLE_INVALID_PORT);
    }

    twi = &twiContext[port];

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(twi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
        return result;
    }
#endif

    if (twi->open == true) {
        result = TWI_SIMPLE_PORT_BUSY;
    }

    if (result == TWI_SIMPLE_SUCCESS) {

        /* Reset the twi port */
        twi_initRegs(twi);

        /* Install the irq handler */
        status = adi_int_InstallHandler(twi->TWI_DATA_IRQ_ID,
            twi_data_irq, twi, true);

        if (status == ADI_INT_SUCCESS) {

            /* Set the prescale */
            *twi->pREG_TWI_CTL &= ~BITM_TWI_CTL_PRESCALE;
            *twi->pREG_TWI_CTL |= (TWI_SIMPLE_PRESCALE << BITP_TWI_CTL_PRESCALE) |
                ENUM_TWI_CTL_EN;

            /* Default the speed to 100KHz */
            _twi_setSpeed(twi, TWI_SIMPLE_SPEED_100);

            /* Enable some tx, rx, and complete irqs */
            *twi->pREG_TWI_IMSK = TWI_SIMPLE_IRQ_ENABLE;

        } else {
            result = TWI_SIMPLE_ERROR;
        }

    }

    if (result == TWI_SIMPLE_SUCCESS) {
        *twiHandle = twi;
        twi->open = true;
    } else {
        status = adi_int_UninstallHandler(twi->TWI_DATA_IRQ_ID);
        *twiHandle = NULL;
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(twi->portLock);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
    }
#endif

    return(result);
}

TWI_SIMPLE_RESULT twi_close(sTWI **twiHandle)
{
    TWI_SIMPLE_RESULT result = TWI_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sTWI *twi = *twiHandle;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (*twiHandle == NULL) {
        return (TWI_SIMPLE_ERROR);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(twi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
        return result;
    }
#endif

    twi->open = false;
    twi_initRegs(twi);

    status = adi_int_EnableInt(twi->TWI_DATA_IRQ_ID, false);
    if (status != ADI_INT_SUCCESS) {
        result = TWI_SIMPLE_ERROR;
    }

    status = adi_int_UninstallHandler(twi->TWI_DATA_IRQ_ID);
    if (status != ADI_INT_SUCCESS) {
        result = TWI_SIMPLE_ERROR;
    }

    *twiHandle = NULL;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(twi->portLock);
    if (rtosResult != pdTRUE) {
        result = TWI_SIMPLE_ERROR;
    }
#endif

    return(result);
}

static void twi_data_irq(uint32_t id, void *usrPtr)
{
    sTWI *twi = (sTWI *)usrPtr;
    uint16_t istat;
    uint16_t mstrctl;
    uint16_t mstrstat;
    uint8_t len;
    bool xferDone;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
    BaseType_t contextSwitch = pdFALSE;
#endif

    istat = *twi->pREG_TWI_ISTAT;
    *twi->pREG_TWI_ISTAT = istat;

    if (istat & TWI_SIMPLE_IRQ_MASK) {

       if (istat & BITM_TWI_ISTAT_TXSERV) {
            /*
             * The TX FIFO is configured to interrupt when empty so
             * set the DCNT field of the MSTRCTL register to the
             * remaining bytes when the length fits.
             *
             * No bytes are being transmitted while the FIFO is empty.
             */
            if ( (twi->xferType == TWI_SIMPLE_WRITE) &&
                 (twi->longXfer) &&
                 (twi->slen < TWI_MAX_DCNT) ) {
                mstrctl = *twi->pREG_TWI_MSTRCTL & ~BITM_TWI_MSTRCTL_DCNT;
                mstrctl |= twi->slen << BITP_TWI_MSTRCTL_DCNT;
                *twi->pREG_TWI_MSTRCTL = mstrctl;
                twi->longXfer = 0;
            }
            twi_fill_fifo(twi);
        }

        if (istat & BITM_TWI_ISTAT_RXSERV) {
            /*
             * The RX FIFO is configured to interrupt when full (two
             * bytes) so set the DCNT field of the MSTRCTL register to
             * two bytes less than the remaining bytes to transfer when
             * the length fits.
             *
             * No bytes are being received while the FIFO is full.
             */
            if (twi->longXfer && (twi->rlen < TWI_MAX_DCNT)) {
                mstrctl = *twi->pREG_TWI_MSTRCTL & ~BITM_TWI_MSTRCTL_DCNT;
                mstrctl |= (twi->rlen - 2) << BITP_TWI_MSTRCTL_DCNT;
                *twi->pREG_TWI_MSTRCTL = mstrctl;
                twi->longXfer = 0;
            }
            twi_empty_fifo(twi);
        }

        if (istat & BITM_TWI_ISTAT_MCOMP) {

            /* Get the master status */
            mstrstat = *twi->pREG_TWI_MSTRSTAT;

            /* End the transfer if any errors are indicated */
            xferDone = (*twi->pREG_TWI_MSTRSTAT & TWI_SIMPLE_ERRORS);

            /* Service read complete */
            if ((!xferDone) && (twi->xferType == TWI_SIMPLE_READ)) {
                if (twi->rlen != 0) {
                    twi_empty_fifo(twi);
                }
                xferDone = true;
            }

            /* Service write complete */
            if ( (!xferDone) && (twi->xferType == TWI_SIMPLE_WRITE)) {
                xferDone = true;
            }

            /* Service write portion of write/read complete */
            if ( (!xferDone) && (twi->xferType == TWI_SIMPLE_WRITEREAD)) {
                if (twi->rlen > TWI_MAX_DCNT) {
                    len = 0xFF;
                    twi->longXfer = 1;
                } else {
                    len = twi->rlen;
                    twi->longXfer = 0;
                }
                mstrctl = *twi->pREG_TWI_MSTRCTL;
                mstrctl &= ~(BITM_TWI_MSTRCTL_RSTART | BITM_TWI_MSTRCTL_DIR);
                mstrctl |= (len << BITP_TWI_MSTRCTL_DCNT) | ENUM_TWI_MSTRCTL_RX;
                *twi->pREG_TWI_MSTRCTL = mstrctl;
                twi->xferType = TWI_SIMPLE_READ;
            }

            if (xferDone) {
#ifdef FREE_RTOS
                rtosResult = xSemaphoreGiveFromISR(twi->portBlock, &contextSwitch);
                portYIELD_FROM_ISR(contextSwitch);
#else
                twi->twiDone = true;
#endif
            }
        }
    }
}
