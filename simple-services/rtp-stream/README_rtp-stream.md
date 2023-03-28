# simple-services/rtp-stream

## Overview

The rtp-stream service contains functions for reading and writing
Ethernet RTP data streams.

## Required components

- lwIP

## Recommended components

- umm_malloc

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header
  file in the 'inc' directory contain the configurable options for the
  rtp-stream service.

## Configure

See 'inc/rtp_stream_cfg.h' for an example utilizing 'umm_malloc' along with
the other configurable parameters.

## Receiving RTP streams

- Create a new RTP_STREAM object and zero it out
- Set the 'ipStr' element to the IP address of the peer and 'port' to the
  IP port.  Set 'isRx' to true.
- Additionally set 'channels' and 'wordSizeBytes' as appropriate for the
  stream.
- Call openRtpStream().
- Upon success, repeatedly call rtpReadSamplesAvailable() to poll for
  received frames.  Samples are available when the function returns a
  non-zero value.  Consume some number of samples, and report the number
  of samples consumed by calling rtpReadSamples(). Repeat as necessary.
- Call closeRtpStream() when finished.

## Writing RTP streams

- Create a new RTP_STREAM object and zero it out
- Set the 'ipStr' element to the IP address of the peer and 'port' to the
  IP port.  Set 'isRx' to false.
- Additionally set 'channels' and 'wordSizeBytes' as appropriate for the
  stream.
- Call openRtpStream().
- Upon success, repeatedly call rtpWriteSamplesAvailable().  Write up to the
  number of samples available for writing to the pointer returned in 'data'.
  Report the number of samples written by calling rtpWriteSamples(). Repeat
  as necessary.
- Call closeRtpStream() when finished.

## Info

- Only 16 and 32-bit word sizes are supported.
- RTP IP ports normally start at 6970

