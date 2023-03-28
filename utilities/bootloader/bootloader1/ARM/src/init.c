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

/* ADI service includes */
#include <services/gpio/adi_gpio.h>
#include <services/int/adi_gic.h>
#include <services/tmr/adi_tmr.h>
#include <services/pwr/adi_pwr.h>

/*
 * The port FER and MUX settings are detailed in:
 *    ADSP-SC582_583_584_587_589_ADSP-21583_584_587.pdf
 *
 */

/* SPI2 GPIO FER bit positions (one bit per FER entry) */
#define SPI2_CLK_PORTC_FER   (1 << BITP_PORT_DATA_PX1)
#define SPI2_MISO_PORTC_FER  (1 << BITP_PORT_DATA_PX2)
#define SPI2_MOSO_PORTC_FER  (1 << BITP_PORT_DATA_PX3)
#define SPI2_D2_PORTC_FER    (1 << BITP_PORT_DATA_PX4)
#define SPI2_D3_PORTC_FER    (1 << BITP_PORT_DATA_PX5)
#define SPI2_SEL_PORTC_FER   (1 << BITP_PORT_DATA_PX6)

/* SPI2 GPIO MUX bit positions (two bits per MUX entry) */
#define SPI2_CLK_PORTC_MUX   (0 << (BITP_PORT_DATA_PX1 << 1))
#define SPI2_MISO_PORTC_MUX  (0 << (BITP_PORT_DATA_PX2 << 1))
#define SPI2_MOSO_PORTC_MUX  (0 << (BITP_PORT_DATA_PX3 << 1))
#define SPI2_D2_PORTC_MUX    (0 << (BITP_PORT_DATA_PX4 << 1))
#define SPI2_D3_PORTC_MUX    (0 << (BITP_PORT_DATA_PX5 << 1))
#define SPI2_SEL_PORTC_MUX   (0 << (BITP_PORT_DATA_PX6 << 1))

/* UART0 GPIO FER bit positions */
#define UART0_TX_PORTC_FER   (1 << BITP_PORT_DATA_PX13)
#define UART0_RX_PORTC_FER   (1 << BITP_PORT_DATA_PX14)
#define UART0_RTS_PORTC_FER  (1 << BITP_PORT_DATA_PX15)
#define UART0_CTS_PORTD_FER  (1 << BITP_PORT_DATA_PX0)

/* UART0 GPIO MUX bit positions (two bits per MUX entry */
#define UART0_TX_PORTC_MUX   (0 << (BITP_PORT_DATA_PX13 << 1))
#define UART0_RX_PORTC_MUX   (0 << (BITP_PORT_DATA_PX14 << 1))
#define UART0_RTS_PORTC_MUX  (0 << (BITP_PORT_DATA_PX15 << 1))
#define UART0_CTS_PORTD_MUX  (0 << (BITP_PORT_DATA_PX0  << 1))

/* Pushbutton bit positions */
#define PB1   (BITM_PORT_DATA_PX0)
#define PB2   (BITM_PORT_DATA_PX1)

void system_clk_init(void)
{
    ADI_PWR_RESULT ePwrResult;
    uint32_t cclk,sclk,sclk0,sclk1,dclk,oclk;

    /* Query primary clocks from CGU0.  Defaults are OK. */
    ePwrResult = adi_pwr_Init (0, 25000000);
    ePwrResult = adi_pwr_GetCoreFreq(0, &cclk);
    ePwrResult = adi_pwr_GetSystemFreq(0, &sclk, &sclk0, &sclk1);
    ePwrResult = adi_pwr_GetDDRClkFreq(0, &dclk);
    ePwrResult = adi_pwr_GetOutClkFreq(0, &oclk);
}

void gpio_init(void)
{
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

    /* Configure UART0 Alternate Function GPIO */
    *pREG_PORTC_FER |= (
        UART0_TX_PORTC_FER |
        UART0_RX_PORTC_FER |
        UART0_RTS_PORTC_FER
    );
    *pREG_PORTC_MUX |= (
        UART0_TX_PORTC_MUX |
        UART0_RX_PORTC_MUX |
        UART0_RTS_PORTC_MUX
    );
    *pREG_PORTD_FER |= (
        UART0_CTS_PORTD_FER
    );
    *pREG_PORTD_MUX |= (
        UART0_CTS_PORTD_MUX
    );

    /* Configure LED outputs and turn off */
    *pREG_PORTD_FER_CLR = BITM_PORT_DATA_PX1 | BITM_PORT_DATA_PX2 | BITM_PORT_DATA_PX3;
    *pREG_PORTD_DIR_SET = BITM_PORT_DATA_PX1 | BITM_PORT_DATA_PX2 | BITM_PORT_DATA_PX3;
    *pREG_PORTD_DATA_CLR = BITM_PORT_DATA_PX1 | BITM_PORT_DATA_PX2 | BITM_PORT_DATA_PX3;

    /* Configure pushbutton inputs */
    *pREG_PORTF_FER_CLR = PB1 | PB2;
    *pREG_PORTF_DIR_CLR = PB1 | PB2;
    *pREG_PORTF_INEN_SET = PB1 | PB2;
}

void gic_init(void)
{
    ADI_GIC_RESULT  result;
    result = adi_gic_Init();
}
