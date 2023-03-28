#ifndef _flash_map_h
#define _flash_map_h

/* Physical start of flash */
#define FLASH_ADDR    (0x00000000)

/* Second-stage boot loader (64k reserved) */
#define BOOT0_OFFSET  (FLASH_ADDR)
#define BOOT0_SIZE    (0x00010000)

/* Third stage boot loader (USB re-flasher, 192k reserved) */
#define BOOT1_OFFSET  (BOOT0_OFFSET + BOOT0_SIZE)
#define BOOT1_SIZE    (0x00030000)

/* Application (1M reserved) */
#define APP_OFFSET    (BOOT1_OFFSET + BOOT1_SIZE)
#define APP_SIZE      (0x00100000)

/* Filesystem (1M reserved) */
#define FS_OFFSET     (APP_OFFSET + APP_SIZE)
#define FS_SIZE       (0x00100000)

#endif
