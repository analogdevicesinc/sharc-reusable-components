// Filesystem implementation
#include "romfs.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "romfs.h"
#include "romfs_platform.h"
#include "romfs_devman.h"

#define ROMFS_ALIGN     4
#define ROMFS_ALIGN_OFFSET(x) (( x + ROMFS_ALIGN - 1 ) & ~( ROMFS_ALIGN - 1 ))

#define fsmin( x , y ) ( ( x ) < ( y ) ? ( x ) : ( y ) )

static FD fd_table[ TOTAL_MAX_FDS ];
static int romfs_num_fd;

#define WOFS_END_MARKER_CHAR  0xFF
#define WOFS_DEL_FIELD_SIZE   ( ROMFS_ALIGN )
#define WOFS_FILE_DELETED     0xAA

// Length of the 'file size' field for both ROMFS/WOFS
#define ROMFS_SIZE_LEN        4

// Maximum length of a WOFS file header
#define WOFS_FILE_HEADER_MAX_LEN (DM_MAX_FNAME_LENGTH + 1 + WOFS_MIN_NEEDED_SIZE)

static int romfs_find_empty_fd(void)
{
  int i;

  for( i = 0; i < TOTAL_MAX_FDS; i ++ )
    if( fd_table[ i ].baseaddr == 0xFFFFFFFF &&
        fd_table[ i ].offset == 0xFFFFFFFF &&
        fd_table[ i ].size == 0xFFFFFFFF )
      return i;
  return -1;
}

static void romfs_close_fd( int fd )
{
  memset( fd_table + fd, -1, sizeof( FD ) );
  fd_table[ fd ].flags = 0;
}

// Helper function: read a byte from the FS
static u8 romfsh_read8( u32 addr, const FSDATA *pfs )
{
  u8 temp;
  if( pfs->flags & ROMFS_FS_FLAG_DIRECT )
    return pfs->pbase[ addr ] & 0xFF;
  pfs->readf( &temp, addr, 1, pfs );
  return temp & 0xFF;
}

// Helper function: read a buffer from the FS
static u32 romfsh_read( u32 addr, void *data, u32 len, const FSDATA *pfs )
{
  if( pfs->flags & ROMFS_FS_FLAG_DIRECT ) {
    memcpy(data, pfs->pbase + addr, len);
  } else {
    pfs->readf( data, addr, len, pfs );
  }
  return(len);
}

// Helper function: return 1 if PFS reffers to a WOFS, 0 otherwise
static int romfsh_is_wofs( const FSDATA* pfs )
{
  return ( pfs->flags & ROMFS_FS_FLAG_WO ) != 0;
}

// Open the given file, returning one of FS_FILE_NOT_FOUND, FS_FILE_ALREADY_OPENED
// or FS_FILE_OK
static u8 romfs_open_file( const char* fname, FD* pfd, FSDATA *pfs, u32 *plast, u32 *pnameaddr )
{
  char fsname[ WOFS_FILE_HEADER_MAX_LEN ];
  int is_deleted;
  u32 i, j, n;
  u32 fsize;

  // Look for the file
  i = 0;
  while( 1 )
  {
    // Clear the previous file header
    memset(fsname, 0, sizeof(fsname));

    // Read a file header block
    if ((i + WOFS_FILE_HEADER_MAX_LEN) < pfs->max_size) {
       j = WOFS_FILE_HEADER_MAX_LEN;
    } else {
       j = pfs->max_size - i;
    }
    romfsh_read(i, fsname, j, pfs);

    // Reset indexes
    n = i; j = 0;

    // Look for end of filesystem
    if (fsname[j] == (char)WOFS_END_MARKER_CHAR) {
      *plast = i;
      return FS_FILE_NOT_FOUND;
    }

    // Skip the file name
    while ((j < DM_MAX_FNAME_LENGTH + 1) && (fsname[j++] != '\0'));

    // Round to a multiple of ROMFS_ALIGN
    j = ( j + ROMFS_ALIGN - 1 ) & ~( ROMFS_ALIGN - 1 );

    // WOFS has an additional WOFS_DEL_FIELD_SIZE bytes before the size
    // as an indication for "file deleted"
    if( romfsh_is_wofs( pfs ) )
    {
      is_deleted = fsname[j] == (char)WOFS_FILE_DELETED;
      j += WOFS_DEL_FIELD_SIZE;
    }
    else
      is_deleted = 0;

    // And read the size
    fsize = ( (((u32)fsname[j+0] & 0xFF) <<  0) |
              (((u32)fsname[j+1] & 0xFF) <<  8) |
              (((u32)fsname[j+2] & 0xFF) << 16) |
              (((u32)fsname[j+3] & 0xFF) << 24) );
    j += ROMFS_SIZE_LEN;

    // If the size hasn't been written (file open for writing), terminate
    if (fsize == 0xFFFFFFFF) {
      *plast = i;
      return FS_FILE_NOT_FOUND;
    }

    // Check for filename match
    if( !strncasecmp( fname, fsname, DM_MAX_FNAME_LENGTH ) && !is_deleted )
    {
      // Found the file
      pfd->baseaddr = i + j;
      pfd->offset = 0;
      pfd->size = fsize;
      if( pnameaddr )
        *pnameaddr = n;
      return FS_FILE_OK;
    }

    // Move to next file
    i += j + fsize;
    // On WOFS, all file names must begin at a multiple of ROMFS_ALIGN
    if( romfsh_is_wofs( pfs ) )
      i = ( i + ROMFS_ALIGN - 1 ) & ~( ROMFS_ALIGN - 1 );
  }
}

