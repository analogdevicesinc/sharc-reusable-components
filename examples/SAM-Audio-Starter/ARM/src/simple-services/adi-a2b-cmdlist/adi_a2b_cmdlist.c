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
#include <string.h>
#include <stdbool.h>

#include "adi_a2b_commandlist.h"
#include "adi_a2b_cmdlist_cfg.h"
#include "adi_a2b_cmdlist.h"

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "task.h"
    #define ADI_A2B_CMDLIST_ENTER_CRITICAL()  vTaskSuspendAll()
    #define ADI_A2B_CMDLIST_EXIT_CRITICAL()   xTaskResumeAll()
#else
    #define ADI_A2B_CMDLIST_ENTER_CRITICAL()
    #define ADI_A2B_CMDLIST_EXIT_CRITICAL()
#endif

#ifndef ADI_A2B_CMDLIST_MAX_LISTS
#define ADI_A2B_CMDLIST_MAX_LISTS 1
#endif

#ifndef ADI_A2B_CMDLIST_MAX_NODES
#define ADI_A2B_CMDLIST_MAX_NODES 16
#endif

#ifndef ADI_A2B_CMDLIST_MEMSET
#define ADI_A2B_CMDLIST_MEMSET memset
#endif

#ifndef ADI_A2B_CMDLIST_MEMCPY
#define ADI_A2B_CMDLIST_MEMCPY memcpy
#endif

#define     AD24xx_REG_CHIP                    0x00
#define     AD24xx_REG_NODEADR                 0x01
#define     AD24xx_REG_VENDOR                  0x02
#define     AD24xx_REG_PRODUCT                 0x03
#define     AD24xx_REG_VERSION                 0x04
#define     AD24xx_REG_CONTROL                 0x12
#define     AD24xx_REG_DISCVRY                 0x13
#define     AD24xx_REG_INTSRC                  0x16
#define     AD24xx_REG_INTTYPE                 0x17
#define     AD24xx_REG_I2SGCFG                 0x41
#define     AD24xx_REG_I2SCFG                  0x42

#define     AD24xx_BITM_NODEADR_BRCST          0x80
#define     AD24xx_BITM_NODEADR_PERI           0x20
#define     AD24xx_BITM_NODEADR_NODE           0x0F
#define     AD24xx_BITM_CONTROL_SOFTRST        0x04
#define     AD24xx_BITM_INTSRC_MSTINT          0x80

enum {
    ADI_A2B_CMDLIST_IRQ_NONE = 0,
    ADI_A2B_CMDLIST_IRQ_PLL_LOCK_PENDING,
    ADI_A2B_CMDLIST_IRQ_DISCOVERY_PENDING,
};

typedef enum _ADI_A2B_CMDLIST_ACCESS_TYPE {
    ADI_A2B_CMDLIST_UNKNOWN_ACCESS = 0,
    ADI_A2B_CMDLIST_NO_ACCESS,
    ADI_A2B_CMDLIST_MASTER_ACCESS,
    ADI_A2B_CMDLIST_SLAVE_ACCESS
} ADI_A2B_CMDLIST_ACCESS_TYPE;

typedef struct _ADI_A2B_CMDLIST_IRQ_REGS {
    uint8_t intSrc;
    uint8_t intType;
} ADI_A2B_CMDLIST_IRQ_REGS;

typedef struct _ADI_A2B_CMDLIST_RESULT_STRING {
    ADI_A2B_CMDLIST_RESULT result;
    const char *string;
} ADI_A2B_CMDLIST_RESULT_STRING;

typedef struct _ADI_A2B_CMDLIST {
    bool busy;
    ADI_A2B_CMDLIST_CFG cfg;
    void *cmdList;
    uint32_t cmdListLen;
    A2B_CMD_TYPE cmdListType;
    void *cmdPtr;
    uint32_t cmdIdx;
    uint32_t cmdLine;
    uint8_t cmdListMasterAddr;
    uint8_t nodesDiscovered;
    ADI_A2B_CMDLIST_NODE_INFO nodeInfo[ADI_A2B_CMDLIST_MAX_NODES];
    ADI_A2B_CMDLIST_OVERRIDE_INFO oi;

} ADI_A2B_CMDLIST;

