#ifndef __CLD_CDC_LIB__
#define __CLD_CDC_LIB__
/*=============================================================================
    FILE:           cld_sc58x_cdc_lib.h

    DESCRIPTION:    CLD CDC peripheral Library.

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
 * @file      cld_sc58x_cdc_lib.h
 * @brief     CLD USB CDC library header file.
 *
 * @details
 *            This file contains the functionality needed to interface with the
 *            CLD CDC/ACM library using the SC58x's Cortex-A5 processor.
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

#ifndef CLD_NULL
/**
 * Defines a CLD library NULL value
 */
#define CLD_NULL    0
#endif

#if defined(_LANGUAGE_C) || defined(__cplusplus) || (defined(__GNUC__) && !defined(__ASSEMBLER__))

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * CLD_Time typedef
 * used by CLD time keeping functions.
 */
typedef unsigned long CLD_Time;

/**
 * SC58x USB Port Configuration
 */
typedef enum
{
    CLD_USB0_CDC = 0, /*!< CDC interface connected to USB0 */
    CLD_USB1_CDC,     /*!< CDC interface connected to USB1 */
} CLD_SC58x_USB_Config;

/**
 * SC58x USB Ports
 */
typedef enum
{
    CLD_USB_0 = 0,              /*!< SC58x USB Port 0 */
    CLD_USB_1,                  /*!< SC58x USB Port 1 */
    CLD_USB_NUM_PORTS,          /*!< Max number of SC58x USB Ports */
} CLD_SC58x_USB_Port_Num;

/**
 * CLD_RV enum defines the CLD library function return codes.
 */
typedef enum
{
    CLD_SUCCESS = 0,    /*!< Process completed successfully. */
    CLD_FAIL = 1,       /*!< Process failed. */
    CLD_ONGOING = 2     /*!< Process isn't complete. */
} CLD_RV;

/**
 * CLD_Boolean enum defines the CLD library's boolean values.
 */
typedef enum
{
    CLD_FALSE = 0,      /*!< True. */
    CLD_TRUE  = 1,      /*!< False. */
} CLD_Boolean;

/**
 * CLD Library USB Events
 */
typedef enum
{
    CLD_USB_CABLE_CONNECTED = 0,    /*!< USB Cable Connected. */
    CLD_USB_CABLE_DISCONNECTED,     /*!< USB Cable Disconnected. */
    CLD_USB_ENUMERATED_CONFIGURED,  /*!< USB Device configured (USB Set Configuration command received enabling the device). */
    CLD_USB_UN_CONFIGURED,          /*!< USB Device unconfigured (USB Set Configuration command received disabling the device). */
    CLD_USB_BUS_RESET,              /*!< USB Bus Reset. */
} CLD_USB_Event;

/**
 * Value returned by the User code to select how the requested transfer should
 * be handled.
 */
typedef enum
{
    CLD_USB_TRANSFER_ACCEPT = 0,                        /*!< Accept the requested transfer using
                                                             the supplied CLD_USB_Transfer_Params. */
    CLD_USB_TRANSFER_PAUSE,                             /*!< Pause the current transfer. The USB
                                                             endpoint will be Nak'ed until the
                                                             appropriate 'resume' function is
                                                             called. This is used to throttle
                                                             the USB host. */
    CLD_USB_TRANSFER_DISCARD,                           /*!< Discard the transfer. For OUT requests
                                                             the received data is discarded. For IN
                                                             requests the device will return a
                                                             zero length packet. */
    CLD_USB_TRANSFER_STALL,                             /*!< Stall the transfer request. */
} CLD_USB_Transfer_Request_Return_Type;

/**
 * Value returned by the User code to notify the USB library if the received data
 * was valid.
 */
typedef enum
{
    CLD_USB_DATA_GOOD = 0,                              /*!< Received data is good.
                                                             For Control OUT requests this
                                                             allows the Status Stage to
                                                             complete. */
    CLD_USB_DATA_BAD_STALL,                             /*!< Received data is bad so stall the endpoint
                                                             For Control OUT requests this
                                                             stalls the Status Stage, for
                                                             other endpoint types the
                                                             next OUT packet will be stalled. */
} CLD_USB_Data_Received_Return_Type;