static int romfs_unlink_r( struct _reent *r, const char *path, void *pdata )
{
   FD tempfs;
   FSDATA *pfsdata = ( FSDATA* )pdata;
   int exists;
   u32 firstfree, nameaddr;

   exists = romfs_open_file( path, &tempfs, pfsdata, &firstfree, &nameaddr ) == FS_FILE_OK;
   if (exists) {
      u8 tempb[] = { WOFS_FILE_DELETED, 0xFF, 0xFF, 0xFF };
      pfsdata->writef( tempb, tempfs.baseaddr - ROMFS_SIZE_LEN - WOFS_DEL_FIELD_SIZE, WOFS_DEL_FIELD_SIZE, pfsdata );
      return FS_FILE_OK;
   } else {
      return FS_FILE_NOT_FOUND;
   }

}

static int romfs_open_r( struct _reent *r, const char *path, int flags, int mode, void *pdata )
{
  FD tempfs;
  int i;
  FSDATA *pfsdata = ( FSDATA* )pdata;
  int must_create = 0;
  int exists;
  u8 lflags = ROMFS_FILE_FLAG_READ;
  u32 firstfree, nameaddr;
  char new_path[DM_MAX_FNAME_LENGTH + 1];
  u8 path_len;

  if( romfs_num_fd == TOTAL_MAX_FDS )
  {
    r->_errno = ENFILE;
    return -1;
  }
  // Truncate the path if too long
  path_len = strlen( path );
  if (path_len > DM_MAX_FNAME_LENGTH) {
     path_len = DM_MAX_FNAME_LENGTH;
     strncpy(new_path, path, path_len);
     new_path[path_len] = '\0';
     path = (char *)new_path;
  }
  // Does the file exist?
  exists = romfs_open_file( path, &tempfs, pfsdata, &firstfree, &nameaddr ) == FS_FILE_OK;
  // Now interpret "flags" to set file flags and to check if we should create the file
  if( flags & O_CREAT )
  {
    // If O_CREAT is specified with O_EXCL and the file already exists, return with error
    if( ( flags & O_EXCL ) && exists )
    {
      r->_errno = EEXIST;
      return -1;
    }
    // Otherwise create the file if it does not exist
    must_create = !exists;
  }
  if( ( flags & O_TRUNC ) && ( flags & ( O_WRONLY | O_RDWR ) ) && exists )
  {
    // The file exists, but it must be truncated
    // In the case of WOFS, this effectively means "create a new file"
    must_create = 1;
  }
  // ROMFS can't create files
  if( must_create && ( ( pfsdata->flags & ROMFS_FS_FLAG_WO ) == 0 ) )
  {
    r->_errno = EROFS;
    return -1;
  }
  // Decode access mode
  if( flags & O_WRONLY )
    lflags = ROMFS_FILE_FLAG_WRITE;
  else if( flags & O_RDWR )
    lflags = ROMFS_FILE_FLAG_READ | ROMFS_FILE_FLAG_WRITE;
  if( flags & O_APPEND )
    lflags |= ROMFS_FILE_FLAG_APPEND;
  // If a write access is requested when the file must NOT be created, this
  // is an error
  if( ( lflags & ( ROMFS_FILE_FLAG_WRITE | ROMFS_FILE_FLAG_APPEND ) ) && !must_create )
  {
    r->_errno = EACCES;
    return -1;
  }
  if( ( lflags & ( ROMFS_FILE_FLAG_WRITE | ROMFS_FILE_FLAG_APPEND ) ) && romfs_fs_is_flag_set( pfsdata, ROMFS_FS_FLAG_WRITING ) )
  {
    // At most one file can be opened in write mode at any given time on WOFS
    r->_errno = EROFS;
    return -1;
  }
  // Do we need to create the file ?
  if( must_create )
  {
    if( exists )
    {
      // Invalidate the file first by changing WOFS_DEL_FIELD_SIZE bytes before
      // the file length to WOFS_FILE_DELETED
      u8 tempb[] = { WOFS_FILE_DELETED, 0xFF, 0xFF, 0xFF };
      pfsdata->writef( tempb, tempfs.baseaddr - ROMFS_SIZE_LEN - WOFS_DEL_FIELD_SIZE, WOFS_DEL_FIELD_SIZE, pfsdata );
    }
    // Find the last available position by asking romfs_open_file to look for a file
    // with an invalid name
    romfs_open_file( "\1", &tempfs, pfsdata, &firstfree, NULL );
    // Is there enough space on the FS for another file?
    if( pfsdata->max_size - firstfree + 1 < path_len + WOFS_MIN_NEEDED_SIZE + WOFS_DEL_FIELD_SIZE )
    {
      r->_errno = ENOSPC;
      return -1;
    }

    // Make sure we can get a file descriptor before writing
    if( ( i = romfs_find_empty_fd() ) < 0 )
    {
      r->_errno = ENFILE;
      return -1;
    }

    // Write the name of the file
    pfsdata->writef( path, firstfree, path_len + 1, pfsdata );
    firstfree += path_len + 1; // skip over the name
    // Align to a multiple of ROMFS_ALIGN
    firstfree = ( firstfree + ROMFS_ALIGN - 1 ) & ~( ROMFS_ALIGN - 1 );
    firstfree += ROMFS_SIZE_LEN + WOFS_DEL_FIELD_SIZE; // skip over the size and the deleted flags area
    tempfs.baseaddr = firstfree;
    tempfs.offset = tempfs.size = 0;
    // Set the "writing" flag on the FS to indicate that there is a file opened in write mode
    romfs_fs_set_flag( pfsdata, ROMFS_FS_FLAG_WRITING );
  }
  else // File must exist (and was found in the previous 'romfs_open_file' call)
  {
    if( !exists )
    {
      r->_errno = ENOENT;
      return -1;
    }

    if( ( i = romfs_find_empty_fd() ) < 0 )
    {
      r->_errno = ENFILE;
      return -1;
    }
  }
  // Copy the descriptor information
  tempfs.flags = lflags;
  memcpy( fd_table + i, &tempfs, sizeof( FD ) );
  romfs_num_fd ++;
  return i;
}

