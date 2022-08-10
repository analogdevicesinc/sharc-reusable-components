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
 * UART Simple provides a simple, efficient, RTOS or bare metal UART driver.
 *
 * This driver supports:
 *  - FreeRTOS or no RTOS main-loop modes
 *  - Fully protected multi-threaded UART transfers
 *  - Blocking transfers
 *
 */
#include <string.h>
#include <sys/platform.h>
#include <services/int/adi_int.h>

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

#include "uart_simple.h"

#define UART_SIMPLE_IRQ_ENABLE ( \
    BITM_UART_IMSK_SET_ERXS | BITM_UART_IMSK_SET_ERBFI | \
    BITM_UART_IMSK_SET_ETXS | \
    BITM_UART_IMSK_SET_ELSI )

#define UART_SIMPLE_PROTOCOL_SETTINGS_MASK ( \
    BITM_UART_CTL_PEN | BITM_UART_CTL_EPS | \
    BITM_UART_CTL_STB | BITM_UART_CTL_WLS )

#define UART_SIMPLE_CHECK_TX_STATUS(x) \
    ((*(x->pREG_UART_STAT)) & BITM_UART_STAT_THRE)

// Size of the UART TX and RX buffers / FIFOs
#define UART_BUFFER_SIZE        (1024)

typedef struct _SPI_SIMPLE_BAUD_RATES {
    uint8_t baudRateEnum;
    uint32_t baudRate;
    uint32_t dFactor1;
    uint32_t dFactor16;
} SPI_SIMPLE_BAUD_RATES;

#define SCLK0_D1(x)   (ENUM_UART_CLK_EN_DIV_BY_ONE | (SCLK0 / 1 / x))
#define SCLK0_D16(x)  (ENUM_UART_CLK_DIS_DIV_BY_ONE | (SCLK0 / 16 / x))

SPI_SIMPLE_BAUD_RATES baudRateTable[] = {
    { UART_SIMPLE_BAUD_NONE,   0,      0,               0 },
    { UART_SIMPLE_BAUD_110,    110,    SCLK0_D1(110),    SCLK0_D16(110) },
    { UART_SIMPLE_BAUD_300,    300,    SCLK0_D1(300),    SCLK0_D16(300) },
    { UART_SIMPLE_BAUD_600,    600,    SCLK0_D1(600),    SCLK0_D16(600) },
    { UART_SIMPLE_BAUD_1200,   1200,   SCLK0_D1(1200),   SCLK0_D16(1200) },
    { UART_SIMPLE_BAUD_2400,   2400,   SCLK0_D1(2400),   SCLK0_D16(2400) },
    { UART_SIMPLE_BAUD_4800,   4800,   SCLK0_D1(4800),   SCLK0_D16(4800) },
    { UART_SIMPLE_BAUD_9600,   9600,   SCLK0_D1(9600),   SCLK0_D16(9600) },
    { UART_SIMPLE_BAUD_14400,  14400,  SCLK0_D1(14400),  SCLK0_D16(14400) },
    { UART_SIMPLE_BAUD_19200,  19200,  SCLK0_D1(19200),  SCLK0_D16(19200) },
    { UART_SIMPLE_BAUD_28800,  28800,  SCLK0_D1(28800),  SCLK0_D16(28800) },
    { UART_SIMPLE_BAUD_31250,  31250,  SCLK0_D1(31250),  SCLK0_D16(31250) },
    { UART_SIMPLE_BAUD_38400,  38400,  SCLK0_D1(38400),  SCLK0_D16(38400) },
    { UART_SIMPLE_BAUD_56000,  56000,  SCLK0_D1(56000),  SCLK0_D16(56000) },
    { UART_SIMPLE_BAUD_57600,  57600,  SCLK0_D1(57600),  SCLK0_D16(57600) },
    { UART_SIMPLE_BAUD_115200, 115200, SCLK0_D1(115200), SCLK0_D16(115200) },
    { UART_SIMPLE_BAUD_230400, 230400, SCLK0_D1(230400), SCLK0_D16(230400) },
    { UART_SIMPLE_BAUD_256000, 256000, SCLK0_D1(256000), SCLK0_D16(256000) },
    { UART_SIMPLE_BAUD_460800, 460800, SCLK0_D1(460800), SCLK0_D16(460800) },
    { UART_SIMPLE_BAUD_921600, 921600, SCLK0_D1(921600), SCLK0_D16(921600) }
};

