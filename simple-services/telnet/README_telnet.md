# simple-services/a2b-xml

## Overview

The telnet service enabling shell access over telnet using lwIP and FreeRTOS server task.

## Required components

- libtelnet
- shell
- FreeRTOS
- lwIP

## Recommended components

- umm_malloc

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header file in the 'inc' directory contain the configurable options for the telnet example.

## Configure

See 'inc/telnet_cfg.h' for an example utilizing 'umm_malloc' along with the other configurable parameters.

## Run

- Call telnet_start() to start the telnet server task.

## Info

- Shell resources are freed upon execution of the `exit` command or when a socket error occurs.
