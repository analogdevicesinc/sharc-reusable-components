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
 * @brief  Simple, efficient, RTOS or bare metal master mode TWI driver
 *
 *   This TWI driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Fully protected multi-threaded device transfers
 *     - Blocking transfers
 *
 * @file      twi_simple.h
 * @version   1.0.0
 * @copyright 2018 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef __ADI_TWI_SIMPLE_H__
#define __ADI_TWI_SIMPLE_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/platform.h>

#include "clocks.h"

/*!****************************************************************
 * @brief Default base SCLK0 to 100MHz if not otherwise defined
 *        in clocks.h
 ******************************************************************/
#ifndef SCLK0
#define SCLK0  100000000
#endif

/*!****************************************************************
 * @brief Hardware TWI ports.
 ******************************************************************/
typedef enum TWI_SIMPLE_PORT {
    TWI0    = (0),      /**< TWI port 0 */
    TWI1    = (1),      /**< TWI port 1 */
    TWI2    = (2),      /**< TWI port 2 */
    TWI_END             /**< End TWI ports */
} TWI_SIMPLE_PORT;

/*!****************************************************************
 * @brief Standard Simple TWI driver speed options.
 ******************************************************************/
typedef enum TWI_SIMPLE_SPEED {
    TWI_SIMPLE_SPEED_100 = 100u,       /**< TWI speed 100KHz */
    TWI_SIMPLE_SPEED_400 = 400u        /**< TWI speed 400KHz */
} TWI_SIMPLE_SPEED;

/*!****************************************************************
 * @brief Simple TWI driver API result codes.
 ******************************************************************/
typedef enum TWI_SIMPLE_RESULT {
    TWI_SIMPLE_SUCCESS,          /**< No error */
    TWI_SIMPLE_INVALID_PORT,     /**< Invalid TWI port open */
    TWI_SIMPLE_PORT_BUSY,        /**< TWI port is already opened */
    TWI_SIMPLE_ERROR,            /**< Generic error */
    TWI_SIMPLE_BAD_LENGTH        /**< Transfer length is too long (>254) */
} TWI_SIMPLE_RESULT;

/*!****************************************************************
 * @brief Opaque Simple TWI driver handle type.
 ******************************************************************/
typedef struct sTWI sTWI;

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 *  @brief Simple TWI driver initialization routine.
 *
 * This function initializes the simple TWI driver.  It should be
 * called once at program start-up.
 *
 * If using the TWI driver under FreeRTOS, this function can be
 * called before or after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_init(void);

/*!****************************************************************
 *  @brief Simple TWI driver deinitialization routine.
 *
 * This function frees all resources allocated by the simple TWI
 * driver.  It should be called once at program shut-down.
 *
 * If using the TWI driver under FreeRTOS, this function should be
 * called after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_deinit(void);

/*!****************************************************************
 * @brief Simple TWI driver port open.
 *
 * This function opens a hardware TWI port.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  port       TWI port number to open
 * @param [out] twiHandle  A pointer to an opaque simple TWI (sTWI)
 *                         handle.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_open(TWI_SIMPLE_PORT port, sTWI **twiHandle);

/*!****************************************************************
 * @brief Simple TWI driver port close.
 *
 * This function closes a hardware TWI port.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out]  twiHandle  TWI handle to close
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_close(sTWI **twiHandle);

/*!****************************************************************
 * @brief Simple TWI device set speed.
 *
 * This function sets the speed of the TWI port.  The speed must be
 * set to the speed of the slowest device on the port.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] twiHandle    A handle to a TWI device
 * @param [in] speed        The TWI speed for this port
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_setSpeed(sTWI *twiHandle, TWI_SIMPLE_SPEED speed);

/*!****************************************************************
 * @brief Simple TWI read.
 *
 * This function performs a single TWI read of a device on the
 * TWI port.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  twiHandle  A handle to a TWI port
 * @param [in]  address    Address of the device to be read
 * @param [out] in         Pointer to buffer to receive data
 * @param [in]  inLen      Number of bytes to read.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_read(sTWI *twiHandle, uint8_t address,
    uint8_t *in, uint16_t inLen);

/*!****************************************************************
 * @brief Simple TWI write.
 *
 * This function performs a single TWI write to a device on the
 * TWI port.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in] twiHandle   A handle to a TWI port
 * @param [in] address     Address of the device to be written
 * @param [in] out         Pointer to buffer containing write data
 * @param [in] outLen      Number of bytes to write.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_write(sTWI *twiHandle, uint8_t address,
    uint8_t *out, uint16_t outLen);

/*!****************************************************************
 * @brief Simple TWI write-read.
 *
 * This function performs a TWI write/read sequence with a repeated
 * start condition in-between to a device on the TWI port.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  twiHandle  A handle to a TWI port
 * @param [in]  address    Address of the device to be written
 * @param [in]  out        Pointer to buffer containing write data
 * @param [in]  outLen     Number of bytes to write.  Must be less than
 *                         or equal to 254.
 * @param [out] in         Pointer to buffer to receive data
 * @param [in]  inLen      Number of bytes to read.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_writeRead(sTWI *twiHandle, uint8_t address,
    uint8_t *out, uint16_t outLen, uint8_t *in, uint16_t inLen);

/*!****************************************************************
 * @brief Simple TWI write-write.
 *
 * This function performs an atomic TWI write of two data buffers
 * with no interruptions.
 *
 * If using the TWI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  twiHandle  A handle to a TWI port
 * @param [in]  address    Address of the device to be written
 * @param [in]  out        Pointer to buffer containing write data
 * @param [in]  outLen     Number of bytes to write.
 * @param [out] out2       Pointer to buffer containing additional write data
 * @param [in]  out2Len    Number of additional bytes to write.
 *
 * @return Returns TWI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
TWI_SIMPLE_RESULT twi_writeWrite(sTWI *twiHandle, uint8_t address,
    uint8_t *out, uint16_t outLen, uint8_t *out2, uint16_t out2Len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
