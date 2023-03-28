/**
 * Copyright (c) 2022 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */
#include <stdint.h>
#include <stdbool.h>

#include "twi_simple.h"
#include "cs2100.h"

#define CS2100_DEVICE_ID    0x01
#define CS2100_DEVICE_CTRL  0x02
#define CS2100_DEVICE_CFG_1 0x03
#define CS2100_GLOBAL_CFG   0x05
#define CS2100_RATIO        0x06
#define CS2100_FUNCT_CFG_1  0x16
#define CS2100_FUNCT_CFG_2  0x17
#define CS2100_FUNCT_CFG_3  0x1E

#define BITM_DEVICE_CFG_1_ENDEVCFG1 0x01
#define BITM_GLOBAL_CFG_ENDEVCFG2   0x01
#define BITM_GLOBAL_CFG_FREEZE      0x08
#define BITM_FUNCT_CFG_1_REFCLKDIV  0x18
#define BITM_FUNCT_CFG_2_LFRATIOCFG 0x08
#define BITM_FUNCT_CFG_3_CLKIN_BW   0x70
#define BITM_MAP_INCR               0x80

#define BITP_FUNCT_CFG_1_REFCLKDIV  3
#define BITP_FUNCT_CFG_2_LFRATIOCFG 3
#define BITP_FUNCT_CFG_3_CLKIN_BW   4

#define REG_BIT_CLR    0x00
#define REG_BIT_SET    0x01

static CS2100_RESULT cs2100_reg_bits(sTWI *twiHandle, uint8_t address,
    uint8_t regAddr, uint8_t mask, bool set)
{
    uint8_t regVal;
    TWI_SIMPLE_RESULT twiResult;

    regAddr &= ~(BITM_MAP_INCR);

    twiResult = twi_writeRead(twiHandle, address,
        &regAddr, sizeof(regAddr), &regVal, sizeof(regVal));

    if (twiResult != TWI_SIMPLE_SUCCESS) {
        return(CS2100_ERROR);
    }

    if (set) {
        regVal |= mask;
    } else {
        regVal &= ~mask;
    }

    twiResult = twi_writeWrite(twiHandle, address,
        &regAddr, sizeof(regAddr), &regVal, sizeof(regVal));

    if (twiResult != TWI_SIMPLE_SUCCESS) {
        return(CS2100_ERROR);
    }

    return(CS2100_SUCCESS);
}

CS2100_RESULT cs2100_set_ratio(sTWI *twiHandle, uint8_t address,
    double ratio)
{
    uint32_t Rud;
    uint8_t regAddr;
    uint8_t regVal;
    TWI_SIMPLE_RESULT twiResult;
    uint8_t RudBuf[4];

    regAddr = CS2100_FUNCT_CFG_2;

    twiResult = twi_writeRead(twiHandle, address,
        &regAddr, sizeof(regAddr), &regVal, sizeof(regVal));

    if (regVal & BITM_FUNCT_CFG_2_LFRATIOCFG) {
        /* High accuracy 12.20 */
        if (ratio >= 4096.0) {
            return(CS2100_ERROR);
        }
        Rud = (uint32_t)((double)(1048576) * ratio);
    } else {
        /* High multiplier 20.12 */
        Rud = (uint32_t)((double)(4096) * ratio);
    }

    RudBuf[0] = (Rud >> 24) & 0xFF;
    RudBuf[1] = (Rud >> 16) & 0xFF;
    RudBuf[2] = (Rud >> 8) & 0xFF;
    RudBuf[3] = (Rud >> 0) & 0xFF;

    regAddr = BITM_MAP_INCR | CS2100_RATIO;
    twiResult = twi_writeWrite(twiHandle, address,
        &regAddr, sizeof(regAddr), RudBuf, sizeof(RudBuf));

    twiResult = twi_writeRead(twiHandle, address,
        &regAddr, sizeof(regAddr), RudBuf, sizeof(RudBuf));

    return(CS2100_SUCCESS);
}

CS2100_RESULT cs2100_init(sTWI *twiHandle, uint8_t address, CS2100_CFG *cfg)
{
    CS2100_RESULT result;

    /* Clear EnDevCfg bits and freeze device */
    cs2100_reg_bits(twiHandle, address,
        CS2100_DEVICE_CFG_1, BITM_DEVICE_CFG_1_ENDEVCFG1, REG_BIT_CLR
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_GLOBAL_CFG, BITM_GLOBAL_CFG_ENDEVCFG2, REG_BIT_CLR
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_GLOBAL_CFG, BITM_GLOBAL_CFG_FREEZE, REG_BIT_SET
    );

    /* Set RefClkDiv */
    cs2100_reg_bits(twiHandle, address,
        CS2100_FUNCT_CFG_1, BITM_FUNCT_CFG_1_REFCLKDIV, REG_BIT_CLR
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_FUNCT_CFG_1, cfg->refClkDiv << BITP_FUNCT_CFG_1_REFCLKDIV,
        REG_BIT_SET
    );

    /* Set ClkIn_Bw */
    cs2100_reg_bits(twiHandle, address,
        CS2100_FUNCT_CFG_3, BITM_FUNCT_CFG_3_CLKIN_BW, REG_BIT_CLR
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_FUNCT_CFG_3, cfg->clkInBw << BITP_FUNCT_CFG_3_CLKIN_BW,
        REG_BIT_SET
    );

    /* Set LFRatioCfg */
    cs2100_reg_bits(twiHandle, address,
        CS2100_FUNCT_CFG_2, BITM_FUNCT_CFG_2_LFRATIOCFG, REG_BIT_CLR
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_FUNCT_CFG_2, cfg->ratioCfg << BITP_FUNCT_CFG_2_LFRATIOCFG,
        REG_BIT_SET
    );

    /* Set the initial ratio */
    result = cs2100_set_ratio(twiHandle, address, cfg->ratio);
    if (result != CS2100_SUCCESS) {
        goto abort;
    }

    /* Set EnDevCfg bits and un-freeze device */
    cs2100_reg_bits(twiHandle, address,
        CS2100_DEVICE_CFG_1, BITM_DEVICE_CFG_1_ENDEVCFG1, REG_BIT_SET
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_GLOBAL_CFG, BITM_GLOBAL_CFG_ENDEVCFG2, REG_BIT_SET
    );
    cs2100_reg_bits(twiHandle, address,
        CS2100_GLOBAL_CFG, BITM_GLOBAL_CFG_FREEZE, REG_BIT_CLR
    );

    result = CS2100_SUCCESS;

abort:
    return(result);
}
