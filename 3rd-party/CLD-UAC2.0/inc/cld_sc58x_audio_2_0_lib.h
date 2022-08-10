#ifndef __CLD_AUDIO_2_0_LIB__
#define __CLD_AUDIO_2_0_LIB__
/*=============================================================================
    FILE:           cld_sc58x_audio_2_0_lib.h

    DESCRIPTION:    CLD Audio v2.0 peripheral Library.

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
 * @file      cld_sc58x_audio_2_0_lib.h
 * @brief     CLD USB Audio 2.0 library header file.
 *
 * @details
 *            This file contains the functionality needed to interface with the
 *            CLD USB Audio 2.0 library using the SC58x's Cortex-A5 processor.
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
    CLD_USB0_AUDIO = 0, /*!< Audio 2.0 interface connected to USB0 */
    CLD_USB1_AUDIO,     /*!< Audio 2.0 interface connected to USB1 */
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

typedef struct
{
    float desired_data_rate;                            /*!< Feeback value in kHz (for example use 44.1 for 44.1kHz). */
    void (*fp_usb_in_transfer_complete) (void);         /*!< Function called when the requested
                                                             data has been transmitted. */
    void (*fp_transfer_aborted_callback) (void);        /*!< Function called if the requested
                                                             transfer is aborted. */
    CLD_Time transfer_timeout_ms;                       /*!< Transfer Timeout in milliseconds
                                                             (0 - Disables the timeout) */
} CLD_USB_Audio_Feedback_Params;
#pragma pack (1)

/**
 * USB Audio v2.0 Audio Stream Isochronous Data Endpoint Descriptor
 * (refer to: USB Device Class Definition of Audio Devices v 2.0 section 4.10.1.2).
 */
typedef struct
{
    unsigned char  b_length;                            /*!< Descriptor length */
    unsigned char  b_descriptor_type;                   /*!< Descriptor type = 0x25 (Class Specific Endpoint) */
    unsigned char  b_descriptor_subtype;                /*!< Descriptor subtype = 0x01 (Endpoint General) */
    unsigned char  bm_attributes;                       /*!< Identifies the attributes supported by this endpoint
                                                             Bit 7: Max Packets Only */
    unsigned char bm_controls;                          /*!< - Bits 0-1: Pitch Control
                                                             - Bits 2-3: Data Overrun Control
                                                             - Bits 4-5: Data Underrun Control */
    unsigned char  b_lock_delay_units;                  /*!< Specifies the units used for w_lock_delay
                                                             - 0 = undefined
                                                             - 1 = Milliseconds
                                                             - 2 = Decoded PCM samples */
    unsigned short w_lock_delay;                        /*!< Specifies the time it takes the endpoint
                                                             to reliably lock its internal clock recovery */
} CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor;

#pragma pack ()
/**
 * Parameters used to configure the USB Audio Stream Isochronous Interface.
 */
typedef struct
{
    /* Parameters used for the Isochronous Endpoint Descriptor */
    unsigned char endpoint_number;                      /*!< Isochronous endpoint number */
    unsigned short max_packet_size_full_speed;          /*!< Isochronous endpoint full-speed max packet size */
    unsigned short max_packet_size_high_speed;          /*!< Isochronous endpoint high-speed max packet size */
    unsigned char b_interval_full_speed;                /*!< Isochronous endpoint full-speed bInterval */
    unsigned char b_interval_high_speed;                /*!< Isochronous endpoint high-speed bInterval */

    /* Parameters used for Audio Stream Interface Descriptor
      (refer to: USB Device Class Definition of Audio Devices v 2.0 section 4.9.2) */
    unsigned char b_terminal_link;                      /*!< The Terminal ID connected to the Isochronous endpoint */
    unsigned char b_format_type;                        /*!< Format Type of the Audio Streaming interface */
    unsigned long bm_formats;                           /*!< Supported Audio data format(s) */
    unsigned char b_nr_channels;                        /*!< Number of physical channels in the audio channel cluster */
    unsigned long bm_channel_config;                    /*!< Describes the spatial location of
                                                           the physical channel(s) */
    unsigned char i_channel_config;                     /*!< Index of the string descriptor describing the
                                                           first physical channel.  These strings
                                                           should be defined in the p_user_string_descriptor_table. */

    unsigned char * p_encoder_descriptor;               /*!< Optional encoder descriptor pointer (set to CLD_NULL of not used)
                                                             (refer to: USB Device Class Definition of Audio Devices v 2.0 sections 4.9.4) */
    unsigned char * p_decoder_descriptor;               /*!< Optional decoder descriptor pointer (set to CLD_NULL of not used)
                                                             (refer to: USB Device Class Definition of Audio Devices v 2.0 sections 4.9.5) */

    unsigned char * p_format_descriptor;                /*!< Pointer to the Interface's format descriptor.
                                                             (refer to: USB Device Class Definition for Audio Data Formats, part of the Audio Devices v 2.0 specification) */
    CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor * p_audio_stream_endpoint_data_descriptor; /*!< Pointer to the Audio Stream endpoint data descriptor */
} CLD_SC58x_Audio_2_0_Stream_Interface_Params;

