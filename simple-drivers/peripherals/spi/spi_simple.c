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
 * SPI Simple provides a simple, efficient, RTOS or bare metal SPI driver.
 *
 * This driver supports:
 *  - FreeRTOS or no RTOS main-loop modes
 *  - Multiple devices, with independent configurations, on a SPI port
 *  - Fully protected multi-threaded device transfers
 *  - Rx/Tx, Rx only, or Tx only DMA transfers
 *  - 8/16/32 bit data device transfers
 *  - Fully managed slave select or application call-back
 *  - Standard, Dual, or Quad I/O device transfers
 *  - Multiple transfers within a single atomic slave select
 *  - Blocking transfers
 *
 */

#include <string.h>

#include <sys/anomaly_macros_rtl.h>
#include <services/int/adi_int.h>
#if defined(__ADSPARM__)
#include <adi/cortex-a5/runtime/cache/adi_cache.h>
#else
#include <sys/cache.h>
#endif
#include <sys/adi_core.h>

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define SPI_ENTER_CRITICAL()  taskENTER_CRITICAL()
    #define SPI_EXIT_CRITICAL()   taskEXIT_CRITICAL()
#else
    #define SPI_ENTER_CRITICAL()
    #define SPI_EXIT_CRITICAL()
#endif

#include "spi_simple.h"

#define SPI_SLVSEL_DEFAULT (0x0000FE00u)

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

struct sSPI {

    ///< Memory mapped control registers for the SPI port
    volatile uint32_t * pREG_SPI_CTL;
    volatile uint32_t * pREG_SPI_CLK;
    volatile uint32_t * pREG_SPI_STAT;
    volatile uint32_t * pREG_SPI_SLVSEL;
    volatile uint32_t * pREG_SPI_DLY;
    volatile const uint32_t * pREG_SPI_IMSK;
    volatile uint32_t * pREG_SPI_IMSK_CLR;
    volatile uint32_t * pREG_SPI_IMSK_SET;
    volatile uint32_t * pREG_SPI_ILAT_CLR;


    volatile const uint32_t * pREG_SPI_RFIFO;
    volatile uint32_t * pREG_SPI_RXCTL;
    volatile uint32_t * pREG_SPI_RWC;
    volatile uint32_t * pREG_SPI_RWCR;

    volatile uint32_t * pREG_SPI_TFIFO;
    volatile uint32_t * pREG_SPI_TXCTL;
    volatile uint32_t * pREG_SPI_TWC;
    volatile uint32_t * pREG_SPI_TWCR;

    ///< Memory mapped control registers for the SPI DMA channel
    volatile uint32_t * pREG_RX_DMA_CFG;
    volatile uint32_t * pREG_RX_DMA_ADDRSTART;
    volatile uint32_t * pREG_RX_DMA_XCNT;
    volatile int32_t  * pREG_RX_DMA_XMOD;
    volatile uint32_t * pREG_RX_DMA_STAT;
    volatile uint32_t * pREG_RX_DMA_SPU_SECURE;

    volatile uint32_t * pREG_TX_DMA_CFG;
    volatile uint32_t * pREG_TX_DMA_ADDRSTART;
    volatile uint32_t * pREG_TX_DMA_XCNT;
    volatile int32_t  * pREG_TX_DMA_XMOD;
    volatile uint32_t * pREG_TX_DMA_STAT;
    volatile uint32_t * pREG_TX_DMA_SPU_SECURE;

    ///< SPI port IRQ number
    uint16_t SPI_STAT_IRQ_ID;

    ///< State tracking variables
    bool open;
    sSPIXfer *xfers;
    uint8_t xferIndex;
    uint8_t xferCount;

    ///< Reference to the active device
    sSPIPeriph *device;

#ifdef FREE_RTOS
    SemaphoreHandle_t portLock;
    SemaphoreHandle_t portBlock;
#else
    volatile bool spiDone;
#endif

};

/* SPI Peripheral Device Handle Structure */
struct sSPIPeriph {
    bool open;
    sSPI *spiHandle;
    uint32_t SPI_CTL_MASK;
    uint32_t SPI_CTL;
    uint32_t SPI_CLK;
    uint32_t SPI_SLVSEL_ASSERT;
    uint32_t SPI_SLVSEL_DEASSERT;
    SPI_SIMPLE_WORDSIZE wordSize;
    SPI_SIMPLE_SLAVE_SELECT_CALLBACK ssCallBack;
    void *ssCallBackUsrPtr;
};

/* SPI port context containers */
static sSPI spiContext[SPI_END];

/* SPI peripheral device context containers */
static sSPIPeriph spiDeviceContext[SPI_SIMPLE_MAX_DEVICES];

/* Slave select assert table */
static const uint32_t SSEL_ASSERT_TABLE[8] = {
    SPI_SLVSEL_DEFAULT,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL1_HI) | ENUM_SPI_SLVSEL_SSEL1_EN,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL2_HI) | ENUM_SPI_SLVSEL_SSEL2_EN,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL3_HI) | ENUM_SPI_SLVSEL_SSEL3_EN,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL4_HI) | ENUM_SPI_SLVSEL_SSEL4_EN,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL5_HI) | ENUM_SPI_SLVSEL_SSEL5_EN,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL6_HI) | ENUM_SPI_SLVSEL_SSEL6_EN,
    (SPI_SLVSEL_DEFAULT & ~ENUM_SPI_SLVSEL_SSEL7_HI) | ENUM_SPI_SLVSEL_SSEL7_EN,
};

