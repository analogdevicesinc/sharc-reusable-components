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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "yxml.h"
#include "a2b_xml_cfg.h"
#include "a2b_xml.h"
#include "adi_a2b_commandlist.h"

#ifdef A2B_XML_USE_SYSLOG
#include "syslog.h"
#endif

/*!****************************************************************
 * @brief  The size of the XML parser and file read buffers.
 *         bigger is faster but less memory efficient.
 ******************************************************************/
#ifndef A2B_XML_BUFSIZE
#define A2B_XML_BUFSIZE       1024
#endif

/*!****************************************************************
 * @brief  The realloc size increase increment used when parsing
 *         XML strings and generating config binaries.  Bigger is
 *         faster but less memory efficient.  Cannot be less than
 *         64.
 ******************************************************************/
#ifndef A2B_XML_REALLOCSIZE
#define A2B_XML_REALLOCSIZE   128
#endif

/*!****************************************************************
 * @brief  The function used to allocate memory for the module.
 *
 * This defaults to the standard C library malloc if not defined.
 ******************************************************************/
#ifndef A2B_XML_MALLOC
#define A2B_XML_MALLOC        malloc
#endif

/*!****************************************************************
 * @brief  The function used to allocate zero'd memory for the
 *         module.
 *
 * This defaults to the standard C library calloc if not defined
 ******************************************************************/
#ifndef A2B_XML_CALLOC
#define A2B_XML_CALLOC        calloc
#endif

/*!****************************************************************
 * @brief  The function used to reallocate memory for the module.
 *
 * This defaults to the standard C library realloc if not defined
 ******************************************************************/
#ifndef A2B_XML_REALLOC
#define A2B_XML_REALLOC       realloc
#endif

/*!****************************************************************
 * @brief  The function used to free memory for the module.
 *
 * This defaults to the standard C library free if not defined
 ******************************************************************/
#ifndef A2B_XML_FREE
#define A2B_XML_FREE          free
#endif

/*!****************************************************************
 * @brief  The function used to fill memory for the module.
 *
 * This defaults to the standard C library memset if not defined
 ******************************************************************/
#ifndef A2B_XML_MEMSET
#define A2B_XML_MEMSET        memset
#endif

/* Container for the attributes and data of an XML "action" tag */
typedef struct {
    /* Common parameters, all formats */
    char *i2caddr;
    char *instr;
    char *addr_width;
    char *addr;
    char *data_width;
    char *len;
    char *data;
    /* AD243x SPI additions, Sigma Studio version 19.9.0+ */
    char *spiCmd;
    char *spiCmdWidth;
    char *protocol;
} A2B_XML_ACTION;

/*****************************************************************************
 * dstring()
 *
 * Manages a 'dynamic' string that grows on the heap in 'increment' size
 * chunks.  Attemps to gracefully handle and report realloc() errors.
 ****************************************************************************/
static void *dstring(char **strPtr, unsigned *lenPtr, unsigned *allocLenPtr,
    unsigned increment)
{
    void *str = *strPtr;
    unsigned allocLen = *allocLenPtr;
    void *p;

    allocLen += increment;
    p = A2B_XML_REALLOC(str, allocLen);
    if (p == NULL) {
        A2B_XML_FREE(*strPtr);
        *strPtr = NULL;
        *lenPtr = 0;
        *allocLenPtr = 0;
    } else {
        *strPtr = p;
        *allocLenPtr = allocLen;
    }

    return(p);
}

/*****************************************************************************
 * hex2nibble()
 *
 * Converts a hex character to it's equivalent nibble value
 ****************************************************************************/
static inline unsigned char hex2nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return(c - 'a' + 10);
    }
    return(0);
}

/*****************************************************************************
 * strtoarray()
 *
 * Allocates, fills, and returns an array of length 'len' with the contents
 * of a SigmaStudio hex array string 'data'.  The length must already be
 * known.
 ****************************************************************************/
static unsigned char *strtoarray(const char *data, unsigned short len)
{
    unsigned char *array;
    unsigned short idx;

    array = A2B_XML_CALLOC(len, sizeof(unsigned char));

    idx = 0;
    while ((*data) && (idx < len)) {
        array[idx] = (hex2nibble(*data) << 4) | hex2nibble(*(data+1));
        idx++; data += 2;
        if (*data == ' ') {
            data++;
        }
    }

    return(array);
}

/*****************************************************************************
 * action2config()
 *
 * Converts an XML 'action' tag to a binary 'ADI_A2B_DISCOVERY_CONFIG' struct
 * as found in 'adi_a2b_i2c_commandlist.h'.
 ****************************************************************************/
