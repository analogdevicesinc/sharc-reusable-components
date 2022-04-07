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

#include <string.h>

#include <sys/platform.h>
#include <cdefSC58x_rom.h>

#include <services/pwr/adi_pwr.h>

#include "flash_map.h"

/*
 * Define this to use a locally configured SPI XIP config for a more
 * optimal quad SPI configuration relative to the boot rom.
 *
 * CAUTION: Does not currently work
 */
#undef CONFIGURE_QUAD_SPI_XIP

/*
 * Define this to have a look at the clock config if desired
 */
#undef QUERY_CLOCKS

/* SPI2 GPIO FER bit positions (one bit per FER entry) */
#define SPI2_CLK_PORTC_FER   (1 << BITP_PORT_DATA_PX1)
#define SPI2_MISO_PORTC_FER  (1 << BITP_PORT_DATA_PX2)
#define SPI2_MOSO_PORTC_FER  (1 << BITP_PORT_DATA_PX3)
#define SPI2_D2_PORTC_FER    (1 << BITP_PORT_DATA_PX4)
#define SPI2_D3_PORTC_FER    (1 << BITP_PORT_DATA_PX5)
#define SPI2_SEL_PORTC_FER   (1 << BITP_PORT_DATA_PX6)

/* SPI2 GPIO MUX bit positions (two bits per MUX entry */
#define SPI2_CLK_PORTC_MUX   (0 << (BITP_PORT_DATA_PX1 << 1))
#define SPI2_MISO_PORTC_MUX  (0 << (BITP_PORT_DATA_PX2 << 1))
#define SPI2_MOSO_PORTC_MUX  (0 << (BITP_PORT_DATA_PX3 << 1))
#define SPI2_D2_PORTC_MUX    (0 << (BITP_PORT_DATA_PX4 << 1))
#define SPI2_D3_PORTC_MUX    (0 << (BITP_PORT_DATA_PX5 << 1))
#define SPI2_SEL_PORTC_MUX   (0 << (BITP_PORT_DATA_PX6 << 1))

/* Pushbutton bit positions */
#define PB1   (BITM_PORT_DATA_PX0)
#define PB2   (BITM_PORT_DATA_PX1)

#define FLASH_XIP_BASE_ADDR   (0x60000000)
#define FLASH_BLOCK_SIZE      (4*1024)
#define FLASH_BLOCK_COUNT     (16384)

#define SPI_BAUD            (3)
#define CPHA                (1)
#define CPOL                (1)
#define QUAD_IO_FAST_READ   (0xEB)

/* Image locations to boot */
#define THIRD_STAGE_LOADER  (BOOT1_OFFSET)
#define MAIN_APPLICATION    (APP_OFFSET)

int adi_mmu_Init()
{
    return(0);
}

void system_clk_init(void)
{
    ADI_PWR_RESULT ePwrResult;

#ifdef QUERY_CLOCKS
    uint32_t cclk,sclk,sclk0,sclk1,dclk,oclk;
#endif

    ePwrResult = adi_pwr_Init(0, 25000000);
    ePwrResult = adi_pwr_SetFreq(0, 450000000, 225000000);

#ifdef QUERY_CLOCKS
    ePwrResult = adi_pwr_GetCoreFreq(0, &cclk);
    ePwrResult = adi_pwr_GetSystemFreq(0, &sclk, &sclk0, &sclk1);
    ePwrResult = adi_pwr_GetDDRClkFreq(0, &dclk);
    ePwrResult = adi_pwr_GetOutClkFreq(0, &oclk);
#endif
}

