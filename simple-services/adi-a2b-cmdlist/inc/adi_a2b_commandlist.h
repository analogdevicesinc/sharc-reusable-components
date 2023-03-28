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

#ifndef _adi_a2b_commandlist_h
#define _adi_a2b_commandlist_h

#ifdef __cplusplus
extern "C" {
#endif

/* Binary struct pulled from "adi_a2b_i2c_commandlist.h" */
typedef struct
{
    /*!  Device address */
    unsigned char nDeviceAddr;

    /*!  Operation code */
    unsigned char eOpCode;

    /*! Reg Sub address width (in bytes) */
    unsigned char nAddrWidth;

    /*! Reg Sub address */
    unsigned int nAddr;

    /*! Reg data width (in bytes) */
    unsigned char nDataWidth;

    /*! Reg data count (in bytes) */
    unsigned short nDataCount;

    /*! Config Data */
    unsigned char* paConfigData;

} A2B_CMD;

/* Binary struct pulled from "adi_a2b_i2c_commandlist.h" */
typedef struct
{
    /*!  Device address */
    unsigned char nDeviceAddr;

    /*!  Operation code */
    unsigned char eOpCode;

    /*! SPI Command width (in bytes), not used for I2C */
    unsigned char nSpiCmdWidth;

    /*! SPI Commands, not used for I2C */
    unsigned int nSpiCmd;

    /*! Reg Sub address width (in bytes) */
    unsigned char nAddrWidth;

    /*! Reg Sub address */
    unsigned int nAddr;

    /*! Reg data width (in bytes) */
    unsigned char nDataWidth;

    /*! Reg data count (in bytes) */
    unsigned short nDataCount;

    /*! Config Data */
    unsigned char* paConfigData;

    /*! Protocol */
    unsigned char eProtocol;

} A2B_CMD_SPI;

typedef enum
{
    A2B_CMD_TYPE_UNKNOWN = 0,
    A2B_CMD_TYPE_I2C,
    A2B_CMD_TYPE_SPI
} A2B_CMD_TYPE;

/* 'eOpCode' values from "adi_a2b_i2c_commandlist.h" */
#define A2B_CMD_OP_WRITE   ((unsigned char) 0x00u)
#define A2B_CMD_OP_READ    ((unsigned char) 0x01u)
#define A2B_CMD_OP_DELAY   ((unsigned char) 0x02u)
#define A2B_CMD_OP_INVALID ((unsigned char) 0xffu)

/* 'eProtocol' values from "adi_a2b_i2c_commandlist.h" */
#define A2B_CMD_PROTO_I2C     ((unsigned char) 0x00u)
#define A2B_CMD_PROTO_SPI     ((unsigned char) 0x01u)

#ifdef __cplusplus
} // extern "C"
#endif

#endif