/* Slave select deassert table */
static const uint32_t SSEL_DEASSERT_TABLE[8] = {
    SPI_SLVSEL_DEFAULT,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL1_HI) | ENUM_SPI_SLVSEL_SSEL1_EN,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL2_HI) | ENUM_SPI_SLVSEL_SSEL2_EN,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL3_HI) | ENUM_SPI_SLVSEL_SSEL3_EN,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL4_HI) | ENUM_SPI_SLVSEL_SSEL4_EN,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL5_HI) | ENUM_SPI_SLVSEL_SSEL5_EN,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL6_HI) | ENUM_SPI_SLVSEL_SSEL6_EN,
    (SPI_SLVSEL_DEFAULT | ENUM_SPI_SLVSEL_SSEL7_HI) | ENUM_SPI_SLVSEL_SSEL7_EN,
};

/**************************************************************************
 * SPI device interface functions
 **************************************************************************/
SPI_SIMPLE_RESULT spi_openDevice(sSPI *spiHandle, sSPIPeriph **deviceHandle)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    sSPIPeriph *device = NULL;
    uint8_t idx;

    /* Find an available device */
    SPI_ENTER_CRITICAL();
    for (idx = 0; idx < SPI_SIMPLE_MAX_DEVICES; idx++) {
        if (spiDeviceContext[idx].open == false) {
            device = &spiDeviceContext[idx];
            device->open = true;
            break;
        }
    }

    /* Initialize the device */
    if (device != NULL) {
        device->spiHandle           = spiHandle;
        device->SPI_CTL_MASK        = 0x00000000u;
        device->SPI_CTL             = ENUM_SPI_CTL_MASTER;
        device->SPI_CLK             = 0x00000000u;
        device->SPI_SLVSEL_ASSERT   = SPI_SLVSEL_DEFAULT;
        device->SPI_SLVSEL_DEASSERT = SPI_SLVSEL_DEFAULT;
        device->ssCallBack          = NULL;
        device->ssCallBackUsrPtr    = NULL;
        device->wordSize            = SPI_WORDSIZE_8BIT;
    } else {
        result = SPI_SIMPLE_NO_MORE_DEVICES;
    }

    /* Return a reference */
    *deviceHandle = device;

    SPI_EXIT_CRITICAL();

    return(result);
}

SPI_SIMPLE_RESULT spi_closeDevice(sSPIPeriph **device)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    if (*device == NULL) {
        return (SPI_SIMPLE_ERROR);
    }

    /* Close the device */
    SPI_ENTER_CRITICAL();
    (*device)->open = false;
    *device = NULL;
    SPI_EXIT_CRITICAL();

    return(result);
}

SPI_SIMPLE_RESULT spi_setFastMode(sSPIPeriph *device, bool fastMode)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    SPI_ENTER_CRITICAL();

    device->SPI_CTL_MASK |= BITM_SPI_CTL_FMODE;

    if (fastMode == true) {
        device->SPI_CTL |= ENUM_SPI_CTL_FAST_EN;
    } else {
        device->SPI_CTL &= ~ENUM_SPI_CTL_FAST_EN;
    }

    SPI_EXIT_CRITICAL();

    return result;
}

SPI_SIMPLE_RESULT spi_setLsbFirst(sSPIPeriph *device, bool lsbFirst)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    SPI_ENTER_CRITICAL();

    device->SPI_CTL_MASK |= BITM_SPI_CTL_LSBF;

    if (lsbFirst) {
        device->SPI_CTL |= ENUM_SPI_CTL_LSB_FIRST;
    } else {
        device->SPI_CTL &= ~ENUM_SPI_CTL_LSB_FIRST;
    }

    SPI_EXIT_CRITICAL();

    return result;
}

SPI_SIMPLE_RESULT spi_setClock(sSPIPeriph *device, uint16_t clock)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    SPI_ENTER_CRITICAL();

    device->SPI_CLK = clock;

    SPI_EXIT_CRITICAL();

    return result;
}

SPI_SIMPLE_RESULT spi_setMode(sSPIPeriph *device, SPI_SIMPLE_MODE mode)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    uint32_t modeMask;

    modeMask = (BITM_SPI_CTL_CPHA | BITM_SPI_CTL_CPOL);

    SPI_ENTER_CRITICAL();

    device->SPI_CTL_MASK |= modeMask;

    /* Clear the mode */
    device->SPI_CTL &= ~modeMask;

    /* Set clock phase (using the BITM define is more clear) */
    if ((mode == SPI_MODE_1) || (mode == SPI_MODE_3)) {
        device->SPI_CTL |= BITM_SPI_CTL_CPHA;
    }

    /* Set clock polarity (using the BITM define is more clear) */
    if ((mode == SPI_MODE_2) || (mode == SPI_MODE_3)) {
        device->SPI_CTL |= BITM_SPI_CTL_CPOL;
    }

    SPI_EXIT_CRITICAL();

    return result;
}

SPI_SIMPLE_RESULT spi_setSlaveSelect(sSPIPeriph *device, SPI_SIMPLE_SLAVE_SEL slaveSelect)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    SPI_ENTER_CRITICAL();

    device->SPI_SLVSEL_ASSERT = SSEL_ASSERT_TABLE[slaveSelect];
    device->SPI_SLVSEL_DEASSERT = SSEL_DEASSERT_TABLE[slaveSelect];

    SPI_EXIT_CRITICAL();

    return result;
}

