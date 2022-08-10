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
 * @brief   The SPORT Simple driver provides a simple, efficient, RTOS or bare
 *          metal ping-pong DMA SPORT driver supporting a wide variety of
 *          clocking options.
 *
 * This driver supports:
 *  - FreeRTOS or no RTOS main-loop modes
 *  - Most regularly used audio clocking options
 *  - 16/32 bit data buffers
 *  - Chained DMA transfers
 *  - Graceful start / stop function
 *  - Ping-pong buffer size calculation
 *  - Cache management
 *  - TDM Slot masks
 *
 * @file      sport_simple.h
 * @version   1.1.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/
#ifndef _sport_simple_h
#define _sport_simple_h

#include <stdbool.h>
#include <stdint.h>

/*!****************************************************************
 * @brief Simple SPORT driver clock master/slave
 ******************************************************************/
typedef enum SPORT_SIMPLE_CLK_DIR {
    SPORT_SIMPLE_CLK_DIR_UNKNOWN = 0,  /**< Invalid bit clock direction */
    SPORT_SIMPLE_CLK_DIR_MASTER,       /**< Bit clock master (out) */
    SPORT_SIMPLE_CLK_DIR_SLAVE         /**< Bit clock slave (in) */
} SPORT_SIMPLE_CLK_DIR;

/*!****************************************************************
 * @brief Simple SPORT driver frame sync clock master/slave
 ******************************************************************/
typedef enum SPORT_SIMPLE_FS_DIR {
    SPORT_SIMPLE_FS_DIR_UNKNOWN = 0,  /**< Invalid sync direction */
    SPORT_SIMPLE_FS_DIR_MASTER,       /**< Sync clock master (out) */
    SPORT_SIMPLE_FS_DIR_SLAVE         /**< Sync clock slave (in) */
} SPORT_SIMPLE_FS_DIR;

/*!****************************************************************
 * @brief Simple SPORT driver data rx/tx direction
 ******************************************************************/
typedef enum SPORT_SIMPLE_DATA_DIR {
    SPORT_SIMPLE_DATA_DIR_UNKNOWN = 0,  /**< Invalid direction */
    SPORT_SIMPLE_DATA_DIR_RX,           /**< Data receive */
    SPORT_SIMPLE_DATA_DIR_TX            /**< Data transmit */
} SPORT_SIMPLE_DATA_DIR;

/*!****************************************************************
 * @brief Simple SPORT driver data pin enable settings.
 * These settings can be or'd together.  Data must be interleaved
 * in the audio buffers when both data pins are active.
 ******************************************************************/
typedef enum SPORT_SIMPLE_ENABLE {
    SPORT_SIMPLE_ENABLE_NONE      = 0, /**< Enable no data pins */
    SPORT_SIMPLE_ENABLE_PRIMARY   = 1, /**< Enable SPORT Primary Data Pin */
    SPORT_SIMPLE_ENABLE_SECONDARY = 2, /**< Enable SPORT Secondary Data Pin */
    SPORT_SIMPLE_ENABLE_BOTH      = 3, /**< Enable both SPORT Data Pins */
} SPORT_SIMPLE_ENABLE;

/*!****************************************************************
 * @brief Simple SPORT driver TDM slot and DMA data size
 * Default is 32-bit
 ******************************************************************/
typedef enum SPORT_SIMPLE_WORD_SIZE {
    SPORT_SIMPLE_WORD_SIZE_UNKNOWN =  0, /**< Invalid word size */
    SPORT_SIMPLE_WORD_SIZE_16BIT   = 16, /**< 16-bit word size */
    SPORT_SIMPLE_WORD_SIZE_32BIT   = 32, /**< 32-bit word size */
} SPORT_SIMPLE_WORD_SIZE;

/*!****************************************************************
 * @brief Simple SPORT driver TDM slot settings.
 ******************************************************************/
typedef enum SPORT_SIMPLE_TDM {
    SPORT_SIMPLE_TDM_UNKNOWN = 0,  /**< Invalid TDM mode */
    SPORT_SIMPLE_TDM_2       = 2,  /**< 2 slot TDM mode (I2S) */
    SPORT_SIMPLE_TDM_4       = 4,  /**< 4 slot TDM mode */
    SPORT_SIMPLE_TDM_8       = 8,  /**< 8 slot TDM mode */
    SPORT_SIMPLE_TDM_16      = 16, /**< 16 slot TDM mode */
    SPORT_SIMPLE_TDM_32      = 32  /**< 32 slot TDM mode */
} SPORT_SIMPLE_TDM;

/*!****************************************************************
 * @brief Simple SPORT driver bit clock options.
 * Default setting is pulse, rising edge frame sync where the
 * frame sync signal asserts in the same cycle as the MSB of the
 * first data slot (TDM).  These settings can be or'd together.
 ******************************************************************/