static void *action2config(A2B_XML_ACTION *action,
    void *cfg, uint32_t *cfgLenPtr, uint32_t *cfgAllocLenPtr,
    A2B_CMD_TYPE type)
{
    uint32_t cfgLen = *cfgLenPtr;
    uint32_t cfgAllocLen = *cfgAllocLenPtr;
    uint8_t *cfgPtr;
    size_t actionLen;
    void *p;

    A2B_CMD *i2cCmd = NULL;
    A2B_CMD_SPI spiCmd;

    if (!action || (action->instr == NULL) || (type == A2B_CMD_TYPE_UNKNOWN)) {
        return(cfg);
    }

    if (type == A2B_CMD_TYPE_I2C) {
        actionLen = sizeof(*i2cCmd);
    } else {
        actionLen = sizeof(spiCmd);
    }

    if ((cfgLen + actionLen) > cfgAllocLen) {
        cfgAllocLen += A2B_XML_REALLOCSIZE;
        p = A2B_XML_REALLOC(cfg, cfgAllocLen);
        if (p) {
            cfg = p;
        } else {
            A2B_XML_FREE(cfg);
            cfg = NULL;
            cfgLen = 0;
        }
    }

    memset(&spiCmd, 0, sizeof(spiCmd));

    if (strcmp("writeXbytes", action->instr) == 0) {
        spiCmd.eOpCode = A2B_CMD_OP_WRITE;
    } else if (strcmp("delay", action->instr) == 0) {
        spiCmd.eOpCode = A2B_CMD_OP_DELAY;
    } else if (strcmp("read", action->instr) == 0) {
        spiCmd.eOpCode = A2B_CMD_OP_READ;
    } else {
        spiCmd.eOpCode = A2B_CMD_OP_INVALID;
    }


    if (cfg) {
        if (action->i2caddr) {
            spiCmd.nDeviceAddr = strtol(action->i2caddr, NULL, 0);
        }
        if (action->addr_width) {
            spiCmd.nAddrWidth = strtol(action->addr_width, NULL, 0);
        }
        if (action->addr) {
            spiCmd.nAddr = strtol(action->addr, NULL, 0);
        }
        if (action->data_width) {
            spiCmd.nDataWidth = strtol(action->data_width, NULL, 0);
        }
        if (action->len) {
            spiCmd.nDataCount = strtol(action->len, NULL, 0) - spiCmd.nAddrWidth;
        } else {
            spiCmd.nDataCount = (strlen(action->data) + 1) / 3;
        }
        if (action->data) {
            spiCmd.paConfigData = strtoarray(action->data, spiCmd.nDataCount);
        }
        if (action->i2caddr) {
            spiCmd.nDeviceAddr = strtol(action->i2caddr, NULL, 0);
        }
        if (action->spiCmdWidth) {
            spiCmd.nSpiCmdWidth = strtol(action->spiCmdWidth, NULL, 0);
        }
        if (action->spiCmd) {
            spiCmd.nSpiCmd = strtol(action->spiCmd, NULL, 0);
        }
        if (action->protocol) {
            if (strcmp(action->protocol, "I2C") == 0) {
                spiCmd.eProtocol = A2B_CMD_PROTO_I2C;
            } else {
                spiCmd.eProtocol = A2B_CMD_PROTO_SPI;
            }
        } else {
            spiCmd.eProtocol = A2B_CMD_PROTO_I2C;
        }

        cfgPtr = (uint8_t *)cfg + cfgLen;
        if (type == A2B_CMD_TYPE_I2C) {
            i2cCmd = (A2B_CMD *)cfgPtr;
            i2cCmd->eOpCode = spiCmd.eOpCode;
            i2cCmd->nDeviceAddr = spiCmd.nDeviceAddr;
            i2cCmd->nAddrWidth = spiCmd.nAddrWidth;
            i2cCmd->nAddr = spiCmd.nAddr;
            i2cCmd->nDataWidth = spiCmd.nDataWidth;
            i2cCmd->nDataCount = spiCmd.nDataCount;
            i2cCmd->paConfigData = spiCmd.paConfigData;
        } else {
            memcpy(cfgPtr, &spiCmd, actionLen);
        }

        cfgLen += actionLen;
    }

    *cfgLenPtr = cfgLen;
    *cfgAllocLenPtr = cfgAllocLen;

    return(cfg);
}

/*****************************************************************************
 * a2b_xml_free()
 *
 * Frees the memory associated with a binary A2B config.
 ****************************************************************************/
