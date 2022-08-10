# simple-services/uac2-soundcard

## Overview

The `uac2-soundcard` service implements a simple, reasonably complete,
run-time configurable, UAC 2.0 soundcard implemented on top of the CLD
UAC 2.0 library.

## Required components

- 3rd-party CLD UAC 2.0 library (CLD library and header only)
- syslog
- cpu-load

## Recommended components

- umm_malloc

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header
  files in the 'inc' directory contain the configurable options for the
  uac2-soundcard service.

## Configure

The uac2-soundcard service has a number of convenient compile-time
configuration options.  See 'inc/uac2_cfg.h' and 'inc/uac20_descriptors_cfg.h'
for an example utilizing 'umm_malloc'.

Most USB configuration options are run-time configured as shown in the
example below.

## Run

- See the example below

## Example

```C
#include <stdint.h>

#include "uac2.h"

#define USB_SAMPLE_RATE             (48000)
#define USB_IN_AUDIO_CHANNELS       (16)
#define USB_OUT_AUDIO_CHANNELS      (16)
#define USB_WORD_SIZE_BITS          (24)
#define USB_VENDOR_ID               (0x064b)   /* Analog Devices Vendor ID */
#define USB_PRODUCT_ID              (0x0005)   /* USB Product ID */
#define USB_MFG_STRING              "Analog Devices, Inc."
#define USB_PRODUCT_STRING          "ADI Audio v2.0 Device"
#define USB_SERIAL_NUMBER_STRING    NULL
#define USB_TIMER                   (0)

/*
 * This callback is called whenever audio data is available from the
 * host via the UAC2 OUT endpoint.  This callback runs in an
 * ISR context.
 */
uint32_t uac2Rx(void *data, uint32_t rxSize, void *usrPtr)
{
}

/*
 * This callback is called whenever audio data is requested by the
 * host via the UAC2 IN endpoint.  This callback runs in an
 * ISR context.
 */
uint32_t uac2Tx(void *data, uint32_t maxSize, void *usrPtr)
{
}

/*
 * This callback is called whenever a sample rate update is requested
 * by the host.  This callback runs in an ISR context.
 */
uint32_t uac2RateFeedback(void *usrPtr)
{
    return(USB_SAMPLE_RATE);
}

/*
 * This callback is called whenever the IN or OUT endpoint is enabled
 * or disabled.  This callback runs in an ISR context.
 */
void uac2EndpointEnabled(UAC2_DIR dir, bool enable, void *usrPtr)
{
}

int main(int argc, char **argv)
{
    USB_AUDIO_STATS uac2stats;
    UAC2_APP_CONFIG uac2cfg;

    /* Configure UAC2 application settings */
    uac2cfg.usbSampleRate = USB_SAMPLE_RATE;
    uac2cfg.usbInChannels = USB_IN_AUDIO_CHANNELS;
    uac2cfg.usbInWordSizeBits = USB_WORD_SIZE_BITS;
    uac2cfg.usbOutChannels = USB_OUT_AUDIO_CHANNELS;
    uac2cfg.usbOutWordSizeBits = USB_WORD_SIZE_BITS;
    uac2cfg.vendorId = USB_VENDOR_ID;
    uac2cfg.productId = USB_PRODUCT_ID;
    uac2cfg.mfgString = USB_MFG_STRING;
    uac2cfg.productString = USB_PRODUCT_STRING;
    uac2cfg.serialNumString = USB_SERIAL_NUMBER_STRING;
    uac2cfg.usbOutStats = &uac2stats.rx.ep;
    uac2cfg.usbInStats = &uac2stats.tx.ep;
    uac2cfg.rxCallback = uac2Rx;
    uac2cfg.txCallback = uac2Tx;
    uac2cfg.rateFeedbackCallback = uac2RateFeedback;
    uac2cfg.endpointEnableCallback = uac2EndpointEnabled;
    uac2cfg.usrPtr = NULL;
    uac2cfg.timerNum = USB_TIMER;

    /* Initialize, configure, and start the CLD UAC20 library */
    ret = uac2_init();
    ret = uac2_config(&uac2cfg);
    ret = uac2_start();

    while (1) {
        /*
         * Run the UAC2 library.  It's OK to delay for up to 10mS
         * between calls to uac2_run().
         */
        ret = uac2_run();
    }

```

## Info

- This module requires the use of a general purpose timer.
- 'clocks.h' defines 'SCLK0' with the frequency of SCLK0 in Hz which
  normally runs at 1/4 of the processor's core clock frequency.
  This is needed to configure the 125uS UAC2 general purpose timer.
