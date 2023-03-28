# simple-services/a2b-xml

## Overview

The a2b-xml service converts a SigmaStudio network configuration exported in XML format into a memory buffer of the same binary format found when compiling 'adi_a2b_i2c_commandlist.h'.  The output of this service is compatible for use with the `ad2425` simple-driver for A2B network discovery and configuration.

## Required components

- yxml
- devio-romfs (or some other stdio filesystem implementation)
- romfs (or some other filesystem)

## Recommended components

- umm_malloc
- syslog
- a2b-ad2425

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the a2b-xml service.

## Configure

The a2b-xml service has a number of convenient compile-time configuration options.  See 'inc/a2b_xml_cfg.h' for an example utilizing 'umm_malloc' and 'syslog'.

## Run

- Call `a2b_xml_load()` with the name of the XML file and a pointer to a 'void *' to receive a pointer to the configuration binary.
- Pass this binary, along with the returned length, to the 'ad2425' driver for A2B network discovery.
- Call `a2b_xml_free()` to free the binary from memory.

## Info
- This module makes aggressive use of 'realloc()' so be aware of potential memory fragmentation.
- Binary blobs must be freed through 'a2b_xml_free()' due to the fact that all parsed config entries have nested data buffers allocated within.
