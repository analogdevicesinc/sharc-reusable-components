#ifndef __USER_AUDIO_W_CDC__
#define __USER_AUDIO_W_CDC__
/*==============================================================================
    FILE:           user_audio_w_cdc.h

    DESCRIPTION:    Uses the cld_sc58x_audio_2_0_w_cdc_lib library to implement a basic
                    USB audio 2.0 and CDC/ACM device.

    Copyright (c) 2018 Closed Loop Design, LLC

    This software is supplied "AS IS" without any warranties, express, implied
    or statutory, including but not limited to the implied warranties of fitness
    for purpose, satisfactory quality and non-infringement. Closed Loop Design LLC
    extends you a royalty-free right to use, reproduce and distribute this source
    file as well as executable files created using this source file for use with
    Analog Devices SC5xx family processors only. Nothing else gives you
    the right to use this source file.

==============================================================================*/
/*!
 * @file      user_audio_w_cdc.h
 * @brief     Uses the cld_sc58x_audio_2_0_w_cdc_lib library to implement a basic
 *            USB audio 2.0 and CDC/ACM device.
 *
 * @details
 *            User defined interface with the cld_sc58x_audio_2_0_w_cdc_lib library to
 *            implement a custom USB Audio 2.0 and CDC device.
 *
 *            Copyright (c) 2018 Closed Loop Design, LLC
 *
 *            This software is supplied "AS IS" without any warranties, express, implied
 *            or statutory, including but not limited to the implied warranties of fitness
 *            for purpose, satisfactory quality and non-infringement. Closed Loop Design LLC
 *            extends you a royalty-free right to use, reproduce and distribute this source
 *            file as well as executable files created using this source file for use with
 *            Analog Devices SC5xx family processors only. Nothing else gives you
 *            the right to use this source file.
 *
 */

#if 0
#include "ll_mgr.h"
#endif

/**
 * When set to 1 the USB audio output data is looped back to the USB Host.
 * When set to 0 the USB audio output data is sent to the Audio CODECs.
 */
#define USER_AUDIO_LOOPBACK             0

/**
 * When set to 1 the channel volume settings are configured as Read-Only.
 */
#define USER_AUDIO_READ_ONLY_VOLUME     0

/**
 * When set to 1 the audio sample rate is set to 48kHz.
 * When set to 0 the audio sample rate is set to 44.1kHz
 */
#define USER_AUDIO_48kHz_SAMPLE_RATE    1

/**
 * When set to 1 the audio rate feedback changes based on status of amount of buffered audio data.
 * When set to 0 the audio rate feedback is hard coded to match the selected sample rate.
 */
#define USER_AUDIO_VARIABLE_FEEDBACK    1


/**
 * User Initialization function return values
 */
typedef enum
{
    USER_AUDIO_INIT_SUCCESS = 0,    /*!< Initialization successful */
    USER_AUDIO_INIT_ONGOING,        /*!< Initialization in process */
    USER_AUDIO_INIT_FAILED,         /*!< Initialization failed */
} User_Init_Return_Code;

extern User_Init_Return_Code user_audio_init (void);
extern void user_audio_main (void);

#endif /* __USER_AUDIO_W_CDC__ */