/**
 * Value returned by the USB library to notify the User code if the requested data
 * will be transmitted.
 */
typedef enum
{
    CLD_USB_TRANSMIT_SUCCESSFUL = 0,                    /*!< The requested USB transfer is being processed. */
    CLD_USB_TRANSMIT_FAILED,                            /*!< The requested USB transfer failed. */
} CLD_USB_Data_Transmit_Return_Type;

/**
 * USB transfer request parameters used to tell the USB library how to handle the
 * current transfer.
 */
typedef struct
{
    unsigned long num_bytes;                            /*!< The number of bytes to
                                                             transmit or receive depending
                                                             on the transfer direction (IN/OUT) */
    unsigned char * p_data_buffer;                      /*!< Pointer to the data to transmit
                                                             (IN Transfer) or a pointer to
                                                             the buffer to store the received
                                                             data (OUT transfer). */

    /* Function pointers used by the USB Library to notify the User code when certain events occur.
       These function pointers can be set to NULL if the User does not want to be notified.*/
    union
    {                                                   /*!< Function called when the data
                                                             has been received. */
        CLD_USB_Data_Received_Return_Type (*fp_usb_out_transfer_complete)(void);
        void (*fp_usb_in_transfer_complete) (void);     /*!< Function called when the requested
                                                             data has been transmitted. */
    }callback;/*!< Function pointers used by the USB Library to notify the User code when certain events occur.
                   These function pointers can be set to NULL if the User does not want to be notified.*/

    void (*fp_transfer_aborted_callback) (void);        /*!< Function called if the requested
                                                             transfer is aborted. */
    CLD_Time transfer_timeout_ms;                       /*!< Transfer Timeout in milliseconds
                                                             (0 - Disables the timeout) */
} CLD_USB_Transfer_Params;

/**
 * Parameters used to configure the CDC Serial Data Bulk endpoints.
 */
typedef struct
{
    unsigned char endpoint_number;                      /*!< USB Endpoint Number */
    unsigned short max_packet_size_full_speed;          /*!< Full-Speed max packet size */
    unsigned short max_packet_size_high_speed;          /*!< High-Speed max packet size */
} CLD_Serial_Data_Bulk_Endpoint_Params;

/**
 * Parameters used to configure the CDC Notifications Interrupt IN endpoint.
 */
typedef struct
{
    unsigned char endpoint_number;
    unsigned short max_packet_size_full_speed;          /*!< Full-Speed max packet size */
    unsigned char polling_interval_full_speed;          /*!< Full-Speed polling interval
                                                           in the USB Endpoint Descriptor.
                                                           (See USB 2.0 section 9.6.6) */
    unsigned short max_packet_size_high_speed;          /*!< High-Speed max packet size */
    unsigned char polling_interval_high_speed;          /*!< High-Speed polling interval
                                                           in the USB Endpoint Descriptor.
                                                           (See USB 2.0 section 9.6.6) */
} CLD_SC58x_CDC_Notification_Endpoint_Params;

/**
 * USB CDC Network Connection Notification state
 */
typedef enum
{
    CLD_SC58x_CDC_NETWORK_DISCONNECTED    = 0,
    CLD_SC58x_CDC_NETWORK_CONNECTED       = 1,
} CLD_SC58x_CDC_Lib_Network_Connection_State;

#pragma pack (1)

/**
 * USB CDC Line Coding structure (refer to: USB CDC PTSM SubClass Rev 1.2 pg 24).
 */
typedef struct
{
    unsigned long data_terminal_rate;                   /*!< CDC Data Terminal Rate in bits per second. */
    unsigned char num_stop_bits;                        /*!< CDC Number of stop bits
                                                            0 = 1 stop bit
                                                            1 = 1.5 stop bits
                                                            2 = 2 stop bits */
    unsigned char parity;                               /*!< CDC Parity setting
                                                            0 = None
                                                            1 = Odd
                                                            2 = Even
                                                            3 = Mark
                                                            4 = Space */
    unsigned char num_data_bits;                        /*!< CDC number of data bits
                                                            (Only 5, 6, 7, 8 and 16 allowed) */
} CLD_SC58x_CDC_Line_Coding;

