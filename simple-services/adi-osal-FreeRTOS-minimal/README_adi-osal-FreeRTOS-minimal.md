# oss-services/adi-osal-FreeRTOS-minimal

## Overview

The `adi-osal-FreeRTOS-minimal` directory contains a *VERY* minimal implemntation of the ADI OSAL layer.  It's basically just a FreeRTOS critical section and semaphore implementation.

## Required components

- FreeRTOS

## Recommended components

- None

## Integrate the source

- Copy the contents of the 'src' directory into the project.

## Configure

- None

## Notes

- Only implemented for SC5xx ARM cores
- WARNING: The TLS support in this module likely conflicts with the LwIP 'errno' handling.
