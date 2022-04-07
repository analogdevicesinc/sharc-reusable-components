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
 * @brief  Simple UART compatibility interface on top of the CLD
 *         CDC + UAC library.
 *
 *   This UART driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Fully protected multi-threaded device transfers
 *     - Blocking transfers
 *
 * @file      uart_simple_cdc.h
 * @version   1.0.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/
#ifndef _UART_SIMPLE_CDC_H
#define _UART_SIMPLE_CDC_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/******************************************************************
 * Import the standard UART defines from uart_simple.h for
 * compatibility.
 ******************************************************************/
#define UART_SIMPLE_DEFINES_ONLY
#include "uart_simple.h"

/*!****************************************************************
 * @brief Opaque Simple UART driver handle type.
 ******************************************************************/
typedef struct sUART sUART;

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 *  @brief Simple UART driver initialization routine.
 *
 * This function initializes the simple UART driver.  It should be
 * called once at program start-up.
 *
 * If using the UART driver under FreeRTOS, this function can be
 * called before or after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_init(void);

/*!****************************************************************
 *  @brief Simple UART driver deinitialization routine.
 *
 * This function frees all resources allocated by the simple UART
 * driver.  It should be called once at program shut-down.
 *
 * If using the UART driver under FreeRTOS, this function should be
 * called after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_deinit(void);

/*!****************************************************************
 * @brief Simple UART driver port open.
 *
 * This function opens a hardware UART port.
 *
 * If using the UART driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  port        UART port number to open
 * @param [out] uartHandle  A pointer to an opaque simple UART (sUART)
 *                          handle.
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_open(UART_SIMPLE_PORT port, sUART **uartHandle);

/*!****************************************************************
 * @brief Simple UART driver port close.
 *
 * This function closes a hardware UART port.
 *
 * If using the UART driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out]  uartHandle  UART handle to close
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_close(sUART **uartHandle);

/*!****************************************************************
 * @brief Simple UART device set protocol.
 *
 * This function sets the protocol parameters for the UART.  These
 * include speed, word length, parity, and stop bits.
 *
 * If using the UART driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] uartHandle   A handle to a UART device
 * @param [in] speed        The baud rate for this port
 * @param [in] length       The word length for this port
 * @param [in] parity       The parity setting for this port
 * @param [in] stop         The stop bit setting for this port
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_setProtocol(sUART *uartHandle,
    UART_SIMPLE_SPEED speed, UART_SIMPLE_WORD_LENGTH length,
    UART_SIMPLE_PARITY parity, UART_SIMPLE_STOP_BITS stop);

/*!****************************************************************
 * @brief Simple UART device set rx and tx timeouts.
 *
 * This function sets the blocking mode for UART reads and writes.
 *
 * If using the UART driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] uartHandle      A handle to a UART device
 * @param [in] readTimeout     Read timeout in mS
 * @param [in] writeTimeout    Write timeout in mS
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_setTimeouts(sUART *uartHandle,
    int32_t readTimeout, int32_t writeTimeout);

/*!****************************************************************
 * @brief Simple UART read.
 *
 * This function performs a read from the UART port
 *
 * If using the UART driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.  If multiple threads request to read
 * from the same device, each thread will sequentially read a contiguous
 * stream of bytes.
 *
 * @param [in]     uartHandle  A handle to a UART port
 * @param [out]    in          Pointer to buffer to receive data
 * @param [in,out] inLen       Number of bytes to read (in).
 *                             Number of bytes actually read (out)
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_read(sUART *uartHandle, uint8_t *in,
    uint8_t *inLen);

/*!****************************************************************
 * @brief Simple UART write.
 *
 * This function performs a write to the UART port
 *
 * If using the UART driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.  If multiple threads request to write
 * to the same device, each thread will sequentially write a contiguous
 * stream of bytes.
 *
 * @param [in] uartHandle  A handle to a UART port
 * @param [in] out         Pointer to buffer containing write data
 * @param [in,out] outLen  Number of bytes to write (in).
 *                         Number of bytes actually written (out)
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_cdc_write(sUART *uartHandle, uint8_t *out,
    uint8_t *outLen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