void a2b_xml_free(void *c, uint32_t cfgLen,
    A2B_CMD_TYPE type)
{
    A2B_CMD *i2cRow;
    A2B_CMD_SPI *spiRow;
    uint32_t numRows;
    uint32_t i;


    if (type == A2B_CMD_TYPE_I2C) {
        numRows = cfgLen / sizeof(*i2cRow);
    } else if (type == A2B_CMD_TYPE_SPI) {
        numRows = cfgLen / sizeof(*spiRow);
    } else {
        return;
    }

    i2cRow = (A2B_CMD *)c;
    spiRow = (A2B_CMD_SPI *)c;

    for (i = 0; i < numRows; i++) {
        if (type == A2B_CMD_TYPE_I2C) {
            if (i2cRow->paConfigData) {
                A2B_XML_FREE(i2cRow->paConfigData);
            }
            i2cRow++;
        } else {
            if (spiRow->paConfigData) {
                A2B_XML_FREE(spiRow->paConfigData);
            }
            spiRow++;
        }
    }

    A2B_XML_FREE(c);
}

/*****************************************************************************
 * a2b_xml_load()
 *
 * Loads and converts an A2B config file in XML format to the same binary
 * format as found in 'adi_a2b_i2c_commandlist.h'.
 *
 * WARNING: This function makes extensive use of the realloc() function!
 ****************************************************************************/
