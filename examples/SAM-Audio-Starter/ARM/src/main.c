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

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/* CCES includes */
#include <services/gpio/adi_gpio.h>
#include <sys/adi_core.h>

/* Simple driver includes */
#include "spi_simple.h"
#include "twi_simple.h"
#include "sport_simple.h"
#include "uart_simple.h"
#include "uart_stdio.h"
#include "sdcard_simple.h"

/* Simple service includes */
#include "buffer_track.h"
#include "cpu_load.h"
#include "syslog.h"
#include "sae.h"
#include "fs_devman.h"
#include "fs_devio.h"
#include "fs_dev_romfs.h"
#include "fs_dev_fatfs.h"

/* oss-services includes */
#include "shell.h"
#include "umm_malloc.h"
#include "romfs.h"
#include "xmodem.h"

/* Project includes */
#include "context.h"
#include "init.h"
#include "clocks.h"
#include "util.h"
#include "ipc.h"
#include "uac2.h"

/* Application context */
APP_CONTEXT mainAppContext;

/* Select proper driver API for stdio operations */
#ifdef USB_CDC_STDIO
#define uart_open uart_cdc_open
#define uart_setProtocol uart_cdc_setProtocol
#else
#define uart_open uart_open
#define uart_setProtocol uart_setProtocol
#endif

/***********************************************************************
 * Shell character I/O functions
 **********************************************************************/
static void term_out( char data )
{
    putc(data, stdout); fflush(stdout);
}

static int term_in( int mode )
{
    int c;
    int timeout;

    if( mode == TERM_INPUT_DONT_WAIT ) {
        timeout = STDIO_TIMEOUT_NONE;
    } else {
        timeout = STDIO_TIMEOUT_INF;
    }

    uart_stdio_set_read_timeout(timeout);

    if ((c = getc(stdin)) == EOF) {
        return(-1);
    }

    return(c);
}

/***********************************************************************
 * XMODEM character I/O functions
 **********************************************************************/
static void xmodemPutChar(unsigned char c)
{
    putc(c, stdout); fflush(stdout);
}

static int xmodemGetChar(int timeout)
{
  int c;

  uart_stdio_set_read_timeout(timeout/1000);

  if ((c = getc(stdin)) == EOF) {
      return(-1);
  }

  return(c);
}

/***********************************************************************
 * CPU idle time / High precision timestamp functions
 **********************************************************************/
uint32_t getTimeStamp(void)
{
    uint32_t timeStamp;
    timeStamp = *pREG_CGU0_TSCOUNT0;
    return timeStamp;
}

void taskSwitchHook(void *taskHandle)
{
    cpuLoadtaskSwitchHook(taskHandle);
}

uint32_t elapsedTimeMs(uint32_t elapsed)
{
    return(((1000ULL) * (uint64_t)elapsed) / CGU_TS_CLK);
}

/***********************************************************************
 * Misc application utility functions (util.h)
 **********************************************************************/
