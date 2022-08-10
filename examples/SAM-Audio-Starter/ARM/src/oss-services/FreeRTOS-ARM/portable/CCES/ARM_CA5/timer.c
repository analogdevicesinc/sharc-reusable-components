/*****************************************************************************
    Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
*****************************************************************************/


/*
 * timer.c
 *
 */

#include <runtime/int/interrupt.h>
#include <sys/platform.h>
#include <stdlib.h>

#include "FreeRTOS.h"

/* FreeRTOS_Tick_Handler() is defined in the RTOS port layer */
extern void FreeRTOS_Tick_Handler( void );

/* Static functions used for timer setup. */
static void EnableGICTimerInt(uint32_t id);
static void SetGICTimerPrio(uint32_t id, uint32_t priority);
static void tickHandler(uint32_t id, void *param);

/* The constant ADI_CFG_GP_TMR_NUM should probably be defined in FreeRTOSConfig.h */
#ifndef ADI_CFG_GP_TMR_NUM
#define ADI_CFG_GP_TMR_NUM 7
#endif

#if !defined (__ADSPSC589_FAMILY__) && !defined (__ADSPSC573_FAMILY__)
#error "This processor family is not supported"
#endif

#if !defined (ADI_CFG_GP_TMR_NUM)
#error "Set up tick management timer is enabled but no GP timer is specified"
#endif

#if (ADI_CFG_GP_TMR_NUM >= PARAM_TIMER0_NUMTIMERS)
#error "Incorrect GP Timer number specified for the processor."
#endif

/* Macros to construct the desired register name from the timer number. */
#define  REG_NAME_STR(RN, END)  pREG_TIMER0_TMR##RN##_##END
#define  REG_NAME(RN, END)      REG_NAME_STR(RN, END)

/* Macros to construct the desired interrupt name from the timer number. */
#define  INT_NAME_STR(RN)       INTR_TIMER0_TMR##RN
#define  INT_NAME(RN)           INT_NAME_STR(RN)

#define TIMER_NUM (ADI_CFG_GP_TMR_NUM)

#define pTIMERCFG REG_NAME(ADI_CFG_GP_TMR_NUM, CFG)
#define pTIMERCNT REG_NAME(ADI_CFG_GP_TMR_NUM, CNT)
#define pTIMERPER REG_NAME(ADI_CFG_GP_TMR_NUM, PER)
#define pTIMERWID REG_NAME(ADI_CFG_GP_TMR_NUM, WID)
#define pTIMERDLY REG_NAME(ADI_CFG_GP_TMR_NUM, DLY)
#define TIMERINT (uint32_t)INT_NAME(ADI_CFG_GP_TMR_NUM)

#define SET_TIMER_MSK_BIT ((uint16_t)(1u << TIMER_NUM))
#define CLR_TIMER_MSK_BIT ((uint16_t)~(1u << TIMER_NUM))

/* Handler for GP Timer interrupt. */
static void tickHandler(uint32_t id, void *param)
{
    /* Call the RTOS tick handler */
    FreeRTOS_Tick_Handler();

    /* Clear the timer interrupt latch before returning (write-1-to-clear) */
    *pREG_TIMER0_DATA_ILAT = SET_TIMER_MSK_BIT;
}

/*
 * Set up a GP Timer as the RTOS tick interrupt source.
 *
 * This function configures a GP Timer to be used for FreeRTOS timing services.
 * The GP Timer to be used is selected by a ADI_CFG_GP_TMR_NUM macro, which is
 * defined by the FreeRTOS UI.  This function only modifies the registers, and
 * parts of registers, that apply to the given GP timer.
 */

