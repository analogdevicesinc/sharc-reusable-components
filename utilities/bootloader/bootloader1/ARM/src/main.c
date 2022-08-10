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

/* Simple driver includes */
#include "spi_simple.h"

/* Console / command line includes */
#include "console/platform.h"

/* Flash driver includes */
#include "flash.h"
#include "mt25ql512.h"
#include "is25lp512.h"

/* CLD includes */
#include "cdc/cdc.h"

#include "init.h"

static FLASH_INFO *fi;

void print_banner(void)
{
    printf("\n");
    printf("------------------------------\n");
    printf("ADI SAM Board USB Bootloader   \n");
    printf("Version 2.0.0\n");
    printf("------------------------------\n");
}

int main(int argc, char *argv[])
{
    sSPI *spiHandle;
    sSPIPeriph *flashHandle;
    SPI_SIMPLE_RESULT spiResult;
    int ledToggle;

    /* Initialize system clocks */
    system_clk_init();

    /* Initialize the GIC */
    gic_init();

    /* Initialize GPIO */
    gpio_init();

    /* Initialize the simple SPI driver */
    spiResult = spi_init();

    /* Initialize the console service */
    platform_init();

    /* Print Hello */
    print_banner();

    /* Open a SPI handle to the flash */
    spiResult = spi_open(SPI2, &spiHandle);
    spiResult = spi_openDevice(spiHandle, &flashHandle);
    spiResult = spi_setClock(flashHandle, 1);
    spiResult = spi_setMode(flashHandle, SPI_MODE_3);
    spiResult = spi_setFastMode(flashHandle, true);
    spiResult = spi_setLsbFirst(flashHandle, false);
    spiResult = spi_setSlaveSelect(flashHandle, SPI_SSEL_1);

    /* Try to open the Micron flash (HW Rev 1.4 and below) then
     * the ISSI flash (HW Rev 1.5 and above).
     */
    fi = mt25q_open(flashHandle);
    if (fi == NULL) {
        fi = is25lp_open(flashHandle);
    }

    /* Stop on flash errors */
    if (fi == NULL) {
        printf("Flash memory error.  Cannot continue.\n");
        while(1);
    }

    printf("Waiting for USB connection...\n");

    /* Turn on the bootloader indicator LED */
    *pREG_PORTD_DATA_SET = BITM_PORT_DATA_PX1;
    ledToggle = 0;

    while (1) {
        cdc_main_loop();
        if (ledToggle == 1000000) {
            *pREG_PORTD_DATA_TGL = BITM_PORT_DATA_PX1;
            ledToggle = 0;
        }
        ledToggle++;
    }
}
