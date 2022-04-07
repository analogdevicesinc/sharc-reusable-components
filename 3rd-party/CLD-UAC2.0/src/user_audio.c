/*==============================================================================
    FILE:           user_audio.c

    DESCRIPTION:    Uses the cld_sc58x_audio_2_0_lib library to implement a basic
                    USB audio 2.0 device.

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
 * @file      user_audio_w_cdc.c
 * @brief     Uses the cld_sc58x_audio_2_0_lib library to implement a basic
 *            USB Audio 2.0 device.
 *
 * @details
 *            User defined interface with the cld_sc58x_audio_2_0_lib library to
 *            implement a custom USB Audio 2.0 device.
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
#include <string.h>
#include <stdint.h>
#include <services/int/adi_int.h>
#include <services/tmr/adi_tmr.h>

#include "user_audio.h"
#include "cld_sc58x_audio_2_0_lib.h"

#define AUDIO_IN_ENABLED        1
#define NUM_AUDIO_CHANNELS      2

#define USER_AUDIO_NUM_CHANNELS   NUM_AUDIO_CHANNELS

/* ARM processor interrupt request IDs. */
#define ADI_INTR_BASE                         32         /* ARM A5 GIC offset needed for interrupt indexing for ADSP-SC58x */

#define USB0_STAT_IRQn          (ADI_INTR_BASE + 132)    /*!< Status/FIFO Data Ready */
#define USB0_DATA_IRQn          (ADI_INTR_BASE + 133)    /*!< DMA Status/Transfer Complete */
#define USB1_STAT_IRQn          (ADI_INTR_BASE + 134)    /*!< Status/FIFO Data Ready */
#define USB1_DATA_IRQn          (ADI_INTR_BASE + 135)    /*!< DMA Status/Transfer Complete */
#define UART0_TXDMA_IRQn        (ADI_INTR_BASE + 114)    /*!< Transmit DMA */
#define UART0_RXDMA_IRQn        (ADI_INTR_BASE + 115)    /*!< Receive DMA */
#define UART1_TXDMA_IRQn        (ADI_INTR_BASE + 117)    /*!< Transmit DMA */
#define UART1_RXDMA_IRQn        (ADI_INTR_BASE + 118)    /*!< Receive DMA */
#define UART2_TXDMA_IRQn        (ADI_INTR_BASE + 120)    /*!< Transmit DMA */
#define UART2_RXDMA_IRQn        (ADI_INTR_BASE + 121)    /*!< Receive DMA */

#define USER_PRODUCT_ID_AUDIO           0x0005  /*!< USB Product ID used when the Audio 2.0 uses it's own USB port. */

/**
  * Feature Unit ID used to control the Headphone volume
  */
#define USER_AUDIO_HEADPHONE_FEATURE_UNIT_ID                    0x09

/**
 * Control and Channel number from the Setup Packet wValue field of
 * the Headphone Master channel
 */
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_MASTER     0x0200
/**
 * Control and Channel number from the Setup Packet wValue field of
 * the Headphone left channel
 */
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_LEFT       0x0201
/**
 *Control and Channel number from the Setup Packet wValue field of
 * the Headphone right channel
 */
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_RIGHT      0x0202

#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_CENTER      0x0203
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_LFE      0x0204
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_BACK_L      0x0205
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_BACK_R      0x0206
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_FLC      0x0207
#define USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_FRC      0x0208

/**
 * Feature Unit ID used to control the Microphone volume
 */
#define USER_AUDIO_MICROPHONE_FEATURE_UNIT_ID                   0x0a
/**
 *Control and Channel number from the Setup Packet wValue field of
 * the Microphone volume
 */
#define USER_AUDIO_MICROPHONE_CONTROL_AND_CHANNEL_NUM_MASTER    0x0200

/**
 * Control and Channel number from the Setup Packet wValue field of
 * the Microphone volume
 */
#define USER_AUDIO_MUTE_CONTROL_MASTER_CHANNEL                  0x0100

/**
 * Clock Source ID connected to the headphones and microphone Units
 */
#define USER_AUDIO_CLOCK_SOURCE_ID_DAC                              0x03
#define USER_AUDIO_CLOCK_SOURCE_ID_ADC                              0x04

/**
 * High Speed max packet size should be large enough to hold 1 bInterval worth
 * of audio samples.
 */
#if (USER_AUDIO_W_CDC_NUM_CHANNELS > 16)
#define USER_AUDIO_MAX_PACKET_SIZE                              1024
#else
#define USER_AUDIO_MAX_PACKET_SIZE                              512
#endif

#pragma pack (1)

/*
   USB Audio v2.0 Unit and Terminal descriptors that describe a simple
   audio device comprised of the following:

    Input Terminal - USB Streaming Endpoint
        ID = 0x01
        Channels: Left, Right
    Input Terminal - Microphone
        ID = 0x02
        Channels: Left, Right
    Output Terminal - Speaker
        ID = 0x06
        Source ID = 0x09
    Output Terminal - USB Streaming Endpoint
        ID = 0x07
        Source ID = 0x0a
    Feature Unit
        ID = 0x09
        Source ID = 0x01
        Controls:
            Master Channel 0: Mute (Control 1)
            Channel 1 (Left): Volume (Control 2)
            Channel 2 (Right): Volume (Control 2)
    Feature Unit
        ID = 0x0a
        Source ID = 0x02
        Controls:
            Master Channel 0: Volume (Control 2)
 */
