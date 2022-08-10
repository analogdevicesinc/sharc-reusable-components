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

/***********************************************************************
 * CAUTION: Despite it's name, this module is not a driver.  Rather,
 *          it is a subset of code lifted from the SAM BareMetal
 *          SDK to discover a static SigmaStudio defined A2B bus
 *          configuration.
 ***********************************************************************/
/*
 * Bare-Metal ("BM") device driver for A2B.  This driver provides basic
 * support for the AD2425W chip on the SHARC Audio Module board.
 *
 */
#include <stdlib.h>
#include <string.h>

#include "ad2425w_cfg.h"
#include "ad2425w.h"
#include "util.h"

#ifndef AD2425_MALLOC
#define AD2425_MALLOC malloc
#endif
#ifndef AD2425_FREE
#define AD2425_FREE   free
#endif
#ifndef AD2425_MEMCPY
#define AD2425_MEMCPY memcpy
#endif
#ifndef AD2425_MEMSET
#define AD2425_MEMSET memset
#endif

// IRQ tracking bits
enum {
    AD2425W_IRQ_NONE = 0,
    AD2425W_IRQ_PLL_LOCK_PENDING = 1,
    AD2425W_IRQ_DISCOVERY_PENDING = 2,
};

// IRQ registers
struct INT_REGS {
    uint8_t intSrc;
    uint8_t intType;
};

/* Line structure - must match that found in adi_a2b_i2c_commandlist.h */
typedef struct _ADI_A2B_DISCOVERY_CONFIG {
    unsigned char i2c_addr;          // nDeviceAddr;
    unsigned char cmd;               // eOpCode;
    unsigned char addr_width;        // nAddrWidth;
    unsigned int addr;               // nAddr;
    unsigned char data_width;        // nDataWidth;
    unsigned short data_count;       // nDataCount;
    unsigned char *config_data;      //paConfigData;
} ADI_A2B_DISCOVERY_CONFIG;

// A2B init result strings
struct RESULT_STRING {
    BM_AD2425W_RESULT result;
    const char *string;
};

// Writes a control register in the AD2425W
static BM_AD2425W_RESULT ad2425w_write_ctrl_reg(BM_AD2425W_CONTROLLER *ad2425w,
    uint8_t reg, uint8_t value, uint8_t i2c_addr);

// Writes a block of data to the AD2425W - used to initialize remote I2C devices
static BM_AD2425W_RESULT ad2425w_write_ctrl_reg_block(BM_AD2425W_CONTROLLER *ad2425w,
    uint32_t addr, uint8_t addr_bytes,
    uint16_t len, uint8_t *values, uint8_t i2c_addr);

// Reads a control register in the AD2425W
static BM_AD2425W_RESULT ad2425w_read_ctrl_reg(BM_AD2425W_CONTROLLER *ad2425w,
    uint8_t reg, uint8_t *value, uint8_t i2c_addr);

// Writes a block of control registers from the AD2425W
static BM_AD2425W_RESULT ad2425w_read_ctrl_reg_block(BM_AD2425W_CONTROLLER *ad2425w,
    uint8_t reg, uint16_t len, uint8_t *values,
    uint8_t i2c_addr);

// A2B Result strings
const struct RESULT_STRING RESULT_STRINGS[] =  {
    { AD2425W_SIMPLE_SUCCESS, "SUCCESS" },
    { AD2425W_A2B_BUS_ERROR, "BUS ERROR" },
    { AD2425W_A2B_BUS_TIMEOUT, "BUS TIMEOUT" },
    { AD2425W_SIMPLE_ODD_I2C_ADDRESS_ERROR, "ODD I2C ADDRESS ERROR" },
    { AD2425W_CORRUPT_INIT_FILE, "CORRUPT INIT FILE" },
    { AD2425W_UNSUPPORTED_INIT_FILE, "UNSUPPORTED INIT FILE" },
    { AD2425W_UNSUPPORTED_READ_WIDTH, "UNSUPPORTED READ WIDTH" },
    { AD2425W_UNSUPPORTED_DATA_WIDTH, "UNSUPPORTED DATA WIDTH" },
    { AD2425W_SIMPLE_ERROR, "GENERIC ERROR" },
    { AD2425W_A2B_MASTER_RUNNING, "MASTER RUNNING" },
    { AD2425W_A2B_NODE_DISCOVERED, "NODE DISCOVERED" },
    { AD2425W_A2B_BUS_SHORT_TO_GROUND, "BUS SHORT TO GROUND" },
    { AD2425W_A2B_BUS_SHORT_TO_BATT, "A2B BUS SHORT TO BATT" },
    { AD2425W_A2B_I2C_WRITE_ERROR, "I2C WRITE ERROR" },
    { AD2425W_A2B_I2C_READ_ERROR, "I2C READ ERROR" },
    { (BM_AD2425W_RESULT)0, NULL }
};

