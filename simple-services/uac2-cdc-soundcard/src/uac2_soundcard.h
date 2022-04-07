/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#ifndef _uac2_soundcard_h
#define _uac2_soundcard_h

#include <stdint.h>
#include <stdbool.h>

#include "uac2_soundcard_cfg.h"
#include "cld_lib.h"

typedef enum {
    UAC2_DIR_UNKNOWN = 0,
    UAC2_DIR_IN,
    UAC2_DIR_OUT
} UAC2_DIR;

typedef uint16_t (*UAC2_RX_CALLBACK)(void *data, void **nextData,
    uint16_t size, void *usrPtr);
typedef uint16_t (*UAC2_TX_CALLBACK)(void *data, void **nextData,
    uint16_t minSize, uint16_t maxSize, void *usrPtr);
typedef uint32_t (*UAC2_RATE_FEEDBACK_CALLBACK)(void *usrPtr);
typedef void (*UAC2_ENDPOINT_ENABLE_CALLBACK)(UAC2_DIR dir, bool enable, void *usrPtr);

/* USB Audio OUT (Rx) endpoint stats */
typedef struct {
    uint32_t count;
    uint32_t maxPktTime;
    uint32_t lastPktTime;
    uint16_t lastPktSize;
    uint16_t maxPktSize;
    uint16_t minPktSize;
    uint32_t failed;
    uint32_t aborted;
    uint32_t ok;
} UAC2_ENDPOINT_STATS;

typedef struct {
#if defined(__ADSPSC589_FAMILY__)
    CLD_SC58x_USB_Port_Num port;      /*!< USB port (CLD_USB_0 or CLD_USB_1) */
#endif
    uint8_t usbInChannels;            /*!< USB IN (Tx) channels */
    uint8_t usbInWordSizeBits;        /*!< USB IN (Tx) word size */
    uint8_t usbOutChannels;           /*!< USB OUT (Rx) channels */
    uint8_t usbOutWordSizeBits;       /*!< USB OUT (Rx) word size */
    uint32_t usbSampleRate;           /*!< USB sample rate */
    uint32_t timerNum;                /*!< ADI Timer Service timer number */
    uint16_t vendorId;                /*!< USB Vendor ID */
    uint16_t productId;               /*!< USB Product ID */
    const char *mfgString;            /*!< USB Manufacturer string */
    const char *productString;        /*!< USB Product string */
    const char *serialNumString;      /*!< USB Serial number string */
    bool lowLatency;                  /*!< Prefer low latency (true) over CPU (false) */
    UAC2_ENDPOINT_STATS *usbInStats;  /*!< USB IN (Tx) stats */
    UAC2_ENDPOINT_STATS *usbOutStats; /*!< USB OUT (Rx) stats */
    UAC2_RX_CALLBACK rxCallback;      /*!< UAC2 OUT (Rx) callback */
    UAC2_TX_CALLBACK txCallback;      /*!< UAC2 IN (Tx) callback */
    UAC2_RATE_FEEDBACK_CALLBACK rateFeedbackCallback;     /*!< UAC2 Rate Feedback callback */
    UAC2_ENDPOINT_ENABLE_CALLBACK endpointEnableCallback; /*!< UAC2 Endpoint enable callback */
    void *usrPtr;
} UAC2_APP_CONFIG;

#ifdef __cplusplus
extern "C"{
#endif

CLD_RV uac2_init(void);
CLD_RV uac2_config(UAC2_APP_CONFIG *cfg);
CLD_RV uac2_start(void);
CLD_RV uac2_run(void);

/* These functions allow the application to query the maximum UAC2 audio
 * packet size following a successful call to uac2_config().
 */
CLD_RV uac2_getRxPktSize(uint16_t *minFull, uint16_t *maxFull,
    uint16_t *minHigh, uint16_t *maxHigh);
CLD_RV uac2_getTxPktSize(uint16_t *minFull, uint16_t *maxFull,
    uint16_t *minHigh, uint16_t *maxHigh);

/* These functions optionally allow the application to allocate the
 * UAC2 Rx and/or Tx data buffers.  They must be called once between the
 * uac2_config() and uac2_start() setup functions.
 *
 * Subsequent updates to the Rx and/or Tx data buffer pointers must happen
 * through the 'nextData' argument of the audio callbacks.
 *
 * If these functions are not called, the soundcard driver will allocate
 * an internal buffer and the callback's 'nextData' argument should not
 * be modified.
 */
CLD_RV uac2_setRxPktBuffer(void *buf);
CLD_RV uac2_setTxPktBuffer(void *buf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
