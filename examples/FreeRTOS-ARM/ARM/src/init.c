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

#ifdef FREE_RTOS
#include "FreeRTOS.h"
#endif

/* Application includes */
#include "clocks.h"
#include "init.h"

/***********************************************************************
 * SC573 EZ-Kit GPIO definitions and initialization
 **********************************************************************/
#ifdef __EZKIT_573__

/* SPI2 GPIO FER bit positions (one bit per FER entry) */
#define SPI2_CLK_PORTB_FER   (1 << BITP_PORT_DATA_PX14)
#define SPI2_MISO_PORTB_FER  (1 << BITP_PORT_DATA_PX10)
#define SPI2_MOSO_PORTB_FER  (1 << BITP_PORT_DATA_PX11)
#define SPI2_D2_PORTB_FER    (1 << BITP_PORT_DATA_PX12)
#define SPI2_D3_PORTB_FER    (1 << BITP_PORT_DATA_PX13)
#define SPI2_SEL_PORTB_FER   (1 << BITP_PORT_DATA_PX15)

/* SPI2 GPIO MUX bit positions (two bits per MUX entry) */
#define SPI2_CLK_PORTB_MUX   (0 << (BITP_PORT_DATA_PX14 << 1))
#define SPI2_MISO_PORTB_MUX  (0 << (BITP_PORT_DATA_PX10 << 1))
#define SPI2_MOSO_PORTB_MUX  (0 << (BITP_PORT_DATA_PX11 << 1))
#define SPI2_D2_PORTB_MUX    (0 << (BITP_PORT_DATA_PX12 << 1))
#define SPI2_D3_PORTB_MUX    (0 << (BITP_PORT_DATA_PX13 << 1))
#define SPI2_SEL_PORTB_MUX   (0 << (BITP_PORT_DATA_PX15 << 1))
/* ezkit enables the quad mode via soft switches */

/* UART0 GPIO FER bit positions */
#define UART0_TX_PORTF_FER   (1 << BITP_PORT_DATA_PX8)
#define UART0_RX_PORTF_FER   (1 << BITP_PORT_DATA_PX9)
#define UART0_RTS_PORTD_FER  (1 << BITP_PORT_DATA_PX5)
#define UART0_CTS_PORTD_FER  (1 << BITP_PORT_DATA_PX6)

/* UART0 GPIO MUX bit positions (two bits per MUX entry */
#define UART0_TX_PORTF_MUX   (0 << (BITP_PORT_DATA_PX8 << 1))
#define UART0_RX_PORTF_MUX   (0 << (BITP_PORT_DATA_PX9 << 1))
#define UART0_RTS_PORTD_MUX  (0 << (BITP_PORT_DATA_PX5 << 1))
#define UART0_CTS_PORTD_MUX  (0 << (BITP_PORT_DATA_PX6  << 1))

void gpio_init(void)
{
    static uint8_t gpioMemory[ADI_GPIO_CALLBACK_MEM_SIZE];
    uint32_t numCallbacks;

    ADI_GPIO_RESULT  result;

    /* Configure SPI2 Alternate Function GPIO */
    *pREG_PORTB_FER |= (
        SPI2_CLK_PORTB_FER |
        SPI2_MISO_PORTB_FER |
        SPI2_MOSO_PORTB_FER |
        SPI2_D2_PORTB_FER |
        SPI2_D3_PORTB_FER |
        SPI2_SEL_PORTB_FER
    );
    *pREG_PORTB_MUX |= (
        SPI2_CLK_PORTB_MUX |
        SPI2_MISO_PORTB_MUX |
        SPI2_MOSO_PORTB_MUX |
        SPI2_D2_PORTB_MUX |
        SPI2_D3_PORTB_MUX |
        SPI2_SEL_PORTB_MUX
    );

    /* Configure UART0 Alternate Function GPIO */
    *pREG_PORTF_FER |= (
        UART0_TX_PORTF_FER |
        UART0_RX_PORTF_FER
    );
    *pREG_PORTF_MUX |= (
        UART0_TX_PORTF_MUX |
        UART0_RX_PORTF_MUX
    );

    *pREG_PORTD_FER |= (
        UART0_RTS_PORTD_FER |
        UART0_CTS_PORTD_FER
    );
    *pREG_PORTD_MUX |= (
        UART0_RTS_PORTD_MUX |
        UART0_CTS_PORTD_MUX
    );

    result = adi_gpio_Init(gpioMemory, sizeof(gpioMemory), &numCallbacks);

    result = adi_gpio_SetDirection(
        ADI_GPIO_PORT_A,
        ADI_GPIO_PIN_9,
        ADI_GPIO_DIRECTION_OUTPUT
    );

    result = adi_gpio_SetDirection(
        ADI_GPIO_PORT_C,
        ADI_GPIO_PIN_4 | ADI_GPIO_PIN_14,
        ADI_GPIO_DIRECTION_OUTPUT
    );

    result = adi_gpio_SetDirection(
        ADI_GPIO_PORT_E,
        ADI_GPIO_PIN_1 | ADI_GPIO_PIN_13 | ADI_GPIO_PIN_15,
        ADI_GPIO_DIRECTION_OUTPUT
    );

    result = adi_gpio_SetDirection(
        ADI_GPIO_PORT_F,
        ADI_GPIO_PIN_11,
        ADI_GPIO_DIRECTION_OUTPUT
    );

    result = adi_gpio_Clear (
        ADI_GPIO_PORT_A,
        ADI_GPIO_PIN_9
    );

    result = adi_gpio_Clear (
        ADI_GPIO_PORT_C,
        ADI_GPIO_PIN_4 | ADI_GPIO_PIN_14
    );

    result = adi_gpio_Clear (
        ADI_GPIO_PORT_E,
        ADI_GPIO_PIN_1 | ADI_GPIO_PIN_13 | ADI_GPIO_PIN_15
    );

    result = adi_gpio_Clear (
        ADI_GPIO_PORT_F,
        ADI_GPIO_PIN_11
    );

}

