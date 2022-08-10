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

#include <services/int/adi_int.h>

#include "msd_host.h"

#define SWAP_ENDIAN_4_BYTES(x)   (   (x << 24)                | \
                                    ((x <<  8) & 0x00ff0000)  | \
                                    ((x >>  8) & 0x0000ff00)  | \
                                     (x >> 24)                  \
                                  )

#ifndef MSD_HOST_ENTER_CRITICAL
#define MSD_HOST_ENTER_CRITICAL  adi_rtl_disable_interrupts()
#endif

#ifndef MSD_HOST_EXIT_CRITICAL
#define MSD_HOST_EXIT_CRITICAL  adi_rtl_reenable_interrupts()
#endif

typedef struct _MSD_HOST_STATE {
    MSD_HOST_APP_CONFIG cfg;

    CLD_Boolean device_connected;
    CLD_Boolean device_initialized;

    CLD_Boolean capacity_valid;
    CLD_MSD_Read_Capasity_Response drive_capacity;

    MSD_HOST_READ_CALLBACK readCb;
    void *readUsr;

    MSD_HOST_WRITE_CALLBACK writeCb;
    void *writeUsr;

    unsigned char get_capacity_retry_cnt;
    unsigned char max_lun;

} MSD_HOST_STATE;


//__attribute__ ((section(".l3_uncached_data")))
static MSD_HOST_STATE msd_host_state;

static void msd_host_read_capacity_rx (unsigned long data_transfer_length);
static void msd_host_read_capacity_failed (CLD_SC5xx_MSD_Host_Cmd_Status status);
static void msd_host_lib_status (
    unsigned short status_code, void * p_additional_data,
    unsigned short additional_data_size);
static void msd_host_usb_event (CLD_USB_Event event);

static CLD_SC5xx_MSD_Host_Lib_Init_Params msd_init_params =
{
    .usb_port                                           = SC58x_USB0,        /* Set during config */
    .enable_dma                                         = CLD_TRUE,

    .use_built_in_vbus_ctrl                             = CLD_TRUE,          /* Set during config */
    .vbus_ctrl_open_drain                               = CLD_FALSE,         /* Set during config */
    .vbus_ctrl_inverted                                 = CLD_FALSE,         /* Set during config */
    .vbus_en_port                                       = CLD_GPIO_PORT_E,   /* Set during config */
    .vbus_en_pin                                        = CLD_GPIO_PIN_11,   /* Set during config */

    .fp_cld_usb_event_callback                          = msd_host_usb_event,
    .fp_cld_lib_status                                  = msd_host_lib_status,
};

/**
 * Function called when the CLD library reports a status.
 *
 * @param status_code, p_additional_data, additional_data_size.
 */
static void msd_host_lib_status (
    unsigned short status_code, void * p_additional_data,
    unsigned short additional_data_size)
{
#if 0
    /* Process the CLD library status */
    char * p_str = cld_lib_status_decode(status_code, p_additional_data, additional_data_size);
    sys_services_uart_print(p_str);
#endif
}

/**
 * Function called if the get max lun request is successful.
 *
 */
static void msd_host_get_lun_callback_success (void)
{
}

/**
 * Function called if the get max lun request failed.
 *
 */
static void msd_host_get_lun_failed (CLD_SC5xx_MSD_Host_Ctrl_Req_Status reason)
{
}

/**
 * Function called if the msd bulk reset request is successful.
 *
 */
static void msd_host_bulk_reset_callback_success (void)
{
    static CLD_SC5xx_MSD_Host_Cmd_Params cmd_params =
    {
        .p_data_transport = (unsigned char *)&msd_host_state.drive_capacity,
        .fp_cmd_successful_callback = msd_host_read_capacity_rx,
        .fp_cmd_failed_callback = msd_host_read_capacity_failed,
    };

    msd_host_state.capacity_valid = CLD_FALSE;

    /* Get the Drive Capacity */
    cmd_params.cmd = CLD_SC5xx_MSD_HOST_COMMAND_READ_CAPASITY;
    cmd_params.cmd_params.read_capasity.logical_unit_num = 0;
    cmd_params.cmd_params.read_capasity.logical_block_address = 0;
    cmd_params.data_transport_size = 8;
    cld_sc5xx_msd_host_lib_send_command(&cmd_params);
}

