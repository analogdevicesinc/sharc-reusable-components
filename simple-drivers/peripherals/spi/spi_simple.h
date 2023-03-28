/**
 * Copyright (c) 2022 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

/*!
 * @brief     Simple, efficient, RTOS or bare metal master mode SPI driver
 *
 *   This SPI driver supports:
 *     - FreeRTOS or no RTOS main-loop modes
 *     - Multiple devices, with independent configurations, on a SPI port
 *     - Fully protected multi-threaded device transfers
 *     - Rx/Tx, Rx only, or Tx only DMA transfers
 *     - 8/16/32 bit data device transfers
 *     - Fully managed slave select or application call-back
 *     - Standard, Dual, or Quad I/O device transfers
 *     - Multiple transfers within a single atomic slave select
 *     - Blocking transfers
 *
 * @file      spi_simple.h
 * @version   1.0.0
 *
*/

#ifndef __ADI_SPI_SIMPLE_H__
#define __ADI_SPI_SIMPLE_H__

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
 * @brief Maximum number of slave peripheral devices.
 *
 * Redefine this as necessary in the application to match the
 * maximum number of SPI peripheral devices to be managed by this
 * driver.  This includes all SPI peripheral devices across all
 * SPI ports.
 ******************************************************************/
#ifndef SPI_SIMPLE_MAX_DEVICES
#define SPI_SIMPLE_MAX_DEVICES   (4)
#endif

/*!****************************************************************
 * @brief Hardware SPI port.
 ******************************************************************/
typedef enum SPI_SIMPLE_PORT {
    SPI0 = (0),      /**< SPI port 0 */
    SPI1 = (1),      /**< SPI port 1 */
    SPI2 = (2),      /**< SPI port 2 */
#if defined(__ADSPSC594_FAMILY__)
    SPI3 = (3),      /**< SPI port 3 */
#endif
    SPI_END          /**< End of SPI ports */
} SPI_SIMPLE_PORT;


/*!****************************************************************
 * @brief Simple SPI driver API result codes.
 ******************************************************************/
typedef enum SPI_SIMPLE_RESULT
{
    SPI_SIMPLE_SUCCESS,          /**< No error */
    SPI_SIMPLE_INVALID_PORT,     /**< Invalid SPI port open */
    SPI_SIMPLE_NO_MORE_DEVICES,  /**< No more device handles available */
    SPI_SIMPLE_PORT_BUSY,        /**< SPI port is already opened */
    SPI_SIMPLE_ERROR             /**< Generic error */
} SPI_SIMPLE_RESULT;


/*!****************************************************************
 * @brief Transfer word size (default is SPI_WORDSIZE_8BIT)
 ******************************************************************/
typedef enum SPI_SIMPLE_WORDSIZE {
    SPI_WORDSIZE_8BIT    = (0),  /**< 8-bit transfers */
    SPI_WORDSIZE_16BIT   = (1),  /**< 16-bit transfers */
    SPI_WORDSIZE_32BIT   = (2)   /**< 32-bit transfers */
} SPI_SIMPLE_WORDSIZE;


/*!****************************************************************
 * @brief Transfer mode (default is SPI_MODE_0)
 *
 * For further details, refer to section 16-6 of the ADSP-SC58x
 * SHARC+ Processor Hardware Manual, Rev 1.0.
 ******************************************************************/
typedef enum SPI_SIMPLE_MODE {
    SPI_MODE_0  = (0),  /**< SPI mode 0 */
    SPI_MODE_1  = (1),  /**< SPI mode 1 */
    SPI_MODE_2  = (2),  /**< SPI mode 2 */
    SPI_MODE_3  = (3)   /**< SPI mode 3 */
} SPI_SIMPLE_MODE;


/*!****************************************************************
 * @brief Transfer slave select (default is NONE)
 *
 * For further details, refer to section 16-10 of the ADSP-SC58x
 * SHARC+ Processor Hardware Manual, Rev 1.0.
 ******************************************************************/
typedef enum SPI_SIMPLE_SLAVE_SEL {
    SPI_SSEL_NONE   = (0),  /**< No SPI select */
    SPI_SSEL_1      = (1),  /**< Hardware SPI select 1 */
    SPI_SSEL_2      = (2),  /**< Hardware SPI select 2 */
    SPI_SSEL_3      = (3),  /**< Hardware SPI select 3 */
    SPI_SSEL_4      = (4),  /**< Hardware SPI select 4 */
    SPI_SSEL_5      = (5),  /**< Hardware SPI select 5 */
    SPI_SSEL_6      = (6),  /**< Hardware SPI select 6 */
    SPI_SSEL_7      = (7)   /**< Hardware SPI select 7 */
} SPI_SIMPLE_SLAVE_SEL;