typedef enum SPORT_SIMPLE_FS_OPTION {
    SPORT_SIMPLE_FS_OPTION_DEFAULT = 0, /**< Default settings  */
    SPORT_SIMPLE_FS_OPTION_INV     = 1, /**< Falling edge frame sync (I2S) */
    SPORT_SIMPLE_FS_OPTION_EARLY   = 2, /**< Early frame sync (I2S) */
    SPORT_SIMPLE_FS_OPTION_50      = 4  /**< 50% duty cycle frame sync (I2S) */
} SPORT_SIMPLE_FS_OPTION;

/*!****************************************************************
 * @brief Simple SPORT driver bit clock options.
 * Default setting is clock on the rising edge, sample on falling (TDM)
 ******************************************************************/
typedef enum SPORT_SIMPLE_CLK_OPTION {
    SPORT_SIMPLE_CLK_DEFAULT = 0,  /**< Default settings */
    SPORT_SIMPLE_CLK_FALLING       /**< Assert on the falling edge, sample on rising (I2S) */
} SPORT_SIMPLE_CLK_OPTION;

/*!****************************************************************
 * @brief Simple SPORT driver API result codes.
 ******************************************************************/
typedef enum SPORT_SIMPLE_RESULT {
    SPORT_SIMPLE_SUCCESS = 0,         /**< No error */
    SPORT_SIMPLE_INVALID_SPORT,       /**< Invalid SPORT open */
    SPORT_SIMPLE_SPORT_BUSY,          /**< SPORT already open */
    SPORT_SIMPLE_CFG_ERROR,           /**< SPORT configuration error */
    SPORT_SIMPLE_ERROR                /**< Generic SPORT error */
} SPORT_SIMPLE_RESULT;

/*!****************************************************************
 * @brief Hardware SPORT
 ******************************************************************/
typedef enum SPORT_SIMPLE_PORT {
    SPORT0A,
    SPORT0B,
    SPORT1A,
    SPORT1B,
    SPORT2A,
    SPORT2B,
    SPORT3A,
    SPORT3B,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    SPORT4A,
    SPORT4B,
    SPORT5A,
    SPORT5B,
    SPORT6A,
    SPORT6B,
    SPORT7A,
    SPORT7B,
#endif
    SPORT_END
} SPORT_SIMPLE_PORT;

/*!****************************************************************
 * @brief Opaque Simple SPORT driver handle type.
 ******************************************************************/
typedef struct sSPORT sSPORT;

/*!****************************************************************
 * @brief Simple SPORT config type.
 ******************************************************************/
typedef struct SPORT_SIMPLE_CONFIG SPORT_SIMPLE_CONFIG;

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 * @brief Simple SPORT audio callback
 *
 * This user defined application function is used to transfer audio
 * data to/from the SPORT.  Cached memory buffers are managed in
 * the driver and need no additional flushing or invalidation here.
 *
 * @param [in] buffer     Pointer to the currently idle audio data
 *                        buffer.  This buffer will be one of the two
 *                        buffers supplied through the 'dataBuffers[]'
 *                        array in the config.
 * @param [in] size       Size of the buffer in bytes
 * @param [in] usrPtr     User pointer @sa SPORT_SIMPLE_CONFIG
 *
 * @return  None
 ******************************************************************/
typedef void (*SPORT_SIMPLE_AUDIO_CALLBACK)(void *buffer, uint32_t size,
    void *usrPtr);

/*!****************************************************************
 *  @brief Simple SPORT driver initialization routine.
 *
 * This function initializes the simple SPORT driver.  It should be
 * called once at program start-up.
 *
 * If using the SPORT driver under FreeRTOS, this function can be
 * called before or after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_init(void);

/*!****************************************************************
 *  @brief Simple SPORT driver deinitialization routine.
 *
 * This function frees all resources allocated by the simple SPORT
 * driver.  It should be called once at program shut-down.
 *
 * If using the SPORT driver under FreeRTOS, this function should be
 * called after the RTOS is started.
 *
 * This function is not thread safe.
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_deinit(void);

/*!****************************************************************
 * @brief Simple SPORT driver port open.
 *
 * This function opens a hardware SPORT port.
 *
 * If using the SPORT driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  port         SPORT port number to open
 * @param [out] sportHandle  A pointer to an opaque simple SPORT (sSPORT)
 *                           handle.
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_open(SPORT_SIMPLE_PORT port, sSPORT **sportHandle);

/*!****************************************************************
 * @brief Simple SPORT driver port close.
 *
 * This function closes a hardware SPORT port.
 *
 * If using the SPORT driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in,out]  sportHandle a pointer to the simple SPORT handle to
 *                  close
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_close(sSPORT **sportHandle);

/*!****************************************************************
 * @brief Simple SPORT driver port configure.
 *
 * This function configures a hardware SPORT port.
 *
 * If using the SPORT driver under FreeRTOS, this function must be
 * called after the RTOS has been started.  The SPORT_SIMPLE_CONFIG
 * structure passed into this call is not referenced after the
 * call returns and can be destroyed.
 *
 * This function is thread safe.
 *
 * @param [in]  sportHandle  SPORT handle to configure
 * @param [in]  config       SPORT configuration
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_configure(sSPORT *sportHandle,
    SPORT_SIMPLE_CONFIG *config);

/*!****************************************************************
 * @brief Simple SPORT driver buffer size calculator.
 *
 * This function returns the size required for each ping-pong buffer
 * in bytes.
 *
 * This function is thread safe.
 *
 * @param [in]  config       SPORT configuration
 *
 * @return Returns the size of each ping-pong buffer in bytes.
 ******************************************************************/