void initMinimalGPIO(void)
{

#ifdef CONFIGURE_QUAD_SPI_XIP
    /* Configure SPI2 Alternate Function GPIO */
    *pREG_PORTC_FER |= (
        SPI2_CLK_PORTC_FER |
        SPI2_MISO_PORTC_FER |
        SPI2_MOSO_PORTC_FER |
        SPI2_D2_PORTC_FER |
        SPI2_D3_PORTC_FER |
        SPI2_SEL_PORTC_FER
    );
    *pREG_PORTC_MUX |= (
        SPI2_CLK_PORTC_MUX |
        SPI2_MISO_PORTC_MUX |
        SPI2_MOSO_PORTC_MUX |
        SPI2_D2_PORTC_MUX |
        SPI2_D3_PORTC_MUX |
        SPI2_SEL_PORTC_MUX
    );
#endif

    /* Configure LED outputs and turn off */
    *pREG_PORTD_FER_CLR = BITM_PORT_DATA_PX1 | BITM_PORT_DATA_PX2 | BITM_PORT_DATA_PX3;
    *pREG_PORTD_DIR_SET = BITM_PORT_DATA_PX1 | BITM_PORT_DATA_PX2 | BITM_PORT_DATA_PX3;
    *pREG_PORTD_DATA_CLR = BITM_PORT_DATA_PX1 | BITM_PORT_DATA_PX2 | BITM_PORT_DATA_PX3;

    /* Configure pushbutton inputs */
    *pREG_PORTF_FER_CLR = PB1 | PB2;
    *pREG_PORTF_DIR_CLR = PB1 | PB2;
    *pREG_PORTF_INEN_SET = PB1 | PB2;
}

void reset_SPI2(void)
{
    *pREG_SPI2_CTL = 0x00000050;
    *pREG_SPI2_SLVSEL = 0x0000FE00;
    *pREG_SPI2_CLK = 0x00000000;
    *pREG_SPI2_DLY = 0x00000301;
    *pREG_SPI2_TXCTL = 0x00000000;
    *pREG_SPI2_RXCTL = 0x00000000;
    *pREG_SPI2_MMRDH = 0x00000000;
    *pREG_SPI2_MMTOP = 0x00000000;
    *pREG_SPI2_IMSK_CLR = 0xFFFFFFFF;
    *pREG_SPI2_ILAT_CLR = 0xFFFFFFFF;
    *pREG_SPI2_STAT = *pREG_SPI2_STAT;
}

#ifdef CONFIGURE_QUAD_SPI_XIP
void configure_SPI2_Flash_XIP(void)
{
    /*******************************************************************
     * Reset SPI2
     *******************************************************************/
    reset_SPI2();

    /*******************************************************************
     * Configure SPI for quad xfers
     *******************************************************************/
    /* Slave Select Control reg */
    *pREG_SPI2_SLVSEL = ENUM_SPI_SLVSEL_SSEL1_HI | ENUM_SPI_SLVSEL_SSEL1_EN;

    /* SPI clock register: SPI_CLK = SCLK / (BAUD + 1) */
    *pREG_SPI2_CLK = (
        (SPI_BAUD << BITP_SPI_CLK_BAUD) & BITM_SPI_CLK_BAUD
    );

    /* SPI Delay register: STOP = 1; LEAD = 1; LAG = 1 */
    *pREG_SPI2_DLY = (
        ((1 << BITP_SPI_DLY_STOP) & BITM_SPI_DLY_STOP) |
        ((1 << BITP_SPI_DLY_LEADX ) & BITM_SPI_DLY_LEADX) |
        ((1 << BITP_SPI_DLY_LAGX ) & BITM_SPI_DLY_LAGX )
    );

    /*
     * Enable the Memory Mapped mode of SPI with following settings:
     *   Master mode, 32-bit transfer size, HW Slave select, MSB bit first,
     *   Quad Multi-IO mode, FAST enable, mode as defined by CPOL/CPHA.
     */
    *pREG_SPI2_CTL = (
        ENUM_SPI_CTL_MM_EN |
        ENUM_SPI_CTL_MASTER |
        ENUM_SPI_CTL_SIZE32 |
        ENUM_SPI_CTL_HW_SSEL|
        ENUM_SPI_CTL_ASSRT_SSEL |
        ENUM_SPI_CTL_MSB_FIRST |
        ENUM_SPI_CTL_FAST_EN |
        ENUM_SPI_CTL_MIO_QUAD |
        ((CPHA << BITP_SPI_CTL_CPHA) & BITM_SPI_CTL_CPHA) |
        ((CPOL << BITP_SPI_CTL_CPOL) & BITM_SPI_CTL_CPOL)
    );

    /*
     * Enable the Transmit part of SPI with TTI = TEN = 1; other bits should
     * be kept to default values
     */
    *pREG_SPI2_TXCTL = (BITM_SPI_TXCTL_TTI | BITM_SPI_TXCTL_TEN);

    /*
     * Enable the Receive part of SPI with REN = 1; other bits should
     * be kept to default values
     */
    *pREG_SPI2_RXCTL = (BITM_SPI_RXCTL_REN);

    /*
     * Configure the Memory Mapped Read Header for the
     * Fast Read Quad I/O (0xEC) command
     */
    *pREG_SPI2_MMRDH = (
        ((QUAD_IO_FAST_READ << BITP_SPI_MMRDH_OPCODE)& BITM_SPI_MMRDH_OPCODE) |
        ((3 << BITP_SPI_MMRDH_ADRSIZE) & BITM_SPI_MMRDH_ADRSIZE) |
        ((1 << BITP_SPI_MMRDH_ADRPINS) & BITM_SPI_MMRDH_ADRPINS) |
        ((0xFF << BITP_SPI_MMRDH_MODE ) & BITM_SPI_MMRDH_MODE) |
        ((5 << BITP_SPI_MMRDH_DMYSIZE) & BITM_SPI_MMRDH_DMYSIZE) |
        ((2 << BITP_SPI_MMRDH_TRIDMY ) & BITM_SPI_MMRDH_TRIDMY) |
        ((0 << BITP_SPI_MMRDH_CMDSKIP) & BITM_SPI_MMRDH_CMDSKIP) |
        ((0 << BITP_SPI_MMRDH_CMDPINS) & BITM_SPI_MMRDH_CMDPINS)
    );

    /* Top addr of Flash device */
    *pREG_SPI2_MMTOP = FLASH_XIP_BASE_ADDR + (FLASH_BLOCK_SIZE * FLASH_BLOCK_COUNT);

    /* Enable SPI*/
    *pREG_SPI2_CTL |= BITM_SPI_CTL_EN;
}
#endif

