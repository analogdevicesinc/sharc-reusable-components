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
 * @brief  Simple, efficient, RTOS or bare metal generic flash driver
 *         interface
 *
 * @file      flash.h
 * @version   1.0.0
 * @copyright 2018 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#include "spi_simple.h"

/*!****************************************************************
 * @brief   Flash result codes
 ******************************************************************/
#define FLASH_OK        (0)   /**< Flash operation successful */
#define FLASH_ERROR     (-1)  /**< Flash operation failed */

typedef struct _FLASH_INFO FLASH_INFO;

/*!****************************************************************
 * @brief  Simple flash read.
 *
 * This function reads from a flash device.
 *
 * This function is thread safe.
 *
 * @param [in]   fi      A handle to the flash device to read
 * @param [in]   addr    The address of the flash device to read
 * @param [out]  buf     The buffer to read into
 * @param [in]   size    The number of bytes to read
 *
 * @return Returns FLASH_OK if successful, otherwise
 *         an error.
 ******************************************************************/
int flash_read(const FLASH_INFO *fi, uint32_t addr, uint8_t *buf, int size);

/*!****************************************************************
 * @brief  Simple flash erase.
 *
 * This function erases a section of a flash device.  Note that erase
 * operations are extended, on both sides, to align with the device's
 * erase block boundaries.
 *
 * This function is thread safe.
 *
 * @param [in]   fi      A handle to the flash device to erase
 * @param [in]   addr    The address of the flash device to erase
 * @param [in]   size    The number of bytes to erase
 *
 * @return Returns FLASH_OK if successful, otherwise
 *         an error.
 ******************************************************************/
int flash_erase(const FLASH_INFO *fi, uint32_t addr, int size);

/*!****************************************************************
 * @brief  Simple flash program.
 *
 * This function programs a section of a flash device.  The program
 * area does not have to align with flash erase boundaries.  Areas
 * already programmed will not be erased.
 *
 * This function is thread safe.
 *
 * @param [in]   fi      A handle to the flash device to program
 * @param [in]   addr    The address of the flash device to program
 * @param [in]   buf     The buffer to the data to be programmed
 * @param [in]   size    The number of bytes to program
 *
 * @return Returns FLASH_OK if successful, otherwise
 *         an error.
 ******************************************************************/
int flash_program(const FLASH_INFO *fi, uint32_t addr, const uint8_t *buf, int size);

/*!****************************************************************
 * @brief   Flash handle (flash info)
 *
 * The application code must not set, modify, or otherwise call
 * the functions contained within this structure directly.  It is
 * configured by the device specific driver for use by the generic
 * flash driver interface.
 ******************************************************************/
struct _FLASH_INFO {
    /** Flash info handle */
    sSPIPeriph *flashHandle;
    /** Flash UID (Filled during init if present) */
    uint8_t UID[16];
    /** Flash device driver read function */
    int (*flash_read)(const FLASH_INFO *fi, uint32_t addr, uint8_t *buf, int size);
    /** Flash device driver erase function */
    int (*flash_erase)(const FLASH_INFO *fi, uint32_t addr, int size);
    /** Flash device driver program function */
    int (*flash_program)(const FLASH_INFO *fi, uint32_t addr, const uint8_t *buf, int size);
};

#endif /* FLASH_H */
