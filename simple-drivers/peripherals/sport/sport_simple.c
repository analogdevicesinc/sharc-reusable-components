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

/*
 * The SPORT Simple driver provides a simple, efficient, RTOS or bare metal
 * ping-pong DMA SPORT driver supporting a wide variety of clocking options.
 *
 * This driver supports:
 *  - FreeRTOS or no RTOS main-loop modes
 *  - Most regularly used audio clocking options
 *  - 16/32 bit data buffers
 *  - Chained DMA transfers
 *  - Graceful start / stop function
 *  - Ping-pong buffer size calculation
 *  - Cache management
 *  - TDM Slot masks
 *
 */
#include <sys/anomaly_macros_rtl.h>
#include <services/int/adi_int.h>
#if defined(__ADSPARM__)
#include <adi/cortex-a5/runtime/cache/adi_cache.h>
#else
#include <sys/cache.h>
#endif
#include <sys/adi_core.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define SPORT_ENTER_CRITICAL()  taskENTER_CRITICAL()
    #define SPORT_EXIT_CRITICAL()   taskEXIT_CRITICAL()
#else
    #define SPORT_ENTER_CRITICAL()
    #define SPORT_EXIT_CRITICAL()
#endif

#include "clocks.h"
#include "sport_simple.h"

/* SHARC L1 Slave 1 port addresses and offsets */
#if !defined(__ADSPARM__)
    #define SHARC_L1_ADDR_START   0x00240000u
    #define SHARC_L1_ADDR_END     0x0039FFFFu
    #if defined(CORE1)
        #define SHARC_L1_ADDR_OFFSET  0x28000000u
    #elif defined(CORE2)
        #define SHARC_L1_ADDR_OFFSET  0x28800000u
    #else
        #error Unsupported SHARC core.  Define "CORE1" for SHARC0 or "CORE2" for SHARC1.
    #endif
#endif

/**************************************************************************
 * SPORT and DMA registers
 **************************************************************************/
volatile uint32_t *SPORT_CTL[SPORT_END] = {
    pREG_SPORT0_CTL_A, pREG_SPORT0_CTL_B,
    pREG_SPORT1_CTL_A, pREG_SPORT1_CTL_B,
    pREG_SPORT2_CTL_A, pREG_SPORT2_CTL_B,
    pREG_SPORT3_CTL_A, pREG_SPORT3_CTL_B,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_SPORT4_CTL_A, pREG_SPORT4_CTL_B,
    pREG_SPORT5_CTL_A, pREG_SPORT5_CTL_B,
    pREG_SPORT6_CTL_A, pREG_SPORT6_CTL_B,
    pREG_SPORT7_CTL_A, pREG_SPORT7_CTL_B
#endif
};

volatile uint32_t *SPORT_MCTL[SPORT_END] = {
    pREG_SPORT0_MCTL_A, pREG_SPORT0_MCTL_B,
    pREG_SPORT1_MCTL_A, pREG_SPORT1_MCTL_B,
    pREG_SPORT2_MCTL_A, pREG_SPORT2_MCTL_B,
    pREG_SPORT3_MCTL_A, pREG_SPORT3_MCTL_B,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_SPORT4_MCTL_A, pREG_SPORT4_MCTL_B,
    pREG_SPORT5_MCTL_A, pREG_SPORT5_MCTL_B,
    pREG_SPORT6_MCTL_A, pREG_SPORT6_MCTL_B,
    pREG_SPORT7_MCTL_A, pREG_SPORT7_MCTL_B
#endif
};

volatile uint32_t *SPORT_CS0[SPORT_END] = {
    pREG_SPORT0_CS0_A, pREG_SPORT0_CS0_B,
    pREG_SPORT1_CS0_A, pREG_SPORT1_CS0_B,
    pREG_SPORT2_CS0_A, pREG_SPORT2_CS0_B,
    pREG_SPORT3_CS0_A, pREG_SPORT3_CS0_B,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_SPORT4_CS0_A, pREG_SPORT4_CS0_B,
    pREG_SPORT5_CS0_A, pREG_SPORT5_CS0_B,
    pREG_SPORT6_CS0_A, pREG_SPORT6_CS0_B,
    pREG_SPORT7_CS0_A, pREG_SPORT7_CS0_B
#endif
};

volatile uint32_t *SPORT_DIV[SPORT_END] = {
    pREG_SPORT0_DIV_A, pREG_SPORT0_DIV_B,
    pREG_SPORT1_DIV_A, pREG_SPORT1_DIV_B,
    pREG_SPORT2_DIV_A, pREG_SPORT2_DIV_B,
    pREG_SPORT3_DIV_A, pREG_SPORT3_DIV_B,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_SPORT4_DIV_A, pREG_SPORT4_DIV_B,
    pREG_SPORT5_DIV_A, pREG_SPORT5_DIV_B,
    pREG_SPORT6_DIV_A, pREG_SPORT6_DIV_B,
    pREG_SPORT7_DIV_A, pREG_SPORT7_DIV_B
#endif
};

