/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#include <time.h>

#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */

#include "fatfs_diskio_cfg.h"

#ifdef FATFS_DISKIO_ENABLE_SDCARD
#include "sdcard_simple.h"
#include "sdcard.h"
static sSDCARD *sdcardHandle = NULL;
#endif

#ifdef FATFS_DISKIO_ENABLE_MSD
#include "msd_simple.h"
#include "msd.h"
static sMSD *msdHandle = NULL;
#endif

#ifndef FATFS_DISKIO_TIME
#include <time.h>
#define FATFS_DISKIO_TIME time
#endif

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    DSTATUS status = RES_ERROR;

    switch (pdrv) {
#ifdef FATFS_DISKIO_ENABLE_SDCARD
        case FATFS_DISKIO_SDCARD_DEVICE:
            if (sdcardHandle != NULL) {
                status = RES_OK;
            }
            break;
#endif
#ifdef FATFS_DISKIO_ENABLE_MSD
        case FATFS_DISKIO_MSD_DEVICE:
            if (msdHandle != NULL) {
                status = RES_OK;
            }
            break;
#endif
        default:
            break;
    }

    return(status);
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive number to identify the drive */
)
{
    DSTATUS status = RES_ERROR;

    switch (pdrv) {
#ifdef FATFS_DISKIO_ENABLE_SDCARD
        case FATFS_DISKIO_SDCARD_DEVICE:
            sdcardHandle = sdcardGetHandle();
            if (sdcardHandle != NULL) {
                status = RES_OK;
            }
            break;
#endif
#ifdef FATFS_DISKIO_ENABLE_MSD
        case FATFS_DISKIO_MSD_DEVICE:
            msdHandle = msdGetHandle();
            if (msdHandle != NULL) {
                status = RES_OK;
            }
            break;
#endif
        default:
            break;
    }

    return(status);
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    DSTATUS status = RES_ERROR;

    switch (pdrv) {
#ifdef FATFS_DISKIO_ENABLE_SDCARD
        case FATFS_DISKIO_SDCARD_DEVICE:
            if (sdcardHandle) {
                SDCARD_SIMPLE_RESULT sdResult;
                sdResult = sdcard_read(sdcardHandle, (void *)buff, sector, count);
                if (sdResult == SDCARD_SIMPLE_SUCCESS) {
                    status = RES_OK;
                }
            }
            break;
#endif
#ifdef FATFS_DISKIO_ENABLE_MSD
        case FATFS_DISKIO_MSD_DEVICE:
            if (msdHandle) {
                MSD_SIMPLE_RESULT msdResult;
                msdResult = msd_read(msdHandle, (void *)buff, sector, count);
                if (msdResult == MSD_SIMPLE_SUCCESS) {
                    status = RES_OK;
                }
            }
            break;
#endif
        default:
            break;
    }

    return(status);
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    DSTATUS status = RES_ERROR;

    switch (pdrv) {
#ifdef FATFS_DISKIO_ENABLE_SDCARD
        case FATFS_DISKIO_SDCARD_DEVICE:
            if (sdcardHandle) {
                SDCARD_SIMPLE_RESULT sdResult;
                sdResult = sdcard_write(sdcardHandle, (void *)buff, sector, count);
                if (sdResult == SDCARD_SIMPLE_SUCCESS) {
                    status = RES_OK;
                }
            }
            break;
#endif
#ifdef FATFS_DISKIO_ENABLE_MSD
        case FATFS_DISKIO_MSD_DEVICE:
            if (msdHandle) {
                MSD_SIMPLE_RESULT msdResult;
                msdResult = msd_write(msdHandle, (void *)buff, sector, count);
                if (msdResult == MSD_SIMPLE_SUCCESS) {
                    status = RES_OK;
                }
            }
            break;
#endif
        default:
            status = RES_ERROR;
            break;
    }

    return(status);
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT status = RES_ERROR;

    switch (cmd) {

        case CTRL_SYNC:
            switch (pdrv) {
#ifdef FATFS_DISKIO_ENABLE_SDCARD
                case FATFS_DISKIO_SDCARD_DEVICE:
                    if (sdcardHandle) {
                        SDCARD_SIMPLE_RESULT result;
                        result = sdcard_readyForData(sdcardHandle);
                        if (result == SDCARD_SIMPLE_SUCCESS) {
                            status = RES_OK;
                        }
                    }
                    break;
#endif
#ifdef FATFS_DISKIO_ENABLE_MSD
                case FATFS_DISKIO_MSD_DEVICE:
                    status = RES_OK;
                    break;
#endif
                default:
                    break;
            }
            break;
        default:
            status = RES_PARERR;
            break;

    }

    return(status);

}

DWORD get_fattime (void)
{
    time_t t;
    struct tm _tm;

    FATFS_DISKIO_TIME(&t);
    gmtime_r(&t, &_tm);

    /* Fix up year to always be greater than 1980 */
    _tm.tm_year += 1900;
    if (_tm.tm_year < 1980) {
        _tm.tm_year = 1980;
    }

    /* Pack date and time into a DWORD variable */
    return(
         ((_tm.tm_year - 1980) << 25) |
         ((_tm.tm_mon + 1) << 21) |
         (_tm.tm_mday << 16) |
         (_tm.tm_hour << 11) |
         (_tm.tm_min << 5) |
         (_tm.tm_sec >> 1)
    );
}

