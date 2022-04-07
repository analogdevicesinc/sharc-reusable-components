# SAM-Audio-Starter

## Overview
The project is tailored for quick audio startup with the SAM board.

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
make -j4
```

## Debugging the code
- Open CCES, create a new debug configuration
- Load `<CCES_INSTALL_DIR>\SHARC\ldr\ezkitSC589_preload_core0_v01` into core0
- Additionally, load the `SAM-Audio-Starter-ARM.exe` executable into core0
- Load the `SAM-Audio-Starter-SHARC0.dxe` executable into core1
- Load the `SAM-Audio-Starter-SHARC1.dxe` executable into core2
- Under the "Automatic Breakpoint" tab, be sure to uncheck the "Enable
  semihosting" checkbox located at the bottom of the tab window.
- Save and start debugging.
