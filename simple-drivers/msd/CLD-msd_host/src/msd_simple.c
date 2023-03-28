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

#include <string.h>

#if defined(__ADSPARM__)
#include <runtime/cache/adi_cache.h>
#else
#include <sys/cache.h>
#endif

#include "msd_host.h"
#include "msd_simple.h"

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define MSD_ENTER_CRITICAL()  taskENTER_CRITICAL()
    #define MSD_EXIT_CRITICAL()   taskEXIT_CRITICAL()
    #define MSD_LOCK()            xSemaphoreTake(msd->portLock, portMAX_DELAY);
    #define MSD_UNLOCK()          xSemaphoreGive(msd->portLock);
#else
    #define MSD_ENTER_CRITICAL()
    #define MSD_EXIT_CRITICAL()
    #define MSD_LOCK()
    #define MSD_UNLOCK()
#endif

struct sMSD {

    bool open;

#ifdef FREE_RTOS
    SemaphoreHandle_t portLock;
    SemaphoreHandle_t portBlock;
#else
    volatile bool msdDone;
#endif

    volatile bool rwOK;

    uint64_t capacity;
};


/* MSD port context containers */
static sMSD msdContext[MSD_END];

static void
msd_flushDataCache(uint8_t *pBuffer, uint32_t bufferSize, int invalidate)
{
    flush_data_buffer(pBuffer, pBuffer + bufferSize - 1, invalidate);
}


MSD_SIMPLE_RESULT msd_init(void)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    uint8_t port;
    sMSD *msd;

    memset(msdContext, 0, sizeof(msdContext));

    for (port = MSD0; port < MSD_END; port++) {

        msd = &msdContext[port];

#ifdef FREE_RTOS
        msd->portLock = xSemaphoreCreateMutex();
        if (msd->portLock == NULL) {
            result = MSD_SIMPLE_ERROR;
        }
        msd->portBlock = xSemaphoreCreateCounting(1, 0);
        if (msd->portBlock == NULL) {
            result = MSD_SIMPLE_ERROR;
        }
#endif

        msd->open = false;

    }

    return(result);
}

MSD_SIMPLE_RESULT msd_deinit(void)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    uint8_t port;
    sMSD *msd;

    for (port = MSD0; port < MSD_END; port++) {

        msd = &msdContext[port];

#ifdef FREE_RTOS
        if (msd->portBlock) {
            vSemaphoreDelete(msd->portBlock);
            msd->portBlock = NULL;
        }
        if (msd->portLock) {
            vSemaphoreDelete(msd->portLock);
            msd->portLock = NULL;
        }
#endif

    }

    return(result);
}

MSD_SIMPLE_RESULT msd_open(MSD_SIMPLE_PORT port, sMSD **msdHandle)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    sMSD *msd;

    if (port >= MSD_END) {
        return(MSD_SIMPLE_INVALID_PORT);
    }

    msd = &msdContext[port];

    MSD_LOCK();

    if (msd->open == true) {
        result = MSD_SIMPLE_PORT_BUSY;
    }

    if (result == MSD_SIMPLE_SUCCESS) {
        *msdHandle = msd;
        msd->open = true;
    } else {
        *msdHandle = NULL;
    }

    MSD_UNLOCK();

    return(result);
}

MSD_SIMPLE_RESULT msd_close(sMSD **msdHandle)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    sMSD *msd = *msdHandle;

    if (*msdHandle == NULL) {
        return (MSD_SIMPLE_ERROR);
    }

    MSD_LOCK();

    msd->open = false;
    *msdHandle = NULL;

    MSD_UNLOCK();

    return(result);
}

MSD_SIMPLE_RESULT msd_present(sMSD *msd)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    CLD_Boolean ready;

    MSD_LOCK();

    ready = msd_host_ready();

    MSD_UNLOCK();

    result = (ready == CLD_TRUE) ? MSD_SIMPLE_SUCCESS : MSD_SIMPLE_ERROR;

    return(result);
}

/***********************************************************************
 * Read/Write Callback
 ***********************************************************************/
static void msdCB(bool success, void *usr)
{
    sMSD *msd = (sMSD *)usr;
    msd->rwOK = success;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
    BaseType_t contextSwitch = pdFALSE;
    rtosResult = xSemaphoreGiveFromISR(msd->portBlock, &contextSwitch);
    portYIELD_FROM_ISR(contextSwitch);
#else
    msd->msdDone = true;
#endif
}

/***********************************************************************
 * Write
 ***********************************************************************/
MSD_SIMPLE_RESULT msd_write(sMSD *msd, void *data, uint64_t sector, uint32_t count)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    CLD_RV cldResult;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    MSD_LOCK();
    if (count > 1) {
        asm("nop;");
    }
    msd_flushDataCache(data, 512u * count, false);
#ifndef FREE_RTOS
    msd->msdDone = false;
#endif
    cldResult = msd_host_write(data, sector, count, msdCB, msd);
#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(msd->portBlock, portMAX_DELAY);
#else
    while (msd->msdDone == false);
#endif
    result = msd->rwOK ? MSD_SIMPLE_SUCCESS : MSD_SIMPLE_ERROR;
    MSD_UNLOCK();

    return(result);
}

/***********************************************************************
 * Read
 ***********************************************************************/
MSD_SIMPLE_RESULT msd_read(sMSD *msd, void *data, uint64_t sector, uint32_t count)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    CLD_RV cldResult;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    MSD_LOCK();
    if (count > 1) {
        asm("nop;");
    }
    msd_flushDataCache(data, 512u * count, false);
#ifndef FREE_RTOS
    msd->msdDone = false;
#endif
    cldResult = msd_host_read(data, sector, count, msdCB, msd);
#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(msd->portBlock, portMAX_DELAY);
#else
    while (msd->msdDone == false);
#endif
    msd_flushDataCache(data, 512u * count, true);
    result = msd->rwOK ? MSD_SIMPLE_SUCCESS : MSD_SIMPLE_ERROR;
    MSD_UNLOCK();

    return(result);
}

/***********************************************************************
 * Info Function
 ***********************************************************************/
MSD_SIMPLE_RESULT msd_info(sMSD *msd, MSD_SIMPLE_INFO *info)
{
    MSD_SIMPLE_RESULT result = MSD_SIMPLE_SUCCESS;
    MSD_HOST_INFO hostInfo;
    CLD_Boolean ok;

    if (msd == NULL) {
        return(MSD_SIMPLE_ERROR);
    }

    MSD_LOCK();
    ok = msd_host_info(&hostInfo);
    if (info) {
        if (ok) {
            info->capacity = hostInfo.capacity;
        } else {
            info->capacity = 0;
        }
    }
    MSD_UNLOCK();

    result = (ok == CLD_TRUE) ? MSD_SIMPLE_SUCCESS : MSD_SIMPLE_ERROR;

    return(result);
}