void vConfigureTickInterrupt(void)
{
    /* GP timers are clocked from SCLK0 */
    const uint32_t nCycles =
        configCPU_CLOCK_HZ /
        configSC5xx_CGU0_SSYSEL_DIVISOR /
        configSC5xx_CGU0_S0SEL_DIVISOR /
        configTICK_RATE_HZ;

    /* Write 1 to configure the timer to "abrupt halt" configuration (doesn't stop timer) */
    *pREG_TIMER0_STOP_CFG_SET    =  SET_TIMER_MSK_BIT;

    /* Write 1 to set the timer to stop (does stop timer). */
    *pREG_TIMER0_RUN_CLR         =  SET_TIMER_MSK_BIT;

    /* Clear any interrupts (write 1 to clear) */
    *pREG_TIMER0_DATA_ILAT       =  SET_TIMER_MSK_BIT;
    *pREG_TIMER0_STAT_ILAT       =  SET_TIMER_MSK_BIT;

    /* Hide all the interrupts (write 1 to disable interrupt)*/
    *pREG_TIMER0_DATA_IMSK       |=  SET_TIMER_MSK_BIT;
    *pREG_TIMER0_STAT_IMSK       |=  SET_TIMER_MSK_BIT;

    /* Clear the trigger output mask (1 to disable) */
    *pREG_TIMER0_TRG_MSK         |=  SET_TIMER_MSK_BIT;

    /* Clear the trigger input mask (0 to disable) */
    *pREG_TIMER0_TRG_IE          &=  CLR_TIMER_MSK_BIT;

    /* Clear the trigger input and output masks */
    *pREG_TIMER0_TRG_MSK         &=  CLR_TIMER_MSK_BIT;
    *pREG_TIMER0_TRG_IE          &=  CLR_TIMER_MSK_BIT;

    adi_rtl_register_dispatched_handler(TIMERINT,   /* GP timer        */
                                        tickHandler,  /* Timer handler   */
                                        NULL);      /* No callback arg */

    EnableGICTimerInt(TIMERINT);

    /* Set the tick timer to the minimum interrupt priority */
    SetGICTimerPrio(TIMERINT, 30 << portPRIORITY_SHIFT);

    /* Clear all registers before starting */
    *pTIMERCFG = 0u;
    *pTIMERDLY = 0u;
    *pTIMERWID = 0u;
    *pTIMERPER = 0u;

    /* Configure the individual timer */
    *pTIMERCFG = (uint16_t)(  ENUM_TIMER_TMR_CFG_CLKSEL_SCLK   /* Use the system clock as a  source */
                            | ENUM_TIMER_TMR_CFG_IRQMODE1      /* Generate pulse when Delay is over */
                            | ENUM_TIMER_TMR_CFG_PWMCONT_MODE  /* Continuous PWM mode */
                            | ENUM_TIMER_TMR_CFG_POS_EDGE      /* Interrupt on the positive pulse edge */
                            | ENUM_TIMER_TMR_CFG_EMU_NOCNT);   /* Timer stops during emulation */

    /* Set the timer delay */
#if 0
    *pTIMERDLY = nCycles;          /* Interrupt generated at this count */
    *pTIMERWID = 0x1u;             /* Pulse held high for 1 cycle       */
    *pTIMERPER = nCycles + 0x1u;   /* Sequence restarts after these cycles       */
#else
    *pTIMERDLY = nCycles - 0x1u;   /* Interrupt generated at this count */
    *pTIMERWID = 0x1u;             /* Pulse held high for 1 cycle       */
    *pTIMERPER = nCycles;          /* Sequence restarts after these cycles       */
#endif

    /* Enable data interrupts (0 for enable, 1 for disable) */
    *pREG_TIMER0_DATA_IMSK   &=  CLR_TIMER_MSK_BIT;

    /* Start the timer */
    *pREG_TIMER0_RUN_SET = SET_TIMER_MSK_BIT;
}

/* Enable the GIC interrupt. */
static void EnableGICTimerInt(uint32_t id)
{
    /* Point to the base of the enable registers,
     * each enable register refers to 32 interrupts.
     * Interrupt IDs start at 32 for the Cortex A5, so bit 1
     * of the pREG_GICDST0_SPI_EN_SET0 applies to interrupt 32. */
    volatile uint32_t *pSetReg = pREG_GICDST0_SPI_EN_SET0;

    /* The following rotates to divide by 32. On Cortex-A the
     * result will always be at least 1 (IDs start at 32) so we
     * need to take that into account when indexing later.
     */
    const uint32_t nRegIndex  = id >> 5ul;      /* Shift to divide by 32. */

    /* Extract the bit position from the lower 5 bits of the
     * interrupt id. */
    const uint32_t nBitNum    = id & 0x1Ful;

    /* Subtract by 1 to get the correct GIC register.
     * IDs will always be 32 or higher, so the above arithmetic
     * will always yield at least 1.  The values passed to this
     * function will always be valid interrupt IDs. */
    pSetReg[nRegIndex - 1] = (1ul << nBitNum);
}

/* Set the GIC interrupt priority. */
static void SetGICTimerPrio(uint32_t id, uint32_t priority)
{
    /* Point to the base of the priority registers */
    volatile uint8_t *pPrioReg = (volatile uint8_t *)( REG_GICDST0_SGI_PRIO0);

    /* Set the priority */
    pPrioReg[id] = priority;
}
