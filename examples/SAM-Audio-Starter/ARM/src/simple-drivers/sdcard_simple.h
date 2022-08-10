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
 * @version   1.0.0
 * @copyright 2021 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef __ADI_SDCARD_SIMPLE_H__
#define __ADI_SDCARD_SIMPLE_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/platform.h>

#include "clocks.h"

/*!****************************************************************
 * @brief Default base MSI CLK to 100MHz if not otherwise defined
 *        in clocks.h
 ******************************************************************/
#ifndef SDCLK
#define SDCLK  100000000
#endif

/*!****************************************************************
 * @brief Hardware SDCARD port.
 ******************************************************************/
typedef enum SDCARD_SIMPLE_PORT {
    SDCARD0 = (0),      /**< SDCARD port 0 */
    SDCARD_END          /**< End of SDCARD ports */
} SDCARD_SIMPLE_PORT;


/*!****************************************************************
 * @brief Simple SDCARD driver API result codes.
 ******************************************************************/
typedef enum SDCARD_SIMPLE_RESULT {
    SDCARD_SIMPLE_SUCCESS,          /**< No error */
    SDCARD_SIMPLE_INVALID_PORT,     /**< Invalid SDCARD port open */
    SDCARD_SIMPLE_PORT_BUSY,        /**< SDCARD port is already opened */
    SDCARD_SIMPLE_ERROR             /**< Generic error */
} SDCARD_SIMPLE_RESULT;

/*!****************************************************************
 * @brief Simple SDCARD types.
 ******************************************************************/
#define SDCARD_TYPE_STRINGS { \
    "SDv2", \
    "SDv2 HC", \
    "Unusable" \
}

typedef enum _SDCARD_SIMPLE_TYPE {
    SDCARD_TYPE_SD_V2X = 0,
    SDCARD_TYPE_SD_V2X_HIGH_CAPACITY,
    SDCARD_UNUSABLE_CARD,
} SDCARD_SIMPLE_TYPE;

/*!****************************************************************
 * @brief Simple SDCARD info
 ******************************************************************/
typedef struct _SDCARD_SIMPLE_INFO {
    SDCARD_SIMPLE_TYPE type;
    uint64_t capacity;
} SDCARD_SIMPLE_INFO;

/*!****************************************************************
 * @brief Opaque Simple SDCARD device handle type.
 ******************************************************************/
typedef struct sSDCARD sSDCARD;

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 *  @brief Simple SDCARD driver initialization routine.
 *
 * This function initializes the simple SDCARD driver.  It should be
 * called once at program start-up.
 *
 * If using the SDCARD driver under FreeRTOS, this function can be
 * called before or after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns SDCARD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SDCARD_SIMPLE_RESULT sdcard_init(void);

/*!****************************************************************
 *  @brief Simple SDCARD driver deinitialization routine.
 *
 * This function frees all resources allocated by the simple SDCARD
 * driver.  It should be called once at program shut-down.
 *
 * If using the SDCARD driver under FreeRTOS, this function should be
 * called after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns SDCARD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SDCARD_SIMPLE_RESULT sdcard_deinit(void);

/*!****************************************************************
 * @brief Simple SDCARD driver port open.
 *
 * This function opens a hardware SDCARD port.
 *
 * If using the SDCARD driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  port       SDCARD port number to open
 * @param [out] sdcardHandle  A pointer to an opaque simple SDCARD (sSDCARD)
 *                         handle.
 *
 * @return Returns SDCARD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SDCARD_SIMPLE_RESULT sdcard_open(SDCARD_SIMPLE_PORT port, sSDCARD **sdcardHandle);

/*!****************************************************************
 * @brief Simple SDCARD driver port close.
 *
 * This function closes a hardware SDCARD port.
 *
 * If using the SDCARD driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out]  sdcardHandle  SDCARD handle to close
 *
 * @return Returns SDCARD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SDCARD_SIMPLE_RESULT sdcard_close(sSDCARD **sdcardHandle);


SDCARD_SIMPLE_RESULT sdcard_present(sSDCARD *sdcardHandle);
SDCARD_SIMPLE_RESULT sdcard_start(sSDCARD *sdcardHandle);
SDCARD_SIMPLE_RESULT sdcard_stop(sSDCARD *sdcardHandle);

SDCARD_SIMPLE_RESULT sdcard_info(sSDCARD *sdcardHandle, SDCARD_SIMPLE_INFO *info);
SDCARD_SIMPLE_RESULT sdcard_write(sSDCARD *sdcard, void *data, uint32_t sector, uint32_t count);
SDCARD_SIMPLE_RESULT sdcard_read(sSDCARD *sdcard, void *data, uint32_t sector, uint32_t count);

SDCARD_SIMPLE_RESULT sdcard_readyForData(sSDCARD *sdcard);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
