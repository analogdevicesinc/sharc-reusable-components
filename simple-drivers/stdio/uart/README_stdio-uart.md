# simple-drivers/stdio/uart

## Overview
The stdio-uart driver provides a bridge between the the Analog Devices Device I/O layer and the simple-uart device driver allowing for standard I/O over a serial link.

By default, the driver operates with the UART driver in blocking mode for both reads and writes.  This driver provides the ability to configure the UART with a read timeout.

## Required components

- uart_simple device driver
- uart_simple_cdc (if stdio operation over USB is desired)

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

- None

## Run
- Configure a simple-uart device handle as required by the project.
- Call `uart_stdio_init()` with the uart handle.
- Use stdio functions as required.
- Use `uart_stdio_set_read_timeout()` to enable read timeouts, or change the read blocking mode.

```C
/* Standard includes */
#include <stdio.h>
#include <string.h>

/* Simple driver includes */
#include "uart_simple.h"
#include "uart_stdio.h"

UART_SIMPLE_RESULT uartResult;
sUART *consoleUart;

/* Initialize and configure the simple UART driver for the console */
uartResult = uart_init();

/* Open UART0 as the console device (115200,N,8,1) */
uartResult = uart_open(UART0, &consoleUart);
uart_setProtocol(consoleUart,
    UART_SIMPLE_BAUD_115200, UART_SIMPLE_8BIT,
    UART_SIMPLE_PARITY_DISABLE, UART_SIMPLE_STOP_BITS1
);

/* Initialize the UART stdio driver */
uart_stdio_init(consoleUart);

/* From here on, stdio will be through the UART */
printf("Hello world!\n");
```

## Info

- If using the `uart-cdc` simple driver, replace all `uart_` API calls with `uart_cdc_` and compile this module with `USB_CDC_STDIO` defined.  It's also advisable to wait for a second or two after starting the UAC2+CDC driver before doing any stdio operations to allow USB enumeration to complete.