SPI_SIMPLE_RESULT spi_setSlaveSelectCallback(sSPIPeriph *device,
    SPI_SIMPLE_SLAVE_SELECT_CALLBACK cb, void *usrPtr)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    SPI_ENTER_CRITICAL();

    device->ssCallBack = cb;
    device->ssCallBackUsrPtr = usrPtr;

    SPI_EXIT_CRITICAL();

    return result;
}

SPI_SIMPLE_RESULT spi_setWordSize(sSPIPeriph *device, SPI_SIMPLE_WORDSIZE wordSize)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    uint32_t wordSizeMask;

    wordSizeMask = BITM_SPI_CTL_SIZE;

    SPI_ENTER_CRITICAL();

    /* Add the transfer word size to the device bit mask */
    device->SPI_CTL_MASK |= wordSizeMask;

    /* Clear the transfer word size */
    device->SPI_CTL &= ~wordSizeMask;

    /* Set the transfer word size.  Default to 8-bit */
    switch (wordSize) {
        case SPI_WORDSIZE_32BIT:
            device->SPI_CTL |= ENUM_SPI_CTL_SIZE32;
            break;
        case SPI_WORDSIZE_16BIT:
            device->SPI_CTL |= ENUM_SPI_CTL_SIZE16;
            break;
        default:
            device->SPI_CTL |= ENUM_SPI_CTL_SIZE08;
            break;
    }

    /* Save the word size for the DMA config */
    device->wordSize = wordSize;

    SPI_EXIT_CRITICAL();

    return result;
}

/**************************************************************************
 * Quad/Dual SPI device functions
 **************************************************************************/
SPI_SIMPLE_RESULT spi_startOnMosi(sSPIPeriph *device, bool mosi)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;

    SPI_ENTER_CRITICAL();

    device->SPI_CTL_MASK |= BITM_SPI_CTL_SOSI;

    if (mosi == true) {
        device->SPI_CTL |= ENUM_SPI_CTL_STMOSI;
    } else {
        device->SPI_CTL &= ~ENUM_SPI_CTL_STMOSI;
    }

    SPI_EXIT_CRITICAL();

    return result;
}

/**************************************************************************
 * SPI port configuration functions
 **************************************************************************/
static void spi_stat_irq(uint32_t id, void *usrPtr);

static void spi_initRegs(sSPI *spi)
{
    int coreId;

    *spi->pREG_SPI_CTL      = 0x00000000u;
    *spi->pREG_SPI_RXCTL    = 0x00000000u;
    *spi->pREG_SPI_TXCTL    = 0x00000000u;
    *spi->pREG_SPI_CLK      = 0x00000000u;
    *spi->pREG_SPI_DLY      = 0x00000300u;
    *spi->pREG_SPI_SLVSEL   = SPI_SLVSEL_DEFAULT;
    *spi->pREG_SPI_RWC      = 0x00000000u;
    *spi->pREG_SPI_RWCR     = 0x00000000u;
    *spi->pREG_SPI_TWC      = 0x00000000u;
    *spi->pREG_SPI_TWCR     = 0x00000000u;
    *spi->pREG_SPI_IMSK_CLR = 0xFFFFFFFFu;
    *spi->pREG_SPI_ILAT_CLR = 0xFFFFFFFFu;
    *spi->pREG_SPI_STAT     = *spi->pREG_SPI_STAT;

    /* If running on a SHARC+ core, enable secure SPI peripheral DMA
     * to gain access to SHARC internal L1 memory space.
     */
    coreId = adi_core_id();
#if defined(ADI_CORE_SHARC1)
    if ( (coreId == ADI_CORE_SHARC0) || (coreId == ADI_CORE_SHARC1) )
#else
    if ( coreId == ADI_CORE_SHARC0 )
#endif
    {
        *spi->pREG_TX_DMA_SPU_SECURE |= BITM_SPU_SECUREP_MSEC;
        *spi->pREG_RX_DMA_SPU_SECURE |= BITM_SPU_SECUREP_MSEC;
    }
}

