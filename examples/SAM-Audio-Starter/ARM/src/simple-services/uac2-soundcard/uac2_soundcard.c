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

#include <string.h>
#include <stdint.h>
#include <services/int/adi_int.h>
#include <services/tmr/adi_tmr.h>

#include "cld_lib.h"

#include "uac2_soundcard.h"
#include "uac2_descriptors.h"
#include "cdc.h"
#include "syslog.h"
#include "cpu_load.h"
#include "clocks.h"

/*
 * General Info:
 *
 * Microsoft USB Audio 2.0 Driver Requirements:
 *    https://docs.microsoft.com/en-us/windows-hardware/drivers/audio/usb-2-0-audio-drivers
 *
 * Gathering logs from Microsoft's USB Audio 2.0 Driver:
 *    https://matthewvaneerde.wordpress.com/2017/10/23/how-to-gather-and-read-logs-for-microsofts-usb-audio-2-0-class-driver/
 *
 * MAC USB Audio 2.0 Driver information:
 *    https://developer.apple.com/library/archive/technotes/tn2274/_index.html
 *
 * A single isochronous transfer can carry 1024 bytes, and can carry at most 256
 * samples (at 24/32 bits). This means that a single isochronous endpoint can transfer
 * 42 channels at 48 kHz, or 10 channels at 192 kHz (assuming that High Speed USB
 * is used - Full Speed USB cannot carry more than a single stereo IN and OUT pair at
 * 48 kHz).
 *
 */

/*
   The function 'createTerminalAndFeatureDescriptors()' generates a set of
   USB Audio v2.0 Unit, Terminal, and Endpoint descriptors that describe
   a simple audio device comprised of the following:

   USB           Input          Feature         Output         USB
   OUT           Terminal       Unit            Terminal       IN
   --------      ---------      ----------      ----------    ----------
   ENDPOINT  ->  USB OUT    ->  Controls    ->  Spkr
   ID = 3        ID = 0x01      ID = 0x09       ID = 0x06
                +------------ Clock ID = 0x03 -----------+

                 Mic        ->  Controls    ->  USB IN     ->  ENDPOINT
                 ID = 0x02      ID = 0x0A       ID = 0x07      ID = 2
                +------------ Clock ID = 0x04 -----------+
*/

/* Basic settings (do not modify) */
#define USB_RATE_FEEDBACK_RATE_MS  1      // Slowest high-speed rate Windows allows
#define USB_MAX_PACKET_SIZE        1024   // Largest full/high speed packet

/* USB IN Endpoint settings */
#define USB_IN_ENDPOINT_ID         0x02

/* USB OUT Endpoint settings */
#define USB_OUT_ENDPOINT_ID        0x03

/* Speaker Audio Function IDs */
#define USB_OUT_INPUT_TERMINAL_ID  0x01
#define SPKR_FEATURE_UNIT_ID       0x09
#define SPKR_OUTPUT_TERMINAL_ID    0x06
#define SPKR_CLOCK_SOURCE_ID       0x03

/* Microphone Audio Function IDs */
#define MIC_INPUT_TERMINAL_ID      0x02
#define MIC_FEATURE_UNIT_ID        0x0A
#define USB_IN_OUTPUT_TERMINAL_ID  0x07
#define MIC_CLOCK_SOURCE_ID        0x04