/* ADI Command-list independent command data structure */
typedef struct {
    unsigned char deviceAddr;
    unsigned char opCode;
    unsigned char spiCmdWidth;
    unsigned int spiCmd;
    unsigned char addrWidth;
    unsigned int addr;
    unsigned char dataWidth;
    unsigned short dataCount;
    unsigned char *configData;
    unsigned char protocol;
} ADI_A2B_CMDLIST_CMD;

static ADI_A2B_CMDLIST cmdList[ADI_A2B_CMDLIST_MAX_LISTS];

const ADI_A2B_CMDLIST_RESULT_STRING ADI_A2B_CMDLIST_RESULT_STRINGS[] =  {
    { ADI_A2B_CMDLIST_SUCCESS, "SUCCESS" },
    { ADI_A2B_CMDLIST_ERROR, "ERROR" },
    { ADI_A2B_CMDLIST_CFG_ERROR, "CFG ERROR" },
    { ADI_A2B_CMDLIST_BUS_ERROR, "BUS ERROR" },
    { ADI_A2B_CMDLIST_BUS_TIMEOUT, "BUS TIMEOUT" },
    { ADI_A2B_CMDLIST_ODD_I2C_ADDRESS_ERROR, "ODD I2C ADDRESS ERROR" },
    { ADI_A2B_CMDLIST_CORRUPT_INIT_FILE, "CORRUPT INIT FILE" },
    { ADI_A2B_CMDLIST_UNSUPPORTED_INIT_FILE, "UNSUPPORTED INIT FILE" },
    { ADI_A2B_CMDLIST_UNSUPPORTED_READ_WIDTH, "UNSUPPORTED READ WIDTH" },
    { ADI_A2B_CMDLIST_UNSUPPORTED_DATA_WIDTH, "UNSUPPORTED DATA WIDTH" },
    { ADI_A2B_CMDLIST_UNSUPPORTED_PROTOCOL, "UNSUPPORTED PROTOCOL" },
    { ADI_A2B_CMDLIST_A2B_BUS_SHORT_TO_GROUND, "BUS SHORT TO GROUND" },
    { ADI_A2B_CMDLIST_A2B_BUS_SHORT_TO_BATT, "A2B BUS SHORT TO BATT" },
    { ADI_A2B_CMDLIST_A2B_I2C_WRITE_ERROR, "I2C WRITE ERROR" },
    { ADI_A2B_CMDLIST_A2B_I2C_READ_ERROR, "I2C READ ERROR" },
    { ADI_A2B_CMDLIST_A2B_MEMORY_ERROR, "BUFFER MEMORY ERROR" },
    { ADI_A2B_CMDLIST_END, "END OF COMMAND LIST" },
    { 0, NULL }
};

static void adi_a2b_cmdlist_reset(ADI_A2B_CMDLIST *list)
{
    list->cmdPtr = list->cmdList;
    list->cmdIdx = 0;
    list->cmdLine = 0;
}

static const char *adi_a2b_cmdlist_result_str(ADI_A2B_CMDLIST_RESULT result)
{
    const char *result_str = "Unknown";
    const ADI_A2B_CMDLIST_RESULT_STRING *rs;

    rs = ADI_A2B_CMDLIST_RESULT_STRINGS;
    while (rs->string != NULL) {
        if (rs->result == result) {
            result_str = rs->string;
            break;
        }
        rs++;
    }

    return(result_str);
}

static ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_write_ctrl_reg(
    ADI_A2B_CMDLIST *list, uint8_t reg, uint8_t value, uint8_t i2cAddr)
{
    ADI_A2B_CMDLIST_CFG *cfg = &list->cfg;
    uint8_t tx[2] = { reg, value };
    ADI_A2B_CMDLIST_RESULT result;

    result = cfg->twiWrite(
        cfg->handle, i2cAddr, tx, 2, cfg->usr
    );

    return(result);
}

static ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_write_ctrl_reg_block(
    ADI_A2B_CMDLIST *list,
    uint32_t addr, uint8_t addrBytes,
    uint16_t len, uint8_t *values, uint8_t i2cAddr)
{
    ADI_A2B_CMDLIST_CFG *cfg = &list->cfg;
    ADI_A2B_CMDLIST_RESULT result;
    uint8_t *tx = NULL;

    tx = cfg->getBuffer(addrBytes + len, cfg->usr);
    if (tx == NULL) {
        return(ADI_A2B_CMDLIST_A2B_MEMORY_ERROR);
    }

    if (addrBytes == 1) {
        tx[0] = addr & 0xFF;
    } else if (addrBytes == 2) {
        tx[0] = ((addr >> 8) & 0xFF);
        tx[1] = addr & 0xFF;
    } else if (addrBytes == 4) {
        tx[0] = ((addr >> 24) & 0xFF);
        tx[1] = ((addr >> 16) & 0xFF);
        tx[2] = ((addr >> 8) & 0xFF);
        tx[3] = addr & 0xFF;
    } else {
        cfg->freeBuffer(tx, cfg->usr);
        return(ADI_A2B_CMDLIST_UNSUPPORTED_ADDR_BYTES);
    }

    ADI_A2B_CMDLIST_MEMCPY(tx + addrBytes, values, len);

    result = cfg->twiWrite(
        cfg->handle, i2cAddr, tx, addrBytes + len, cfg->usr
    );

    cfg->freeBuffer(tx, cfg->usr);

    return (result);
}

static ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_read_ctrl_reg(
    ADI_A2B_CMDLIST *list,
    uint8_t reg, uint8_t *value, uint8_t i2cAddr)
{
    ADI_A2B_CMDLIST_CFG *cfg = &list->cfg;
    ADI_A2B_CMDLIST_RESULT result;

    result = cfg->twiWriteRead(
        cfg->handle, i2cAddr, &reg, 1, value, 1, cfg->usr
    );

    return(result);
}

static ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_read_ctrl_reg_block(
    ADI_A2B_CMDLIST *list,
    uint8_t reg, uint16_t len, uint8_t *values,
    uint8_t i2cAddr)
{
    ADI_A2B_CMDLIST_CFG *cfg = &list->cfg;
    ADI_A2B_CMDLIST_RESULT result;

    result = cfg->twiWriteRead(
        cfg->handle, i2cAddr, &reg, 1, values, len, cfg->usr
    );

    return(result);
}

static ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_next(
    ADI_A2B_CMDLIST *list, ADI_A2B_CMDLIST_CMD *cmd,
    ADI_A2B_CMDLIST_ACCESS_TYPE *mode,
    uint8_t *reg, uint8_t *val, bool *singleReg

)
{
    A2B_CMD *i2cCmd = NULL;
    A2B_CMD_SPI *spiCmd = NULL;
    size_t cmdLen = 0;
    uint8_t _reg, _val;
    bool _singleReg;
    ADI_A2B_CMDLIST_ACCESS_TYPE _mode;

    if (list->cmdIdx >= list->cmdListLen) {
        return(ADI_A2B_CMDLIST_END);
    }

    if (list->cmdListType == A2B_CMD_TYPE_I2C) {
        cmdLen = sizeof(*i2cCmd);
        i2cCmd = list->cmdPtr;
    } else {
        cmdLen = sizeof(*spiCmd);
        spiCmd = list->cmdPtr;
    }

    if (i2cCmd) {
        cmd->deviceAddr = i2cCmd->nDeviceAddr;
        cmd->opCode = i2cCmd->eOpCode;
        cmd->addrWidth = i2cCmd->nAddrWidth;
        cmd->addr = i2cCmd->nAddr;
        cmd->dataWidth = i2cCmd->nDataWidth;
        cmd->dataCount = i2cCmd->nDataCount;
        cmd->configData = i2cCmd->paConfigData;
        cmd->spiCmdWidth = 0;
        cmd->spiCmd = 0;
        cmd->protocol = A2B_CMD_PROTO_I2C;
    } else {
        cmd->deviceAddr = spiCmd->nDeviceAddr;
        cmd->opCode = spiCmd->eOpCode;
        cmd->addrWidth = spiCmd->nAddrWidth;
        cmd->addr = spiCmd->nAddr;
        cmd->dataWidth = spiCmd->nDataWidth;
        cmd->dataCount = spiCmd->nDataCount;
        cmd->configData = spiCmd->paConfigData;
        cmd->spiCmdWidth = spiCmd->nSpiCmdWidth;
        cmd->spiCmd = spiCmd->nSpiCmd;
        cmd->protocol = spiCmd->eProtocol;
    }

    list->cmdPtr = (void *)((uintptr_t)list->cmdPtr + cmdLen);
    list->cmdIdx += cmdLen;
    list->cmdLine++;

    /* SPI Not currently supported */
    if (cmd->protocol == A2B_CMD_PROTO_SPI) {
        return(ADI_A2B_CMDLIST_UNSUPPORTED_PROTOCOL);
    }

    /* Check to see if this access is to the master or slave I2C address */
    if (cmd->deviceAddr == list->cmdListMasterAddr) {
        _mode = ADI_A2B_CMDLIST_MASTER_ACCESS;
    } else if (cmd->deviceAddr == list->cmdListMasterAddr + 1) {
        _mode = ADI_A2B_CMDLIST_SLAVE_ACCESS;
    } else if ((cmd->deviceAddr == 0x00) && (cmd->opCode == A2B_CMD_OP_DELAY)) {
        _mode = ADI_A2B_CMDLIST_NO_ACCESS;
    } else {
        _mode = ADI_A2B_CMDLIST_UNKNOWN_ACCESS;
    }

    /*
     * Pre-decode single register accesses
     */
    if ((cmd->dataCount == 1) && (cmd->addrWidth == 1)) {
        _reg = (uint8_t)(cmd->addr & 0xFF);
        _val = cmd->configData[0];
        _singleReg = true;
    } else {
        _reg = 0xFF;
        _val = 0;
        _singleReg = false;
    }

    /* Override master address if requested */
    if (list->oi.masterAddr_override) {
        if (_mode == ADI_A2B_CMDLIST_MASTER_ACCESS) {
            cmd->deviceAddr = list->oi.masterAddr;
        } else if (_mode == ADI_A2B_CMDLIST_SLAVE_ACCESS) {
            cmd->deviceAddr = list->oi.masterAddr + 1;
        }
    }

    /* Override master registers if requested */
    if ( (_singleReg) &&
         (_mode == ADI_A2B_CMDLIST_MASTER_ACCESS) &&
         (cmd->opCode == A2B_CMD_OP_WRITE) ) {
        if ((_reg == AD24xx_REG_I2SGCFG) && (list->oi.I2SGCFG_override)) {
            _val = list->oi.I2SGCFG;
        } else if ((_reg == AD24xx_REG_I2SCFG) && (list->oi.I2SCFG_override)) {
            _val = list->oi.I2SCFG;
        }
    }

    *mode = _mode; *reg = _reg; *val = _val; *singleReg = _singleReg;

    return(ADI_A2B_CMDLIST_SUCCESS);
}

static uint8_t adi_a2b_cmdlist_master_address(ADI_A2B_CMDLIST *list)
{
    uint8_t masterAddr;

    masterAddr = list->oi.masterAddr_override ?
        list->oi.masterAddr : list->cmdListMasterAddr;

    return(masterAddr);
}

static ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_irq_poll(
    ADI_A2B_CMDLIST *list, uint8_t irqPending, uint32_t timeoutMs)
{
    ADI_A2B_CMDLIST_RESULT result;
    uint32_t start, elapsedMs;
    ADI_A2B_CMDLIST_IRQ_REGS irqRegs;
    bool polling;
    uint8_t masterAddr;

    polling = true;
    start = list->cfg.getTime(list->cfg.usr);
    masterAddr = adi_a2b_cmdlist_master_address(list);
    result = ADI_A2B_CMDLIST_SUCCESS;

    do {

        /* Perform atomic read of interrupt status registers */
        result = adi_a2b_cmdlist_read_ctrl_reg_block(list, AD24xx_REG_INTSRC,
            sizeof(irqRegs), (uint8_t *)&irqRegs, masterAddr);
        if (result != ADI_A2B_CMDLIST_SUCCESS) {
            return(result);
        }

        /* Service a select group of possible interrupts */
        if (irqRegs.intSrc & AD24xx_BITM_INTSRC_MSTINT) {
            switch (irqRegs.intType) {
                case 255:
                    if (irqPending == ADI_A2B_CMDLIST_IRQ_PLL_LOCK_PENDING) {
                        result = ADI_A2B_CMDLIST_SUCCESS;
                        polling = false;
                    }
                    break;
                case 24:
                    if (irqPending == ADI_A2B_CMDLIST_IRQ_DISCOVERY_PENDING) {
                        result = ADI_A2B_CMDLIST_SUCCESS;
                        list->nodesDiscovered++;
                        polling = false;
                    }
                    break;
                case 41:
                    result = ADI_A2B_CMDLIST_A2B_BUS_SHORT_TO_GROUND;
                    polling = false;
                    break;
                case 42:
                    result = ADI_A2B_CMDLIST_A2B_BUS_SHORT_TO_BATT;
                    polling = false;
                    break;
                default:
                    break;
            }
        }

        /* Check polling inverval */
        if (polling) {
            elapsedMs = list->cfg.getTime(list->cfg.usr) - start;
            if (elapsedMs > timeoutMs) {
                result = ADI_A2B_CMDLIST_BUS_TIMEOUT;
                polling = false;
            } else {
                list->cfg.delay(5, list->cfg.usr);
            }
        }

    } while (polling);

    return(result);
}


ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_override(
    ADI_A2B_CMDLIST *list, ADI_A2B_CMDLIST_OVERRIDE_INFO *oi)
{
    if ((list == NULL) || (oi == NULL)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }

    ADI_A2B_CMDLIST_MEMCPY(&list->oi, oi, sizeof(list->oi));

    return(ADI_A2B_CMDLIST_SUCCESS);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_scan(
    ADI_A2B_CMDLIST *list, ADI_A2B_CMDLIST_SCAN_INFO *scan)
{
    ADI_A2B_CMDLIST_CMD cmd;
    ADI_A2B_CMDLIST_ACCESS_TYPE mode;
    ADI_A2B_CMDLIST_RESULT result;
    bool singleReg;
    uint8_t reg;
    uint8_t val;

    if ((list == NULL) || (scan == NULL)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }

    adi_a2b_cmdlist_reset(list);

    ADI_A2B_CMDLIST_MEMSET(scan, 0, sizeof(*scan));

    while (1) {
        result = adi_a2b_cmdlist_next(list, &cmd, &mode, &reg, &val, &singleReg);
        if (result != ADI_A2B_CMDLIST_SUCCESS) {
            break;
        }
        if ( (singleReg) &&
             (mode == ADI_A2B_CMDLIST_MASTER_ACCESS) &&
             (cmd.opCode == A2B_CMD_OP_WRITE) ) {
            if (reg == AD24xx_REG_I2SGCFG) {
                scan->I2SGCFG = val;
                scan->I2SGCFG_valid = true;
            } else if (reg == AD24xx_REG_I2SCFG) {
                scan->I2SCFG = val;
                scan->I2SCFG_valid = true;
            }
        }
    }

    if (result == ADI_A2B_CMDLIST_END) {
        result = ADI_A2B_CMDLIST_SUCCESS;
    }

    return(result);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_get_node_info(
    ADI_A2B_CMDLIST *list, uint8_t node, ADI_A2B_CMDLIST_NODE_INFO *nodeInfo)
{
    if ((list == NULL) || (nodeInfo == NULL) ||
        (node >= list->nodesDiscovered) ||
        (node >= ADI_A2B_CMDLIST_MAX_NODES)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }
    ADI_A2B_CMDLIST_MEMCPY(nodeInfo, &list->nodeInfo[node], sizeof(*nodeInfo));
    return(ADI_A2B_CMDLIST_SUCCESS);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_node_twi_transfer(
    ADI_A2B_CMDLIST *list, uint8_t node,
    bool peripheral, bool broadcast, uint8_t address,
    void *out, uint16_t outLen, void *in, uint16_t inLen)
{
    ADI_A2B_CMDLIST_RESULT result = ADI_A2B_CMDLIST_SUCCESS;
    ADI_A2B_CMDLIST_CFG *cfg = &list->cfg;
    uint8_t masterAddr;
    uint8_t value;

    /* Check parameters */
    if ((list == NULL) ||
        (node >= list->nodesDiscovered) ||
        (node >= ADI_A2B_CMDLIST_MAX_NODES)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }
    if (peripheral && broadcast) {
        return(ADI_A2B_CMDLIST_ERROR);
    }
    if ((out == NULL) && (in == NULL)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }

    /* Get the working master TWI address */
    masterAddr = adi_a2b_cmdlist_master_address(list);

    /* Set the remote node CHIP register if peripheral access */
    if (peripheral) {
        if (result == ADI_A2B_CMDLIST_SUCCESS) {
            result = adi_a2b_cmdlist_write_ctrl_reg(
                list, AD24xx_REG_NODEADR, node & AD24xx_BITM_NODEADR_NODE,
                masterAddr
            );
        }
        if (result == ADI_A2B_CMDLIST_SUCCESS) {
            result = adi_a2b_cmdlist_write_ctrl_reg(
                list, AD24xx_REG_CHIP, address,
                masterAddr + 1
            );
        }
    }

    /* Set the master NODEADR register for this transaction */
    value = node & AD24xx_BITM_NODEADR_NODE;
    value |= peripheral ? AD24xx_BITM_NODEADR_PERI : 0x00;
    value |= broadcast ? AD24xx_BITM_NODEADR_BRCST : 0x00;
    result = adi_a2b_cmdlist_write_ctrl_reg(
        list, AD24xx_REG_NODEADR, value, masterAddr
    );

    /* Perform the TWI transaction */
    if (result == ADI_A2B_CMDLIST_SUCCESS) {
        if (out && in) {
            result = cfg->twiWriteRead(
                cfg->handle, masterAddr + 1,
                out, outLen, in, inLen,
                cfg->usr
            );
        } else if (out) {
            result = cfg->twiWrite(
                cfg->handle, masterAddr + 1,
                out, outLen,
                cfg->usr
            );
        } else {
            result = cfg->twiRead(
                cfg->handle, masterAddr + 1,
                in, inLen,
                cfg->usr
            );
        }
    }

    return(result);
}


ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_set(
    ADI_A2B_CMDLIST *list, uint8_t cmdListMasterAddr,
    void *cmdList, uint32_t cmdListLen, A2B_CMD_TYPE cmdListType)
{
    if ((list == NULL) || (cmdList == NULL) || (cmdListLen == 0)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }
    if ((cmdListType != A2B_CMD_TYPE_I2C) &&
        (cmdListType != A2B_CMD_TYPE_SPI)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }

    list->cmdList = cmdList;
    list->cmdListLen = cmdListLen;
    list->cmdListType = cmdListType;
    list->cmdListMasterAddr = cmdListMasterAddr;

    return(ADI_A2B_CMDLIST_SUCCESS);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_execute(
    ADI_A2B_CMDLIST *list, ADI_A2B_CMDLIST_EXECUTE_INFO *results)
{
    ADI_A2B_CMDLIST_CMD cmd;
    ADI_A2B_CMDLIST_RESULT result;
    ADI_A2B_CMDLIST_ACCESS_TYPE mode;
    bool singleReg;
    uint8_t reg;
    uint8_t val;
    uint32_t delayMs;
    uint8_t irqPending;
    uint8_t readVal;

    adi_a2b_cmdlist_reset(list);

    irqPending = ADI_A2B_CMDLIST_IRQ_NONE;
    result = ADI_A2B_CMDLIST_ERROR;
    delayMs = 0;

    list->nodesDiscovered = 0;

    while (1) {
        result = adi_a2b_cmdlist_next(list, &cmd, &mode, &reg, &val, &singleReg);
        if (result != ADI_A2B_CMDLIST_SUCCESS) {
            break;
        }

        if (mode == ADI_A2B_CMDLIST_UNKNOWN_ACCESS) {
            continue;
        }

        switch (cmd.opCode) {

            /*********************************************************************
             * Write command
             ********************************************************************/
            case A2B_CMD_OP_WRITE:
                if (singleReg) {
                    result = adi_a2b_cmdlist_write_ctrl_reg(list,
                        reg, val, cmd.deviceAddr
                    );
                }
                else {
                    result = adi_a2b_cmdlist_write_ctrl_reg_block(list,
                        cmd.addr, cmd.addrWidth,
                        cmd.dataCount, cmd.configData, cmd.deviceAddr
                    );
                }
                if (result != ADI_A2B_CMDLIST_SUCCESS) {
                    break;
                }
                break;

            /*********************************************************************
             * Read command
             ********************************************************************/
            case A2B_CMD_OP_READ:
                if (singleReg) {
                    result = adi_a2b_cmdlist_read_ctrl_reg(
                        list, reg, &readVal, cmd.deviceAddr
                    );
                    if (result != ADI_A2B_CMDLIST_SUCCESS) {
                        break;
                    }
                } else {
                    result = ADI_A2B_CMDLIST_UNSUPPORTED_DATA_WIDTH;
                    break;
                }
                break;

            /*********************************************************************
             * Delay command
             ********************************************************************/
            case A2B_CMD_OP_DELAY:
                if (cmd.dataCount > 2) {
                    result = ADI_A2B_CMDLIST_UNSUPPORTED_DATA_WIDTH;
                    break;
                }

                /* Calculate the delay */
                delayMs = cmd.configData[0];
                if (cmd.dataCount == 2) {
                    delayMs = (delayMs << 8) + cmd.configData[1];
                }

                /* Do only straight delays here.  More advanced IRQ polling
                 * is done below using the delay as a timeout.
                 */
                if (irqPending == ADI_A2B_CMDLIST_IRQ_NONE) {
                    list->cfg.delay(delayMs, list->cfg.usr);
                }
                break;

        }

        /*
         * Perform more advanced IRQ handling during delay intervals
         */
        if ( (cmd.opCode == A2B_CMD_OP_DELAY) &&
             (irqPending != ADI_A2B_CMDLIST_IRQ_NONE) ) {
            result = adi_a2b_cmdlist_irq_poll(list, irqPending, delayMs);
            if (result != ADI_A2B_CMDLIST_SUCCESS) {
                break;
            }
        }

        /*
         * Track limited slave node information
         */
        if ( (singleReg) &&
             (mode == ADI_A2B_CMDLIST_SLAVE_ACCESS) &&
             (cmd.opCode == A2B_CMD_OP_READ) ) {

            if (list->nodesDiscovered > 0) {
                uint8_t currentNode = list->nodesDiscovered - 1;
                if (currentNode < ADI_A2B_CMDLIST_MAX_NODES) {
                    switch (reg) {
                        case AD24xx_REG_VENDOR:
                            list->nodeInfo[currentNode].vendor  = readVal;
                            break;
                        case AD24xx_REG_PRODUCT:
                            list->nodeInfo[currentNode].product  = readVal;
                            break;
                        case AD24xx_REG_VERSION:
                            list->nodeInfo[currentNode].version  = readVal;
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        /*
         * Track the state of a select set of master register writes
         * for limited error detection and IRQ handling during the next
         * delay interval.
         */
        if ( (singleReg) &&
             (mode == ADI_A2B_CMDLIST_MASTER_ACCESS) &&
             (cmd.opCode == A2B_CMD_OP_WRITE) ) {

            /* Track PLL lock IRQ following reset */
            if ((reg == AD24xx_REG_CONTROL) &&
                (val & AD24xx_BITM_CONTROL_SOFTRST)) {
                irqPending = ADI_A2B_CMDLIST_IRQ_PLL_LOCK_PENDING;

            /* Track discovery IRQ */
            } else if (reg == AD24xx_REG_DISCVRY) {
                irqPending = ADI_A2B_CMDLIST_IRQ_DISCOVERY_PENDING;

            /* Nothing special happening  */
            } else {
                irqPending = ADI_A2B_CMDLIST_IRQ_NONE;
            }
        }

    }

    /* Reaching the end is success */
    if (result == ADI_A2B_CMDLIST_END) {
        result = ADI_A2B_CMDLIST_SUCCESS;
    }

    /* Pass along the results */
    if (results) {
        results->nodesDiscovered = list->nodesDiscovered;
        results->linesProcessed = list->cmdLine;
        results->resultStr = adi_a2b_cmdlist_result_str(result);
    }

    return(result);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_init(void)
{
    ADI_A2B_CMDLIST_MEMSET(cmdList, 0, sizeof(cmdList));
    return(ADI_A2B_CMDLIST_SUCCESS);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_open(
    ADI_A2B_CMDLIST **list, ADI_A2B_CMDLIST_CFG *cfg)
{
    ADI_A2B_CMDLIST *l = NULL;
    ADI_A2B_CMDLIST_RESULT result = ADI_A2B_CMDLIST_ERROR;
    unsigned i;

    if ((list == NULL) || (cfg == NULL)) {
        return(ADI_A2B_CMDLIST_ERROR);
    }

    if ((cfg->twiWrite == NULL) || (cfg->twiWriteRead == NULL) ||
        (cfg->delay == NULL) || (cfg->getTime == NULL) ||
        (cfg->getBuffer == NULL) || (cfg->freeBuffer == NULL)) {
        return(ADI_A2B_CMDLIST_CFG_ERROR);
    }

    ADI_A2B_CMDLIST_ENTER_CRITICAL();
    for (i = 0; i < ADI_A2B_CMDLIST_MAX_LISTS; i++) {
        if (cmdList[i].busy == false) {
            l = &cmdList[i];
            l->busy = true;
            break;
        }
    }
    ADI_A2B_CMDLIST_EXIT_CRITICAL();

    if (l) {
        ADI_A2B_CMDLIST_MEMCPY(&l->cfg, cfg, sizeof(l->cfg));
        result = ADI_A2B_CMDLIST_SUCCESS;
        *list = l;
    } else {
        result = ADI_A2B_CMDLIST_ERROR;
        *list = NULL;
    }

    return(result);
}

ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_close(
    ADI_A2B_CMDLIST **list)
{
    if (list == NULL) {
        return(ADI_A2B_CMDLIST_ERROR);
    }

    ADI_A2B_CMDLIST_ENTER_CRITICAL();
    ADI_A2B_CMDLIST_MEMSET(*list, 0, sizeof(**list));
    ADI_A2B_CMDLIST_EXIT_CRITICAL();
    *list = NULL;

    return(ADI_A2B_CMDLIST_SUCCESS);
}
