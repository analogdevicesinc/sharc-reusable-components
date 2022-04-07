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
#define WINBOND_MFG_ID                    0xEF

/* Instruction Set utilized by this driver */
#define CMD_WRITE_ENABLE                  0x06
#define CMD_WRITE_DISABLE                 0x04
#define CMD_READ_ID                       0x9F
#define CMD_READ_STATUS_REGISTER_1        0x05
#define CMD_READ_STATUS_REGISTER_2        0x35
#define CMD_READ_STATUS_REGISTER_3        0x15
#define CMD_WRITE_ENABLE_VOLATILE_STATUS  0x50
#define CMD_WRITE_STATUS_REGISTER_1       0x01
#define CMD_WRITE_STATUS_REGISTER_2       0x31
#define CMD_WRITE_STATUS_REGISTER_3       0x11
#define CMD_FAST_READ_QUAD_OUTPUT         0x6b
#define CMD_SECTOR_ERASE_4K               0x20
#define CMD_QUAD_PAGE_PROGRAM             0x32

/* Status Register-1 bits */
#define STATUS_REGISTER_1_BUSY            0x01

/* Status Register-2 bits */
#define STATUS_REGISTER_2_QE              0x02

/* Status Register-3 bits */
#define STATUS_REGISTER_3_DRV0            0x20
#define STATUS_REGISTER_3_DRV1            0x40

/* Write size */
#define WRITE_PAGE_SIZE                   256

/* Erase sizes and commands */
#define ERASE_CMD                        (CMD_SECTOR_ERASE_4K)
#define ERASE_SECTOR_SIZE                (4*1024)

static int w25q128fv_write_cmd(const FLASH_INFO *fi, uint8_t cmd)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t tx[1];

    result = FLASH_OK;

    tx[0] = cmd;

    spiResult = spi_xfer(fi->flashHandle, 1, NULL, tx);
    if (spiResult != SPI_SIMPLE_SUCCESS) {
        result = FLASH_ERROR;
    }

    return(result);
}

static int w25q128fv_write_volatile_status(const FLASH_INFO *fi,
    uint8_t reg, uint8_t value)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t tx[2];

    result = w25q128fv_write_cmd(fi, CMD_WRITE_ENABLE_VOLATILE_STATUS);

    if (result == FLASH_OK) {

        tx[0] = reg;
        tx[1] = value;

        spiResult = spi_xfer(fi->flashHandle, 2, NULL, tx);
        if (spiResult != SPI_SIMPLE_SUCCESS) {
            result = FLASH_ERROR;
        }
    }

    return(result);
}

static int w25q128fv_init(const FLASH_INFO *fi)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t rx[4];
    uint8_t tx[4];

    result = FLASH_OK;

    /* Make sure it's a Winbond device */
    tx[0] = CMD_READ_ID;

    spiResult = spi_xfer(fi->flashHandle, 4, rx, tx);
    if ((spiResult != SPI_SIMPLE_SUCCESS) || (rx[1] != WINBOND_MFG_ID)) {
        result = FLASH_ERROR;
    }

    if (result == FLASH_OK) {
        /* Status Register-1: default */
        result = w25q128fv_write_volatile_status(fi,
            CMD_WRITE_STATUS_REGISTER_1, 0x00);
    }

    if (result == FLASH_OK) {
        /* Status Register-2: default except enable QSPI mode */
        result = w25q128fv_write_volatile_status(fi,
            CMD_WRITE_STATUS_REGISTER_2, STATUS_REGISTER_2_QE);
    }

    if (result == FLASH_OK) {
        /* Status Register-3: default except 100% drive strength */
        result = w25q128fv_write_volatile_status(fi,
            CMD_WRITE_STATUS_REGISTER_3, 0x00);
    }

    return(result);
}

static int w25q128fv_read_status(const FLASH_INFO *fi,
    uint8_t reg, uint8_t *value)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t rx[2];
    uint8_t tx[2];

    result = FLASH_OK;

    tx[0] = reg;
    tx[1] = 0x00;

    spiResult = spi_xfer(fi->flashHandle, 2, rx, tx);
    if (spiResult != SPI_SIMPLE_SUCCESS) {
        result = FLASH_ERROR;
    }

    if (result == FLASH_OK) {
        *value = rx[1];
    }

    return(result);
}

