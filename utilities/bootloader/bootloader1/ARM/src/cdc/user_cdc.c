/*=============================================================================
    FILE:           user_cdc.c

    DESCRIPTION:    Uses the cld_sc58x_cdc_lib library to implement a basic
                    CDC/ACM device.

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
 * @file      user_cdc.c
 * @brief     Uses the cld_sc58x_cdc_lib library to implement a basic
 *            USB CDC/ACM device.
 *
 * @details
 *            This file contains mainline state machine used to implement a
 *            CDC/ACM device using the cld_sc58x_cdc_lib library on
 *            a SC589 Cortex-A5 processor.
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

#include "user_cdc.h"
#include "cld_sc58x_cdc_lib.h"
#include "flash_cmd.h"

#include <services/gpio/adi_gpio.h>
#include <services/int/adi_int.h>
#include <services/tmr/adi_tmr.h>
#include <string.h>

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

#define USER_PRODUCT_ID_CDC             0x0006  /*!< USB Product ID used when the CDC uses it's own USB port. */




/* Function prototypes */
static void user_cdc_usb_in_transfer_complete (void);
static CLD_USB_Data_Received_Return_Type user_cdc_serial_data_rx_transfer_complete (void);
static CLD_USB_Transfer_Request_Return_Type user_cdc_serial_data_received (CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Transfer_Request_Return_Type user_cdc_cmd_send_encapsulated_cmd (CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Transfer_Request_Return_Type user_cdc_cmd_get_encapsulated_resp (CLD_USB_Transfer_Params * p_transfer_data);
static CLD_USB_Data_Received_Return_Type user_cdc_cmd_set_line_coding (CLD_SC58x_CDC_Line_Coding * p_line_coding);
static CLD_RV user_cdc_cmd_get_line_coding (CLD_SC58x_CDC_Line_Coding * p_line_coding);
static CLD_USB_Data_Received_Return_Type user_cdc_cmd_set_control_line_state (CLD_SC58x_CDC_Control_Line_State * p_control_line_state);
static CLD_USB_Data_Received_Return_Type user_cdc_cmd_send_break (unsigned short duration);
void user_cdc_tx_serial_data (unsigned short length, unsigned char * p_buffer);

static void user_cdc_usb_event (CLD_USB_Event event);
static void user_cld_lib_status (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size);

static void user_cdc_usb0_isr(uint32_t Event, void *pArg);
static void user_cdc_usb1_isr(uint32_t Event, void *pArg);

/*!< CDC Notification Interrupt IN endpoint parameters. */
static CLD_SC58x_CDC_Notification_Endpoint_Params user_cdc_notification_ep_params =
{
    .endpoint_number                = 4,
    .max_packet_size_full_speed     = 64,
    .polling_interval_full_speed    = 1,
    .max_packet_size_high_speed     = 64,
    .polling_interval_high_speed    = 4,  /* 1ms */
};

/*!< CDC Serial Data Bulk OUT endpoint parameters. */
static CLD_Serial_Data_Bulk_Endpoint_Params user_cdc_serial_data_rx_ep_params =
{
    .endpoint_number                = 5,
    .max_packet_size_full_speed     = 64,
    .max_packet_size_high_speed     = 512,
};

/*!< CDC Serial Data Bulk IN endpoint parameters. */
static CLD_Serial_Data_Bulk_Endpoint_Params user_cdc_serial_data_tx_ep_params =
{
    .endpoint_number                = 5,
    .max_packet_size_full_speed     = 64,
    .max_packet_size_high_speed     = 512,
};


/*!< CLD SC58x CDC library initialization data. */
static CLD_SC58x_CDC_Lib_Init_Params user_cdc_init_params =
{
    .usb_config                                         = CLD_USB0_CDC,
//    .usb_config                                         = CLD_USB1_CDC,
    .enable_dma                                         = CLD_FALSE,

    .vendor_id                                          = 0x064b,       /* Analog Devices Vendor ID */
    .product_id                                         = USER_PRODUCT_ID_CDC,

    .usb_bus_max_power                                  = 0,
    .device_descriptor_bcdDevice                        = 0x0100,


    .p_serial_data_rx_endpoint_params                   = &user_cdc_serial_data_rx_ep_params,
    .p_serial_data_tx_endpoint_params                   = &user_cdc_serial_data_tx_ep_params,
    .p_notification_endpoint_params                     = &user_cdc_notification_ep_params,

    .fp_serial_data_received                            = user_cdc_serial_data_received,

    .fp_cdc_cmd_send_encapsulated_cmd                   = user_cdc_cmd_send_encapsulated_cmd,
    .fp_cdc_cmd_get_encapsulated_resp                   = user_cdc_cmd_get_encapsulated_resp,

    .fp_cdc_cmd_set_line_coding                         = user_cdc_cmd_set_line_coding,
    .fp_cdc_cmd_get_line_coding                         = user_cdc_cmd_get_line_coding,

    .fp_cdc_cmd_set_control_line_state                  = user_cdc_cmd_set_control_line_state,

    .fp_cdc_cmd_send_break                              = user_cdc_cmd_send_break,

    .support_cdc_network_connection                     = 1,
    .cdc_class_bcd_version                              = 0x0120,       /* CDC Version 1.2 */
    .cdc_class_control_protocol_code                    = 0,            /* No Class Specific protocol */

    .p_usb_string_manufacturer                          = "Analog Devices Inc",
    .p_usb_string_product                               = "SHARC Audio Module",
    .p_usb_string_serial_number                         = CLD_NULL,
    .p_usb_string_configuration                         = CLD_NULL,

    .p_usb_string_communication_class_interface         = "CLD CDC Ctrl",
    .p_usb_string_data_class_interface                  = "CLD CDC Data",

    .usb_string_language_id                             = 0x0409,       /* English (US) language ID */

    .fp_cld_usb_event_callback                          = user_cdc_usb_event,
    .fp_cld_lib_status                                  = user_cld_lib_status,
};


/* Current CDC line coding settings (default to 115.2k 8 bits, 1 Stop bit, no parity) */
static CLD_SC58x_CDC_Line_Coding user_cdc_line_coding =
{
    .data_terminal_rate = 115200,
    .num_stop_bits      = CLD_SC58x_CDC_LINE_CODING_1_STOP_BITS,
    .parity             = CLD_SC58x_CDC_LINE_CODING_PARITY_NONE,
    .num_data_bits      = 8,
};

volatile int paused = 0;
volatile int xfer_state = XFER_STATE_IDLE;

#define USER_CDC_SERIAL_BUFFER_SIZE     512                 /*!< CDC Serial Data receive buffer size */
unsigned short user_cdc_serial_data_num_rx_bytes;           /*!< Number of received Serial Data bytes */
/**
 * Buffer used to store the received serial data.
 */
unsigned char user_cdc_serial_data_rx_buffer[USER_CDC_SERIAL_BUFFER_SIZE]__attribute__ ((aligned (4),section(".l2_uncached")));

/**
 * Initializes the CLD SC58x CDC library.
 *
 * @retval USER_CDC_INIT_SUCCESS.
 * @retval USER_CDC_INIT_ONGOING.
 * @retval USER_CDC_INIT_FAILED.
 */
User_Init_Return_Code user_cdc_init (void)
{
    CLD_RV cld_rv = CLD_ONGOING;

    /* Initialize the CLD CDC Library */
    cld_rv = cld_sc58x_cdc_lib_init(&user_cdc_init_params);

    if (cld_rv == CLD_SUCCESS)
    {
        /* Register and enable the USB interrupt ISR functions */
        if (user_cdc_init_params.usb_config == CLD_USB0_CDC)
        {
            adi_int_InstallHandler(USB0_STAT_IRQn, user_cdc_usb0_isr, NULL, 1);
            adi_int_InstallHandler(USB0_DATA_IRQn, user_cdc_usb0_isr, NULL, 1);
        }
        if (user_cdc_init_params.usb_config == CLD_USB1_CDC)
        {
            adi_int_InstallHandler(USB1_STAT_IRQn, user_cdc_usb1_isr, NULL, 1);
            adi_int_InstallHandler(USB1_DATA_IRQn, user_cdc_usb1_isr, NULL, 1);
        }
        cld_lib_usb_connect();

#if 0
        /* Configure the GPIO pins connected to the SC589 EZ-Board general purpose LEDs (LED10, LED11, LED12) */
        adi_gpio_SetDirection(ADI_GPIO_PORT_E, ADI_GPIO_PIN_13, ADI_GPIO_DIRECTION_OUTPUT);
        adi_gpio_Clear(ADI_GPIO_PORT_E, ADI_GPIO_PIN_13);
        adi_gpio_SetDirection(ADI_GPIO_PORT_E, ADI_GPIO_PIN_14, ADI_GPIO_DIRECTION_OUTPUT);
        adi_gpio_Clear(ADI_GPIO_PORT_E, ADI_GPIO_PIN_14);
        adi_gpio_SetDirection(ADI_GPIO_PORT_E, ADI_GPIO_PIN_15, ADI_GPIO_DIRECTION_OUTPUT);
        adi_gpio_Clear(ADI_GPIO_PORT_E, ADI_GPIO_PIN_15);
#endif

        return USER_CDC_INIT_SUCCESS;
    }
    else if (cld_rv == CLD_FAIL)
    {
        return USER_CDC_INIT_FAILED;
    }
    else
    {
        return USER_CDC_INIT_ONGOING;
    }
}


/**
 * Example User Mainline function.
 *
 */
void user_cdc_main (void)
{
    static CLD_Time main_time = 0;

    flash_cmd_main();

    cld_sc58x_cdc_lib_main();

    if (cld_time_passed_ms(main_time) >= 250u)
    {
        main_time = cld_time_get();
#if 0
        adi_gpio_Toggle(ADI_GPIO_PORT_E, ADI_GPIO_PIN_13);
#endif
    }
}

/**
 * This function is called by the CLD library when Bulk IN serial data has been
 * transmitted to the Host.
 */
static void user_cdc_usb_in_transfer_complete (void)
{
}

/**
 * Function called when Serial data has been received.
 *
 * @retval CLD_USB_DATA_GOOD
 * @retval CLD_USB_DATA_BAD_STALL
 */
static CLD_USB_Data_Received_Return_Type user_cdc_serial_data_rx_transfer_complete (void)
{
    xfer_state = XFER_STATE_DATA_READY;
    return CLD_USB_DATA_GOOD;
}

/**
 * This function is called by the cld_sc58x_cdc_lib library when data is
 * received on the Bulk OUT endpoint. This function sets the
 * p_transfer_data parameters to select where the received data
 * should be stored, and what callback function should be called when the
 * transfer is complete.
 *
 * @param p_transfer_data                - Pointer to the Bulk OUT transfer data.
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
 *                                  until the cld_sc58x_cdc_lib_resume_paused_serial_data_transfer
 *                                  function is called.
 * @retval CLD_USB_TRANSFER_DISCARD - Discard this packet.
 * @retval CLD_USB_TRANSFER_STALL - Stall the OUT endpoint.
 *
 */
static CLD_USB_Transfer_Request_Return_Type user_cdc_serial_data_received (CLD_USB_Transfer_Params * p_transfer_data)
{
    CLD_USB_Transfer_Request_Return_Type ret;

    if (xfer_state == XFER_STATE_IDLE) {
        p_transfer_data->p_data_buffer = (unsigned char *)&user_cdc_serial_data_rx_buffer;
        p_transfer_data->callback.fp_usb_out_transfer_complete = user_cdc_serial_data_rx_transfer_complete;
        p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
        p_transfer_data->transfer_timeout_ms = 1000;
        user_cdc_serial_data_num_rx_bytes = p_transfer_data->num_bytes;
        xfer_state = XFER_STATE_ACCEPTED;
        ret = CLD_USB_TRANSFER_ACCEPT;
    } else {
        ret = CLD_USB_TRANSFER_PAUSE;
        paused = 1;
    }

    return(ret);
}

/**
 * This function is called by the CLD library when a CDC Send Encapsulated Command
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
 * @retval CLD_USB_TRANSFER_ACCEPT - Store the data using the p_transfer_data
 *                                  parameters.
 * @retval CLD_USB_TRANSFER_PAUSE - The device isn't ready to process this
 *                                  out packet so pause the transfer
 *                                  until the cld_sc58x_cdc_lib_resume_paused_control_transfer
 *                                  function is called.
 * @retval CLD_USB_TRANSFER_DISCARD - Discard this packet.
 * @retval CLD_USB_TRANSFER_STALL - Stall the OUT endpoint.
 */
static CLD_USB_Transfer_Request_Return_Type user_cdc_cmd_send_encapsulated_cmd (CLD_USB_Transfer_Params * p_transfer_data)
{
    /* Not currently supported by this example. */
    return CLD_USB_TRANSFER_STALL;
}

/**
 * This function is called by the CLD library when a CDC Get Encapsulated Command
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
 * @retval CLD_USB_TRANSFER_ACCEPT - Store the data using the p_transfer_data
 *                                   parameters.
 * @retval CLD_USB_TRANSFER_PAUSE - The device isn't ready to process this
 *                                  out packet so pause the transfer
 *                                  until the cld_sc58x_cdc_lib_resume_paused_control_transfer
 *                                  function is called.
 * @retval CLD_USB_TRANSFER_DISCARD - Discard this packet.
 * @retval CLD_USB_TRANSFER_STALL - Stall the OUT endpoint.
 */
static CLD_USB_Transfer_Request_Return_Type user_cdc_cmd_get_encapsulated_resp (CLD_USB_Transfer_Params * p_transfer_data)
{
    /* Not currently supported by this example. */
    return CLD_USB_TRANSFER_STALL;
}

/**
 * This function is called by the CLD library when a CDC Set Line Coding
 * request is received. This function saves the requested line coding.
 *
 * @retval CLD_USB_DATA_GOOD
 * @retval CLD_USB_DATA_BAD_STALL
 */
static CLD_USB_Data_Received_Return_Type user_cdc_cmd_set_line_coding (CLD_SC58x_CDC_Line_Coding * p_line_coding)
{
    user_cdc_line_coding = *p_line_coding;
    return CLD_USB_DATA_GOOD;
}

/**
 * This function is called by the CLD library when a CDC Get Line Coding
 * request is received. This function loads the current line coding into the
 * p_line_coding structure passed to the function.
 *
 * @retval CLD_SUCCESS - There was no errors copying the current line coding.
 * @retval CLD_FAIL    - There was a problem copying the current line coding.
 */
static CLD_RV user_cdc_cmd_get_line_coding (CLD_SC58x_CDC_Line_Coding * p_line_coding)
{
    *p_line_coding = user_cdc_line_coding;

    return CLD_SUCCESS;
}

/**
 * This function is called by the CLD library when a CDC Set Control Line State
 * request is received.
 *
 * @retval CLD_USB_DATA_GOOD - Set control line state parameters accepted.
 * @retval CLD_USB_DATA_BAD_STALL - An error occurred while applying the requested
 *                                  set control line state parameters.
 */
static CLD_USB_Data_Received_Return_Type user_cdc_cmd_set_control_line_state (CLD_SC58x_CDC_Control_Line_State * p_control_line_state)
{
    return CLD_USB_DATA_GOOD;
}

/**
 * This function is called by the CLD library when a CDC Send Break
 * request is received.
 *
 * @retval CLD_USB_DATA_GOOD
 * @retval CLD_USB_DATA_BAD_STALL
 */
static CLD_USB_Data_Received_Return_Type user_cdc_cmd_send_break (unsigned short duration)
{
    return CLD_USB_DATA_GOOD;
}

/**
 * Transmits the specified data to the CDC Host using the serial data Bulk IN endpoint.
 *
 * @param length   - number of bytes to transmit.
 * @param p_buffer - pointer to the data buffer to transmit
 */
void user_cdc_tx_serial_data (unsigned short length, unsigned char * p_buffer)
{
    static CLD_USB_Transfer_Params transfer_params;
    transfer_params.num_bytes = length;
    transfer_params.p_data_buffer = p_buffer;
    transfer_params.callback.fp_usb_in_transfer_complete = user_cdc_usb_in_transfer_complete;
    transfer_params.fp_transfer_aborted_callback = CLD_NULL;
    transfer_params.transfer_timeout_ms = 1000;

    cld_sc58x_cdc_lib_transmit_serial_data (&transfer_params);
}

/**
 * Function Called when a USB event occurs on the CDC USB Port.
 *
 * @param event - identifies which USB event has occurred.
 */
static void user_cdc_usb_event (CLD_USB_Event event)
{
    switch (event)
    {
        case CLD_USB_CABLE_CONNECTED:
        break;
        case CLD_USB_CABLE_DISCONNECTED:
        break;
        case CLD_USB_ENUMERATED_CONFIGURED:
        break;
        case CLD_USB_UN_CONFIGURED:
        break;
        case CLD_USB_BUS_RESET:
        break;
    }
}

/**
 * User defined USB 0 ISR.
 *
 * @param Event, pArg.
 */
static void user_cdc_usb0_isr(uint32_t Event, void *pArg)
{
    cld_usb0_isr_callback();
}

/**
 * User defined USB 1 ISR.
 *
 * @param Event, pArg.
 */
static void user_cdc_usb1_isr(uint32_t Event, void *pArg)
{
    cld_usb1_isr_callback();
}

/**
 * User defined Timer ISR.
 *
 * @param Event, pArg.
 */

void user_cdc_timer_handler(void *pCBParam, uint32_t Event, void *pArg)
{
    switch(Event)
    {
        case ADI_TMR_EVENT_DATA_INT:
            /* Call the CLD library 125 microsecond function */
            cld_time_125us_tick();
            break;

        default:
            break;
    }

    return;
}

/**
 * Function called when the CLD library reports a status.
 *
 * @param status_code, p_additional_data, additional_data_size.
 */
static void user_cld_lib_status (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size)
{
#if 0
    /* Process the CLD library status */
    char * p_str = cld_lib_status_decode(status_code, p_additional_data, additional_data_size);
#endif
}
