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

#include <stdlib.h>
#include <string.h>

#include "flash.h"

/* Device Info */
#define SPANSION_MFG_ID         (0x01)

/* Instruction Set utilized by this driver */
#define CMD_READ_ID                         0x9F // NA
#define CMD_4_BYTE_READ_QUAD_OUT            0x6C // 4b
#define CMD_WRITE_ENABLE                    0x06 // NA
#define CMD_WRITE_DISABLE                   0x04 // NA
#define CMD_4_BYTE_SECTOR_ERASE             0xDC // 4b
#define CMD_BANK_REGISTER_WRITE             0x17 // NA
#define CMD_READ_STATUS_REGISTER1           0x05 // NA
#define CMD_4_BYTE_PAGE_PROGRAM             0x34 // 4b
#define CMD_READ_CONFIG_REGISTER            0x35 // NA
#define CMD_WRITE_REGISTER                  0x01 // NA

/* Configuration Register 1 bits */
#define CR1_LC1                 (0x80)
#define CR1_LC0                 (0x40)
#define CR1_QUAD                (0x02)

/* Status register bits */
#define WRITE_IN_PROGRESS       (0x01)

/* Write size */
#define WRITE_PAGE_SIZE         (512)

/* Erase sizes and commands */
#define ERASE_CMD               (CMD_4_BYTE_SECTOR_ERASE)
#define ERASE_SECTOR_SIZE       (256*1024)

static int s25fl512s_write_enable(const FLASH_INFO *fi);
static int s25fl512s_wait_ready(const FLASH_INFO *fi);

static int s25fl512s_init(const FLASH_INFO *fi)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t rx[4];
    uint8_t tx[4];

    uint8_t CR1;
    uint8_t SR1;

    result = FLASH_OK;

    /* Make sure it's a Cypress device */
    tx[0] = CMD_READ_ID;
    spiResult = spi_xfer(fi->flashHandle, 4, rx, tx);

    if ((spiResult != SPI_SIMPLE_SUCCESS) || (rx[1] != SPANSION_MFG_ID)) {
        result = FLASH_ERROR;
    }

    /* Clear LC and set QUAD nonvolatile CR1 bits if necessary */
    if (result == FLASH_OK) {
        tx[0] = CMD_READ_CONFIG_REGISTER;
        tx[1] = 0x00;
        spiResult = spi_xfer(fi->flashHandle, 2, rx, tx);
        if (spiResult != SPI_SIMPLE_SUCCESS) {
            result = FLASH_ERROR;
        }
        CR1 = rx[1];
    }
    if ((CR1 & (CR1_LC0 | CR1_LC1 | CR1_QUAD)) != CR1_QUAD) {
        /* Clear the LC bits */
        CR1 &= ~(CR1_LC0 | CR1_LC1);
        /* Set the QUAD bits */
        CR1 |= CR1_QUAD;
        /* Read Status Register-1 */
        if (result == FLASH_OK) {
            tx[0] = CMD_READ_STATUS_REGISTER1;
            tx[1] = 0x00;
            spiResult = spi_xfer(fi->flashHandle, 2, rx, tx);
            if (spiResult != SPI_SIMPLE_SUCCESS) {
                result = FLASH_ERROR;
            }
            SR1 = rx[1];
        }
        /* Write CR1 (must also write SR1) */
        if (result == FLASH_OK) {
            result = s25fl512s_write_enable(fi);
            if (result == FLASH_OK) {
                tx[0] = CMD_WRITE_REGISTER;
                tx[1] = SR1;
                tx[2] = CR1;
                spiResult = spi_xfer(fi->flashHandle, 3, NULL, tx);
                if (spiResult != SPI_SIMPLE_SUCCESS) {
                    result = FLASH_ERROR;
                }
                if (result == FLASH_OK) {
                    result = s25fl512s_wait_ready(fi);
                }
            }
        }
    }

    return(result);
}

static int s25fl512s_write_enable(const FLASH_INFO *fi)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t tx[1];

    result = FLASH_OK;

    tx[0] = CMD_WRITE_ENABLE;
    spiResult = spi_xfer(fi->flashHandle, 1, NULL, tx);

    if (spiResult != SPI_SIMPLE_SUCCESS) {
        result = FLASH_ERROR;
    }

    return(result);
}

static int s25fl512s_write_disable(const FLASH_INFO *fi)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t tx[1];

    result = FLASH_OK;

    tx[0] = CMD_WRITE_DISABLE;
    spiResult = spi_xfer(fi->flashHandle, 1, NULL, tx);

    if (spiResult != SPI_SIMPLE_SUCCESS) {
        result = FLASH_ERROR;
    }

    return(result);
}

static int s25fl512s_wait_ready(const FLASH_INFO *fi)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t cmd[2];
    uint8_t status[2];

    result = FLASH_OK;

    cmd[0] = CMD_READ_STATUS_REGISTER1;
    do {
        spiResult = spi_xfer(fi->flashHandle, 2, status, cmd);
        if (spiResult != SPI_SIMPLE_SUCCESS) {
            result = FLASH_ERROR;
            break;
        }
    } while (status[1] & WRITE_IN_PROGRESS);

    return(result);
}

