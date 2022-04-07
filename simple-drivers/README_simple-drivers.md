# simple-drivers

## peripherals
The `peripherals` directory contains simple device drivers for the most commonly used SC589 peripherals.  Additional documentation for the simple SPI, TWI, SPORT, PCG and UART drivers can be found by browsing to `docs/html/index.html`.

## stdio
The `stdio` directory contains code to bridge stdin, stdout, and stderr file descriptors to a peripheral device, most commonly a UART for a console command-line shell.

## devio
The `devio` directory contains code to bridge the standard library device I/O to storage devices like
the romfs.

## flash
The `flash` directory contains a generic flash driver interface `flash.c` and a collection of drivers for specific devices.

## lwip
The `lwip` directory contains lwip related netif drivers.

## uart_cdc
The `uart-cdc` directory contains a `uart_simple` compatible interface driver over the 3rd-party CLD UAC2+CDC driver.

## msd
The `msd` directory contains a Host Mass Storage device driver for use over the msd-host simple service and 3rd-party CLD Mass Storage drvice driver library.

## sd
The `sd` directory contains a SD card device driver for use over the ADI rsi device driver.