/*!****************************************************************
 * @brief Transfer I/O flags.
 *
 * For further details, refer to section 16-12 and 16-13 of the
 * ADSP-SC58x SHARC+ Processor Hardware Manual, Rev 1.0.
 ******************************************************************/
typedef enum SPI_SIMPLE_XFER_FLAGS {
    SPI_SIMPLE_XFER_NORMAL_IO = 0x00000000,  /**< Normal mode SPI transfer */
    SPI_SIMPLE_XFER_DUAL_IO   = 0x00000001,  /**< Normal dual mode SPI transfer */
    SPI_SIMPLE_XFER_QUAD_IO   = 0x00000002   /**< Normal quad mode SPI transfer */
} SPI_SIMPLE_XFER_FLAGS;
#define SPI_SIMPLE_XFER_IO_MASK  0x00000003  /**< Transfer mode mask */


/*!****************************************************************
 * @brief Transfer descrption for batch transfers.
 ******************************************************************/
typedef struct sSPIXfer {
    void *rx;          /**< Pointer to receive buffer */
    void *tx;          /**< Pointer to transmit buffer */
    uint16_t len;      /**< Length of buffer in native word size */
    uint32_t flags;    /**< Logical OR of SPI_SIMPLE_XFER_FLAGS */
} sSPIXfer;

/*!****************************************************************
 * @brief Opaque Simple SPI peripheral device handle type.
 ******************************************************************/
typedef struct sSPIPeriph sSPIPeriph;

/*!****************************************************************
 * @brief Opaque Simple SPI driver handle type.
 ******************************************************************/
typedef struct sSPI sSPI;

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 * @brief Slave select call-back.
 *
 * This user defined application function should not block and
 * must not call any Simple SPI functions.
 *
 * @param [in] assert     true = assert /CS, false = deassert /CS
 * @param [in] usrPtr     User supplied data pointer
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *          an error.
 *
 * @sa spi_setSlaveSelectCallback
 ******************************************************************/
typedef void (*SPI_SIMPLE_SLAVE_SELECT_CALLBACK)(bool assert, void *usrPtr);

