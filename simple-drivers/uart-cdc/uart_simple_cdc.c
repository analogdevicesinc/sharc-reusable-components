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
 * Simple UART compatibility interface on top of the CLD
 * CDC + UAC library.
 *
 *   This UART driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Fully protected multi-threaded device transfers
 *     - Blocking transfers
 *
 * Copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/
#include <string.h>

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define UART_ENTER_CRITICAL()  taskENTER_CRITICAL()
    #define UART_EXIT_CRITICAL()   taskEXIT_CRITICAL()
#else
    #define UART_ENTER_CRITICAL()
    #define UART_EXIT_CRITICAL()
#endif

#include "uart_simple_cdc.h"
#include "cdc.h"

#define UART_BUFFER_SIZE        (1024)
#define UART_END_CDC            (UART1)

typedef enum UART_SIMPLE_INT_RESULT
{
    UART_SIMPLE_TX_OK,
    UART_SIMPLE_TX_ERROR,
    UART_SIMPLE_TX_FIFO_EMPTY,
    UART_SIMPLE_RX_OK,
    UART_SIMPLE_RX_FIFO_FULL_INT
} UART_SIMPLE_INT_RESULT;

struct sUART {

    // UART receive buffer
    uint8_t rx_buffer[UART_BUFFER_SIZE];
    uint16_t rx_buffer_readptr;
    volatile uint16_t rx_buffer_writeptr;

    // UART transmit buffer
    uint8_t tx_buffer[UART_BUFFER_SIZE];
    volatile uint16_t tx_buffer_readptr;
    uint16_t tx_buffer_writeptr;

    // read/write timeouts mode
    int32_t readTimeout;
    int32_t writeTimeout;

    // misc
    bool transmitting;
    bool open;
    bool rxSleeping;
    bool txSleeping;

#ifdef FREE_RTOS
    SemaphoreHandle_t portLock;
    SemaphoreHandle_t portRxLock;
    SemaphoreHandle_t portTxLock;
    SemaphoreHandle_t portRxBlock;
    SemaphoreHandle_t portTxBlock;
    TickType_t rtosReadTimeout;
    TickType_t rtosWriteTimeout;
#else
    volatile bool uartDone;
#endif

};

/* UART port context containers.  Must currently be in uncached
 * memory since the CDC+UAC library is in DMA mode.
 *
 * FIXME: Remove uncached requirement
 */
__attribute__ ((section(".l3_uncached_data")))
    static sUART uartContext[UART_END_CDC];

static bool uart_cdc_initialized = false;

static UART_SIMPLE_INT_RESULT _uart_cdc_isr_writeToRXBuffer(sUART *uart, uint8_t val)
{
    // First check if RX buffer is full
    if (((uart->rx_buffer_writeptr+1) % UART_BUFFER_SIZE) == uart->rx_buffer_readptr) {
        return UART_SIMPLE_RX_FIFO_FULL_INT;
    }

    // Write value into FIFO
    uart->rx_buffer[uart->rx_buffer_writeptr] = val;
    uart->rx_buffer_writeptr++;

    // wrap pointer if necessary
    if (uart->rx_buffer_writeptr >= UART_BUFFER_SIZE) {
        uart->rx_buffer_writeptr = 0;
    }

    return UART_SIMPLE_RX_OK;
}

/*
 * FIXME: Send out chunks of data instead of char by char
 */
UART_SIMPLE_INT_RESULT _uart_cdc_isr_readFromTXBuffer(sUART *uart, uint8_t *val)
{
    uint8_t *c;
    CLD_USB_Data_Transmit_Return_Type ok;
    UART_SIMPLE_INT_RESULT ret;

    // First check if write buffer is empty
    if (uart->tx_buffer_writeptr == uart->tx_buffer_readptr) {
        return UART_SIMPLE_TX_FIFO_EMPTY;
    }

    // fetch latest value from TX buffer
    c = &uart->tx_buffer[uart->tx_buffer_readptr];

    /* Send out the byte */
    ok = cdc_tx_serial_data(1, c, 10);

    /* Return the result */
    ret = (ok == CLD_USB_TRANSMIT_SUCCESSFUL) ?
        UART_SIMPLE_TX_OK : UART_SIMPLE_TX_ERROR;

    return ret;
}