uint32_t sport_buffer_size(SPORT_SIMPLE_CONFIG *config);

/*!****************************************************************
 * @brief Simple SPORT driver port start.
 *
 * This function starts a hardware SPORT port.  Both 'ping' and
 * 'pong' buffers must be initialized with valid data prior to calling
 * this function when the SPORT is configured to transmit
 * (SPORT_SIMPLE_DATA_DIR_TX).
 *
 * If using the SPORT driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  sportHandle  SPORT handle to start
 * @param [in]  flush        Flushes TX buffers from cache.  This can
 *                           take a long time, so if it is necessary
 *                           to start the sport very quickly, pre-flush
 *                           the buffers and set this to false.
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_start(sSPORT *sportHandle, bool flush);

/*!****************************************************************
 * @brief Simple SPORT driver port stop.
 *
 * This function stops a hardware SPORT port.
 *
 * If using the SPORT driver under FreeRTOS, this function must be
 * called after the RTOS has been started.
 *
 * This function is thread safe.
 *
 * @param [in]  sportHandle  SPORT handle to stop
 *
 * @return Returns SPORT_SIMPLE_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
SPORT_SIMPLE_RESULT sport_stop(sSPORT *sportHandle);

#ifdef __cplusplus
} // extern "C"
#endif

/*!****************************************************************
 * @brief SPORT configuration parameters
 ******************************************************************/
struct SPORT_SIMPLE_CONFIG {

    /** Bit clock direction (master/slave) */
    SPORT_SIMPLE_CLK_DIR clkDir;

    /** Frame sync clock direction (master/slave) */
    SPORT_SIMPLE_FS_DIR fsDir;

    /** Bit clock options */
    SPORT_SIMPLE_CLK_OPTION bitClkOptions;

    /** Frame sync options */
    uint32_t fsOptions;

    /** Data direction (rx/tx) */
    SPORT_SIMPLE_DATA_DIR dataDir;

    /** Data pins to enable */
    SPORT_SIMPLE_ENABLE dataEnable;

    /** TDM slots */
    SPORT_SIMPLE_TDM tdmSlots;

    /** TDM / DMA word size */
    SPORT_SIMPLE_WORD_SIZE wordSize;

    /** Number of TDM frames to DMA before callback (latency) */
    uint16_t frames;

    /** Frame Sync frequency (SPORT_SIMPLE_CLK_DIR_MASTER mode only) */
    uint32_t fs;

    /**
     * By default, all TDM slots defined by 'tdmSlots' are configured for
     * data transfer on all data pins enabled by 'dataEnable'.  For larger
     * TDM settings (i.e. SPORT_SIMPLE_TDM_[8,16,32]) this can result
     * in very large and sparsely populated buffers if all TDM slots are
     * not being actively used.
     *
     * The 'slotMask' setting below can be used to define the active
     * TDM slots and reduce the data buffer size, memory bandwidth, and CPU
     * utilization.
     *
     * 'slotMask' is equally applied to all enabled data pins.  For example,
     * if 'slotMask' == 0x01, and both data pins are enabled, two audio
     * samples will be transferred on each frame.  The first sample
     * will be in the first TDM slot on D0 and the second sample will be
     * in the first TDM slot on D1.  The remaining slots in the frame will
     * not be driven (or received) by the SPORT.
     *
     * Setting 'slotMask' to zero (the default setting) enables all TDM
     * slots.
     *
     * NOTE: If the 'SPORT_SIMPLE_FS_OPTION_50' frame sync option is enabled,
     *       the 'slotMask' is applied by the SPORT hardware to each half
     *       of the frame sync essentially doubling the number of active
     *       slots in the frame.  While supported, the combination of
     *       these two options is discouraged since better control can
     *       be obtained using standard pulse frame sync TDM mode.
     *
     */
    uint32_t slotMask;

    /**
     * Pointers to buffers to holding audio data.  Each buffer should
     * point to an array of size 'tdmSlots * dataEnable * frames' comprised
     * of 'wordSize' elements.  The size of the buffer in bytes can be
     * programatically determined by passing the configuration to
     * 'sport_buffer_size()'.
     *
     * Set 'dataBuffersCached' if the dataBuffers are located in
     * cached memory.  The driver will flush/invalidate the buffers as
     * necessary.
     */
    void *dataBuffers[2];

    /** Set to true if the data buffers are in cached memory */
    bool dataBuffersCached;

    /**
     * Set to true for tighter synchronization between the DMA work unit
     * completion and audio callback.  May cause SPORT underruns for very 
     * high bandwidth streams.
     */
    bool syncDMA;

    /**
     * Provide a link to a callback function upon completion.  Can be
     * NULL for no callback.
     */
    SPORT_SIMPLE_AUDIO_CALLBACK callBack;

    /** User pointer passed to the audio callback function */
    void *usrPtr;

};

#endif