static const unsigned char user_audio_unit_and_terminal_descriptor[] =  /*!< USB Audio v2.0 Unit and Terminal descriptors that describe a simple audio device. */
{
    /* Input Terminal Descriptor - USB Endpoint */
    0x11,                   /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x02,                   /* bDescriptorSubType = Input Terminal */
    0x01,                   /* bTerminalID */
    0x01, 0x01,             /* wTerminalType = USB Streaming */
    0x00,                   /* bAssocTerminal */
    0x03,                   /* bCSourceID */
    USER_AUDIO_NUM_CHANNELS,                   /* bNRChannels */

    /* wChannelConfig */
    ((1 << USER_AUDIO_NUM_CHANNELS)-1) & 0xff,
    (((1 << USER_AUDIO_NUM_CHANNELS)-1) >> 8) & 0xff,
    (((1 << USER_AUDIO_NUM_CHANNELS)-1) >> 16) & 0xff,
    (((1 << USER_AUDIO_NUM_CHANNELS)-1) >> 24) & 0xff,

    0x00,                   /* iChannelNames */
    0x00,0x00,              /* bmControls */
    0x00,                   /* iTerminal */
#if (AUDIO_IN_ENABLED == 1)
    /* Input Terminal Descriptor - Microphone */
    0x11,                   /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x02,                   /* bDescriptorSubType = Input Terminal */
    0x02,                   /* bTerminalID */
    0x01, 0x02,             /* wTerminalType = Microphone */
    0x00,                   /* bAssocTerminal */
    0x04,                   /* bCSourceID */
    USER_AUDIO_NUM_CHANNELS,                   /* bNRChannels */

    /* wChannelConfig */
    ((1 << USER_AUDIO_NUM_CHANNELS)-1) & 0xff,
    (((1 << USER_AUDIO_NUM_CHANNELS)-1) >> 8) & 0xff,
    (((1 << USER_AUDIO_NUM_CHANNELS)-1) >> 16) & 0xff,
    (((1 << USER_AUDIO_NUM_CHANNELS)-1) >> 24) & 0xff,

    0x00,                   /* iChannelNames */
    0x00,0x00,              /* bmControls */
    0x00,                   /* iTerminal */
#endif
    /* Output Terminal Descriptor - Speaker */
    0x0c,                   /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x03,                   /* bDescriptorSubType = Output Terminal */
    0x06,                   /* bTerminalID */
    0x01, 0x03,             /* wTerminalType - Speaker */
    0x00,                   /* bAssocTerminal */
    0x09,                   /* bSourceID */
    0x03,                   /* bCSourceID */
    0x00, 0x00,             /* bmControls */
    0x00,                   /* iTerminal */
#if (AUDIO_IN_ENABLED == 1)
    /* Output Terminal Descriptor - USB Endpoint */
    0x0c,                   /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x03,                   /* bDescriptorSubType = Output Terminal */
    0x07,                   /* bTerminalID */
    0x01, 0x01,             /* wTerminalType - USB Streaming */
    0x00,                   /* bAssocTerminal */
    0x0a,                   /* bSourceID */
    0x04,                   /* bCSourceID */
    0x00, 0x00,             /* bmControls */
    0x00,                   /* iTerminal */
#endif
    /* Feature Unit Descriptor */
    //0x2a,                   /* bLength */
    (10 + (4 * USER_AUDIO_NUM_CHANNELS)), /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x06,                   /* bDescriptorSubType = Feature Unit */
    0x09,                   /* bUnitID */
    0x01,                   /* bSourceID */
    0x00, 0x00, 0x00, 0x00, /* bmaControls - Master */
#if (USER_AUDIO_READ_ONLY_VOLUME)
    0x05, 0x00, 0x00, 0x00, /* bmaControls - Front Left */
    0x05, 0x00, 0x00, 0x00, /* bmaControls - Front Right */
#else
    0x0f, 0x00, 0x00, 0x00, /* bmaControls - Front Left */
    0x0f, 0x00, 0x00, 0x00, /* bmaControls - Front Right */
#endif
    0x00,                   /* iFeature */
    /* Feature Unit Descriptor */
#if (AUDIO_IN_ENABLED == 1)
    (10 + (4 * USER_AUDIO_NUM_CHANNELS)), /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x06,                   /* bDescriptorSubType = Feature Unit */
    0x0A,                   /* bUnitID */
    0x02,                   /* bSourceID */
#if (USER_AUDIO_READ_ONLY_VOLUME)
    0x05, 0x00, 0x00, 0x00, /* bmaControls - Master */
#else
    0x0f, 0x00, 0x00, 0x00, /* bmaControls - Master */
#endif
    0x00, 0x00, 0x00, 0x00, /* bmaControls - Front Left */
    0x00, 0x00, 0x00, 0x00, /* bmaControls - Front Right */
    0x00,                   /* iFeature */
#endif
    /* Clock Source Descriptor */
    0x08,                   /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x0a,                   /* bDescriptorSubType = Clock Source */
    0x03,                   /* ClockID */
    0x01,                   /* bmAttributes - Internal Fixed Clock */
    0x01,                   /* bmControls */
    0x00,                   /* bAssocTerminal */
    0x00,                   /* iClockSource */
    /* Clock Source Descriptor */
    0x08,                   /* bLength */
    0x24,                   /* bDescriptorType = Class Specific Interface */
    0x0a,                   /* bDescriptorSubType = Clock Source */
    0x04,                   /* ClockID */
    0x01,                   /* bmAttributes - Internal Fixed Clock */
    0x01,                   /* bmControls */
    0x00,                   /* bAssocTerminal */
    0x00,                   /* iClockSource */

};

static const unsigned char user_audio_in_stream_format_descriptor[] =   /*!<  Isochronous IN endpoint PCM format descriptor */
{
    0x06,               /* bLength */
    0x24,               /* bDescriptorType - Class Specific Interface */
    0x02,               /* bDescriptorSubType - Format Type */
    0x01,               /* bFormatType - Format Type 1 */
    0x04,               /* bSubSlotSize */
    24,                 /* bBitResolution */
};

