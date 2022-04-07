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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "flash_cmd.h"
#include "serial.h"
#include "slip.h"
#include "util.h"

#define RESP_TIMEOUT_NS  (5000000000)

/***********************************************************************
 * Command Variables
 **********************************************************************/
unsigned char cmd[512];
unsigned char resp[512] __attribute__ ((aligned(4)));

int inLen = 0;
int inOffset = 0;
int respOffset = 0;
unsigned char in[512];

/***********************************************************************
 * Command Functions
 **********************************************************************/
static int xfer(ser_handle serial, void *blob, int blobLen)
{
    unsigned char sBuf[512];
    int xferLen;
    int ret;

    xferLen = slip((unsigned char *)blob, blobLen, sBuf, sizeof(sBuf));
    ret = ser_write(serial, sBuf, xferLen);

    return(ret);
}

static int waitForResp(ser_handle serial)
{
    int frameReady;
    int timeout;
    int readable;
    struct timespec start, now;
    uint64_t elapsed;

    timeout = 0;
    frameReady = 0;
    clock_gettime(0, &start);

    do {

        /* Service incoming characters */
        if (inOffset == inLen) {
            readable = ser_readable(serial);
            if (readable > 0) {
                if (readable > sizeof(in)) {
                    readable = sizeof(in);
                }
                inLen = ser_read(serial, in, readable);
                inOffset = 0;
            }
        } else {
            frameReady = unslip(in, inLen, &inOffset, resp, sizeof(resp), &respOffset);
            if (frameReady) {
                respOffset = 0;
            }
        }

        /* Check timeout */
        clock_gettime(0, &now);
        elapsed = clock_elapsed(&start, &now);
        if (elapsed > RESP_TIMEOUT_NS) {
            timeout = 1;
        }


    } while (!frameReady && !timeout);

    return(frameReady);
}

int waitfor(ser_handle serial, uint8_t cmd)
{
    int frameReady;
    int ok;
    MSG_HEADER *header;

    ok = 0;
    frameReady = waitForResp(serial);
    if (frameReady) {
        header = (MSG_HEADER *)resp;
        if (header->cmd == cmd) {
            ok = 1;
        }
    }

    return(ok);
}

int send_nop(ser_handle serial)
{
    CMD_NOP nop;
    int ret;

    memset(&nop, 0, sizeof(nop));

    nop.header.cmd = CMD_ID_NOP;
    ret = xfer(serial, &nop, sizeof(nop));

    return(ret);
}

int send_erase_application(ser_handle serial)
{
    CMD_ERASE_APPLICATION erase;
    int ret;

    memset(&erase, 0, sizeof(erase));

    erase.header.cmd = CMD_ID_ERASE_APPLICATION;
    ret = xfer(serial, &erase, sizeof(erase));

    return(ret);
}

int send_program(ser_handle serial, int address, unsigned char *buf, int len)
{
    CMD_PROGRAM program;
    int ret;

    memset(&program, 0, sizeof(program));

    program.header.cmd = CMD_ID_PROGRAM;
    program.header.len = sizeof(program) - sizeof(program.data) + len;

    program.address = address;
    program.length = len;
    memcpy(program.data, buf, len);

    ret = xfer(serial, &program, program.header.len);

    return(ret);
}

int process_erase_status(int *percent)
{
    CMD_ERASE_STATUS *erase_status = (CMD_ERASE_STATUS *)resp;

    if (erase_status->max != 0) {
        *percent = ((int)erase_status->current * 100) / (int)erase_status->max;
    } else {
        *percent = 0;
    }

    return(erase_status->current == erase_status->max);
}

int process_program_ack(uint32_t address)
{
    CMD_PROGRAM_ACK *program_ack = (CMD_PROGRAM_ACK *)resp;

    return((program_ack->address == address && program_ack->ok));
}