/**
 * Function called if the msd bulk reset request failed.
 *
 */
static void msd_host_bulk_reset_failed (CLD_SC5xx_MSD_Host_Ctrl_Req_Status reason)
{
}

/**
 * Function called if the msd Get Capacity command is successful.
 *
 * @param data_transfer_length.
 */
static void msd_host_read_capacity_rx (unsigned long data_transfer_length)
{
    if (data_transfer_length == sizeof (msd_host_state.drive_capacity))
    {
        msd_host_state.drive_capacity.block_length =
            SWAP_ENDIAN_4_BYTES(msd_host_state.drive_capacity.block_length);
        msd_host_state.drive_capacity.last_logical_block_address =
            SWAP_ENDIAN_4_BYTES(msd_host_state.drive_capacity.last_logical_block_address);
        msd_host_state.capacity_valid = CLD_TRUE;
        msd_host_state.device_initialized = CLD_TRUE;
        msd_host_state.get_capacity_retry_cnt = 5;
    }
    else
    {
        /* Request capacity again */
        if (msd_host_state.get_capacity_retry_cnt)
        {
            msd_host_bulk_reset_callback_success();
            msd_host_state.get_capacity_retry_cnt--;
        }
    }
}

/**
 * Function called if the msd Get Capacity command failed.
 *
 * @param status.
 */
static void msd_host_read_capacity_failed (CLD_SC5xx_MSD_Host_Cmd_Status status)
{
    msd_host_state.capacity_valid = CLD_FALSE;
    /* Request capacity again */
    if (msd_host_state.get_capacity_retry_cnt)
    {
        msd_host_bulk_reset_callback_success();
        msd_host_state.get_capacity_retry_cnt--;
    }
}

/**
 * Function Called when a USB event occurs on the USB Audio USB Port.
 *
 * @param event - identifies which USB event has occurred.
 */
static void msd_host_usb_event (CLD_USB_Event event)
{
    static CLD_SC5xx_MSD_Host_Get_Max_Lun_Params get_lun_params =
    {
        .p_max_lun = &msd_host_state.max_lun,
        .fp_cmd_successful_callback = msd_host_get_lun_callback_success,
        .fp_cmd_failed_callback = msd_host_get_lun_failed,
    };
    static CLD_SC5xx_MSD_Host_Bulk_Only_Reset_Params reset_params =
    {
        .fp_cmd_successful_callback = msd_host_bulk_reset_callback_success,
        .fp_cmd_failed_callback = msd_host_bulk_reset_failed,
    };


    switch (event)
    {
        case CLD_USB_ENUMERATED_CONFIGURED_HS:
        case CLD_USB_ENUMERATED_CONFIGURED_FS:
            cld_sc5xx_msd_host_lib_get_max_lun(&get_lun_params);
            cld_sc5xx_msd_host_lib_bulk_only_reset(&reset_params);
            msd_host_state.device_connected = CLD_TRUE;
        break;
        case CLD_USB_MSD_DISCONNECTED:
            msd_host_state.device_connected = CLD_FALSE;
            msd_host_state.device_initialized = CLD_FALSE;
            msd_host_state.get_capacity_retry_cnt = 5;
        break;
    }
}

/**
 * User defined USB 0 ISR.
 *
 * @param Event, pArg.
 */
static void msd_host_usb0_isr(uint32_t Event, void *pArg)
{
    cld_usb0_isr_callback();
}

