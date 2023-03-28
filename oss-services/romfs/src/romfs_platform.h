#ifndef _romfs_platform_h
#define _romfs_platform_h

#if defined(__ADSPARM__)
   #include <reent.h>
   #include <sys/types.h>
   #include <unistd.h>
   #include <fcntl.h>
#else
   #include "local_fcntl.h"
   #include "local_errno.h"
   struct _reent
   {
     int _errno;
   };
   #define _ssize_t long int
   #define _off_t   _ssize_t
   #ifndef off_t
   #define off_t    _ssize_t
   #endif
#endif

#define strncasecmp(a,b,c) strcmp(a,b)

#include "flash.h"
#include "romfs_type.h"

// Error / status codes
enum
{
  PLATFORM_ERR,
  PLATFORM_OK,
  PLATFORM_UNDERFLOW = -1
};

// Platform initialization
int platform_init(const FLASH_INFO *fi);

// *****************************************************************************
// Internal flash erase/write functions
// Currently used by WOFS

u32 platform_flash_get_first_free_block_address( u32 *psect );
u32 platform_flash_get_sector_of_address( u32 addr );
u32 platform_flash_write( const void *from, u32 toaddr, u32 size );
u32 platform_s_flash_write( const void *from, u32 toaddr, u32 size );
u32 platform_flash_get_num_sectors(void);
int platform_flash_erase_sector( u32 sector_id );
u32 platform_flash_read( u32 fromaddr, void *toaddr, u32 size );

#endif