SPI_SIMPLE_RESULT spi_init(void)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    uint8_t port;
    sSPI *spi;

    memset(spiContext, 0, sizeof(spiContext));
    memset(spiDeviceContext, 0, sizeof(spiDeviceContext));

    for (port = SPI0; port < SPI_END; port++) {

        spi = &spiContext[port];

        if (port == SPI0) {
            spi->pREG_SPI_SLVSEL   = pREG_SPI0_SLVSEL;
            spi->pREG_SPI_CTL      = pREG_SPI0_CTL;
            spi->pREG_SPI_CLK      = pREG_SPI0_CLK;
            spi->pREG_SPI_TFIFO    = pREG_SPI0_TFIFO;
            spi->pREG_SPI_RFIFO    = pREG_SPI0_RFIFO;
            spi->pREG_SPI_TXCTL    = pREG_SPI0_TXCTL;
            spi->pREG_SPI_RXCTL    = pREG_SPI0_RXCTL;
            spi->pREG_SPI_STAT     = pREG_SPI0_STAT;
            spi->pREG_SPI_DLY      = pREG_SPI0_DLY;
            spi->pREG_SPI_RWC      = pREG_SPI0_RWC;
            spi->pREG_SPI_RWCR     = pREG_SPI0_RWCR;
            spi->pREG_SPI_TWC      = pREG_SPI0_TWC;
            spi->pREG_SPI_TWCR     = pREG_SPI0_TWCR;
            spi->pREG_SPI_IMSK     = pREG_SPI0_IMSK;
            spi->pREG_SPI_IMSK_CLR = pREG_SPI0_IMSK_CLR;
            spi->pREG_SPI_IMSK_SET = pREG_SPI0_IMSK_SET;
            spi->pREG_SPI_ILAT_CLR = pREG_SPI0_ILAT_CLR;

            spi->SPI_STAT_IRQ_ID   = INTR_SPI0_STAT;

            spi->pREG_TX_DMA_CFG        = pREG_DMA22_CFG;
            spi->pREG_TX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA22_ADDRSTART;
            spi->pREG_TX_DMA_XCNT       = pREG_DMA22_XCNT;
            spi->pREG_TX_DMA_XMOD       = pREG_DMA22_XMOD;
            spi->pREG_TX_DMA_STAT       = pREG_DMA22_STAT;
#if defined(__ADSP21569_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP73;
#elif defined(__ADSPSC573_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP62;
#elif defined(__ADSPSC589_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP101;
#elif defined(__ADSPSC594_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP93;
#else
#error Unsupported processor!
#endif
            spi->pREG_RX_DMA_CFG        = pREG_DMA23_CFG;
            spi->pREG_RX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA23_ADDRSTART;
            spi->pREG_RX_DMA_XCNT       = pREG_DMA23_XCNT;
            spi->pREG_RX_DMA_XMOD       = pREG_DMA23_XMOD;
            spi->pREG_RX_DMA_STAT       = pREG_DMA23_STAT;
#if defined(__ADSP21569_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP74;
#elif defined(__ADSPSC573_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP63;
#elif defined(__ADSPSC589_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP102;
#elif defined(__ADSPSC594_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP94;
#else
#error Unsupported processor!
#endif
        }
        else if (port == SPI1) {
            spi->pREG_SPI_SLVSEL   = pREG_SPI1_SLVSEL;
            spi->pREG_SPI_CTL      = pREG_SPI1_CTL;
            spi->pREG_SPI_CLK      = pREG_SPI1_CLK;
            spi->pREG_SPI_TFIFO    = pREG_SPI1_TFIFO;
            spi->pREG_SPI_RFIFO    = pREG_SPI1_RFIFO;
            spi->pREG_SPI_TXCTL    = pREG_SPI1_TXCTL;
            spi->pREG_SPI_RXCTL    = pREG_SPI1_RXCTL;
            spi->pREG_SPI_STAT     = pREG_SPI1_STAT;
            spi->pREG_SPI_DLY      = pREG_SPI1_DLY;
            spi->pREG_SPI_RWC      = pREG_SPI1_RWC;
            spi->pREG_SPI_RWCR     = pREG_SPI1_RWCR;
            spi->pREG_SPI_TWC      = pREG_SPI1_TWC;
            spi->pREG_SPI_TWCR     = pREG_SPI1_TWCR;
            spi->pREG_SPI_IMSK     = pREG_SPI1_IMSK;
            spi->pREG_SPI_IMSK_CLR = pREG_SPI1_IMSK_CLR;
            spi->pREG_SPI_IMSK_SET = pREG_SPI1_IMSK_SET;
            spi->pREG_SPI_ILAT_CLR = pREG_SPI1_ILAT_CLR;

            spi->SPI_STAT_IRQ_ID   = INTR_SPI1_STAT;

            spi->pREG_TX_DMA_CFG        = pREG_DMA24_CFG;
            spi->pREG_TX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA24_ADDRSTART;
            spi->pREG_TX_DMA_XCNT       = pREG_DMA24_XCNT;
            spi->pREG_TX_DMA_XMOD       = pREG_DMA24_XMOD;
            spi->pREG_TX_DMA_STAT       = pREG_DMA24_STAT;
#if defined(__ADSP21569_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP75;
#elif defined(__ADSPSC573_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP64;
#elif defined(__ADSPSC589_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP103;
#elif defined(__ADSPSC594_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP95;
#else
#error Unsupported processor!
#endif

            spi->pREG_RX_DMA_CFG        = pREG_DMA25_CFG;
            spi->pREG_RX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA25_ADDRSTART;
            spi->pREG_RX_DMA_XCNT       = pREG_DMA25_XCNT;
            spi->pREG_RX_DMA_XMOD       = pREG_DMA25_XMOD;
            spi->pREG_RX_DMA_STAT       = pREG_DMA25_STAT;
#if defined(__ADSP21569_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP76;
#elif defined(__ADSPSC573_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP65;
#elif defined(__ADSPSC589_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP104;
#elif defined(__ADSPSC594_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP96;
#else
#error Unsupported processor!
#endif
        }
        else if (port == SPI2) {
            spi->pREG_SPI_SLVSEL   = pREG_SPI2_SLVSEL;
            spi->pREG_SPI_CTL      = pREG_SPI2_CTL;
            spi->pREG_SPI_CLK      = pREG_SPI2_CLK;
            spi->pREG_SPI_TFIFO    = pREG_SPI2_TFIFO;
            spi->pREG_SPI_RFIFO    = pREG_SPI2_RFIFO;
            spi->pREG_SPI_TXCTL    = pREG_SPI2_TXCTL;
            spi->pREG_SPI_RXCTL    = pREG_SPI2_RXCTL;
            spi->pREG_SPI_STAT     = pREG_SPI2_STAT;
            spi->pREG_SPI_DLY      = pREG_SPI2_DLY;
            spi->pREG_SPI_RWC      = pREG_SPI2_RWC;
            spi->pREG_SPI_RWCR     = pREG_SPI2_RWCR;
            spi->pREG_SPI_TWC      = pREG_SPI2_TWC;
            spi->pREG_SPI_TWCR     = pREG_SPI2_TWCR;
            spi->pREG_SPI_IMSK     = pREG_SPI2_IMSK;
            spi->pREG_SPI_IMSK_CLR = pREG_SPI2_IMSK_CLR;
            spi->pREG_SPI_IMSK_SET = pREG_SPI2_IMSK_SET;
            spi->pREG_SPI_ILAT_CLR = pREG_SPI2_ILAT_CLR;

            spi->SPI_STAT_IRQ_ID   = INTR_SPI2_STAT;

            spi->pREG_TX_DMA_CFG        = pREG_DMA26_CFG;
            spi->pREG_TX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA26_ADDRSTART;
            spi->pREG_TX_DMA_XCNT       = pREG_DMA26_XCNT;
            spi->pREG_TX_DMA_XMOD       = pREG_DMA26_XMOD;
            spi->pREG_TX_DMA_STAT       = pREG_DMA26_STAT;
#if defined(__ADSP21569_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP77;
#elif defined(__ADSPSC573_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP73;
#elif defined(__ADSPSC589_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP105;
#elif defined(__ADSPSC594_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP97;
#else
#error Unsupported processor!
#endif

            spi->pREG_RX_DMA_CFG        = pREG_DMA27_CFG;
            spi->pREG_RX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA27_ADDRSTART;
            spi->pREG_RX_DMA_XCNT       = pREG_DMA27_XCNT;
            spi->pREG_RX_DMA_XMOD       = pREG_DMA27_XMOD;
            spi->pREG_RX_DMA_STAT       = pREG_DMA27_STAT;
#if defined(__ADSP21569_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP78;
#elif defined(__ADSPSC573_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP74;
#elif defined(__ADSPSC589_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP106;
#elif defined(__ADSPSC594_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP98;
#else
#error Unsupported processor!
#endif
#if defined(__ADSPSC594_FAMILY__)
        }
        else if (port == SPI3) {
            spi->pREG_SPI_SLVSEL   = pREG_SPI3_SLVSEL;
            spi->pREG_SPI_CTL      = pREG_SPI3_CTL;
            spi->pREG_SPI_CLK      = pREG_SPI3_CLK;
            spi->pREG_SPI_TFIFO    = pREG_SPI3_TFIFO;
            spi->pREG_SPI_RFIFO    = pREG_SPI3_RFIFO;
            spi->pREG_SPI_TXCTL    = pREG_SPI3_TXCTL;
            spi->pREG_SPI_RXCTL    = pREG_SPI3_RXCTL;
            spi->pREG_SPI_STAT     = pREG_SPI3_STAT;
            spi->pREG_SPI_DLY      = pREG_SPI3_DLY;
            spi->pREG_SPI_RWC      = pREG_SPI3_RWC;
            spi->pREG_SPI_RWCR     = pREG_SPI3_RWCR;
            spi->pREG_SPI_TWC      = pREG_SPI3_TWC;
            spi->pREG_SPI_TWCR     = pREG_SPI3_TWCR;
            spi->pREG_SPI_IMSK     = pREG_SPI3_IMSK;
            spi->pREG_SPI_IMSK_CLR = pREG_SPI3_IMSK_CLR;
            spi->pREG_SPI_IMSK_SET = pREG_SPI3_IMSK_SET;
            spi->pREG_SPI_ILAT_CLR = pREG_SPI3_ILAT_CLR;

            spi->SPI_STAT_IRQ_ID   = INTR_SPI3_STAT;

            spi->pREG_TX_DMA_CFG        = pREG_DMA55_CFG;
            spi->pREG_TX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA55_ADDRSTART;
            spi->pREG_TX_DMA_XCNT       = pREG_DMA55_XCNT;
            spi->pREG_TX_DMA_XMOD       = pREG_DMA55_XMOD;
            spi->pREG_TX_DMA_STAT       = pREG_DMA55_STAT;
#if defined(__ADSPSC594_FAMILY__)
            spi->pREG_TX_DMA_SPU_SECURE = pREG_SPU0_SECUREP99;
#else
#error Unsupported processor!
#endif
            spi->pREG_RX_DMA_CFG        = pREG_DMA56_CFG;
            spi->pREG_RX_DMA_ADDRSTART  = (volatile uint32_t *)pREG_DMA56_ADDRSTART;
            spi->pREG_RX_DMA_XCNT       = pREG_DMA56_XCNT;
            spi->pREG_RX_DMA_XMOD       = pREG_DMA56_XMOD;
            spi->pREG_RX_DMA_STAT       = pREG_DMA56_STAT;
#if defined(__ADSPSC594_FAMILY__)
            spi->pREG_RX_DMA_SPU_SECURE = pREG_SPU0_SECUREP100;
#else
#error Unsupported processor!
#endif
#endif
        } else {
            result = SPI_SIMPLE_ERROR;
            break;
        }

#ifdef FREE_RTOS
        spi->portLock = xSemaphoreCreateMutex();
        if (spi->portLock == NULL) {
            result = SPI_SIMPLE_ERROR;
        }

        spi->portBlock = xSemaphoreCreateCounting(1, 0);
        if (spi->portBlock == NULL) {
            result = SPI_SIMPLE_ERROR;
        }
#endif

        spi->open = false;

    }

    return(result);
}