/* CDC Line Coding #defines */
#define CLD_SC58x_CDC_LINE_CODING_1_STOP_BITS             0 /*<! CDC Line Coding: 1 Stop Bit */
#define CLD_SC58x_CDC_LINE_CODING_1_5_STOP_BITS           1 /*<! CDC Line Coding: 1.5 Stop Bits */
#define CLD_SC58x_CDC_LINE_CODING_2_STOP_BITS             2 /*<! CDC Line Coding: 2 Stop Bits */

#define CLD_SC58x_CDC_LINE_CODING_PARITY_NONE             0 /*<! CDC Line Coding: No Parity */
#define CLD_SC58x_CDC_LINE_CODING_PARITY_ODD              1 /*<! CDC Line Coding: Odd Parity */
#define CLD_SC58x_CDC_LINE_CODING_PARITY_EVEN             2 /*<! CDC Line Coding: Even Parity */
#define CLD_SC58x_CDC_LINE_CODING_PARITY_MARK             3 /*<! CDC Line Coding: Force Mark */
#define CLD_SC58x_CDC_LINE_CODING_PARITY_SPACE            4 /*<! CDC Line Coding: Force Space */

#define CLD_SC58x_CDC_LINE_CODING_NUM_DATA_BITS_5         5 /*<! CDC Line Coding: 5 data Bits */
#define CLD_SC58x_CDC_LINE_CODING_NUM_DATA_BITS_6         6 /*<! CDC Line Coding: 6 data Bits */
#define CLD_SC58x_CDC_LINE_CODING_NUM_DATA_BITS_7         7 /*<! CDC Line Coding: 7 data Bits */
#define CLD_SC58x_CDC_LINE_CODING_NUM_DATA_BITS_8         8 /*<! CDC Line Coding: 8 data Bits */
#define CLD_SC58x_CDC_LINE_CODING_NUM_DATA_BITS_16        16 /*<! CDC Line Coding: 16 data Bits */

/**
 * USB CDC Serial State bitfield (refer to: USB CDC PTSM SubClass Rev 1.2 pg 33).
 */
typedef struct
{
    union
    {
        struct
        {
            unsigned short rx_carrier       : 1;        /*<! State of receiver carrier detection
                                                             mechanism of device. This signal
                                                             corresponds to V.24 signal 109 and
                                                             RS-232 signal DCD */
            unsigned short tx_carrier       : 1;        /*<! State of transmission carrier. This
                                                             signal corresponds to V.24 signal 106
                                                             and RS-232 signal DSR. */
            unsigned short break_detect     : 1;        /*<! State of break detection mechanism of the device.  */
            unsigned short ring_signal      : 1;        /*<! State of ring signal detection of the device. */
            unsigned short framing_error    : 1;        /*<! A framing error has occurred.  */
            unsigned short parity_error     : 1;        /*<! A parity error has occurred.  */
            unsigned short rx_data_overrun  : 1;        /*<! Received data has been discarded due to overrun in the device */
            unsigned short reserved         : 9;
        } bits;
        unsigned short state;
    } u;
} CLD_SC58x_CDC_Serial_State;

/*
 * USB CDC Control Line State bitfield (refer to: USB CDC PTSM SubClass Rev 1.2 pg 25).
 */
typedef struct
{
    union
    {
        struct
        {
            unsigned short dte_present  : 1;            /*<! Indicates to DCE if DTE is present or not.
                                                             This signal corresponds to V.24 signal 108/2
                                                             and RS-232 signal DTR.
                                                                0 - Not Present
                                                                1 - Present  */
            unsigned short activate_carrier : 1;        /*<! Carrier control for half duplex modems.
                                                             This signal corresponds to V.24 signal
                                                             105 and RS-232 signal RTS.
                                                                0 - Deactivate carrier
                                                                1 - Activate carrier
                                                             The device ignores the value of this
                                                             bit when operating in full duplex mode.  */
            unsigned short reserved         : 14;
        } bits;
        unsigned short state;
    } u;
} CLD_SC58x_CDC_Control_Line_State;