void * volatile *DMA_ADDRSTART[SPORT_END] = {
    pREG_DMA0_ADDRSTART, pREG_DMA1_ADDRSTART,
    pREG_DMA2_ADDRSTART, pREG_DMA3_ADDRSTART,
    pREG_DMA4_ADDRSTART, pREG_DMA5_ADDRSTART,
    pREG_DMA6_ADDRSTART, pREG_DMA7_ADDRSTART,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_DMA10_ADDRSTART, pREG_DMA11_ADDRSTART,
    pREG_DMA12_ADDRSTART, pREG_DMA13_ADDRSTART,
    pREG_DMA14_ADDRSTART, pREG_DMA15_ADDRSTART,
    pREG_DMA16_ADDRSTART, pREG_DMA17_ADDRSTART
#endif
};

void * volatile *DMA_DSCPTR_NXT[SPORT_END] = {
    pREG_DMA0_DSCPTR_NXT, pREG_DMA1_DSCPTR_NXT,
    pREG_DMA2_DSCPTR_NXT, pREG_DMA3_DSCPTR_NXT,
    pREG_DMA4_DSCPTR_NXT, pREG_DMA5_DSCPTR_NXT,
    pREG_DMA6_DSCPTR_NXT, pREG_DMA7_DSCPTR_NXT,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_DMA10_DSCPTR_NXT, pREG_DMA11_DSCPTR_NXT,
    pREG_DMA12_DSCPTR_NXT, pREG_DMA13_DSCPTR_NXT,
    pREG_DMA14_DSCPTR_NXT, pREG_DMA15_DSCPTR_NXT,
    pREG_DMA16_DSCPTR_NXT, pREG_DMA17_DSCPTR_NXT
#endif
};

volatile uint32_t *DMA_XCNT[SPORT_END] = {
    pREG_DMA0_XCNT, pREG_DMA1_XCNT,
    pREG_DMA2_XCNT, pREG_DMA3_XCNT,
    pREG_DMA4_XCNT, pREG_DMA5_XCNT,
    pREG_DMA6_XCNT, pREG_DMA7_XCNT,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_DMA10_XCNT, pREG_DMA11_XCNT,
    pREG_DMA12_XCNT, pREG_DMA13_XCNT,
    pREG_DMA14_XCNT, pREG_DMA15_XCNT,
    pREG_DMA16_XCNT, pREG_DMA17_XCNT
#endif
};

volatile int32_t *DMA_XMOD[SPORT_END] = {
    pREG_DMA0_XMOD, pREG_DMA1_XMOD,
    pREG_DMA2_XMOD, pREG_DMA3_XMOD,
    pREG_DMA4_XMOD, pREG_DMA5_XMOD,
    pREG_DMA6_XMOD, pREG_DMA7_XMOD,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_DMA10_XMOD, pREG_DMA11_XMOD,
    pREG_DMA12_XMOD, pREG_DMA13_XMOD,
    pREG_DMA14_XMOD, pREG_DMA15_XMOD,
    pREG_DMA16_XMOD, pREG_DMA17_XMOD
#endif
};

volatile uint32_t *DMA_CFG[SPORT_END] = {
    pREG_DMA0_CFG, pREG_DMA1_CFG,
    pREG_DMA2_CFG, pREG_DMA3_CFG,
    pREG_DMA4_CFG, pREG_DMA5_CFG,
    pREG_DMA6_CFG, pREG_DMA7_CFG,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_DMA10_CFG, pREG_DMA11_CFG,
    pREG_DMA12_CFG, pREG_DMA13_CFG,
    pREG_DMA14_CFG, pREG_DMA15_CFG,
    pREG_DMA16_CFG, pREG_DMA17_CFG
#endif
};

volatile uint32_t *DMA_STAT[SPORT_END] = {
    pREG_DMA0_STAT, pREG_DMA1_STAT,
    pREG_DMA2_STAT, pREG_DMA3_STAT,
    pREG_DMA4_STAT, pREG_DMA5_STAT,
    pREG_DMA6_STAT, pREG_DMA7_STAT,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    pREG_DMA10_STAT, pREG_DMA11_STAT,
    pREG_DMA12_STAT, pREG_DMA13_STAT,
    pREG_DMA14_STAT, pREG_DMA15_STAT,
    pREG_DMA16_STAT, pREG_DMA17_STAT
#endif
};

