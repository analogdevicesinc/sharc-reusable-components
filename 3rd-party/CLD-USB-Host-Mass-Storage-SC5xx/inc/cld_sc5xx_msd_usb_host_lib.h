#ifndef __CLD_MSD_HOST_LIB__
#define __CLD_MSD_HOST_LIB__
/*=============================================================================
    FILE:           cld_sc5xx_msd_usb_host_lib.h

    DESCRIPTION:    CLD Mass Storage Device (Bulk Only) USB Host.

    Copyright (c) 2020 Closed Loop Design, LLC

    This software is supplied "AS IS" without any warranties, express, implied
    or statutory, including but not limited to the implied warranties of fitness
    for purpose, satisfactory quality and non-infringement. Closed Loop Design LLC
    extends you a royalty-free right to use, reproduce and distribute this source
    file as well as executable files created using this source file for use with
    Analog Devices SC5xx family processors only. Nothing else gives you
    the right to use this source file..

==============================================================================*/
/*!
 * @file      cld_sc5xx_msd_usb_host_lib.h
 * @brief     CLD Mass Storage Device (Bulk Only) USB Host.
 *
 * @details
 *            This file contains the functionality needed to interface with the
 *            CLD USB MSD USB Host library using the SC5xx's Cortex-A5 processor.
 *
 *            Copyright (c) 2020 Closed Loop Design, LLC
 *
 *            This software is supplied "AS IS" without any warranties, express, implied
 *            or statutory, including but not limited to the implied warranties of fitness
 *            for purpose, satisfactory quality and non-infringement. Closed Loop Design LLC
 *            extends you a royalty-free right to use, and reproduce this source
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

#include "cld_sc5xx_msd_defs.h"

/**
 * CLD_Time typedef
 * used by CLD time keeping functions.
 */
typedef unsigned long CLD_Time;

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
 * CLD_USB_Port enum defines the CLD library's USB port selection values.
 */
typedef enum
{
    SC58x_USB0  = 0,
    SC58x_USB1,
    SC57x_USB,
} CLD_USB_Port;

/**
 * CLD_GPIO_Port enum defines the CLD library's GPIO port selection values.
 */
typedef enum
{
    CLD_GPIO_PORT_A = 0,
    CLD_GPIO_PORT_B,
    CLD_GPIO_PORT_C,
    CLD_GPIO_PORT_D,
    CLD_GPIO_PORT_E,
    CLD_GPIO_PORT_F,
    CLD_GPIO_PORT_G,
    CLD_GPIO_NUM_PORTS
} CLD_GPIO_Port;

/**
 * CLD_GPIO_PIN enum defines the CLD library's GPIO pin selection values.
 */
typedef enum
{
    CLD_GPIO_PIN_0,
    CLD_GPIO_PIN_1,
    CLD_GPIO_PIN_2,
    CLD_GPIO_PIN_3,
    CLD_GPIO_PIN_4,
    CLD_GPIO_PIN_5,
    CLD_GPIO_PIN_6,
    CLD_GPIO_PIN_7,
    CLD_GPIO_PIN_8,
    CLD_GPIO_PIN_9,
    CLD_GPIO_PIN_10,
    CLD_GPIO_PIN_11,
    CLD_GPIO_PIN_12,
    CLD_GPIO_PIN_13,
    CLD_GPIO_PIN_14,
    CLD_GPIO_PIN_15,
} CLD_GPIO_PIN;

/**
 * Supported SCSI commands
 */
typedef enum
{
    CLD_SC5xx_MSD_HOST_COMMAND_INQUIRY = 0,
    CLD_SC5xx_MSD_HOST_COMMAND_MODE_SELECT_6,
    CLD_SC5xx_MSD_HOST_COMMAND_MODE_SELECT_10,
    CLD_SC5xx_MSD_HOST_COMMAND_MODE_SENSE_6,
    CLD_SC5xx_MSD_HOST_COMMAND_MODE_SENSE_10,
    CLD_SC5xx_MSD_HOST_COMMAND_PREVENT_ALLOW_REMOVAL,
    CLD_SC5xx_MSD_HOST_COMMAND_READ_10,
    CLD_SC5xx_MSD_HOST_COMMAND_READ_12,
    CLD_SC5xx_MSD_HOST_COMMAND_READ_CAPASITY,
    CLD_SC5xx_MSD_HOST_COMMAND_READ_FORMAT_CAPACITIES,
    CLD_SC5xx_MSD_HOST_COMMAND_REQUEST_SENSE,
    CLD_SC5xx_MSD_HOST_COMMAND_SEND_DIAGNOSTIC,
    CLD_SC5xx_MSD_HOST_COMMAND_START_STOP_UNIT,
    CLD_SC5xx_MSD_HOST_COMMAND_TEST_UNIT_READY,
    CLD_SC5xx_MSD_HOST_COMMAND_VERIFY,
    CLD_SC5xx_MSD_HOST_COMMAND_WRITE_10,
    CLD_SC5xx_MSD_HOST_COMMAND_WRITE_12,
    CLD_SC5xx_MSD_HOST_COMMAND_WRITE_AND_VERIFY,
    CLD_SC5xx_MSD_HOST_NUM_COMMANDS
} CLD_SC5xx_MSD_Host_Commands;