#pragma pack ()

/**
 * CLD USB CDC Library initialization parameters
 */
typedef struct
{
    CLD_SC58x_USB_Config usb_config;                    /*!< Sets the USB Port configuration
                                                             - CLD_USB0_CDC  = CDC on USB0
                                                             - CLD_USB1_CDC  = CDC on USB1 */
    CLD_Boolean enable_dma;                             /*!< Used to enable/disable USB DMA support
                                                             - CLD_TRUE = DMA enabled for USB transfers
                                                                         larger than 32 bytes that are
                                                                         aligned on a 4-byte boundary.
                                                             - CLD_FALSE = DMA disabled. */

    unsigned short vendor_id;                           /*!< USB Vendor ID */
    unsigned short product_id;                          /*!< USB Product ID */

    unsigned char usb_bus_max_power;                    /*!< USB Configuration Descriptor
                                                             bMaxPower value (0 = self powered).
                                                             See USB 2.0 Section 9.6.3. */
    unsigned short device_descriptor_bcdDevice;         /*!< USB Device Descriptor bcdDevice
                                                         value. See USB 2.0 section 9.6.1. */

    /**
     * Pointer to the endpoint parameters for the serial data RX Bulk OUT endpoint.
     */
    CLD_Serial_Data_Bulk_Endpoint_Params * p_serial_data_rx_endpoint_params;

    /**
     * Pointer to the endpoint parameters for the serial data TX Bulk IN endpoint.
     */
    CLD_Serial_Data_Bulk_Endpoint_Params * p_serial_data_tx_endpoint_params;

    /**
     * Pointer to the endpoint parameters for the CDC notification Interrupt IN endpoint.
     */
    CLD_SC58x_CDC_Notification_Endpoint_Params * p_notification_endpoint_params;

    /**
     * Function is called when the data is received on the Bulk OUT endpoint.
     * If this function returns an error it causes the next Bulk OUT packet to be stalled.
     * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure used to define how to process the received serial data.
     * @return Notifies the CLD Library if the received data should be accepted, stalled, paused, or discarded
     */
    CLD_USB_Transfer_Request_Return_Type (*fp_serial_data_received) (CLD_USB_Transfer_Params * p_transfer_data);

    /**
     * Function called when an CDC Send Encapsulated Command request is received. The User code
     * should set p_transfer_data->p_data_buffer to address where to store the
     * encapsulated command data.
     * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure used to define where the encapsulated data from the USB Host is stored.
     * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded
      */
    CLD_USB_Transfer_Request_Return_Type (*fp_cdc_cmd_send_encapsulated_cmd) (CLD_USB_Transfer_Params * p_transfer_data);

    /**
     * Function called when an CDC Get Encapsulated Command request is received.
     * The User code should set p_transfer_data->p_data_buffer and
     * p_transfer_data->num_bytes to describe the data to return to the Host.
     * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure used to define the encapsulated data to send to the USB Host.
     * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded
     */
    CLD_USB_Transfer_Request_Return_Type (*fp_cdc_cmd_get_encapsulated_resp) (CLD_USB_Transfer_Params * p_transfer_data);

   /**
    * Function called when an CDC Set Line Coding Command is received. The User code
    * should configure its UART parameters based on the CLD_SC58x_CDC_Line_Coding
    * structure passed to the function.
    *
    * <b>Set to CLD_NULL if not supported.</b>
    * @param p_line_coding - Pointer to the CLD_SC58x_CDC_Line_Coding structure with the line coding defined by the USB Host.
    * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded
    */
    CLD_USB_Data_Received_Return_Type (*fp_cdc_cmd_set_line_coding) (CLD_SC58x_CDC_Line_Coding * p_line_coding);

   /**
    * Function called when an CDC Get Line Coding Command is received. The User code
    * should load the CLD_SC58x_CDC_Line_Coding structure passed to the function with
    * the current line coding values.
    *
    * <b>Set to CLD_NULL if not supported.</b>
    * @param p_line_coding - Pointer to the CLD_SC58x_CDC_Line_Coding structure with the line coding values to send to the USB Host.
    * @return CLD_SUCCESS- line coding values updated, CLD_FAIL - Line coding values were not updated.
    */
    CLD_RV (*fp_cdc_cmd_get_line_coding) (CLD_SC58x_CDC_Line_Coding * p_line_coding);

   /**
    * Function called when an CDC Set Control Line State Command is received.
    * The User code should process the CLD_SC58x_CDC_Control_Line_State structure
    * passed to the function.
    *
    * <b>Set to CLD_NULL if not supported.</b>
    * @param p_control_line_state - Pointer to the CLD_SC58x_CDC_Control_Line_State structure with the control line state defined by the USB Host.
    * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded
    */
    CLD_USB_Data_Received_Return_Type (*fp_cdc_cmd_set_control_line_state) (CLD_SC58x_CDC_Control_Line_State * p_control_line_state);

   /**
    * Function called when an CDC Send Break Command is received.
    * The User code should generate a RS-232 style break for the number of milliseconds
    * passed to the function.
    * If the duration is set to 0xFFFF the device should send a break until this
    * function is called again with a duration of 0.
    *
    * <b>Set to CLD_NULL if not supported.</b>
    * @param duration - the requested break duration in milliseconds.
    * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded
    */
    CLD_USB_Data_Received_Return_Type (*fp_cdc_cmd_send_break) (unsigned short duration);

    unsigned char support_cdc_network_connection;               /*!< 1 = Network Connection Notification supported
                                                                     0 = Network Connection Notification not supported */
    unsigned short cdc_class_bcd_version;                       /*!< CDC Class version in BCD */
    unsigned char  cdc_class_control_protocol_code;             /*!< CDC Protocol Code defined in USB CDC Rev 1.2 (Table 5 Pg 13). */


    const char * p_usb_string_manufacturer;         /*!< Pointer to the Manufacturer string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_product;              /*!< Pointer to the Product string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_serial_number;        /*!< Pointer to the Serial Number string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_configuration;        /*!< Pointer to the Configuration string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_communication_class_interface;    /*!< Pointer to the CDC Control interface string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_data_class_interface;             /*!< Pointer to the CDC Data interface string descriptor (set to CLD_NULL if not used) */

    unsigned short usb_string_language_id;              /*!< USB String Descriptor Language ID code
                                                           (0x0409 = English US). */

    /**
     * Function called when one of the defined USB events occurs.
     * @param event - Defined USB Event
     *                              - CLD_USB_CABLE_CONNECTED
     *                              - CLD_USB_CABLE_DISCONNECTED
     *                              - CLD_USB_ENUMERATED_CONFIGURED
     *                              - CLD_USB_UN_CONFIGURED
     *                              - CLD_USB_BUS_RESET
     */
    void (*fp_cld_usb_event_callback) (CLD_USB_Event event);

     /**
         * Function called when the library has a status to report.
          * @param status_code - Status code returned by the CLD library using the fp_cld_lib_status callback function.
          * @param p_additional_data - optional additional data returned by the CLD library using the fp_cld_lib_status callback function.
          * @param additional_data_size - optional additional data size returned by the CLD library using the fp_cld_lib_status callback function.
         */
    void (*fp_cld_lib_status) (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size);
} CLD_SC58x_CDC_Lib_Init_Params;

