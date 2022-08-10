# simple-services/SHARC-audio-engine

WARNING:  The stream management component of the SHARC Audio Engine is still under development and not recommended for use.

## Overview

The `SHARC-audio-engine` directory contains a service that implements a very lightweight, but highly flexible and efficient, multi-core inter-processor communication (IPC) and audio streaming engine.

The SAE is comprised of two parts, an efficient IPC and a stream management component.

SC5xx hardware features utilized:

  - ARM and SHARC+ "Atomic Compare and Swap" to implement multi-core shared memory hardware mutexes
  - TRU to trigger core-to-core software interrupts
  - Endian compatible L2 between ARM and SHARC+ core
  - Consistent ARM and SHARC L2 memory mapping

Software features utilized include:

  - A simple and fast (first-fit, coalescing) unified L2 heap that allows messages allocated on one core to be freed on a different core.
  - Reference counted message buffers that are automatically freed when the reference count reaches zero.
  - IRQ and thread safe API calls for both FreeRTOS and main-loop applications.
  - Zero-copy message data between cores

## Required components

- None

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the SHARC Audio Engine service.

## Configure

The SHARC Audio Engine service compile-time configuration is in 'inc/sa2_cfg.h'.

Proper allocation and configuration of the shared L2 space between the cores is very important.  Carefully examine the example `apt-sc589.c` in the `examples` directory for an example cache configuration for the SC589 and SC573.  The relevant section is within an `#define(SAE_IPC)`.  If using this file directly, be sure to include `-DSAE_IPC` on the compiler's command line.

## API

See the API documentation in the `docs` directory.

## Example

One core must be delegated the "master" core and perform system startup initialization.  After initialization, all cores are considered peers.  The "master" core must complete initialization prior to any other core running the initialization routine.

### Initialize

Example master initialization on the ARM core

```C
SAE_CONTEXT *saeContext;

static void ipcMsgHandler(SAE_CONTEXT *context, SAE_MSG_BUFFER *buffer, void *usrPtr)
{
    SAE_RESULT result;
    result = sae_unRefMsgBuffer(context, buffer);
}

void SAE_Init(void)
{
    /* Init the SHARC Audio Engine.  This core is configured to be the
     * IPC master so this function must run to completion before any
     * other core calls sae_initialize().
     */
    sae_initialize(&saeContext, SAE_CORE_IDX_0, true);

    /* Register an IPC message callback */
    sae_registerMsgReceivedCallback(saeContext, ipcMsgHandler, NULL);

    /* OK to initialize the SAE on the other cores now */
}
```

Example non-master initialization on a SHARC+ core

```C
SAE_CONTEXT *saeContext;

static void ipcMsgRx(SAE_CONTEXT *saeContext, SAE_MSG_BUFFER *buffer, void *usrPtr)
{
    SAE_RESULT result;
    result = sae_unRefMsgBuffer(saeContext, buffer);
}

void SAE_Init(void)
{
    /* Initialize the SHARC Audio Engine */
    sae_initialize(&saeContext, SAE_CORE_IDX_1, false);

    /* Register an IPC message Rx callback */
    sae_registerMsgReceivedCallback(saeContext, ipcMsgRx, NULL);
}
```

### Allocate and send a message

Example of the ARM core sending a zero-length ping to both SHARC cores

```C
SAE_CONTEXT *saeContext;
SAE_MSG_BUFFER *ipcBuffer;
SAE_RESULT result;

/*
 * Ping both SHARCs with the same empty message
 */

/* Allocate a new zero length message.  The message is initialized with
 * a reference count of 1.
 */
ipcBuffer = sae_createMsgBuffer(saeContext, 0, NULL);
if (ipcBuffer) {

    /* Add a reference for the second SHARC before sending it to the
     * first SHARC to keep the first SHARC from freeing the message in
     * case it's faster than the ARM.
     */
    sae_refMsgBuffer(saeContext, ipcBuffer);

    /* Send it to the first SHARC */
    result = sae_sendMsgBuffer(saeContext, ipcBuffer, SAE_CORE_IDX_1, true);

    /* If it failed, decrement the reference count */
    if (result != SAE_RESULT_OK) {
        sae_unRefMsgBuffer(saeContext, ipcBuffer);
    }

    /* Send it to the second SHARC */
    result = sae_sendMsgBuffer(saeContext, ipcBuffer, SAE_CORE_IDX_2, true);

    /* If it failed, decrement the reference count.  At this point, the
     * message is guaranteed to have been free'd by one of the cores even
     * if some of the SHARC cores have been halted for debugging and their
     * queues are full.
     */
    if (result != SAE_RESULT_OK) {
        sae_unRefMsgBuffer(saeContext, ipcBuffer);
    }
}
```

## Info

- The SHARC+ must be compiled with the `-char-size-8` option to insure endian and alignment compatibility with the ARM core.

- See the `docs` folder for API documentation.