static int romfs_close_r( struct _reent *r, int fd, void *pdata )
{
  FD* pfd = fd_table + fd;
  FSDATA *pfsdata = ( FSDATA* )pdata;
  u8 temp[ ROMFS_SIZE_LEN ];

  if( pfd->flags & ( ROMFS_FILE_FLAG_WRITE | ROMFS_FILE_FLAG_APPEND ) )
  {
    // Write back the size
    temp[ 0 ] = pfd->size & 0xFF;
    temp[ 1 ] = ( pfd->size >> 8 ) & 0xFF;
    temp[ 2 ] = ( pfd->size >> 16 ) & 0xFF;
    temp[ 3 ] = ( pfd->size >> 24 ) & 0xFF;
    pfsdata->writef( temp, pfd->baseaddr - ROMFS_SIZE_LEN, ROMFS_SIZE_LEN, pfsdata );
    // Clear the "writing" flag on the FS instance to allow other files to be opened
    // in write mode
    romfs_fs_clear_flag( pfsdata, ROMFS_FS_FLAG_WRITING );
  }
  romfs_close_fd( fd );
  romfs_num_fd --;
  return 0;
}

static _ssize_t romfs_write_r( struct _reent *r, int fd, const void* ptr, size_t len, void *pdata )
{
  FD* pfd = fd_table + fd;
  FSDATA *pfsdata = ( FSDATA* )pdata;

  if( ( pfd->flags & ( ROMFS_FILE_FLAG_WRITE | ROMFS_FILE_FLAG_APPEND ) ) == 0 )
  {
    r->_errno = EINVAL;
    return -1;
  }
  // Append mode: set the file pointer to the end
  if( pfd->flags & ROMFS_FILE_FLAG_APPEND )
    pfd->offset = pfd->size;
  // Only write at the end of the file!
  if( pfd->offset != pfd->size )
    return 0;
  // Check if we have enough space left on the device. Always keep 1 byte for the final 0xFF
  // and ROMFS_ALIGN - 1 bytes for aligning the contents of the file data in the worst case
  // scenario (so ROMFS_ALIGN bytes in total)
  if( pfd->baseaddr + pfd->size + len > pfsdata->max_size - ROMFS_ALIGN )
    len = pfsdata->max_size - ( pfd->baseaddr + pfd->size ) - ROMFS_ALIGN;
  pfsdata->writef( ptr, pfd->offset + pfd->baseaddr, len, pfsdata );
  pfd->offset += len;
  pfd->size += len;
  return len;
}

