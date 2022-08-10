# simple-apps/FreeRTOS-multi-core

## Overview
The `FreeRTOS-multi-core` simple-app provides a minimal starting point for a
FreeRTOS project running on the ARM core along with independent projects
running on the SHARC+ cores.  The hardware configuration is tailored for
the SC589 based SAM board from Analog Devices.

This project demonstrates how to re-purpose the McAPI shared memory segment
for simple IPC purposes when not using the McAPI module.

## To build

### Install the tools
- Install CCES as-per Analog Devices instructions
- Install Git bash shell from here: https://git-scm.com/downloads
- Instead of Git bash, one can also install MinGW/MSYS2 from
  here: http://www.msys2.org/

### Source the environment
- Check your path first to see if it already includes the desired version of
  CCES.  In general Git bash inherits the path and MinGW/MSYS2 does not
  unless it's launched with '-use-full-path' option.

```
echo $PATH  # Check inherited path
```

- If necessary, modify the 'env.sh' shell to match your CCES installation
  directory and include the environment in your shell.

```
. ./env.sh # Note the space between the first two periods!
```

### Build the code
- Go into the build directory and type `make`

```
cd build
make
```

## Debugging the code
- Open CCES, create a new debug configuration
- Load `ezkitSC589_preload_core0_2.8.3_eg_modified` into core0
- Additionally, load the `FreeRTOS-multi-core-ARM.exe` executable into core0
- Load the `FreeRTOS-multi-core-SHARC0.dxe` executable into core1
- Load the `FreeRTOS-multi-core-SHARC1.dxe` executable into core2
- Under the "Automatic Breakpoint" tab, be sure to disable the "Enable
  semihosting" checkbox located at the bottom of the tab window.
- Save and start debugging.