#endif

/***********************************************************************
 * SAM Board (SC589 based) GPIO definitions and initialization
 **********************************************************************/
#ifdef __SAM_V1__
/*
 * The port FER and MUX settings are detailed in:
 *    ADSP-SC582_583_584_587_589_ADSP-21583_584_587.pdf
 */

/* EMAC0 GPIO FER bit positions (one bit per FER entry) */
#define EMAC0_MDIO_PORTA_FER          (1 << BITP_PORT_DATA_PX3)
#define EMAC0_MDC_PORTA_FER           (1 << BITP_PORT_DATA_PX2)
#define EMAC0_RXD0_PORTA_FER          (1 << BITP_PORT_DATA_PX4)
#define EMAC0_RXD1_PORTA_FER          (1 << BITP_PORT_DATA_PX5)
#define EMAC0_RXD2_PORTA_FER          (1 << BITP_PORT_DATA_PX8)
#define EMAC0_RXD3_PORTA_FER          (1 << BITP_PORT_DATA_PX9)
#define EMAC0_TXD0_PORTA_FER          (1 << BITP_PORT_DATA_PX0)
#define EMAC0_TXD1_PORTA_FER          (1 << BITP_PORT_DATA_PX1)
#define EMAC0_TXD2_PORTA_FER          (1 << BITP_PORT_DATA_PX12)
#define EMAC0_TXD3_PORTA_FER          (1 << BITP_PORT_DATA_PX13)
#define EMAC0_TXEN_PORTA_FER          (1 << BITP_PORT_DATA_PX10)
#define EMAC0_CRS_PORTA_FER           (1 << BITP_PORT_DATA_PX7)
#define EMAC0_RXCLK_REFCLK_PORTA_FER  (1 << BITP_PORT_DATA_PX6)
#define EMAC0_TXCLK_PORTA_FER         (1 << BITP_PORT_DATA_PX11)

/* EMAC GPIO MUX bit positions (two bits per MUX entry, all mux 0) */
#define EMAC0_MDIO_PORTA_MUX          (0 << (BITP_PORT_DATA_PX3 << 1))
#define EMAC0_MDC_PORTA_MUX           (0 << (BITP_PORT_DATA_PX2 << 1))
#define EMAC0_RXD0_PORTA_MUX          (0 << (BITP_PORT_DATA_PX4 << 1))
#define EMAC0_RXD1_PORTA_MUX          (0 << (BITP_PORT_DATA_PX5 << 1))
#define EMAC0_RXD2_PORTA_MUX          (0 << (BITP_PORT_DATA_PX8 << 1))
#define EMAC0_RXD3_PORTA_MUX          (0 << (BITP_PORT_DATA_PX9 << 1))
#define EMAC0_TXD0_PORTA_MUX          (0 << (BITP_PORT_DATA_PX0 << 1))
#define EMAC0_TXD1_PORTA_MUX          (0 << (BITP_PORT_DATA_PX1 << 1))
#define EMAC0_TXD2_PORTA_MUX          (0 << (BITP_PORT_DATA_PX12 << 1))
#define EMAC0_TXD3_PORTA_MUX          (0 << (BITP_PORT_DATA_PX13 << 1))
#define EMAC0_TXEN_PORTA_MUX          (0 << (BITP_PORT_DATA_PX10 << 1))
#define EMAC0_CRS_PORTA_MUX           (0 << (BITP_PORT_DATA_PX7 << 1))
#define EMAC0_RXCLK_REFCLK_PORTA_MUX  (0 << (BITP_PORT_DATA_PX6 << 1))
#define EMAC0_TXCLK_PORTA_MUX         (0 << (BITP_PORT_DATA_PX11 << 1))

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

