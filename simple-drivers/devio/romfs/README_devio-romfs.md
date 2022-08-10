# simple-drivers/devio/romfs

## Overview
The devio-romfs driver provides a bridge between the the Analog Devices Device I/O layer and the romfs allowing for stdio.h file I/O to the romfs.

## Required components

- romfs

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

- None

## Run

- Call `romfs_devio_init()` after the romfs has been initialized.

## Info

- None