SPI_SIMPLE_RESULT spi_deinit(void)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    uint8_t port;
    sSPI *spi;

    for (port = SPI0; port < SPI_END; port++) {

        spi = &spiContext[port];

#ifdef FREE_RTOS
        if (spi->portBlock) {
            vSemaphoreDelete(spi->portBlock);
            spi->portBlock = NULL;
        }

        if (spi->portLock) {
            vSemaphoreDelete(spi->portLock);
            spi->portLock = NULL;
        }
#endif

    }

    return(result);
}

SPI_SIMPLE_RESULT spi_open(SPI_SIMPLE_PORT port, sSPI **spiHandle)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sSPI *spi;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (port >= SPI_END) {
        return(SPI_SIMPLE_INVALID_PORT);
    }

    spi = &spiContext[port];

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(spi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = SPI_SIMPLE_ERROR;
    }
#endif

    if (spi->open == true) {
        result = SPI_SIMPLE_PORT_BUSY;
    }

    if (result == SPI_SIMPLE_SUCCESS) {
        spi_initRegs(spi);
        status = adi_int_InstallHandler(spi->SPI_STAT_IRQ_ID,
            spi_stat_irq, spi, true);
        if (status != ADI_INT_SUCCESS) {
            result = SPI_SIMPLE_ERROR;
        }
    }

    if (result == SPI_SIMPLE_SUCCESS) {
        *spiHandle = spi;
        spi->open = true;
    } else {
        status = adi_int_UninstallHandler(spi->SPI_STAT_IRQ_ID);
        *spiHandle = NULL;
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(spi->portLock);
    if (rtosResult != pdTRUE) {
        result = SPI_SIMPLE_ERROR;
    }
#endif

    return(result);
}

