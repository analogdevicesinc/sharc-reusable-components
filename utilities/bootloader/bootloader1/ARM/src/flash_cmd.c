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

/* Standard system includes */
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Flash driver includes */
#include "drivers/flash/flash.h"

/* Flash map includes */
#include "flash_map.h"

/* CLD library includes */
#include "cld_sc58x_cdc_lib.h"

/* Flash command includes */
#include "slip.h"
#include "flash_cmd.h"
#include "cdc/user_cdc.h"

#define ERASE_SECTOR_SIZE            (4 * 1024)

#define APPLICATION_BASE_ADDR        (APP_OFFSET)
#define APPLICATION_ERASE_SECTORS    (APP_SIZE / ERASE_SECTOR_SIZE)

uint8_t cmd[512];
uint8_t resp[512];

enum {
    PROGRAM_STATE_IDLE = 0,
    PROGRAM_STATE_ERASING,
    PROGRAM_PROGRAMMING
};

extern FLASH_INFO mt25q_info;
static FLASH_INFO *fi = &mt25q_info;

uint8_t state = PROGRAM_STATE_IDLE;
uint32_t erase_sector = 0;
uint32_t max_erase_sector = 0;

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
 * End CMD Interface
 **********************************************************************/
void xfer(void *blob, uint16_t blobLen)
{
    uint16_t xfer_len;

    xfer_len = slip((uint8_t *)blob, blobLen, resp, sizeof(resp));
    user_cdc_tx_serial_data(xfer_len, resp);
}

void program_flash(CMD_PROGRAM *program)
{
    CMD_PROGRAM_ACK program_ack;
    int ok;

    ok = flash_program(fi, APPLICATION_BASE_ADDR + program->address,
        program->data, program->length);

    memset(&program_ack, 0, sizeof(program_ack));
    program_ack.header.cmd = CMD_ID_PROGRAM_ACK;
    program_ack.address = program->address;
    program_ack.ok = (ok == FLASH_OK);

    xfer(&program_ack, sizeof(program_ack));
}

void process_command(MSG_HEADER *msg, uint16_t len)
{
    CMD_NOP nop;
    static int programming = 0;

    //printf("Cmd: %d, Len: %d\n", msg->cmd, msg->len);

    switch (msg->cmd) {
        case CMD_ID_NOP:
            printf("Connected...\n");
            memset(&nop, 0, sizeof(nop));
            nop.header.cmd = CMD_ID_NOP;
            xfer(&nop, sizeof(nop));
            break;
        case CMD_ID_ERASE_APPLICATION:
            printf("Erasing application...\n");
            erase_sector = 0;
            max_erase_sector = APPLICATION_ERASE_SECTORS - 1;
            state = PROGRAM_STATE_ERASING;
            programming = 0;
            break;
        case CMD_ID_PROGRAM:
            if (programming == 0) {
                printf("Programming application...\n");
                programming = 1;
            }
            program_flash((CMD_PROGRAM *)msg);
            break;
        default:
            break;
    }
}

void send_erase_status(uint32_t erase_sector, uint32_t max_sector)
{
    CMD_ERASE_STATUS erase_status;

    memset(&erase_status, 0, sizeof(erase_status));

    erase_status.header.cmd = CMD_ID_ERASE_STATUS;
    erase_status.header.len = sizeof(erase_status);
    erase_status.current = erase_sector;
    erase_status.max = max_sector;

    xfer(&erase_status, sizeof(erase_status));
}

void decode_frame(unsigned char *buf, unsigned short len)
{
    int frameReady;
    int bufOffset;
    int cmdOffset;

    bufOffset = 0;
    cmdOffset = 0;
    while (bufOffset < len) {
        frameReady = unslip(buf, len, &bufOffset, cmd, 512, &cmdOffset);
        if (frameReady) {
            process_command((MSG_HEADER *)cmd, cmdOffset);
            cmdOffset = 0;
        }
    }
}

void flash_cmd_main(void)
{
    int ok;

    if (xfer_state == XFER_STATE_DATA_READY) {
        decode_frame(user_cdc_serial_data_rx_buffer, user_cdc_serial_data_num_rx_bytes);
        xfer_state = XFER_STATE_IDLE;
        if (paused) {
            paused = false;
            cld_sc58x_cdc_lib_resume_paused_serial_data_transfer();
        }
    }

    switch (state) {
        case PROGRAM_STATE_IDLE:
            break;
        case PROGRAM_STATE_ERASING:
            ok = flash_erase(fi, APPLICATION_BASE_ADDR + erase_sector * ERASE_SECTOR_SIZE, ERASE_SECTOR_SIZE);
            send_erase_status(erase_sector, max_erase_sector);
            if (erase_sector == max_erase_sector) {
                state = PROGRAM_STATE_IDLE;
                erase_sector = 0;
                max_erase_sector = 0;
            } else {
                erase_sector++;
            }
            break;
        default:
            break;
    }
}
