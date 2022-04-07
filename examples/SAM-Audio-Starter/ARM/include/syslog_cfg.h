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

#ifndef _SYSLOG_CFG_H
#define _SYSLOG_CFG_H

#include "umm_malloc.h"

/*! @cond */

#define SYSLOG_NAME        "Console Log"

#define SYSLOG_HEAP        UMM_SDRAM_HEAP
#define SYSLOG_MALLOC(x)   umm_malloc_heap(SYSLOG_HEAP, x)
#define SYSLOG_FREE(x)     umm_free_heap(SYSLOG_HEAP, x)

#define SYSLOG_LINE_MAX    (128)
#define SYSLOG_MAX_LINES   (4096)

/*! @endcond example-config */

#endif