/* CLD Audio 2.0 w/CDC Library function that should be called once per millisecond. */
extern void cld_time_125us_tick (void);
/* CLD Audio 2.0 w/CDC Library USB interrupt service routine processing function */
extern void cld_usb0_isr_callback (void);
extern void cld_usb1_isr_callback (void);

/**
 * Function called to get the current time reference in milliseconds.
 * @return the current time reference.
 */
extern CLD_Time cld_time_get(void);
/**
 * Function called to get the number of milliseconds that have elapsed since the specified time
 * reference was set using cld_time_get.
 * @param time - time reference used to calculate the elapsed time in milliseconds
 * @return the elapsed time.
 */
extern CLD_Time cld_time_passed_ms(CLD_Time time);

/**
 * Function called to get the current time reference in 125 microsecond increments.
 * @return the current time reference.
 */
extern CLD_Time cld_time_get_125us (void);
/**
 * Function called to get the number of 125 microsecond increments that have elapsed since the
 * specified time reference was set using cld_time_get_125us.
 * @param time - time reference used to calculate the elapsed time in 125 microsecond increments
 * @return the elapsed time.
 */
extern CLD_Time cld_time_passed_125us (CLD_Time time);


/**
 * Initializes the CLD CDC library. This function needs to be called until
 * CLD_SUCCESS or CLD_FAIL is returned.  If CLD_FAIL is returned there was a
 * problem initializing the library.
 * @param cld_sc58x_cdc_lib_params - Pointer to the CLD library initialization parameters,
 * @return The status of the library initialization (CLD_SUCCESS, CLD_FAIL, CLD_ONGOING).
 */