/**
 * Parameters used to configure the optional Audio control Interrupt IN endpoint
 */
typedef struct
{
    unsigned char endpoint_number;                      /*!< Interrupt endpoint number */
    unsigned char b_interval_full_speed;                /*!< Interrupt endpoint full-speed bInterval */
    unsigned char b_interval_high_speed;                /*!< Interrupt endpoint high-speed bInterval */
} CLD_SC58x_Audio_2_0_Control_Interrupt_Params;

/**
 * Parameters used to configure the optional USB Audio OUT stream's rate feedback Isochonous IN endpoint
 */
typedef struct
{
    /* Parameters used for the Isochronous Rate Feedback Endpoint Descriptor */
    unsigned short max_packet_size_full_speed;          /* Isochronous endpoint full-speed max packet size */
    unsigned short max_packet_size_high_speed;          /* Isochronous endpoint high-speed max packet size */
    unsigned char b_interval_full_speed;                /* Isochronous endpoint full-speed bInterval */
    unsigned char b_interval_high_speed;                /* Isochronous endpoint high-speed bInterval */
} CLD_SC58x_Audio_2_0_Rate_Feedback_Params;

/**
 * USB Audio v2.0 Set and Get Control Endpoint bRequest values
 */
typedef enum
{
    CLD_REQ_CURRENT     = 0x01,                         /*!< Used to get/set the current value of an audio Unit */
    CLD_REQ_RANGE       = 0x02,                         /*!< Used to get the support range of an audio Unit */
    CLD_REQ_MEMORY      = 0x03,                         /*!< Used for Memory access requests */
} CLD_SC58x_Audio_2_0_Requests;

/**
 * Parameters passed to the fp_audio_set_req_cmd and fp_audio_get_req_cmd functions used to
 * identify the active command.
 */
typedef struct
{
    CLD_SC58x_Audio_2_0_Requests    req;                    /*!< bRequest value of the active command */
    CLD_Boolean                     recipient_is_interface; /*!< CLD_TRUE - The request was sent to the interface specified in interface_or_endpoint_num
                                                                 CLD_FALSE - The request was sent to the streaming endpoint specified in interface_or_endpoint_num */
    unsigned char                   entity_id;              /*!< Target entity ID (Unit ID, Terminal ID, etc) */
    unsigned char                   interface_or_endpoint_num;  /*!< Target interface or endpoint number depending on
                                                                     the entity. */
    unsigned short                  setup_packet_wValue;    /*!< Setup Packet wValue */
} CLD_SC58x_Audio_2_0_Cmd_Req_Parameters;

/**
 * Table Entry used to specify user defined USB strings. For example these strings could be used along with
   the Unit and Terminal descriptor to define a descriptive string for an entity.
  */
typedef struct
{
    unsigned char string_index;                         /*!< String Descriptor index. */
    unsigned char * p_string;                           /*!< Null terminated string */
} CLD_SC58x_Audio_2_0_Lib_User_String_Descriptors;


/**
 * CLD USB Audio v2.0 Library initialization parameters
 */