void _uart_cdc_tx_complete(CDC_TX_STATUS status, void *usrPtr)
{
    sUART *uart = (sUART *)usrPtr;
    UART_SIMPLE_INT_RESULT result;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
    BaseType_t contextSwitch = pdFALSE;
#endif

    /* Increment the TX read pointer upon completion */
    uart->tx_buffer_readptr++;
    if (uart->tx_buffer_readptr >= UART_BUFFER_SIZE) {
        uart->tx_buffer_readptr = 0;
    }

    /* Send the next chunk and update the transmitting status */
    result = _uart_cdc_isr_readFromTXBuffer(uart, NULL);
    if (result == UART_SIMPLE_TX_FIFO_EMPTY) {
        uart->transmitting = false;
    }

#ifdef FREE_RTOS
    /* Wake any blocked threads */
    if (uart->txSleeping) {
        rtosResult = xSemaphoreGiveFromISR(uart->portTxBlock, &contextSwitch);
        portYIELD_FROM_ISR(contextSwitch);
        uart->txSleeping = 0;
    }
#endif
}

void _uart_cdc_rx_complete(unsigned char *buffer,
    unsigned short length, void *usrPtr)
{
    sUART *uart = (sUART *)usrPtr;
    UART_SIMPLE_INT_RESULT result;
    unsigned short i;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
    BaseType_t contextSwitch = pdFALSE;
#endif

    /* Put received bytes into the RX buffer */
    for (i = 0; i < length; i++) {
        result = _uart_cdc_isr_writeToRXBuffer(uart, buffer[i]);
        if (result != UART_SIMPLE_RX_OK) {
            break;
        }
    }

#ifdef FREE_RTOS
    /* Wake any blocked threads if new data is available */
    if ((i > 0) && (uart->rxSleeping)) {
        rtosResult = xSemaphoreGiveFromISR(uart->portRxBlock, &contextSwitch);
        portYIELD_FROM_ISR(contextSwitch);
        uart->rxSleeping = false;
    }
#endif

}

UART_SIMPLE_RESULT uart_cdc_read(sUART *uart, uint8_t *in, uint8_t *inLen)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    bool empty;
    int i;

#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(uart->portRxLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

#ifdef FREE_RTOS
    UART_ENTER_CRITICAL();
    empty = (uart->rx_buffer_writeptr == uart->rx_buffer_readptr);
    if (empty) {
        uart->rxSleeping = true;
    }
    UART_EXIT_CRITICAL();
    if (empty) {
        rtosResult = xSemaphoreTake(uart->portRxBlock, uart->readTimeout);
        if (rtosResult == pdFALSE) {
            UART_ENTER_CRITICAL();
            if (uart->rxSleeping) {
                uart->rxSleeping = false;
            }
            UART_EXIT_CRITICAL();
        }
    }
#else
    if (uart->readTimeout == UART_SIMPLE_TIMEOUT_INF) {
        do {
            empty = (uart->rx_buffer_writeptr == uart->rx_buffer_readptr);
        } while (empty);
    }
#endif

    empty = (uart->rx_buffer_writeptr == uart->rx_buffer_readptr);
    for (i = 0; (i < *inLen) && !empty; i++) {
        in[i] = uart->rx_buffer[uart->rx_buffer_readptr++];
        if (uart->rx_buffer_readptr >= UART_BUFFER_SIZE) {
            uart->rx_buffer_readptr = 0;
        }
        empty = (uart->rx_buffer_writeptr == uart->rx_buffer_readptr);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portRxLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    *inLen = i;

    return(result);
}