uint32_t a2b_xml_load(const char *filename, void **c,
    A2B_CMD_TYPE *type)
{
    FILE *handle;
    yxml_t *x;

    char *buf;
    uint32_t len;
    bool parseError;
    yxml_ret_t r;
    unsigned i;

    char **attrVal;
    unsigned attrLen;
    unsigned attrAllocLen;

    unsigned elemLen;
    unsigned elemAllocLen;

    A2B_XML_ACTION *action;
    void *cfg;
    uint32_t cfgLen;
    uint32_t cfgAllocLen;
    A2B_CMD_TYPE cfgType;

    /* Open the XML file for reading */
    handle = fopen(filename, "r" );
    if (!handle) {
#ifdef A2B_XML_USE_SYSLOG
        syslog_printf("Unable to open \"%s\" config file", filename);
#endif
        *c = NULL;
        return 0;
    }

    /* Allocate a buffer for XML file data */
    buf = A2B_XML_MALLOC(A2B_XML_BUFSIZE);
    if (buf == NULL) {
        *c = NULL;
        return 0;
    }

    /* Allocate an instance and stack area for the 'yxml' XML
     * parser library
     */
    x = A2B_XML_MALLOC(sizeof(yxml_t) + A2B_XML_BUFSIZE);
    if (x == NULL) {
        A2B_XML_FREE(buf);
        *c = NULL;
        return 0;
    }

    /* Initialize the state tracking variables */
    parseError = false;
    r = YXML_OK;
    attrVal = NULL;
    attrLen = 0;
    attrAllocLen = 0;
    elemLen = 0;
    elemAllocLen = 0;
    action = NULL;
    cfg = NULL;
    cfgLen = 0;
    cfgAllocLen = 0;
    cfgType = A2B_CMD_TYPE_UNKNOWN;

    /* Initialize the 'yxml' XML parsing library */
    yxml_init(x, x+1, A2B_XML_BUFSIZE);

    /* Read the initial chunk of the XML file */
    len = fread(buf, sizeof(*buf), A2B_XML_BUFSIZE, handle);

    /* Parse and process the file until complete or a parsing error */
    while (!parseError && (len > 0)) {

        /* Iterate through the file buffer character by character */
        for (i = 0; (i < len) && (parseError == false); i++) {

            /* Run the character through the parser */
            r = yxml_parse(x, buf[i]);

            /* Process parser result codes */
            if (r == YXML_OK) {
                continue;
            } else if (r > YXML_OK) {
                switch (r) {

                    /* Look for an 'action' tag in the XML file */
                    case YXML_ELEMSTART:
                        if (strcmp("action", x->elem) == 0) {
                            /* Allocate a new 'action' container */
                            action = A2B_XML_CALLOC(1, sizeof(A2B_XML_ACTION));
                            if (action == NULL) {
                                parseError = true;
                                break;
                            }
                            /* Allocate the hex data string */
                            action->data = dstring(&action->data, &elemLen,
                                &elemAllocLen, A2B_XML_REALLOCSIZE);
                            if (action->data) {
                                action->data[0] = 0;
                                elemLen = 1;
                            }
                        }
                        break;

                    /* Consume 'action' data growing the data string as
                     * necessary
                     */
                    case YXML_CONTENT:
                        if (action && action->data) {
                            elemLen += strlen(x->data);
                            if (elemLen > elemAllocLen) {
                                action->data = dstring(&action->data, &elemLen,
                                    &elemAllocLen, A2B_XML_REALLOCSIZE);
                            }
                            if (action->data) {
                                strcat(action->data, x->data);
                            }
                        }
                        break;

                    /* Convert and append 'action' data and attributes into a
                     * binary 'config' struct entry.  Free all 'action' data and
                     * attribute string.
                     */
                    case YXML_ELEMEND:
                        if (action) {
                            if (cfgType == A2B_CMD_TYPE_UNKNOWN) {
                                if (action->protocol) {
                                    cfgType = A2B_CMD_TYPE_SPI;
                                } else {
                                    cfgType = A2B_CMD_TYPE_I2C;
                                }
                            }
                            cfg = action2config(action, cfg, &cfgLen, &cfgAllocLen, cfgType);
                            if (cfg == NULL) {
                                parseError = true;
                            }
                            if (action->i2caddr) {
                                A2B_XML_FREE(action->i2caddr);
                            }
                            if (action->instr) {
                                A2B_XML_FREE(action->instr);
                            }
                            if (action->addr_width) {
                                A2B_XML_FREE(action->addr_width);
                            }
                            if (action->addr) {
                                A2B_XML_FREE(action->addr);
                            }
                            if (action->data_width) {
                                A2B_XML_FREE(action->data_width);
                            }
                            if (action->len) {
                                A2B_XML_FREE(action->len);
                            }
                            if (action->data) {
                                A2B_XML_FREE(action->data);
                            }
                            if (action->spiCmd) {
                                A2B_XML_FREE(action->spiCmd);
                            }
                            if (action->spiCmdWidth) {
                                A2B_XML_FREE(action->spiCmdWidth);
                            }
                            if (action->protocol) {
                                A2B_XML_FREE(action->protocol);
                            }
                            A2B_XML_FREE(action);
                            action = NULL;
                            elemLen = elemAllocLen = 0;
                        }
                        break;

                    /* Capture interesting 'action' attribute strings */
                    case YXML_ATTRSTART:
                        if (action) {
                            if (strcmp("i2caddr", x->attr) == 0) {
                                attrVal = &action->i2caddr;
                            } else if (strcmp("instr", x->attr) == 0) {
                                attrVal = &action->instr;
                            } else if (strcmp("addr_width",  x->attr) == 0) {
                                attrVal = &action->addr_width;
                            } else if (strcmp("addr", x->attr) == 0) {
                                attrVal = &action->addr;
                            } else if (strcmp("data_width", x->attr) == 0) {
                                attrVal = &action->data_width;
                            } else if (strcmp("len", x->attr) == 0) {
                                attrVal = &action->len;
                            } else if (strcmp("SpiCmd", x->attr) == 0) {
                                attrVal = &action->spiCmd;
                            } else if (strcmp("SpiCmdWidth", x->attr) == 0) {
                                attrVal = &action->spiCmdWidth;
                            } else if (strcmp("Protocol", x->attr) == 0) {
                                attrVal = &action->protocol;
                            } else {
                                attrVal = NULL;
                            }
                            /* Allocate a new attribute string */
                            if (attrVal) {
                                *attrVal = dstring(attrVal, &attrLen, &attrAllocLen, A2B_XML_REALLOCSIZE);
                                if (*attrVal) {
                                    *attrVal[0] = 0;
                                    attrLen = 1;
                                }
                            }
                        }
                        break;

                    /* Consume 'action' attribute data growing the attribute string as
                     * necessary
                     */
                    case YXML_ATTRVAL:
                        if (action && attrVal) {
                            attrLen += strlen(x->data);
                            if (attrLen > attrAllocLen) {
                                *attrVal = dstring(attrVal, &attrLen, &attrAllocLen, A2B_XML_REALLOCSIZE);
                            }
                            if (*attrVal) {
                                strcat(*attrVal, x->data);
                            }
                        }
                        break;

                    /* Reset the 'action' attribute state */
                    case YXML_ATTREND:
                        if (action && attrVal) {
                            attrVal = NULL;
                            attrLen = attrAllocLen = 0;
                        }
                        break;

                    default:
                        break;
                }
            } else {
                parseError = true;
                break;
            }
        }

        /* Read the next chunk of XML data */
        len = fread(buf, sizeof(*buf), A2B_XML_BUFSIZE, handle);
    }

    /* If no parsing errors were found, close the 'yxml' library */
    if (!parseError) {
        r = yxml_eof(x);
        if (r < 0) {
            parseError = true;
        }
    }

    /* Make one last check for errors */
    if (parseError) {
        if (cfg) {
            A2B_XML_FREE(cfg);
            cfg = NULL;
            cfgLen = 0;
        }
#ifdef A2B_XML_USE_SYSLOG
        syslog_printf("Error parsing '%s' at:%"PRIu32":%"PRIu64" byte offset %"PRIu64,
          filename, x->line, x->byte, x->total);
#endif
    }

    /* Close the XML file */
    fclose(handle);

    /* Free buffers */
    A2B_XML_FREE(x);
    A2B_XML_FREE(buf);

    /* Return the results */
    if (type) {
        *type = cfgType;
    }
    *c = cfg;
    return(cfgLen);
}

