# simple-drivers/flash

## Overview

The flash driver is comprised of a generic flash driver interface implemented in `flash.c` and a series of device specific drivers.  Currently supported devices include:

 1. Micron mt25ql512
 2. Winbond w25q128fv
 3. Spansion s25fl512s
 4. ISSI is25lp512

Additional documentation for the simple flash drivers can be found by browsing to `docs/html/index.html`.

## Required components

- spi_simple device driver

## Recommended components

- None

## Integrate the source

- Copy `flash.c`, `flash.h` and the `.c` and `.h` of the desired device from the 'src' directory into an appropriate location in the host project.

## Configure

- None

## Run

- Pass a fully configured simple-spi device handle to the device specific `open()` function.
- Operate on the flash through the generic routines described in flash.h

```C
    #include "spi_simple.h"
    #include "flash.h"
    #include "mt25ql512.h"

    SPI_SIMPLE_RESULT spiResult;
    sSPI *spi2Handle;
    sSPIPeriph *spiFlashHandle;
    FLASH_INFO *flashHandle;
    uint8_t buf[64];
    int result;

    /* Open a SPI handle to SPI2 */
    spiResult = spi_open(SPI2, &spi2Handle);

    /* Open a SPI2 device handle for the flash */
    spiResult = spi_openDevice(spi2Handle, &spiFlashHandle);

    /* Configure the flash device handle */
    spiResult = spi_setClock(spiFlashHandle, 9);
    spiResult = spi_setMode(spiFlashHandle, SPI_MODE_3);
    spiResult = spi_setFastMode(spiFlashHandle, true);
    spiResult = spi_setLsbFirst(spiFlashHandle, false);
    spiResult = spi_setSlaveSelect(spiFlashHandle, SPI_SSEL_1);

    /* Open the flash driver with the configured SPI device handle */
    flashHandle = mt25q_open(spiFlashHandle);

    /* Read 'buf' from flash address 0 */
    result = flash_read(flashHandle, 0, buf sizeof(buf));
```

## Info

- None