/**
 * Mass Storage Device SCSI command failed reason.
 */
typedef enum
{
    COMMAND_FAILED = 0,
    COMMAND_PHASE_ERROR,
    NAK_LIMIT_REACHED,
} CLD_SC5xx_MSD_Host_Cmd_Status;

/**
 * CLD Library MSD SCSI Command Parameters
 */
typedef struct
{
    CLD_SC5xx_MSD_Host_Commands         cmd;
    CLD_SC5xx_MSD_Command_Parameters    cmd_params;
    unsigned long                       data_transport_size;
    unsigned char *                     p_data_transport;
    void                                (*fp_cmd_successful_callback) (unsigned long data_transport_length);
    void                                (*fp_cmd_failed_callback) (CLD_SC5xx_MSD_Host_Cmd_Status status);
} CLD_SC5xx_MSD_Host_Cmd_Params;

/**
 * Mass Storage Device Control endpoint command failed reason.
 */
typedef enum
{
    CTRL_REQ_STALLED = 0,
    CTRL_REQ_NAK_LIMIT,
    CTRL_REQ_TIMEOUT,
    CTRL_REQ_ERROR_READING_RX_FIFO,
} CLD_SC5xx_MSD_Host_Ctrl_Req_Status;

/**
 * CLD library MSD get max lun Control endpoint command parameters
 */
typedef struct
{
    unsigned char *                     p_max_lun;
    void                                (*fp_cmd_successful_callback) (void);
    void                                (*fp_cmd_failed_callback) (CLD_SC5xx_MSD_Host_Ctrl_Req_Status reason);
} CLD_SC5xx_MSD_Host_Get_Max_Lun_Params;

/**
 * CLD library MSD Bulk-only reset control endpoint command parameters
 */
typedef struct
{
    void                                (*fp_cmd_successful_callback) (void);
    void                                (*fp_cmd_failed_callback) (CLD_SC5xx_MSD_Host_Ctrl_Req_Status reason);
} CLD_SC5xx_MSD_Host_Bulk_Only_Reset_Params;

/**
 * CLD Library USB Events
 */
typedef enum
{
    CLD_USB_ENUMERATED_CONFIGURED_HS = 0,   /*!< USB Device configured High-Speed. */
    CLD_USB_ENUMERATED_CONFIGURED_FS,       /*!< USB Device configured Full-Speed. */
    CLD_USB_MSD_DISCONNECTED,               /*!< USB MSD disconnected. */
} CLD_USB_Event;

/**
 * CLD MSD USB Host Library initialization parameters
 */
typedef struct
{
    CLD_USB_Port    usb_port;                           /*!< Specifies the USB Port used by the USB Host */
    CLD_Boolean     enable_dma;                         /*!< Used to enable/disable USB DMA support
                                                         *   - CLD_TRUE = DMA enabled for USB transfers
                                                         *               larger than 32 bytes that are
                                                         *               aligned on a 4-byte boundary.
                                                         *   - CLD_FALSE = DMA disabled. */

    CLD_Boolean     use_built_in_vbus_ctrl;             /*!< Used to select if the SC5xx USB VBC output is used
                                                         *   to control the external Vbus switch.
                                                         *   - CLD_TRUE = Use VBC signal
                                                         *   - CLD_FALSE = Use GPIO pin specified by the
                                                         *                 vbus_en_port & vbus_en_pin
                                                         *                 parameters. */
    CLD_Boolean     vbus_ctrl_open_drain;               /*!< When use_built_in_vbus_ctrl = CLD_TRUE this
                                                         *   parameter selects if VBC is configured as open drain. */
    CLD_Boolean     vbus_ctrl_inverted;                 /*!< Selects the polarity of the Vbus control.
                                                         *   - CLD_TRUE = Vbus enable is active high
                                                         *   - CLD_FALSE = Vbus enable is active low */
    CLD_GPIO_Port   vbus_en_port;                       /*!< When use_built_in_vbus_ctrl = CLD_FALSE this
                                                         *   parameter selects GPIO port used to control
                                                         *   Vbus. */
    CLD_GPIO_PIN    vbus_en_pin;                        /*!< When use_built_in_vbus_ctrl = CLD_FALSE this
                                                         *   parameter selects GPIO pin used to control
                                                         *   Vbus. */
    /**
     * Function called when one of the defined USB events occurs.
     * @param event - Defined USB Event
     *                              - CLD_USB_ENUMERATED_CONFIGURED_HS
     *                              - CLD_USB_ENUMERATED_CONFIGURED_FS
     *                              - CLD_USB_MSD_DISCONNECTED
     */
    void (*fp_cld_usb_event_callback) (CLD_USB_Event event);

     /**
         * Function called when the library has a status to report.
          * @param status_code - Status code returned by the CLD library using the fp_cld_lib_status callback function.
          * @param p_additional_data - optional additional data returned by the CLD library using the fp_cld_lib_status callback function.
          * @param additional_data_size - optional additional data size returned by the CLD library using the fp_cld_lib_status callback function.
         */
    void (*fp_cld_lib_status) (unsigned short status_code, void * p_additional_data, unsigned short additional_data_size);

} CLD_SC5xx_MSD_Host_Lib_Init_Params;

