# oss-services/lwip-arch-FreeRTOS

## Overview

The `lwip-arch-FreeRTOS` directory contains a FreeRTOS porting layer
for lwIP.  Use when NO_SYS==1 (the default).

## Required components

- oss-services/lwip

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into the 'lwip/arch' directory replacing any existing files of the same name.
- Insure the lwIP option NO_SYS is defined as '0'

## Configure

- None

## Notes

- WARNING: The TLS support for 'errno' likely conflicts with the TLS support for the ADI FreeRTOS OSAL layer.
