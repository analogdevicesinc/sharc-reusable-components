# simple-services/fs-dev

## Overview

This service a filesystem device manager capable of dispatching file and
directory requests to a run-time configurable set of underlying filesystem
drivers.  Plugins for FatFs and WOFS managed devices are included.

## Required components

- None

## Recommended components

- FatFs with SD card and USB Mass Storage components
- oss-services/romfs

## Integrate the source

- Copy the 'inc' directory contents into a project include directory.
- Copy the 'src' directory contents into a project static library directory

## Configure

- Edit `fs_devman_cfg.h` if necessary

## Run

- Initialize the component by calling `fs_devman_init()`
- Initialize the device I/O layer by calling `fs_devio_init()`
- Register filesystem volumes by calling `fs_devman_register()`
- Optionally set a default volume by calling `fs_devman_set_default()`

## Example code

Initializing
```
/* Intialize the filesystem device manager */
fs_devman_init();

/* Intialize the filesystem device I/O layer */
fs_devio_init();
```

Registering filesystem volumes with the device manager
```
FS_DEVMAN_DEVICE *device;
FS_DEVMAN_RESULT fsdResult;

/* Hook the internal filesystem into the stdio libraries */
device = fs_dev_romfs_device();
fsdResult = fs_devman_register("wo:", device, NULL);

/* Hook the Mass Storage filesystem into the stdio libraries */
device = fs_dev_fatfs_device();
fsdResult = fs_devman_register("usb:", device, NULL);

/* Hook the SD card filesystem into the stdio libraries */
device = fs_dev_fatfs_device();
fsdResult = fs_devman_register("sd:", device, NULL);
```

## Info

- To access files on a registered volume append the prefix to the filename or
  directory name.  I.e. `f = fopen("sd:/subdir/file.txt", "r");`.  The
  default device will be used if set and no prefix is given.

- When using FatFs, keep the names of the of the volumes consistent between
  this component and FatFs.  Utilize the `FF_VOLUME_STRS` option for this or
  register the filesystem volumes with '0:', '1:', etc..  See the sdcard_simple
  or msd_simple device drivers for example code.
