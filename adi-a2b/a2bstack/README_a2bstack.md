# adi-a2b/a2bstack

This directory contains the Analog Devices A2B Stack repackaged as a reusable component.  This version of the A2B Stack also aims to achieve compatability with both Analog Devices Sigma Studio and Mentor Graphics Network Configurator.

The A2B Stack is a relatively large package containing many configurable options along with extensive system integration flexibility.  Detailed integration and A2B Stack configuration instructions can are outside of the scope of this README.  Please contact Analog Devices technical support for additional information.

At a minimum, a project must include the `a2bstack`, `a2bplugin-master`, and a single `a2bapp-pal-*` (Platform Abstraction Layer).  Application writers are highly encouraged to also include `a2bapp-helpers`.

In many cases, especially with a C99+ compliant tool-chain, most directories can simply be copied directly into a project for building and integration.

## Include directories

The contents of the top level `inc` directory should be copied to an appropriate project include directory.  Be sure to maintain the `a2b` path in the destination include directory.

These files should be modified as required to accurately reflect the system configuration.  In particular, `conf.h` and `features.h` should be reviewed.

## Source directories

Below is a brief summary of the directories included in the top level `src` directory of this reusable component.

### a2bapp-helpers

This directory contains a higher level application abstraction layer add-on to the A2B Stack.  Application writers are encouraged to interact exclusively with this layer for routine discovery and diagnostic processing.

When integrating this component, copy the contents of the `inc` directory into a project include directory.  The `inc`directory contains headers with tunable parameters.  Copy the contents of the `src` directory into any appropriate location in the host project.

### a2bapp-pal-SAM-minimal

This directory contains an example of a minimal PAL configured for the SC589 SAM board with a fully integrated Simple TWI driver and an ADI system services timer.

When integrating this component, Copy the contents of the `inc` directory into a project include directory.  Copy the contents of the `src` directory into an appropriate location in the host project.

Alternatively, the directory can be copied as-is into the project and added to the project's source and include paths.

Only a single PAL should be included in a project.

### a2bapp-pal-SAM-RTOS

This directory contains an example of a minimal PAL with support for FreeRTOS.  This PAL illustrates how to pass a shared application Simple TWI device driver handle to the PAL through the PAL ECB `ecb->palEcb.usrPtr` configured for use on the SC589 SAM board.

When integrating this component, copy the `inc` directory contents into a project include directory.  Copy the contents of the `src` directory into an appropriate location in the host project.

Alternatively, the directory can be copied as-is into the project and added to the project's source and include paths.

Only a single PAL should be included in a project.

### a2bapp-pal-stubs

This directory contains an "empty" PAL for use when porting the A2B Stack to a new platform.

When integrating this component, copy the `inc` directory contents into a project include directory.  Copy the contents of the `src` directory into an appropriate location in the host project.

Alternatively, the directory can be copied as-is into the project and added to the project's source and include paths.

Only a single PAL should be included in a project.

### a2bplugin-master

This directory contains the A2B Stack Master Plugin.

When integrating this component, copy the `inc` directory contents into a project include directory.  Be sure to maintain the `a2bplugin-master` path in the destination include directory.  Other plugins, not included in this reusable component, also contain a `plugin.h` which may result in a naming conflict if merged into a single include directory.

Copy the contents of the `src` directory into any appropriate location in the host project.

Alternatively, the directory can be copied as-is into the project and added to the project's source and include paths.

### a2bplugin-periph-init

This directory contains an A2B slave plugin to perform peripheral initialization from ADI Sigma Studio BCF files.

When integrating this component, copy the contents of the `src` directory into any appropriate location in the host project.

This plugin attaches to any slave node for the purpose of peripheral initilization.  Be sure to place this plugin last in the PAL plugin list to allow other plugins the opportunity to claim the slave before this one.

### a2bstack

This directory contains the core A2B Stack components.

When integrating this component, copy the `inc` directory contents into a project include directory.  Be sure to maintain the `a2b` path in the destination include directory.

Copy the contents of the `src` directory into any appropriate location in the host project.

Alternatively, the directory can be copied as-is into the project and added to the project's source and include paths.

### a2bstack-commchannel

This directory contains a communication channel component utilizing the mailbox feature of the AD242x+ family of A2B transceivers for master / slave node communication.

*Note*: This feature requires the use of SigmaStudio and inclusion of the `a2bstack-protobuf-ADI` directory.

### a2bstack-protobuf-ADI

This directory contains helper functions to translate SigmaStudio binary network descriptions from the portable Google Protobuf format into the platform specific A2B Stack structs.

*Note*: ADI protobuf schema is not compatible with Mentor.  Please only include use this directory when using SigmaStudio.

### a2bstack-protobuf-Mentor

This directory contains helper functions to translate Mentor Graphics'  binary network descriptions from the portable Google Protobuf format into the platform specific A2B Stack structs.

*Note*: Mentor protobuf schema is not compatible with SigmaStudio.  Please only include use this directory when using Mentor's Network Configurator.

## Required components

- None

## Recommended components

- twi-simple

## Example directory layout and include paths

The examples below illustrate a 'cut-n-paste' integrated Stack with include paths, source paths and directory layout.

### Include paths
```
include
src/a2bstack/a2bapp-helpers
src/a2bstack/a2bstack/inc
src/a2bstack/a2bplugin-master/inc
src/a2bstack/a2bapp-pal-SAM-RTOS/inc
src/a2bstack/a2bstack-protobuf-Mentor/inc
```

### Source paths

