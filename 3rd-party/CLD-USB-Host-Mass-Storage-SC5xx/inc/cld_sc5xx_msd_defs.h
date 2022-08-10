#ifndef __CLD_MSD_DEFS__
#define __CLD_MSD_DEFS__
/*=============================================================================
    FILE:           cld_sc5xx_msd_defs.h

    DESCRIPTION:    CLD Mass Storage Device (Bulk Only) USB Host Mass Storage
                    Class message structures.

                    For more information on the command parameters
                    refer to the USB Mass Storage UFI Command Specification or SCSI Reduced
                    Block Commands documentation

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
 * @file      cld_sc5xx_msd_defs.h
 * @brief     CLD Mass Storage Device (Bulk Only) USB Host.
 *
 * @details
 *            This file contains USB Mass Storage Device message structures used with the
 *            CLD USB MSD USB Host library.
 *
 *            For more information on the command parameters
 *            refer to the USB Mass Storage UFI Command Specification or SCSI Reduced
 *            Block Commands documentation
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


#if defined(_LANGUAGE_C) || defined(__cplusplus) || (defined(__GNUC__) && !defined(__ASSEMBLER__))

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * CLD library MSD SCSI Inquiry command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  page_code;
    unsigned char  allocation_length;
} CLD_MSD_Command_Inquiry;

/**
 * CLD library MSD SCSI Mode Select 6-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  parameter_list_length;
} CLD_MSD_Command_Mode_Select_6;

/**
 * CLD library MSD SCSI Mode Select 10-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned short parameter_list_length;
} CLD_MSD_Command_Mode_Select_10;

/**
 * CLD library MSD SCSI Mode Sense 6-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  page_code;
    unsigned char  page_control;
    unsigned char  allocation_length;
} CLD_MSD_Command_Mode_Sense_6;

/**
 * CLD library MSD SCSI Mode Sense 10-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  page_code;
    unsigned char  page_control;
    unsigned short allocation_length;
} CLD_MSD_Command_Mode_Sense_10;

/**
 * CLD library MSD SCSI Prevent/Allow Removal command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  prevent;
} CLD_MSD_Command_Prevent_Allow_Removal;

/**
 * CLD library MSD SCSI Read 10-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned long  logical_block_address;
    unsigned short transfer_length;
} CLD_MSD_Command_Read_10;

/**
 * CLD library MSD SCSI Read 12-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned long  logical_block_address;
    unsigned long  transfer_length;
} CLD_MSD_Command_Read_12;

/**
 * CLD library MSD SCSI Read Capasity command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned long  logical_block_address;
} CLD_MSD_Command_Read_Capasity;

/**
 * CLD library MSD SCSI Read Capasity response data.
 */
typedef struct
{
    unsigned long last_logical_block_address;
    unsigned long block_length;
} CLD_MSD_Read_Capasity_Response;

/**
 * CLD library MSD SCSI Read Format Capasities command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned short allocation_length;
} CLD_MSD_Command_Read_Format_Capasities;

/**
 * CLD library MSD SCSI Request Sense command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned short allocation_length;
} CLD_MSD_Command_Request_Sense;

/**
 * CLD library MSD SCSI Send Diagnostic command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  self_test;
} CLD_MSD_Command_Send_Diagnostic;

/**
 * CLD library MSD SCSI Start/Stop Unit command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned char  start;
    unsigned char  load_eject;
} CLD_MSD_Command_Start_Stop_Unit;

/**
 * CLD library MSD SCSI Test Unit Ready command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
} CLD_MSD_Command_Test_Unit_Ready;

/**
 * CLD library MSD SCSI Verify command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned long  logical_block_address;
    unsigned short verification_length;
} CLD_MSD_Command_Verify;

/**
 * CLD library MSD SCSI Write 10-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned long  logical_block_address;
    unsigned short transfer_length;
} CLD_MSD_Command_Write_10;

/**
 * CLD library MSD SCSI Write 12-byte command parameters.
 */
typedef struct
{
    unsigned char  logical_unit_num;
    unsigned long  logical_block_address;
    unsigned long  transfer_length;
} CLD_MSD_Command_Write_12;

typedef union
{
    CLD_MSD_Command_Inquiry                 inquriy;
    CLD_MSD_Command_Mode_Select_6           mode_select_6;
    CLD_MSD_Command_Mode_Select_6           mode_select_10;
    CLD_MSD_Command_Mode_Sense_6            mode_sense_6;
    CLD_MSD_Command_Mode_Sense_10           mode_sense_10;
    CLD_MSD_Command_Prevent_Allow_Removal   prevent_allow_removal;
    CLD_MSD_Command_Read_10                 read_10;
    CLD_MSD_Command_Read_12                 read_12;
    CLD_MSD_Command_Read_Capasity           read_capasity;
    CLD_MSD_Command_Read_Format_Capasities  read_format_capasities;
    CLD_MSD_Command_Request_Sense           request_sense;
    CLD_MSD_Command_Send_Diagnostic         send_diagnostic;
    CLD_MSD_Command_Start_Stop_Unit         start_stop;
    CLD_MSD_Command_Test_Unit_Ready         test_unit_ready;
    CLD_MSD_Command_Verify                  verify;
    CLD_MSD_Command_Write_10                write_10;
    CLD_MSD_Command_Write_12                write_12;
    CLD_MSD_Command_Write_10                write_and_verify;
} CLD_SC5xx_MSD_Command_Parameters;


#ifdef __cplusplus
}
#endif
#endif /* defined(_LANGUAGE_C) || defined(__cplusplus) || (defined(__GNUC__) && !defined(__ASSEMBLER__)) */
#endif /* __CLD_MSD_DEFS__ */