static _ssize_t romfs_read_r( struct _reent *r, int fd, void* ptr, size_t len, void *pdata )
{
  FD* pfd = fd_table + fd;
  long actlen = fsmin( len, pfd->size - pfd->offset );
  FSDATA *pfsdata = ( FSDATA* )pdata;

  if( ( pfd->flags & ROMFS_FILE_FLAG_READ ) == 0 )
  {
    r->_errno = EBADF;
    return -1;
  }
  if( pfsdata->flags & ROMFS_FS_FLAG_DIRECT )
    memcpy( ptr, pfsdata->pbase + pfd->offset + pfd->baseaddr, actlen );
  else
    actlen = pfsdata->readf( ptr, pfd->offset + pfd->baseaddr, actlen, pfsdata );
  pfd->offset += actlen;
  return actlen;
}

// lseek
static off_t romfs_lseek_r( struct _reent *r, int fd, off_t off, int whence, void *pdata )
{
  FD* pfd = fd_table + fd;
  u32 newpos = 0;

  switch( whence )
  {
    case SEEK_SET:
      newpos = off;
      break;

    case SEEK_CUR:
      newpos = pfd->offset + off;
      break;

    case SEEK_END:
      newpos = pfd->size + off;
      break;

    default:
      return -1;
  }
  if( newpos > pfd->size )
    return -1;
  pfd->offset = newpos;
  return newpos;
}

// Directory operations
static u32 romfs_dir_data = 0;

// opendir
static void* romfs_opendir_r( struct _reent *r, const char* dname, void *pdata )
{
  if( !dname || strlen( dname ) == 0 || ( strlen( dname ) == 1 && !strcmp( dname, "/" ) ) )
  {
    romfs_dir_data = 0;
    return &romfs_dir_data;
  }
  return NULL;
}

// readdir
static struct dm_dirent dm_shared_dirent;
static char dm_shared_fname[ WOFS_FILE_HEADER_MAX_LEN ];
static struct dm_dirent* romfs_readdir_r( struct _reent *r, void *d, void *pdata )
{
  u32 off = *( u32* )d;
  struct dm_dirent *pent = &dm_shared_dirent;
  char *header = dm_shared_fname;
  FSDATA *pfsdata = ( FSDATA* )pdata;
  int is_deleted;
  u32 j;

  while( 1 )
  {

    // Clear the previous file header
    memset(dm_shared_fname, 0, sizeof(dm_shared_fname));

    // Read a file header block
    if ((off + WOFS_FILE_HEADER_MAX_LEN) < pfsdata->max_size) {
       j = WOFS_FILE_HEADER_MAX_LEN;
    } else {
       j = pfsdata->max_size - off;
    }
    romfsh_read(off, header, j, pfsdata);

    // Look for end of filesystem
    if (header[0] == (char)WOFS_END_MARKER_CHAR) {
      return NULL;
    }

    // Reset indexes
    pent->fname = header;
    j = 0;

    // Skip the file name
    while ((j < DM_MAX_FNAME_LENGTH + 1) && (header[j++] != '\0'));

    // Round to a multiple of ROMFS_ALIGN
    j = ( j + ROMFS_ALIGN - 1 ) & ~( ROMFS_ALIGN - 1 );

    // WOFS has an additional WOFS_DEL_FIELD_SIZE bytes before the size
    // as an indication for "file deleted"
    if( romfsh_is_wofs( pfsdata ) )
    {
      is_deleted = header[j] == (char)WOFS_FILE_DELETED;
      j += WOFS_DEL_FIELD_SIZE;
    }
    else
    {
      is_deleted = 0;
    }

    // And read the size
    pent->fsize = ( (((u32)header[j+0] & 0xFF) <<  0) |
                    (((u32)header[j+1] & 0xFF) <<  8) |
                    (((u32)header[j+2] & 0xFF) << 16) |
                    (((u32)header[j+3] & 0xFF) << 24) );
    pent->ftime = 0;
    pent->flags = 0;
    j += ROMFS_SIZE_LEN;

    // If the size hasn't been written (file open for writing), terminate
    if (pent->fsize == 0xFFFFFFFF) {
       return(NULL);
    }

    // Fast forward to next file
    off += j + pent->fsize;
    if( romfsh_is_wofs( pfsdata ) )
      off = ( off + ROMFS_ALIGN - 1 ) & ~( ROMFS_ALIGN - 1 );

    if( !is_deleted )
      break;
  }