```
src/a2bstack/a2bstack/src
src/a2bstack/a2bapp-helpers
src/a2bstack/a2bplugin-master/src
src/a2bstack/a2bstack-protobuf-Mentor/src
src/a2bstack/a2bapp-pal-SAM-RTOS/src
```

### Directory layout

```
include/a2bapp_helpers_cfg.h

include/a2b:
 conf.h
 ctypes.h
 features.h
 palecb.h

src/a2bstack/a2bapp-helpers:
 a2bapp_helpers.c
 a2bapp_helpers.h

src/a2bstack/a2bapp-pal-SAM-RTOS/inc:
 a2bapp_pal.h

src/a2bstack/a2bapp-pal-SAM-RTOS/src:
 a2bapp_pal.c

src/a2bstack/a2bplugin-master/inc/a2bplugin-master:
 plugin.h

src/a2bstack/a2bplugin-master/src:
 a2b_bert.c
 a2bmaster_plugin.c
 a2bmaster_verinfo.c
 discovery.c
 discovery.h
 override.c
 override.h
 periphcfg.c
 periphcfg.h
 periphutil.c
 periphutil.h
 plugin_priv.h
 pwrdiag.c
 pwrdiag.h
 verinfo.h

src/a2bstack/a2bstack/inc/a2b:
  a2b.h
  audio.h
  defs.h
  diag.h
  ecb.h
  error.h
  gpio.h
  i2c.h
  interrupt.h
  macros.h
  msg.h
  msgrtr.h
  msgtypes.h
  pal.h
  pluginapi.h
  regdefs.h
  seqchart.h
  seqchartctl.h
  stack.h
  stackctxmailbox.h
  stackversion.h
  stringbuffer.h
  system.h
  timer.h
  trace.h
  tracectl.h
  util.h

src/a2bstack/a2bstack/src:
  audio.c
  diag.c
  gpio.c
  i2c.c
  i2c_priv.h
  interrupt.c
  interrupt_priv.h
  job.h
  jobexec.c
  jobexec.h
  memmgr.c
  memmgr.h
  msg.c
  msg_priv.h
  msgrtr.c
  msgrtr_priv.h
  pool.c
  pool.h
  queue.h
  seqchart.c
  seqchart_priv.h
  stack.c
  stack_priv.h
  stackctx.c
  stackctx.h
  stackctxmailbox.c
  stackinfo.c
  stringbuffer.c
  system.c
  timer.c
  timer_priv.h
  trace.c
  trace_priv.h
  util.c
  utilmacros.h

src/a2bstack/a2bstack-protobuf-Mentor/inc:
  a2b_bdd_helper.h
  bdd_pb2.pb.h
  pb.h
  pb_common.h
  pb_decode.h
  pb_syshdr.h

src/a2bstack/a2bstack-protobuf-Mentor/schema:
  bdd.options
  bdd.proto

src/a2bstack/a2bstack-protobuf-Mentor/src:
  a2b_bdd_helper.c
  bdd_pb2.pb.c
  pb_common.c
  pb_decode.c
```

## Example A2B network bring-up function using the A2B App Helper functions

```
#include "a2bapp_helpers.h"

#ifdef ADI_SIGMASTUDIO_BCF
// Sigma Studio BCF Bus Configuration File
#include "adi_a2b_busconfig.h"
#else
// Mentor BDD Bus Description Data
#include "2x_2425WBZ_s2s.h"
#include "2x_2425WBZ_s2s_eeprom.h"
#endif

a2b_helperContext *a2b;

int main(int argc, char **argv)
{
    /* A2B Stack Init */
    a2b = umm_calloc(1, sizeof(*a2b));
    a2b_stack_init(a2b);
    a2b->twiHandle = <TWI Simple device handle>;
}

void a2b(void)
{
    const char *failReason;
    int i;
#ifdef ADI_SIGMASTUDIO_BCF
    ADI_A2B_NODE_PERICONFIG aPeriDownloadTable[A2B_CONF_MAX_NUM_SLAVE_NODES];
#endif

    /* Set the BDD to load */
#ifdef ADI_SIGMASTUDIO_BCF
    // Sigma Studio Bus Configuration File (BCF)
    memset(aPeriDownloadTable, 0, sizeof(aPeriDownloadTable));
    a2b->sBusDescription = &sBusDescription;
    adi_a2b_ParsePeriCfgTable(a2b->sBusDescription, aPeriDownloadTable, 0);
    a2b->periphPkg = (const a2b_Byte *)aPeriDownloadTable;
#else
    // Mentor Bus Description Data (BDD)
    a2b->network = gA2bNetwork;
    a2b->networkLen = gA2bNetworkLen;
    a2b->periphPkg = gA2bNetworkPeripPkg;
    a2b->periphPkgLen = gA2bNetworkPeripPkgLen;
#endif

    /* Load the network */
    a2b_stack_load(a2b);

    /* Start the Stack */
    a2b_stack_start(a2b);

    /* Discover the network */
    a2b_stack_discover(a2b);

    /* Display success */
    if (a2b->discoverySuccessful == A2B_TRUE) {
        printf("Success: Discovered %d nodes.\n", a2b->nodesDiscovered);
    } else {
        /* Tick the stack a few times for diagnostics */
        for (i = 0; i < 10; i++) {
            a2b_stack_tick(a2b);
        }
        /* Report the line diagnostic results */
        if (a2b->faultSuccessful == A2B_TRUE) {
            failReason = a2b->faultStatus;
        } else {
            failReason = "Unknown";
        }
        printf("Failed: Node %d (%s)\n", a2b->faultNode, failReason);
    }

    /* Stop the network */
    a2b_stack_stop(a2b);
}
```
