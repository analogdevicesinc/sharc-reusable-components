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
 * @brief  Cypress s25fl512s specific driver for use with the Simple
 *         SPI device driver and generic flash interface driver.
 *
 * @file       s25fl512s.h
 * @version    1.0.0
 * @copyright  2020 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef _s25fl512s_h
#define _s25fl512s_h

#include "flash.h"

/*!****************************************************************
 * @brief  Cypress s25fl512s device driver open.
 *
 * This function returns a flash info handle for use by the generic
 * flash interface driver.
 *
 * This function is thread safe.
 *
 * @param [in]   spiFlashHandle    A pointer to a fully configured
 *                                 simple SPI driver handle.
 *
 * @return Returns a pointer to a s25fl512s device driver or NULL
 *         on error.
 ******************************************************************/
FLASH_INFO *s25fl512s_open(sSPIPeriph *spiFlashHandle);

/*!****************************************************************
 * @brief  Cypress s25fl512s device driver close.
 *
 * This function closes a flash info handle.
 *
 * This function is thread safe.
 *
 * @param [in]   fi    A pointer to a valid flash flash info handle.
 *
 * @return FLASH_OK on success or FLASH_FAIL on error.
 ******************************************************************/
int s25fl512s_close(const FLASH_INFO *fi);

#endif