SPI_SIMPLE_RESULT spi_close(sSPI **spiHandle)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    ADI_INT_STATUS status;
    sSPI *spi = *spiHandle;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif

    if (*spiHandle == NULL) {
        return (SPI_SIMPLE_ERROR);
    }

#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(spi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = SPI_SIMPLE_ERROR;
    }
#endif

    spi->open = false;
    spi_initRegs(spi);

    status = adi_int_EnableInt(spi->SPI_STAT_IRQ_ID, false);
    if (status != ADI_INT_SUCCESS) {
        result = SPI_SIMPLE_ERROR;
    }

    status = adi_int_UninstallHandler(spi->SPI_STAT_IRQ_ID);
    if (status != ADI_INT_SUCCESS) {
        result = SPI_SIMPLE_ERROR;
    }

    *spiHandle = NULL;

#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(spi->portLock);
    if (rtosResult != pdTRUE) {
        result = SPI_SIMPLE_ERROR;
    }
#endif

    return(result);
}

#if !defined(__ADSPARM__)
static inline void *local_to_system_addr(void *x)
{
    if (((uint32_t)x >= SHARC_L1_ADDR_START) && ((uint32_t)x <= SHARC_L1_ADDR_END)) {
        x = (void *)((uint32_t)x + SHARC_L1_ADDR_OFFSET);
    }
    return(x);
}
#else
static inline void *local_to_system_addr(void *x) { return x; }
#endif

