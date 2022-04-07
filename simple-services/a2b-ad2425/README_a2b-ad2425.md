# simple-services/a2b-ad2425

## Overview

The a2b-ad2425 service discovers an A2B network using a SigmaStudio A2B exported network configuration.

NOTE: This module has been depricated in favor of the 'adi-a2b-cmdlist' component.  Users of SigmaStudio A2B plugin version 19.9.0 or higher must use the the new component.

## Required components

- twi-simple driver

## Recommended components

- a2b-xml
- umm_malloc

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the a2b-ad2425 service.

## Configure

The a2b-ad2425 service has a number of convenient compile-time configuration options.  See 'inc/a2b_ad2425_cfg.h' for an example utilizing 'umm_malloc'.

## Run

- See the example below

## Example

```C
#define AD2425_I2C_ADDR  (0x68)

sTWI *ad2425TwiHandle;
void *a2bInitSequence;
uint32_t a2bIinitLength;
BM_AD2425W_RESULT a2bResult;
BM_AD2425W_CONTROLLER ad2425w;
const char *a2bResultStr;

/* Initialize the simple TWI driver */
twiResult = twi_init();

/* Open up a device handle for TWI0 @ 400KHz */
twiResult = twi_open(TWI0, &ad2425TwiHandle);
twi_setSpeed(ad2425TwiHandle, TWI_SIMPLE_SPEED_400);

/* Initialize the AD2425 service */
ad2425w_initialize(&ad2425w, AD2425W_SIMPLE_MASTER,
    AD2425_I2C_ADDR, ad2425TwiHandle);

//
//  Initialize 'a2bInitSequence' and 'a2bIinitLength' appropriately
//

/* Initialize the A2B network */
a2bResult = ad2425w_load_init_sequence(&ad2425w,
    a2bInitSequence, a2bIinitLength);

/* Convert the result to a string */
a2bResultStr = ad2425w_result_str(a2bResult);

/* Print the results */
printf("A2B config lines processed: %u", ad2425w.init_line);
printf("A2B discovery result: %s", a2bResultStr);
if (a2bResult == AD2425W_SIMPLE_SUCCESS) {
    printf("A2B nodes discovered: %d", ad2425w.nodes_discovered);
}
```

## Info

- This module assumes the AD2425 is in a power on reset state prior to
  calling `ad2425w_load_init_sequence()`
- It is OK to call `ad2425w_load_init_sequence()` multiple times.  Just
  be sure to reset the AD2425 by gating the SYNC pin clock for at least 1mS
  prior to each call.
