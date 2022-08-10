# 3rd-party/CLD-USB-Host-Mass-Storage-SC5xx

## Overview

This component is contains the CLD USB Mass Storage device driver
library.

## Required components

- None

## Recommended components

- msd_simple driver
- msd-host service

## Integrate the source

- Copy the 'inc' directory contents into a project include directory.
- Copy the 'lib' directory contents into a project static library directory

## Configure

- Refer to the CLD documentation for other configurable options.

## Run

- This library is meant to be run through the msd-host simple service and
  msd_simple device drivers.  See those components for further integration
  details.
