/* Kernel includes. */
#ifdef FREE_RTOS
    #include "FreeRTOS.h"
#endif

/* ADI service includes */
#include <services/tmr/adi_tmr.h>

/* Application includes */
#include "clocks.h"
#include "uac20.h"
#include "user_audio.h"

/**
 * 125uS UAC2.0 timer clock
 */
#define UAC20_125US_CLOCK_TICK_PERIOD ((125UL * SCLK0) / 1000000UL)

/**
 * Mainline state machine states.
 */
typedef enum
{
    MAIN_STATE_SYSTEM_INIT,                     /*!< Initialize the system */
    MAIN_STATE_USER_INIT_CODEC,                 /*!< Initialize the DAC and ADC CODECs */
    MAIN_STATE_USER_INIT,                       /*!< Initialize the User code */
    MAIN_STATE_RUN,                             /*!< Runtime state */
    MAIN_STATE_ERROR                            /*!< Error State */
} Main_States;

/***********************************************************************
 * Prototypes
 **********************************************************************/
static void timer_init (void);

/***********************************************************************
 * Public functions
 **********************************************************************/
int cld_uac20_init(void)
{
    return(0);
}

int cld_uac20_run(void)
{
    static Main_States main_state = MAIN_STATE_SYSTEM_INIT;
    User_Init_Return_Code rv;

    switch (main_state) {

        case MAIN_STATE_SYSTEM_INIT:
            timer_init();
            main_state = MAIN_STATE_USER_INIT_CODEC;
            break;

        case MAIN_STATE_USER_INIT_CODEC:
            main_state = MAIN_STATE_USER_INIT;
            break;

        case MAIN_STATE_USER_INIT:
            rv = user_audio_init();
            if (rv == USER_AUDIO_INIT_SUCCESS) {
                main_state = MAIN_STATE_RUN;
            }
            else if (rv == USER_AUDIO_INIT_FAILED) {
                main_state = MAIN_STATE_ERROR;
            }
            break;

        case MAIN_STATE_RUN:
             user_audio_main();
            break;

        case MAIN_STATE_ERROR:
            break;
    }

    return(0);
}

/***********************************************************************
 * Private functions
 **********************************************************************/
static unsigned char TimerMemory[ADI_TMR_MEMORY];

/**
 * User defined Timer ISR.
 *
 * @param pCBParam, Event, pArg.
 */
static void uac20_audio_timer_handler(void *pCBParam, uint32_t Event, void *pArg)
{
    switch(Event)
    {
        case ADI_TMR_EVENT_DATA_INT:
            cld_time_125us_tick();
            break;

        default:
            break;
    }

#ifdef FREE_RTOS
    portYIELD_FROM_ISR(pdFALSE);
#endif
}

/**
* @brief Initializes Timer service.
*
* @details
* Initializes the ADI timer system service, and configures the timer to
* generate a interrupt every 125us.
*/
static void timer_init (void)
{
    ADI_TMR_HANDLE   hTimer;
    ADI_TMR_RESULT   eTmrResult;

    /* Configure a 1ms Timer using the ADI system services */

    /* Open the timer */
    if( (eTmrResult = adi_tmr_Open (0,
                        TimerMemory,
                        ADI_TMR_MEMORY,
                        uac20_audio_timer_handler,
                        NULL,
                        &hTimer)) != ADI_TMR_SUCCESS)
     {
         /* Failed to open the timer handle the error here */
     }

     /*
     * Use the GP timer's API's to configure and enable the timer
     *
     */

    /* Set the mode to PWM OUT */
    eTmrResult = adi_tmr_SetMode(hTimer, ADI_TMR_MODE_CONTINUOUS_PWMOUT);

    /* Set the IRQ mode to get interrupt after timer counts to Delay + Width */
    eTmrResult = adi_tmr_SetIRQMode(hTimer, ADI_TMR_IRQMODE_WIDTH_DELAY);

    /* Set the Period */
    eTmrResult = adi_tmr_SetPeriod(hTimer, UAC20_125US_CLOCK_TICK_PERIOD);

    /* Set the timer width */
    eTmrResult = adi_tmr_SetWidth(hTimer, UAC20_125US_CLOCK_TICK_PERIOD / 2);

    /* Set the timer delay */
    eTmrResult = adi_tmr_SetDelay(hTimer, 100);

    /* Enable the timer */
    eTmrResult = adi_tmr_Enable(hTimer, true);
}