/**
 * @brief      Translates an A2B result to a readable string
 *
 * @param      result      Result code
 *
 * @return     Result string
 */
const char *ad2425w_result_str(BM_AD2425W_RESULT result)
{
    const char *result_str = "Unknown";
    const struct RESULT_STRING *rs;

    rs = RESULT_STRINGS;
    while (rs->string != NULL) {
        if (rs->result == result) {
            result_str = rs->string;
            break;
        }
        rs++;
    }

    return(result_str);
}

/**
 * @brief      Resets the discovery state
 *
 * @param      ad2425w      Pointer to instance structure
 *
 * @return     None
 */
static void ad2425w_reset(BM_AD2425W_CONTROLLER *ad2425w)
{
    ad2425w->nodes_discovered = 0;
    ad2425w->init_line = 0;
    AD2425_MEMSET(ad2425w->nodes, 0, sizeof(ad2425w->nodes));
    ad2425w->I2SGCFG = 0x00;
    ad2425w->I2SCFG = 0x00;
}


/**
 * @brief      Initializes the a2b controller (AD2425W)
 *
 * @param      ad2425w       Pointer to instance structure
 * @param[in]  mode          specify if this device is a master or slave
 * @param[in]  twi_address   The twi address (master) for the ad2425w
 * @param[in]  device        The TWI/I2C peripheral device handle
 *
 * @return     Success or failure
 */
BM_AD2425W_RESULT ad2425w_initialize(BM_AD2425W_CONTROLLER *ad2425w,
    BM_AD2425W_MODE mode, uint8_t twi_address, sTWI *device)
{

    // Ensure that the I2C/TWI address is even
    if (twi_address & 0x1) {
        return AD2425W_SIMPLE_ODD_I2C_ADDRESS_ERROR;
    }

    // Set master and slave access addresses
    ad2425w->_twi_master_addr = twi_address;
    ad2425w->_twi_slave_addr = twi_address + 1;

    // Set mode of operation (Master or Slave)
    ad2425w->_mode = mode;

    // Save the TWI device handle
    ad2425w->_twi = device;

    // Reset state
    ad2425w_reset(ad2425w);

    return AD2425W_SIMPLE_SUCCESS;
}

/**
 * @brief      This function polls for a limited set of IRQs from the AD2425
 *
 * @param[in]  ad2425w      The driver instance
 * @param[in]  irq_pending  What IRQ to track
 * @param[in]  timeout      IRQ timeout in milliseconds
 *
 */
static BM_AD2425W_RESULT ad2425w_irq_poll(BM_AD2425W_CONTROLLER *ad2425w,
    uint32_t irq_pending, uint16_t timeout)
{
    uint32_t start, elapsed;
    struct INT_REGS intInfo;
    BM_AD2425W_RESULT result;
    bool polling;

    polling = true;
    start = getTimeStamp();
    result = AD2425W_SIMPLE_SUCCESS;

    do {

        /* Perform atomic read of interrupt status registers */
        result = ad2425w_read_ctrl_reg_block(ad2425w, AD2425W_REG_INTSRC,
            sizeof(intInfo), (uint8_t *)&intInfo, ad2425w->_twi_master_addr);
        if (result != AD2425W_SIMPLE_SUCCESS) {
            return(result);
        }

        /* Service a select group of possible interrupts */
        if (intInfo.intSrc & AD2425W_BITM_INTSRC_MSTINT) {
            switch (intInfo.intType) {
                case 255:
                    if (irq_pending == AD2425W_IRQ_PLL_LOCK_PENDING) {
                        result = AD2425W_SIMPLE_SUCCESS;
                        polling = false;
                    }
                    break;
                case 24:
                    if (irq_pending == AD2425W_IRQ_DISCOVERY_PENDING) {
                        result = AD2425W_SIMPLE_SUCCESS;
                        ad2425w->nodes_discovered++;
                        polling = false;
                    }
                    break;
                case 41:
                    result = AD2425W_A2B_BUS_SHORT_TO_GROUND;
                    polling = false;
                    break;
                case 42:
                    result = AD2425W_A2B_BUS_SHORT_TO_BATT;
                    polling = false;
                    break;
                default:
                    break;
            }
        }

        /* Check polling inverval */
        if (polling) {
            elapsed = getTimeStamp() - start;
            if (elapsedTimeMs(elapsed) > timeout) {
                result = AD2425W_A2B_BUS_TIMEOUT;
                polling = false;
            } else {
                delay(5);
            }
        }

    } while (polling);

    return (result);
}

