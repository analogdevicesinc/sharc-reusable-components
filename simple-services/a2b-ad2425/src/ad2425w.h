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

/*
 * Bare-Metal ("BM") device driver header for A2B
 */
#ifndef _BM_AD2425W_H
#define _BM_AD2425W_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "twi_simple.h"

// Used to read A2BConfig files generated by SigmaStudio
#define     A2B_WRITE   ((unsigned char)0x00u)
#define     A2B_READ    ((unsigned char)0x01u)
#define     A2B_DELAY   ((unsigned char)0x02u)
#define     A2B_INVALID ((unsigned char)0xffu)

#define     A2B_MAX_NODES   (8)

#define     AD2425W_REG_CHIP                    0x00
#define     AD2425W_REG_NODEADDR                0x01
#define     AD2425W_REG_VENDOR                  0x02
#define     AD2425W_REG_PRODUCT                 0x03
#define     AD2425W_REG_VERSION                 0x04
#define     AD2425W_REG_CAPABILITY              0x05

#define     AD2425W_REG_SWCTL                   0x09
#define     AD2425W_REG_BCDNSLOTS               0x0A
#define     AD2425W_REG_LDNSLOTS                0x0B
#define     AD2425W_REG_LUPSLOTS                0x0C
#define     AD2425W_REG_DNSLOTS                 0x0D
#define     AD2425W_REG_UPSLOTS                 0x0E
#define     AD2425W_REG_RESPCYCS                0x0F
#define     AD2425W_REG_SLOTFMT                 0x10

#define     AD2425W_REG_DATCTL                  0x11
#define     AD2425W_REG_CONTROL                 0x12
#define     AD2425W_REG_DISCVRY                 0x13

#define     AD2425W_REG_SWSTAT                  0x14
#define     AD2425W_REG_INTSTAT                 0x15
#define     AD2425W_REG_INTSRC                  0x16
#define     AD2425W_REG_INTTYPE                 0x17
#define     AD2425W_REG_INTPND0                 0x18
#define     AD2425W_REG_INTPND1                 0x19
#define     AD2425W_REG_INTPND2                 0x1A
#define     AD2425W_REG_INTMSK0                 0x1B
#define     AD2425W_REG_INTMSK1                 0x1C
#define     AD2425W_REG_INTMSK2                 0x1D

#define     AD2425W_REG_BECCTL                  0x1E
#define     AD2425W_REG_BECCNT                  0x1F
#define     AD2425W_REG_TESTMODE                0x20
#define     AD2425W_REG_ERRCNT0                 0x21
#define     AD2425W_REG_ERRCNT1                 0x22
#define     AD2425W_REG_ERRCNT2                 0x23
#define     AD2425W_REG_ERRCNT3                 0x24

#define     AD2425W_REG_NODE                    0x29
#define     AD2425W_REG_DISCSTAT                0x2B
#define     AD2425W_REG_LINTTYPE                0x3E

#define     AD2425W_REG_I2CCFG                  0x3F
#define     AD2425W_REG_PLLCTL                  0x40
#define     AD2425W_REG_I2SGCFG                 0x41
#define     AD2425W_REG_I2SCFG                  0x42
#define     AD2425W_REG_I2SRATE                 0x43
#define     AD2425W_REG_I2STXOFFSET             0x44
#define     AD2425W_REG_I2SRXOFFSET             0x45
#define     AD2425W_REG_SYNCOFFSET              0x46
#define     AD2425W_REG_PDMCTL                  0x47

#define     AD2425W_REG_ERRMGMT                 0x48
#define     AD2425W_REG_CLKCFG                  0x49

#define     AD2425W_REG_GPIODAT                 0x4A
#define     AD2425W_REG_GPIODATSET              0x4B
#define     AD2425W_REG_GPIODATCLR              0x4C
#define     AD2425W_REG_GPIOOEN                 0x4D
#define     AD2425W_REG_GPIOIEN                 0x4E
#define     AD2425W_REG_GPIOIN                  0x4F

#define     AD2425W_REG_PINTEN                  0x50
#define     AD2425W_REG_PINTINV                 0x51
#define     AD2425W_REG_PINCFG                  0x52
#define     AD2425W_REG_I2STEST                 0x53
#define     AD2425W_REG_RAISE                   0x54
#define     AD2425W_REG_GENERR                  0x55
#define     AD2425W_REG_I2SRRATE                0x56
#define     AD2425W_REG_I2SRRCTL                0x57
#define     AD2425W_REG_I2SRROFFS               0x58
#define     AD2425W_REG_CLK1CFG                 0x59
#define     AD2425W_REG_CLK2CFG                 0x5A
#define     AD2425W_REG_BMMCFG                  0x5B
#define     AD2425W_REG_SUSCFG                  0x5C
#define     AD2425W_REG_UPMASK0                 0x60
#define     AD2425W_REG_UPMASK1                 0x61
#define     AD2425W_REG_UPMASK2                 0x62
#define     AD2425W_REG_UPMASK3                 0x63
#define     AD2425W_REG_UPOFFSET                0x64
#define     AD2425W_REG_DNMASK0                 0x65
#define     AD2425W_REG_DNMASK1                 0x66
#define     AD2425W_REG_DNMASK2                 0x67
#define     AD2425W_REG_DNMASK3                 0x68
#define     AD2425W_REG_DNOFFSET                0x69

/*GPIO Registers */

