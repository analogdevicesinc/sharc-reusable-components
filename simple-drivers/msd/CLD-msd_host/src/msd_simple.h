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
 * @brief     Simple, efficient, RTOS or bare metal Mass Storage
 *            Device driver.  This driver layers on top of the
 *            msd_host simple driver which layers on top of the
 *            CLD Mass Storage Device driver library.
 *
 *   This Mass Storage Device (MSD) driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Fully protected multi-threaded device transfers
 *
 * @file      msd_simple.h
 * @version   1.0.0
 * @copyright 2021 Analog Devices, Inc.  All rights reserved.
 *
*/

#ifndef __ADI_MSD_SIMPLE_H__
#define __ADI_MSD_SIMPLE_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/platform.h>

/*!****************************************************************
 * @brief Hardware MSD port.
 ******************************************************************/
typedef enum MSD_SIMPLE_PORT {
    MSD0 = (0),      /**< MSD port 0 */
    MSD_END          /**< End of MSD ports */
} MSD_SIMPLE_PORT;


/*!****************************************************************
 * @brief Simple MSD driver API result codes.
 ******************************************************************/
typedef enum MSD_SIMPLE_RESULT {
    MSD_SIMPLE_SUCCESS,          /**< No error */
    MSD_SIMPLE_INVALID_PORT,     /**< Invalid MSD port open */
    MSD_SIMPLE_PORT_BUSY,        /**< MSD port is already opened */
    MSD_SIMPLE_ERROR             /**< Generic error */
} MSD_SIMPLE_RESULT;

/*!****************************************************************
 * @brief Simple MSD info
 ******************************************************************/
typedef struct _MSD_SIMPLE_INFO {
    uint64_t capacity;
} MSD_SIMPLE_INFO;

/*!****************************************************************
 * @brief Opaque Simple MSD device handle type.
 ******************************************************************/
typedef struct sMSD sMSD;

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 *  @brief Simple MSD driver initialization routine.
 *
 * This function initializes the simple MSD driver.  It should be
 * called once at program start-up.
 *
 * If using the MSD driver under FreeRTOS, this function can be
 * called before or after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns MSD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
MSD_SIMPLE_RESULT msd_init(void);

/*!****************************************************************
 *  @brief Simple MSD driver deinitialization routine.
 *
 * This function frees all resources allocated by the simple MSD
 * driver.  It should be called once at program shut-down.
 *
 * If using the MSD driver under FreeRTOS, this function should be
 * called after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns MSD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
MSD_SIMPLE_RESULT msd_deinit(void);

/*!****************************************************************
 * @brief Simple MSD driver port open.
 *
 * This function opens a hardware MSD port.
 *
 * If using the MSD driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  port       MSD port number to open
 * @param [out] msdHandle  A pointer to an opaque simple MSD (sMSD)
 *                         handle.
 *
 * @return Returns MSD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
MSD_SIMPLE_RESULT msd_open(MSD_SIMPLE_PORT port, sMSD **msdHandle);

/*!****************************************************************
 * @brief Simple MSD driver port close.
 *
 * This function closes a hardware MSD port.
 *
 * If using the MSD driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out]  msdHandle  MSD handle to close
 *
 * @return Returns MSD_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
MSD_SIMPLE_RESULT msd_close(sMSD **msdHandle);


MSD_SIMPLE_RESULT msd_present(sMSD *msdHandle);
MSD_SIMPLE_RESULT msd_info(sMSD *msdHandle, MSD_SIMPLE_INFO *info);
MSD_SIMPLE_RESULT msd_write(sMSD *msd, void *data, uint64_t sector, uint32_t count);
MSD_SIMPLE_RESULT msd_read(sMSD *msd, void *data, uint64_t sector, uint32_t count);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