void gpio_init(void)
{
    static uint8_t gpioMemory[ADI_GPIO_CALLBACK_MEM_SIZE];
    uint32_t numCallbacks;

    ADI_GPIO_RESULT  result;

    /* Configure ETH0 Alternate Function GPIO */
    *pREG_PORTA_FER |= (
        EMAC0_MDIO_PORTA_FER |
        EMAC0_MDC_PORTA_FER |
        EMAC0_RXD0_PORTA_FER |
        EMAC0_RXD1_PORTA_FER |
        EMAC0_TXD0_PORTA_FER |
        EMAC0_TXD1_PORTA_FER |
        EMAC0_RXD2_PORTA_FER |
        EMAC0_RXD3_PORTA_FER |
        EMAC0_TXD2_PORTA_FER |
        EMAC0_TXD3_PORTA_FER |
        EMAC0_TXCLK_PORTA_FER |
        EMAC0_TXEN_PORTA_FER |
        EMAC0_CRS_PORTA_FER |
        EMAC0_RXCLK_REFCLK_PORTA_FER
    );

    *pREG_PORTA_MUX |= (
        EMAC0_MDIO_PORTA_MUX |
        EMAC0_MDC_PORTA_MUX |
        EMAC0_RXD0_PORTA_MUX |
        EMAC0_RXD1_PORTA_MUX |
        EMAC0_TXD0_PORTA_MUX |
        EMAC0_TXD1_PORTA_MUX |
        EMAC0_RXD2_PORTA_MUX |
        EMAC0_RXD3_PORTA_MUX |
        EMAC0_TXD2_PORTA_MUX |
        EMAC0_TXD3_PORTA_MUX |
        EMAC0_TXCLK_PORTA_MUX |
        EMAC0_TXEN_PORTA_MUX |
        EMAC0_CRS_PORTA_MUX |
        EMAC0_RXCLK_REFCLK_PORTA_MUX
    );

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

    result = adi_gpio_Init(gpioMemory, sizeof(gpioMemory), &numCallbacks);

    result = adi_gpio_SetDirection(
        ADI_GPIO_PORT_D,
        ADI_GPIO_PIN_1 | ADI_GPIO_PIN_2 | ADI_GPIO_PIN_3,
        ADI_GPIO_DIRECTION_OUTPUT
    );

    result = adi_gpio_Clear (
        ADI_GPIO_PORT_D,
        ADI_GPIO_PIN_1 | ADI_GPIO_PIN_2 | ADI_GPIO_PIN_3
    );

    /* Init Ethernet reset pin (leave in reset)*/
    result = adi_gpio_Clear (
        ADI_GPIO_PORT_B,
        ADI_GPIO_PIN_7
    );
    result = adi_gpio_SetDirection(
        ADI_GPIO_PORT_B,
        ADI_GPIO_PIN_7,
        ADI_GPIO_DIRECTION_OUTPUT
    );

    /* PADS0 DAI0/1 Port Input Enable Control Register */
    *pREG_PADS0_DAI0_IE = (unsigned int)0x001FFFFE;
    *pREG_PADS0_DAI1_IE = (unsigned int)0x001FFFFE;
}

#endif

/***********************************************************************
 * System Clock Initialization
 **********************************************************************/
