#ifndef __USER_CDC__
#define __USER_CDC__
/*=============================================================================
    FILE:           user_cdc.h

    DESCRIPTION:    Uses the cld_sc58x_cdc_lib library to implement a basic
                    USB CDC/ACM device.

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
 * @file      user_cdc.h
 * @brief     Uses the cld_sc58x_cdc_lib library to implement a basic
 *            USB CDC/ACM device.
 *
 * @details
 *            User defined interface with the cld_sc58x_cdc_lib library to
 *            implement a custom USB CDC device.
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



/**
 * User Initialization function return values
 */
typedef enum
{
    USER_CDC_INIT_SUCCESS = 0,    /*!< Initialization successful */
    USER_CDC_INIT_ONGOING,        /*!< Initialization in process */
    USER_CDC_INIT_FAILED,         /*!< Initialization failed */
} User_Init_Return_Code;

extern User_Init_Return_Code user_cdc_init (void);
extern void user_cdc_main (void);

extern void user_cdc_timer_handler(void *pCBParam, unsigned long Event, void *pArg);

enum _xfer_state {
    XFER_STATE_IDLE = 0,
    XFER_STATE_ACCEPTED,
    XFER_STATE_DATA_READY,
};

extern volatile int xfer_state;
extern unsigned char user_cdc_serial_data_rx_buffer[];
extern unsigned short user_cdc_serial_data_num_rx_bytes;
extern volatile int paused;

extern void user_cdc_tx_serial_data (unsigned short length, unsigned char * p_buffer);

#endif /* __USER_CDC__ */
