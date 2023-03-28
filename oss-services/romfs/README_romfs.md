# oss-services/romfs

## Overview

The `romfs` component provides a 'write once' flash filesystem.  Once a file is written to the filesystem it continues to take up space, even if deleted or overwritten, until the filesystem is re-formattted.

Much of this code came from the [eLua](http://www.eluaproject.net/) project.  SAM specific enhancements include removal of eLua specific components and a moderate restructuring and clean-up of the code.

## Required components

- flash device driver

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the romfs.

## Configure

The `romfs` has a couple of configurable parameters which should be set in `romfs_cfg.h`:

 1. `FD_OFFSET`sets the lowest file descriptor the filesystem will return for calls to `fs_open()`.  This defaults to (3) if not defined to avoid stdin, stdout, and stderr.
 2. `TOTAL_MAX_FDS` sets the maximum number of simultaneous files that can be opened at the same time.  This defaults to (8) if not defined.

A couple of additional parameters *must* be defined to configure the area of flash that the romfs filesystem will utilize.  These parameters must be set in `romfs_cfg.h`:

 1. `ROMFS_FLASH_START_ADDRESS` corresponds to the physical start of flash.  For a SPI flash this is address 0, for an on-board flash this would correspond to it's memory mapped start address.
 2. `ROMFS_FLASH_SECTOR_SIZE` corresponds to the flash sector size.  The romfs assumes the flash has a homogeneous flash sector size throughout the filesystem.  This limitation can be overcome by modifying the address/sector mapping functions in `romfs_platform.c`.
 3. `ROMFS_FS_START_ADDRESS` corresponds to the physical start of the filesystem in flash.  For any flash device, this address will be greater than, or equal to, `ROMFS_FLASH_START_ADDRESS`.
 4. `ROMFS_FS_END_ADDRESS` corresponds to the physical end of the filesystem in flash.  For any flash device, this address will be at least `ROMFS_FS_START_ADDRESS` + `ROMFS_FLASH_SECTOR_SIZE`.

It is highly recommended these parameters come from a larger project wide flash layout header file.  An example flash layout header consistent with the SAM board bootloader, `flash_map.h` has been included for reference along with `romfs_cfg.h`.  It should be modified as necessary when integrated.

## Run

- Call `romfs_init()` with a fully configured `FLASH_INFO` handle from the `flash` component.
- Call any of the Public Filesystem Access Functions available at the bottom of `romfs_devman.h`

## Examples

An example initialization:

```C
/* Simple driver includes */
#include "spi_simple.h"

/* OSS service includes */
#include "romfs.h"

sSPI *spi2Handle;
sSPIPeriph *spiFlashHandle;
FLASH_INFO *flashHandle;
SPI_SIMPLE_RESULT spiResult;

/* Initialize and configure the simple SPI driver for the flash */
spiResult = spi_init();

/* Initialize the SPI and attached flash */
spiResult = spi_open(SPI2, &spi2Handle);
spiResult = spi_openDevice(spi2Handle, &spiFlashHandle);
spiResult = spi_setClock(spiFlashHandle, 9);
spiResult = spi_setMode(spiFlashHandle, SPI_MODE_3);
spiResult = spi_setFastMode(spiFlashHandle, true);
spiResult = spi_setLsbFirst(spiFlashHandle, false);
spiResult = spi_setSlaveSelect(spiFlashHandle, SPI_SSEL_1);

/* Open the flash driver with the configured SPI device handle */
flashHandle = mt25q_open(spiFlashHandle);

/* Initialize the filesystem */
romfs_init(flashHandle);
```

Example `open / close / write / read` commands:

```C
#include <fcntl.h>
#include "romfs_devman.h"

int fd;
fd = fs_open("test", O_WRONLY | O_CREAT);
fs_write(fd, "abc123", 6);
fs_close(fd);
char buf[6];
fd = fs_open("test", O_RDONLY);
fs_read(fd, buf, 6);
fs_close(fd);
```

An example `ls / dir` command:

```C
#include "romfs_devman.h"

static u32 shell_ls_helper( const char *crtname, int recursive, int *phasdirs )
{
  DM_DIR *d;
  u32 total = 0;
  struct dm_dirent *ent;
  unsigned i;
  char *fullname;
  int ndirs = 0;

  if( ( d = dm_opendir( crtname ) ) != NULL )
  {
    total = 0;
    printf( "%s", crtname );
    while( ( ent = dm_readdir( d ) ) != NULL )
    {
      printf( "\n%s", ent->fname );
      for( i = strlen( ent->fname ); i <= DM_MAX_FNAME_LENGTH; i++ )
        printf( " " );
      if( ent->flags & DM_DIRENT_FLAG_DIR )
      {
        printf( "<DIR>" );
        ndirs = ndirs + 1;
        if( phasdirs )
          *phasdirs = 1;
      }
      else
      {
        printf( "%u bytes", ( unsigned )ent->fsize );
        total = total + ent->fsize;
      }
    }
    dm_closedir( d );
    printf( "\nTotal on %s: %u bytes\n", crtname, ( unsigned )total );
  }
}

void romfs_ls(int argc, char **argv)
{
    int phasdirs;
    shell_ls_helper("/wo", 0, &phasdirs );
}
```

An example `format` command:

```C
#include "romfs.h"

void shell_format(int argc, char **argv)
{
    romfs_format();
}
```

An example `df` (disk full) command:

```C
void shell_df( int argc, char **argv )
{
   u32 size, used;
   u32 err;

   printf("%-10s %10s %10s %10s %5s\n", "Filesystem", "Size", "Used", "Available", "Use %");
   err = romfs_full(&size, &used);
   if (err > 0) {
      printf("%-10s %10u %10u %10u %5u\n", "/wo", size, used, size - used, (100 * used) / size);
   }
}
```

## Info

-  Once all available space has been written, the filesystem must be fully formatted to write additional data.