typedef struct
{
    CLD_SC58x_USB_Config usb_config;                    /*!< Sets the USB Port configuration
                                                             - CLD_USB0_AUDIO  = Audio 2.0 on USB0
                                                             - CLD_USB1_AUDIO  = Audio 2.0 on USB1 */
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


    unsigned char audio_control_category_code;          /*!< Audio Control Interface Header Descriptor bCategory code
                                                           (refer to: USB Device Class Definition of Audio Devices v 2.0 section 4.7.2)*/

    CLD_SC58x_Audio_2_0_Control_Interrupt_Params * p_audio_control_interrupt_params; /*!< Pointer to the optional Control Interface Interrupt IN endpoint.
                                                                                          Set to CLD_NULL if not required.*/

    unsigned char * p_unit_and_terminal_descriptors;    /*!< Pointer to the beginning of the Unit
                                                             and Terminal Descriptors (these descriptors need to be packed). */
    unsigned short  unit_and_terminal_descriptors_length; /*!< The total length of the above Unit and Terminal Descriptors. */

    CLD_SC58x_Audio_2_0_Stream_Interface_Params * p_audio_streaming_rx_interface_params; /*!< Pointer to the interface parameters
                                                                                              for the Audio Stream RX Isochronous OUT interface.
                                                                                              <b>Set to CLD_NULL if not required.</b>*/

    /* Pointer to the Optional Audio Stream RX Isochronous OUT's rate Feedback Isochonous IN interface.
       Set to CLD_NULL if not required.*/
    CLD_SC58x_Audio_2_0_Rate_Feedback_Params * p_audio_rate_feedback_rx_params;

    CLD_SC58x_Audio_2_0_Stream_Interface_Params * p_audio_streaming_tx_interface_params; /*!< Pointer to the interface parameters
                                                                                              for the Audio Stream TX Isochronous IN interface.
                                                                                              <b>Set to CLD_NULL if not required.</b>*/

    /**
     * Audio Data received function pointer.
     * This function is called when the data is received on the Isochronous OUT endpoint.
     * If this function returns an error it causes the next Isochronous OUT packet to be stalled.
     * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure describing the received audio data.
     * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded */
    CLD_USB_Transfer_Request_Return_Type (*fp_audio_stream_data_received) (CLD_USB_Transfer_Params * p_transfer_data);

    /**
     * Set Command received function pointer.
     * Function called when an USB Audio 2.0 Set Request is received. The User code should set
     * p_transfer_data->p_data_buffer to address where to store the data for the command
     * described by p_req_params.
     * @param p_req_param - Pointer to the CLD_SC58x_Audio_2_0_Cmd_Req_Parameters for the current Control Endpoint Set request
     * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure used to define where the command's data should be stored.
     * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded */
    CLD_USB_Transfer_Request_Return_Type (*fp_audio_set_req_cmd) (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data);

    /**
     * Get Command received function pointer.
     * Function called when an USB Audio 2.0 Get Request is received. The User code should
     * set p_transfer_data->p_data_buffer and p_transfer_data->num_bytes to describe the
     * data to return to the Host.
     * @param p_req_param - Pointer to the CLD_SC58x_Audio_2_0_Cmd_Req_Parameters for the current Control Endpoint Get request
     * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure used to define the data to send to the USB Host.
     * @return Notifies the CLD Library if the received command should be accepted, stalled, paused, or discarded */
    CLD_USB_Transfer_Request_Return_Type (*fp_audio_get_req_cmd) (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data);

    /**
     * Function called when the Isochronous OUT endpoint interface is enabled/disabled by
     * the USB Host using the Set Interface Alternate Setting parameter.
     * @param enabled - CLD_TRUE = Enabled, CLD_FALSE = Disabled
     */
    void (*fp_audio_streaming_rx_endpoint_enabled) (CLD_Boolean enabled);

    /**
     * Function called when the Isochronous IN endpoint interface is enabled/disabled by
     * the USB Host using the Set Interface Alternate Setting parameter.
     * @param enabled - CLD_TRUE = Enabled, CLD_FALSE = Disabled
     */
    void (*fp_audio_streaming_tx_endpoint_enabled) (CLD_Boolean enabled);

    const char * p_usb_string_manufacturer;                     /*!< Pointer to the Manufacturer string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_product;                          /*!< Pointer to the Product string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_serial_number;                    /*!< Pointer to the Serial Number string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_configuration;                    /*!< Pointer to the Configuration string descriptor (set to CLD_NULL if not used) */

    const char * p_usb_string_audio_control_interface;          /*!< Pointer to the Audio Control Interface string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_audio_streaming_out_interface;    /*!< Pointer to the Audio Streaming OUT interface string descriptor (set to CLD_NULL if not used) */
    const char * p_usb_string_audio_streaming_in_interface;     /*!< Pointer to the Audio Streaming IN interface string descriptor (set to CLD_NULL if not used) */

    unsigned char user_string_descriptor_table_num_entries; /*!< Number of entries in the
                                                                 p_user_string_descriptor_table array
                                                                 (Set to 0 if not used) */
  /**
   * Array of USB specified String Descriptors.(Set to CLD_NULL if not used.)
   */
    CLD_SC58x_Audio_2_0_Lib_User_String_Descriptors * p_user_string_descriptor_table;

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

} CLD_SC58x_Audio_2_0_Lib_Init_Params;