BM_AD2425W_RESULT ad2425w_new_master_addr(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len,
    uint8_t old_addr, uint8_t new_addr)
{
    ADI_A2B_DISCOVERY_CONFIG *row;
    BM_AD2425W_RESULT result;
    uint32_t num_rows;
    int i;

    // Ensure that the I2C/TWI address is even
    if (new_addr & 0x1) {
        return AD2425W_SIMPLE_ODD_I2C_ADDRESS_ERROR;
    }

    row = (ADI_A2B_DISCOVERY_CONFIG *)init_sequence;
    num_rows = init_len / sizeof(ADI_A2B_DISCOVERY_CONFIG);
    result = AD2425W_SIMPLE_SUCCESS;

    // Rewrite addresses in init sequence
    for (i = 0; i < num_rows; i++) {
        if (row->i2c_addr == old_addr) {
            row->i2c_addr = new_addr;
        } else if (row->i2c_addr == (old_addr + 1)) {
           row->i2c_addr = new_addr + 1;
        }
        row++;
    }

    // Rewrite addresses in controller
    ad2425w->_twi_master_addr = new_addr;
    ad2425w->_twi_slave_addr = new_addr + 1;

    return(result);
}

#define INIT_SCAN_1 0x00000001
#define INIT_SCAN_2 0x00000002

#define INIT_SCAN_ALL (        \
    INIT_SCAN_1 | INIT_SCAN_2  \
)

BM_AD2425W_RESULT ad2425w_scan_init_sequence(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len)
{
    BM_AD2425W_ACCESS_TYPE mode;
    ADI_A2B_DISCOVERY_CONFIG *row;
    BM_AD2425W_RESULT result;
    uint32_t num_rows;
    uint8_t reg, val;
    bool single_reg;
    int i;
    uint32_t found;

    row = (ADI_A2B_DISCOVERY_CONFIG *)init_sequence;
    num_rows = init_len / sizeof(ADI_A2B_DISCOVERY_CONFIG);
    result = AD2425W_SIMPLE_SUCCESS;
    found = 0x00000000;

    for (i = 0; i < num_rows; i++) {

        /* Check to see if this access is to the master or slave I2C address */
        if (row->i2c_addr == ad2425w->_twi_master_addr) {
            mode = AD2425W_SIMPLE_MASTER_ACCESS;
        } else if (row->i2c_addr == ad2425w->_twi_slave_addr) {
            mode = AD2425W_SIMPLE_SLAVE_ACCESS;
        } else if ((row->i2c_addr == 0x00) && (row->cmd == A2B_DELAY)) {
            mode = AD2425W_SIMPLE_UNKNOWN_ACCESS;
        } else {
            /* For now, don't initialize local components */
            goto next_row;
        }

        /* Pre-decode single register accesses for use in
         * read, write, and IRQ tracking below.
         */
        if ((row->data_count == 1) && (row->addr_width == 1)) {
            reg = (uint8_t)(row->addr & 0xFF);
            val = row->config_data[0];
            single_reg = true;
        } else {
            reg = 0xFF; val = 0; single_reg = false;
        }

        /*
         * Scan for a variety of items
         */
        if ( (single_reg) &&
             (mode == AD2425W_SIMPLE_MASTER_ACCESS) &&
             (row->cmd == A2B_WRITE) ) {

            if (reg == AD2425W_REG_I2SGCFG) {
                ad2425w->I2SGCFG = val;
                found |= INIT_SCAN_1;
            } else if (reg == AD2425W_REG_I2SCFG) {
                ad2425w->I2SCFG = val;
                found |= INIT_SCAN_2;
            }
        }

        if (found == INIT_SCAN_ALL) {
            break;
        }

next_row:
        row++;
    }

    if (found != INIT_SCAN_ALL) {
        result = AD2425W_UNSUPPORTED_INIT_FILE;
    }

    return (result);
}

