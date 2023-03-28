#include "romfs.h"
#include "romfs_devman.h"

DM_INSTANCE_DATA romfs;
struct _reent reent;

int dm_register( const char *name, void *pdata, const DM_DEVICE *pdev )
{
   romfs.name = name;
   romfs.pdev = pdev;
   romfs.pdata = pdata;
   return(DM_OK);
}

struct dm_dirent* dm_readdir( DM_DIR *d )
{
   struct dm_dirent *dirent;
   dirent = romfs.pdev->p_readdir_r(&reent, d, romfs.pdata);
   return(dirent);
}

DM_DIR *dm_opendir( const char* dirname )
{
   void *dir;
   dir = romfs.pdev->p_opendir_r(&reent, dirname, romfs.pdata);
   return((DM_DIR *)dir);
}

int dm_closedir( DM_DIR *d )
{
   int ret;
   ret = romfs.pdev->p_closedir_r(&reent, (void *)d, romfs.pdata);
   return(ret);
}

int dm_unlink( const char *pathname)
{
   int ret;
   ret = romfs.pdev->p_unlink_r(&reent, pathname, romfs.pdata);
   return(ret);
}

const DM_INSTANCE_DATA* dm_get_instance_at( int idx )
{
   if (idx == 0)
   {
      return(&romfs);
   }
   return(NULL);
}

int dm_get_num_devices()
{
   return(1);
}

/* The following belong somewhere else, but this will have to do for now */

int fs_open(const char *name, int mode)
{
   int fd;
   fd = romfs.pdev->p_open_r(&reent, name, mode, 0, romfs.pdata);

   /* If no errors, add an offset to the FD to keep it out of the
    * stdin/out/err range */
   if (fd >= 0) {
      fd += FD_OFFSET;
   }

   return(fd);
}

int fs_close(int fd)
{
   romfs.pdev->p_close_r(&reent, fd - FD_OFFSET, romfs.pdata);
   return(0);
}

int fs_read(int fd, unsigned char *buf, int size)
{
   size = romfs.pdev->p_read_r(&reent, fd - FD_OFFSET, buf, size, romfs.pdata);
   return(size);
}

int fs_write(int fd, unsigned char *buf, int size)
{
    size = romfs.pdev->p_write_r(&reent, fd - FD_OFFSET, buf, size, romfs.pdata);
    return(size);
}

long fs_seek(int fd, long offset, int whence)
{
   offset = romfs.pdev->p_lseek_r(&reent, fd - FD_OFFSET, offset, whence, romfs.pdata);
   return(offset);
}
