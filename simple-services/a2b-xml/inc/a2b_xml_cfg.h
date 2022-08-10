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

#ifndef _a2b_xml_cfg_h
#define _a2b_xml_cfg_h

#include "umm_malloc.h"

#define A2B_XML_USE_SYSLOG

#define A2B_XML_BUFSIZE       4096
#define A2B_XML_REALLOCSIZE   256

#define A2B_XML_MALLOC        umm_malloc
#define A2B_XML_CALLOC        umm_calloc
#define A2B_XML_REALLOC       umm_realloc
#define A2B_XML_FREE          umm_free

#endif