static const unsigned char user_audio_out_stream_format_descriptor[] =  /*!< Isochronous OUT endpoint PCM format descriptor */
{
    0x06,               /* bLength */
    0x24,               /* bDescriptorType - Class Specific Interface */
    0x02,               /* bDescriptorSubType - Format Type */
    0x01,               /* bFormatType - Format Type 1 */
    0x04,               /* bSubSlotSize */
    24,                 /* bBitResolution */
};


#pragma pack ()

/*!< IN Audio Stream Interface Endpoint Data Descriptor */
static const CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor user_audio_in_stream_endpoint_desc =
{
    .b_length = sizeof(CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor),
    .b_descriptor_type                  = 0x25,             /* Class Specific Endpoint */
    .b_descriptor_subtype               = 0x01,             /* Endpoint - General */
    .bm_attributes                      = 0x00,             /* max packet only set to 0 */
    .bm_controls                        = 0x00,
    .b_lock_delay_units                 = 0x00,             /* Undefined */
    .w_lock_delay                       = 0x00,
};

/*!< OUT Audio Stream Interface Endpoint Data Descriptor */
static const CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor user_audio_out_stream_endpoint_desc =
{
    .b_length = sizeof(CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor),
    .b_descriptor_type                  = 0x25,             /* Class Specific Endpoint */
    .b_descriptor_subtype               = 0x01,             /* Endpoint - General */
    .bm_attributes                      = 0x00,             /* max packet only set to 0 */
    .bm_controls                        = 0x00,
    .b_lock_delay_units                 = 0x02,             /* Milliseconds */
    .w_lock_delay                       = 0x01,             /* 1 Millisecond */
};


