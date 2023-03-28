# oss-services/xmodem

## Overview

This component implements an xmodem receive protocol.  It was borrowed from the eLua project and enhanced to allow more flexibility handling received data.

## Required components

- None

## Recommended components

- Shell

## Integrate the source

- Copy the 'src' directory contents into an appropriate source directory in the host project.
- Copy the 'inc' directory contents into an approporiate configuration include directory in the host project.

## Run

- Call `xmodem_receive()` whenever required.

## Initialization example

The example below was taken from a non-RTOS implementation and busy spins waiting for characters to arrive.  A better approach would be to use an RTOS and configure the UART driver to block for requested timeout.

```C
/***********************************************************************
 * Variables
 **********************************************************************/
 sUART *stdioUartHandle = NULL;

/***********************************************************************
 * XMODEM character I/O functions realized with a blocking uart write
 * and a non-blocking uart read with integrated timeout.
 **********************************************************************/
static void xmodemPutChar(unsigned char c, void *usr)
{
    uint8_t writeLen = 1;
    uart_write(stdioUartHandle, &c, &writeLen);
}

static int xmodemGetChar(int timeout, void *usr)
{
    uint8_t readLen;
    UART_SIMPLE_RESULT uartResult;
    unsigned start;
    unsigned char c;

    timeout /= 1000;

    uart_setTimeouts(stdioUartHandle,
            UART_SIMPLE_TIMEOUT_NONE, UART_SIMPLE_TIMEOUT_NO_CHANGE);

    start = getTime();
    do {
        readLen = 1;
        uartResult = uart_read(stdioUartHandle, &c, &readLen);
    } while ((readLen == 0) && (elapsedTime(start) < timeout));

    if (readLen == 0) {
        return(-1);
    }

    return(c);
}

int main(int argc, char **argv)
{
    UART_SIMPLE_RESULT uartResult;

    /* Initialize GPIO */
    gpio_init();

    /* Initialize the SEC */
    adi_sec_Init();

    ...
    ...
    ...

    /* Initialize the simple UART driver */
    uartResult = uart_init();

    /* Open UART0 as the console device (115200,N,8,1) */
    uartResult = uart_open(UART0, &stdioUartHandle);
    uart_setProtocol(stdioUartHandle,
        UART_SIMPLE_BAUD_115200, UART_SIMPLE_8BIT,
        UART_SIMPLE_PARITY_DISABLE, UART_SIMPLE_STOP_BITS1
    );

    /* Register the UART stdio driver with the console device */
    uart_stdio_init(stdioUartHandle);

    ...
    ...
    ...

}
```

## File transfer example

```C
/***********************************************************************
 * XMODEM helper functions
 **********************************************************************/
struct fileWriteState {
   FILE *f;
};

int fileDataWrite(unsigned char *data, unsigned size, bool final, void *usr)
{
   struct fileWriteState *state = (struct fileWriteState *)usr;
   size_t wsize;

   if (size > 0) {
      wsize = fwrite(data, sizeof( unsigned char ), size, state->f);
      if (wsize != size) {
         return(XMODEM_ERROR_CALLBACK);
      }
   }
   return(XMODEM_ERROR_NONE);
}

static void xmodem_putchar(unsigned char c, void *usr)
{
    putc(data, stdout); fflush(stdout);
}

static int xmodem_getchar(int timeout, void *usr)
{
    int c;

    uart_stdio_set_read_timeout(timeout / 1000);

    if ((c = getc(stdin)) == EOF) {
        return(-1);
    }

    return(c);
}

void shell_recv( int argc, char **argv )
{
    struct fileWriteState fileState;
    long size;

    // we've received an argument, try saving it to a file
    if( argc == 2 )
    {
        fileState.f = fopen( argv[ 1 ], "w" );
        if( fileState.f == NULL ) {
            printf( "unable to open file %s\n", argv[ 1 ] );
            return;
        }
        printf( "Waiting for file ... " );
        size = xmodem_receive(fileDataWrite, &fileState,
            xmodem_putchar, xmodem_getchar);
        if (size < 0) {
            printf( "XMODEM Error: %ld\n", size);
        } else {
            printf( "Received and saved as %s\n", argv[ 1 ] );
        }
        fclose ( fileState.f );
    }
}

```

## Info

- The `xmodem_receive()` command allocates two ~1k transfer buffers on the heap.  The heap functions can be overridden in `xmodem_cfg.h`.
- The `xmodem_receive()` timeouts are enforced by the blocking character receive function passed through the init call.  The xmodem module itself has no sense of time.
