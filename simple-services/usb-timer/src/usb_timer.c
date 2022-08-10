#include "usb_timer_cfg.h"
#include "usb_timer.h"

#ifdef FREE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #define USB_TIMER_CRITICAL_ENTRY taskENTER_CRITICAL
    #define USB_TIMER_CRITICAL_EXIT  taskEXIT_CRITICAL
#else
    #define USB_TIMER_CRITICAL_ENTRY
    #define USB_TIMER_CRITICAL_EXIT
#endif

#ifndef USB_TIMER_MAX_CALLBACKS
#define USB_TIMER_MAX_CALLBACKS 4
#endif

/* Use default 450MHz CCLK if not otherwise defined */
#ifndef SCLK0
#define SCLK0                        (450000000 / 4)
#endif

#define USB_125US_TIMER_TICK_PERIOD ((125ULL * SCLK0) / 1000000ULL)

typedef struct _USB_CALLBACK {
    USB_TIMER_CALLBACK cb;
    void *usr;
} USB_CALLBACK;

static USB_CALLBACK USB_TIMERS[USB_TIMER_MAX_CALLBACKS] = { { NULL, NULL } };
static unsigned usbTimerIdx = 0;

static void usb_timer_handler(void *pCBParam, uint32_t Event, void *pArg)
{
    unsigned i;
    for (i = 0; i < usbTimerIdx; i++) {
        USB_TIMERS[i].cb(USB_TIMERS[i].usr);
    }
}

ADI_TMR_HANDLE usb_timer_start(uint32_t num)
{
    static unsigned char TimerMemory[ADI_TMR_MEMORY];
    ADI_TMR_HANDLE hTimer;
    ADI_TMR_RESULT eTmrResult;

    /* Open the timer */
    eTmrResult = adi_tmr_Open (
        num, TimerMemory, ADI_TMR_MEMORY,
        usb_timer_handler, NULL, &hTimer
    );

    /* Set the mode to PWM OUT */
    eTmrResult = adi_tmr_SetMode(hTimer, ADI_TMR_MODE_CONTINUOUS_PWMOUT);

    /* Set the IRQ mode to get interrupt after timer counts to Delay + Width */
    eTmrResult = adi_tmr_SetIRQMode(hTimer, ADI_TMR_IRQMODE_WIDTH_DELAY);

    /* Set the Period */
    eTmrResult = adi_tmr_SetPeriod(hTimer, USB_125US_TIMER_TICK_PERIOD);

    /* Set the timer width */
    eTmrResult = adi_tmr_SetWidth(hTimer, USB_125US_TIMER_TICK_PERIOD / 2);

    /* Set the timer delay */
    eTmrResult = adi_tmr_SetDelay(hTimer, 100);

    /* Enable the timer */
    eTmrResult = adi_tmr_Enable(hTimer, true);

    return(hTimer);
}

bool usb_timer_register(USB_TIMER_CALLBACK cb, void *usr)
{
    bool ok;

    USB_TIMER_CRITICAL_ENTRY();
    if (usbTimerIdx < USB_TIMER_MAX_CALLBACKS) {
        USB_TIMERS[usbTimerIdx].cb = cb;
        USB_TIMERS[usbTimerIdx].usr = usr;
        usbTimerIdx++;
    } else {
        ok = false;
    }
    USB_TIMER_CRITICAL_EXIT();
    return(ok);
}