  *( u32* )d = off;
  return pent;
}

// closedir
static int romfs_closedir_r( struct _reent *r, void *d, void *pdata )
{
  *( u32* )d = 0;
  return 0;
}

// getaddr
static const char* romfs_getaddr_r( struct _reent *r, int fd, void *pdata )
{
  FD* pfd = fd_table + fd;
  FSDATA *pfsdata = ( FSDATA* )pdata;

  if( pfsdata->flags & ROMFS_FS_FLAG_DIRECT )
    return ( const char* )pfsdata->pbase + pfd->baseaddr;
  else
    return NULL;
}

// ****************************************************************************
// Our ROMFS device descriptor structure
// These functions apply to both ROMFS and WOFS

static const DM_DEVICE romfs_device =
{
  romfs_open_r,         // open
  romfs_close_r,        // close
  romfs_write_r,        // write
  romfs_read_r,         // read
  romfs_lseek_r,        // lseek
  romfs_opendir_r,      // opendir
  romfs_readdir_r,      // readdir
  romfs_closedir_r,     // closedir
  romfs_getaddr_r,      // getaddr
  NULL,                 // mkdir
  romfs_unlink_r,       // unlink
  NULL,                 // rmdir
  NULL                  // rename
};

// ****************************************************************************
// WOFS functions and instance descriptor for real hardware

static u32 romfs_write( const void *from, u32 toaddr, u32 size, const void *pdata )
{
  const FSDATA *pfsdata = ( const FSDATA* )pdata;

  toaddr += ( u32 )pfsdata->pbase;
  return platform_flash_write( from, toaddr, size );
}

static u32 romfs_read( void *to, u32 fromaddr, u32 size, const void *pdata )
{
  const FSDATA *pfsdata = ( const FSDATA* )pdata;

  fromaddr += ( u32 )pfsdata->pbase;
  return platform_flash_read( fromaddr, to, size );
}

// This must NOT be a const!
static FSDATA wofs_fsdata =
{
  NULL,
  ROMFS_FS_FLAG_WO,
  romfs_read,
  romfs_write,
  0
};

// WOFS formatting function
// Returns 1 if OK, 0 for error
int romfs_format( u8 all )
{
  u32 sect_first, sect_last;
  FD tempfd;

  platform_flash_get_first_free_block_address( &sect_first );
  // Get the first free address in WOFS. We use this address to compute the last block that we need to
  // erase, instead of simply erasing everything from sect_first to the last Flash page.
  if (all) {
    sect_last = platform_flash_get_sector_of_address( ROMFS_FS_END_ADDRESS - 1);
  } else {
    romfs_open_file( "\1", &tempfd, &wofs_fsdata, &sect_last, NULL );
    sect_last = platform_flash_get_sector_of_address( sect_last + ( u32 )wofs_fsdata.pbase );
  }
  while( sect_first <= sect_last )
    if( platform_flash_erase_sector( sect_first ++ ) == PLATFORM_ERR )
      return 0;
  return 1;
}

int romfs_full(u32 *size, u32 *used)
{
  u32 plast;
  FD tempfd;

  // Get the first free address in WOFS.
  romfs_open_file( "\1", &tempfd, &wofs_fsdata, &plast, NULL );

  // Calculate the amount used
  *size = wofs_fsdata.max_size;
  *used = plast;

  return 1;
}

// Initialize both ROMFS and WOFS as needed
int romfs_init( const FLASH_INFO *fi )
{
  unsigned i;

  for( i = 0; i < TOTAL_MAX_FDS; i ++ )
  {
    memset( fd_table + i, -1, sizeof( FD ) );
    fd_table[ i ].flags = 0;
  }

  // Get the start address and size of WOFS and register it
  wofs_fsdata.pbase = ( u8* )platform_flash_get_first_free_block_address( NULL );
  wofs_fsdata.max_size = ROMFS_FS_END_ADDRESS - ROMFS_FS_START_ADDRESS;

  // Initialize the platform with the flash handle
  platform_init(fi);

  // Register the filesystem with the device manager
  dm_register( "/wo", &wofs_fsdata, &romfs_device );

  return 0;
}

