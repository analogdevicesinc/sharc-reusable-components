# oss-services/shell

## Overview

The shell provides a command-line interface on the target and works in both FreeRTOS and "main-loop" environments.  It does this by supporting both a non-blocking and blocking mode of operation.

The shell has additional features such as command line history, command line editing, and simple mechanisms for adding project specific commands and help.

Much of this code came from the [eLua](http://www.eluaproject.net/) project.  SHARC specific enhancements include removal of eLua specific components, significant restructuring and clean-up of the code, and the addition of a context block to maintain state for non-blocking operation and multiple instances.

## Required components

- None

## Recommended components

- umm_malloc
- uart_simple device driver
- stdio-uart driver
- telnet service

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the shell.

## Configure

The shell has a number of convenient compile-time configuration options.  Please refer to the Doxygen documentation found in the [docs](./docs/html/index.html) directory for additional detail.

## Run

- Call `shell_init()` after configuring and initializing the uart_simple_stdio middleware driver.
- Call `shell_start()` to start the shell.  This call internally calls `shell_poll()` which will never return in blocking mode.  This is the normal mode of operation for a FreeRTOS based project with a dedicated background shell task.
- For non-blocking implementations, call `shell_poll()` regularly within the main loop to service incoming keystrokes and execute commands.

## Example serial command console

```C
/* Standard includes */
#include <stdio.h>
#include <string.h>

/* Simple driver includes */
#include "uart_simple.h"
#include "uart_stdio.h"

/* OSS service includes */
#include "shell.h"

/***********************************************************************
 * Shell character I/O functions
 **********************************************************************/
static void term_out( char data, void *usr )
{
    putc(data, stdout); fflush(stdout);
}

static int term_in( int mode, void *usr )
{
  int c;
  int timeout;

  if( mode == TERM_INPUT_DONT_WAIT )
    timeout = STDIO_TIMEOUT_NONE;
  else
    timeout = STDIO_TIMEOUT_INF;

  uart_stdio_set_read_timeout(timeout);

  if ((c = getc(stdin)) == EOF) {
      return(-1);
  }

  return(c);
}

int main(int argc, char **argv)
{
    UART_SIMPLE_RESULT uartResult;
    sUART *consoleUart;
    SHELL_CONTEXT shell;

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

    /* Drop into a non-blocking shell */
    shell_init(&shell, term_out, term_in, SHELL_MODE_NON_BLOCKING, NULL);
    shell_start(&shell);
    while (1) {
        shell_poll(&shell);
    }
}
```

## Info

- Project specific commands and help should be added to `shell_cmds.c`.    Commands can similarly be added to individual files if preferred.  References to any new commands must be added to the various structures and macros located in `shell.c`.  A simple `hello`command has been implemented as an example.
- if `SHELL_MAX_HISTORIES` is greater than zero, the shell will call the standard system `malloc()` and `free()` by default to allocate and free historical command lines.  Custom heap functions can be substituted by defining the `SHELL_MALLOC` and `SHELL_FREE` macros.  Be aware that heap functions provided by the standard C libraries can disable system interrupts for extended periods even when called from low priority tasks.  Using the umm_malloc middleware component avoids this problem and provides more efficient heap routines suitable for many real-time systems.
- The shell also utilizes custom versions of `strdup()` and `strndup()`as defind in `shell_string.c`.  Both functions ultimately use the memory allocator defined above.  Custom functions can be substituted by defining the `SHELL_STRDUP` and `SHELL_STRNDUP`macros.
- Multiple instances of the shell can be created and destroyed at runtime.  This feature is utilized by the telnet service.
