# simple-services/usb-timer

## Overview

This component implements a simple service that starts a 125uS timer for servicing USB module callbacks.

## Required components

- None

## Recommended components

- CLD USB device/host libraries and related services

## Integrate the source

- Copy the 'inc' directory contents into a project include directory.
- Copy the 'src' directory contents into a project static library directory

## Configure

- Edit `usb_timer_cfg.h` as necessary with the appropriate SCLK0 frequency.

## Run

- Initialize by calling `usb_timer_start()`.  Provide which SC5xx General-Purpose
  Timer to use.
- Register callbacks by calling usb_timer_register()
