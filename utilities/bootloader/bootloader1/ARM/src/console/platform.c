/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 *
 * Some portions of this file were borrowed from the eLua project:
 *
 * https://github.com/elua/elua/blob/master/LICENSE
 *
 * See NOTICE for the full MIT license.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#if defined(__ADSPARM__)
#include <libio/device.h>
#include <libio/device_int.h>
#else
#include <device.h>
#endif

#include "drivers/uart_simple.h"
#include "term.h"

#define PLATFORM_TIMER_NO_TIMEOUT  (0)
#define PLATFORM_TIMER_INF_TIMEOUT (-1)

#define CONSOLE_DEV          100
#define CONSOLE_STDIN_FD     0
#define CONSOLE_STDOUT_FD    1
#define CONSOLE_STDERR_FD    2

static sUART *uart0 = NULL;
static UART_SIMPLE_RESULT uartResult;

int console_init(struct DevEntry *deventry)
{
    /* Initialize the simple UART driver (ignore errors) */
    uartResult = uart_init();

    /* Open UART0 as the console device (115200,N,8,1) */
    uartResult = uart_open(UART0, &uart0);
    uart_setProtocol(uart0,
        UART_SIMPLE_BAUD_115200, UART_SIMPLE_8BIT,
        UART_SIMPLE_PARITY_DISABLE, UART_SIMPLE_STOP_BITS1
    );

    return(_DEV_IS_THREADSAFE);
}

int console_open(const char *path, int flags)
{
    return(0);
}

int console_close(int fh)
{
    uartResult = uart_close(&uart0);
    return(0);
}

int console_write(int fh, unsigned char *ptr, int len)
{
    int i;
    uint8_t writeLen = 1;
    unsigned char c = '\r';
    UART_SIMPLE_RESULT uartResult;

    if (!uart0) {
        return(-1);
    }

    for (i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            writeLen = 1;
            uartResult = uart_write(uart0, &c, &writeLen);
        }
        writeLen = 1;
        uartResult = uart_write(uart0, ptr+i, &writeLen);
    }

#if defined(__ADSPARM__)
    len = 0;
#endif

    return(len);
}

int console_read(int fh, unsigned char *buffer, int len)
{
    uint8_t readLen;
    UART_SIMPLE_RESULT uartResult;

    if (!uart0) {
        return(-1);
    }

    readLen = (len > 255) ? 255 : len;
    uartResult = uart_read(uart0, buffer, &readLen);

    if (readLen == 0) {
        len = -1;
    } else {
#if defined(__ADSPARM__)
        len = len - readLen;
#else
        len = readLen;
#endif
    }

    return(len);
}

long console_seek(int fh, long pos, int dir)
{
    return(-1);
}

int console_isatty(int fh)
{
    return(0);
}

int console_unlink(const char *path)
{
    return(-1);
}

int console_rename(const char *oldpath, const char *newpath)
{
    return(-1);
}

int console_system(const char *cmd)
{
    return(0);
}

clock_t console_times(void)
{
    return(0);
}

void console_gettimeofday(struct timeval *tp, void *tzvp)
{
}

int console_kill(int processID, int signal)
{
    return(0);
}

int console_get_errno(void)
{
    return(0);
}

DevEntry console_deventry = {
    CONSOLE_DEV,            /* int DeviceID */
    NULL,                   /* void *data */
    console_init,           /* int device _init(struct DevEntry *d) */
    console_open,           /* int device _open(const char *path, int flags) */
    console_close,          /* int device _close(int fh) */
    console_write,          /* int device _write(int fh, char *ptr, int len) */
    console_read,           /* int device _read(int fh, char *ptr, int len) */
    console_seek,           /* int device _seek(int fh, int pos, int dir) */
    CONSOLE_STDIN_FD,       /* int stdinfd */
    CONSOLE_STDOUT_FD,      /* int stdoitfd */
    CONSOLE_STDERR_FD,      /* int stderrfd */
    console_unlink,         /* int device _unlink(const char *path) */
    console_rename,         /* int device _rename(const char *oldpath, const char *newpath) */
    console_system,         /* int device _system(const char *cmd) */
    console_isatty,         /* int device _isatty(int fh) */
    console_times,          /* clock_t device _times(void) */
    console_gettimeofday,   /* void device _gettimeofday(struct timeval *tp, void *tzvp) */
    console_kill,           /* int device _kill(int processID, int signal) */
    console_get_errno       /* int device _get_errno(void) */
};
extern sDevTab     DeviceIOtable[MAXFD];

DevEntry_t DevDrvTable[MAXDEV] = {
  &console_deventry,
  0,
};

void PutChar(char c)
{
    write(CONSOLE_STDOUT_FD, &c, 1);
}

int GetChar(int timeout)
{
    char c;

    read(CONSOLE_STDIN_FD, &c, 1);

    return(c);
}

static void term_out( u8 data )
{
    PutChar( data );
}

static int term_in( int mode )
{
  if( mode == TERM_INPUT_DONT_WAIT )
    return GetChar( PLATFORM_TIMER_NO_TIMEOUT );
  else
    return GetChar( PLATFORM_TIMER_INF_TIMEOUT );
}


static int term_translate( int data )
{
  static int escape = 0;
  static int escape_char = 0;

  if (escape)
  {
   switch (escape)
   {
      case 1:
         if (data == '[')
            escape = 2;
         else if (data == 0x1B) {
            escape = 0;
            return(KC_ESC);
         }
         else
            escape = 0;
         break;
      case 2:
         if( data >= 'A' && data <= 'D' )
         {
            escape = 0;
            switch( data )
            {
               case 'A':
                  return KC_UP;
               case 'B':
                  return KC_DOWN;
               case 'C':
                  return KC_RIGHT;
               case 'D':
                  return KC_LEFT;
            }
         }
         else if( data > '0' && data < '7' )
         {
            escape_char = data;
            escape = 3;
         }
         break;
      case 3:
         escape = 0;
         if (data == '~')
         {
            switch( escape_char )
            {
               case '1':
                  return KC_HOME;
               case '4':
                  return KC_END;
               case '5':
                  return KC_PAGEUP;
               case '6':
                  return KC_PAGEDOWN;
            }
         }
         break;
      default:
         break;
   }
   return KC_UNKNOWN;
  }
  else if( isprint( data ) )
    return data;
  else if( data == 0x1B ) // escape sequence
  {
     escape = 1;
  }
  else if( data == 0x0D )
  {
    return KC_ENTER;
  }
  else
  {
    switch( data )
    {
      case 0x09:
        return KC_TAB;
      case 0x7F:
//        return KC_DEL; // bogdanm: some terminal emulators (for example screen) return 0x7F for BACKSPACE :(
      case 0x08:
        return KC_BACKSPACE;
      case 26:
        return KC_CTRL_Z;
      case 1:
        return KC_CTRL_A;
      case 5:
        return KC_CTRL_E;
      case 3:
        return KC_CTRL_C;
      case 20:
        return KC_CTRL_T;
      case 21:
        return KC_CTRL_U;
      case 11:
        return KC_CTRL_K;
    }
  }
  return KC_UNKNOWN;
}

void platform_init(void)
{
    set_default_io_device(CONSOLE_DEV);
    /*
     * newlib line buffering is broken in CCES 2.8.3 (latest as
     * of the time of this comment).  Lines with multiple newlines
     * are truncated after the first newline so disable line buffering.
     *
     * More detail can be found here:
     *   https://github.com/espressif/esp-idf/issues/44
     */
    setvbuf(stdout, NULL, _IONBF, 0);
}