UART_SIMPLE_RESULT uart_cdc_write(sUART *uart, uint8_t *out, uint8_t *outLen)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    UART_SIMPLE_INT_RESULT intResult;
    int i;
    bool full;
    bool goToSleep;
    bool transmitting;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(uart->portTxLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    for (i = 0; i < *outLen; i++) {

#ifdef FREE_RTOS
        UART_ENTER_CRITICAL();
        full = ((uart->tx_buffer_writeptr+1) % UART_BUFFER_SIZE) == uart->tx_buffer_readptr;
        transmitting = uart->transmitting;
        goToSleep = full & transmitting;
        if (goToSleep) {
            uart->txSleeping = true;
        }
        UART_EXIT_CRITICAL();
        if (goToSleep) {
            rtosResult = xSemaphoreTake(uart->portTxBlock, portMAX_DELAY);
            if (rtosResult != pdTRUE) {
                result = UART_SIMPLE_ERROR;
            }
        } else {
            if (full & !transmitting) {
                break;
            }
        }
#else
        do {
            full = ((uart->tx_buffer_writeptr+1) % UART_BUFFER_SIZE) == uart->tx_buffer_readptr;
        } while (full);
#endif

        uart->tx_buffer[uart->tx_buffer_writeptr++] = out[i];

        if (uart->tx_buffer_writeptr >= UART_BUFFER_SIZE) {
            uart->tx_buffer_writeptr = 0;
        }
    }

    /* Report back the bytes written */
    *outLen = i;

    /*
     * Kick off a write if needed
     */
     UART_ENTER_CRITICAL();
     if (!uart->transmitting) {
        intResult = _uart_cdc_isr_readFromTXBuffer(uart, NULL);
        if (intResult == UART_SIMPLE_TX_OK) {
            uart->transmitting = true;
        } else {
            result = UART_SIMPLE_ERROR;
        }
     }
     UART_EXIT_CRITICAL();

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portTxLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    return(result);
}

UART_SIMPLE_RESULT uart_cdc_setProtocol(sUART *uartHandle,
    UART_SIMPLE_SPEED speed, UART_SIMPLE_WORD_LENGTH length,
    UART_SIMPLE_PARITY parity, UART_SIMPLE_STOP_BITS stop)
{
    return(UART_SIMPLE_SUCCESS);
}

#include <stdio.h>
UART_SIMPLE_RESULT uart_cdc_setTimeouts(sUART *uartHandle,
    int32_t readTimeout, int32_t writeTimeout)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    sUART *uart = uartHandle;

#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(uart->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    if ((readTimeout != UART_SIMPLE_TIMEOUT_NO_CHANGE) &&
        (readTimeout != uart->readTimeout)) {
        uart->readTimeout = readTimeout;
#ifdef FREE_RTOS
        if (uart->readTimeout == UART_SIMPLE_TIMEOUT_INF) {
            uart->rtosReadTimeout = portMAX_DELAY;
        } else if (uart->readTimeout == UART_SIMPLE_TIMEOUT_NONE) {
            uart->rtosReadTimeout = 0;
        } else {
            uart->rtosReadTimeout = pdMS_TO_TICKS(uart->readTimeout);
        }
#endif
    }
    if ((writeTimeout != UART_SIMPLE_TIMEOUT_NO_CHANGE) &&
        (writeTimeout != uart->writeTimeout)) {
        uart->writeTimeout = writeTimeout;
#ifdef FREE_RTOS
        if (uart->writeTimeout == UART_SIMPLE_TIMEOUT_INF) {
            uart->rtosWriteTimeout = portMAX_DELAY;
        } else if (uart->writeTimeout == UART_SIMPLE_TIMEOUT_NONE) {
            uart->rtosWriteTimeout = 0;
        } else {
            uart->rtosWriteTimeout = pdMS_TO_TICKS(uart->readTimeout);
        }
#endif
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    return(result);
}