void delay(unsigned ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

time_t util_time(time_t *tloc)
{
    APP_CONTEXT *context = &mainAppContext;
    time_t t;

    /*
     * Our time starts at zero so add 10 years + 2 days of milliseconds
     * to adjust UNIX epoch of 1970 to FAT epoch of 1980 to keep
     * FatFS happy.  See get_fattime() in diskio.c.
     *
     * https://www.timeanddate.com/date/timeduration.html
     *
     */
    vTaskSuspendAll();
    t = (context->now + (uint64_t)315532800000) / configTICK_RATE_HZ;
    xTaskResumeAll();

    if (tloc) {
        memcpy(tloc, &t, sizeof(*tloc));
    }

    return(t);
}

/***********************************************************************
 * Application IPC functions
 **********************************************************************/
SAE_RESULT ipcToCore(SAE_CONTEXT *saeContext, SAE_MSG_BUFFER *ipcBuffer, SAE_CORE_IDX core)
{
    SAE_RESULT result;

    result = sae_sendMsgBuffer(saeContext, ipcBuffer, core, true);
    if (result != SAE_RESULT_OK) {
        sae_unRefMsgBuffer(saeContext, ipcBuffer);
    }

    return(result);
}

SAE_RESULT quickIpcToCore(APP_CONTEXT *context, enum IPC_TYPE type, SAE_CORE_IDX core)
{
    SAE_CONTEXT *saeContext = context->saeContext;
    SAE_MSG_BUFFER *ipcBuffer;
    SAE_RESULT result;
    IPC_MSG *msg;

    ipcBuffer = sae_createMsgBuffer(saeContext, sizeof(*msg), (void **)&msg);
    msg->type = type;

    result = ipcToCore(saeContext, ipcBuffer, core);

    return(result);
}

static void ipcMsgHandler(SAE_CONTEXT *saeContext, SAE_MSG_BUFFER *buffer,
    void *payload, void *usrPtr)
{
    APP_CONTEXT *context = (APP_CONTEXT *)usrPtr;
    IPC_MSG *msg = (IPC_MSG *)payload;
    IPC_MSG_AUDIO *audio;
    IPC_MSG_CYCLES *cycles;
    SAE_RESULT result;

    UNUSED(context);

    /* Process the message */
    switch (msg->type) {
        case IPC_TYPE_PING:
            /* Do nothing */
            break;
        case IPC_TYPE_SHARC0_READY:
            context->sharc0Ready = true;
            break;
        case IPC_TYPE_AUDIO:
            audio = (IPC_MSG_AUDIO *)&msg->audio;
            break;
        case IPC_TYPE_CYCLES:
            cycles = (IPC_MSG_CYCLES *)&msg->cycles;
            if (cycles->core == IPC_CORE_SHARC0) {
                context->sharc0Cycles = cycles->cycles;
            } else if (cycles->core == IPC_CORE_SHARC1) {
                context->sharc1Cycles = cycles->cycles;
            }
        default:
            break;
    }

    /* Done with the message so decrement the ref count */
    result = sae_unRefMsgBuffer(saeContext, buffer);
}

/***********************************************************************
 * Tasks
 **********************************************************************/
#include "ff.h"

static void sdcardCheck(APP_CONTEXT *context)
{
    bool sd;
    SDCARD_SIMPLE_RESULT sdResult;
    SDCARD_SIMPLE_INFO sdInfo;
    char *sdTypes[] = SDCARD_TYPE_STRINGS;
    static FATFS *fs = NULL;
    FRESULT fatResult;

    /* Check for sdcard insertion/removal */
    sdResult = sdcard_present(context->sdcardHandle);
    sd = (sdResult == SDCARD_SIMPLE_SUCCESS);
    if (sd != context->sdPresent) {
        syslog_printf(
            "SDCARD: %s\n", sd ? "Inserted" : "Removed"
        );
        if (sd) {
            sdResult = sdcard_start(context->sdcardHandle);
            if (sdResult == SDCARD_SIMPLE_SUCCESS) {
                sdResult = sdcard_info(context->sdcardHandle, &sdInfo);
                if (sdResult == SDCARD_SIMPLE_SUCCESS) {
                    syslog_printf("SDCARD: Type %s\n",
                        sdTypes[sdInfo.type]
                    );
                    syslog_printf("SDCARD: Capacity %llu bytes\n",
                        (unsigned long long)sdInfo.capacity
                    );
                    fs = umm_malloc(sizeof(*fs));
                    fatResult = f_mount(fs, "sd:", 1);
                    if (fatResult == FR_OK) {
                        syslog_printf("SDCARD: FatFs mounted");
                    } else {
                        syslog_printf("SDCARD: FatFs error (%d)!", fatResult);
                    }
                } else {
                    syslog_print("SDCARD: Error getting info!\n");
                }
            } else {
                syslog_print("SDCARD: Error starting!\n");
            }
        } else {
            if (fs) {
                fatResult = f_unmount("sd:");
                if (fatResult == FR_OK) {
                    syslog_printf("SDCARD: FatFs unmount");
                } else {
                    syslog_printf("SDCARD: FatFs error (%d)!", fatResult);
                }
                umm_free(fs);
                fs = NULL;
            }
            sdcard_stop(context->sdcardHandle);
        }
        context->sdPresent = sd;
    }

}

/* Background storage polling task */
static portTASK_FUNCTION( pollStorage, pvParameters )
{
    APP_CONTEXT *context = (APP_CONTEXT *)pvParameters;

    /* Spin forever doing houseKeeping tasks */
    while (1) {
        sdcardCheck(context);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Background housekeeping task */
static portTASK_FUNCTION( houseKeepingTask, pvParameters )
{
    APP_CONTEXT *context = (APP_CONTEXT *)pvParameters;
    SAE_CONTEXT *saeContext = context->saeContext;
    SAE_MSG_BUFFER *msgBuffer;
    TickType_t flashRate, lastFlashTime, clk, lastClk;
    bool calcLoad;
    IPC_MSG *msg;

    /* Configure the LED to flash at a 1Hz rate */
    flashRate = pdMS_TO_TICKS(500);
    lastFlashTime = xTaskGetTickCount();
    lastClk = xTaskGetTickCount();

    /* Calculate the system load every other cycle */
    calcLoad = false;

    /* Spin forever doing houseKeeping tasks */
    while (1) {

        /* Calculate the system load */
        if (calcLoad) {
            cpuLoadCalculateLoad(NULL);
            calcLoad = false;
        } else {
            calcLoad = true;
        }

        /* Toggle the LED */
        adi_gpio_Toggle(ADI_GPIO_PORT_D, ADI_GPIO_PIN_1);

        /* Ping both SHARCs with the same message */
        msgBuffer = sae_createMsgBuffer(saeContext, sizeof(*msg), (void **)&msg);
        if (msgBuffer) {
            msg->type = IPC_TYPE_PING;
            sae_refMsgBuffer(saeContext, msgBuffer);
            ipcToCore(saeContext, msgBuffer, IPC_CORE_SHARC0);
            ipcToCore(saeContext, msgBuffer, IPC_CORE_SHARC1);
        }

        /* Get cycles from both SHARCs */
        msgBuffer = sae_createMsgBuffer(saeContext, sizeof(*msg), (void **)&msg);
        if (msgBuffer) {
            msg->type = IPC_TYPE_CYCLES;
            sae_refMsgBuffer(saeContext, msgBuffer);
            ipcToCore(saeContext, msgBuffer, IPC_CORE_SHARC0);
            ipcToCore(saeContext, msgBuffer, IPC_CORE_SHARC1);
        }

        clk = xTaskGetTickCount();
        context->now += (uint64_t)(clk - lastClk);
        lastClk = clk;

        /* Sleep for a while */
        vTaskDelayUntil( &lastFlashTime, flashRate );

    }
}

static void setAppDefaults(APP_CFG *cfg)
{
    cfg->usbOutChannels = USB_DEFAULT_OUT_AUDIO_CHANNELS;
    cfg->usbInChannels = USB_DEFAULT_IN_AUDIO_CHANNELS;
    cfg->usbWordSizeBits = USB_DEFAULT_WORD_SIZE_BITS;
    cfg->usbRateFeedbackHack = false;
}

static void execShellCmdFile(void)
{
    FILE *f = NULL;
    char *name = NULL;
    char cmd[32];

    name = "sd:shell.cmd";
    f = fopen(name, "r");
    if (f == NULL) {
        name = "wo:shell.cmd";
        f = fopen(name, "r");
    }
    if (f) {
        fclose(f);
        cmd[0] = '\0';
        strcat(cmd, "run "); strcat(cmd, name);
        shell_exec(NULL, cmd);
    }
}


/* System startup task -> background shell task */
static portTASK_FUNCTION( startupTask, pvParameters )
{
    APP_CONTEXT *context = (APP_CONTEXT *)pvParameters;
    SPI_SIMPLE_RESULT spiResult;
    TWI_SIMPLE_RESULT twiResult;
    SPORT_SIMPLE_RESULT sportResult;
    SDCARD_SIMPLE_RESULT sdcardResult;
    int fsckRepaired;
    int fsckOk;
    FS_DEVMAN_DEVICE *device;
    FS_DEVMAN_RESULT fsdResult;

    /* Initialize the CPU load module. */
    cpuLoadInit(getTimeStamp, CGU_TS_CLK);

    /* Initialize the simple SPI driver */
    spiResult = spi_init();

    /* Initialize the simple TWI driver */
    twiResult = twi_init();

    /* Initialize the simple SPORT driver */
    sportResult = sport_init();

    /* Intialize the filesystem device manager */
    fs_devman_init();

    /* Intialize the filesystem device I/O layer */
    fs_devio_init();

    /* Initialize the SDCARD interface */
    sdcardResult = sdcard_init();

    /* Open up a global device handle for TWI0 @ 400KHz */
    twiResult = twi_open(TWI0, &context->twi0Handle);
    if (twiResult != TWI_SIMPLE_SUCCESS) {
        syslog_print("Could not open TWI0 device handle!");
        return;
    }
    twi_setSpeed(context->twi0Handle, TWI_SIMPLE_SPEED_400);

    /* Set ad2425 (A2B), Ethernet clk gen, and adau1761 (on-board CODEC)
     * TWI handles to TWI0 */
    context->ad2425TwiHandle = context->twi0Handle;
    context->ethClkTwiHandle = context->twi0Handle;
    context->adau1761TwiHandle = context->twi0Handle;

    /* Reset the audio system MCLK to 24.576MHz */
    audio_mclk_24576_mhz(context->twi0Handle);

    /* Init the SHARC Audio Engine.  This core is configured to be the
     * IPC master so this function must run to completion before any
     * other core calls sae_initialize().
     */
    sae_initialize(&context->saeContext, SAE_CORE_IDX_0, true);

    /* Register an IPC message callback */
    sae_registerMsgReceivedCallback(context->saeContext,
        ipcMsgHandler, context);

    /* Start the SHARC cores after the IPC is ready to go */
    adi_core_enable(ADI_CORE_SHARC0);
    adi_core_enable(ADI_CORE_SHARC1);

    /* Initialize the flash */
    flash_init(context);

    /* Initialize the filesystem */
    romfs_init(context->flashHandle);

    /* Check the filesystem */
    fsckOk = romfs_fsck(1, &fsckRepaired);
    if (fsckOk != FS_OK) {
        printf("Filesystem corrupt: %s Repaired\n", fsckRepaired ? "" : "Not");
    }

    /* Hook the internal filesystem into the stdio libraries */
    device = fs_dev_romfs_device();
    fsdResult = fs_devman_register("wo:", device, NULL);

    /* Open the SDCARD driver */
    sdcardResult = sdcard_open(SDCARD0, &context->sdcardHandle);

    /* Hook the SD card filesystem into the stdio libraries */
    device = fs_dev_fatfs_device();
    fsdResult = fs_devman_register("sd:", device, NULL);

    /* Set the SD card as the default device */
    device = fs_dev_fatfs_device();
    fsdResult = fs_devman_set_default("sd:");

    /* Load configuration */
    setAppDefaults(&context->cfg);

    /* Disable all SPORT clocks during audio init */
    disable_sport_clocks(context);

    /* Initialize the IPC audio buffers in shared L2 SAE memory */
    sae_buffer_init(context);

    /* Initialize the IPC audio routing message in shared L2 SAE memory */
    audio_routing_init(context);

    /* Tell SHARC0 where to find the routing table.  Add a reference to
     * so it doesn't get destroyed upon receipt.
     */
    sae_refMsgBuffer(context->saeContext, context->routingMsgBuffer);
    ipcToCore(context->saeContext, context->routingMsgBuffer, IPC_CORE_SHARC0);

    /* Disable main MCLK/BCLK */
    disable_sport_clocks(context);

    /* Initialize the PCG to make a 12.288MHz BCLK */
    pcg_init(context);

    /* Initialize the ADAU1761 CODEC */
    adau1761_init(context);

    /* Configure the AD2425 sport to match the ADAU1761 configuration
     * which is 8 channel packed 32-bit I2S.  Enable both serializers
     * interleaved for 16x16 channel I/O.
     *
     * AD242x configuration in include/context.h
     */
    ad2425_init(context, SYSTEM_I2SGCFG, SYSTEM_I2SCFG);

    /* Enable all SPORT clocks for a synchronous start */
    enable_sport_clocks(context);

    /* Get the idle task handle */
    context->idleTaskHandle = xTaskGetIdleTaskHandle();

    /* Initialize the xmodem component */
    xmodem_init(xmodemPutChar, xmodemGetChar);

    /* Start the housekeeping tasks */
    xTaskCreate( houseKeepingTask, "HouseKeepingTask", GENERIC_TASK_STACK_SIZE,
        context, HOUSEKEEPING_PRIORITY, &context->houseKeepingTaskHandle );
    xTaskCreate( pollStorage, "PollStorageTask", GENERIC_TASK_STACK_SIZE,
        context, HOUSEKEEPING_PRIORITY, &context->pollStorageTaskHandle );

    /* Start the UAC20 task */
    xTaskCreate( uac2Task, "UAC2Task", UAC20_TASK_STACK_SIZE,
        context, UAC20_TASK_PRIORITY, &context->uac2TaskHandle );

    /* Lower the startup task priority for the shell */
    vTaskPrioritySet( NULL, STARTUP_TASK_LOW_PRIORITY);

    /* Initialize the shell */
    shell_init(&context->shell, term_out, term_in, SHELL_MODE_BLOCKING);

#ifdef USB_CDC_STDIO
    /* Delay a little bit for USB enumeration to complete to see
     * the entire shell banner.
     */
    delay(1000);
#endif

    /* Execute shell initialization command file */
    execShellCmdFile();

    /* Drop into the shell */
    while (1) {
        shell_start(&context->shell);
    }
}

int main(int argc, char *argv[])
{
    APP_CONTEXT *context = &mainAppContext;
    UART_SIMPLE_RESULT uartResult;

    /* Initialize system clocks */
    system_clk_init();

    /* Enable the CGU timestamp */
    cgu_ts_init();

    /* Initialize the application context */
    memset(context, 0, sizeof(*context));

    /* Initialize the GIC */
    gic_init();

    /* Initialize GPIO */
    gpio_init();

    /* Init the system heaps */
    heap_init();

    /* Init the system logger */
    syslog_init();

    /* Initialize the simple UART driver */
    uartResult = uart_init();

    /* Initialize the simple CDC UART driver */
    uartResult = uart_cdc_init();

    /* Open UART0 as the console device (115200,N,8,1) */
    uartResult = uart_open(UART0, &context->stdioHandle);
    uart_setProtocol(context->stdioHandle,
        UART_SIMPLE_BAUD_115200, UART_SIMPLE_8BIT,
        UART_SIMPLE_PARITY_DISABLE, UART_SIMPLE_STOP_BITS1
    );

    /* Initialize the UART stdio driver with the console device */
    uart_stdio_init(context->stdioHandle);

    /* Init the rest of the system and launch the remaining tasks */
    xTaskCreate( startupTask, "StartupTask", STARTUP_TASK_STACK_SIZE,
        context, STARTUP_TASK_HIGH_PRIORITY, &context->startupTaskHandle );

    /* Start the scheduler. */
    vTaskStartScheduler();

    return(0);
}

/*-----------------------------------------------------------
 * FreeRTOS idle hook
 *-----------------------------------------------------------*/
void vApplicationIdleHook( void )
{
}

/*-----------------------------------------------------------
 * FreeRTOS critical error and debugging hooks
 *-----------------------------------------------------------*/
void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    ( void ) pcFile;
    ( void ) ulLine;

    /* Disable interrupts so the tick interrupt stops executing, then sit in a loop
    so execution does not move past the line that failed the assertion. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(ADI_GPIO_PORT_D, ADI_GPIO_PIN_1);
    while (1);
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(ADI_GPIO_PORT_D, ADI_GPIO_PIN_1);
    while (1);
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Run time allocation failure checking is performed if
    configUSE_MALLOC_FAILED_HOOK is defined.  This hook
    function is called if an allocation failure is detected. */
    taskDISABLE_INTERRUPTS();
    adi_gpio_Set(ADI_GPIO_PORT_D, ADI_GPIO_PIN_1);
    while (1);
}

/*-----------------------------------------------------------*/