static int w25q128fv_wait_ready(const FLASH_INFO *fi)
{
    int result;
    uint8_t status;

    result = FLASH_OK;

    do {
        result = w25q128fv_read_status(fi, CMD_READ_STATUS_REGISTER_1, &status);
        if (result != FLASH_OK) {
            break;
        }
    } while (status & STATUS_REGISTER_1_BUSY);

    return(result);
}

static int w25q128fv_write_enable(const FLASH_INFO *fi)
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

static int w25q128fv_write_disable(const FLASH_INFO *fi)
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

static int w25q128fv_read(const FLASH_INFO *fi, uint32_t addr, uint8_t *buf, int size)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    sSPIXfer spiMultiXfer[2];
    uint8_t cmd[5];

    result = FLASH_OK;

    /* Clear the SPI transfer struct */
    memset(&spiMultiXfer, 0, sizeof(spiMultiXfer));

    /* Configure the "Fast Read Quad Output" command with 8 dummy cycles */
    cmd[0] = CMD_FAST_READ_QUAD_OUTPUT;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = (addr >> 0) & 0xFF;
    cmd[4] = 0x00;

    spiMultiXfer[0].tx = cmd;
    spiMultiXfer[0].len = 5;
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

static int w25q128fv_erase(const FLASH_INFO *fi, uint32_t addr, int size)
{
    int result;
    SPI_SIMPLE_RESULT spiResult;

    uint8_t cmd[4];

    /* Align to the start of the ERASE_SECTOR_SIZE boundary */
    addr -= addr % ERASE_SECTOR_SIZE;
    size += addr % ERASE_SECTOR_SIZE;

    result = FLASH_OK;

    while (size > 0) {

        /* Set write enable */
        result = w25q128fv_write_enable(fi);
        if (result != FLASH_OK) {
            break;
        }

        /* Erase the sector */
        cmd[0] = ERASE_CMD;
        cmd[1] = (addr >> 16) & 0xFF;
        cmd[2] = (addr >> 8) & 0xFF;
        cmd[3] = (addr >> 0) & 0xFF;
        spiResult = spi_xfer(fi->flashHandle, 4, NULL, cmd);
        if (spiResult != SPI_SIMPLE_SUCCESS) {
            result = FLASH_ERROR;
            break;
        }

        /* Poll for complete */
        result = w25q128fv_wait_ready(fi);
        if (result != FLASH_OK) {
            break;
        }

        addr += ERASE_SECTOR_SIZE;
        size -= ERASE_SECTOR_SIZE;
    }

    return(result);
}

static int w25q128fv_program(const FLASH_INFO *fi,
        uint32_t addr, const uint8_t *buf, int size)
{
    int psize;

    int result;
    SPI_SIMPLE_RESULT spiResult;

    sSPIXfer spiMultiXfer[2];
    uint8_t cmd[4];

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
        result = w25q128fv_write_enable(fi);
        if (result != FLASH_OK) {
            break;
        }

        /* Configure the CMD_QUAD_PAGE_PROGRAM command */
        cmd[0] = CMD_QUAD_PAGE_PROGRAM;
        cmd[1] = (addr >> 16) & 0xFF;
        cmd[2] = (addr >> 8) & 0xFF;
        cmd[3] = (addr >> 0) & 0xFF;

        spiMultiXfer[0].tx = cmd;
        spiMultiXfer[0].len = 4;
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
        result = w25q128fv_wait_ready(fi);
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

FLASH_INFO w25q128fv_info =
{
    .flashHandle = NULL,
    .flash_read = w25q128fv_read,
    .flash_erase = w25q128fv_erase,
    .flash_program = w25q128fv_program
};


FLASH_INFO *w25q128fv_open(sSPIPeriph *spiFlashHandle)
{
    FLASH_INFO *handle = NULL;
    int result;

    w25q128fv_info.flashHandle = spiFlashHandle;
    handle = &w25q128fv_info;

    result = w25q128fv_init(handle);

    if (result != FLASH_OK) {
        handle = NULL;
    }

    return(handle);
}

int w25q128fv_close(const FLASH_INFO *fi)
{
    return(FLASH_OK);
}