#define     AD2425W_REG_GPIODEN                 0x80
#define     AD2425W_REG_GPIOD0MSK               0x81
#define     AD2425W_REG_GPIOD1MSK               0x82
#define     AD2425W_REG_GPIOD2MSK               0x83
#define     AD2425W_REG_GPIOD3MSK               0x84
#define     AD2425W_REG_GPIOD4MSK               0x85
#define     AD2425W_REG_GPIOD5MSK               0x86
#define     AD2425W_REG_GPIOD6MSK               0x87
#define     AD2425W_REG_GPIOD7MSK               0x88
#define     AD2425W_REG_GPIODDAT                0x89
#define     AD2425W_REG_GPIODINV                0x8A

/*Mailbox Registers */

#define     AD2425W_REG_MBOX0_CTL               0x90
#define     AD2425W_REG_MBOX0_STAT              0x91
#define     AD2425W_REG_MBOX0_BYTE0             0x92
#define     AD2425W_REG_MBOX0_BYTE1             0x93
#define     AD2425W_REG_MBOX0_BYTE2             0x94
#define     AD2425W_REG_MBOX0_BYTE3             0x95

#define     AD2425W_REG_MBOX1_CTL               0x96
#define     AD2425W_REG_MBOX1_STAT              0x97
#define     AD2425W_REG_MBOX1_BYTE0             0x98
#define     AD2425W_REG_MBOX1_BYTE1             0x99
#define     AD2425W_REG_MBOX1_BYTE2             0x9A
#define     AD2425W_REG_MBOX1_BYTE3             0x9B

// Base address for AD2425W (A2B controller)
#define     AD2425W_SAM_I2C_ADDR                0x68
#define     AD2425W_SAM_AUTO_I2C_ADDR           0x6A

/* Various Register Bits */
#define     AD2425W_BITM_CONTROL_MSTR           0x80
#define     AD2425W_BITM_CONTROL_SOFTRST        0x04
#define     AD2425W_BITM_INTSRC_MSTINT          0x80

typedef enum
{
    AD2425W_SIMPLE_MASTER,                  // Master mode
    AD2425W_SIMPLE_SLAVE                    // Slave mode
} BM_AD2425W_MODE;


typedef enum
{
    AD2425W_SIMPLE_SUCCESS,                 // The API call is success
    AD2425W_A2B_BUS_ERROR,                  // A bus error was encountered while initializing the bus
    AD2425W_A2B_BUS_TIMEOUT,                // A timeout occurred while initializing the bus
    AD2425W_SIMPLE_ODD_I2C_ADDRESS_ERROR,   // I2C address needs to be even
    AD2425W_CORRUPT_INIT_FILE,              // SS Generated Init file is corrupt
    AD2425W_UNSUPPORTED_INIT_FILE,          // The init file type is not supported by this driver
    AD2425W_UNSUPPORTED_READ_WIDTH,         // An init file has a multibyte read command which isn't yet implemented in this driver
    AD2425W_UNSUPPORTED_DATA_WIDTH,         // An init file has a multibyte data format which isn't yet implemented in this driver
    AD2425W_SIMPLE_ERROR,                   // General failure
    AD2425W_A2B_MASTER_RUNNING,
    AD2425W_A2B_NODE_DISCOVERED,
    AD2425W_A2B_BUS_SHORT_TO_GROUND,
    AD2425W_A2B_BUS_SHORT_TO_BATT,
    AD2425W_A2B_I2C_WRITE_ERROR,
    AD2425W_A2B_I2C_READ_ERROR,
} BM_AD2425W_RESULT;

typedef enum
{
    AD2425W_SIMPLE_UNKNOWN_ACCESS,
    AD2425W_SIMPLE_MASTER_ACCESS,
    AD2425W_SIMPLE_SLAVE_ACCESS
} BM_AD2425W_ACCESS_TYPE;

typedef struct
{
    uint8_t vendor;
    uint8_t product;
    uint8_t version;
} BM_AD2425W_A2B_BUS_NODE;

typedef struct
{
    /* These elements are used by the application */
    uint8_t nodes_discovered;
    BM_AD2425W_A2B_BUS_NODE nodes[A2B_MAX_NODES];
    uint16_t init_line;
    uint8_t I2SGCFG;
    uint8_t I2SCFG;

    /* These structure elements are used exclusively by the driver */
    sTWI *_twi;                        // Simple TWI driver
    uint8_t _twi_master_addr;          // Master address of A2B controller
    uint8_t _twi_slave_addr;           // Slave address of A2B controller
    BM_AD2425W_MODE _mode;

} BM_AD2425W_CONTROLLER;

#ifdef __cplusplus
extern "C" {
#endif

// Initializes the a2b controller (AD2425W)
BM_AD2425W_RESULT ad2425w_initialize(BM_AD2425W_CONTROLLER *ad2425w,
                                     BM_AD2425W_MODE mode,
                                     uint8_t twi_address,
                                     sTWI *device);

// Scan the A2B system "commandlist" for important info
BM_AD2425W_RESULT ad2425w_scan_init_sequence(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len);

// Initializes the A2B system using a "commandlist" file from sigmastudio
BM_AD2425W_RESULT ad2425w_load_init_sequence(BM_AD2425W_CONTROLLER *ad2425w,
                                             void *init_Sequence,
                                             uint32_t init_len);

// Convert network and controller to a new master I2C address
BM_AD2425W_RESULT ad2425w_new_master_addr(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len,
    uint8_t old_addr, uint8_t new_addr);

// Convert master I2S config to new values
BM_AD2425W_RESULT ad2425w_new_master_i2s_cfg(
    BM_AD2425W_CONTROLLER *ad2425w, void *init_sequence, uint32_t init_len,
    uint8_t I2SGCFG, uint8_t I2SCFG);

// Convert the result to a readable string
const char *ad2425w_result_str(BM_AD2425W_RESULT result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  //_BM_AD2425W_H
