#include "romfs_platform.h"

#include "romfs_cfg.h"
#include "flash.h"

static const FLASH_INFO *flashHandle = NULL;

u32 platform_flash_write( const void *from, u32 toaddr, u32 size )
{
    int result = 0;
    if (size > 0) {
        result = flash_program(flashHandle, toaddr, from, size);
        if (result == FLASH_OK) {
            result = size;
        }
    }
    return(result);
}

u32 platform_flash_read( u32 fromaddr, void *toaddr, u32 size )
{
    int result = 0;
    if (size > 0) {
        result = flash_read(flashHandle, fromaddr, toaddr, size);
        if (result == FLASH_OK) {
            result = size;
        }
    }
    return(result);
}

u32 platform_flash_get_first_free_block_address( u32 *psect )
{
    u32 address;
    address = ROMFS_FS_START_ADDRESS;
    if (psect) {
        *psect = (ROMFS_FS_START_ADDRESS - ROMFS_FLASH_START_ADDRESS) / ROMFS_FLASH_SECTOR_SIZE;
    }
    return(address);
}

u32 platform_flash_get_sector_of_address( u32 addr )
{
    u32 sector;
    sector = (addr - ROMFS_FLASH_START_ADDRESS) / ROMFS_FLASH_SECTOR_SIZE;
    return(sector);
}

int platform_flash_erase_sector( u32 sector_id )
{
    int result;
    u32 addr;
    addr = ROMFS_FLASH_START_ADDRESS + sector_id * ROMFS_FLASH_SECTOR_SIZE;
    result = flash_erase(flashHandle, addr, ROMFS_FLASH_SECTOR_SIZE);
    return(PLATFORM_OK);
}

int platform_init(const FLASH_INFO *fi)
{
    flashHandle = fi;
    return(PLATFORM_OK);
}