struct sUART {

    // pointers into the various control registers
    volatile uint32_t * pREG_UART_CTL;
    volatile uint32_t * pREG_UART_CLK;
    volatile uint32_t * pREG_UART_STAT;
    volatile uint32_t * pREG_UART_THR;
    volatile uint32_t * pREG_UART_RBR;
    volatile uint32_t * pREG_UART_IMSK_CLR;
    volatile uint32_t * pREG_UART_IMSK_SET;

    uint16_t UART_STATUS_IRQ_ID;

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

/* UART port context containers */
static sUART uartContext[UART_END];
static bool uart_initialized = false;

static UART_SIMPLE_RESULT _uart_isr_writeToRXBuffer(sUART *uart, uint8_t val)
{

    // First check if RX buffer is full
    if (((uart->rx_buffer_writeptr+1) % UART_BUFFER_SIZE) == uart->rx_buffer_readptr) {
        return UART_SIMPLE_RX_FIFO_FULL;
    }

    // Write value into FIFO
    uart->rx_buffer[uart->rx_buffer_writeptr] = val;
    uart->rx_buffer_writeptr++;

    // wrap pointer if necessary
    if (uart->rx_buffer_writeptr >= UART_BUFFER_SIZE) {
        uart->rx_buffer_writeptr = 0;
    }

    return UART_SIMPLE_SUCCESS;
}

UART_SIMPLE_RESULT _uart_isr_readFromTXBuffer(sUART *uart, uint8_t *val)
{

    // First check if write buffer is empty
    if (uart->tx_buffer_writeptr == uart->tx_buffer_readptr) {
        return UART_SIMPLE_ERROR;
    }

    // fetch latest value from TX buffer
    *val = uart->tx_buffer[uart->tx_buffer_readptr++];

    // wrap pointer
    if (uart->tx_buffer_readptr >= UART_BUFFER_SIZE) {
        uart->tx_buffer_readptr = 0;
    }

    return UART_SIMPLE_SUCCESS;

}

static void uart_status_irq(uint32_t id, void *usrPtr)
{
    sUART *uart = (sUART *)usrPtr;
    UART_SIMPLE_RESULT result;
    uint8_t byte;
    uint8_t rxCount;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
    BaseType_t contextSwitch = pdFALSE;
#endif

    /*
     ********************************************************************************
     * RX - Check if data is ready that has been received
     ********************************************************************************
     */
    if ((*uart->pREG_UART_STAT) & BITM_UART_STAT_OE) {
        *uart->pREG_UART_STAT = BITM_UART_STAT_OE;
    }

    // copy any bytes that have arrived into our RX FIFO
    rxCount = 0;
    while (*uart->pREG_UART_STAT & BITM_UART_STAT_DR) {

        // read value from UART
        byte = *uart->pREG_UART_RBR;

        // write this value to our receive FIFO
        result = _uart_isr_writeToRXBuffer(uart, byte);

        // Tally bytes received
        if (result == UART_SIMPLE_SUCCESS) {
            rxCount++;
        }

    }

    // Wake any task waiting for data
#ifdef FREE_RTOS
    if ((rxCount > 0) && (uart->rxSleeping)) {
        rtosResult = xSemaphoreGiveFromISR(uart->portRxBlock, &contextSwitch);
        portYIELD_FROM_ISR(contextSwitch);
        uart->rxSleeping = false;
    }
#endif

    /*
     ********************************************************************************
     * TX - Check if we have data to send
     ********************************************************************************
     */

    // check if the transmit buffer is empty and we can add another byte
    if (UART_SIMPLE_CHECK_TX_STATUS(uart)) {
        // Fetch next byte from the tx buffer.  We know there is data in the FIFO.
        result = _uart_isr_readFromTXBuffer(uart, &byte);
        if (result == UART_SIMPLE_SUCCESS) {
            *uart->pREG_UART_THR = byte;
            // Wake any task waiting to send data
#ifdef FREE_RTOS
            if (uart->txSleeping) {
                rtosResult = xSemaphoreGiveFromISR(uart->portTxBlock, &contextSwitch);
                portYIELD_FROM_ISR(contextSwitch);
                uart->txSleeping = false;
            }
#endif
        } else {
            *uart->pREG_UART_IMSK_CLR = BITM_UART_IMSK_SET_ETBEI;
            uart->transmitting = false;
        }
    }

    /*
     ********************************************************************************
     * Line Status - Blindly clear any line status errors
     ********************************************************************************
     */
    *uart->pREG_UART_STAT = ( BITM_UART_STAT_OE | BITM_UART_STAT_PE |
        BITM_UART_STAT_FE | BITM_UART_STAT_BI );

}

UART_SIMPLE_RESULT uart_read(sUART *uart, uint8_t *in, uint8_t *inLen)
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


UART_SIMPLE_RESULT uart_write(sUART *uart, uint8_t *out, uint8_t *outLen)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    int i;
    bool full;
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
        if (full) {
            uart->txSleeping = true;
        }
        UART_EXIT_CRITICAL();
        if (full) {
            rtosResult = xSemaphoreTake(uart->portTxBlock, portMAX_DELAY);
            if (rtosResult != pdTRUE) {
                result = UART_SIMPLE_ERROR;
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

    /*
     * Kick off a write if needed
     */
     UART_ENTER_CRITICAL();
     if (!uart->transmitting) {
        *uart->pREG_UART_IMSK_SET = BITM_UART_IMSK_SET_ETBEI;
        uart->transmitting = true;
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

static void uart_initRegs(sUART *uart)
{
    *uart->pREG_UART_CTL = 0x00000000;
    *uart->pREG_UART_CLK = 0x00000000;
    *uart->pREG_UART_STAT = 0xFFFFFFFF & ~(BITM_UART_STAT_ADDR);
    *uart->pREG_UART_IMSK_CLR = 0xFFFFFFFF;
}

static UART_SIMPLE_RESULT _uart_setProtocol(sUART *uartHandle,
    UART_SIMPLE_SPEED speed, UART_SIMPLE_WORD_LENGTH length,
    UART_SIMPLE_PARITY parity, UART_SIMPLE_STOP_BITS stop)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    sUART *uart = uartHandle;
    uint32_t ctl;

    /* Get the value of the uart_ctl register */
    ctl = *uart->pREG_UART_CTL;

    /* Disable the port to reset the state machine */
    ctl = (ctl & ~(BITM_UART_CTL_EN)) | ENUM_UART_CTL_CLK_DIS;
    *uart->pREG_UART_CTL = ctl;

    /* Mask off the relevant protocol setting */
    ctl &= ~(UART_SIMPLE_PROTOCOL_SETTINGS_MASK);

    /* Set speed */
    if (speed > UART_SIMPLE_BAUD_115200) {
        *uart->pREG_UART_CLK = baudRateTable[speed].dFactor1;
    } else {
        *uart->pREG_UART_CLK = baudRateTable[speed].dFactor16;
    }

    /* Set parity */
    if (parity == UART_SIMPLE_PARITY_EVEN) {
        ctl |= ENUM_UART_CTL_PARITY_EN;
        ctl |= ENUM_UART_CTL_EVEN_PARITY;
    } if (parity == UART_SIMPLE_PARITY_EVEN) {
        ctl |= ENUM_UART_CTL_PARITY_EN;
        ctl |= ENUM_UART_CTL_ODD_PARITY;
    } else {
        ctl |= ENUM_UART_CTL_PARITY_DIS;
    }

    /* Set data bits */
    if (length == UART_SIMPLE_5BIT) {
        ctl |= ENUM_UART_CTL_WL5BITS;
    } else if (length == UART_SIMPLE_6BIT) {
        ctl |= ENUM_UART_CTL_WL6BITS;
    } else if (length == UART_SIMPLE_7BIT) {
        ctl |= ENUM_UART_CTL_WL7BITS;
    } else  {
        ctl |= ENUM_UART_CTL_WL8BITS;
    }

    /* Set stop bits */
    if (stop == UART_SIMPLE_STOP_BITS2) {
        ctl |= ENUM_UART_CTL_ONE_EXTRA_STB;
    } else {
        ctl |= ENUM_UART_CTL_NO_EXTRA_STB;
    }

    /* Store settings and enable */
    *uart->pREG_UART_CTL = ctl | ENUM_UART_CTL_CLK_EN;

    return(result);
}

UART_SIMPLE_RESULT uart_setProtocol(sUART *uartHandle,
    UART_SIMPLE_SPEED speed, UART_SIMPLE_WORD_LENGTH length,
    UART_SIMPLE_PARITY parity, UART_SIMPLE_STOP_BITS stop)
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

    _uart_setProtocol(uart, speed, length, parity, stop);

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    return(result);
}

#include <stdio.h>
UART_SIMPLE_RESULT uart_setTimeouts(sUART *uartHandle,
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

UART_SIMPLE_RESULT uart_open(UART_SIMPLE_PORT port, sUART **uartHandle)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sUART *uart;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (port >= UART_END) {
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

        /* Reset the uart port */
        uart_initRegs(uart);

        /* Install the irq handler */
        status = adi_int_InstallHandler(uart->UART_STATUS_IRQ_ID,
            uart_status_irq, uart, true);

        if (status == ADI_INT_SUCCESS) {

            memset(uart->rx_buffer, 0, sizeof(uart->rx_buffer));
            uart->rx_buffer_readptr = 0;
            uart->rx_buffer_writeptr = 0;

            memset(uart->tx_buffer, 0, sizeof(uart->tx_buffer));
            uart->tx_buffer_readptr = 0;
            uart->tx_buffer_writeptr = 0;

            _uart_setProtocol(uart,
                UART_SIMPLE_BAUD_9600, UART_SIMPLE_8BIT,
                UART_SIMPLE_PARITY_DISABLE, UART_SIMPLE_STOP_BITS1
            );

            *uart->pREG_UART_IMSK_SET = UART_SIMPLE_IRQ_ENABLE;

            uart->readTimeout = UART_SIMPLE_TIMEOUT_INF;
            uart->writeTimeout = UART_SIMPLE_TIMEOUT_INF;
#ifdef FREE_RTOS
            uart->rtosReadTimeout = portMAX_DELAY;
            uart->rtosWriteTimeout = portMAX_DELAY;
#endif
        } else {
            result = UART_SIMPLE_ERROR;
        }

    }

    if (result == UART_SIMPLE_SUCCESS) {
        *uartHandle = uart;
        uart->open = true;
    } else {
        status = adi_int_UninstallHandler(uart->UART_STATUS_IRQ_ID);
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

UART_SIMPLE_RESULT uart_close(sUART **uartHandle)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
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
    uart_initRegs(uart);

    status = adi_int_EnableInt(uart->UART_STATUS_IRQ_ID, false);
    if (status != ADI_INT_SUCCESS) {
        result = UART_SIMPLE_ERROR;
    }

    status = adi_int_UninstallHandler(uart->UART_STATUS_IRQ_ID);
    if (status != ADI_INT_SUCCESS) {
        result = UART_SIMPLE_ERROR;
    }

    *uartHandle = NULL;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(uart->portLock);
    if (rtosResult != pdTRUE) {
        result = UART_SIMPLE_ERROR;
    }
#endif

    return(result);
}

UART_SIMPLE_RESULT uart_init(void)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    uint8_t port;
    sUART *uart;

    if (uart_initialized == true) {
        return(UART_SIMPLE_ERROR);
    }

    memset(uartContext, 0, sizeof(uartContext));

    for (port = UART0; port < UART_END; port++) {

        uart = &uartContext[port];

        if (port == UART0) {
            uart->pREG_UART_CTL       = (volatile uint32_t *) pREG_UART0_CTL;
            uart->pREG_UART_CLK       = (volatile uint32_t *) pREG_UART0_CLK;
            uart->pREG_UART_STAT      = (volatile uint32_t *) pREG_UART0_STAT;
            uart->pREG_UART_THR       = (volatile uint32_t *) pREG_UART0_THR;
            uart->pREG_UART_RBR       = (volatile uint32_t *) pREG_UART0_RBR;
            uart->pREG_UART_IMSK_CLR  = (volatile uint32_t *) pREG_UART0_IMSK_CLR;
            uart->pREG_UART_IMSK_SET  = (volatile uint32_t *) pREG_UART0_IMSK_SET;
            uart->pREG_UART_STAT      = (volatile uint32_t *) pREG_UART0_STAT;

            uart->UART_STATUS_IRQ_ID    = INTR_UART0_STAT;
        }
        else if (port == UART1) {
            uart->pREG_UART_CTL       = (volatile uint32_t *) pREG_UART1_CTL;
            uart->pREG_UART_CLK       = (volatile uint32_t *) pREG_UART1_CLK;
            uart->pREG_UART_STAT      = (volatile uint32_t *) pREG_UART1_STAT;
            uart->pREG_UART_THR       = (volatile uint32_t *) pREG_UART1_THR;
            uart->pREG_UART_RBR       = (volatile uint32_t *) pREG_UART1_RBR;
            uart->pREG_UART_IMSK_CLR  = (volatile uint32_t *) pREG_UART1_IMSK_CLR;
            uart->pREG_UART_IMSK_SET  = (volatile uint32_t *) pREG_UART1_IMSK_SET;
            uart->pREG_UART_STAT      = (volatile uint32_t *) pREG_UART1_STAT;

            uart->UART_STATUS_IRQ_ID    = INTR_UART1_STAT;
        }
        else if (port == UART2) {
            uart->pREG_UART_CTL       = (volatile uint32_t *) pREG_UART2_CTL;
            uart->pREG_UART_CLK       = (volatile uint32_t *) pREG_UART2_CLK;
            uart->pREG_UART_STAT      = (volatile uint32_t *) pREG_UART2_STAT;
            uart->pREG_UART_THR       = (volatile uint32_t *) pREG_UART2_THR;
            uart->pREG_UART_RBR       = (volatile uint32_t *) pREG_UART2_RBR;
            uart->pREG_UART_IMSK_CLR  = (volatile uint32_t *) pREG_UART2_IMSK_CLR;
            uart->pREG_UART_IMSK_SET  = (volatile uint32_t *) pREG_UART2_IMSK_SET;
            uart->pREG_UART_STAT      = (volatile uint32_t *) pREG_UART2_STAT;

            uart->UART_STATUS_IRQ_ID    = INTR_UART2_STAT;
        }

#ifdef FREE_RTOS
#if 1
        uart->portLock = xSemaphoreCreateCounting(1, 1);
        uart->portRxLock = xSemaphoreCreateCounting(1, 1);
        uart->portTxLock = xSemaphoreCreateCounting(1, 1);
#else
        /*
         * There's an FreeRTOS assert at line 408 of tasks.c that is
         * triggered when the semaphore is created as a mutex.
         */
        uart->portLock = xSemaphoreCreateMutex();
        uart->portRxLock = xSemaphoreCreateMutex();
        uart->portTxLock = xSemaphoreCreateMutex();
#endif
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

    uart_initialized = true;

    return(result);
}


UART_SIMPLE_RESULT uart_deinit(void)
{
    UART_SIMPLE_RESULT result = UART_SIMPLE_SUCCESS;
    uint8_t port;
    sUART *uart;

    for (port = UART0; port < UART_END; port++) {

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
