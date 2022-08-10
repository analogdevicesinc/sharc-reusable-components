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

#include <stddef.h>
#include "cdc.h"

CDC_TX_COMPLETE_CALLBACK txCB = NULL;
void *txUsrPtr = NULL;

CDC_RX_COMPLETE_CALLBACK rxCB = NULL;
void *rxUsrPtr = NULL;
unsigned short cdc_rx_bytes;

/**
 * Buffer used to store the received serial data.
 */
#define CDC_RX_BUFFER_SIZE     512
__attribute__ ((aligned(4), section(".l3_uncached")))
    unsigned char cdc_rx_buffer[CDC_RX_BUFFER_SIZE];

CLD_SC58x_CDC_Line_Coding
cdc_line_coding =
{
    .data_terminal_rate = 115200,
    .num_stop_bits      = CLD_SC58x_CDC_LINE_CODING_1_STOP_BITS,
    .parity             = CLD_SC58x_CDC_LINE_CODING_PARITY_NONE,
    .num_data_bits      = 8,
};

/*!< CDC Serial Data Bulk OUT endpoint parameters. */
CLD_Serial_Data_Bulk_Endpoint_Params
cdc_serial_data_rx_ep_params =
{
    .endpoint_number                = 5,
    .max_packet_size_full_speed     = 64,
    .max_packet_size_high_speed     = 512,
};

/*!< CDC Serial Data Bulk IN endpoint parameters. */
CLD_Serial_Data_Bulk_Endpoint_Params
cdc_serial_data_tx_ep_params =
{
    .endpoint_number                = 5,
    .max_packet_size_full_speed     = 64,
    .max_packet_size_high_speed     = 512,
};

/*!< CDC Notification Interrupt IN endpoint parameters. */
CLD_SC58x_CDC_Notification_Endpoint_Params
cdc_notification_ep_params =
{
    .endpoint_number                = 4,
    .max_packet_size_full_speed     = 64,
    .polling_interval_full_speed    = 1,
    .max_packet_size_high_speed     = 64,
    .polling_interval_high_speed    = 4,  /* 1ms */
};

CLD_USB_Data_Received_Return_Type
cdc_serial_data_rx_transfer_complete(void)
{
    if (rxCB) {
        rxCB(cdc_rx_buffer, cdc_rx_bytes, rxUsrPtr);
    }
    return CLD_USB_DATA_GOOD;
}

CLD_USB_Transfer_Request_Return_Type
cdc_serial_data_received(CLD_USB_Transfer_Params *p_transfer_data)
{
    p_transfer_data->p_data_buffer = cdc_rx_buffer;
    p_transfer_data->callback.fp_usb_out_transfer_complete = cdc_serial_data_rx_transfer_complete;
    p_transfer_data->fp_transfer_aborted_callback = CLD_NULL;
    p_transfer_data->transfer_timeout_ms = 0;
    cdc_rx_bytes = p_transfer_data->num_bytes;
    return CLD_USB_TRANSFER_ACCEPT;
}

CLD_USB_Transfer_Request_Return_Type
cdc_cmd_send_encapsulated_cmd(CLD_USB_Transfer_Params *p_transfer_data)
{
    /* Not currently supported */
    return CLD_USB_TRANSFER_STALL;
}

CLD_USB_Transfer_Request_Return_Type
cdc_cmd_get_encapsulated_resp(CLD_USB_Transfer_Params *p_transfer_data)
{
    /* Not currently supported */
    return CLD_USB_TRANSFER_STALL;
}

CLD_USB_Data_Received_Return_Type
cdc_cmd_set_line_coding(CLD_SC58x_CDC_Line_Coding *p_line_coding)
{
    cdc_line_coding = *p_line_coding;
    return CLD_USB_DATA_GOOD;
}

CLD_RV
cdc_cmd_get_line_coding(CLD_SC58x_CDC_Line_Coding *p_line_coding)
{
    *p_line_coding = cdc_line_coding;
    return CLD_SUCCESS;
}

CLD_USB_Data_Received_Return_Type
cdc_cmd_set_control_line_state(
    CLD_SC58x_CDC_Control_Line_State *p_control_line_state)
{
    return CLD_USB_DATA_GOOD;
}

CLD_USB_Data_Received_Return_Type
cdc_cmd_send_break(unsigned short duration)
{
    return CLD_USB_DATA_GOOD;
}

static void cdc_usb_tx_complete(void)
{
    if (txCB) {
        txCB(CDC_TX_STATUS_OK, txUsrPtr);
    }
}

static void cdc_usb_tx_timeout(void)
{
    if (txCB) {
        txCB(CDC_TX_STATUS_TIMEOUT, txUsrPtr);
    }
}

CLD_USB_Data_Transmit_Return_Type
cdc_tx_serial_data(unsigned short length, unsigned char *p_buffer, unsigned timeout)
{
    static CLD_USB_Transfer_Params transfer_params;
    CLD_USB_Data_Transmit_Return_Type ret;
    transfer_params.num_bytes = length;
    transfer_params.p_data_buffer = p_buffer;
    transfer_params.callback.fp_usb_in_transfer_complete = cdc_usb_tx_complete;
    transfer_params.fp_transfer_aborted_callback = cdc_usb_tx_timeout;
    transfer_params.transfer_timeout_ms = timeout;
    ret = cld_sc58x_cdc_lib_transmit_serial_data(&transfer_params);
    return(ret);
}

void cdc_register_tx_callback(CDC_TX_COMPLETE_CALLBACK cb, void *usrPtr)
{
    txCB = cb;
    txUsrPtr = usrPtr;
}

void cdc_register_rx_callback(CDC_RX_COMPLETE_CALLBACK cb, void *usrPtr)
{
    rxCB = cb;
    rxUsrPtr = usrPtr;
}