/** CLD Library function that should be called once every 125 microseconds. */
extern void cld_time_125us_tick (void);
/** CLD MSD USB Host USB interrupt service routine processing functions */
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
 * Initializes the CLD MSD USB Host library. This function needs to be called until
 * CLD_SUCCESS or CLD_FAIL is returned.  If CLD_FAIL is returned there was a
 * problem initializing the library.
 * @param p_lib_params - Pointer to the CLD library initialization parameters,
 * @return The status of the library initialization (CLD_SUCCESS, CLD_FAIL, CLD_ONGOING).
 */
extern CLD_RV cld_sc5xx_msd_host_lib_init (CLD_SC5xx_MSD_Host_Lib_Init_Params * p_lib_params);

/**
 * CLD MSD USB Host library mainline function. <b>The CLD library mainline function is
 * required and should be called in each iteration of the main program loop.</b>
 */
extern void cld_sc5xx_msd_host_lib_main (void);

/**
 * cld_sc5xx_msd_host_lib_send_command initiates a MSD Bulk I/O reqeust.
 * @param cmd - MSD command to process.
 * @param cmd_params - command specific parameters for the command selected by cmd.
 * @param data_transport_size - Max number of bytes expected in the Data Transport stage of the
 *                              command.
 * @param p_data_transport - Memory address to source/store the data transfered during the data
 *                           transport stage.
 * @param fp_cmd_successful_callback - function called if the command was completed successfully.
 * @param fp_cmd_failed_callback - function called if the command failed.  THis function is passed
 *                                 the reason the command failed reported in the Command Status
 *                                 Wrapper.
 * @return CLD_SUCCESS - request scheduled, CLD_FAIL - Device not attached, or command parameter is
 *         invalid.
 */
extern CLD_RV cld_sc5xx_msd_host_lib_send_command (CLD_SC5xx_MSD_Host_Cmd_Params * p_params);

/**
 * cld_sc5xx_msd_host_lib_abort_command aborts and active MSD Bulk I/O reqeust initiated by the
 * cld_sc5xx_msd_host_lib_send_command function.  It is recommended to issue a Bulk-Only reset
 * by calling cld_sc5xx_msd_host_lib_bulk_only_reset after aborting a command to re-sync with
 * the attached thumb drive.
 */
extern void cld_sc5xx_msd_host_lib_abort_command (void);

/**
 * cld_sc5xx_msd_host_lib_get_max_lun initiates a Get Max Lun control endpoint request.
 * @param p_max_lun - Pointer to the memory to store the Max Lun returned by the attached device.
 * @param fp_cmd_successful_callback - function called when Max Lun is received.
 * @param fp_cmd_failed_callback - function called if the request failed (stalled).
 * @return CLD_SUCCESS - request scheduled, CLD_FAIL - Device not attached, or p_max_lun = NULL.
 */
extern CLD_RV cld_sc5xx_msd_host_lib_get_max_lun (CLD_SC5xx_MSD_Host_Get_Max_Lun_Params * p_params);

/**
 * cld_sc5xx_msd_host_lib_bulk_only_reset initiates a Bulk-Only Mass Storage Reset control endpoint
 * request.
  @return CLD_SUCCESS - request scheduled, CLD_FAIL - Device not attached.
 */
extern CLD_RV cld_sc5xx_msd_host_lib_bulk_only_reset (CLD_SC5xx_MSD_Host_Bulk_Only_Reset_Params * p_params);


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
#endif  /* __CLD_MSD_HOST_LIB__ */