volatile uint32_t *SPU_SECUREP[SPORT_END] = {
#if defined(__ADSP21569_FAMILY__)
    pREG_SPU0_SECUREP49, pREG_SPU0_SECUREP50,
    pREG_SPU0_SECUREP51, pREG_SPU0_SECUREP52,
    pREG_SPU0_SECUREP53, pREG_SPU0_SECUREP54,
    pREG_SPU0_SECUREP55, pREG_SPU0_SECUREP56,
    pREG_SPU0_SECUREP57, pREG_SPU0_SECUREP58,
    pREG_SPU0_SECUREP59, pREG_SPU0_SECUREP60,
    pREG_SPU0_SECUREP61, pREG_SPU0_SECUREP62,
    pREG_SPU0_SECUREP63, pREG_SPU0_SECUREP64,
#elif defined(__ADSPSC573_FAMILY__)
    pREG_SPU0_SECUREP48, pREG_SPU0_SECUREP49,
    pREG_SPU0_SECUREP50, pREG_SPU0_SECUREP51,
    pREG_SPU0_SECUREP52, pREG_SPU0_SECUREP53,
    pREG_SPU0_SECUREP54, pREG_SPU0_SECUREP55
#elif defined(__ADSPSC589_FAMILY__)
    pREG_SPU0_SECUREP66, pREG_SPU0_SECUREP67,
    pREG_SPU0_SECUREP68, pREG_SPU0_SECUREP69,
    pREG_SPU0_SECUREP70, pREG_SPU0_SECUREP71,
    pREG_SPU0_SECUREP72, pREG_SPU0_SECUREP73,
    pREG_SPU0_SECUREP74, pREG_SPU0_SECUREP75,
    pREG_SPU0_SECUREP76, pREG_SPU0_SECUREP77,
    pREG_SPU0_SECUREP78, pREG_SPU0_SECUREP79,
    pREG_SPU0_SECUREP80, pREG_SPU0_SECUREP81,
#elif defined(__ADSPSC594_FAMILY__)
    pREG_SPU0_SECUREP63, pREG_SPU0_SECUREP64,
    pREG_SPU0_SECUREP65, pREG_SPU0_SECUREP66,
    pREG_SPU0_SECUREP67, pREG_SPU0_SECUREP68,
    pREG_SPU0_SECUREP69, pREG_SPU0_SECUREP70,
    pREG_SPU0_SECUREP71, pREG_SPU0_SECUREP72,
    pREG_SPU0_SECUREP73, pREG_SPU0_SECUREP74,
    pREG_SPU0_SECUREP75, pREG_SPU0_SECUREP76,
    pREG_SPU0_SECUREP77, pREG_SPU0_SECUREP78,
#else
#error Unsupported processor family!
#endif
};

uint16_t SPORT_DMA_IRQ_ID[SPORT_END] = {
    INTR_SPORT0_A_DMA, INTR_SPORT0_B_DMA,
    INTR_SPORT1_A_DMA, INTR_SPORT1_B_DMA,
    INTR_SPORT2_A_DMA, INTR_SPORT2_B_DMA,
    INTR_SPORT3_A_DMA, INTR_SPORT3_B_DMA,
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
    INTR_SPORT4_A_DMA, INTR_SPORT4_B_DMA,
    INTR_SPORT5_A_DMA, INTR_SPORT5_B_DMA,
    INTR_SPORT6_A_DMA, INTR_SPORT6_B_DMA,
    INTR_SPORT7_A_DMA, INTR_SPORT7_B_DMA,
#endif
};

/* Whether or not to force a flush regardless of user setting */
#define SPORT_SIMPLE_NO_FORCE_FLUSH (0)
#define SPORT_SIMPLE_FORCE_FLUSH    (1)

/**************************************************************************
 * SPORT context
 **************************************************************************/
typedef struct {
    void *next;
    void *start;
} SPORT_SIMPLE_DMA_DESC;

struct sSPORT {

    ///< Memory mapped control registers for the SPORT
    volatile uint32_t *pREG_SPORT_CTL;
    volatile uint32_t *pREG_SPORT_MCTL;
    volatile uint32_t *pREG_SPORT_CS0;
    volatile uint32_t *pREG_SPORT_DIV;

    ///< Memory mapped control registers for the SPORT DMA channel
    void * volatile   *pREG_DMA_ADDRSTART;
    void * volatile   *pREG_DMA_DSCPTR_NXT;
    volatile uint32_t *pREG_DMA_XCNT;
    volatile int32_t  *pREG_DMA_XMOD;
    volatile uint32_t *pREG_DMA_CFG;
    volatile uint32_t *pREG_DMA_STAT;

