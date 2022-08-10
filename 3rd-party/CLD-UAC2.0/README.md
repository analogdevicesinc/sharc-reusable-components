# 3rd-party/CLD-UAC2.0

## Overview

This component is a de-integrated version of the proprietary UAC2.0
driver and example application package from Closed Loop Design.
It has been modified to operate in both FreeRTOS and main-loop
applications.

## Required components

- None

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.
- Copy the 'lib' directory into a project static library directory

## Configure

Refer to the CLD documentation for configurable options.

## Run

- Call `cld_uac20_init()` once during system initialization
- Call `cld_uac20_run()` regularly approximately every 10mS during normal operation
