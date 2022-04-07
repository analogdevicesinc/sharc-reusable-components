# Windows Device driver
Make sure you have the CLD CDC .inf file installed prior to flashing
and booting the SAM board so the driver is installed properly.

# Folders

## bootloader0:
Contains a very small L2 only bootloader which boots either bootloader1
or the application depending on the pushbutton state.

## bootloader1:
Contains an application that allows reflash over USB

## SAM-flasher:
Contains a Windows/Linux command-line application for reflashing
the SAM board over USB.

## SAM-flasher-GUI:
Contains a Windows/Linux GUI application for reflashing
the SAM board over USB.

## prebuilt:
Contains pre-built binaries for all of the above.

# Flash layout
The flash on the SAM board is organized now as:

  - bootloader0 :  0x000000 - 0x00FFFF  (64k)
  - bootloader1 :  0x010000 - 0x03FFFF  (192k)
  - Application :  0x040000+ (2M)

# To Build:
Follow the individual instructions found in each folder

# To Flash bootloader on target:

## bootloader0 / bootloader1
  - Go into prebuilt
  - Edit setenv.bat as necessary to point to your CCES install and run it.
    If cldp is already in your path you can omit this step.
  - Run boot0_hw_rev_1_4_cldp.bat if your HW revision is 1.4 or less or
    boot0_hw_rev_1_5_cldp.bat if your HW revision is 1.5 or greater.
  - Run boot1_hw_rev_1_4_cldp.bat if your HW revision is 1.4 or less or
    boot1_hw_rev_1_5_cldp.bat if your HW revision is 1.5 or greater.

Reset your board.  The bootloader0 will rapid blink LED10 twice to show
it's alive then take the following action based on the pushbutton state:

  - PB1 and PB2 pressed, launch bootloader1 (to reflash over USB)
  - PB1 only pressed, stay in a "safe" state in bootloader0 so cldp runs properly
  - No pushbutton, launch application

## Flasher GUI:
  - Go into prebuilt
  - Run the setup exec

## Flasher Command Line:
  - Go into prebuilt
  - Run from here or copy and run from anywhere convenient

# Linux permissions

By default normal users do not have permissions to access USB serial
ports.  To resolve a `ser_open:unable to open port:permission denied
` error, either run the application using the `sudo` command or
install an appropriate `udev` rule.

The `udev` approach is better.  Once the `udev` rule is installed,
there's no need to restart the machine.  The permissions change will
take effect the next time the SAM board is plugged in or put in
boot-loader mode and will persist until the rule is removed.

Examples for both are shown below:

## sudo
```
sudo bin/sam-flasher
```

## udev
```
echo 'KERNEL=="ttyACM0", MODE="0666"' | sudo tee /etc/udev/rules.d/99-SAM-Flasher.rules > /dev/null
```

# 3rd party license
The GUI toolkit is wxWidgets 3.0 and the graphical form was generated
with wxFormBuilder.  The installer was created with InnoSetup.  All of
these tools are free and unencumbered for commercial use.

See the NOTICE file in each subdirectory for specific OSS component
licenses.  

ADI source code licensed under an Apache 2.0 license.