    ///< Memory mapped security registers for the SPORT
    volatile uint32_t *pREG_SPU_SECUREP;

    ///< SPORT IRQ number
    uint16_t SPORT_DMA_IRQ_ID;

    ///< SPORT ping/pong DMA descriptors
    SPORT_SIMPLE_DMA_DESC dmaDescriptors[2];

#ifdef FREE_RTOS
    SemaphoreHandle_t portLock;
#endif

    ///< State tracking variables
    bool open;
    bool running;
    uint32_t enable;

    SPORT_SIMPLE_DATA_DIR dataDir;
    uint32_t dataSizeBytes;
    bool dataBuffersCached;
    SPORT_SIMPLE_AUDIO_CALLBACK callBack;
    void *usrPtr;

    // Ping pong index
    int pp;
};

/* SPORT context containers */
static sSPORT sportContext[SPORT_END];

/**************************************************************************
 * SPORT configuration functions
 **************************************************************************/
static void sport_dma_irq(uint32_t id, void *usrPtr);

static void sport_initRegs(sSPORT *sport)
{
    // Reset SPORT and DMA channels
    *sport->pREG_SPORT_CTL  = 0;
    *sport->pREG_SPORT_MCTL = 0;
    *sport->pREG_SPORT_CS0  = 0;
    *sport->pREG_DMA_CFG = 0;
    *sport->pREG_DMA_STAT = *sport->pREG_DMA_STAT;
    *sport->pREG_DMA_DSCPTR_NXT = 0;
}

static inline void sport_flush_buffer(sSPORT *sport,
    uint8_t *dataAddr, uint32_t dataSizeBytes, int invalidate, bool force)
{
    if (!force && !sport->dataBuffersCached) {
        return;
    }
    flush_data_buffer(dataAddr,
        dataAddr + dataSizeBytes,
        invalidate
    );
}

#if !defined(__ADSPARM__)
static inline void *local_to_system_addr(void *x)
{
    if (((uint32_t)x >= SHARC_L1_ADDR_START) && ((uint32_t)x <= SHARC_L1_ADDR_END)) {
        x = (void *)((uint32_t)x + SHARC_L1_ADDR_OFFSET);
    }
    return(x);
}

static inline void *system_to_local_addr(void *x)
{
    if ( ((uint32_t)x >= (SHARC_L1_ADDR_START + SHARC_L1_ADDR_OFFSET)) &&
         ((uint32_t)x <= (SHARC_L1_ADDR_END + SHARC_L1_ADDR_OFFSET)) ) {
        x = (void *)((uint32_t)x - SHARC_L1_ADDR_OFFSET);
    }
    return(x);
}
#else
static inline void *local_to_system_addr(void *x) { return x; }
static inline void *system_to_local_addr(void *x) { return x; }
#endif

SPORT_SIMPLE_RESULT sport_start(sSPORT *sport, bool flush)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    void *dataAddr;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(sport->portLock, portMAX_DELAY);
#endif

    if (!sport->running) {

        /* Flush both ping pong buffers if transmitting */
        if ((flush) && (sport->dataDir == SPORT_SIMPLE_DATA_DIR_TX)) {
            dataAddr = sport->dmaDescriptors[0].start;
            /* Translate the system address back to the local address */
            dataAddr = system_to_local_addr(dataAddr);
            sport_flush_buffer(
                sport, dataAddr, sport->dataSizeBytes,
                ADI_FLUSH_DATA_NOINV, SPORT_SIMPLE_NO_FORCE_FLUSH
            );
            dataAddr = sport->dmaDescriptors[1].start;
            /* Translate the system address back to the local address */
            dataAddr = system_to_local_addr(dataAddr);
            sport_flush_buffer(
                sport, dataAddr, sport->dataSizeBytes,
                ADI_FLUSH_DATA_NOINV, SPORT_SIMPLE_NO_FORCE_FLUSH
            );
        }

        /* The first quiescent buffer will be the 'ping' buffer */
        sport->pp = 0;

        /* Start the DMA on the 'ping' buffer (pong descriptor next) */
        *sport->pREG_DMA_DSCPTR_NXT = (void *)sport->dmaDescriptors[1].next;

        /* Enable the DMA */
        *sport->pREG_DMA_CFG |= 0x1 << BITP_DMA_CFG_EN;

        /* Enable the SPORT */
        *sport->pREG_SPORT_CTL |= sport->enable;

        sport->running = true;
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(sport->portLock);
#endif

    return(result);
}