void system_clk_init(void)
{
    ADI_PWR_RESULT ePwrResult;
    uint32_t cclk,sclk,sclk0,sclk1,dclk,oclk;

    /* Initialize ADI power service */
    ePwrResult = adi_pwr_Init(0, OSC_CLK);

    /* Set up core and system clocks to the values in clocks.h */
    ePwrResult = adi_pwr_SetFreq(0, CCLK, SYSCLK);

    /* Query primary clocks from CGU0 for confirmation */
    ePwrResult = adi_pwr_GetCoreFreq(0, &cclk);
    ePwrResult = adi_pwr_GetSystemFreq(0, &sclk, &sclk0, &sclk1);
    ePwrResult = adi_pwr_GetDDRClkFreq(0, &dclk);
    ePwrResult = adi_pwr_GetOutClkFreq(0, &oclk);

    /*
     * Take CGU1 out of bypass and enter full on.
     * Pg. 3-9 of ADSP-SC58x/ADSP-2158x SHARC+ Processor Hardware Reference
     */
    *pREG_CGU1_PLLCTL |= BITM_CGU_PLLCTL_PLLBPCL;
    while((*pREG_CGU1_STAT & 0xF) != 0x5);

    /*
     * ADSP-SC5xx EMAC0 require a clock of 125Mhz
     *
     * Set CGU1 to create a 250MHz core clock and 125MHz SYSCLK
     * Divide SYSCLK by 1 to derive 125MHz SYSCLK1 which can
     * be routed by the CDU via mux input 1 to EMAC0.
     */
    ePwrResult = adi_pwr_Init (1, OSC_CLK);
    ePwrResult = adi_pwr_SetFreq(1, 250000000, 125000000);
    ePwrResult = adi_pwr_SetClkDivideRegister(1, ADI_PWR_CLK_DIV_S1SEL, 1);

    ePwrResult = adi_pwr_GetCoreFreq(1, &cclk);
    ePwrResult = adi_pwr_GetSystemFreq(1, &sclk, &sclk0, &sclk1);
    ePwrResult = adi_pwr_GetDDRClkFreq(1, &dclk);
    ePwrResult = adi_pwr_GetOutClkFreq(1, &oclk);

    /*
     * Set EMAC0 GigE/RGMII interface clock to SCLK1_1 and enable
     * Table 4-2, Pg. 4-3 of ADSP-SC58x/ADSP-2158x SHARC+ Processor Hardware Reference
     */
    ePwrResult = adi_pwr_ConfigCduInputClock(ADI_PWR_CDU_CLKIN_1, ADI_PWR_CDU_CLKOUT_7);
    ePwrResult = adi_pwr_EnableCduClockOutput(ADI_PWR_CDU_CLKOUT_7, true);

}

/***********************************************************************
 * GIC Initialization
 **********************************************************************/
void gic_init(void)
{
    ADI_GIC_RESULT  result;

    result = adi_gic_Init();

#ifdef FREE_RTOS
    /*
     * Setup peripheral interrupt priorities:
     *   Details: FreeRTOSv9.0.0/portable/GCC/ARM_CA9/port.c (line 574)
     *
     * All registered system interrupts can be identified by setting a breakpoint in
     * adi_rtl_register_dispatched_handler().  Only interrupts that need to call FreeRTOS
     * functions must be registered with the required interrupt priority.
     */
    adi_gic_SetBinaryPoint(ADI_GIC_CORE_0, 0);
    adi_gic_SetIntPriority(INTR_SPI0_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_SPI1_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_SPI2_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_TWI0_DATA, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_TWI1_DATA, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_TWI2_DATA, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_UART0_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_UART1_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_UART2_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_SPORT0_B_DMA, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_SPORT1_B_DMA, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);

    /* TMR0, INTR_USB0_DATA is used for UAC2.0 */
    adi_gic_SetIntPriority(INTR_TIMER0_TMR0, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_USB0_DATA, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    adi_gic_SetIntPriority(INTR_USB0_STAT, configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);

    /* WARNING: The ADI FreeRTOS port uses TMR7 as the tick timer which must be configured as the lowest
     *          priority interrupt.  If you're using the stock ADI v9.0.0 or v10.0.1 port, be sure to
     *          enable the line below.  This countermeasure has been applied to the reusable module
     *          FreeRTOS v10.2.1.
     *
     *    The SysTick handler needs to run at the lowest priority.  This is because the critical section
     *    within the handler itself assumes it is running at the lowest priority, so saves time by not
     *    saving the old priority mask and then restoring the previous priority mask.
     */
    //adi_gic_SetIntPriority(INTR_TIMER0_TMR7, 30 << portPRIORITY_SHIFT);
#endif
}

/***********************************************************************
 * CGU Timestamp init
 **********************************************************************/
void cgu_ts_init(void)
{
    /* Configure the CGU timestamp counter.  See clocks.h for more detail. */
    *pREG_CGU0_TSCTL =
        ( 1 << BITP_CGU_TSCTL_EN ) |
        ( CGU_TS_DIV << BITP_CGU_TSCTL_TSDIV );
}


