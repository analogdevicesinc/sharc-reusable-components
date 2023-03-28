/*-
 * Copyright (c) 2006 M. Warner Losh.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This software is derived from software provide by Kwikbyte who specifically
 * disclaimed copyright on the code.  This version of xmodem has been nearly
 * completely rewritten, but the CRC is from the original.
 *
 * $FreeBSD: src/sys/boot/arm/at91/libat91/xmodem.c,v 1.1 2006/04/19 17:16:49 imp Exp $
 */

// Modified by BogdanM for eLua

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "xmodem_cfg.h"
#include "xmodem.h"

// Line control codes
#define XM_SOH  0x01
#define XM_STX  0x02
#define XM_ACK  0x06
#define XM_NAK  0x15
#define XM_CAN  0x18
#define XM_EOT  0x04

// Arguments to xmodem_flush
#define XMODEM_FLUSH_ONLY       0
#define XMODEM_FLUSH_AND_XM_CAN    1

// Delay in "flush packet" mode
#define XMODEM_PACKET_DELAY        10000UL

// Maximum buffer size
#define XMODEM_MAX_BUF_SIZE        (1024 + 4)

// Utility function: flush the receive buffer
static void xmodem_flush( int how, p_xm_send_func send, p_xm_recv_func recv, void *usr )
{
  while( recv( XMODEM_PACKET_DELAY, usr ) != -1 );
  if( how == XMODEM_FLUSH_AND_XM_CAN )
  {
    send( XM_CAN, usr );
    send( XM_CAN, usr );
    send( XM_CAN, usr );
  }
}

// This private function receives a x-modem record to the pointer and
// returns 1 on success and 0 on error
static int xmodem_get_record( unsigned char blocknum, unsigned char *pbuf, unsigned psize,
    p_xm_send_func send, p_xm_recv_func recv, void *usr)
{
  unsigned chk, j, size;
  int ch;

  // Read packet
  for( j = 0; j < psize + 4; j ++ )
  {
    if( ( ch = recv( XMODEM_TIMEOUT, usr ) ) == -1 )
      goto err;
    pbuf[ j ] = ( unsigned char )ch;
  }

  // Check block number
  if( *pbuf ++ != blocknum )
    goto err;
  if( *pbuf ++ != ( unsigned char )(~blocknum & 0xFF))
    goto err;
  // Check CRC
  for( size = chk = 0; size < psize; size++, pbuf ++ )
  {
    chk = chk ^ *pbuf << 8;
    for( j = 0; j < 8; j ++ )
    {
      if( chk & 0x8000 )
        chk = chk << 1 ^ 0x1021;
      else
        chk = chk << 1;
    }
  }
  chk &= 0xFFFF;
  if( *pbuf ++ != ( ( chk >> 8 ) & 0xFF ) )
    goto err;
  if( *pbuf ++ != ( chk & 0xFF ) )
    goto err;
  return 1;

err:
  send( XM_NAK, usr );
  return 0;
}

// This global function receives a x-modem transmission consisting of
// (potentially) several blocks.  Returns the number of bytes received or
// an error code an error
long xmodem_receive(p_xm_data_func callback, void *usr,
    p_xm_send_func send, p_xm_recv_func recv)
{
  int starting = 1, ch;
  unsigned char packnum = 1;
  unsigned char *buf[2];
  unsigned retries = XMODEM_RETRY_LIMIT;
  unsigned psize[2];
  int x, size = 0;
  int inBuf, outBuf;
  int err;

  outBuf = -1;
  inBuf = -1;
  psize[0] = 128;
  psize[1] = 128;

  buf[0] = XMODEM_MALLOC(XMODEM_MAX_BUF_SIZE);
  buf[1] = XMODEM_MALLOC(XMODEM_MAX_BUF_SIZE);

  memset(buf[0], '\x1A', XMODEM_MAX_BUF_SIZE);
  memset(buf[1], '\x1A', XMODEM_MAX_BUF_SIZE);

  while( retries-- )
  {
    if( starting )
      send( 'C', usr );
    if( ( ( ch = recv( XMODEM_TIMEOUT, usr ) ) == -1 ) || ( ch != XM_SOH && ch != XM_STX && ch != XM_EOT && ch != XM_CAN ) )
      continue;
    if( ch == XM_EOT )
    {
      if (inBuf >= 0) {
        x = 2 + psize[inBuf] - 1;
        while (buf[inBuf][x] == '\x1A') {
           x--; size--;
        }
        if (callback) {
           err = callback(&buf[inBuf][2], x - 2 + 1, true, usr);
           if (err != XMODEM_ERROR_NONE) {
              size = err;
           }
        }
      }
      // End of transmission
      send( XM_ACK, usr );
      xmodem_flush( XMODEM_FLUSH_ONLY, send, recv, usr );
      XMODEM_FREE(buf[0]); XMODEM_FREE(buf[1]);
      return size;
    }
    else if( ch == XM_CAN )
    {
      // The remote part ended the transmission
      send( XM_ACK, usr );
      xmodem_flush( XMODEM_FLUSH_ONLY, send, recv, usr );
      XMODEM_FREE(buf[0]); XMODEM_FREE(buf[1]);
      return XMODEM_ERROR_REMOTECANCEL;
    }
    starting = 0;

    if (++inBuf > 1) {
       inBuf = 0;
    }

    // Get XMODEM packet
    if (ch == XM_STX)
    {
       psize[inBuf] = 1024;
    }
    else
    {
       psize[inBuf] = 128;
    }
    if( !xmodem_get_record( packnum, buf[inBuf], psize[inBuf], send, recv, usr ) )
      continue; // allow for retransmission
    xmodem_flush( XMODEM_FLUSH_ONLY, send, recv, usr );
    retries = XMODEM_RETRY_LIMIT;
    packnum ++; packnum &= 0xFF;

    if (outBuf >= 0) {
       if (callback) {
          err = callback(&buf[outBuf][2], psize[outBuf], false, usr);
          // If error, force cancel and return
          if (err != XMODEM_ERROR_NONE) {
             xmodem_flush( XMODEM_FLUSH_AND_XM_CAN, send, recv, usr );
             XMODEM_FREE(buf[0]); XMODEM_FREE(buf[1]);
             return err;
          }
       }
    }
    if (++outBuf > 1) {
       outBuf = 0;
    }

    // Acknowledge and consume packet
    send( XM_ACK, usr );
    size += psize[inBuf];
  }

  // Exceeded retry count
  xmodem_flush( XMODEM_FLUSH_AND_XM_CAN, send, recv, usr );
  XMODEM_FREE(buf[0]); XMODEM_FREE(buf[1]);
  return XMODEM_ERROR_RETRYEXCEED;
}
