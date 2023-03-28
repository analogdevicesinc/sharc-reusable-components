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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "shell_printf.h"
#include "term.h"
#include "shell.h"

int shell_printf(SHELL_CONTEXT *ctx, const char* fmt, ...)
{
    va_list va;
    char *str;
    va_start(va, fmt);
    int len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);
    va_start(va, fmt);
    str = SHELL_MALLOC(len + 1);
    vsnprintf(str, len + 1, fmt, va);
    va_end(va);
    term_putstr(&ctx->t, str, len);
    SHELL_FREE(str);
    return(len);
}