/* Function prototypes */
static void user_audio_audio_stream_data_transmitted (void);
static void user_audio_audio_stream_data_transmit_aborted (void);
static CLD_USB_Transfer_Request_Return_Type user_audio_stream_data_received (CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Data_Received_Return_Type user_audio_stream_data_receive_complete (void);
static CLD_USB_Transfer_Request_Return_Type user_audio_set_req_cmd (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Transfer_Request_Return_Type user_audio_get_req_cmd (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Data_Received_Return_Type user_audio_set_volume_req (void);
static void user_audio_streaming_rx_endpoint_enabled (CLD_Boolean enabled);
static void user_audio_streaming_tx_endpoint_enabled (CLD_Boolean enabled);

static void user_audio_usb_event (CLD_USB_Event event);

static void user_audio_tx_audio_data (void);
static void user_audio_tx_feedback_data (void);
static void user_audio_feedback_xfr_done (void);

static void user_audio_usb0_isr(uint32_t Event, void *pArg);
static void user_audio_usb1_isr(uint32_t Event, void *pArg);

static void user_cld_lib_status (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size);

/*!< Audio Stream IN Interface parameters */
static CLD_SC58x_Audio_2_0_Stream_Interface_Params user_audio_in_endpoint_params =
{
    .endpoint_number            = 2,                        /* Isochronous endpoint number */
    .max_packet_size_full_speed = USER_AUDIO_MAX_PACKET_SIZE,/* Isochronous endpoint full-speed max packet size */
    .max_packet_size_high_speed = USER_AUDIO_MAX_PACKET_SIZE,/* Isochronous endpoint high-speed max packet size */
    .b_interval_full_speed      = 1,                        /* Isochronous endpoint full-speed bInterval */
    .b_interval_high_speed      = 4,                        /* Isochronous endpoint high-speed bInterval - 1 millisecond */
    .b_terminal_link            = 7,                        /* Terminal ID of the associated Output Terminal */
    .b_format_type              = 1,                        /* Type 1 Format */
    .bm_formats                 = 0x00000001,               /* Type 1 - PCM format */
    .b_nr_channels              = USER_AUDIO_NUM_CHANNELS,/* Number of Channels */
    .bm_channel_config          = ((1 << USER_AUDIO_NUM_CHANNELS)-1), /* Channel Config */
    .p_encoder_descriptor       = CLD_NULL,
    .p_decoder_descriptor       = CLD_NULL,
    .p_format_descriptor        = (unsigned char*)user_audio_in_stream_format_descriptor,
    .p_audio_stream_endpoint_data_descriptor = (CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor*)&user_audio_in_stream_endpoint_desc,
};

/*!< Audio Stream OUT Interface parameters */
static CLD_SC58x_Audio_2_0_Stream_Interface_Params user_audio_out_endpoint_params =
{
    .endpoint_number            = 3,                        /* Isochronous endpoint number */
    .max_packet_size_full_speed = USER_AUDIO_MAX_PACKET_SIZE,/* Isochronous endpoint full-speed max packet size */
    .max_packet_size_high_speed = USER_AUDIO_MAX_PACKET_SIZE,/* Isochronous endpoint high-speed max packet size */
    .b_interval_full_speed      = 1,                        /* Isochronous endpoint full-speed bInterval */
    .b_interval_high_speed      = 4,                        /* Isochronous endpoint high-speed bInterval - 1 millisecond */
    .b_terminal_link            = 1,                        /* Terminal ID of the associated Output Terminal */
    .b_format_type              = 1,                        /* Type 1 Format */
    .bm_formats                 = 0x00000001,               /* Type 1 - PCM format */
    .b_nr_channels              = USER_AUDIO_NUM_CHANNELS, /* Number of Channels */
    .bm_channel_config          = ((1 << USER_AUDIO_NUM_CHANNELS)-1), /* Channel Config */
    .p_encoder_descriptor       = CLD_NULL,
    .p_decoder_descriptor       = CLD_NULL,
    .p_format_descriptor        = (unsigned char*)user_audio_out_stream_format_descriptor,
    .p_audio_stream_endpoint_data_descriptor = (CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor*)&user_audio_out_stream_endpoint_desc,
};

/*!< Audio Control Interrupt IN endpoint parameters */
static CLD_SC58x_Audio_2_0_Control_Interrupt_Params user_audio_interrupt_in_params =
{
    .endpoint_number            = 1,                        /* Endpoint number */
    .b_interval_full_speed      = 1,                        /* Interrupt IN endpoint full-speed bInterval */
    .b_interval_high_speed      = 4,                        /* Interrupt IN endpoint high-speed bInterval - 1 millisecond */
};


static CLD_SC58x_Audio_2_0_Rate_Feedback_Params user_audio_rate_feedback_params =
{
    /* Parameters used for the Isochronous Rate Feedback Endpoint Descriptor */
    .max_packet_size_full_speed = 32,
    .max_packet_size_high_speed = 32,
    .b_interval_full_speed      = 1,                        /* Isochronous endpoint full-speed bInterval */
    .b_interval_high_speed      = 4,                        /* Isochronous endpoint high-speed bInterval - 1 millisecond */
};


/*!< CLD SC58x Audio 2.0 library initialization data. */
static CLD_SC58x_Audio_2_0_Lib_Init_Params user_audio_init_params =
{
    .usb_config                                         = CLD_USB0_AUDIO,
//    .usb_config                                         = CLD_USB1_AUDIO,
    .enable_dma                                         = CLD_TRUE,
//    .enable_dma                                         = CLD_FALSE,

    .vendor_id                                          = 0x064b,       /* Analog Devices Vendor ID */
    .product_id                                         = USER_PRODUCT_ID_AUDIO,

    .usb_bus_max_power                                  = 0,
    .device_descriptor_bcdDevice                        = 0x0100,

    .audio_control_category_code                        = 0x0a,                /* Pro Audio */
    .p_audio_control_interrupt_params                   = CLD_NULL,//&user_audio_interrupt_in_params,

    .p_unit_and_terminal_descriptors                    = (unsigned char*)user_audio_unit_and_terminal_descriptor,
    .unit_and_terminal_descriptors_length               = sizeof(user_audio_unit_and_terminal_descriptor),

    .p_audio_streaming_rx_interface_params              = &user_audio_out_endpoint_params,
    .p_audio_rate_feedback_rx_params                    = &user_audio_rate_feedback_params,

#if (AUDIO_IN_ENABLED == 1)
    .p_audio_streaming_tx_interface_params              = &user_audio_in_endpoint_params,
#else
    .p_audio_streaming_tx_interface_params              = CLD_NULL,
#endif

    .fp_audio_stream_data_received                      = user_audio_stream_data_received,

    .fp_audio_set_req_cmd                               = user_audio_set_req_cmd,
    .fp_audio_get_req_cmd                               = user_audio_get_req_cmd,

    .fp_audio_streaming_rx_endpoint_enabled             = user_audio_streaming_rx_endpoint_enabled,
    .fp_audio_streaming_tx_endpoint_enabled             = user_audio_streaming_tx_endpoint_enabled,


    .p_usb_string_manufacturer                          = "Analog Devices Inc",
    .p_usb_string_product                               = "SC584 Audio v2.0 Device",
    .p_usb_string_serial_number                         = CLD_NULL,
    .p_usb_string_configuration                         = CLD_NULL,

    .p_usb_string_audio_control_interface               = CLD_NULL,
    .p_usb_string_audio_streaming_out_interface         = "Audio 2.0 Output",
    .p_usb_string_audio_streaming_in_interface          = "Audio 2.0 Input",

    .user_string_descriptor_table_num_entries           = 0,
    .p_user_string_descriptor_table                     = NULL,

    .usb_string_language_id                             = 0x0409,       /* English (US) language ID */

    .fp_cld_usb_event_callback                          = user_audio_usb_event,

    .fp_cld_lib_status                                  = user_cld_lib_status,
};

#pragma pack(1)
#define USB_AUDIO_MAX_NUM_SUB_RANGES     1  /*!< Maximum number of sub-range values the device supports */

/**
 * Structure used to report 32-bit USB Audio 2.0 range values.
 */
typedef struct
{
    unsigned short  w_num_sub_ranges;   /*!< The number of sub ranges being reported */
    struct
    {
        unsigned long   d_min;          /*!< Range Minimum */
        unsigned long   d_max;          /*!< Range Minimum */
        unsigned long   d_res;          /*!< Range Minimum */
    } sub_ranges[USB_AUDIO_MAX_NUM_SUB_RANGES]; /*!< Array of supported sub-ranges */
} USB_Audio_4_byte_Range_Response;

/**
 * Structure used to report 16-bit USB Audio 2.0 range values.
 */
typedef struct
{
    unsigned short  w_num_sub_ranges;   /*!< The number of sub-ranges being reported */
    struct
    {
        unsigned short  d_min;          /*!< Range Minimum */
        unsigned short  d_max;          /*!< Range Minimum */
        unsigned short  d_res;          /*!< Range Minimum */
    } sub_ranges[USB_AUDIO_MAX_NUM_SUB_RANGES]; /*!< Array of supported sub-ranges */
} USB_Audio_2_byte_Range_Response;

static USB_Audio_2_byte_Range_Response user_audio_2_byte_range_resp;

/**
 * Structure used to report 8-bit USB Audio 2.0 range values.
 */
typedef struct
{
    unsigned short  w_num_sub_ranges;   /*!< The number of sub-ranges being reported */
    struct
    {
        unsigned char  d_min;           /*!< Range Minimum */
        unsigned char  d_max;           /*!< Range Minimum */
        unsigned char  d_res;           /*!< Range Minimum */
    } sub_ranges[USB_AUDIO_MAX_NUM_SUB_RANGES]; /*!< Array of supported sub-ranges */
} USB_Audio_1_byte_Range_Response;
#pragma pack()

/**
 * Structure used to store the current USB volume settings.
 */
typedef struct
{
    unsigned short master;              /*!< Master Volume setting */
    unsigned short vol[USER_AUDIO_NUM_CHANNELS];
    unsigned short min;                 /*!< Volume range minimum setting */
    unsigned short max;                 /*!< Volume range maximum setting */
    unsigned short resolution;          /*!< Volume range resolution */
    unsigned char mute;                 /*!< Mute setting */
} User_Audio_Data_Volume;

/**
 * Structure used to store the requested USB volume settings.
 */
typedef struct
{
    unsigned short vol[USER_AUDIO_NUM_CHANNELS];
    unsigned char mute;                 /*!< Mute setting */
} User_Audio_Req_Volume;

/**
 * User Audio data
 */
typedef struct
{
    CLD_Boolean             volume_changed;                 /*!< CLD_TRUE when a Set Volume or
                                                                 Mute command is received */
    User_Audio_Data_Volume  headphone_output_volume;        /*!< Current Volume settings */
    User_Audio_Req_Volume   headphone_req_output_volume;    /*!< Requested Volume settings */
    User_Audio_Data_Volume  mic_input_volume;               /*!< Current Microphone Volume Settings */
    unsigned short          microphone_req_input_volume;    /*!< Requested Microphone Volume */
    CLD_Boolean isochronous_in_enabled;                     /*!< CLD_TRUE - Isochronous IN endpoint enabled */
    CLD_Boolean isochronous_in_idle;                        /*!< CLD_TRUE - Endpoint is idle so data can
                                                                            be transmitted. */
    unsigned long clock_sample_rate;                        /*!< Active audio sample rate */
    CLD_Boolean isochronous_out_enabled;                    /*!< CLD_TRUE - Isochronous OUT endpoint enabled */
    CLD_Boolean rate_feedback_idle;
} User_Audio_Data;

/* Locate the User_Audio_Data in un-cached memory since USB DMA will be used to access the
   audio data buffers. */
static User_Audio_Data user_audio_data =
{
    .volume_changed = CLD_FALSE,
    .headphone_output_volume.min = 0xa060,                  /* ADAU1962A USB Minimum Volume Setting = -95.6250dB */
    .headphone_output_volume.max = 0x0000,                  /* ADAU1962A USB Maximum Volume Setting = 0dB */
    .headphone_output_volume.resolution = 0x0060,           /* ADAU1962A USB Volume Resolution Setting = 0.3750dB */

    .mic_input_volume.min = 0xDC60,                         /* ADAU1979 USB Minimum Volume Setting = -35.625dB */
    .mic_input_volume.max = 0x3C00,                         /* ADAU1979 USB Maximum Volume Setting = 60dB */
    .mic_input_volume.resolution = 0x0060,                  /* ADAU1979 USB Volume Resolution Setting = 0.3750dB */

    .isochronous_in_enabled = CLD_FALSE,
    .isochronous_in_idle = CLD_TRUE,
#if (USER_AUDIO_48kHz_SAMPLE_RATE)
    .clock_sample_rate = 48000,
#else
    .clock_sample_rate = 44100,
#endif
    .isochronous_out_enabled = CLD_FALSE,
    .rate_feedback_idle = CLD_TRUE,
};

/* Clock Source sample rate range. */
static USB_Audio_4_byte_Range_Response user_audio_get_clock_sample_rate_range_resp =
{
    .w_num_sub_ranges = 1,
#if (USER_AUDIO_48kHz_SAMPLE_RATE)
    .sub_ranges[0].d_min = 48000,
    .sub_ranges[0].d_max = 48000,
#else
    .sub_ranges[0].d_min = 44100,
    .sub_ranges[0].d_max = 44100,
#endif
    .sub_ranges[0].d_res = 1,
};

static CLD_Boolean first_feedback = CLD_TRUE;

/* TODO: Refactor me */
#define AUDIO_IN_DATA_SIZE      1024
#pragma align 4
uint8_t inData[AUDIO_IN_DATA_SIZE] __attribute__ ((section (".l3_uncached_data")));
uint32_t inSize;

/* TODO: Refactor me */
#define AUDIO_OUT_DATA_SIZE      1024
#pragma align 4
uint8_t outData[AUDIO_OUT_DATA_SIZE] __attribute__ ((section (".l3_uncached_data")));
uint32_t outSize;

/**
 * Initializes the CLD SC58x USB Audio 2.0 library.
 *
 * @retval USER_AUDIO_INIT_SUCCESS.
 * @retval USER_AUDIO_INIT_ONGOING.
 * @retval USER_AUDIO_INIT_FAILED.
 */
User_Init_Return_Code user_audio_init (void)
{
    CLD_RV cld_rv = CLD_ONGOING;
    unsigned int i;

    /* Initialize the CLD Audio v2.0 with CDC Library */
    cld_rv = cld_sc58x_audio_2_0_lib_init(&user_audio_init_params);

    if (cld_rv == CLD_SUCCESS)
    {
        /* Clear USB IN/OUT buffers */
        memset(inData, 0x00, AUDIO_IN_DATA_SIZE);
        memset(outData, 0x00, AUDIO_OUT_DATA_SIZE);

        /* Register and enable the USB interrupt ISR functions */
        if (user_audio_init_params.usb_config == CLD_USB0_AUDIO)
        {
            adi_int_InstallHandler(USB0_STAT_IRQn, user_audio_usb0_isr, NULL, 1);
            adi_int_InstallHandler(USB0_DATA_IRQn, user_audio_usb0_isr, NULL, 1);
        }
        if (user_audio_init_params.usb_config == CLD_USB1_AUDIO)
        {
            adi_int_InstallHandler(USB1_STAT_IRQn, user_audio_usb1_isr, NULL, 1);
            adi_int_InstallHandler(USB1_DATA_IRQn, user_audio_usb1_isr, NULL, 1);
        }

        /* Set current volume to max */
        for (i = 0; i < NUM_AUDIO_CHANNELS; i++) {
            user_audio_data.headphone_output_volume.vol[i] = user_audio_data.headphone_output_volume.max;
        }
        user_audio_data.headphone_output_volume.mute = 0;

        for (i = 0; i < NUM_AUDIO_CHANNELS; i++) {
            user_audio_data.mic_input_volume.vol[i] =  user_audio_data.mic_input_volume.max;
        }
        user_audio_data.microphone_req_input_volume = user_audio_data.mic_input_volume.vol[0];

        /* Connect to the USB Host */
        cld_lib_usb_connect();

        return USER_AUDIO_INIT_SUCCESS;
    }
    else if (cld_rv == CLD_FAIL)
    {
        return USER_AUDIO_INIT_FAILED;
    }
    else
    {
        return USER_AUDIO_INIT_ONGOING;
    }
}


/**
 * Example User Mainline function.
 *
 */
void user_audio_main (void)
{
    static CLD_Time main_time = 0;
    unsigned int i;

    cld_sc58x_audio_2_0_lib_main();

    if (cld_time_passed_ms(main_time) >= 250u)
    {
        main_time = cld_time_get();
    }

    if (user_audio_data.volume_changed == CLD_TRUE)
    {
        user_audio_data.volume_changed = CLD_FALSE;
        for (i = 0; i < USER_AUDIO_NUM_CHANNELS; i++)
        {
            if (user_audio_data.headphone_req_output_volume.vol[i] != user_audio_data.headphone_output_volume.vol[i])
            {
                user_audio_data.headphone_output_volume.vol[i] = user_audio_data.headphone_req_output_volume.vol[i];
            }
        }

        if (user_audio_data.microphone_req_input_volume != user_audio_data.mic_input_volume.vol[0])
        {
            for (i = 0; i < USER_AUDIO_NUM_CHANNELS; i++)
            {
                user_audio_data.mic_input_volume.vol[i] = user_audio_data.microphone_req_input_volume;
            }
        }
    }

    user_audio_tx_feedback_data();

    user_audio_tx_audio_data();
}

/**
 * Transmits the pending audio IN data to the USB Host.
 *
 */
static void user_audio_tx_audio_data (void)
{
    static CLD_USB_Transfer_Params audio_data_tx_params;

#if (AUDIO_IN_ENABLED == 1)
    adi_rtl_disable_interrupts();

    /* If the Isochronous IN endpoint is enabled and idle */
    if ((user_audio_data.isochronous_in_enabled == CLD_TRUE) &&
        (user_audio_data.isochronous_in_idle == CLD_TRUE))
    {

        audio_data_tx_params.num_bytes = AUDIO_IN_DATA_SIZE;
        audio_data_tx_params.p_data_buffer = inData;
        audio_data_tx_params.callback.fp_usb_in_transfer_complete = user_audio_audio_stream_data_transmitted;
        audio_data_tx_params.transfer_timeout_ms = 100;
        audio_data_tx_params.fp_transfer_aborted_callback = user_audio_audio_stream_data_transmit_aborted;
        if (cld_sc58x_audio_2_0_lib_transmit_audio_data (&audio_data_tx_params) == CLD_USB_TRANSMIT_SUCCESSFUL)
        {
            user_audio_data.isochronous_in_idle = CLD_FALSE;
        } else {
            asm("nop;");
        }

    }
    adi_rtl_reenable_interrupts();
#endif
}

/**
 * Transmits the current feedback value to the USB Host.
 *
 */
static void user_audio_tx_feedback_data (void)
{
    static CLD_USB_Audio_Feedback_Params feedback_transfer_data =
    {
        .fp_transfer_aborted_callback = user_audio_feedback_xfr_done,
        .transfer_timeout_ms = 1100,
        .fp_usb_in_transfer_complete = user_audio_feedback_xfr_done,
    };

    adi_rtl_disable_interrupts();
    if (user_audio_data.isochronous_out_enabled == CLD_TRUE &&
            user_audio_data.rate_feedback_idle == CLD_TRUE)
    {
        if (first_feedback == CLD_FALSE)
        {
            /* TODO: Real rate feedback goes here */
        }
        else
        {
#if (USER_AUDIO_48kHz_SAMPLE_RATE)
            feedback_transfer_data.desired_data_rate = 48;
#else
            feedback_transfer_data.desired_data_rate = 44.1;
#endif
        }

        if (cld_sc58x_audio_2_0_lib_transmit_audio_rate_feedback_data(&feedback_transfer_data)== CLD_USB_TRANSMIT_SUCCESSFUL)
        {
            user_audio_data.rate_feedback_idle = CLD_FALSE;
        }
    }
    adi_rtl_reenable_interrupts();
}

/**
 * Function called when a microphone data packet has been transmitted
 * to the USB Host.
 *
 */
static void user_audio_audio_stream_data_transmitted (void)
{
    user_audio_data.isochronous_in_idle = CLD_TRUE;
    user_audio_tx_audio_data();
}

/**
 * Function called when a the microphone data transmission fails..
 *
 */
static void user_audio_audio_stream_data_transmit_aborted (void)
{
    user_audio_data.isochronous_in_idle = CLD_TRUE;
    user_audio_tx_audio_data();
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
 *                                         aborted.
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
static CLD_USB_Transfer_Request_Return_Type user_audio_stream_data_received (CLD_USB_Transfer_Params * p_transfer_data)
{
    outSize = p_transfer_data->num_bytes;
    p_transfer_data->p_data_buffer = outData;
    p_transfer_data->transfer_timeout_ms = 0;
    p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
    p_transfer_data->callback.fp_usb_out_transfer_complete = user_audio_stream_data_receive_complete;

    return CLD_USB_TRANSFER_ACCEPT;
}

/**
 * Function called when headphone data has been received.
 *
 * @retval CLD_USB_DATA_GOOD
 * @retval CLD_USB_DATA_BAD_STALL
 */
static CLD_USB_Data_Received_Return_Type user_audio_stream_data_receive_complete (void)
{
    /* TODO: Do something good with the OUT audio data */
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
static CLD_USB_Transfer_Request_Return_Type user_audio_set_req_cmd (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data)
{
    CLD_USB_Transfer_Request_Return_Type rv = CLD_USB_TRANSFER_DISCARD;

    /* If the USB Host is changing the Headphone Volume */
    if (p_req_params->entity_id == USER_AUDIO_HEADPHONE_FEATURE_UNIT_ID)
    {
        if (p_req_params->req == CLD_REQ_CURRENT)
        {
            if (p_req_params->setup_packet_wValue == USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_MASTER) /* Master Volume */
            {
                /* Store the volume setting in the headphone_req_output_volume structure */
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.headphone_req_output_volume.vol[0];
                p_transfer_data->num_bytes = 2;
            }
            else if (((p_req_params->setup_packet_wValue & 0xff00) == 0x0200) &&
                     ((p_req_params->setup_packet_wValue & 0x00ff) <= USER_AUDIO_NUM_CHANNELS))
            {
                /* Store the volume setting in the headphone_req_output_volume structure */
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.headphone_req_output_volume.vol[(p_req_params->setup_packet_wValue & 0x00ff)-1];
                p_transfer_data->num_bytes = 2;
            }
            else /* Mute */
            {
                /* Store the mute setting in the headphone_req_output_volume structure */
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.headphone_req_output_volume.mute;
                p_transfer_data->num_bytes = 1;
            }
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_out_transfer_complete = user_audio_set_volume_req;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
    }
    /* If the USB Host is changing the Microphone Volume */
    else if (p_req_params->entity_id == USER_AUDIO_MICROPHONE_FEATURE_UNIT_ID)
    {
        if (p_req_params->req == CLD_REQ_CURRENT)
        {
            /* Master Volume */
            if (p_req_params->setup_packet_wValue == USER_AUDIO_MICROPHONE_CONTROL_AND_CHANNEL_NUM_MASTER)
            {
                /* Store the volume setting in the microphone_req_input_volume buffer */
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.microphone_req_input_volume;
                p_transfer_data->num_bytes = 2;
                p_transfer_data->transfer_timeout_ms = 0;
                p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
                p_transfer_data->callback.fp_usb_out_transfer_complete = user_audio_set_volume_req;
                rv = CLD_USB_TRANSFER_ACCEPT;
            }
        }
    }
    return rv;
}

/**
 * This function is called by the CLD Audio library when a Set Volume request data
 * has been received.
 *
 * @retval CLD_USB_DATA_GOOD - Received data is valid.
 * @retval CLD_USB_DATA_BAD_STALL - Received data is invalid.
 */
static CLD_USB_Data_Received_Return_Type user_audio_set_volume_req (void)
{
    /* Flag that new volume settings have been received. */
    user_audio_data.volume_changed = CLD_TRUE;
    return CLD_USB_DATA_GOOD;
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
static CLD_USB_Transfer_Request_Return_Type user_audio_get_req_cmd (CLD_SC58x_Audio_2_0_Cmd_Req_Parameters * p_req_params, CLD_USB_Transfer_Params * p_transfer_data)
{
    CLD_USB_Transfer_Request_Return_Type rv = CLD_USB_TRANSFER_DISCARD;

    /* If the USB Host is Requesting the Headphone Volume */
    if (p_req_params->entity_id == USER_AUDIO_HEADPHONE_FEATURE_UNIT_ID)
    {
        /* Current Setting */
        if (p_req_params->req == CLD_REQ_CURRENT)
        {
            if (p_req_params->setup_packet_wValue == USER_AUDIO_HEADPHONE_CONTROL_AND_CHANNEL_NUM_MASTER) /* Master Volume */
            {
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.headphone_output_volume.vol[0];
                p_transfer_data->num_bytes = 2;
            }
            else if (((p_req_params->setup_packet_wValue & 0xff00) == 0x0200) &&
                     ((p_req_params->setup_packet_wValue & 0x00ff) <= USER_AUDIO_NUM_CHANNELS))
            {
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.headphone_output_volume.vol[(p_req_params->setup_packet_wValue & 0x00ff)-1];
                p_transfer_data->num_bytes = 2;
            }
            else /* Mute */
            {
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.headphone_output_volume.mute;
                p_transfer_data->num_bytes = 1;
            }

            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
        /* Headphone Range Settings */
        else if (p_req_params->req == CLD_REQ_RANGE)
        {
            user_audio_2_byte_range_resp.w_num_sub_ranges = 1;
            user_audio_2_byte_range_resp.sub_ranges[0].d_max = user_audio_data.headphone_output_volume.max;
            user_audio_2_byte_range_resp.sub_ranges[0].d_min = user_audio_data.headphone_output_volume.min;
            user_audio_2_byte_range_resp.sub_ranges[0].d_res = user_audio_data.headphone_output_volume.resolution;

            p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_2_byte_range_resp;
            p_transfer_data->num_bytes = sizeof(user_audio_2_byte_range_resp);
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
    }
    /* If the USB Host is Requesting the Microphone Volume */
    else if (p_req_params->entity_id == USER_AUDIO_MICROPHONE_FEATURE_UNIT_ID)
    {
        /* Current microphone Volume */
        if (p_req_params->req == CLD_REQ_CURRENT)
        {
            if (p_req_params->setup_packet_wValue == USER_AUDIO_MICROPHONE_CONTROL_AND_CHANNEL_NUM_MASTER) /* Master Volume */
            {
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.mic_input_volume.vol[0];
                p_transfer_data->num_bytes = 2;
            }
            else /* Mute */
            {
                p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.mic_input_volume.mute;
                p_transfer_data->num_bytes = 1;
            }
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;

        }
        /* Minimum microphone setting */
        else if (p_req_params->req == CLD_REQ_RANGE)
        {
            user_audio_2_byte_range_resp.w_num_sub_ranges = 1;
            user_audio_2_byte_range_resp.sub_ranges[0].d_max = user_audio_data.mic_input_volume.max;
            user_audio_2_byte_range_resp.sub_ranges[0].d_min = user_audio_data.mic_input_volume.min;
            user_audio_2_byte_range_resp.sub_ranges[0].d_res = user_audio_data.mic_input_volume.resolution;

            p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_2_byte_range_resp;
            p_transfer_data->num_bytes = sizeof(user_audio_2_byte_range_resp);
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
    }
    /* If the USB Host is Requesting the Clock Source data */
    else if (p_req_params->entity_id == USER_AUDIO_CLOCK_SOURCE_ID_DAC)
    {
        if (p_req_params->req == CLD_REQ_CURRENT)
        {
            p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.clock_sample_rate;
            p_transfer_data->num_bytes = 4;
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
        else if (p_req_params->req == CLD_REQ_RANGE)
        {
            p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_get_clock_sample_rate_range_resp;
            p_transfer_data->num_bytes = sizeof(user_audio_get_clock_sample_rate_range_resp);
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
    }
    else if (p_req_params->entity_id == USER_AUDIO_CLOCK_SOURCE_ID_ADC)
    {
        if (p_req_params->req == CLD_REQ_CURRENT)
        {
            p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_data.clock_sample_rate;
            p_transfer_data->num_bytes = 4;
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
        else if (p_req_params->req == CLD_REQ_RANGE)
        {
            p_transfer_data->p_data_buffer = (unsigned char *)&user_audio_get_clock_sample_rate_range_resp;
            p_transfer_data->num_bytes = sizeof(user_audio_get_clock_sample_rate_range_resp);
            p_transfer_data->transfer_timeout_ms = 0;
            p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
            p_transfer_data->callback.fp_usb_in_transfer_complete = CLD_NULL;
            rv = CLD_USB_TRANSFER_ACCEPT;
        }
    }
    return rv;
}

/**
 * Function called when the Isochronous OUT interface is enabled/disabled by the
 * USB Host using the Set Interface Alternate Setting request.
 *
 * @param enabled - CLD_TRUE = Isochronous OUT endpoint Enabled.
 */
static void user_audio_streaming_rx_endpoint_enabled (CLD_Boolean enabled)
{
    user_audio_data.isochronous_out_enabled = enabled;

    if (enabled == CLD_TRUE)
    {
        first_feedback = CLD_TRUE;
    }
    else
    {
        user_audio_data.rate_feedback_idle = CLD_TRUE;
    }
}

/**
 * Function called when the Isochronous IN interface is enabled/disabled by the
 * USB Host using the Set Interface Alternate Setting request.
 *
 * @param enabled - CLD_TRUE = Isochronous IN endpoint Enabled.
 */
static void user_audio_streaming_tx_endpoint_enabled (CLD_Boolean enabled)
{
    user_audio_data.isochronous_in_enabled = enabled;

    if (enabled == CLD_TRUE)
    {
        asm("nop;");
    }
    else
    {
        user_audio_data.isochronous_in_idle = CLD_TRUE;
    }
}

/**
 * Function Called when a USB event occurs on the USB Audio USB Port.
 *
 * @param event - identifies which USB event has occurred.
 */
static void user_audio_usb_event (CLD_USB_Event event)
{
    switch (event)
    {
        case CLD_USB_CABLE_CONNECTED:
            asm("nop;");
            break;
        case CLD_USB_CABLE_DISCONNECTED:
            user_audio_streaming_tx_endpoint_enabled(CLD_FALSE);
            user_audio_streaming_rx_endpoint_enabled(CLD_FALSE);
            break;
        case CLD_USB_ENUMERATED_CONFIGURED:
            asm("nop;");
            break;
        case CLD_USB_UN_CONFIGURED:
            user_audio_streaming_tx_endpoint_enabled(CLD_FALSE);
            user_audio_streaming_rx_endpoint_enabled(CLD_FALSE);
            break;
        case CLD_USB_BUS_RESET:
            user_audio_streaming_tx_endpoint_enabled(CLD_FALSE);
            user_audio_streaming_rx_endpoint_enabled(CLD_FALSE);
            break;
        default:
            break;
    }
}

/**
 * Function called when the usb feedback endpoint data has been transmitted.
 *
 * @param event - identifies which USB event has occurred.
 */
static void user_audio_feedback_xfr_done (void)
{
    user_audio_data.rate_feedback_idle = CLD_TRUE;
    first_feedback = CLD_FALSE;

    user_audio_tx_feedback_data();
}

/**
 * User defined USB 0 ISR.
 *
 * @param Event, pArg.
 */
static void user_audio_usb0_isr(uint32_t Event, void *pArg)
{
    cld_usb0_isr_callback();
}

/**
 * User defined USB 1 ISR.
 *
 * @param Event, pArg.
 */
static void user_audio_usb1_isr(uint32_t Event, void *pArg)
{
    cld_usb1_isr_callback();
}

/**
 * Function called when the CLD library reports a status.
 *
 * @param status_code, p_additional_data, additional_data_size.
 */
static void user_cld_lib_status (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size)
{
    /* Process the CLD library status */
    char * p_str = cld_lib_status_decode(status_code, p_additional_data, additional_data_size);
}
