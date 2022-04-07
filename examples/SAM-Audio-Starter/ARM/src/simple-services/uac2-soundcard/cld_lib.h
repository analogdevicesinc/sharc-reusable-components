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

#ifndef _cld_lib_h
#define _cld_lib_h

#if defined(__ADSPSC589_FAMILY__)
    #include "cld_sc58x_audio_2_0_w_cdc_lib.h"
    #include <adi/cortex-a5/sys/ADSP_SC589_cdef.h>
    #include <sys/ADSP_SC589.h>
#else
    #include "cld_sc57x_audio_2_0_w_cdc_lib.h"
    #include <adi/cortex-a5/sys/ADSP_SC573_cdef.h>
    #include <sys/ADSP_SC573.h>

    /* CDC macros */
    #define CLD_SC58x_CDC_Line_Coding \
        CLD_SC57x_CDC_Line_Coding
    #define CLD_SC58x_CDC_Notification_Endpoint_Params \
        CLD_SC57x_CDC_Notification_Endpoint_Params
    #define CLD_SC58x_CDC_Control_Line_State \
        CLD_SC57x_CDC_Control_Line_State
    #define CLD_SC58x_CDC_LINE_CODING_1_STOP_BITS \
        CLD_SC57x_CDC_LINE_CODING_1_STOP_BITS
    #define CLD_SC58x_CDC_LINE_CODING_PARITY_NONE \
        CLD_SC57x_CDC_LINE_CODING_PARITY_NONE
    #define cld_sc58x_cdc_lib_transmit_serial_data \
        cld_sc57x_cdc_lib_transmit_serial_data

    /* UAC2 macros */
    #define CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor \
        CLD_SC57x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor
    #define CLD_SC58x_Audio_2_0_Stream_Interface_Params \
        CLD_SC57x_Audio_2_0_Stream_Interface_Params
    #define CLD_SC58x_Audio_2_0_Rate_Feedback_Params \
        CLD_SC57x_Audio_2_0_Rate_Feedback_Params
    #define CLD_SC58x_Audio_2_0_Cmd_Req_Parameters \
        CLD_SC57x_Audio_2_0_Cmd_Req_Parameters
    #define cld_sc58x_audio_2_0_w_cdc_lib_transmit_audio_rate_feedback_data \
        cld_sc57x_audio_2_0_w_cdc_lib_transmit_audio_rate_feedback_data
    #define cld_sc58x_audio_2_0_w_cdc_lib_transmit_audio_data \
        cld_sc57x_audio_2_0_w_cdc_lib_transmit_audio_data
    #define CLD_SC58x_Audio_2_0_w_CDC_Lib_Init_Params \
        CLD_SC57x_Audio_2_0_w_CDC_Lib_Init_Params
    #define cld_sc58x_audio_2_0_w_cdc_lib_init \
        cld_sc57x_audio_2_0_w_cdc_lib_init
    #define cld_sc58x_audio_2_0_w_cdc_lib_main \
        cld_sc57x_audio_2_0_w_cdc_lib_main

#endif

#endif