BM_AD2425W_RESULT ad2425w_new_master_i2s_cfg(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len,
    uint8_t I2SGCFG, uint8_t I2SCFG)
{
    BM_AD2425W_ACCESS_TYPE mode;
    ADI_A2B_DISCOVERY_CONFIG *row;
    BM_AD2425W_RESULT result;
    uint32_t num_rows;
    uint8_t reg;
    bool single_reg;
    int i;
    uint32_t found;

    row = (ADI_A2B_DISCOVERY_CONFIG *)init_sequence;
    num_rows = init_len / sizeof(ADI_A2B_DISCOVERY_CONFIG);
    result = AD2425W_SIMPLE_SUCCESS;
    found = 0x00000000;

    for (i = 0; i < num_rows; i++) {

        /* Check to see if this access is to the master or slave I2C address */
        if (row->i2c_addr == ad2425w->_twi_master_addr) {
            mode = AD2425W_SIMPLE_MASTER_ACCESS;
        } else if (row->i2c_addr == ad2425w->_twi_slave_addr) {
            mode = AD2425W_SIMPLE_SLAVE_ACCESS;
        } else if ((row->i2c_addr == 0x00) && (row->cmd == A2B_DELAY)) {
            mode = AD2425W_SIMPLE_UNKNOWN_ACCESS;
        } else {
            /* For now, don't initialize local components */
            goto next_row;
        }

        /* Pre-decode single register accesses for use in
         * read, write, and IRQ tracking below.
         */
        if ((row->data_count == 1) && (row->addr_width == 1)) {
            reg = (uint8_t)(row->addr & 0xFF);
            single_reg = true;
        } else {
            reg = 0xFF; single_reg = false;
        }

        /*
         * Scan for a variety of items
         */
        if ( (single_reg) &&
             (mode == AD2425W_SIMPLE_MASTER_ACCESS) &&
             (row->cmd == A2B_WRITE) ) {

            if (reg == AD2425W_REG_I2SGCFG) {
                row->config_data[0] = I2SGCFG;
                found |= INIT_SCAN_1;
            } else if (reg == AD2425W_REG_I2SCFG) {
                row->config_data[0] = I2SCFG;
                found |= INIT_SCAN_2;
            }
        }

        if (found == INIT_SCAN_ALL) {
            break;
        }

next_row:
        row++;
    }

    if (found != INIT_SCAN_ALL) {
        result = AD2425W_UNSUPPORTED_INIT_FILE;
    }

    return(result);
}

/**
 * @brief      Initializes the A2B system using a "commandlist" file from
 *             sigmastudio
 *
 * @param      ad2425w          Pointer to instance structure
 * @param      init_Sequence    The initialize sequence
 * @param[in]  init_len         The size of entire initialization sequence
 *                              in bytes (not lines)
 *
 * @return     Success or failure
 */
