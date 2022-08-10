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
 * @brief  Simple system logger
 *
 * This logger supports FreeRTOS and bare-metal projects.
 *
 * @file      syslog.h
 * @version   1.0.0
 * @copyright 2018 Analog Devices, Inc.  All rights reserved.
 *
*/

#include "syslog_cfg.h"

#ifndef SYSLOG_H_
#define SYSLOG_H_

/*!****************************************************************
 * @brief  The maximum number of entries in the syslog FIFO
 ******************************************************************/
#ifndef SYSLOG_MAX_LINES
#define SYSLOG_MAX_LINES   (1000)
#endif

/*!****************************************************************
 * @brief  The maximum line length of a syslog entry
 ******************************************************************/
#ifndef SYSLOG_LINE_MAX
#define SYSLOG_LINE_MAX    (128)
#endif

/*!****************************************************************
 * @brief  The function used to allocate memory for the syslog
 * buffer.
 *
 * This defaults to the standard C library malloc if not defined
 * otherwise.
 ******************************************************************/
#ifndef SYSLOG_MALLOC
#define SYSLOG_MALLOC(x)   malloc(x)
#endif

/*!****************************************************************
 * @brief  The function used to free memory.
 *
 * This defaults to the standard C library free if not defined
 * otherwise.
 ******************************************************************/
#ifndef SYSLOG_FREE
#define SYSLOG_FREE (x)    free(x)
#endif

/*!****************************************************************
 * @brief  The name of the syslog instance.
 ******************************************************************/
#ifndef SYSLOG_NAME
#define SYSLOG_NAME        "Console Log"
#endif

/*!****************************************************************
 * @brief  System log init
 *
 * This function initializes the system logger.
 *
 * This function can be called before or after the FreeRTOS is
 * started.  The logger will allocate the log buffer using
 * the function defined by SYSLOG_MALLOC
 *
 ******************************************************************/
void syslog_init(void);

/*!****************************************************************
 * @brief  System log print
 *
 * This function prints a fixed string to the system log.  Strings
 * which do not fit within the log line length will be truncated.
 *
 * This function is thread safe.
 *
 * @param [in]   msg     Null-terminated string to save in the log
 *
 ******************************************************************/
void syslog_print(char *msg);

/*!****************************************************************
 * @brief  System log printf
 *
 * This function prints a variable argument string to the system
 * log.  Strings which do not fit within the log line length
 * will be truncated.
 *
 * This function is thread safe.
 *
 * @param [in]  fmt   Null-terminated format string
 * @param [in]  ...   Remaining arguments
 *
 ******************************************************************/
void syslog_printf(char *fmt, ...);

/*!****************************************************************
 * @brief  System log dump
 *
 * This function dumps the contents of the system log to stdout.
 *
 * This function is thread safe.
 *
 ******************************************************************/
void syslog_dump(unsigned max);

#endif