/* CLD Audio 2.0 Library function that should be called once every 125 microseconds. */
extern void cld_time_125us_tick (void);
/* CLD Audio 2.0 Library USB interrupt service routine processing function */
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
 * Initializes the CLD Audio v2.0 library. This function needs to be called until
 * CLD_SUCCESS or CLD_FAIL is returned.  If CLD_FAIL is returned there was a
 * problem initializing the library.
 * @param cld_sc58x_audio_2_0_lib_params - Pointer to the CLD library initialization parameters,
 * @return The status of the library initialization (CLD_SUCCESS, CLD_FAIL, CLD_ONGOING).
 */
extern CLD_RV cld_sc58x_audio_2_0_lib_init (CLD_SC58x_Audio_2_0_Lib_Init_Params * cld_sc58x_audio_2_0_lib_params);

/**
 * CLD Audio v2.0 library mainline function. <b>The CLD library mainline function is
 * required and should be called in each iteration of the main program loop.</b>
 */
extern void cld_sc58x_audio_2_0_lib_main (void);

/**
 * Function used to transmit audio data using the Isochronous IN endpoint.
 * If the Isochronous IN endpoint is disabled this function will return
 * CLD_USB_TRANSMIT_FAILED.
 * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure describing the audio data being transmitted.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_audio_2_0_lib_transmit_audio_data (CLD_USB_Transfer_Params * p_transfer_data);

/**
 * Function used to transmit interrupt data using the optional Audio Control Interrupt IN endpoint.
 * @param p_transfer_data - Pointer to the CLD_USB_Transfer_Params structure describing the interrupt data being transmitted.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_audio_2_0_lib_transmit_interrupt_data (CLD_USB_Transfer_Params * p_transfer_data);

/**
 * Function used to transmit audio rate feedback using the optional rate feedback Isochronous IN endpoint.
 * @param p_transfer_data - Pointer to the CLD_USB_Audio_Feedback_Params structure describing the feedback data being transmitted.
 * @return If the requested transmission was successfully initiated or if it failed..
 */
extern CLD_USB_Data_Transmit_Return_Type cld_sc58x_audio_2_0_lib_transmit_audio_rate_feedback_data (CLD_USB_Audio_Feedback_Params * p_transfer_data);

/**
 * Function used to resume a paused Isochronous OUT transfer. When this function is called
 * the fp_audio_stream_data_received function will be called with the CLD_USB_Transfer_Params
 * of the paused transfer.  The fp_audio_stream_data_received function can then
 * accept, discard or stall the previously paused transfer.
 */
extern void cld_sc58x_audio_2_0_lib_resume_paused_audio_data_transfer (void);

/**
 * Function used to resume a paused control transfer. When this function is called
 * the fp_audio_set_req_cmd or fp_audio_get_req_cmd function
 * will be called depending on which request was paused.  The function can then
 * accept, discard or stall the previously paused transfer.
 */
extern void cld_sc58x_audio_2_0_lib_resume_paused_control_transfer (void);

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
#endif  /* __CLD_AUDIO_2_0_LIB__ */
