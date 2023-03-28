# simple-services

This repository contains middleware components developed for SAM board projects.

## syslog

The `syslog` directory contains a system logging component useful for logging events in real-time.

## a2b-xml

The `a2b-xml` directory contains a component capable of parsing SigmaStudio A2B network configurations exported in XML format and converting them for direct use by the `ad2425` driver.

## adi-osal-minimal

The `adi-osal-minimal` directory contains a *VERY* minimal implemntation of the ADI OSAL layer.  It's basically just FreeRTOS critical section code.

## buffer-track

The `buffer-track` directory contains a component capable of accurately tracking circular buffer fill levels even when the size of the in/out transfers are asynchronous or large compared to the overall size of the buffer itself.

## FreeRTOS-cpu-load

The `FreeRTOS-cpu-load` directory contains a basic CPU load calculation module for FreeRTOS.

## a2b-ad2425

The `a2b-ad2425` directory contains a service to configure an A2B network using an AD2425 transceiver.  WARNING: This module is depricated.  Use the 'adi-a2b-cmdlist' module instead.

## adi-a2b-cmdlist

The `adi-a2b-cmdlist` directory contains a service to configure an A2B network from a SigmaStudio command list export header.

## uac2-soundcard

The `uac2-soundcard` directory contains a simple run-time configurable UAC2 soundcard implemetation layered on top of the CLD UAC 2.0 library.

## uac2-cdc-soundcard

The `uac2-cdc-soundcard` directory contains a simple run-time configurable UAC2 soundcard implemetation layered on top of the CLD UAC 2.0 + CDC library.  This component can be used with the `uart-cdc` driver to implement a `simple-uart` compatible device driver interface over USB.

## usb-timer

The `usb-timer` directory contains a simple service that starts and manages a 125uS timer for servicing multiple USB modules from CLD.

## msd-host

The `msd-host` directory contains the application run-time functions required to coordinate the activities between the CLD Host Mass Storage library and msd_simple device driver.

## fs-dev

The `fs-dev` directory contains a filesystem device manager capable of dispatching file and directory
requests to a run-time configurable set of underlying filesystem drivers.  Plugins for FatFs and WOFS managed devices are included.

## SHARC Audio Engine

The `SHARC-audio-engine` directory contains a service that implements a
very lightweight, but highly flexible and efficient, inter-processor communication
(IPC) and audio streaming engine.

## telnet

The `telnet` directory contains a service enabling shell access over telnet using lwIP and FreeRTOS server task.

## wave-file

The `wave-file` directory contains a service for reading and writing wave files.

## rtp-stream

The `rtp-stream` directory contains a service for reading and writing Ethernet RTP audio streams.

## vban-stream

The `vban-stream` directory contains a service for reading and writing Ethernet VBAN audio streams.
