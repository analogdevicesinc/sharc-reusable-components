# adi-drivers/ethernet

## Overview

This component adds SAM support to the ADI EMAC and DP83867 PHY driver.

## Required components

- None

## Recommended components

- lwip-adi-ether-netif
- lwIP

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

This implementation adds support for the SAM Evaluation board in addition
to the existing supported ADI eval boards.

A custom 'gemac_config.h' file needs to be included in the application
for any EV-SOMCRR-EZKIT based platforms.

An example is included for the SC594 SOM.  Rename this file to
'gemac_config.h' and include it in your application.

## Run

- None

## Info

- See the `lwip-adi-ether-netif` repo for more detailed lwIP integration
  information.
