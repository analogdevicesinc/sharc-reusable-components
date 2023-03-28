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
 * @brief  Winbond w25q128fv specific driver for use with the Simple
 *         SPI device driver and generic flash interface driver.
 *
 * @file       w25q128fv.h
 * @version    1.0.0
 * @copyright  2019 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef _w25q128fv_h
#define _w25q128fv_h

#include "flash.h"

/*!****************************************************************
 * @brief  Winbond w25q128fv device driver open.
 *
 * This function returns a flash info handle for use by the generic
 * flash interface driver.
 *
 * This function is thread safe.
 *
 * @param [in]   spiFlashHandle    A pointer to a fully configured
 *                                 simple SPI driver handle.
 *
 * @return Returns a pointer to a w25q128fv device driver or NULL
 *         on error.
 ******************************************************************/
FLASH_INFO *w25q128fv_open(sSPIPeriph *spiFlashHandle);

/*!****************************************************************
 * @brief  Winbond w25q128fv device driver close.
 *
 * This function closes a flash info handle.
 *
 * This function is thread safe.
 *
 * @param [in]   fi    A pointer to a valid flash flash info handle.
 *
 * @return FLASH_OK on success or FLASH_FAIL on error.
 ******************************************************************/
int w25q128fv_close(const FLASH_INFO *fi);

#endif
