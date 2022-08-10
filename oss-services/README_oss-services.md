# oss-services

This repository contains middleware components derived from a variety of
Open Source Software (OSS) projects and adapted as reusable components
for SAM board projects.

All of these components have BSD, MIT, or similar commercial friendly
licenses.  Confirm these licenses are compatible with your company's OSS
policy before integrating them into your project.

## crc

The `crc` directory contains a simple and fast table driven crc16 routine
borrowed from 'https://www.menie.org/georges/embedded/crc16.html'.

## dbglog

The `dbglog` directory contains a very simple header file to support
system level debugging and logging.

## FatFs

The `FatFs` directory contains an easy to integrate and recent version
of the popular FatFs FAT filesystem.

## FreeRTOSv10.2.1-ARM

The `FreeRTOSv10.2.1-ARM` directory contains a cleaned-up version of the
ADI port (version 1.3.1) of FreeRTOS for the SC589 ARM core applied to
a base FreeRTOS version 10.2.1.

## libtelnet

The `libtelnet` directory contains an telnet option negotiation library.
Original source from [libtelnet](https://github.com/seanmiddleditch/libtelnet).

## lwIP

The `lwIP` directory contains an easy to integrate and recent version
of the popular lwIP network stack.

## lwip-arch-FreeRTOS

The `lwip-arch-FreeRTOS` directory contains a FreeRTOS porting layer
for lwIP.  Use when NO_SYS==1 (the default).

## pa-pinknoise

The `pa-pinknoise` directory contains an modified extract of the
[Port Audio pink noise example]
(https://github.com/PortAudio/portaudio/blob/master/examples/paex_pink.c).

## pa-ringbuffer

The `pa-ringbuffer` directory contains an unmodified extract of the
[Port Audio lock-free ring buffer code]
(https://app.assembla.com/spaces/portaudio/git/source/master/src/common).

## printf

The `printf` directory contains a smaller printf implementation.

## romfs

The `romfs` directory contains a 'write-once' ROM filesystem borrowed
from the eLua project.

## sharc-lua

The `sharc-lua` directory builds an easy to integrate version of the Lua 5.3
scripting language built for the SHARC+.

## shell

The `shell` directory contains a basic command-line shell borrowed from
the eLua project.  This version has the ability to easily add project
specific commands.

# spiffs
The `spiffs` directory contains the SPIFFS filesystem

## tftp-server

The `tftp` directory contains an extract of the lwIP tftp server example
application with additional code illustrating how to use the tftp server
to read and write to the romfs component.

## umm_malloc

The `umm_malloc` directory contains an enhanced version of the popular
umm_malloc heap manager.

## xmodem

The `xmodem` directory contains a small xmodem file transfer module borrowed
from the eLua project.

## yxml

The `yxml` directory contains an unmodified copy of the
[yxml parser library](https://dev.yorhel.nl/yxml).