static void spi_setup_dma(sSPI *spi, uint16_t len, void *rx, void *tx)
{
    sSPIPeriph *device = spi->device;
    uint8_t *flushStart, *flushEnd;
    uint32_t stride;
    uint32_t cfg;

    /* Set the word size.  Default to 8-bit */
    switch (device->wordSize) {
        case SPI_WORDSIZE_32BIT:
            cfg = ENUM_DMA_CFG_MSIZE04 | ENUM_DMA_CFG_PSIZE04;
            stride = sizeof(uint32_t);
            break;
        case SPI_WORDSIZE_16BIT:
            cfg = ENUM_DMA_CFG_MSIZE02 | ENUM_DMA_CFG_PSIZE02;
            stride = sizeof(uint16_t);
            break;
        default:
            cfg = ENUM_DMA_CFG_MSIZE01 | ENUM_DMA_CFG_PSIZE01;
            stride = sizeof(uint8_t);
            break;
    }
    cfg |= ENUM_DMA_CFG_SYNC | ENUM_DMA_CFG_STOP | ENUM_DMA_CFG_EN;

#if defined(__ADSPARM__)
    /* Invalidate rx buffer cache */
    if (rx) {
        flushStart = (uint8_t *)rx;  flushEnd = flushStart + (len * stride);
        flush_data_buffer(flushStart, flushEnd, ADI_FLUSH_DATA_INV);
    }
    /* Flush tx buffer to memory */
    if (tx) {
        flushStart = (uint8_t *)tx;  flushEnd = flushStart + (len * stride);
        flush_data_buffer(flushStart, flushEnd, ADI_FLUSH_DATA_NOINV);
    }
#else
    /*
     * flush_data_buffers() causes invalid instruction crashes on the SHARC+
     * core so use dcache_invalidate_both() to flush and invalidate
     * both caches instead even though it's much less efficient.
     *
     * https://ez.analog.com/dsp/sharc-processors/adsp-sc5xxadsp-215xx/f/q-a/63896/sc584-rev-1-0-flush_data_buffer-does-not-work-as-expected
     */
    dcache_invalidate_both(ADI_DCACHE_INV_WB);
#endif

    *spi->pREG_RX_DMA_CFG = 0x00000000u;
    if (rx != NULL) {
        rx = local_to_system_addr(rx);
        *spi->pREG_RX_DMA_STAT = ENUM_DMA_STAT_PIRQ | ENUM_DMA_STAT_IRQERR | ENUM_DMA_STAT_IRQDONE;
        *spi->pREG_RX_DMA_ADDRSTART = (uint32_t)rx;
        *spi->pREG_RX_DMA_XCNT = len;
        *spi->pREG_RX_DMA_XMOD = stride;
        *spi->pREG_RX_DMA_CFG = cfg | ENUM_DMA_CFG_WRITE;
    }

    *spi->pREG_TX_DMA_CFG = 0x00000000u;
    if (tx != NULL) {
        tx = local_to_system_addr(tx);
        *spi->pREG_TX_DMA_STAT = ENUM_DMA_STAT_PIRQ | ENUM_DMA_STAT_IRQERR | ENUM_DMA_STAT_IRQDONE;
        *spi->pREG_TX_DMA_ADDRSTART = (uint32_t)tx;
        *spi->pREG_TX_DMA_XCNT = len;
        *spi->pREG_TX_DMA_XMOD = stride;
        *spi->pREG_TX_DMA_CFG = cfg | ENUM_DMA_CFG_READ;
    }
}

static void spi_apply_flags(uint32_t *reg, uint32_t flags)
{
    /* Set the SPI IO mode (normal, dual, quad) */
    *reg &= ~BITM_SPI_CTL_MIOM;
    if (flags & SPI_SIMPLE_XFER_DUAL_IO) {
        *reg |= ENUM_SPI_CTL_MIO_DUAL;
    } else if (flags & SPI_SIMPLE_XFER_QUAD_IO) {
        *reg |= ENUM_SPI_CTL_MIO_QUAD;
    }
}

static void spi_setup_port_rxtx(sSPI *spi, uint16_t len, void *rx, void *tx)
{
    uint32_t reg;

    /* Reset Rx and Tx ctl for the next transfer */
    *spi->pREG_SPI_RXCTL = 0x00000000u;
    *spi->pREG_SPI_TXCTL = 0x00000000u;

    /* Enable only one Rx or Tx finish IRQ */
    *spi->pREG_SPI_IMSK_CLR = (ENUM_SPI_IMSK_RF_HI | ENUM_SPI_IMSK_TF_HI);
    if (rx != NULL) {
        *spi->pREG_SPI_IMSK_SET =  ENUM_SPI_IMSK_RF_HI;
    } else {
        *spi->pREG_SPI_IMSK_SET =  ENUM_SPI_IMSK_TF_HI;
    }

    /*
     * Setup Rx/Tx count and enable.  The transmitter is configured last because
     * it is the preferred initiator.  If no Tx is configured, then the Rx
     * is the transfer initiator.
     */
    if (rx != NULL) {
        *spi->pREG_SPI_RWC = len;
        reg = ENUM_SPI_RXCTL_RDR_NE | ENUM_SPI_RXCTL_RWC_EN | ENUM_SPI_RXCTL_RX_EN;
        if (tx == NULL) {
            reg |= ENUM_SPI_RXCTL_RTI_EN;
        }
        *spi->pREG_SPI_RXCTL = reg;
    }

    if (tx != NULL) {
        *spi->pREG_SPI_TWC = len;
        *spi->pREG_SPI_TXCTL = ENUM_SPI_TXCTL_TDR_NF| ENUM_SPI_TXCTL_TWC_EN |
            ENUM_SPI_TXCTL_TX_EN | ENUM_SPI_TXCTL_TTI_EN;
    }
}

static void spi_setup_port_device(sSPIPeriph *deviceHandle, uint16_t len, void *rx, void *tx, uint32_t flags)
{
    sSPIPeriph *device = deviceHandle;
    sSPI *spi = device->spiHandle;
    uint32_t reg;

    /* Configure device SPI_CLK */
    *spi->pREG_SPI_CLK  = device->SPI_CLK;

    /* Configure device SPI_CTL */
    reg = *spi->pREG_SPI_CTL & ~(device->SPI_CTL_MASK);
    reg |= device->SPI_CTL;

    /* Set the SPI IO mode (normal, dual, quad) */
    spi_apply_flags(&reg, flags);

    *spi->pREG_SPI_CTL = reg | ENUM_SPI_CTL_EN;
}