BM_AD2425W_RESULT ad2425w_load_init_sequence(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len)
{
    BM_AD2425W_ACCESS_TYPE mode;
    ADI_A2B_DISCOVERY_CONFIG *row;
    BM_AD2425W_RESULT result;
    uint32_t num_rows;
    uint32_t irq_pending;
    uint8_t reg, val;
    uint8_t read_val;
    bool single_reg;
    uint16_t delay_ms;
    int i;

    /* Clear out old state */
    ad2425w_reset(ad2425w);

    row = (ADI_A2B_DISCOVERY_CONFIG *)init_sequence;
    num_rows = init_len / sizeof(ADI_A2B_DISCOVERY_CONFIG);
    irq_pending = AD2425W_IRQ_NONE;
    result = AD2425W_SIMPLE_SUCCESS;
    delay_ms = 0;

    for (i = 0; i < num_rows; i++) {

        /* Check to see if this access is to the master or slave I2C address */
        if (row->i2c_addr == ad2425w->_twi_master_addr) {
            mode = AD2425W_SIMPLE_MASTER_ACCESS;
        } else if (row->i2c_addr == ad2425w->_twi_slave_addr) {
            mode = AD2425W_SIMPLE_SLAVE_ACCESS;
        } else if ((row->i2c_addr == 0x00) && (row->cmd == A2B_DELAY)) {
            mode = AD2425W_SIMPLE_UNKNOWN_ACCESS;
        } else {
            /* For now, don't initialize local components */
            goto next_row;
        }

        /* Pre-decode single register accesses for use in
         * read, write, and IRQ tracking below.
         */
        if ((row->data_count == 1) && (row->addr_width == 1)) {
            reg = (uint8_t)(row->addr & 0xFF);
            val = row->config_data[0];
            single_reg = true;
        } else {
            reg = 0xFF; val = 0; single_reg = false;
        }

        switch (row->cmd) {

            //*********************************************************************
            // Write command
            //*********************************************************************
            case A2B_WRITE:

                /* Optimize the single register write case */
                if (single_reg) {
                    result = ad2425w_write_ctrl_reg(ad2425w,
                        reg, val, row->i2c_addr
                    );
                }
                else {
                    result = ad2425w_write_ctrl_reg_block(ad2425w,
                        row->addr, row->addr_width,
                        row->data_count, row->config_data, row->i2c_addr
                    );
                }
                if (result != AD2425W_SIMPLE_SUCCESS) {
                    return(result);
                }
                break;

            //*********************************************************************
            // Read command
            //*********************************************************************
            case A2B_READ:

                /* Can only process single register reads */
                if (single_reg) {
                    result = ad2425w_read_ctrl_reg(ad2425w, reg, &read_val, row->i2c_addr);
                    if (result != AD2425W_SIMPLE_SUCCESS) {
                        return(result);
                    }
                } else {
                    return(AD2425W_UNSUPPORTED_DATA_WIDTH);
                }
                break;

            //*********************************************************************
            // Delay command
            //*********************************************************************
            case A2B_DELAY:

                if (row->data_count > 2) {
                    return(AD2425W_UNSUPPORTED_DATA_WIDTH);
                }

                /* Calculate the delay */
                delay_ms = row->config_data[0];
                if (row->data_count == 2) {
                    delay_ms = (delay_ms << 8) + row->config_data[1];
                }

                /* Do only straight delays here.  More advanced IRQ polling
                 * is done below using the delay as a timeout.
                 */
                if (irq_pending == AD2425W_IRQ_NONE) {
                    delay(delay_ms);
                }
                break;

            default:
                break;
        }

        /*
         * Perform more advanced IRQ handling during delay intervals
         */
        if ( (row->cmd == A2B_DELAY) &&
             (irq_pending != AD2425W_IRQ_NONE) ) {
            result = ad2425w_irq_poll(ad2425w, irq_pending, delay_ms);
            if (result != AD2425W_SIMPLE_SUCCESS) {
                return(result);
            }
        }

        /*
         * Track limited slave node information
         */
        if ( (single_reg) &&
             (mode == AD2425W_SIMPLE_SLAVE_ACCESS) &&
             (row->cmd == A2B_READ) ) {

            if (ad2425w->nodes_discovered > 0) {
                uint8_t current_node = ad2425w->nodes_discovered - 1;
                switch (reg) {
                    case AD2425W_REG_VENDOR:
                        ad2425w->nodes[current_node].vendor  = read_val;
                        break;
                    case AD2425W_REG_PRODUCT:
                        ad2425w->nodes[current_node].product  = read_val;
                        break;
                    case AD2425W_REG_VERSION:
                        ad2425w->nodes[current_node].version  = read_val;
                        break;
                    default:
                        break;
                }
            }
        }

        /*
         * Track the state of a select set of master register writes
         * for limited error detection and IRQ handling during the next
         * delay interval.
         */
        if ( (single_reg) &&
             (mode == AD2425W_SIMPLE_MASTER_ACCESS) &&
             (row->cmd == A2B_WRITE) ) {

            /* Track PLL lock IRQ following reset */
            if ((reg == AD2425W_REG_CONTROL) &&
                (val & AD2425W_BITM_CONTROL_SOFTRST)) {
                irq_pending = AD2425W_IRQ_PLL_LOCK_PENDING;

            /* Track discovery IRQ */
            } else if (reg == AD2425W_REG_DISCVRY) {
                irq_pending = AD2425W_IRQ_DISCOVERY_PENDING;

            /* Nothing special happening  */
            } else {
                irq_pending = AD2425W_IRQ_NONE;
            }
        }

next_row:
        ad2425w->init_line++;
        row++;
    }

    return AD2425W_SIMPLE_SUCCESS;
}

/**
 * @brief      Writes a control register in the AD2425W
 *
 * @param      ad2425w  The instance of the driver
 * @param[in]  reg      The register address
 * @param[in]  value    The value
 * @param[in]  i2cAddr  I2C address
 */
