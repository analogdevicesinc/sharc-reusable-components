#ifndef _fatfs_diskio_cfg_h
#define _fatfs_diskio_cfg_h

/* Use application util_time() function instead of standard
 * c runtime time()
 */
#include "util.h"
#define FATFS_DISKIO_TIME util_time

/* Enable SD card support through sdcard_simple driver */
#define FATFS_DISKIO_ENABLE_SDCARD
#define FATFS_DISKIO_SDCARD_DEVICE 0

/* Enable USB Mass Storage support through msd_simple driver */
#define FATFS_DISKIO_ENABLE_MSD
#define FATFS_DISKIO_MSD_DEVICE    1

#endif