void toggle_led(void)
{
    int i;
    *pREG_PORTD_DATA_TGL  = BITM_PORT_DATA_PX1;
    for (i = 0; i < 1000000; i++);
}

int main()
{
    int k;

#ifdef CONFIGURE_QUAD_SPI_XIP
    uint32_t boot_cmd = (
        BITM_ROM_BCMD_NOAUTO |
        BITM_ROM_BCMD_NOCFG |
        ENUM_ROM_BCMD_DEVICE_SPIXIP
    );
#else
    uint32_t boot_cmd = (
        ENUM_ROM_BCMD_SPIM_SSEL1 |
        ENUM_ROM_BCMD_SPIM_DEVENUM_2 |
        ENUM_ROM_BCMD_SPIM_DEVICE_SPIXIP |
        ENUM_ROM_BCMD_SPIM_BCODE_1
    );
#endif

    uint32_t boot_address;
    uint32_t push_buttons;

    /* Need to set system clock for a fast boot */
    system_clk_init();

    /* Initialize a minimum set of GPIO */
    initMinimalGPIO();

    /* Reset SPI2 to POR values */
    reset_SPI2();

#ifdef CONFIGURE_QUAD_SPI_XIP
    /* Configure flash and SPI2 for quad mode 4-byte address XIP */
    configure_SPI2_Flash_XIP();
#endif

    /* Turn on the indicator LED */
    *pREG_PORTD_DATA_SET = BITM_PORT_DATA_PX1;

    /* Flash it a few times */
    for (k = 0; k < 4; k++) {
        toggle_led();
    }

    /* Turn it off */
    *pREG_PORTD_DATA_CLR = BITM_PORT_DATA_PX1;

    /* Read the push buttons */
    push_buttons = *pREG_PORTF_DATA & (PB1 | PB2);

    /* Both buttons pressed mean boot the USB reflash application */
    if (push_buttons == (PB1 | PB2)) {
        boot_address = FLASH_XIP_BASE_ADDR + THIRD_STAGE_LOADER;
    } else if (push_buttons == 0x00000000) {
        boot_address = FLASH_XIP_BASE_ADDR + MAIN_APPLICATION;
    } else {
        while (1) {
            toggle_led();
        }
    }

    /* Boot the requested image and never return */
    adi_rom_Boot((void *)boot_address, 0, 0, 0, boot_cmd);

    /* This never happens */
    return 0;
}