int romfs_fsck(int repair, int *repaired)
{
   u32 i, j, x, fsize;
   FSDATA *pfsdata = &wofs_fsdata;
   char header[WOFS_FILE_HEADER_MAX_LEN];
   u8 temp[ ROMFS_SIZE_LEN ];
   unsigned repairOff, scanOff;
   char c;
   bool ok;

   // Can't repair a ROMFS
   if( repair && !romfsh_is_wofs( pfsdata ) ) {
      repair = 0;
   }
   if (repaired) {
      *repaired = 0;
   }

   i = 0;
   while( 1 )
   {
       // Clear the previous file header
       memset(dm_shared_fname, 0, sizeof(dm_shared_fname));

      // Read a block from the beginning of the file
      if ((i + WOFS_FILE_HEADER_MAX_LEN) <= pfsdata->max_size) {
         j = WOFS_FILE_HEADER_MAX_LEN;
      } else {
         j = pfsdata->max_size - i;
      }
      romfsh_read(i, header, j, pfsdata);
      j = 0;

      // Check for end of filesystem
      if (header[j] == (char)WOFS_END_MARKER_CHAR) {
         return(FS_OK);
      }

      // Confirm filename is OK
      ok = false;
      do {
         if (header[j] == '\0') {
            if (j > 0) {
               ok = true;
               j++;
            }
            break;
         }
         if (isprint(header[j]) == 0) {
            ok = false;
            break;
         }
         j++;
      } while (j <= DM_MAX_FNAME_LENGTH + 1);
      if (!ok) {
         return(FS_ERROR);
      }

      // Align and check deleted fields
      j = ROMFS_ALIGN_OFFSET(j);
      if( romfsh_is_wofs( pfsdata ) ) {
         if ( (header[j] != (char)WOFS_FILE_DELETED) &&
              (header[j] != (char)WOFS_END_MARKER_CHAR) ) {
            ok = false;
         }
         for (x = 1; x < WOFS_DEL_FIELD_SIZE; x++) {
            if (header[j+x] != (char)WOFS_END_MARKER_CHAR) {
               ok = false;
               break;
            }
         }
         j += WOFS_DEL_FIELD_SIZE;
      }
      if (!ok) {
         return(FS_ERROR);
      }

      // Check the size
      fsize = ( (((u32)header[j+0] & 0xFF) <<  0) |
                (((u32)header[j+1] & 0xFF) <<  8) |
                (((u32)header[j+2] & 0xFF) << 16) |
                (((u32)header[j+3] & 0xFF) << 24) );
      j += ROMFS_SIZE_LEN;

      // If size = 0xFFFFFFFF, then the file was not properly closed.
      // Attempt a repair by scanning for at least 256 consecutive
      // 0xFF's and writing the size of the file based off of the
      // location of the last non 0xFF character found.
      if (fsize == 0xFFFFFFFF) {
         if (repair) {
            scanOff = repairOff = i + j;
            while (1) {
               c = romfsh_read8(scanOff, pfsdata); scanOff++;
               if (c != (char)WOFS_END_MARKER_CHAR) {
                  repairOff = scanOff;
               }
               if (((scanOff-repairOff) >= 256) || (scanOff == pfsdata->max_size)) {
                  break;
               }
            }
            fsize = repairOff - (i + j);
            temp[ 0 ] = fsize & 0xFF;
            temp[ 1 ] = ( fsize >> 8 ) & 0xFF;
            temp[ 2 ] = ( fsize >> 16 ) & 0xFF;
            temp[ 3 ] = ( fsize >> 24 ) & 0xFF;
            pfsdata->writef( temp, i + j - ROMFS_SIZE_LEN, ROMFS_SIZE_LEN, pfsdata );
            if (repaired) {
               *repaired = 1;
            }
         } else {
            ok = false;
         }
      }
      if (fsize > pfsdata->max_size) {
         ok = false;
      }
      if (i + fsize > pfsdata->max_size) {
         ok = false;
      }
      if (!ok) {
         return(FS_ERROR);
      }

      // Skip to the next file
      i += j + fsize;
      if( romfsh_is_wofs( pfsdata ) ) {
         i = ROMFS_ALIGN_OFFSET(i);
      }
   }
}

