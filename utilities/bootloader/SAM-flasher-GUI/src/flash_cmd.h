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

#ifndef _flash_cmd_h
#define _flash_cmd_h

#include <stdint.h>

#include "serial.h"

/***********************************************************************
 * Start CMD Interface
 **********************************************************************/
enum {
    CMD_ID_NOP = 0,
    CMD_ID_ERASE_APPLICATION,
    CMD_ID_ERASE_STATUS,
    CMD_ID_PROGRAM,
    CMD_ID_PROGRAM_ACK,
    CMD_ID_RESET
};

struct _msg_header {
    uint8_t cmd;
    uint16_t len;
    uint8_t reserved;
} __attribute__((__packed__));
typedef struct _msg_header MSG_HEADER;

struct _cmd_nop {
    MSG_HEADER header;
} __attribute__((__packed__));
typedef struct _cmd_nop CMD_NOP;

struct _cmd_erase_application {
    MSG_HEADER header;
} __attribute__((__packed__));
typedef struct _cmd_erase_application CMD_ERASE_APPLICATION;

struct _cmd_erase_status {
    MSG_HEADER header;
    uint32_t current;
    uint32_t max;
} __attribute__((__packed__));
typedef struct _cmd_erase_status CMD_ERASE_STATUS;

struct _cmd_program {
    MSG_HEADER header;
    uint32_t address;
    uint32_t length;
    uint8_t data[256];
}  __attribute__((__packed__));
typedef struct _cmd_program CMD_PROGRAM;

struct _cmd_program_ack {
    MSG_HEADER header;
    uint32_t address;
    uint8_t ok;
} __attribute__((__packed__));
typedef struct _cmd_program_ack CMD_PROGRAM_ACK;

struct _cmd_reset {
    MSG_HEADER header;
} __attribute__((__packed__));
typedef struct _cmd_reset CMD_RESET;

/***********************************************************************
 * Start CMD Functions
 **********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

int waitfor(ser_handle serial, uint8_t cmd);
int send_nop(ser_handle serial);
int send_erase_application(ser_handle serial);
int send_program(ser_handle serial, int address, unsigned char *buf, int len);
int process_erase_status(int *percent);
int process_program_ack(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif
