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

#ifndef _ADI_A2B_CMDLIST_H
#define _ADI_A2B_CMDLIST_H

#include <stdint.h>
#include <stdbool.h>

#include "adi_a2b_commandlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************
 * @brief Command result codes.
 ******************************************************************/
typedef enum _ADI_A2B_CMDLIST_RESULT {
    ADI_A2B_CMDLIST_SUCCESS = 0,
    ADI_A2B_CMDLIST_ERROR,
    ADI_A2B_CMDLIST_CFG_ERROR,
    ADI_A2B_CMDLIST_BUS_ERROR,
    ADI_A2B_CMDLIST_BUS_TIMEOUT,
    ADI_A2B_CMDLIST_ODD_I2C_ADDRESS_ERROR,
    ADI_A2B_CMDLIST_CORRUPT_INIT_FILE,
    ADI_A2B_CMDLIST_UNSUPPORTED_INIT_FILE,
    ADI_A2B_CMDLIST_UNSUPPORTED_READ_LENGTH,
    ADI_A2B_CMDLIST_UNSUPPORTED_DATA_WIDTH,
    ADI_A2B_CMDLIST_UNSUPPORTED_ADDR_BYTES,
    ADI_A2B_CMDLIST_UNSUPPORTED_PROTOCOL,
    ADI_A2B_CMDLIST_A2B_I2C_WRITE_ERROR,
    ADI_A2B_CMDLIST_A2B_I2C_READ_ERROR,
    ADI_A2B_CMDLIST_A2B_MEMORY_ERROR,
    ADI_A2B_CMDLIST_A2B_BUS_POS_SHORT_TO_GROUND,
    ADI_A2B_CMDLIST_A2B_BUS_NEG_SHORT_TO_VBAT,
    ADI_A2B_CMDLIST_A2B_BUS_SHORT_TOGETHER,
    ADI_A2B_CMDLIST_A2B_BUS_OPEN_OR_WRONG_PORT,
    ADI_A2B_CMDLIST_A2B_BUS_REVERSED_OR_WRONG_PORT,
    ADI_A2B_CMDLIST_A2B_BUS_INDETERMINATE_FAULT,
    ADI_A2B_CMDLIST_A2B_BUS_UNKNOWN_FAULT,
    ADI_A2B_CMDLIST_A2B_BUS_NO_FAULT,
    ADI_A2B_CMDLIST_A2B_BUS_SHORT_TO_GROUND,
    ADI_A2B_CMDLIST_A2B_BUS_SHORT_TO_VBAT,
    ADI_A2B_CMDLIST_A2B_BUS_DISCONNECT_OR_OPEN_CIRCUIT,
    ADI_A2B_CMDLIST_A2B_BUS_REVERSE_CONNECTED,
    ADI_A2B_CMDLIST_END
} ADI_A2B_CMDLIST_RESULT;

/*!****************************************************************
 * @brief Opaque A2B command list object handle.
 ******************************************************************/
typedef struct _ADI_A2B_CMDLIST ADI_A2B_CMDLIST;

/*!****************************************************************
 * @brief TWI read call-back.
 *
 * This user defined application function is used to perform a
 * TWI read.
 *
 * @param [in]  twiHandle    User supplied TWI device driver handle
 * @param [in]  address      TWI address of the AD24xx device
 * @param [out] in           Data in buffer pointer
 * @param [in]  inLen        Data in buffer length
 * @param [in]  usr          User supplied data pointer
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *          an error.
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
typedef ADI_A2B_CMDLIST_RESULT (ADI_A2B_CMDLIST_TWI_READ) (
    void *twiHandle, uint8_t address,
    void *in, uint16_t inLen, void *usr
);

/*!****************************************************************
 * @brief TWI write call-back.
 *
 * This user defined application function is used to perform a
 * TWI write.
 *
 * @param [in] twiHandle    User supplied TWI device driver handle
 * @param [in] address      TWI address of the AD24xx device
 * @param [in] out          Data out buffer pointer
 * @param [in] outLen       Data out buffer length
 * @param [in] usr          User supplied data pointer
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *          an error.
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
typedef ADI_A2B_CMDLIST_RESULT (ADI_A2B_CMDLIST_TWI_WRITE) (
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *usr
);

/*!****************************************************************
 * @brief TWI atomic write/read call-back.
 *
 * This user defined application function is used to perform an
 * atomic TWI write/read.
 *
 * @param [in]  twiHandle    User supplied TWI device driver handle
 * @param [in]  address      TWI address of the AD24xx device
 * @param [in]  out          Data out buffer pointer
 * @param [in]  outLen       Data out buffer length
 * @param [out] in           Data in buffer pointer
 * @param [in]  inLen        Data in buffer length
 * @param [in]  usr          User supplied data pointer
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *          an error.
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
typedef ADI_A2B_CMDLIST_RESULT (ADI_A2B_CMDLIST_TWI_WRITE_READ) (
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *in, uint16_t inLen, void *usr
);

/*!****************************************************************
 * @brief TWI atomic write/write call-back.
 *
 * This user defined application function is used to perform an
 * atomic TWI write/write.  This function callback is optional.
 *
 * @param [in]  twiHandle    User supplied TWI device driver handle
 * @param [in]  address      TWI address of the AD24xx device
 * @param [in]  out          Data out buffer pointer
 * @param [in]  outLen       Data out buffer length
 * @param [out] out2         Data out2 buffer pointer
 * @param [in]  out2Len      Data out2 buffer length
 * @param [in]  usr          User supplied data pointer
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *          an error.
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
typedef ADI_A2B_CMDLIST_RESULT (ADI_A2B_CMDLIST_TWI_WRITE_WRITE) (
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *out2, uint16_t out2Len, void *usr
);

/*!****************************************************************
 * @brief Delay function.
 *
 * This user defined application function is used to delay
 * execution.
 *
 * @param [in] mS     Time to delay in milliseconds
 * @param [in] usr    User supplied data pointer
 *
 * @return Nothing
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
typedef void (ADI_A2B_CMDLIST_DELAY) (
    uint32_t mS, void *usr
);

/*!****************************************************************
 * @brief Get time function.
 *
 * This user defined application function is used to get the
 * current time in milliseconds.
 *
 * @param [in] usr    User supplied data pointer
 *
 * @return Returns the current time in milliseconds.
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
typedef uint32_t (ADI_A2B_CMDLIST_GET_TIME) (
    void *usr
);

/*!****************************************************************
 * @brief Get buffer function.
 *
 * This user defined application function is used to get a
 * temporary data transfer buffer.
 *
 * @param [in] size   Data buffer size in bytes
 * @param [in] usr    User supplied data pointer
 *
 * @return Returns a pointer to a data buffer or NULL on error.
 *
 * @sa ADI_A2B_CMDLIST_CFG, ADI_A2B_CMDLIST_FREE_BUFFER
 ******************************************************************/
typedef void *(ADI_A2B_CMDLIST_GET_BUFFER) (
    uint16_t size, void *usr
);

/*!****************************************************************
 * @brief Free buffer function.
 *
 * This user defined application function is used to free a
 * temporary data transfer buffer.
 *
 * @param [in] buffer  Data buffer pointer
 * @param [in] usr     User supplied data pointer
 *
 * @return Nothing.
 *
 * @sa ADI_A2B_CMDLIST_CFG, ADI_A2B_CMDLIST_GET_BUFFER
 ******************************************************************/
typedef void (ADI_A2B_CMDLIST_FREE_BUFFER) (
    void *buffer, void *usr
);

/*!****************************************************************
 * @brief Command list configuration structure
 *
 * This configuration structure is only used during the call to
 * adi_a2b_cmdlist_open() and can be freed by the application
 * immediately thereafter.
 *
 * @sa adi_a2b_cmdlist_open
 ******************************************************************/
typedef struct _ADI_A2B_CMDLIST_CFG {
    /** Pointer to an application TWI read function */
    ADI_A2B_CMDLIST_TWI_WRITE *twiRead;
    /** Pointer to an application TWI write function */
    ADI_A2B_CMDLIST_TWI_WRITE *twiWrite;
    /** Pointer to an application TWI write/read function */
    ADI_A2B_CMDLIST_TWI_WRITE_READ *twiWriteRead;
    /** Optional pointer to an application TWI write/write function.
     * This pointer can be NULL  */
    ADI_A2B_CMDLIST_TWI_WRITE_WRITE *twiWriteWrite;
    /** Pointer to an application delay function */
    ADI_A2B_CMDLIST_DELAY *delay;
    /** Pointer to an application current time function */
    ADI_A2B_CMDLIST_GET_TIME *getTime;
    /** Pointer to an application buffer allocation function */
    ADI_A2B_CMDLIST_GET_BUFFER *getBuffer;
    /** Pointer to an application buffer free function */
    ADI_A2B_CMDLIST_FREE_BUFFER *freeBuffer;
    /** Pointer to an application TWI device handle */
    void *handle;
    /** User data pointer passed back to callback functions */
    void *usr;
} ADI_A2B_CMDLIST_CFG;

/*!****************************************************************
 * @brief Node information structure
 *
 * @sa adi_a2b_cmdlist_get_node_info
 ******************************************************************/
typedef struct _ADI_A2B_CMDLIST_NODE_INFO {
    uint8_t vendor;      /**< Node vendor information */
    uint8_t product;     /**< Node product information */
    uint8_t version;     /**< Node version information */
} ADI_A2B_CMDLIST_NODE_INFO;

/*!****************************************************************
 * @brief Command list scan information
 *
 * @sa adi_a2b_cmdlist_scan
 ******************************************************************/
typedef struct _ADI_A2B_CMDLIST_SCAN_INFO {
    bool I2SGCFG_valid;   /**< True if Master I2SGCFG was found */
    uint8_t I2SGCFG;      /**< Master I2SGCFG register value */
    bool I2SCFG_valid;    /**< True if Master I2SCFG was found */
    uint8_t I2SCFG;       /**< Master I2SCFG register value */
    bool DNSLOTS_valid;   /**< True if Master DNSLOTS was found */
    uint8_t DNSLOTS;      /**< Master DNSLOTS register value */
    bool UPSLOTS_valid;   /**< True if Master UPSLOTS was found */
    uint8_t UPSLOTS;      /**< Master UPSLOTS register value */
    bool SLOTFMT_valid;   /**< True if Master SLOTFMT was found */
    uint8_t SLOTFMT;      /**< Master SLOTFMT register value */
    bool DATCTL_valid;    /**< True if Master DATCTL was found */
    uint8_t DATCTL;       /**< Master DATCTL register value */
} ADI_A2B_CMDLIST_SCAN_INFO;

/*!****************************************************************
 * @brief Command list override information
 *
 * @sa adi_a2b_cmdlist_override
 ******************************************************************/
typedef struct _ADI_A2B_CMDLIST_OVERRIDE_INFO {
    /** True if Master TWI address should be overridden */
    bool masterAddr_override;
    /** Master TWI address override value */
    uint8_t masterAddr;
    /** True if Master I2SGCFG register should be overridden */
    bool I2SGCFG_override;
    /** Master I2SGCFG override value */
    uint8_t I2SGCFG;
    /** True if Master I2SCFG register should be overridden */
    bool I2SCFG_override;
    /** Master I2SCFG override value */
    uint8_t I2SCFG;
    /** True if Master DNSLOTS register should be overridden */
    bool DNSLOTS_override;
    /** Master DNSLOTS override value */
    uint8_t DNSLOTS;
    /** True if Master UPSLOTS register should be overridden */
    bool UPSLOTS_override;
    /** Master UPSLOTS override value */
    uint8_t UPSLOTS;
    /** True if Master SLOTFMT register should be overridden */
    bool SLOTFMT_override;
    /** Master SLOTFMT override value */
    uint8_t SLOTFMT;
    /** True if Master DATCTL register should be overridden */
    bool DATCTL_override;
    /** Master DATCTL override value */
    uint8_t DATCTL;
    /** True if Slave LDNSLOTS register should be overridden */
    bool LDNSLOTS_override;
    /** Slave LDNSLOTS override value */
    uint8_t LDNSLOTS;
    /** True if Slave LUPSLOTS register should be overridden */
    bool LUPSLOTS_override;
    /** Slave LUPSLOTS override value */
    uint8_t LUPSLOTS;
} ADI_A2B_CMDLIST_OVERRIDE_INFO;