static SPORT_SIMPLE_RESULT sport_stop_internal(sSPORT *sport)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;

    if (sport->running) {

        /* Disable the SPORT */
        *sport->pREG_SPORT_CTL &= ~sport->enable;

        /* Disable the DMA */
        *sport->pREG_DMA_CFG &= ~(0x1 << BITP_DMA_CFG_EN);

        sport->running = false;
    }

    return(result);
}

SPORT_SIMPLE_RESULT sport_stop(sSPORT *sport)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(sport->portLock, portMAX_DELAY);
#endif

    result = sport_stop_internal(sport);

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(sport->portLock);
#endif

    return(result);
}

SPORT_SIMPLE_RESULT sport_init(void)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    uint8_t port;
    sSPORT *sport;
    int coreId;

    memset(sportContext, 0, sizeof(sportContext));

    for (port = SPORT0A; port < SPORT_END; port++) {

        sport = &sportContext[port];

        /* Map SPORT, DMA, SPU and IRQ registers */
        sport->pREG_SPU_SECUREP = SPU_SECUREP[port];
        sport->pREG_DMA_DSCPTR_NXT = DMA_DSCPTR_NXT[port];
        sport->pREG_DMA_ADDRSTART = DMA_ADDRSTART[port];
        sport->pREG_DMA_XCNT = DMA_XCNT[port];
        sport->pREG_DMA_XMOD = DMA_XMOD[port];
        sport->pREG_DMA_CFG = DMA_CFG[port];
        sport->pREG_DMA_STAT = DMA_STAT[port];
        sport->pREG_SPORT_CTL = SPORT_CTL[port];
        sport->pREG_SPORT_MCTL = SPORT_MCTL[port];
        sport->pREG_SPORT_CS0 = SPORT_CS0[port];
        sport->pREG_SPORT_DIV = SPORT_DIV[port];
        sport->SPORT_DMA_IRQ_ID = SPORT_DMA_IRQ_ID[port];

        /* If running on a SHARC+ core, enable secure SPORT peripheral DMA
         * to the SHARC internal L1 memory.
         */
        coreId = adi_core_id();
#if defined(ADI_CORE_SHARC1)
        if ( (coreId == ADI_CORE_SHARC0) || (coreId == ADI_CORE_SHARC1) )
#else
        if ( coreId == ADI_CORE_SHARC0 )
#endif
        {
            *sport->pREG_SPU_SECUREP |= BITM_SPU_SECUREP_MSEC;
        }

#ifdef FREE_RTOS
        sport->portLock = xSemaphoreCreateMutex();
        if (sport->portLock == NULL) {
            result = SPORT_SIMPLE_ERROR;
        }
#endif

    }

    return(result);
}

SPORT_SIMPLE_RESULT sport_deinit(void)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    uint8_t port;
    sSPORT *sport;
    int coreId;

    for (port = SPORT0A; port < SPORT_END; port++) {

        sport = &sportContext[port];

        if (sport->pREG_SPU_SECUREP) {
            coreId = adi_core_id();
#if defined(ADI_CORE_SHARC1)
            if ( (coreId == ADI_CORE_SHARC0) || (coreId == ADI_CORE_SHARC1) )
#else
            if ( coreId == ADI_CORE_SHARC0 )
#endif
            {
                *sport->pREG_SPU_SECUREP &= ~BITM_SPU_SECUREP_MSEC;
            }
        }

#ifdef FREE_RTOS
        if (sport->portLock) {
            vSemaphoreDelete(sport->portLock);
            sport->portLock = NULL;
        }
#endif

    }

    return(result);
}

SPORT_SIMPLE_RESULT sport_open(SPORT_SIMPLE_PORT port, sSPORT **sportHandle)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sSPORT *sport;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (port >= SPORT_END) {
        return(SPORT_SIMPLE_INVALID_SPORT);
    }

    if (sportHandle == NULL) {
        return (SPORT_SIMPLE_ERROR);
    }

    sport = &sportContext[port];

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(sport->portLock, portMAX_DELAY);
#endif

    if (sport->open) {
        result = SPORT_SIMPLE_SPORT_BUSY;
    }

    if (result == SPORT_SIMPLE_SUCCESS) {
        sport_initRegs(sport);
        status = adi_int_InstallHandler(sport->SPORT_DMA_IRQ_ID,
            sport_dma_irq, sport, true);
        *sportHandle = sport;
        sport->open = true;
    } else {
        *sportHandle = NULL;
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(sport->portLock);
#endif

    return(result);
}

SPORT_SIMPLE_RESULT sport_close(sSPORT **sportHandle)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sSPORT *sport;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if ((sportHandle == NULL) || (*sportHandle == NULL)) {
        return (SPORT_SIMPLE_ERROR);
    }

    sport = *sportHandle;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(sport->portLock, portMAX_DELAY);
