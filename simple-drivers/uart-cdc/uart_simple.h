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
 * @brief  Simple, efficient, RTOS or bare metal master mode UART driver
 *
 *   This UART driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Fully protected multi-threaded device transfers
 *     - Blocking transfers
 *
 * @file      uart_simple.h
 * @version   1.0.0
 * @copyright 2018 Analog Devices, Inc.  All rights reserved.
 *
*/
#ifndef _UART_SIMPLE_H
#define _UART_SIMPLE_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "clocks.h"

/*!****************************************************************
 * @brief Default base SCLK0 to 100MHz if not otherwise defined
 *        in clocks.h
 ******************************************************************/
#ifndef SCLK0
#define SCLK0  100000000
#endif

/*!****************************************************************
 * @brief Hardware UART ports.
 ******************************************************************/
typedef enum UART_SIMPLE_PORT {
    UART0 = (0),      /**< UART port 0 */
    UART1 = (1),      /**< UART port 1 */
    UART2 = (2),      /**< UART port 2 */
    UART_END          /**< End UART ports */
} UART_SIMPLE_PORT;

/*!****************************************************************
 * @brief Standard Simple UART driver baud rates.
 ******************************************************************/
typedef enum UART_SIMPLE_SPEED {
    UART_SIMPLE_BAUD_NONE = 0,   /**< No UART baud rate (do not use) */
    UART_SIMPLE_BAUD_110,        /**< 110 bps    */
    UART_SIMPLE_BAUD_300,        /**< 300 bps    */
    UART_SIMPLE_BAUD_600,        /**< 600 bps    */
    UART_SIMPLE_BAUD_1200,       /**< 1200 bps   */
    UART_SIMPLE_BAUD_2400,       /**< 2400 bps   */
    UART_SIMPLE_BAUD_4800,       /**< 4800 bps   */
    UART_SIMPLE_BAUD_9600,       /**< 9600 bps   */
    UART_SIMPLE_BAUD_14400,      /**< 14400 bps  */
    UART_SIMPLE_BAUD_19200,      /**< 19200 bps  */
    UART_SIMPLE_BAUD_28800,      /**< 28800 bps  */
    UART_SIMPLE_BAUD_31250,      /**< 31250 bps (MIDI) */
    UART_SIMPLE_BAUD_38400,      /**< 38400 bps  */
    UART_SIMPLE_BAUD_56000,      /**< 56000 bps  */
    UART_SIMPLE_BAUD_57600,      /**< 57600 bps  */
    UART_SIMPLE_BAUD_115200,     /**< 115200 bps (higher power mode) */
    UART_SIMPLE_BAUD_230400,     /**< 230400 bps (higher power mode) */
    UART_SIMPLE_BAUD_256000,     /**< 256000 bps (higher power mode) */
    UART_SIMPLE_BAUD_460800,     /**< 468000 bps (higher power mode) */
    UART_SIMPLE_BAUD_921600      /**< 921600 bps (higher power mode) */
} UART_SIMPLE_SPEED;

/*!****************************************************************
 * @brief Standard Simple MIDI baud rate.
 ******************************************************************/
#define UART_BAUD_RATE_MIDI (UART_BAUD_RATE_31250) /**< 31250 bps (MIDI) */

/*!****************************************************************
 * @brief Standard Simple UART driver word size.
 ******************************************************************/
typedef enum UART_SIMPLE_WORD_LENGTH {
    UART_SIMPLE_5BIT = 0,   /**< 5-bit word size */
    UART_SIMPLE_6BIT,       /**< 6-bit word size */
    UART_SIMPLE_7BIT,       /**< 7-bit word size */
    UART_SIMPLE_8BIT        /**< 8-bit word size */
} UART_SIMPLE_WORD_LENGTH;

/*!****************************************************************
 * @brief Standard Simple UART driver stop bits.
 ******************************************************************/
typedef enum UART_SIMPLE_STOP_BITS {
    UART_SIMPLE_STOP_BITS1 = 0,    /**< 1 stop bit */
    UART_SIMPLE_STOP_BITS2         /**< 2 stop bits */
} UART_SIMPLE_STOP_BITS;

/*!****************************************************************
 * @brief Standard Simple UART driver parity.
 ******************************************************************/
typedef enum UART_SIMPLE_PARITY {
    UART_SIMPLE_PARITY_DISABLE = 0,     /**< No parity */
    UART_SIMPLE_PARITY_EVEN,            /**< Even parity */
    UART_SIMPLE_PARITY_ODD,             /**< Odd parity */
} UART_SIMPLE_PARITY;

/*!****************************************************************
 * @brief Simple UART driver blocking timeout values.
 ******************************************************************/
enum UART_SIMPLE_BLOCKING_MODE {
    UART_SIMPLE_TIMEOUT_NONE       =  0,   /**< Do not block */
    UART_SIMPLE_TIMEOUT_INF        = -1,   /**< Block infinitely */
    UART_SIMPLE_TIMEOUT_NO_CHANGE  = -2,   /**< Do not change */
};

/*!****************************************************************
 * @brief Simple UART driver API result codes.
 ******************************************************************/
typedef enum UART_SIMPLE_RESULT
{
    UART_SIMPLE_SUCCESS = 0,         /**< No error */
    UART_SIMPLE_ERROR,               /**< Generic error */
    UART_SIMPLE_INVALID_PORT,        /**< Invalid UART port open */
    UART_SIMPLE_PORT_BUSY,           /**< UART port is already opened */
    UART_SIMPLE_RX_FIFO_FULL,        /**< Internal driver error, not returned */
    UART_SIMPLE_TX_FIFO_FULL,        /**< Internal driver error, not returned */
} UART_SIMPLE_RESULT;

/*!****************************************************************
 * @brief Opaque Simple UART driver handle type.
 ******************************************************************/
typedef struct sUART sUART;

#ifndef UART_SIMPLE_DEFINES_ONLY

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
UART_SIMPLE_RESULT uart_init(void);

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
UART_SIMPLE_RESULT uart_deinit(void);

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
UART_SIMPLE_RESULT uart_open(UART_SIMPLE_PORT port, sUART **uartHandle);

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
UART_SIMPLE_RESULT uart_close(sUART **uartHandle);

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
UART_SIMPLE_RESULT uart_setProtocol(sUART *uartHandle,
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
UART_SIMPLE_RESULT uart_setTimeouts(sUART *uartHandle,
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
 * @param [in]  uartHandle  A handle to a UART port
 * @param [out] in          Pointer to buffer to receive data
 * @param [in,out] inLen       Number of bytes to read (in).
 *                             Number of bytes actually read (out)
 *
 * @return Returns UART_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
UART_SIMPLE_RESULT uart_read(sUART *uartHandle, uint8_t *in, uint8_t *inLen);

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
UART_SIMPLE_RESULT uart_write(sUART *uartHandle, uint8_t *out, uint8_t *outLen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UART_SIMPLE_DEFINES_ONLY

#endif //_UART_SIMPLE_H
