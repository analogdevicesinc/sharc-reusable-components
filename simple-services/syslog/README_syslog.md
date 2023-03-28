# simple-services/syslog

## Overview

The syslog service provides a convenient way to log system events "live" with minimal impact on run-time performance.  Log messages are inserted into a FIFO of configurable depth.  The buffer used for logging is allocated from the heap during initialization.

The syslog service runs under both FreeRTOS and bare-metal main loop applications.  All functions are safe for multi-threaded logging under FreeRTOS.

Logged messages can be dumped to stdout at intervals convenient for the project.

## Required components

- None

## Recommended components

- umm_malloc
- dbglog
- stdio

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the syslog service.

## Configure

The syslog has a number of convenient compile-time configuration options.  Please refer to the Doxygen documentation found in the [docs](./docs/html/index.html) directory for additional detail.

## Run

- Call `syslog_init()`
- Once initialized, call `syslog_print()` to quickly log fixed-length messages or `syslog_printf()` to log messages with variable arguments and formatting.
- Call `syslog_dump()` to dump any messages accumulated from the last call to `syslog_dump()`.

## Info
- Custom heap functions can be substituted by defining the `SYSLOG_MALLOC` and `SYSLOG_FREE` macros.