/**
 * User defined USB 1 ISR.
 *
 * @param Event, pArg.
 */
static void msd_host_usb1_isr(uint32_t Event, void *pArg)
{
    cld_usb1_isr_callback();
}

/********************************************************
 * Public API
 ********************************************************/

CLD_RV msd_host_init_(void)
{
    msd_host_state.max_lun = 0;
    msd_host_state.device_connected = CLD_FALSE;
    msd_host_state.device_initialized = CLD_FALSE;
    msd_host_state.capacity_valid = CLD_FALSE;
    msd_host_state.get_capacity_retry_cnt = 0;

    return(CLD_SUCCESS);
}

CLD_RV msd_host_config(MSD_HOST_APP_CONFIG *cfg)
{
    /* Copy in application configuration parameters */
    MSD_HOST_MEMCPY(&msd_host_state.cfg, cfg, sizeof(msd_host_state.cfg));

    /* Copy static parameters from app cfg to CLD init params */
    msd_init_params.usb_port = cfg->port;
    msd_init_params.use_built_in_vbus_ctrl = cfg->useBuiltInVbusCtrl;
    msd_init_params.vbus_ctrl_open_drain = cfg->vbusCtrlOpenDrain;
    msd_init_params.vbus_ctrl_inverted = cfg->vbusCtrlInverted;
    msd_init_params.vbus_en_port = cfg->vbusEnablePort;
    msd_init_params.vbus_en_pin = cfg->vbusEnablePin;

    return(CLD_SUCCESS);
}

CLD_RV msd_host_start(void)
{
    CLD_RV cld_rv = CLD_ONGOING;

    /* Initialize the library */
    do {
        cld_rv = cld_sc5xx_msd_host_lib_init(&msd_init_params);
    } while (cld_rv == CLD_ONGOING);

    if (cld_rv == CLD_SUCCESS)
    {
        /* Register and enable the USB interrupt ISR functions */
#if defined(__ADSPSC589_FAMILY__)
        if (msd_init_params.usb_port == SC58x_USB0) {
            adi_int_InstallHandler(INTR_USB0_STAT, msd_host_usb0_isr, NULL, 1);
            adi_int_InstallHandler(INTR_USB0_DATA, msd_host_usb0_isr, NULL, 1);
        }
        if (msd_init_params.usb_port == SC58x_USB1) {
            adi_int_InstallHandler(INTR_USB1_STAT, msd_host_usb1_isr, NULL, 1);
            adi_int_InstallHandler(INTR_USB1_DATA, msd_host_usb1_isr, NULL, 1);
        }
#else
        if (msd_init_params.usb_port == SC57x_USB) {
            adi_int_InstallHandler(INTR_USB0_STAT, msd_host_usb0_isr, NULL, 1);
            adi_int_InstallHandler(INTR_USB0_DATA, msd_host_usb0_isr, NULL, 1);
        }
#endif
    }

    return(CLD_SUCCESS);
}

CLD_RV msd_host_run(void)
{
    static CLD_Time main_time = 0;

    cld_sc5xx_msd_host_lib_main();

    /* How to check elapsed time */
    if (cld_time_passed_ms(main_time) >= 250u) {
        main_time = cld_time_get();
    }

    return(CLD_SUCCESS);
}

CLD_RV msd_host_stop(void)
{
    return(CLD_SUCCESS);
}

CLD_RV msd_host_125us_timer_tick(void)
{
    cld_time_125us_tick();
    return(CLD_SUCCESS);
}

CLD_Boolean msd_host_ready(void)
{
    return(msd_host_state.device_initialized);
}

CLD_Boolean msd_host_info(MSD_HOST_INFO *info)
{
    CLD_Boolean ready;

    MSD_HOST_ENTER_CRITICAL;

    ready = msd_host_ready();
    if (ready && info) {
        info->capacity =
            (uint64_t)msd_host_state.drive_capacity.block_length *
            (uint64_t)msd_host_state.drive_capacity.last_logical_block_address;
    } else {
        info->capacity = 0;
    }

    MSD_HOST_EXIT_CRITICAL;

    return(ready);
}