#endif

    sport_stop_internal(sport);
    sport_initRegs(sport);
    status = adi_int_EnableInt(sport->SPORT_DMA_IRQ_ID, false);
    status = adi_int_UninstallHandler(sport->SPORT_DMA_IRQ_ID);
    sport->open = false;
    *sportHandle = NULL;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(sport->portLock);
#endif

    return(result);
}

#if 0
static SPORT_SIMPLE_RESULT sport_config_dai(sSPORT *sport,
    SPORT_SIMPLE_CONFIG *config)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;

    /*
     * If the SPORT is operating as receive master, programs should
     * route SPT0_ACLK_O to SPT0_ACLK_I.
     */

    return(result);
}
#endif

static uint8_t countBits(uint32_t bitMask, uint8_t maxBits)
{
    uint8_t bits = 0;
    while (bitMask && maxBits) {
        if (bitMask & 0x1) {
            bits++;
        }
        bitMask >>= 1;
        maxBits--;
    }
    return(bits);
}

uint32_t sport_buffer_size(SPORT_SIMPLE_CONFIG *config)
{
    uint8_t serializers;
    uint8_t activeSlots;
    uint8_t wordSizeBytes;
    uint8_t tdmSlots;
    uint32_t size;

    serializers = 0;
    serializers += (config->dataEnable & SPORT_SIMPLE_ENABLE_PRIMARY) ? 1 : 0;
    serializers += (config->dataEnable & SPORT_SIMPLE_ENABLE_SECONDARY) ? 1 : 0;

    wordSizeBytes = 0;
    if (config->wordSize == SPORT_SIMPLE_WORD_SIZE_16BIT) {
        wordSizeBytes = sizeof(uint16_t);
    } else if (config->wordSize == SPORT_SIMPLE_WORD_SIZE_32BIT) {
        wordSizeBytes = sizeof(uint32_t);
    }

    if (config->slotMask) {
        tdmSlots = config->tdmSlots;
        /* The SPORT applies the slot mask to each half of the L/R clock
         * in packed I2S mode so only check the bottom half of the mask bits.
         */
        if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_50) {
            tdmSlots /= 2;
        }
        activeSlots = countBits(config->slotMask, tdmSlots);
        /* The SPORT applies the slot mask to each half of the L/R clock
         * in packed I2S mode essentially doubling the number of active
         * slots.
         */
        if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_50) {
            activeSlots *= 2;
        }
        if (activeSlots > config->tdmSlots) {
            activeSlots = config->tdmSlots;
        }
    } else {
        activeSlots = config->tdmSlots;
    }

    size = activeSlots * config->frames * serializers * wordSizeBytes;

    return(size);
}

static SPORT_SIMPLE_RESULT sport_config_dma(sSPORT *sport,
    SPORT_SIMPLE_CONFIG *config)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    uint32_t stride;
    uint32_t dmaCfg;

    /* Set the DMA word size */
    switch (config->wordSize) {
        case SPORT_SIMPLE_WORD_SIZE_16BIT:
            dmaCfg = ENUM_DMA_CFG_MSIZE02 | ENUM_DMA_CFG_PSIZE02;
            stride = sizeof(uint16_t);
            break;
        default:
            dmaCfg = ENUM_DMA_CFG_MSIZE04 | ENUM_DMA_CFG_PSIZE04;
            stride = sizeof(uint32_t);
            break;
    }

    /* Set the DMA memory direction */
    if (config->dataDir == SPORT_SIMPLE_DATA_DIR_RX) {
        dmaCfg |= ENUM_DMA_CFG_WRITE;
    } else if (config->dataDir == SPORT_SIMPLE_DATA_DIR_TX) {
        dmaCfg |= ENUM_DMA_CFG_READ;
    } else {
        result = SPORT_SIMPLE_ERROR;
    }

    /* Enable DMA complete trigger */
    dmaCfg |= ENUM_DMA_CFG_XCNT_TRIG;

    /* Enable DMA complete interrupt if a callback was supplied */
    if (config->callBack) {
        dmaCfg |= ENUM_DMA_CFG_XCNT_INT;
    }

    /* Set the sync bit if required */
    if (config->syncDMA) {
        dmaCfg |= ENUM_DMA_CFG_SYNC;
    }

    /* Set remaining DMA config parameters */
    dmaCfg |= ENUM_DMA_CFG_FETCH02 | ENUM_DMA_CFG_DSCLIST;

    /* Configure the DMA descriptors */
    sport->dmaDescriptors[0].next =
        local_to_system_addr(&sport->dmaDescriptors[1]);
    sport->dmaDescriptors[0].start =
        (int32_t *)local_to_system_addr(config->dataBuffers[0]);
    sport->dmaDescriptors[1].next =
        local_to_system_addr(&sport->dmaDescriptors[0]);
    sport->dmaDescriptors[1].start =
        (int32_t *)local_to_system_addr(config->dataBuffers[1]);

    /* Flush the DMA descriptors to memory */
    sport_flush_buffer(
        sport,
        (uint8_t *)sport->dmaDescriptors, sizeof(sport->dmaDescriptors),
        ADI_FLUSH_DATA_NOINV, SPORT_SIMPLE_FORCE_FLUSH
    );

    /* Configure DMA channel, but do not enable it yet */
    *sport->pREG_DMA_XCNT = sport->dataSizeBytes / stride;
    *sport->pREG_DMA_XMOD = stride;
    *sport->pREG_DMA_CFG = dmaCfg;

    return(result);
}