static BM_AD2425W_RESULT ad2425w_write_ctrl_reg(BM_AD2425W_CONTROLLER *ad2425w,
    uint8_t reg, uint8_t value, uint8_t i2cAddr)
{

    TWI_SIMPLE_RESULT twiResult;

    // Perform write
    uint8_t seq[2] = {reg, value};

    twiResult = twi_write(ad2425w->_twi, i2cAddr, seq, 2);

    return (twiResult == TWI_SIMPLE_SUCCESS ?
        AD2425W_SIMPLE_SUCCESS : AD2425W_A2B_I2C_WRITE_ERROR);
}

/**
 * @brief      Writes a block of data to the AD2425W.  This is used primarily to initialize remote A2B nodes
 *
 * @param      ad2425w     The instance of the driver
 * @param[in]  addr        The address of the I2C control register to write in the remote note
 * @param[in]  addr_bytes  The number of bytes in the address
 * @param[in]  len         The total number of bytes to write
 * @param      values      The values to be written
 * @param[in]  i2cAddr     I2C address
 */
static BM_AD2425W_RESULT ad2425w_write_ctrl_reg_block(BM_AD2425W_CONTROLLER *ad2425w,
    uint32_t addr, uint8_t addr_bytes,
    uint16_t len, uint8_t *values, uint8_t i2cAddr)
{

    TWI_SIMPLE_RESULT twiResult;

    // Get address ready to transmit - send MSB first
    uint8_t *seq = AD2425_MALLOC(addr_bytes + len);
    if (seq == NULL) {
        return(AD2425W_A2B_I2C_WRITE_ERROR);
    }

    if (addr_bytes == 1) {
        seq[0] = addr & 0xFF;
    }
    else if (addr_bytes == 2) {
        seq[0] = ((addr >> 8) & 0xFF);
        seq[1] = addr & 0xFF;
    }
    if (addr_bytes == 4) {
        seq[0] = ((addr >> 24) & 0xFF);
        seq[1] = ((addr >> 16) & 0xFF);
        seq[2] = ((addr >> 8) & 0xFF);
        seq[3] = addr & 0xFF;
    }

    // Copy data to send
    AD2425_MEMCPY(seq + addr_bytes, values, len);

    // Write address followed by block of data
    twiResult = twi_write(ad2425w->_twi, i2cAddr,
        seq, addr_bytes + len);

    // Free addr+data buffer
    if (seq) {
        AD2425_FREE(seq);
    }

    return (twiResult == TWI_SIMPLE_SUCCESS ?
        AD2425W_SIMPLE_SUCCESS : AD2425W_A2B_I2C_WRITE_ERROR);
}

/**
 * @brief      Reads a control register in the AD2425W
 *
 * @param      ad2425w  The instance of the driver
 * @param[in]  reg      The register address
 * @param[in]  i2cAddr  I2C address
 *
 * @return     { description_of_the_return_value }
 */
static BM_AD2425W_RESULT ad2425w_read_ctrl_reg(BM_AD2425W_CONTROLLER *ad2425w,
    uint8_t reg, uint8_t *value, uint8_t i2cAddr)
{

    TWI_SIMPLE_RESULT twiResult;

    twiResult = twi_writeRead(ad2425w->_twi, i2cAddr, &reg, 1, value, 1);

    return (twiResult == TWI_SIMPLE_SUCCESS ?
        AD2425W_SIMPLE_SUCCESS : AD2425W_A2B_I2C_READ_ERROR);
}

/**
 * @brief      Reads a block of control registers in the AD2425W
 *
 * @param      ad2425w  The instance of the driver
 * @param[in]  reg      The register address
 * @param[in]  len      The number of control registers to read
 * @param[in]  values   The control register contents
 * @param[in]  i2cAddr  I2C address
 *
 * @return     { description_of_the_return_value }
 */
static BM_AD2425W_RESULT ad2425w_read_ctrl_reg_block(BM_AD2425W_CONTROLLER *ad2425w,
    uint8_t reg, uint16_t len, uint8_t *values,
    uint8_t i2cAddr)
{
    TWI_SIMPLE_RESULT twiResult;

    twiResult = twi_writeRead(ad2425w->_twi, i2cAddr, &reg, 1, values, len);

    return (twiResult == TWI_SIMPLE_SUCCESS ?
        AD2425W_SIMPLE_SUCCESS : AD2425W_A2B_I2C_READ_ERROR);
}