static void spi_stat_irq(uint32_t id, void *usrPtr)
{
    sSPI *spi = (sSPI *)usrPtr;
    void *rx;
    void *tx;
    uint16_t len;
    uint32_t flags;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
    BaseType_t contextSwitch = pdFALSE;
#endif
    uint8_t *flushStart, *flushEnd;

    *spi->pREG_SPI_STAT = *spi->pREG_SPI_STAT;

    /* Invalidate Rx buffers */
    rx = spi->xfers[spi->xferIndex].rx;
    if (rx) {
        len = spi->xfers[spi->xferIndex].len;
        flushStart = (uint8_t *)rx;  flushEnd = flushStart + len;
        flush_data_buffer(flushStart, flushEnd, ADI_FLUSH_DATA_INV);
    }

    if (spi->xferCount == 0) {
#ifdef FREE_RTOS
        rtosResult = xSemaphoreGiveFromISR(spi->portBlock, &contextSwitch);
        portYIELD_FROM_ISR(contextSwitch);
#else
        spi->spiDone = true;
#endif
    } else {
        /* Modify the xfer counts */
        spi->xferIndex++;
        spi->xferCount--;

        /* Set some convenience variables */
        rx = spi->xfers[spi->xferIndex].rx;
        tx = spi->xfers[spi->xferIndex].tx;
        len = spi->xfers[spi->xferIndex].len;
        flags = spi->xfers[spi->xferIndex].flags;

        /* Configure the DMA */
        spi_setup_dma(spi, len, rx, tx);

        /* Set the SPI IO mode (normal, dual, quad) */
        spi_apply_flags((uint32_t *)spi->pREG_SPI_CTL, flags);

        /* Initiate the next transfer */
        spi_setup_port_rxtx(spi, len, rx, tx);
    }
}

SPI_SIMPLE_RESULT spi_batch_xfer(sSPIPeriph *deviceHandle, uint16_t numXfers, sSPIXfer *xfers)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    sSPIPeriph *device = deviceHandle;
    sSPI *spi = device->spiHandle;
#ifdef FREE_RTOS
    BaseType_t rtosResult;
#endif
    void *rx;
    void *tx;
    uint16_t len;

    if (numXfers == 0) {
        return(SPI_SIMPLE_ERROR);
    }

    /* Lock the SPI port */
#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(spi->portLock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        return(SPI_SIMPLE_ERROR);
    }
#endif

    /* Store a reference to the current active device */
    spi->device = deviceHandle;

    /* Indicate to the ISR now many xfers remain */
    spi->xfers = xfers;
    spi->xferIndex = 0;
    spi->xferCount = numXfers - 1;

    /* Set some convenience variables */
    len = xfers[spi->xferIndex].len;
    rx = xfers[spi->xferIndex].rx;
    tx = xfers[spi->xferIndex].tx;

    /* Setup the device specific SPI settings, IO mode, and enable to assert outputs */
    spi_setup_port_device(device, len, rx, tx, xfers[spi->xferIndex].flags);

    /* Configure the DMA */
    spi_setup_dma(spi, len, rx, tx);

    /* Assert the device slave select */
    if (device->ssCallBack != NULL) {
        device->ssCallBack(true, device->ssCallBackUsrPtr);
    } else {
        *spi->pREG_SPI_SLVSEL = device->SPI_SLVSEL_ASSERT;
#if defined WA_20000062 && WA_20000062
        /* Must write twice for some devices */
        *spi->pREG_SPI_SLVSEL = device->SPI_SLVSEL_ASSERT;
#endif
    }

#ifndef FREE_RTOS
    spi->spiDone = false;
#endif

    /* Configure the Rx/Tx ctl and count registers to initiate transfer */
    spi_setup_port_rxtx(spi, len, rx, tx);

    /* Block until complete */
#ifdef FREE_RTOS
    rtosResult = xSemaphoreTake(spi->portBlock, portMAX_DELAY);
    if (rtosResult != pdTRUE) {
        result = SPI_SIMPLE_ERROR;
    }
#else
    while (spi->spiDone == false);
#endif

    /* Deassert chip select (must write twice for some devices) */
    if (device->ssCallBack != NULL) {
        device->ssCallBack(false, device->ssCallBackUsrPtr);
    } else {
        *spi->pREG_SPI_SLVSEL = device->SPI_SLVSEL_DEASSERT;
#if defined WA_20000062 && WA_20000062
        /* Must write twice for some devices */
        *spi->pREG_SPI_SLVSEL = device->SPI_SLVSEL_DEASSERT;
#endif
    }

    /* Disable the SPI port */
    *spi->pREG_SPI_CTL &= ~ENUM_SPI_CTL_EN;

    /* Unlock the SPI port */
#ifdef FREE_RTOS
    rtosResult = xSemaphoreGive(spi->portLock);
    if (rtosResult != pdTRUE) {
        result = SPI_SIMPLE_ERROR;
    }
#endif

    return(result);
}

SPI_SIMPLE_RESULT spi_xfer(sSPIPeriph *deviceHandle, uint16_t len, void *rx, void *tx)
{
    SPI_SIMPLE_RESULT result = SPI_SIMPLE_SUCCESS;
    sSPIXfer xfer;

    if (((rx == NULL) && (tx == NULL)) || (len == 0)) {
        return(SPI_SIMPLE_ERROR);
    }

    /* Copy the transfer parameters */
    xfer.len = len;
    xfer.rx = rx;
    xfer.tx = tx;
    xfer.flags = SPI_SIMPLE_XFER_NORMAL_IO;

    result = spi_batch_xfer(deviceHandle, 1, &xfer);

    return(result);
}

