# To build

## Install the tools

- Install CCES as-per Analog Devices instructions
- Install Git bash shell from here: https://git-scm.com/downloads
- Instead of Git bash, one can also install MinGW/MSYS2 from
  here: http://www.msys2.org/

## Source the environment

- Check your path first to see if the path to the desired version of
  CCES is already in place.  In general Git bash inherits the path and
  MinGW/MSYS2 does not unless it's launched with '-use-full-path' option.

```
echo $PATH  # Check inherited path
```

- If necessary, modify the 'env.sh' shell to match your CCES installation
  directory and include the environment in your shell.

```
. ./env.sh # Note the space between the first two periods!
```

## Build the code

- Go into the build directory and type make

```
cd build
make
```

## Debugging the code

- Open CCES, create a new debug configuration
- Load the executable into core0
- Leave core1 and core2 empty
- Under the "Automatic Breakpoint" tab, be sure to disable the "Enable
  semihosting" checkbox located at the bottom of the tab window.
- Save and start debugging.

## Programming the stage 0 bootloader

- Go into the build directory
- If necessary, modify the PATH in 'setenv.bat' to match your CCES installation
  directory and run the script in a DOS shell to insure the 'cldp' utility is
  in the path.
- If the SAM hardware version is less than 1.5, run boot0_hw_rev_1_4_cldp.bat
  to program the stage 0 bootloader.  If the SAM hardware version is
  1.5 or greater, run 'boot0_hw_rev_1_5_cldp.bat'.
- Proceed to programming the stage 1 bootloader

## Bootloader operation

- Under normal conditions, the stage 0 bootloader flashes LED10 twice upon
  startup.
- If no push buttons are pressed, the stage 0 bootloader will
  attempt to boot the application.
- If PB1 and PB2 are both pressed, the stage 0 bootloader will load the
  stage 1 bootloader to allow reprogramming of the application.
- If either PB1 or BP2 are individually pressed, the stage 0 bootloader
  will enter a "safe place" for 'cldp' progamming and continuously
  rapid-flash LED10.
- Press the reset button, or power cycle the SAM board to exit the
  bootloader.
