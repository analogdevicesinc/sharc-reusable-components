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

/*!
 * @brief  SigmaStudio A2B config XML format conversion module
 *
 * This module converts an SigmaStudio network configuration exported
 * in XML format to the same binary format as found in
 * 'adi_a2b_i2c_commandlist.h'
 *
 * @file      adi_xml.h
 * @version   1.0.0
 * @copyright 2019 Analog Devices, Inc.  All rights reserved.
 *
*/
#ifndef _a2b_xml_h
#define _a2b_xml_h

#include <stdint.h>

#include "adi_a2b_commandlist.h"

/*!****************************************************************
 *  @brief Convert a SigmaStudio A2B config file in XML format to
 *         binary.
 *
 * This function loads and converts an A2B config file in XML format
 * to the same binary format as found in 'adi_a2b_i2c_commandlist.h'.
 *
 * @param [in]  filename   A2B XML file to load
 * @param [out] cfg        Pointer to config binary in memory
 * @param [out] type       A2B XML file protocol type
 *
 * @return Returns size of the binary in bytes if successful,
 *         otherwise zero.
 ******************************************************************/
uint32_t a2b_xml_load(const char *filename, void **cfg,
    A2B_CMD_TYPE *type);

/*!****************************************************************
 *  @brief Frees the binary created by a2b_xml_load.
 *
 * This function frees the memory allocated for the binary returned
 * by 'a2b_xml_load'.
 *
 * @param [in]  cfg     Binary A2B config
 * @param [in]  cfgLen  Size of the binary in bytes
 * @param [in]  type    A2B XML file protocol type
 *
 * @return Returns size of the binary in bytes if successful,
 *         otherwise zero.
 ******************************************************************/
void a2b_xml_free(void *cfg, uint32_t cfgLen,
    A2B_CMD_TYPE type);

#endif
