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

#ifndef _cdc_h
#define _cdc_h

#include "cld_lib.h"

/***********************************************************************
 * Interface descriptors back to the CLD UAC+CDC library.  These are
 * installed through the 'uac2_init_params' configuration structure in
 * 'uac2_soundcard.c'.
 ***********************************************************************/
extern CLD_Serial_Data_Bulk_Endpoint_Params
    cdc_serial_data_rx_ep_params;
extern CLD_Serial_Data_Bulk_Endpoint_Params
    cdc_serial_data_tx_ep_params;
extern CLD_SC58x_CDC_Notification_Endpoint_Params
    cdc_notification_ep_params;

#ifdef __cplusplus
extern "C"{
#endif

/***********************************************************************
 * Interface functions back to the CLD UAC+CDC library.  These are
 * installed through the 'uac2_init_params' configuration structure in
 * 'uac2_soundcard.c'.
 ***********************************************************************/
CLD_USB_Transfer_Request_Return_Type
    cdc_serial_data_received(CLD_USB_Transfer_Params *p_transfer_data);

CLD_USB_Transfer_Request_Return_Type
    cdc_cmd_send_encapsulated_cmd(CLD_USB_Transfer_Params *p_transfer_data);

CLD_USB_Transfer_Request_Return_Type
    cdc_cmd_get_encapsulated_resp(CLD_USB_Transfer_Params *p_transfer_data);

CLD_USB_Data_Received_Return_Type
    cdc_cmd_set_line_coding(CLD_SC58x_CDC_Line_Coding *p_line_coding);

CLD_RV
    cdc_cmd_get_line_coding(CLD_SC58x_CDC_Line_Coding *p_line_coding);

CLD_USB_Data_Received_Return_Type
    cdc_cmd_set_control_line_state(
        CLD_SC58x_CDC_Control_Line_State *p_control_line_state);

CLD_USB_Data_Received_Return_Type
    cdc_cmd_send_break(unsigned short duration);

/***********************************************************************
 * Interface functions and enumerations for the uart_cdc_simple driver.
 ***********************************************************************/
typedef enum _CDC_TX_STATUS {
    CDC_TX_STATUS_UNKNOWN = 0,
    CDC_TX_STATUS_OK,
    CDC_TX_STATUS_TIMEOUT
} CDC_TX_STATUS;

typedef void (*CDC_TX_COMPLETE_CALLBACK)(CDC_TX_STATUS status, void *usrPtr);
typedef void (*CDC_RX_COMPLETE_CALLBACK)(unsigned char *p_buffer,
    unsigned short length, void *usrPtr);

CLD_USB_Data_Transmit_Return_Type cdc_tx_serial_data(unsigned short length,
    unsigned char *p_buffer, unsigned timeout);

void cdc_register_tx_callback(CDC_TX_COMPLETE_CALLBACK cb, void *usrPtr);
void cdc_register_rx_callback(CDC_RX_COMPLETE_CALLBACK cb, void *usrPtr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