static inline void msd_host_read_cb(bool success)
{
    if (msd_host_state.readCb) {
        msd_host_state.readCb(success, msd_host_state.readUsr);
    }
}

/**
 * Function called if the disk_read MSD Read command is successful.
 *
 * @param data_transfer_length.
 */
static void msd_host_read_disk_successful_callback(unsigned long data_transfer_length)
{
    msd_host_read_cb(true);
}

/**
 * Function called if the disk_read MSD Read command failed.
 *
 * @param status.
 */
static void msd_host_read_disk_failed_callback(CLD_SC5xx_MSD_Host_Cmd_Status status)
{
    msd_host_read_cb(false);
}

CLD_RV msd_host_read(void *data, uint64_t sector, uint32_t count,
    MSD_HOST_READ_CALLBACK cb, void *usr)
{
    CLD_RV ret;

    if (msd_host_state.device_initialized == CLD_FALSE) {
        return(CLD_FAIL);
    }

    CLD_SC5xx_MSD_Host_Cmd_Params cmd_params =
    {
        .p_data_transport = data,
        .fp_cmd_successful_callback = msd_host_read_disk_successful_callback,
        .fp_cmd_failed_callback     = msd_host_read_disk_failed_callback,
    };

    msd_host_state.readCb = cb;
    msd_host_state.readUsr = usr;

    cmd_params.cmd = CLD_SC5xx_MSD_HOST_COMMAND_READ_10;
    cmd_params.cmd_params.read_10.logical_block_address = sector;
    cmd_params.cmd_params.read_10.logical_unit_num = 0;
    cmd_params.cmd_params.read_10.transfer_length = count;
    cmd_params.data_transport_size = count * msd_host_state.drive_capacity.block_length;
    ret = cld_sc5xx_msd_host_lib_send_command(&cmd_params);

    return(ret);
}

static inline void msd_host_write_cb(bool success)
{
    if (msd_host_state.writeCb) {
        msd_host_state.writeCb(success, msd_host_state.writeUsr);
    }
}

/**
 * Function called if the disk_write MSD Read command is successful.
 *
 * @param data_transfer_length.
 */
static void msd_host_write_disk_successful_callback(unsigned long data_transfer_length)
{
    msd_host_write_cb(true);
}

/**
 * Function called if the disk_write MSD Read command failed.
 *
 * @param status.
 */
static void msd_host_write_disk_failed_callback(CLD_SC5xx_MSD_Host_Cmd_Status status)
{
    msd_host_write_cb(false);
}

CLD_RV msd_host_write(void *data, uint64_t sector, uint32_t count,
    MSD_HOST_READ_CALLBACK cb, void *usr)
{
    CLD_RV ret;

    if (msd_host_state.device_initialized == CLD_FALSE) {
        return(CLD_FAIL);
    }

    CLD_SC5xx_MSD_Host_Cmd_Params cmd_params =
    {
        .p_data_transport = data,
        .fp_cmd_successful_callback = msd_host_write_disk_successful_callback,
        .fp_cmd_failed_callback     = msd_host_write_disk_failed_callback,
    };

    msd_host_state.writeCb = cb;
    msd_host_state.writeUsr = usr;

    cmd_params.cmd = CLD_SC5xx_MSD_HOST_COMMAND_WRITE_10;
    cmd_params.cmd_params.write_10.logical_block_address = sector;
    cmd_params.cmd_params.write_10.logical_unit_num = 0;
    cmd_params.cmd_params.write_10.transfer_length = count;
    cmd_params.data_transport_size = count * msd_host_state.drive_capacity.block_length;
    ret = cld_sc5xx_msd_host_lib_send_command(&cmd_params);

    return(ret);
}
