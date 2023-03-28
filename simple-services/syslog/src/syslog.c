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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>

#include "syslog.h"

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"

    #define SYSLOG_CRITICAL_ENTRY(x) \
        xSemaphoreTake(x, portMAX_DELAY);

    #define SYSLOG_CRITICAL_EXIT(x) \
        xSemaphoreGive(x)
#else
    #define SYSLOG_CRITICAL_ENTRY(x)
    #define SYSLOG_CRITICAL_EXIT(x)
#endif

/* TODO: abstract out mutex create/destroy/type */

typedef struct _SYSLOG_CONTEXT {
#ifdef FREE_RTOS
    SemaphoreHandle_t lock;
#endif
    char *name;
    uint32_t headIdx;
    uint32_t tailIdx;
    uint32_t seq;
    int32_t logDropped;
    uint64_t *tsData;
    char *logData;
} SYSLOG_CONTEXT;

SYSLOG_CONTEXT consoleLog = {
    .name = SYSLOG_NAME
};

void syslog_init(void)
{
    SYSLOG_CONTEXT *log = &consoleLog;

#ifdef FREE_RTOS
    log->lock = xSemaphoreCreateMutex();
#endif
    log->headIdx = 0;
    log->tailIdx = 0;
    log->logDropped = 0;
    log->seq = 0;
    log->logData = SYSLOG_MALLOC(SYSLOG_MAX_LINES * SYSLOG_LINE_MAX);
    log->tsData = SYSLOG_MALLOC(SYSLOG_MAX_LINES * sizeof(*(log->tsData)));
}

void syslog_print(char *msg)
{
    SYSLOG_CONTEXT *log = &consoleLog;
    char *start, *end;

    SYSLOG_CRITICAL_ENTRY(log->lock);

    log->headIdx++;
    if (log->headIdx == SYSLOG_MAX_LINES) {
        log->headIdx = 0;
    }

    if (log->headIdx == log->tailIdx) {
        log->tailIdx++;
        if (log->tailIdx == SYSLOG_MAX_LINES) {
            log->tailIdx = 0;
        }
        log->logDropped++;
    }

    start = &log->logData[log->headIdx * SYSLOG_LINE_MAX];
    end = start + SYSLOG_LINE_MAX - 1;
    strncpy(start, msg, SYSLOG_LINE_MAX); *end = 0;
#ifdef FREE_RTOS
    log->tsData[log->headIdx] = xTaskGetTickCount() / portTICK_PERIOD_MS;
#else
    log->tsData[log->headIdx] = log->seq;
#endif

    log->seq++;

    SYSLOG_CRITICAL_EXIT(log->lock);
}

void syslog_printf(char *fmt, ...)
{
    char line[SYSLOG_LINE_MAX];
    va_list args;

    va_start(args, fmt);
    vsnprintf(line, SYSLOG_LINE_MAX, fmt, args);
    va_end(args);

    syslog_print(line);
}

void syslog_dump(unsigned max)
{
    SYSLOG_CONTEXT *log = &consoleLog;
    uint64_t ts;
    char *start;
    char *end;
    char *line;
    unsigned count = 0;

    line = SYSLOG_MALLOC(SYSLOG_LINE_MAX);
    SYSLOG_CRITICAL_ENTRY(log->lock);
    while ((log->tailIdx != log->headIdx) && (count < max)) {
        log->tailIdx++;
        if (log->tailIdx == SYSLOG_MAX_LINES) {
            log->tailIdx = 0;
        }
        ts = log->tsData[log->tailIdx];
        start = &log->logData[log->tailIdx * SYSLOG_LINE_MAX];
        strncpy(line, start, SYSLOG_LINE_MAX);
        SYSLOG_CRITICAL_EXIT(log->lock);
        end = line + strlen(line) - 1;
        while (isspace((int)(*end))) {
            *end = 0;
            end--;
        }
#ifdef FREE_RTOS
        uint64_t ts_sec, ts_ms;
        ts_sec = ts / 1000;
        ts_ms = ts - ts_sec * 1000;
        printf("[%6llu.%03llu] ", ts_sec, ts_ms);
#else
        printf("[%9llu] ", ts);
#endif
        puts(line);
        count++;
        SYSLOG_CRITICAL_ENTRY(log->lock);
    }
    SYSLOG_CRITICAL_EXIT(log->lock);
    SYSLOG_FREE(line);
}
