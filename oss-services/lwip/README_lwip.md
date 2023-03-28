# oss-services/lwip

## Overview

This package is a recent version of the popular [lwIP](https://savannah.nongnu.org/projects/lwip/) networking package.

## Required components

- adi-drivers/ethernet

## Recommended components

- simple-drivers/lwip/lwip-adi-ether-netif
- oss-services/lwip-arch-FreeRTOS

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy either 'lwipopts_nosys.h' or 'lwipopts_sys.h' as 'lwipopts.h' into a project include directory.  The header file 'lwipopts_nosys.h' contain a minimally configured set of options to enable transmission and reception of UDP datagrams with no operating system.  The header file 'lwipopts_sys.h' contains a more comprehensive configuration supporting FreeRTOS and lwIP's threaded APIs.

## Configure

lwIP has many configurable options.  Refer to the on-line lwIP documentation for additional details.