extern CLD_RV cld_sc58x_cdc_lib_init (CLD_SC58x_CDC_Lib_Init_Params * cld_sc58x_cdc_lib_params);

/**
 * CLD CDC library mainline function. <b>The CLD library mainline function is
 * required and should be called in each iteration of the main program loop.</b>
 */
extern void cld_sc58x_cdc_lib_main (void);


/**
 * Function used to transmit serial data using the Bulk IN endpoint.
 * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure describing the serial data being transmitted.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_cdc_lib_transmit_serial_data (CLD_USB_Transfer_Params * p_transfer_data);

/**
 * Function used to transmit the Network Connection notification using the CDC Interrupt IN endpoint.
 * @param CLD_SC58x_CDC_Lib_Network_Connection_State - The CLD_SC58x_CDC_Lib_Network_Connection_State to send to the USB Host.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_cdc_lib_send_network_connection_state (CLD_SC58x_CDC_Lib_Network_Connection_State state);

/**
 * Function used to transmit the Response Available notification using the CDC Interrupt IN endpoint.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_cdc_lib_send_response_available (void);
/* Function used to transmit the Serial State notification using the CDC
   Interrupt IN endpoint. */
/**
 * Function used to transmit the Serial State notification using the CDC Interrupt IN endpoint.
 * @param p_serial_state - Pointer to the CLD_SC58x_CDC_Serial_State structure describing the serial state being transmitted.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_cdc_lib_send_serial_state (CLD_SC58x_CDC_Serial_State * p_serial_state);

/**
 * Function used to resume a paused Bulk OUT transfer. When this function is called
 * the fp_serial_data_received function will be called with the CLD_USB_Transfer_Params
 * of the paused transfer.  The fp_serial_data_received function can then
 * accept, discard or stall the previously paused transfer.
 */
extern void cld_sc58x_cdc_lib_resume_paused_serial_data_transfer (void);


/**
 * Function used to resume a paused control transfer. When this function is called
 * the fp_cdc_cmd_send_encapsulated_cmd or fp_cdc_cmd_get_encapsulated_cmd function
 * will be called depending on which request was paused.  The function can then
 * accept, discard or stall the previously paused transfer.
 */
extern void cld_sc58x_cdc_lib_resume_paused_control_transfer (void);

/**
 * Connects the specified USB Port to the USB Host.
 * @param usb_port - USB Port to connect.
 */
extern void cld_lib_usb_connect (void);
/**
 * Disconnects the specified from the USB Host.
 * @param usb_port - USB Port to disconnect.
 */
extern void cld_lib_usb_disconnect (void);

/**
 * cld_lib_status_decode will return a null terminated string describing the CLD library status
 * code passed to the function.
 * @param status_code - Status code returned by the CLD library using the fp_cld_lib_status callback function.
 * @param p_additional_data - optional additional data returned by the CLD library using the fp_cld_lib_status callback function.
 * @param additional_data_size - optional additional data size returned by the CLD library using the fp_cld_lib_status callback function.
 * @return Decoded null terminated status string.
 */
extern char * cld_lib_status_decode (unsigned short status_cod, void * p_additional_data, unsigned short additional_data_size);

#ifdef __cplusplus
}
#endif

#endif /* _LANGUAGE_C */
#endif  /* __CLD_CDC_LIB__ */