/*!****************************************************************
 *  @brief Simple SPI driver initialization routine.
 *
 * This function initializes the simple SPI driver.  It should be
 * called once at program start-up.
 *
 * If using the SPI driver under FreeRTOS, this function can be
 * called before or after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_init(void);

/*!****************************************************************
 *  @brief Simple SPI driver deinitialization routine.
 *
 * This function frees all resources allocated by the simple SPI
 * driver.  It should be called once at program shut-down.
 *
 * If using the SPI driver under FreeRTOS, this function should be
 * called after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_deinit(void);

/*!****************************************************************
 * @brief Simple SPI driver port open.
 *
 * This function opens a hardware SPI port.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  port       SPI port number to open
 * @param [out] spiHandle  A pointer to an opaque simple SPI (sSPI)
 *                         handle.
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_open(SPI_SIMPLE_PORT port, sSPI **spiHandle);

/*!****************************************************************
 * @brief Simple SPI driver port close.
 *
 * This function closes a hardware SPI port.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out]  spiHandle  SPI handle to close
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_close(sSPI **spiHandle);

/*!****************************************************************
 * @brief Simple SPI device open.
 *
 * This function opens a SPI device attached to a hardware SPI port.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  spiHandle     SPI port handle
 * @param [out] deviceHandle  A pointer to an opaque simple SPI
 *                            device handle (sSPIPeriph)
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_openDevice(sSPI *spiHandle, sSPIPeriph **deviceHandle);

/*!****************************************************************
 * @brief Simple SPI device close.
 *
 * This function closes a SPI device attached to a hardware SPI port.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out] deviceHandle  A pointer to an opaque simple SPI
 *                               device handle (sSPIPeriph)
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_closeDevice(sSPIPeriph **deviceHandle);

/*!****************************************************************
 * @brief Simple SPI device set mode.
 *
 * This function sets the SPI mode to use for transfers with the
 * device associated with the device handle.  Standard SPI modes
 * 0-3 are supported.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] mode          The SPI transfer mode to use with this
 *                           device.
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setMode(sSPIPeriph *deviceHandle, SPI_SIMPLE_MODE mode);

/*!****************************************************************
 * @brief Simple SPI device set clock
 *
 * This function sets the SPI clock speed to use for transfers with
 * the device associated with the device handle.  The clock value
 * passed in is used directly to set the clock divisor in the SPI
 * port.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] clock         The SPI clock divisor
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setClock(sSPIPeriph *deviceHandle, uint16_t clock);

/*!****************************************************************
 * @brief Simple SPI device set fast mode.
 *
 * This function sets the SPI to use "fast mode".  This mode is
 * recommended for nearly all transfers.  With this mode enabled,
 * data bits are sampled at the end of the clock period instead of
 * in the middle.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] fastMode      true = enabled, false = disabled (default)
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setFastMode(sSPIPeriph *deviceHandle, bool fastMode);

/*!****************************************************************
 * @brief Simple SPI device set lsb first.
 *
 * This function sets whether the LSB or MSB of the data word
 * transfers first.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] lsbFirst      true = LSB, false = MSB (default)
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setLsbFirst(sSPIPeriph *deviceHandle, bool lsbFirst);

/*!****************************************************************
 * @brief Simple SPI device slave select.
 *
 * This function selects the appropriate slave select pin to use
 * with this device.  If no select is desired, then SPI_SSEL_NONE
 * can be selected.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] slaveSelect   One of SPI_SSEL_XXXX.  SPI_SSEL_NONE default.
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setSlaveSelect(sSPIPeriph *deviceHandle,
    SPI_SIMPLE_SLAVE_SEL slaveSelect);

/*!****************************************************************
 * @brief Simple SPI device slave select callback.
 *
 * This function sets a user defined callback to be called to assert
 * and deassert the chip select for the device.  If set, this function
 * takes priority over a select set by spi_setSlaveSelect().
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] cb            Pointer to the user callback function
 * @param [in] usrPtr        Pointer to user defined data which will be
 *                            passed back to the caller
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa SPI_SIMPLE_SLAVE_SELECT_CALLBACK
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setSlaveSelectCallback(sSPIPeriph *deviceHandle,
    SPI_SIMPLE_SLAVE_SELECT_CALLBACK cb, void *usrPtr);

/*!****************************************************************
 * @brief Simple SPI device set word size.
 *
 * This function sets the transfer word size.  8, 16, and 32 bit
 * word sizes are supported.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] wordSize      One of SPI_WORDSIZE_xxxx.
 *                            SPI_WORDSIZE_8BIT default.
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_setWordSize(sSPIPeriph *deviceHandle,
    SPI_SIMPLE_WORDSIZE wordSize);

/*!****************************************************************
 * @brief Simple SPI set quad/dual MSB on MOSI pin.
 *
 * This function sets the whether or not the MSB of the quad
 * or dual transfer word is on the MOSI pin.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] mosi          true = MSB on MOSI pin,
 *                            false = MSB on MISO pin (default)
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_startOnMosi(sSPIPeriph *deviceHandle, bool mosi);

/*!****************************************************************
 * @brief Simple SPI single transfer.
 *
 * This function performs a single bi-directional transfer.  One of
 * either 'rx' or 'tx' can be NULL to set a uni-directional transfer.
 * For bi-directional transfers, the length of the 'rx' and 'tx'
 * buffers must be the same.
 *
 * The buffer type passed into 'rx' and 'tx' must be the same and
 * should be consistent with the value passed to spi_setWordSize().
 *
 * The 'len' is the length of the 'tx' and 'rx' buffers in their
 * native word size.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  deviceHandle  A handle to a SPI device
 * @param [in]  len           Length of the 'rx' and 'tx' buffers
 * @param [out] rx            Pointer to buffer to receive data
 * @param [in]  tx            Pointer to buffer of transmit data
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_xfer(sSPIPeriph *deviceHandle, uint16_t len, void *rx, void *tx);

/*!****************************************************************
 * @brief Simple SPI multiple transfer.
 *
 * This function performs multiple bi-directional transfers in a
 * single chip select.  Each transfer is independent and can be
 * bi-direction, uni-directional, normal, quad, or dual.
 *
 * For any single transfer, one of either 'rx' or 'tx' can be NULL
 * to set a uni-directional transfer.  For bi-directional transfers,
 * the length of the 'rx' and 'tx' buffers must be the same.
 *
 * The buffer type passed into 'rx' and 'tx' must be the same and
 * should be consistent with the value passed to spi_setWordSize().
 *
 * The 'len' is the length of the 'tx' and 'rx' buffers in their
 * native word size.
 *
 * If using the SPI driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe though calling this function from
 * multiple threads is discouraged.
 *
 * @param [in] deviceHandle  A handle to a SPI device
 * @param [in] numXfers      Number of transfers to perform
 * @param [in] xfers         Pointer to the start of an array of
 *                           transfers
 *
 * @return Returns SPI_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPI_SIMPLE_RESULT spi_batch_xfer(sSPIPeriph *deviceHandle, uint16_t numXfers, sSPIXfer *xfers);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