static int s25fl512s_read(const FLASH_INFO *fi, uint32_t addr, uint8_t *buf, int size)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    sSPIXfer spiMultiXfer[2];
    uint8_t cmd[6];

    result = FLASH_OK;

    /* Clear the SPI transfer struct */
    memset(&spiMultiXfer, 0, sizeof(spiMultiXfer));

    /* Configure the CMD_4_BYTE_READ_QUAD_OUT command with 8 dummy cycles */
    cmd[0] = CMD_4_BYTE_READ_QUAD_OUT;
    cmd[1] = (addr >> 24) & 0xFF;
    cmd[2] = (addr >> 16) & 0xFF;
    cmd[3] = (addr >> 8) & 0xFF;
    cmd[4] = (addr >> 0) & 0xFF;
    cmd[5] = 0x00;

    spiMultiXfer[0].tx = cmd;
    spiMultiXfer[0].len = 6;
    spiMultiXfer[0].flags = SPI_SIMPLE_XFER_NORMAL_IO;

    /* Configure the data transfer */
    spiMultiXfer[1].rx = buf;
    spiMultiXfer[1].len = size;
    spiMultiXfer[1].flags = SPI_SIMPLE_XFER_QUAD_IO;

    /* Read the data */
    spiResult = spi_batch_xfer(fi->flashHandle, 2, spiMultiXfer);

    if (spiResult != SPI_SIMPLE_SUCCESS) {
        result = FLASH_ERROR;
    }

    return(result);
}

static int s25fl512s_erase(const FLASH_INFO *fi, uint32_t addr, int size)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t cmd[5];

    /* Align to the start of the ERASE_SECTOR_SIZE boundary */
    addr -= addr % ERASE_SECTOR_SIZE;
    size += addr % ERASE_SECTOR_SIZE;

    result = FLASH_OK;

    while (size > 0) {

        /* Set write enable */
        result = s25fl512s_write_enable(fi);
        if (result != FLASH_OK) {
            break;
        }

        /* Erase the sector */
        cmd[0] = ERASE_CMD;
        cmd[1] = (addr >> 24) & 0xFF;
        cmd[2] = (addr >> 16) & 0xFF;
        cmd[3] = (addr >> 8) & 0xFF;
        cmd[4] = (addr >> 0) & 0xFF;
        spiResult = spi_xfer(fi->flashHandle, 5, NULL, cmd);
        if (spiResult != SPI_SIMPLE_SUCCESS) {
            result = FLASH_ERROR;
            break;
        }

        /* Poll for complete */
        result = s25fl512s_wait_ready(fi);
        if (result != FLASH_OK) {
            break;
        }

        addr += ERASE_SECTOR_SIZE;
        size -= ERASE_SECTOR_SIZE;
    }

    return(result);
}

static int s25fl512s_program(const FLASH_INFO *fi,
        uint32_t addr, const uint8_t *buf, int size)
{
    int psize;

    int result;
    SPI_SIMPLE_RESULT spiResult;

    sSPIXfer spiMultiXfer[2];
    uint8_t cmd[5];

    result = FLASH_OK;

    /* Clear the SPI transfer struct */
    memset(&spiMultiXfer, 0, sizeof(spiMultiXfer));

    while (size > 0) {

        /* Write at most up to the next page boundary */
        psize = WRITE_PAGE_SIZE - (addr % WRITE_PAGE_SIZE);
        if (size < psize) {
            psize = size;
        }

        /* Set write enable */
        result = s25fl512s_write_enable(fi);
        if (result != FLASH_OK) {
            break;
        }

        /* Configure the CMD_4_BYTE_PAGE_PROGRAM command */
        cmd[0] = CMD_4_BYTE_PAGE_PROGRAM;
        cmd[1] = (addr >> 24) & 0xFF;
        cmd[2] = (addr >> 16) & 0xFF;
        cmd[3] = (addr >> 8) & 0xFF;
        cmd[4] = (addr >> 0) & 0xFF;

        spiMultiXfer[0].tx = cmd;
        spiMultiXfer[0].len = 5;
        spiMultiXfer[0].flags = SPI_SIMPLE_XFER_NORMAL_IO;

        /* Configure the data transfer */
        spiMultiXfer[1].tx = (uint8_t *)buf;
        spiMultiXfer[1].len = psize;
        spiMultiXfer[1].flags = SPI_SIMPLE_XFER_QUAD_IO;

        /* Write the data */
        spiResult = spi_batch_xfer(fi->flashHandle, 2, spiMultiXfer);
        if (spiResult != SPI_SIMPLE_SUCCESS) {
            result = FLASH_ERROR;
            break;
        }

        /* Poll for complete */
        result = s25fl512s_wait_ready(fi);
        if (result != FLASH_OK) {
            break;
        }

        /* Increment the addresses and sizes */
        addr += psize;
        size -= psize;
        buf += psize;
    }

    return(result);
}

FLASH_INFO s25fl512s_info =
{
    .flashHandle = NULL,
    .flash_read = s25fl512s_read,
    .flash_erase = s25fl512s_erase,
    .flash_program = s25fl512s_program
};


FLASH_INFO *s25fl512s_open(sSPIPeriph *spiFlashHandle)
{
    FLASH_INFO *handle = NULL;
    int result;

    s25fl512s_info.flashHandle = spiFlashHandle;
    handle = &s25fl512s_info;

    result = s25fl512s_init(handle);

    if (result != FLASH_OK) {
        handle = NULL;
    }

    return(handle);
}

int s25fl512s_close(const FLASH_INFO *fi)
{
    return(FLASH_OK);
}