static SPORT_SIMPLE_RESULT sport_config_sport(sSPORT *sport,
    SPORT_SIMPLE_CONFIG *config)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
    uint32_t ctl;
    uint32_t mctl;
    uint32_t cs0;
    uint32_t div;
    uint32_t fsDiv;
    uint32_t clkDiv;
    uint32_t bitClk;
    int i;

    /* Configure constant SPORT_CTL parameters */
    ctl = (1 << BITP_SPORT_CTL_A_DTYPE) |    // Right justify, sign extend
          (1 << BITP_SPORT_CTL_A_FSED) |     // Frame sync edge detect
          (1 << BITP_SPORT_CTL_A_FSR) |      // Frame sync required
          (1 << BITP_SPORT_CTL_A_DIFS);      // Data independent frame sync

    /* Set data direction */
    if (config->dataDir == SPORT_SIMPLE_DATA_DIR_TX) {
        ctl |= 1 << BITP_SPORT_CTL_A_SPTRAN;
    }

    /* Set frame sync options */
    if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_INV) {
        ctl |= 1 << BITP_SPORT_CTL_A_LFS;
    }
    if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_50) {
        ctl |= 1 << BITP_SPORT_CTL_A_OPMODE;
    }

    /* Set bit clock options */
    if (config->bitClkOptions & SPORT_SIMPLE_CLK_FALLING) {
        ctl |= 1 << BITP_SPORT_CTL_A_CKRE;
    }

    /* Set transfer size */
    ctl |= (config->wordSize - 1) << BITP_SPORT_CTL_A_SLEN;

    /* Save enable settings */
    sport->enable = 0x00000000;
    if (config->dataEnable & SPORT_SIMPLE_ENABLE_PRIMARY) {
        sport->enable |= 1 << BITP_SPORT_CTL_SPENPRI;
    }
    if (config->dataEnable & SPORT_SIMPLE_ENABLE_SECONDARY) {
        sport->enable |= 1 << BITP_SPORT_CTL_SPENSEC;
    }

    /* Set constant multi-channel parameters */
    mctl = (1 << BITP_SPORT_MCTL_A_MCE) |
           (1<< BITP_SPORT_MCTL_A_MCPDE);

    /* Set frame sync delay */
    if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_EARLY) {
        mctl |= (0x1 << BITP_SPORT_MCTL_A_MFD);
    }

    /* Reset the Fs/CLK divisor */
    div = 0x00000000;

    /* Set the bit clock direction and speed */
    if (config->clkDir == SPORT_SIMPLE_CLK_DIR_MASTER) {
        bitClk = config->wordSize * config->tdmSlots * config->fs;
        clkDiv = (SCLK0 / bitClk) - 1;
        div |= clkDiv;
        ctl |= 1 << BITP_SPORT_CTL_A_ICLK;
    }

    /* When the SPORT_SIMPLE_FS_OPTION_50 option is selected (i.e.
     * ctl.opmode=1), the SPORT applies the slot configuration to each half
     * of the L/R clock cycle instead of the entire frame so temporarily
     * cut the number of slots in half while computing active slots
     * to compensate.
     */
    if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_50) {
        config->tdmSlots /= 2;
    }

    /* Set the slot window size */
    mctl |= (config->tdmSlots - 1) << BITP_SPORT_MCTL_A_WSIZE;

    /* Set the slot mask */
    cs0 = 0x00000000;
    if (config->slotMask) {
        cs0 = config->slotMask;
    } else {
        for (i = 0; i < config->tdmSlots; i++) {
            cs0 = (cs0 << 1) | 0x1;
       }
    }

    /* Set the fs clock direction and speed */
    if (config->fsDir == SPORT_SIMPLE_FS_DIR_MASTER) {
        fsDiv = (config->wordSize * config->tdmSlots) - 1;
        div |= (fsDiv << BITP_SPORT_DIV_A_FSDIV);
        ctl |= 1 << BITP_SPORT_CTL_A_IFS;
    }

    /* Set the TDM slots back if necessary */
    if (config->fsOptions & SPORT_SIMPLE_FS_OPTION_50) {
        config->tdmSlots *= 2;
    }

    /* Configure, but do not enable, the SPORT */
    *sport->pREG_SPORT_DIV = div;
    *sport->pREG_SPORT_MCTL = mctl;
    *sport->pREG_SPORT_CS0 = cs0;
    *sport->pREG_SPORT_CTL = ctl;

    return(result);
}