UART_SIMPLE_RESULT uart_cdc_open(UART_SIMPLE_PORT port, sUART **uartHandle)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    sUART *uart;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (port >= UART_END_CDC) {
        return(UART_SIMPLE_INVALID_PORT);
    }

    uart = &uartContext[port];

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(uart->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    if (uart->open == true) {
        result = UART_SIMPLE_PORT_BUSY;
    }

    if (result == UART_SIMPLE_SUCCESS) {

        cdc_register_tx_callback(_uart_cdc_tx_complete, uart);
        cdc_register_rx_callback(_uart_cdc_rx_complete, uart);

        memset(uart->rx_buffer, 0, sizeof(uart->rx_buffer));
        uart->rx_buffer_readptr = 0;
        uart->rx_buffer_writeptr = 0;

        memset(uart->tx_buffer, 0, sizeof(uart->tx_buffer));
        uart->tx_buffer_readptr = 0;
        uart->tx_buffer_writeptr = 0;

        uart->readTimeout = UART_SIMPLE_TIMEOUT_INF;
        uart->writeTimeout = UART_SIMPLE_TIMEOUT_INF;

#ifdef FREE_RTOS
        uart->rtosReadTimeout = portMAX_DELAY;
        uart->rtosWriteTimeout = portMAX_DELAY;
#endif

    }

    if (result == UART_SIMPLE_SUCCESS) {
        *uartHandle = uart;
        uart->open = true;
    } else {
        *uartHandle = NULL;
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    return(result);
}

UART_SIMPLE_RESULT uart_cdc_close(sUART **uartHandle)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    sUART *uart = *uartHandle;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (*uartHandle == NULL) {
        return (UART_SIMPLE_ERROR);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(uart->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    uart->open = false;

    cdc_register_tx_callback(NULL, NULL);
    cdc_register_rx_callback(NULL, NULL);

    *uartHandle = NULL;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    return(result);
}

UART_SIMPLE_RESULT uart_cdc_init(void)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    uint8_t port;
    sUART *uart;

    if (uart_cdc_initialized == true) {
        return(UART_SIMPLE_ERROR);
    }

    memset(uartContext, 0, sizeof(uartContext));

    for (port = UART0; port < UART_END_CDC; port++) {

        uart = &uartContext[port];

#ifdef FREE_RTOS
        uart->portLock = xSemaphoreCreateMutex();
        uart->portRxLock = xSemaphoreCreateMutex();
        uart->portTxLock = xSemaphoreCreateMutex();

        if (uart->portLock == NULL) {
            result = UART_SIMPLE_ERROR;
        }

        if (uart->portRxLock == NULL) {
            result = UART_SIMPLE_ERROR;
        }

        if (uart->portTxLock == NULL) {
            result = UART_SIMPLE_ERROR;
        }

        uart->portRxBlock = xSemaphoreCreateCounting(1, 0);
        if (uart->portRxBlock == NULL) {
            result = UART_SIMPLE_ERROR;
        }

        uart->portTxBlock = xSemaphoreCreateCounting(1, 0);
        if (uart->portTxBlock == NULL) {
            result = UART_SIMPLE_ERROR;
        }
#endif
        uart->open = false;

    }

    uart_cdc_initialized = true;

    return(result);
}


UART_SIMPLE_RESULT uart_cdc_deinit(void)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    uint8_t port;
    sUART *uart;

    for (port = UART0; port < UART_END_CDC; port++) {

        uart = &uartContext[port];

#ifdef FREE_RTOS
        if (uart->portRxBlock) {
            vSemaphoreDelete(uart->portRxBlock);
            uart->portRxBlock = NULL;
        }

        if (uart->portTxBlock) {
            vSemaphoreDelete(uart->portTxBlock);
            uart->portTxBlock = NULL;
        }

        if (uart->portLock) {
            vSemaphoreDelete(uart->portLock);
            uart->portLock = NULL;
        }

        if (uart->portRxLock) {
            vSemaphoreDelete(uart->portRxLock);
            uart->portLock = NULL;
        }

        if (uart->portRxLock) {
            vSemaphoreDelete(uart->portRxLock);
            uart->portLock = NULL;
        }
#endif

    }

    return(result);
}
