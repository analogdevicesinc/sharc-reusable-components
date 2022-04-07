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

#ifndef _SHELL_CFG_H
#define _SHELL_CFG_H

#include "umm_malloc.h"

/*! @cond */

#define SHELL_MAX_ARGS          10
#define SHELL_WELCOMEMSG         "SAM Audio Starter\n" \
                                 "Version: %s (%s %s)\n"
#define SHELL_PROMPT            "# "
#define SHELL_ERRMSG            "Invalid command, type 'help' for help\n"
#define SHELL_MAX_LINE_LEN      79
#define SHELL_COLUMNS           80
#define SHELL_LINES             25
#define SHELL_MAX_HISTORIES     50
#define SHELL_MALLOC            umm_malloc
#define SHELL_FREE              umm_free
#define SHELL_STRDUP            shell_strdup
#define SHELL_STRNDUP           shell_strndup

/*! @endcond example-config */

#endif