/* Function prototypes */
static void uac2_audio_stream_data_transmitted (void);
static void uac2_audio_stream_data_transmit_aborted (void);
static void uac2_audio_stream_data_transmitted_X (void);
static void uac2_audio_stream_data_transmit_aborted_X (void);
static CLD_USB_Transfer_Request_Return_Type uac2_stream_data_received (CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Data_Received_Return_Type uac2_stream_data_receive_complete (void);
static CLD_USB_Transfer_Request_Return_Type uac2_set_req_cmd (
    CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params,
    CLD_USB_Transfer_Params * p_transfer_data
);
static CLD_USB_Transfer_Request_Return_Type uac2_get_req_cmd (
    CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params,
    CLD_USB_Transfer_Params * p_transfer_data
);
static CLD_USB_Data_Received_Return_Type uac2_set_volume_req (void);
static void uac2_streaming_rx_endpoint_enabled (CLD_Boolean enabled);
static void uac2_streaming_tx_endpoint_enabled (CLD_Boolean enabled);
static void uac2_usb_event (CLD_USB_Event event);
static void uac2_tx_audio_data (void);
static void uac2_tx_feedback_data (void);
static void uac2_feedback_xfr_done (void);
static void uac2_usb0_isr(uint32_t Event, void *pArg);
static void uac2_usb1_isr(uint32_t Event, void *pArg);
static void user_cld_lib_status(unsigned short status_code,
    void *p_additional_data, unsigned short additional_data_size
);
static CLD_RV uac20_timer_start(uint32_t num, ADI_TMR_HANDLE *handle);
void uac2_syslog(char *log);

/**
 * Structure used to store the current USB volume settings.
 */
typedef struct
{
    uint16_t master;              /*!< Master Volume setting */
    uint16_t *vol;                /*!< Pointer to array of channel volumes */
    uint16_t min;                 /*!< Volume range minimum setting */
    uint16_t max;                 /*!< Volume range maximum setting */
    uint16_t resolution;          /*!< Volume range resolution */
    uint8_t  mute;                /*!< Mute setting */
} UAC2_VOLUME;

/**
 * Structure used to store the current USB clock source.
 */
typedef struct
{
    uint32_t current;
    uint32_t min;                 /*!< Clock source range minimum setting */
    uint32_t max;                 /*!< Clock source range maximum setting */
    uint32_t resolution;          /*!< Clock source range resolution */
} UAC2_CLOCK_SOURCE;

/**
 * User Audio data
 */
typedef struct
{
    CLD_Boolean volume_changed;

    UAC2_VOLUME speaker_output_volume;
    UAC2_VOLUME mic_input_volume;
    UAC2_CLOCK_SOURCE clock_source;

    CLD_Boolean in_enabled;
    CLD_Boolean in_idle;
    CLD_Boolean in_active;
    CLD_Boolean in_data_allocated;
    uint8_t *in_data;
    uint16_t in_size;
    uint16_t minInSize;
    uint16_t maxInSize;
    uint16_t minInSizeFull;
    uint16_t maxInSizeFull;
    uint16_t minInSizeHigh;
    uint16_t maxInSizeHigh;

    CLD_Boolean inPktFirst;
    uint32_t inPktLastTime;
    uint32_t inPktTime;
    uint32_t maxInPktTime;

    CLD_Boolean out_enabled;
    CLD_Boolean out_data_allocated;
    uint8_t *out_data;
    uint16_t out_size;
    uint16_t minOutSize;
    uint16_t maxOutSize;
    uint16_t minOutSizeFull;
    uint16_t maxOutSizeFull;
    uint16_t minOutSizeHigh;
    uint16_t maxOutSizeHigh;

    CLD_Boolean outPktFirst;
    uint32_t outPktLastTime;
    uint32_t outPktTime;
    uint32_t maxOutPktTime;

    CLD_Boolean rate_feedback_idle;
    CLD_Boolean first_feedback;

    UAC2_APP_CONFIG cfg;
    ADI_TMR_HANDLE periodic_timer_handle;

    char *syslog[UAC2_SYSLOG_DEPTH];
    uint8_t syslogHead;
    uint8_t syslogTail;

} UAC2_STATE;

/*
 * Locate 'uac2_state' in un-cached memory since DMA will be used
 * to directly access the UAC2_VOLUME and UAC2_CLOCK_SOURCE structures.
 *
 * Also, be sure to allocate all 'UAC2_VOLUME.vol' buffers in uncached
 * memory since they are also directly manipulated via DMA.
 *
 */
__attribute__ ((section(".l3_uncached_data")))
static UAC2_STATE uac2_state;

/*************************************************************************
 * USB Feedback Isochronous IN Endpoint functions and data
 *************************************************************************/

/**
 * Function called when the usb feedback endpoint data has been transmitted.
 *
 * @param event - identifies which USB event has occurred.
 */
static void uac2_feedback_xfr_done (void)
{
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    uac2_state.rate_feedback_idle = CLD_TRUE;
    uac2_state.first_feedback = CLD_FALSE;
    uac2_tx_feedback_data();

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

/**
 * Transmits the current feedback value to the USB Host.
 *
 */
static void uac2_tx_feedback_data ( void )
{
    uint32_t rate;

    static CLD_USB_Audio_Feedback_Params feedback_transfer_data =
    {
        .fp_transfer_aborted_callback = uac2_feedback_xfr_done,
        .transfer_timeout_ms = 5 * USB_RATE_FEEDBACK_RATE_MS,
        .fp_usb_in_transfer_complete = uac2_feedback_xfr_done,
    };

    if ( uac2_state.out_enabled == CLD_TRUE &&
         uac2_state.rate_feedback_idle == CLD_TRUE) {
        if (uac2_state.first_feedback == CLD_TRUE) {
            feedback_transfer_data.desired_data_rate =
                (float)uac2_state.cfg.usbSampleRate / 1000.0f;
        } else {
            if (uac2_state.cfg.rateFeedbackCallback) {
                rate = uac2_state.cfg.rateFeedbackCallback(uac2_state.cfg.usrPtr);
                if (rate) {
                    feedback_transfer_data.desired_data_rate = (float)rate / 1000.0;
                }
            }
        }

        if (cld_sc58x_audio_2_0_w_cdc_lib_transmit_audio_rate_feedback_data(&feedback_transfer_data)== CLD_USB_TRANSMIT_SUCCESSFUL) {
            uac2_state.rate_feedback_idle = CLD_FALSE;
        }
    }
}

/*************************************************************************
 * USB Audio Isochronous IN Endpoint / Audio Transmit functions
 *************************************************************************/

/**
 * Function called when a normal IN data packet has been transmitted
 * to the USB Host.
 *
 */
static void uac2_audio_stream_data_transmitted (void)
{
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    /* Keep track of IN (Tx) packet departure times for statistics */
    if (uac2_state.cfg.usbInStats) {
        if (uac2_state.inPktFirst == CLD_TRUE) {
            uac2_state.maxInPktTime = 0;
            uac2_state.cfg.usbInStats->minPktSize = uac2_state.in_size;
            uac2_state.cfg.usbInStats->maxPktSize = uac2_state.in_size;
            uac2_state.inPktFirst = CLD_FALSE;
        } else {
            uac2_state.inPktTime = cpuLoadGetTimeStamp() - uac2_state.inPktLastTime;
            uac2_state.cfg.usbInStats->lastPktTime = uac2_state.inPktTime;
            if (uac2_state.inPktTime > uac2_state.maxInPktTime) {
                uac2_state.maxInPktTime = uac2_state.inPktTime;
                uac2_state.cfg.usbInStats->maxPktTime = uac2_state.maxInPktTime;
            }
            if (uac2_state.in_size < uac2_state.cfg.usbInStats->minPktSize) {
                uac2_state.cfg.usbInStats->minPktSize = uac2_state.in_size;
            }
            if (uac2_state.in_size > uac2_state.cfg.usbInStats->maxPktSize) {
                uac2_state.cfg.usbInStats->maxPktSize = uac2_state.in_size;
            }
        }
        uac2_state.inPktLastTime = cpuLoadGetTimeStamp();
        uac2_state.cfg.usbInStats->ok++;
    }

    /* Prepare next IN transfer */
    uac2_state.in_idle = CLD_TRUE;
    uac2_tx_audio_data();

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

/**
 * Function called after the first IN data packet has been transmitted
 * to the USB Host.
 *
 */
static void uac2_audio_stream_data_transmitted_X (void)
{
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    /* Indicate IN (Tx) data is active */
    if (uac2_state.in_active == CLD_FALSE) {

        /* Tell the application that the IN endpoint is active only after
         * data has started flowing to prevent overruns between the alternate
         * configuration being enabled and IN tokens being sent.
         */
        uac2_syslog("UAC 2.0 IN Enabled");
        if (uac2_state.cfg.endpointEnableCallback) {
            uac2_state.cfg.endpointEnableCallback(
                UAC2_DIR_IN, true, uac2_state.cfg.usrPtr
            );
        }
        uac2_state.in_active = CLD_TRUE;
    }
    uac2_state.in_idle = CLD_TRUE;
    uac2_tx_audio_data();

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

/**
 * Function called when the IN data transmission fails or when
 * the IN endpoint stops and times out.
 */
static void uac2_audio_stream_data_transmit_aborted (void)
{
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    if (uac2_state.in_active == CLD_TRUE) {
        if (uac2_state.cfg.usbInStats) {
            uac2_state.cfg.usbInStats->aborted++;
        }
    }

    /* HACK: This clears the isochronous IN endpoint fifo when
     *       the IN stream stops to keep the last packet from
     *       leaking out if the stream restarts.
     */
    if ( *pREG_USB0_EP2_TXCSR_P &
         (BITM_USB_EP_TXCSR_P_NEFIFO | BITM_USB_EP_TXCSR_P_TXPKTRDY) ) {
        uint16_t w0cRegisters = BITM_USB_EP_TXCSR_P_INCOMPTX |
            BITM_USB_EP_TXCSR_P_SENTSTALL | BITM_USB_EP_TXCSR_P_URUNERR;
        *pREG_USB0_EP2_TXCSR_P |= BITM_USB_EP_TXCSR_P_FLUSHFIFO | w0cRegisters;
    }

    uac2_state.in_idle = CLD_TRUE;
    uac2_tx_audio_data();

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

/**
 * Function called while waiting for USB IN tokens.
 */
static void uac2_audio_stream_data_transmit_aborted_X (void)
{
    uint32_t inCycles, outCycles;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    uac2_state.in_idle = CLD_TRUE;
    uac2_tx_audio_data();

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

/**
 * Transmits audio IN data to the USB Host.
 */
static void uac2_tx_audio_data (void)
{
    static CLD_USB_Transfer_Params audio_data_tx_params;
    void *nextData;

    /* If the Isochronous IN endpoint is enabled and idle */
    if ((uac2_state.in_enabled == CLD_TRUE) &&
        (uac2_state.in_idle == CLD_TRUE))
    {
        /* Wait until the first isochronous IN token before setting active.
         * Send a zero length packet to keep from tainting the desired audio
         * data stream.
         */
        if (uac2_state.in_active == CLD_FALSE) {
            audio_data_tx_params.num_bytes = 0;
            audio_data_tx_params.p_data_buffer = uac2_state.in_data;
            audio_data_tx_params.callback.fp_usb_in_transfer_complete =
                uac2_audio_stream_data_transmitted_X;
            audio_data_tx_params.transfer_timeout_ms = 100;
            audio_data_tx_params.fp_transfer_aborted_callback =
                uac2_audio_stream_data_transmit_aborted_X;
            if (cld_sc58x_audio_2_0_w_cdc_lib_transmit_audio_data(&audio_data_tx_params) == CLD_USB_TRANSMIT_SUCCESSFUL) {
                uac2_state.in_idle = CLD_FALSE;
            }
        } else {
            if (uac2_state.cfg.txCallback) {
                nextData = uac2_state.in_data;
                uac2_state.in_size = uac2_state.cfg.txCallback(
                    uac2_state.in_data, &nextData, uac2_state.minInSize, uac2_state.maxInSize,
                    uac2_state.cfg.usrPtr
                );
                uac2_state.in_data = nextData;
            } else {
                uac2_state.in_size = 0;
            }
            /* Queue the packet for transmission */
            audio_data_tx_params.num_bytes = uac2_state.in_size;
            audio_data_tx_params.p_data_buffer = uac2_state.in_data;
            audio_data_tx_params.callback.fp_usb_in_transfer_complete =
                uac2_audio_stream_data_transmitted;
            audio_data_tx_params.transfer_timeout_ms = 100;
            audio_data_tx_params.fp_transfer_aborted_callback =
                uac2_audio_stream_data_transmit_aborted;
            if (cld_sc58x_audio_2_0_w_cdc_lib_transmit_audio_data(&audio_data_tx_params) == CLD_USB_TRANSMIT_SUCCESSFUL) {
                uac2_state.in_idle = CLD_FALSE;
                uac2_state.cfg.usbInStats->lastPktSize = uac2_state.in_size;
            } else {
                if (uac2_state.cfg.usbInStats) {
                    uac2_state.cfg.usbInStats->failed++;
                }
            }
            if (uac2_state.cfg.usbInStats) {
                uac2_state.cfg.usbInStats->count++;
            }
        }
    }
}

/*************************************************************************
 * USB Audio Isochronous OUT / Audio Receive functions
 *************************************************************************/

/**
 * Function called when speaker data has been received.
 *
 * @retval CLD_USB_DATA_GOOD
 * @retval CLD_USB_DATA_BAD_STALL
 */
static CLD_USB_Data_Received_Return_Type uac2_stream_data_receive_complete (void)
{
    uint32_t inCycles, outCycles;
    void *nextData;

    /* Track ISR cycle count for CPU load */
    inCycles = cpuLoadGetTimeStamp();

    /* Keep track of OUT (Rx) packet arrival times for statistics */
    if (uac2_state.cfg.usbOutStats) {
        if (uac2_state.outPktFirst == CLD_TRUE) {
            uac2_state.maxOutPktTime = 0;
            uac2_state.cfg.usbOutStats->minPktSize = uac2_state.out_size;
            uac2_state.cfg.usbOutStats->maxPktSize = uac2_state.out_size;
            uac2_state.outPktFirst = CLD_FALSE;
        } else {
            uac2_state.outPktTime = cpuLoadGetTimeStamp() - uac2_state.outPktLastTime;
            uac2_state.cfg.usbOutStats->lastPktTime = uac2_state.outPktTime;
            if (uac2_state.outPktTime > uac2_state.maxOutPktTime) {
                uac2_state.maxOutPktTime = uac2_state.outPktTime;
                uac2_state.cfg.usbOutStats->maxPktTime = uac2_state.maxOutPktTime;
            }
            uac2_state.cfg.usbOutStats->lastPktSize = uac2_state.out_size;
            if (uac2_state.out_size < uac2_state.cfg.usbOutStats->minPktSize) {
                uac2_state.cfg.usbOutStats->minPktSize = uac2_state.out_size;
            }
            if (uac2_state.out_size > uac2_state.cfg.usbOutStats->maxPktSize) {
                uac2_state.cfg.usbOutStats->maxPktSize = uac2_state.out_size;
            }
        }
        uac2_state.outPktLastTime = cpuLoadGetTimeStamp();
        uac2_state.cfg.usbOutStats->count++;
    }

    if (uac2_state.cfg.rxCallback) {
        nextData = uac2_state.out_data;
        uac2_state.cfg.rxCallback(
            uac2_state.out_data, &nextData, uac2_state.out_size, uac2_state.cfg.usrPtr
        );
        uac2_state.out_data = nextData;
    }

    /* Track ISR cycle count for CPU load */
    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);

    return CLD_USB_DATA_GOOD;
}

/**
 * This function is called by the cld_sc58x_audio_2_0_w_cdc_lib library when data is
 * received on the Isochronous OUT endpoint. This function sets the
 * p_transfer_data parameters to select where the received data
 * should be stored, and what function should be called when the
 * transfer is complete.
 *
 * @param p_transfer_data                - Pointer to the Isochronous OUT transfer data.
 * @param p_transfer_data->num_bytes     - Number of received OUT bytes.
 *                                         This value can be set to the total
 *                                         transfer size if more then one
 *                                         packet is expected.
 * @param p_transfer_data->p_data_buffer - Set to the address where the
 *                                         data should be written.
 * @param p_transfer_data->callback.fp_usb_out_transfer_complete -
 *                                         Function called when the
 *                                         requested received bytes have
 *                                         been written to p_data_buffer.
 * @param p_transfer_data->fp_transfer_aborted_callback -
 *                                         Optional function that is
 *                                         called if the transfer is
 *    f                                     aborted.
 * @param p_transfer_data->transfer_timeout_ms - Transfer timeout in milliseconds.
 *                                         Set to 0 to disable timeout.
 * @retval CLD_USB_TRANSFER_ACCEPT - Store the data using the p_transfer_data
 *                                  parameters..
 * @retval CLD_USB_TRANSFER_PAUSE - The device isn't ready to process this
 *                                  out packet so pause the transfer
 *                                  until the cld_sc58x_audio_2_0_w_cdc_lib_resume_paused_audio_data_transfer
 *                                  function is called.
 * @retval CLD_USB_TRANSFER_DISCARD - Discard this packet.
 * @retval CLD_USB_TRANSFER_STALL - Stall the OUT endpoint.
 *
 */
static CLD_USB_Transfer_Request_Return_Type uac2_stream_data_received (CLD_USB_Transfer_Params * p_transfer_data)
{
    uac2_state.out_size = p_transfer_data->num_bytes;
    p_transfer_data->p_data_buffer = uac2_state.out_data;
    p_transfer_data->transfer_timeout_ms = 0;
    p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
    p_transfer_data->callback.fp_usb_out_transfer_complete = uac2_stream_data_receive_complete;

    return CLD_USB_TRANSFER_ACCEPT;
}

/*************************************************************************
 * Volume / Mute control functions
 *************************************************************************/

/**
 * This function is called by the CLD Audio library when a Set Volume request data
 * has been completed.
 *
 * @retval CLD_USB_DATA_GOOD - Received data is valid.
 * @retval CLD_USB_DATA_BAD_STALL - Received data is invalid.
 */
static CLD_USB_Data_Received_Return_Type uac2_set_volume_req (void)
{
    /* Flag that new volume settings have been received. */
    uac2_state.volume_changed = CLD_TRUE;
    return CLD_USB_DATA_GOOD;
}

/**
 * This function is called by the CLD Audio v2.0 library when a Set Command
 * request is received. This function sets the p_transfer_data parameters to
 * select where the received data should be stored, and what function should
 * be called when the transfer is complete.
 *
 * @param p_transfer_data                - Pointer to the Control OUT transfer data.
 * @param p_transfer_data->num_bytes     - Number of Control OUT bytes.
 * @param p_transfer_data->p_data_buffer - Set to the address where to
 *                                         store the Control OUT data.
 * @param p_transfer_data->callback.usb_in_transfer_complete -
 *                                         Function called when the
 *                                         Control OUT bytes have been
 *                                         received.
 * @param p_transfer_data->transfer_aborted_callback -
 *                                         Optional function that is
 *                                         called if the transfer is
 *                                         aborted.
 * @param p_req_params->req              - Setup Packet bRequest value.
 * @param p_req_params->entity_id        - Requested entity ID (Unit ID,
 *                                         Terminal ID, etc)
 * @param p_req_params->interface_or_endpoint_num -
 *                                         Requested interface or endpoint
 *                                         number depending on the entity.
 * @param p_req_params->setup_packet_wValue
 *
 * @retval CLD_USB_TRANSFER_ACCEPT - Store the data using the p_transfer_data
 *                                   parameters.
 * @retval CLD_USB_TRANSFER_PAUSE - The device isn't ready to process this
 *                                  out packet so pause the transfer
 *                                  until the cld_sc58x_audio_2_0_w_cdc_lib_resume_paused_control_transfer
 *                                  function is called.
 * @retval CLD_USB_TRANSFER_DISCARD - Discard this packet.
 * @retval CLD_USB_TRANSFER_STALL - Stall the OUT endpoint.
 *
 */
static CLD_USB_Transfer_Request_Return_Type uac2_set_req_cmd (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data)
{
    CLD_USB_Transfer_Request_Return_Type rv;
    uint8_t controlSelector;
    uint8_t channel;
    void *pDataBuffer;
    uint16_t numBytes;
    uint8_t maxChannels;

    UAC2_VOLUME *featureUnit;

    /* Default to discarding the packet */
    rv = CLD_USB_TRANSFER_DISCARD;
    featureUnit = NULL;
    pDataBuffer = NULL;
    numBytes = 0;
    maxChannels = 0;

    /* Select to the correct Feature Unit */
    switch (p_req_params->entity_id) {
        case SPKR_FEATURE_UNIT_ID:
            featureUnit = &uac2_state.speaker_output_volume;
            maxChannels = uac2_state.cfg.usbOutChannels + 1;
            break;
        case MIC_FEATURE_UNIT_ID:
            featureUnit = &uac2_state.mic_input_volume;
            maxChannels = uac2_state.cfg.usbInChannels + 1;
            break;
        default:
            break;
    }

    /* Select the appropriate control */
    if (featureUnit) {

        /* Decode the Feature Unit control and channel */
        controlSelector = (p_req_params->setup_packet_wValue >> 8) & 0xFF;
        channel = p_req_params->setup_packet_wValue & 0xFF;

        /* Point to the control channel data */
        switch (controlSelector) {

            /* Volume set */
            case UAC_FU_VOLUME_CONTROL:
                if (channel == 0) {
                    pDataBuffer =&featureUnit->master;
                    numBytes = 2;
                } else if (channel < maxChannels) {
                    pDataBuffer = &featureUnit->vol[channel-1];
                    numBytes = 2;
                }
                break;

            /* Mute set */
            case UAC_FU_MUTE_CONTROL:
                pDataBuffer = &featureUnit->mute;
                numBytes = 1;
                break;
            default:
                break;
        }
    }

    /* Complete the set request */
    if (pDataBuffer) {
        p_transfer_data->p_data_buffer = (unsigned char *)pDataBuffer;
        p_transfer_data->num_bytes = numBytes;
        p_transfer_data->transfer_timeout_ms = 0;
        p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
        p_transfer_data->callback.fp_usb_out_transfer_complete = uac2_set_volume_req;
        rv = CLD_USB_TRANSFER_ACCEPT;
    }

    return rv;
}

/**
 * This function is called by the CLD Audio v2.0 library when a Get Command
 * request is received. This function sets the p_transfer_data parameters to
 * select where the transmit data should be sourced, and what function should
 * be called when the transfer is complete.
 *
 * @param p_transfer_data                - Pointer to the Control IN transfer data.
 * @param p_transfer_data->num_bytes     - Number of Control IN bytes.
 * @param p_transfer_data->p_data_buffer - Set to the address where to
 *                                         source the Control IN data.
 * @param p_transfer_data->callback.usb_in_transfer_complete -
 *                                         Function called when the
 *                                         Control IN bytes have been
 *                                         sent.
 * @param p_transfer_data->transfer_aborted_callback -
 *                                         Optional function that is
 *                                         called if the transfer is
 *                                         aborted.
 * @param p_req_params->req              - Setup Packet bRequest value.
 * @param p_req_params->entity_id        - Requested entity ID (Unit ID,
 *                                         Terminal ID, etc)
 * @param p_req_params->interface_or_endpoint_num -
 *                                         Requested interface or endpoint
 *                                         number depending on the entity.
 * @param p_req_params->setup_packet_wValue
 *
 * @retval LD_USB_TRANSFER_ACCEPT - Store the data using the p_transfer_data
 *                                  parameters.
 * @retval CLD_USB_TRANSFER_PAUSE - The device isn't ready to process this
 *                                  out packet so pause the transfer
 *                                  until the cld_sc58x_audio_2_0_w_cdc_lib_resume_paused_control_transfer
 *                                  function is called.
 * @retval CLD_USB_TRANSFER_DISCARD - Discard this packet.
 * @retval CLD_USB_TRANSFER_STALL - Stall the OUT endpoint.
 *
 */
__attribute__ ((section(".l3_uncached_data")))
static uac2_2_byte_control_range_parameter_block uac2_2_byte_range_resp;

__attribute__ ((section(".l3_uncached_data")))
static uac2_4_byte_control_range_parameter_block uac2_4_byte_range_resp;

static CLD_USB_Transfer_Request_Return_Type uac2_get_req_cmd (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data)
{
    CLD_USB_Transfer_Request_Return_Type rv = CLD_USB_TRANSFER_DISCARD;

    uint8_t controlSelector;
    uint8_t channel;
    void *pDataBuffer;
    uint16_t numBytes;
    uint8_t maxChannels;

    UAC2_VOLUME *featureUnit;
    UAC2_CLOCK_SOURCE *clockSource;

    /* Default to discarding the packet */
    rv = CLD_USB_TRANSFER_DISCARD;
    clockSource = NULL;
    featureUnit = NULL;
    pDataBuffer = NULL;
    numBytes = 0;
    maxChannels = 0;

    /* Select the correct Feature Unit or Clock Source */
    switch (p_req_params->entity_id) {
        case SPKR_FEATURE_UNIT_ID:
            featureUnit = &uac2_state.speaker_output_volume;
            maxChannels = uac2_state.cfg.usbOutChannels + 1;
            break;
        case MIC_FEATURE_UNIT_ID:
            featureUnit = &uac2_state.mic_input_volume;
            maxChannels = uac2_state.cfg.usbInChannels + 1;
            break;
        case MIC_CLOCK_SOURCE_ID:
        case SPKR_CLOCK_SOURCE_ID:
            clockSource = &uac2_state.clock_source;
            break;
        default:
            break;
    }

    /* Handle Feature Unit requests */
    if (featureUnit) {

        /* Feature Unit range request */
        if (p_req_params->req == CLD_REQ_RANGE) {

            uac2_2_byte_range_resp.wNumSubRanges = 1;
            uac2_2_byte_range_resp.sub_ranges[0].wMAX = featureUnit->max;
            uac2_2_byte_range_resp.sub_ranges[0].wMIN = featureUnit->min;
            uac2_2_byte_range_resp.sub_ranges[0].wRES = featureUnit->resolution;
            pDataBuffer = &uac2_2_byte_range_resp;
            numBytes = sizeof(uac2_2_byte_range_resp);

        /* Feature Unit current requests */
        } else if (p_req_params->req == CLD_REQ_CURRENT) {

            /* Decode the Feature Unit control and channel */
            controlSelector = (p_req_params->setup_packet_wValue >> 8) & 0xFF;
            channel = p_req_params->setup_packet_wValue & 0xFF;

            switch (controlSelector) {

                /* Volume requests */
                case UAC_FU_VOLUME_CONTROL:
                    if (channel == 0) {
                        pDataBuffer =&featureUnit->master;
                        numBytes = 2;
                    } else if (channel < maxChannels) {
                        pDataBuffer = &featureUnit->vol[channel-1];
                        numBytes = 2;
                    }
                    break;

                /* Mute requests */
                case UAC_FU_MUTE_CONTROL:
                    pDataBuffer = &featureUnit->mute;
                    numBytes = 1;
                    break;

                /* Unsupported */
                default:
                    break;
            }
        }

    }

    /* Handle Clock Source requests */
    if (clockSource) {

        /* Clock Source range requests */
        if (p_req_params->req == CLD_REQ_RANGE) {
            uac2_4_byte_range_resp.wNumSubRanges = 1;
            uac2_4_byte_range_resp.sub_ranges[0].wMAX = clockSource->max;
            uac2_4_byte_range_resp.sub_ranges[0].wMIN = clockSource->min;
            uac2_4_byte_range_resp.sub_ranges[0].wRES = clockSource->resolution;
            pDataBuffer = &uac2_4_byte_range_resp;
            numBytes = sizeof(uac2_4_byte_range_resp);

        /* Clock Source current requests */
        } else if (p_req_params->req == CLD_REQ_CURRENT) {
            pDataBuffer = &clockSource->current;
            numBytes = sizeof(clockSource->current);
        }
    }

    /* Send response for supported entities */
    if (pDataBuffer) {
        p_transfer_data->p_data_buffer = pDataBuffer;
        p_transfer_data->num_bytes = numBytes;
        p_transfer_data->transfer_timeout_ms = 0;
        p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
        p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
        rv = CLD_USB_TRANSFER_ACCEPT;
    }

    return rv;
}

/**************************************************************************
 * Misc ISR and endpoint enable/disable handlers
 *************************************************************************/

/**
 * Function called when the Isochronous OUT interface is enabled/disabled by the
 * USB Host using the Set Interface Alternate Setting request.
 *
 * @param enabled - CLD_TRUE = Isochronous OUT endpoint Enabled.
 */
static void uac2_streaming_rx_endpoint_enabled (CLD_Boolean enabled)
{
    uac2_state.out_enabled = enabled;

    if (uac2_state.cfg.endpointEnableCallback) {
        uac2_state.cfg.endpointEnableCallback(
            UAC2_DIR_OUT,
            enabled == CLD_TRUE, uac2_state.cfg.usrPtr
        );
    }

    if (enabled == CLD_TRUE) {
        uac2_syslog("UAC 2.0 OUT Enabled");
        uac2_state.first_feedback = CLD_TRUE;
    } else {
        uac2_syslog("UAC 2.0 OUT Disabled");
        uac2_state.rate_feedback_idle = CLD_TRUE;
        uac2_state.outPktFirst = CLD_TRUE;
    }
}

/**
 * Function called when the Isochronous IN interface is enabled/disabled by the
 * USB Host using the Set Interface Alternate Setting request.
 *
 * @param enabled - CLD_TRUE = Isochronous IN endpoint Enabled.
 */
static void uac2_streaming_tx_endpoint_enabled (CLD_Boolean enabled)
{
    uac2_state.in_enabled = enabled;

    if (enabled == CLD_TRUE) {
        /* Do nothing */
    } else {
        uac2_syslog("UAC 2.0 IN Disabled");
        if (uac2_state.cfg.endpointEnableCallback) {
            uac2_state.cfg.endpointEnableCallback(
                UAC2_DIR_IN, false, uac2_state.cfg.usrPtr
            );
        }
        uac2_state.in_active = CLD_FALSE;
        uac2_state.in_idle = CLD_TRUE;
        uac2_state.inPktFirst = CLD_TRUE;
    }
}

/**
 * User defined USB 0 ISR.
 *
 * @param Event, pArg.
 */
static void uac2_usb0_isr(uint32_t Event, void *pArg)
{
    cld_usb0_isr_callback();
}

/**
 * User defined USB 1 ISR.
 *
 * @param Event, pArg.
 */
#if defined(__ADSPSC589_FAMILY__)
static void uac2_usb1_isr(uint32_t Event, void *pArg)
{
    cld_usb1_isr_callback();
}
#endif

/**
 * Function called when the CLD library reports a status.
 *
 * @param status_code, p_additional_data, additional_data_size.
 */
static void user_cld_lib_status (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size)
{
    /*
     * WARNING: This function is called within an the USB interrupt handler.
     *          cld_lib_status_decode() calls sprintf() which ultimately
     *          calls malloc().  Do not enable this code.  It is only here
     *          to not lose track of the API call.
     */
#if 0
    char * p_str = cld_lib_status_decode(status_code, p_additional_data, additional_data_size);
#endif
}

/**
 * Function Called when a USB event occurs on the USB Audio USB Port.
 *
 * @param event - identifies which USB event has occurred.
 */
static void uac2_usb_event (CLD_USB_Event event)
{
    bool highSpeed;

    switch (event)
    {
        case CLD_USB_CABLE_CONNECTED:
            uac2_syslog("USB Cable connected");
            break;
        case CLD_USB_CABLE_DISCONNECTED:
            uac2_syslog("USB Cable disconnected");
            uac2_streaming_tx_endpoint_enabled(CLD_FALSE);
            uac2_streaming_rx_endpoint_enabled(CLD_FALSE);
            break;
        case CLD_USB_ENUMERATED_CONFIGURED:
            /* HACK: Get the speed directly from the peripheral */
            highSpeed = (*pREG_USB0_POWER & BITM_USB_POWER_HSEN);
            if (highSpeed) {
                uac2_state.minInSize = uac2_state.minInSizeHigh;
                uac2_state.maxInSize = uac2_state.maxInSizeHigh;
                uac2_state.minOutSize = uac2_state.minOutSizeHigh;
                uac2_state.maxOutSize = uac2_state.maxOutSizeHigh;
                uac2_syslog("UAC 2.0 High Speed Ready\n");
            } else {
                uac2_state.minInSize = uac2_state.minInSizeFull;
                uac2_state.maxInSize = uac2_state.maxInSizeFull;
                uac2_state.minOutSize = uac2_state.minOutSizeFull;
                uac2_state.maxOutSize = uac2_state.maxOutSizeFull;
                uac2_syslog("UAC 2.0 Low Speed Ready\n");
            }
            break;
        case CLD_USB_UN_CONFIGURED:
            uac2_syslog("UAC 2.0 Offline");
            uac2_streaming_tx_endpoint_enabled(CLD_FALSE);
            uac2_streaming_rx_endpoint_enabled(CLD_FALSE);
            break;
        case CLD_USB_BUS_RESET:
            uac2_syslog("UAC 2.0 Reset");
            uac2_streaming_tx_endpoint_enabled(CLD_FALSE);
            uac2_streaming_rx_endpoint_enabled(CLD_FALSE);
            break;
        default:
            break;
    }
}

void uac2_syslog(char *log)
{
    uac2_state.syslog[uac2_state.syslogHead++] = log;
    if (uac2_state.syslogHead == UAC2_SYSLOG_DEPTH) {
        uac2_state.syslogHead = 0;
    }
}

/**************************************************************************
 * Initialization / periodic timer / main-loop functions and data
 *************************************************************************/
/* 125uS UAC2.0 periodic timer */
#define UAC20_125US_CLOCK_TICK_PERIOD ((125ULL * SCLK0) / 1000000ULL)

/*!< CLD SC58x Audio 2.0 library initialization data. */
static CLD_SC58x_Audio_2_0_w_CDC_Lib_Init_Params uac2_init_params =
{
#if defined(__ADSPSC589_FAMILY__)
    .usb_config                                         = CLD_USB0_AUDIO_AND_CDC,
#endif
    .enable_dma                                         = CLD_TRUE,

    .audio_control_category_code                        = 0x0A,       /* Pro Audio */
    .p_audio_control_interrupt_params                   = CLD_NULL,

    .p_unit_and_terminal_descriptors                    = CLD_NULL,   /* Set during cfg */
    .unit_and_terminal_descriptors_length               = 0,          /* Set during cfg */
    .p_audio_streaming_rx_interface_params              = CLD_NULL,   /* Set during cfg */
    .p_audio_rate_feedback_rx_params                    = CLD_NULL,   /* Set during cfg */
    .p_audio_streaming_tx_interface_params              = CLD_NULL,   /* Set during cfg */

    .fp_audio_stream_data_received                      = uac2_stream_data_received,

    .fp_audio_set_req_cmd                               = uac2_set_req_cmd,
    .fp_audio_get_req_cmd                               = uac2_get_req_cmd,

    .fp_audio_streaming_rx_endpoint_enabled             = uac2_streaming_rx_endpoint_enabled,
    .fp_audio_streaming_tx_endpoint_enabled             = uac2_streaming_tx_endpoint_enabled,

    .p_usb_string_audio_control_interface               = CLD_NULL,
    .p_usb_string_audio_streaming_out_interface         = "Audio 2.0 Output",
    .p_usb_string_audio_streaming_in_interface          = "Audio 2.0 Input",

    .user_string_descriptor_table_num_entries           = 0,
    .p_user_string_descriptor_table                     = NULL,

    .usb_string_language_id                             = 0x0409,        /* English (US) language ID */

#if defined(__ADSPSC589_FAMILY__)
    .usb_port_settings[CLD_USB_0].vendor_id             = 0,             /* Set during cfg */
    .usb_port_settings[CLD_USB_0].product_id            = 0,             /* Set during cfg */
    .usb_port_settings[CLD_USB_0].usb_bus_max_power     = 0,
    .usb_port_settings[CLD_USB_0].device_descriptor_bcdDevice = 0x0100,
    .usb_port_settings[CLD_USB_0].p_usb_string_manufacturer  = CLD_NULL, /* Set during cfg */
    .usb_port_settings[CLD_USB_0].p_usb_string_product       = CLD_NULL, /* Set during cfg */
    .usb_port_settings[CLD_USB_0].p_usb_string_serial_number = CLD_NULL,
    .usb_port_settings[CLD_USB_0].p_usb_string_configuration = CLD_NULL,
    .usb_port_settings[CLD_USB_0].fp_cld_usb_event_callback = uac2_usb_event,

    .usb_port_settings[CLD_USB_1].vendor_id             = 0,             /* Set during cfg */
    .usb_port_settings[CLD_USB_1].product_id            = 0,             /* Set during cfg */
    .usb_port_settings[CLD_USB_1].usb_bus_max_power     = 0,
    .usb_port_settings[CLD_USB_1].device_descriptor_bcdDevice = 0x0100,
    .usb_port_settings[CLD_USB_1].p_usb_string_manufacturer  = CLD_NULL, /* Set during cfg */
    .usb_port_settings[CLD_USB_1].p_usb_string_product       = CLD_NULL, /* Set during cfg */
    .usb_port_settings[CLD_USB_1].p_usb_string_serial_number = CLD_NULL,
    .usb_port_settings[CLD_USB_1].p_usb_string_configuration = CLD_NULL,
    .usb_port_settings[CLD_USB_1].fp_cld_usb_event_callback = uac2_usb_event,
#else
    .vendor_id                   = 0,         /* Set during cfg */
    .product_id                  = 0,         /* Set during cfg */
    .usb_bus_max_power           = 0,
    .device_descriptor_bcdDevice = 0x0100,
    .p_usb_string_manufacturer   = CLD_NULL,  /* Set during cfg */
    .p_usb_string_product        = CLD_NULL,  /* Set during cfg */
    .p_usb_string_serial_number  = CLD_NULL,
    .p_usb_string_configuration  = CLD_NULL,
    .fp_cld_usb_event_callback   = uac2_usb_event,
#endif

    /* Begin CDC settings (cdc.h) */

    .p_serial_data_rx_endpoint_params                   = &cdc_serial_data_rx_ep_params,
    .p_serial_data_tx_endpoint_params                   = &cdc_serial_data_tx_ep_params,
    .p_notification_endpoint_params                     = &cdc_notification_ep_params,

    .fp_serial_data_received                            = cdc_serial_data_received,

    .fp_cdc_cmd_send_encapsulated_cmd                   = cdc_cmd_send_encapsulated_cmd,
    .fp_cdc_cmd_get_encapsulated_resp                   = cdc_cmd_get_encapsulated_resp,

    .fp_cdc_cmd_set_line_coding                         = cdc_cmd_set_line_coding,
    .fp_cdc_cmd_get_line_coding                         = cdc_cmd_get_line_coding,

    .fp_cdc_cmd_set_control_line_state                  = cdc_cmd_set_control_line_state,

    .fp_cdc_cmd_send_break                              = cdc_cmd_send_break,

    .support_cdc_network_connection                     = 1,
    .cdc_class_bcd_version                              = 0x0120,       /* CDC Version 1.2 */
    .cdc_class_control_protocol_code                    = 0,            /* No Class Specific protocol */

    .p_usb_string_communication_class_interface         = "CLD CDC Ctrl",
    .p_usb_string_data_class_interface                  = "CLD CDC Data",

    /* End CDC settings */

    .fp_cld_lib_status                                  = user_cld_lib_status,

};


/**
 * User defined Timer ISR.
 *
 * @param pCBParam, Event, pArg.
 */
static void uac20_timer_handler(void *pCBParam, uint32_t Event, void *pArg)
{
    uint32_t inCycles, outCycles;

    inCycles = cpuLoadGetTimeStamp();

    if (Event == ADI_TMR_EVENT_DATA_INT) {
        cld_time_125us_tick();
    }

    outCycles = cpuLoadGetTimeStamp();
    cpuLoadIsrCycles(outCycles - inCycles);
}

/**
* @brief Starts the UAC2 125uS periodic timer.
*/
static CLD_RV uac20_timer_start(uint32_t num, ADI_TMR_HANDLE *hTimer)
{
    static unsigned char TimerMemory[ADI_TMR_MEMORY];
    ADI_TMR_RESULT eTmrResult;

    if (hTimer == NULL) {
         return(CLD_FAIL);
    }

    /* Open the timer */
    eTmrResult = adi_tmr_Open (
        num, TimerMemory, ADI_TMR_MEMORY,
        uac20_timer_handler, NULL, hTimer
    );
    if (eTmrResult != ADI_TMR_SUCCESS) {
         return(CLD_FAIL);
    }

    /* Set the mode to PWM OUT */
    eTmrResult = adi_tmr_SetMode(*hTimer, ADI_TMR_MODE_CONTINUOUS_PWMOUT);

    /* Set the IRQ mode to get interrupt after timer counts to Delay + Width */
    eTmrResult = adi_tmr_SetIRQMode(*hTimer, ADI_TMR_IRQMODE_WIDTH_DELAY);

    /* Set the Period */
    eTmrResult = adi_tmr_SetPeriod(*hTimer, UAC20_125US_CLOCK_TICK_PERIOD);

    /* Set the timer width */
    eTmrResult = adi_tmr_SetWidth(*hTimer, UAC20_125US_CLOCK_TICK_PERIOD / 2);

    /* Set the timer delay */
    eTmrResult = adi_tmr_SetDelay(*hTimer, 100);

    /* Enable the timer */
    eTmrResult = adi_tmr_Enable(*hTimer, true);

    return(CLD_SUCCESS);
}

unsigned char *createTerminalAndFeatureDescriptors(unsigned short *descriptorLength)
{
    uac2_input_terminal_descriptor *inputTerminalUsbOut;
    uac2_input_terminal_descriptor *inputTerminalMicrophone;
    uac2_output_terminal_descriptor *outputTerminalSpeaker;
    uac2_output_terminal_descriptor *outputTerminalUsbIn;
    uac2_feature_unit_descriptor *speakerVolume;
    uac2_feature_unit_descriptor *micVolume;
    uac2_clock_source_descriptor *cs1;
    uac2_clock_source_descriptor *cs2;

    unsigned char *descriptors;
    unsigned short offset, length;

    descriptors = NULL;
    offset = 0;
    length = 0;

    /* INPUT TERMINAL: USB OUT */
    inputTerminalUsbOut = newInputTerminalDescriptor(
        USB_OUT_INPUT_TERMINAL_ID, UAC_TERMINAL_TYPE_USB_STREAMING,
        uac2_state.cfg.usbOutChannels, SPKR_CLOCK_SOURCE_ID
    );
    length = inputTerminalUsbOut->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, inputTerminalUsbOut, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(inputTerminalUsbOut);

    /* INPUT TERMINAL: Microphone */
    inputTerminalMicrophone = newInputTerminalDescriptor(
        MIC_INPUT_TERMINAL_ID, UAC_TERMINAL_TYPE_MICROPHONE,
        uac2_state.cfg.usbInChannels, MIC_CLOCK_SOURCE_ID
    );
    length = inputTerminalMicrophone->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, inputTerminalMicrophone, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(inputTerminalMicrophone);

    /* OUTPUT TERMINAL: Speaker */
    outputTerminalSpeaker = newOutputTerminalDescriptor (
        SPKR_OUTPUT_TERMINAL_ID, UAC_TERMINAL_TYPE_SPEAKER,
        SPKR_FEATURE_UNIT_ID, SPKR_CLOCK_SOURCE_ID
    );
    length = outputTerminalSpeaker->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, outputTerminalSpeaker, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(outputTerminalSpeaker);

    /* OUTPUT TERMINAL: USB IN */
    outputTerminalUsbIn = newOutputTerminalDescriptor (
        USB_IN_OUTPUT_TERMINAL_ID, UAC_TERMINAL_TYPE_USB_STREAMING,
        MIC_FEATURE_UNIT_ID, MIC_CLOCK_SOURCE_ID
    );
    length = outputTerminalUsbIn->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, outputTerminalUsbIn, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(outputTerminalUsbIn);

    /* FEATURE UNIT: Speaker volume */
    speakerVolume = newFeatureUnitDescriptor (
        SPKR_FEATURE_UNIT_ID, USB_OUT_INPUT_TERMINAL_ID,
        uac2_state.cfg.usbOutChannels, false
    );
    length = speakerVolume->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, speakerVolume, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(speakerVolume);

    /* FEATURE UNIT: Mic volume */
    micVolume = newFeatureUnitDescriptor (
        MIC_FEATURE_UNIT_ID, MIC_INPUT_TERMINAL_ID,
        uac2_state.cfg.usbInChannels, false
    );
    length = micVolume->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, micVolume, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(micVolume);

    /* CLOCK SOURCE UNIT: cs1 */
    cs1 = newClockSourceDescriptor(SPKR_CLOCK_SOURCE_ID);
    length = cs1->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, cs1, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(cs1);

    /* CLOCK SOURCE UNIT: cs2 */
    cs2 = newClockSourceDescriptor(MIC_CLOCK_SOURCE_ID);
    length = cs2->bLength;
    descriptors = UAC20_DESCRIPTORS_REALLOC(descriptors, offset + length);
    UAC2_MEMCPY(descriptors + offset, cs2, length);
    offset += length;
    UAC20_DESCRIPTORS_FREE(cs2);

    if (descriptorLength) {
        *descriptorLength = offset;
    }

    return(descriptors);
}

/**
 * Initializes the CLD SC58x USB Audio 2.0 state.
 */
CLD_RV uac2_init(void)
{
    /* Reset state variables */
    uac2_state.volume_changed = CLD_FALSE;
    uac2_state.in_enabled = CLD_FALSE;
    uac2_state.in_idle = CLD_TRUE;
    uac2_state.in_active = CLD_FALSE;
    uac2_state.in_data = NULL;
    uac2_state.in_data_allocated = CLD_FALSE;
    uac2_state.in_size = 0;
    uac2_state.minInSize = 0;
    uac2_state.maxInSize = 0;
    uac2_state.minInSizeFull = 0;
    uac2_state.maxInSizeFull = 0;
    uac2_state.minInSizeHigh = 0;
    uac2_state.maxInSizeHigh = 0;
    uac2_state.out_enabled = CLD_FALSE;
    uac2_state.out_data = NULL;
    uac2_state.out_data_allocated = CLD_FALSE;
    uac2_state.out_size = 0;
    uac2_state.minOutSize = 0;
    uac2_state.maxOutSize = 0;
    uac2_state.minOutSizeFull = 0;
    uac2_state.maxOutSizeFull = 0;
    uac2_state.minOutSizeHigh = 0;
    uac2_state.maxOutSizeHigh = 0;
    uac2_state.rate_feedback_idle = CLD_TRUE;
    uac2_state.first_feedback = CLD_TRUE;
    uac2_state.periodic_timer_handle = NULL;
    uac2_state.inPktFirst = CLD_TRUE;
    uac2_state.outPktFirst = CLD_TRUE;
    memset(uac2_state.syslog, 0, sizeof(uac2_state.syslog));
    uac2_state.syslogHead = 0;
    uac2_state.syslogTail = 0;

    /* Initialize constant volume state:
     *
     *   Minimum Volume = -100.0 dB
     *   Maximum Volume = 0.0dB
     *   Resolution = 1.0dB
     *
     * See "Audio Final.pdf" section 5.2.5.7.2 Volume Control for
     * more information about these attributes.
     */
    uac2_state.speaker_output_volume.min = 0x9c00;
    uac2_state.speaker_output_volume.max = 0x0000;
    uac2_state.speaker_output_volume.resolution = 0x0100;

    uac2_state.mic_input_volume.min = 0x9c00;
    uac2_state.mic_input_volume.max = 0x0000;
    uac2_state.mic_input_volume.resolution = 0x0100;

    return(CLD_SUCCESS);
}

/**
 * Configures the CLD SC58x USB Audio 2.0 library.
 */
CLD_RV uac2_config(UAC2_APP_CONFIG *cfg)
{
    uint8_t i;

    /* Copy in application configuration parameters */
    UAC2_MEMCPY(&uac2_state.cfg, cfg, sizeof(uac2_state.cfg));

    /* Initialize clock source info */
    uac2_state.clock_source.current = uac2_state.cfg.usbSampleRate;
    uac2_state.clock_source.min = uac2_state.cfg.usbSampleRate;
    uac2_state.clock_source.max = uac2_state.cfg.usbSampleRate;
    uac2_state.clock_source.resolution = 0;

    /* Copy static parameters from app cfg to CLD init params */
#if defined(__ADSPSC589_FAMILY__)
    if (cfg->port == CLD_USB_0) {
        uac2_init_params.usb_config = CLD_USB0_AUDIO_AND_CDC;
        uac2_init_params.usb_port_settings[CLD_USB_0].vendor_id = cfg->vendorId;
        uac2_init_params.usb_port_settings[CLD_USB_0].product_id = cfg->productId;
        uac2_init_params.usb_port_settings[CLD_USB_0].p_usb_string_manufacturer = cfg->mfgString;
        uac2_init_params.usb_port_settings[CLD_USB_0].p_usb_string_product = cfg->productString;
        uac2_init_params.usb_port_settings[CLD_USB_0].p_usb_string_serial_number = cfg->serialNumString;
    } else {
        uac2_init_params.usb_config = CLD_USB1_AUDIO_AND_CDC;
        uac2_init_params.usb_port_settings[CLD_USB_1].vendor_id = cfg->vendorId;
        uac2_init_params.usb_port_settings[CLD_USB_1].product_id = cfg->productId;
        uac2_init_params.usb_port_settings[CLD_USB_1].p_usb_string_manufacturer = cfg->mfgString;
        uac2_init_params.usb_port_settings[CLD_USB_1].p_usb_string_product = cfg->productString;
        uac2_init_params.usb_port_settings[CLD_USB_1].p_usb_string_serial_number = cfg->serialNumString;
    }
#else
    uac2_init_params.vendor_id = cfg->vendorId;
    uac2_init_params.product_id = cfg->productId;
    uac2_init_params.p_usb_string_manufacturer = cfg->mfgString;
    uac2_init_params.p_usb_string_product = cfg->productString;
    uac2_init_params.p_usb_string_serial_number = cfg->serialNumString;
#endif

    /* Create the Terminal and Feature Unit Descriptors for this
     * signal flow
     */
    uac2_init_params.p_unit_and_terminal_descriptors =
        createTerminalAndFeatureDescriptors(
            &uac2_init_params.unit_and_terminal_descriptors_length
        );

    /* Create and configure IN Endpoint Descriptors */
    uac2_format_type_i_descriptor *inFormatDescriptor =
        newFormatTypeIDescriptor(cfg->usbInWordSizeBits);
    CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor
        *inEndpointDescriptor = newAudioStreamEndpointDescriptor(0, 0);
    uac2_init_params.p_audio_streaming_tx_interface_params =
        newStreamInterfaceParams(
            USB_IN_ENDPOINT_ID, USB_IN_OUTPUT_TERMINAL_ID,
            &uac2_state.minInSizeFull, &uac2_state.maxInSizeFull,
            &uac2_state.minInSizeHigh, &uac2_state.maxInSizeHigh,
            uac2_state.cfg.usbSampleRate, uac2_state.cfg.usbInChannels,
            uac2_state.cfg.lowLatency,
            inFormatDescriptor, inEndpointDescriptor
        );

    /* Create and configure OUT Endpoint Descriptors */
    uac2_format_type_i_descriptor *outFormatDescriptor =
        newFormatTypeIDescriptor(cfg->usbOutWordSizeBits);
    CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor
        *outEndpointDescriptor = newAudioStreamEndpointDescriptor(2, 1);
    uac2_init_params.p_audio_streaming_rx_interface_params =
        newStreamInterfaceParams(
            USB_OUT_ENDPOINT_ID, USB_OUT_INPUT_TERMINAL_ID,
            &uac2_state.minOutSizeFull, &uac2_state.maxOutSizeFull,
            &uac2_state.minOutSizeHigh, &uac2_state.maxOutSizeHigh,
            uac2_state.cfg.usbSampleRate, uac2_state.cfg.usbOutChannels,
            uac2_state.cfg.lowLatency,
            outFormatDescriptor, outEndpointDescriptor
        );

    /* Create and configure Rate Feedback parameters */
    uac2_init_params.p_audio_rate_feedback_rx_params =
        newRateFeedbackParams(USB_RATE_FEEDBACK_RATE_MS);

    /* Allocate storage for the channel volume settings in
     * uncached memory.
     */
    uac2_state.speaker_output_volume.vol = UAC2_CALLOC(
        uac2_state.cfg.usbOutChannels,
        sizeof(*uac2_state.speaker_output_volume.vol)
    );
    uac2_state.mic_input_volume.vol = UAC2_CALLOC(
        uac2_state.cfg.usbInChannels,
        sizeof(*uac2_state.mic_input_volume.vol)
    );

    /* Set current output volume to max */
    for (i = 0; i < uac2_state.cfg.usbOutChannels; i++) {
        uac2_state.speaker_output_volume.vol[i] =
            uac2_state.speaker_output_volume.max;
    }
    uac2_state.speaker_output_volume.master =
        uac2_state.speaker_output_volume.max;
    uac2_state.speaker_output_volume.mute = 0;

    /* Set current input volume to max */
    for (i = 0; i < uac2_state.cfg.usbInChannels; i++) {
        uac2_state.mic_input_volume.vol[i] = uac2_state.mic_input_volume.max;
    }

    /* Set master volume to max and unmute */
    uac2_state.mic_input_volume.master = uac2_state.mic_input_volume.max;
    uac2_state.mic_input_volume.mute = 0;

    return(CLD_SUCCESS);
}

/**
 * Queries min/max RX packet sizes for application allocation
 */
CLD_RV uac2_getRxPktSize(uint16_t *minFull, uint16_t *maxFull,
    uint16_t *minHigh, uint16_t *maxHigh)
{
    if (minFull) {
        *minFull = uac2_state.minOutSizeFull;
    }
    if (maxFull) {
        *maxFull = uac2_state.maxOutSizeFull;
    }
    if (minHigh) {
        *minHigh = uac2_state.minOutSizeHigh;
    }
    if (maxHigh) {
        *maxHigh = uac2_state.maxOutSizeHigh;
    }
    return(CLD_SUCCESS);
}

/**
 * Queries min/max TX packet sizes for application allocation
 */
CLD_RV uac2_getTxPktSize(uint16_t *minFull, uint16_t *maxFull,
    uint16_t *minHigh, uint16_t *maxHigh)
{
    if (minFull) {
        *minFull = uac2_state.minInSizeFull;
    }
    if (maxFull) {
        *maxFull = uac2_state.maxInSizeFull;
    }
    if (minHigh) {
        *minHigh = uac2_state.minInSizeHigh;
    }
    if (maxHigh) {
        *maxHigh = uac2_state.maxInSizeHigh;
    }
    return(CLD_SUCCESS);
}

/**
 * Sets an application allocated RX packet buffer
 */
CLD_RV uac2_setRxPktBuffer(void *buf)
{
    uac2_state.out_data = buf;
    uac2_state.out_data_allocated = CLD_FALSE;
    return(CLD_SUCCESS);
}

/**
 * Sets an application allocated TX packet buffer
 */
CLD_RV uac2_setTxPktBuffer(void *buf)
{
    uac2_state.in_data = buf;
    uac2_state.in_data_allocated = CLD_FALSE;
    return(CLD_SUCCESS);
}

/**
 * Starts the CLD SC58x USB Audio 2.0 Library.
 */
CLD_RV uac2_start(void)
{
    CLD_RV cld_rv = CLD_ONGOING;

    /* Allocate storage for USB audio transfer buffers in uncached
     * memory. */
    if (uac2_state.out_data == NULL) {
        uac2_state.out_data = UAC2_L2_MALLOC_ALIGNED(USB_MAX_PACKET_SIZE);
        uac2_state.out_data_allocated = CLD_TRUE;
    }
    if (uac2_state.in_data == NULL) {
        uac2_state.in_data = UAC2_L2_MALLOC_ALIGNED(USB_MAX_PACKET_SIZE);
        uac2_state.in_data_allocated = CLD_TRUE;
    }

    /* Start the periodic timer */
    cld_rv = uac20_timer_start(
        uac2_state.cfg.timerNum, &uac2_state.periodic_timer_handle
    );
    if (cld_rv != CLD_SUCCESS) {
        return(cld_rv);
    }

    /* Initialize the CLD Audio v2.0 with CDC Library */
    do {
        cld_rv = cld_sc58x_audio_2_0_w_cdc_lib_init(&uac2_init_params);
    } while (cld_rv == CLD_ONGOING);

    /* Connect if successful */
    if (cld_rv == CLD_SUCCESS)
    {
        /* Register and enable the USB interrupt ISR functions */
#if defined(__ADSPSC589_FAMILY__)
        if (uac2_init_params.usb_config == CLD_USB0_AUDIO_AND_CDC) {
            adi_int_InstallHandler(INTR_USB0_STAT, uac2_usb0_isr, NULL, 1);
            adi_int_InstallHandler(INTR_USB0_DATA, uac2_usb0_isr, NULL, 1);
        }
        if (uac2_init_params.usb_config == CLD_USB1_AUDIO_AND_CDC) {
            adi_int_InstallHandler(INTR_USB1_STAT, uac2_usb1_isr, NULL, 1);
            adi_int_InstallHandler(INTR_USB1_DATA, uac2_usb1_isr, NULL, 1);
        }
#else
        if (1) {
            adi_int_InstallHandler(INTR_USB0_STAT, uac2_usb0_isr, NULL, 1);
            adi_int_InstallHandler(INTR_USB0_DATA, uac2_usb0_isr, NULL, 1);
        }
#endif
        /* Connect to the USB Host */
#if defined(__ADSPSC589_FAMILY__)
        cld_lib_usb_connect(uac2_state.cfg.port);
#else
        cld_lib_usb_connect();
#endif
    }

    return(cld_rv);
}

/**
 * Stops the CLD SC58x USB Audio 2.0 Library and frees all descriptors.
 */
CLD_RV uac2_stop(void)
{
    /* Disconnect from the USB Host */
#if defined(__ADSPSC589_FAMILY__)
    cld_lib_usb_disconnect(uac2_state.cfg.port);
#else
    cld_lib_usb_disconnect();
#endif

    /* Turn off the periodic timer */
    if (uac2_state.periodic_timer_handle) {
        adi_tmr_Close(uac2_state.periodic_timer_handle);
        uac2_state.periodic_timer_handle = NULL;
    }

    /* Unregister and disable the USB interrupt ISR functions */
#if defined(__ADSPSC589_FAMILY__)
    if (uac2_init_params.usb_config == CLD_USB0_AUDIO_AND_CDC) {
        adi_int_UninstallHandler(INTR_USB0_STAT);
        adi_int_UninstallHandler(INTR_USB0_DATA);
    }
    if (uac2_init_params.usb_config == CLD_USB1_AUDIO_AND_CDC) {
        adi_int_UninstallHandler(INTR_USB1_STAT);
        adi_int_UninstallHandler(INTR_USB1_DATA);
    }
#else
    if (1) {
        adi_int_UninstallHandler(INTR_USB0_STAT);
        adi_int_UninstallHandler(INTR_USB0_DATA);
    }
#endif

    /* Free the IN and OUT transfer buffers */
    if ((uac2_state.in_data_allocated == CLD_TRUE) && uac2_state.in_data) {
        UAC2_L2_FREE_ALIGNED(uac2_state.in_data);
        uac2_state.in_data = NULL;
    }
    if ((uac2_state.out_data_allocated == CLD_TRUE) && uac2_state.out_data) {
        UAC2_L2_FREE_ALIGNED(uac2_state.out_data);
        uac2_state.out_data = NULL;
    }

    /* Free channel volume settings */
    if (uac2_state.speaker_output_volume.vol) {
        UAC2_FREE(uac2_state.speaker_output_volume.vol);
        uac2_state.speaker_output_volume.vol = NULL;
    }
    if (uac2_state.mic_input_volume.vol) {
        UAC2_FREE(uac2_state.mic_input_volume.vol);
        uac2_state.mic_input_volume.vol = NULL;
    }

    /* Free all descriptors */
    if (uac2_init_params.p_unit_and_terminal_descriptors) {
        UAC20_DESCRIPTORS_FREE(
            uac2_init_params.p_unit_and_terminal_descriptors
        );
        uac2_init_params.p_unit_and_terminal_descriptors = CLD_NULL;
    }
    if (uac2_init_params.p_audio_streaming_tx_interface_params) {
        freeStreamInterfaceParams(
            uac2_init_params.p_audio_streaming_tx_interface_params
        );
        uac2_init_params.p_audio_streaming_tx_interface_params = CLD_NULL;
    }
    if (uac2_init_params.p_audio_streaming_rx_interface_params) {
        freeStreamInterfaceParams(
            uac2_init_params.p_audio_streaming_rx_interface_params
        );
        uac2_init_params.p_audio_streaming_tx_interface_params = CLD_NULL;
    }
    if (uac2_init_params.p_audio_rate_feedback_rx_params) {
        UAC20_DESCRIPTORS_FREE(
            uac2_init_params.p_audio_rate_feedback_rx_params
        );
        uac2_init_params.p_audio_rate_feedback_rx_params = CLD_NULL;
    }

    return(CLD_SUCCESS);
}

/**
 * Application polling loop.
 *
 */
CLD_RV uac2_run(void)
{
    static CLD_Time main_time = 0;

    cld_sc58x_audio_2_0_w_cdc_lib_main();

    /* How to check elapsed time */
    if (cld_time_passed_ms(main_time) >= 250u) {
        main_time = cld_time_get();
    }

    /* Primitive method to save ISR messages to the syslog */
    while (uac2_state.syslogTail != uac2_state.syslogHead) {
        syslog_print(uac2_state.syslog[uac2_state.syslogTail++]);
        if (uac2_state.syslogTail == UAC2_SYSLOG_DEPTH) {
            uac2_state.syslogTail = 0;
        }
    }

    /* Disable interrupts */
    adi_rtl_disable_interrupts();

    /* Apply volume control changes here */
    if (uac2_state.volume_changed == CLD_TRUE) {
        uac2_state.volume_changed = CLD_FALSE;
    }

    /* Poll for rate feedback */
    uac2_tx_feedback_data();

    /* Poll for USB IN / Tx data */
    uac2_tx_audio_data();

    /* Re-enable interrupts */
    adi_rtl_reenable_interrupts();

    return(CLD_SUCCESS);
}