SPORT_SIMPLE_RESULT sport_configure(sSPORT *sport,
    SPORT_SIMPLE_CONFIG *config)
{
    SPORT_SIMPLE_RESULT result = SPORT_SIMPLE_SUCCESS;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    /* Sanity check handle and config */
    if ((sport == NULL) || (config == NULL)) {
        return (SPORT_SIMPLE_ERROR);
    }

    /* Make sure critical settings are configured */
    if ( (config->clkDir == SPORT_SIMPLE_CLK_DIR_UNKNOWN) ||
         (config->fsDir == SPORT_SIMPLE_FS_DIR_UNKNOWN) ||
         (config->dataDir == SPORT_SIMPLE_DATA_DIR_UNKNOWN) ||
         (config->dataEnable == SPORT_SIMPLE_ENABLE_NONE) ||
         (config->tdmSlots == SPORT_SIMPLE_TDM_UNKNOWN) ||
         (config->wordSize == SPORT_SIMPLE_WORD_SIZE_UNKNOWN) ) {
        return(SPORT_SIMPLE_CFG_ERROR);
    }
    if (config->clkDir == SPORT_SIMPLE_CLK_DIR_MASTER) {
        if (config->fs == 0) {
            return(SPORT_SIMPLE_CFG_ERROR);
        }
    };
    if ( (config->dataBuffers[0] == NULL) ||
         (config->dataBuffers[1] == NULL) ||
         (config->frames == 0) ) {
            return(SPORT_SIMPLE_CFG_ERROR);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(sport->portLock, portMAX_DELAY);
#endif

    /* Stop and re-initialize the SPORT if necessary */
    sport_stop_internal(sport);
    sport_initRegs(sport);

    /* Save / calculate misc config items */
    sport->dataDir = config->dataDir;
    sport->callBack = config->callBack;
    sport->usrPtr = config->usrPtr;
    sport->dataSizeBytes = sport_buffer_size(config);
    sport->dataBuffersCached = config->dataBuffersCached;

    /* Reconfigure the SPORT and DMA */
    result = sport_config_dma(sport, config);
    if (result == SPORT_SIMPLE_SUCCESS) {
        result = sport_config_sport(sport, config);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(sport->portLock);
#endif

    return(result);
}

static void sport_dma_irq(uint32_t id, void *usrPtr)
{
    sSPORT *sport = (sSPORT *)usrPtr;
    uint8_t *dataAddr;

    /* Clear the interrupt */
    *sport->pREG_DMA_STAT |= ENUM_DMA_STAT_IRQDONE;

    /* Call the callback with buffer and cache management */
    if (sport->callBack) {

        /* Get the quiescent data buffer address */
        dataAddr = sport->dmaDescriptors[sport->pp].start;

        /* Translate the system address back to the local address */
        dataAddr = system_to_local_addr(dataAddr);

        /* If receiving from cached memory, invalidate the data buffer */
        if (sport->dataDir == SPORT_SIMPLE_DATA_DIR_RX) {
            sport_flush_buffer(
                sport, dataAddr, sport->dataSizeBytes,
                ADI_FLUSH_DATA_INV, SPORT_SIMPLE_NO_FORCE_FLUSH
            );
        }

        /* Make application audio callback */
        sport->callBack(dataAddr, sport->dataSizeBytes, sport->usrPtr);

        /* If transmitting from cached memory, flush the data buffer */
        if (sport->dataDir == SPORT_SIMPLE_DATA_DIR_TX) {
            sport_flush_buffer(
                sport, dataAddr, sport->dataSizeBytes,
                ADI_FLUSH_DATA_NOINV, SPORT_SIMPLE_NO_FORCE_FLUSH
            );
        }

        /*
         * Synchronize with the DMA.  Emulator breakpoints can cause
         * the DMA to lose sync with the application.
         *
         * Set the ping pong index to the currently active buffer which
         * will become the quiescent buffer on the next interrupt.
         */
        sport->pp = (*sport->pREG_DMA_ADDRSTART == sport->dmaDescriptors[0].start) ?
            0 : 1;
    }
}
