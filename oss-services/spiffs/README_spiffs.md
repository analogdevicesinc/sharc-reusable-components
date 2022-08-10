# oss-services/spiffs

## Overview

This component contains an unmodified version of the SPIFFS filesystem
with a custom configuration file and application shim layer.

## Origin

https://github.com/pellepl/spiffs

## Required components

- spi-simple
- flash-simple compatible driver

## Recommended components

- fs-dev
- umm_malloc

## Integrate the source

- Copy the contents of the `src` and `app` directory to a project source
  directory.
- Copy the contents of the `inc` directory to a project include directory
  and edit `spiffs_config.h` and `spiffs_fs_cfg.h` as needed for your
  application.

## Example usage

```
#include "spiffs.h"
#include "spiffs_fs.h"

#include "fs_devman.h"
#include "fs_dev_spiffs.h"

#define SPIFFS_VOL_NAME  "sf:"

spiffs *spiffsHandle;
FLASH_INFO *flashHandle;
FS_DEVMAN_DEVICE *device;
FS_DEVMAN_RESULT fsdResult;

/* Initialize flash device */
flashHandle = INIT_FLASH();

/* Initialize the SPIFFS filesystem */
spiffsHandle = umm_calloc(1, sizeof(*spiffsHandle));
spiffsResult = spiffs_mount(spiffsHandle, flashHandle);
if (spiffsResult == SPIFFS_OK) {
    device = fs_dev_spiffs_device();
    fsdResult = fs_devman_register(SPIFFS_VOL_NAME, device, spiffsHandle);
} else {
    syslog_print("SPIFFS mount error, reformat via command line\n");
}
```