/*!****************************************************************
 * @brief Command list execution results
 *
 * @sa adi_a2b_cmdlist_execute
 ******************************************************************/
typedef struct _ADI_A2B_CMDLIST_EXECUTE_INFO {
    /** Result value as a string */
    const char *resultStr;
    /** Number of discovered nodes */
    uint8_t nodesDiscovered;
    /** Fault detected */
    bool faultDetected;
    /** Fault Node */
    int8_t faultNode;
    /** Number of command list lines processed */
    uint32_t linesProcessed;
} ADI_A2B_CMDLIST_EXECUTE_INFO;

/*!****************************************************************
 * @brief Command list module initialization.
 *
 * This function should be called once during system initialization.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_init(void);

/*!****************************************************************
 * @brief Command list open.
 *
 * This function returns a handle to a command list instance.
 *
 * This function is thread safe if FREE_RTOS is defined.
 *
 * @param [in] list  Pointer to a ADI_A2B_CMDLIST handle pointer.  The
 *                   underlying pointer will be filled upon success.
 * @param [in] cfg   Pointer to a ADI_A2B_CMDLIST_CFG structure.  This
 *                   structure will be copied internally and does not
 *                   need to be maintained after this function returns.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa ADI_A2B_CMDLIST_CFG
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_open(
    ADI_A2B_CMDLIST **list, ADI_A2B_CMDLIST_CFG *cfg
);

/*!****************************************************************
 * @brief Command list open.
 *
 * This function closes a command list instance.
 *
 * This function is thread safe if FREE_RTOS is defined.
 *
 * @param [in] list  Pointer to a ADI_A2B_CMDLIST handle pointer.  The
 *                   underlying pointer will be cleared upon success.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_close(
    ADI_A2B_CMDLIST **list
);

/*!****************************************************************
 * @brief Command list set.
 *
 * This function sets a command list.
 *
 * This function is not thread safe.
 *
 * @param [in] list               Pointer to a ADI_A2B_CMDLIST handle.
 * @param [in] cmdListMasterAddr  The TWI master address contained
 *                                within the ADI A2B command list.  The
 *                                normal SigmaStudio default value is 0x68.
 * @param [in] cmdList            Pointer to a SigmaStudio 'gaA2BConfig'
 *                                adi_a2b_i2c_commandlist.h export.
 * @param [in] cmdListLen         Length of the SigmaStudio 'CONFIG_LEN'
 *                                adi_a2b_i2c_commandlist.h export.
 * @param [in] cmdListType        Type of SigmaStudio export.  Use
 *                                'A2B_CMD_TYPE_I2C' for SigmaStudio
 *                                A2B plugin versions prior 19.9.0 and
 *                                'A2B_CMD_TYPE_SPI' for version 19.9.0
 *                                and after.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_set(
    ADI_A2B_CMDLIST *list, uint8_t cmdListMasterAddr,
    void *cmdList, uint32_t cmdListLen, A2B_CMD_TYPE cmdListType
);

/*!****************************************************************
 * @brief Command list execute.
 *
 * This function intelligently executes an A2B discovery
 * command list.
 *
 * This function is not thread safe.
 *
 * @param [in]  list    Pointer to a ADI_A2B_CMDLIST handle.
 * @param [out] results Pointer to a ADI_A2B_CMDLIST_EXECUTE_INFO result
 *                      structure.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa ADI_A2B_CMDLIST_EXECUTE_INFO
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_execute(
    ADI_A2B_CMDLIST *list, ADI_A2B_CMDLIST_EXECUTE_INFO *results
);

/*!****************************************************************
 * @brief Command list play.
 *
 * This function plays a command list.  The list is played exactly
 * as-is with no processing.  Meant to be used in place of
 * adi_a2b_cmdlist_execute().
 *
 * USE WITH CAUTION!
 *
 * This function is not thread safe.
 *
 * @param [in]  list    Pointer to a ADI_A2B_CMDLIST handle.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa ADI_A2B_CMDLIST_EXECUTE_INFO
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_play(ADI_A2B_CMDLIST *list);

/*!****************************************************************
 * @brief Command list override.
 *
 * This function overrides defined parts of a command list.
 *
 * This function is not thread safe.
 *
 * @param [in]  list  Pointer to a ADI_A2B_CMDLIST handle.
 * @param [in]  oi    Pointer to a ADI_A2B_CMDLIST_OVERRIDE_INFO
 *                    override structure.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa ADI_A2B_CMDLIST_OVERRIDE_INFO
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_override(
    ADI_A2B_CMDLIST *list,
    ADI_A2B_CMDLIST_OVERRIDE_INFO *oi
);

/*!****************************************************************
 * @brief Command list scan.
 *
 * This function scans for interesting parts of a command list.
 *
 * This function is not thread safe.
 *
 * @param [in]  list  Pointer to a ADI_A2B_CMDLIST handle.
 * @param [in]  scan  Pointer to a ADI_A2B_CMDLIST_SCAN_INFO
 *                    structure.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa ADI_A2B_CMDLIST_SCAN_INFO
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_scan(
    ADI_A2B_CMDLIST *list, ADI_A2B_CMDLIST_SCAN_INFO *scan
);

/*!****************************************************************
 * @brief Retrieve A2B slave node info.
 *
 * This function returns slave node information after executing
 * a command list.
 *
 * This function is not thread safe.
 *
 * @param [in]  list      Pointer to a ADI_A2B_CMDLIST handle.
 * @param [in]  node      Node index.  First node is index zero.
 * @param [out] nodeInfo  Pointer to a ADI_A2B_CMDLIST_NODE_INFO
 *                        structure.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 * @sa ADI_A2B_CMDLIST_NODE_INFO
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_get_node_info(
    ADI_A2B_CMDLIST *list, uint8_t node, ADI_A2B_CMDLIST_NODE_INFO *nodeInfo
);

/*!****************************************************************
 * @brief Performs TWI operations on a remote node
 *
 * This function performs TWI read, write, and write/read
 * operations on remote nodes.
 *
 * The type of operation will be inferred from the 'out' and 'in'
 * pointers as follows:
 *
 *   'out'   'in'    Operation
 *   ------  ------  --------------
 *   NULL    SET     Read
 *   SET     NULL    Write
 *   SET     SET     Write/Read  (io == true)
 *   SET     SET     Write/Write (io == false)
 *
 * This function is thread safe if the underlying TWI callbacks
 * are thread safe.
 *
 * @param [in]  list        Pointer to a ADI_A2B_CMDLIST handle.
 * @param [in]  node        Node index.  First node is index zero.
 * @param [in]  peripheral  Operation is a node peripheral access otherwise
 *                          a slave AD24xx register access
 * @param [in]  broadcast   Operation is a broadcast operation ('peripheral'
 *                          must be false if this is set)
 * @param [in]  address     Slave peripheral TWI address used when
 *                          'peripheral' is true.
 * @param [in]  out         Data out buffer pointer
 * @param [in]  outLen      Data out buffer length
 * @param [out] io          Data IO buffer pointer
 * @param [in]  ioLen       Data IO buffer length
 * @param [in]  ioDir       Data IO type.  Input when true.
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_node_twi_transfer(
    ADI_A2B_CMDLIST *list, uint8_t node,
    bool peripheral, bool broadcast, uint8_t address,
    void *out, uint16_t outLen, void *io, uint16_t ioLen, bool ioDir
);

/*!****************************************************************
 * @brief Allows access to the command list delay mechanism
 *
 * This function delays for some number of milliseconds.
 *
 * This function is not thread safe.
 *
 * @param [in]  list      Pointer to a ADI_A2B_CMDLIST handle.
 * @param [in]  ms        Delay in milliseconds
 *
 * @return Returns ADI_A2B_CMDLIST_SUCCESS if successful, otherwise
 *         an error.
 *
 ******************************************************************/
ADI_A2B_CMDLIST_RESULT adi_a2b_cmdlist_delay(
    ADI_A2B_CMDLIST *list, uint32_t ms
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
