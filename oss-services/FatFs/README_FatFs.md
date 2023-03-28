# oss-services/FatFs

## Overview

This package is a recent version of the popular [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) FAT filesystem package.

## Required components

- None

## Recommended components

- SD card components
- USB Mass Storage components
- FreeRTOS
- umm-malloc

## Integrate the source

- Copy the `FatFs` directory into an appropriate place in the host project

- Overwrite the FatFS `diskio.c` with `src/diskio.c` for a diskio layer compatible with the SD card and USB Mass Storage reusable components.  Copy `inc/fatfs_diskio_cfg.h` into a project include directory and modify as appropriate for the system.

- The application must implement the following functions to return a simple device handle for the SD card or USB Mass Storage device if enabled:

    ```
    sSDCARD *sdcardGetHandle(void);
    sMSD *msdGetHandle(void);
    ```

- These function prototypes must be made available through `sdcard.h` and `msd.h` header files (see `diskio.c`).

- Overwrite the FatFS `ffsystem.c` with `src/ffsystem.c` for a system layer compatible with FreeRTOS that also uses umm_malloc as the system heap.

## Configure

- Overwrite the FatFs `ffconf.h` file with `src/ffconf.h` for a FreeRTOS compatible configuration and modify as appropriate or simply take inspiration.
