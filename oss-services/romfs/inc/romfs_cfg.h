#ifndef _romfs_cfg_h
#define _romfs_cfg_h

#include "flash_map.h"

#define ROMFS_FLASH_START_ADDRESS (FLASH_ADDR)
#define ROMFS_FLASH_SECTOR_SIZE   (4 * 1024)

#define ROMFS_FS_START_ADDRESS    (FS_OFFSET)
#define ROMFS_FS_END_ADDRESS      (FS_OFFSET + FS_SIZE)

#endif

