/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ossIO.hpp

   Descriptive Name = Operating System Services IO Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for IO operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSIO_HPP_
#define OSSIO_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossTypes.hpp"
#include "ossLatch.hpp"

#define  OSS_FILE_BLOCK    4096
/*
 * File access modes
 */

// open/create mode
#define OSS_DEFAULT        0x00000000     // default open option

// create file when not exist, otherwise return error
#define OSS_CREATEONLY     0x00000001

// empty file when exist, otherwise create one
#define OSS_REPLACE        0x00000002

// create file when not exist, then open the file
#define OSS_CREATE         (OSS_CREATEONLY | OSS_REPLACE)

// read/write access mode
#define OSS_READONLY       0x00000004     // read only mode
#define OSS_WRITEONLY      0x00000008     // write only mode
#define OSS_READWRITE      (OSS_READONLY | OSS_WRITEONLY)

// exclusive/share mode
#define OSS_EXCLUSIVE      0x00000000     // exclusive access, by default
#define OSS_SHAREREAD      0x00000010     // shared read
// shared write, must be shared read too
#define OSS_SHAREWRITE     OSS_SHAREREAD|0x00000020
#define OSS_WRITETHROUGH   0x00000040     // write through mode
#define OSS_DIRECTIO       0x00000080     // direct io

/*
 * File access permissions
 */
#if defined (_WINDOWS)
  // user
#define OSS_RU      0x00000400
#define OSS_WU      0x00000200
#define OSS_XU      0x00000100
#define OSS_RWXU     (OSS_RU | OSS_WU | OSS_XU)
 // group
#define OSS_RG      0x00000040
#define OSS_WG      0x00000020
#define OSS_XG      0x00000010
#define OSS_RWXG     (OSS_RG | OSS_WG | OSS_XG)
 // other
#define OSS_RO      0x00000004
#define OSS_WO      0x00000002
#define OSS_XO      0x00000001
#define OSS_RWXO     (OSS_RO | OSS_WO | OSS_XO)
 // sticky bit
#define OSS_STICKY     0

// rwxr-x---
#define OSS_DEFAULTFILE   (OSS_RWXU | OSS_RG | OSS_XG) 
#else
 // user
#define OSS_RU      S_IRUSR
#define OSS_WU      S_IWUSR
#define OSS_XU      S_IXUSR
#define OSS_RWXU    S_IRWXU
 // group
#define OSS_RG      S_IRGRP
#define OSS_WG      S_IWGRP
#define OSS_XG      S_IXGRP
#define OSS_RWXG    S_IRWXG
 // other
#define OSS_RO      S_IROTH
#define OSS_WO      S_IWOTH
#define OSS_XO      S_IXOTH
#define OSS_RWXO    S_IRWXO
// sticky bit
#define OSS_STICKY  S_ISVTX

// rw-r-----
#define OSS_DEFAULTFILE   (OSS_RU | OSS_WU | OSS_RG)
#endif

// rwxrwxrwx
#define OSS_PERMALL       (OSS_RWXU | OSS_RWXG | OSS_RWXO )
// rwxr-xr-x
#define OSS_DEFAULTDIR    (OSS_RWXU | OSS_RG | OSS_XG | OSS_RO | OSS_XO )

class _OSS_FILE : public SDBObject
{
public :
#if defined (_WINDOWS)
#ifdef OSSFILE_XLOCK
#undef OSSFILE_XLOCK
#endif
#define OSSFILE_XLOCK ossScopedLock _lock ( &(pFile->_mutex), EXCLUSIVE ) ;
#ifdef OSSFILE_SLOCK
#undef OSSFILE_SLOCK
#endif
#define OSSFILE_SLOCK ossScopedLock _lock ( &(pFile->_mutex), SHARED ) ;
    HANDLE hFile;
    ossSpinSLatch _mutex ;
#else
    int fd;
#endif
    _OSS_FILE ()
    {
#if defined (_WINDOWS)
       hFile=INVALID_HANDLE_VALUE;
#else
       fd=0;
#endif
    }

    BOOLEAN isOpened() const
    {
#if defined (_WINDOWS)
       return INVALID_HANDLE_VALUE != hFile ;
#else
       return 0 != fd && -1 != fd ;
#endif
    }
};

typedef class _OSS_FILE OSSFILE;

enum OSS_SEEK
{
   OSS_SEEK_SET = 0,
   OSS_SEEK_CUR,
   OSS_SEEK_END
};

enum OSS_FS_TYPE
{
   OSS_FS_TYPE_UNKNOWN = 0 ,
   OSS_FS_NTFS ,
   OSS_FS_EXT2_OLD = 0xEF51 ,
   OSS_FS_EXT2     = 0xEF53 ,
   OSS_FS_EXT3     = 0xEF53 ,
};

enum OSS_FILE_LOCK
{
   OSS_LOCK_SH = 0,
   OSS_LOCK_EX,
   OSS_LOCK_UN
} ;

/*
 * Open a file (create)
 * Input
 * file name (string)
 * mode (integer)
 * permission (integer)
 * Output
 * OSSFILE reference
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (file not exist)
 * SDB_FE (file already exist)
 * SDB_IO (generic IO error)
 */
INT32 ossOpen ( const CHAR  *pFileName,
                UINT32      iMode,
                UINT32      iPermission,
                OSSFILE     &pFile ) ;

/*
 * Close a file
 * Input
 * OSSFILE reference
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_IO (generic IO error)
 */
INT32 ossClose ( OSSFILE& pFile ) ;

/*
 * Changes the mode of the file or directory specified in pathname
 * Input
 * file or dir (string)
 * permission (integer)
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (file or dir not exist)
 */
INT32 ossChmod( const CHAR* pPathName, UINT32 iPermission ) ;

/*
 * Changes the mode of the file or directory specified in pathname
 * Input
 * file or dir (string)
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (path not exist)
 */
INT32 ossChown( const CHAR* pPathName, OSSUID uid, OSSGID gid ) ;

/*
 * Create a directory
 * Input
 * dir (string)
 * permission (integer)
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FE (dir already exist)
 */
INT32 ossMkdir( const CHAR   *pPathName,
                UINT32 iPermission = OSS_DEFAULTDIR ) ;

/*
 * get permissions
 * Input
 * path (string)
 * Output
 * permission (integer)
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (path not exist)
 */
INT32 ossPermissions( const CHAR* pPathName, UINT32 &iPermission ) ;

/*
   * Get current working directory
   * pPath   : output
   * maxSize : the path size
   * Return  : SDB_OK ( succeed ), otherwise failed
*/
INT32 ossGetCWD( CHAR *pPath, UINT32 maxSize ) ;

/*
   * Change the current directory to pPath
   * pPath   : input
   * Return  : SDB_OK ( succeed ), otherwise failed
*/
INT32 ossChDir( const CHAR *pPath ) ;

/*
 * Delete a file or directory
 * Input
 * file (string)
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (file not exist)
 */
INT32 ossDelete( const CHAR  *pPathName ) ;

/*
 * Copy pSrcFile to pDstFile
 * Input
 * pSrcFile (string)
 * pDstFile (string)
 * iPermission ( UINT32 )
 * isReplace ( BOOLEAN )
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (file not exist)
 * SDB_FE ( file already exist)
 * SDB_IO ( io error )
 */
INT32 ossFileCopy( const CHAR *pSrcFile,
                   const CHAR *pDstFile,
                   UINT32      iPermission = OSS_DEFAULTFILE,
                   BOOLEAN     isReplace = TRUE ) ;

#define  OSS_MODE_ACCESS         00       /// F_OK
#define  OSS_MODE_EXCUSIVE       01       /// X_OK
#define  OSS_MODE_WRITE          02       /// W_OK
#define  OSS_MODE_READ           04       /// R_OK
#define  OSS_MODE_READWRITE      ( OSS_MODE_WRITE | OSS_MODE_READ )

INT32 ossAccess ( const CHAR  *pPathName, int flags = OSS_MODE_ACCESS ) ;

/*
 * Read a file from current file descriptor location
 * Input
 * file descriptor (OSSFILE)
 * buffer (char*)
 * length (integer)
 * Output
 * pBufferRead (integer pointer)
 * Return
 * Success: read length (>=0)
 *  Failed:
 * SDB_INVALIDARG (invalid parameter passed in)
 * SDB_INVALIDSIZE (read size is not valid)
 * SDB_OOM (out of memory)
 * SDB_IO (IO error)
 * SDB_EOF (hit end of the file)
 * SDB_INTERRUPT (interrupt received)
 */
INT32 ossRead ( OSSFILE  *pFile,
                CHAR     *pBufferRead,
                SINT64   iLenToRead,
                SINT64   *pLenRead ) ;

/*
 * Write a file at current file descriptor location
 * Input
 * file descriptor (OSSFILE)
 * buffer (char*)
 * length (integer)
 * Output
 * N/A
 * Return
 * Success: write length (>=0)
 *  Failed:
 * SDB_INVALIDARG (invalid parameter passed in)
 * SDB_INVALIDSIZE (write size is not valid)
 * SDB_IO (IO error)
 * SDB_INTERRUPT (interrupt received)
 */
INT32 ossWrite ( OSSFILE        *pFile,
                 const CHAR     *pBufferWrite,
                 SINT64         iLenToWrite,
                 SINT64         *pLenWritten);

/*
 * Replisition read/write file offset
 * Input
 * File descriptor (OSSFILE)
 * offset (off_t)
 * whence (OSS_SEEK)
 * Output
 * position (the resulting offset location from the beginning of the file)
 * Return
 * SDB_OK (success)
 * SDB_INVALIDSIZE (seek location is not valid)
 * SDB_INVALIDARGS (bad arguments)
 */
INT32 ossSeek( OSSFILE  *pFile,
               INT64 offset,
               OSS_SEEK whence,
               INT64* position = NULL );

/*
 * Read a file from current file descriptor location
 * Input
 * file descriptor (OSSFILE)
 * offset (off_t)
 * whence (OSS_SEEK)
 * buffer (char*)
 * length (integer)
 * Output
 * N/A
 * Return
 * Success: read length (>=0)
 *  Failed:
 * SDB_INVALIDARG (invalid parameter passed in)
 * SDB_INVALIDSIZE (read size is not valid)
 * SDB_IO (IO error)
 * SDB_INTERRUPT (interrupt received)
 */
INT32 ossSeekAndRead( OSSFILE   *pFile,
                       INT64     offset,
                       CHAR      *pBufferRead,
                       SINT64     iLenToRead,
                       SINT64    *pLenRead );

/*
 * Write a file from current file descriptor location
 * Input
 * file descriptor (OSSFILE)
 * offset (off_t)
 * whence (OSS_SEEK)
 * buffer (char*)
 * length (integer)
 * Output
 * N/A
 * Return
 * Success: write length (>=0)
 *  Failed:
 * SDB_INVALIDARG (invalid parameter passed in)
 * SDB_INVALIDSIZE (write size is not valid)
 * SDB_IO (IO error)
 * SDB_INTERRUPT (interrupt received)
 */
INT32 ossSeekAndWrite( OSSFILE    *pFile,
                       INT64       offset,
                       const CHAR *pBufferWrite,
                       SINT64      iLenToWrite,
                       SINT64     *pLenWritten);

/*
 * Read a file from current file descriptor location using MMAP
 * Input
 * file descriptor (OSSFILE)
 * buffer (char*)
 * length (integer)
 * Output
 * pBufferRead (integer pointer)
 * Return
 * Success: read length (>=0)
 *  Failed:
 * SDB_INVALIDARG (invalid parameter passed in)
 * SDB_INVALIDSIZE (read size is not valid)
 * SDB_OOM (out of memory)
 * SDB_IO (IO error)
 * SDB_EOF (hit end of the file)
 * SDB_INTERRUPT (interrupt received)
 */
INT32 ossMmapRead( OSSFILE  *pFile,
                   CHAR     *pBuffer,
                   size_t   iLength,
                   size_t   *pBufferRead);

/*
  * Synchronize a file
  * Input
  * file descriptor (OSSFILE)
  * Output
  * N/A
  * Return
  * SDB_OK (success)
  * SDB_IO (IO error)
  * SDB_INVALIDARG (invalid file descriptor)
  */
INT32 ossFsync(OSSFILE* pFile);

enum SDB_OSS_FILETYPE
{
   SDB_OSS_UNK = 0,  // unknown type
   SDB_OSS_FIL,      // file type
   SDB_OSS_DIR,      // directory type
   SDB_OSS_SYM,      // symbolic link
   SDB_OSS_DEV,      // device
   SDB_OSS_PIP,      // pipe
   SDB_OSS_SCK       // socket
} ;

#define SDB_OSS_ISFIL(x) (SDB_OSS_FIL==x)
#define SDB_OSS_ISDIR(x) (SDB_OSS_DIR==x)
#define SDB_OSS_ISSYM(x) (SDB_OSS_SYM==x)
#define SDB_OSS_ISDEV(x) (SDB_OSS_DEV==x)
#define SDB_OSS_ISPIP(x) (SDB_OSS_PIP==x)
 /*
  * Type of a given path
  * Input
  * path (string)
  * Output
  * N/A
  * Return
  * type (integer, when >=0)
  * SDB_IO (IO error)
  * SDB_FNE (Path not exist)
  * SDB_PERM (permission error)
  * SDB_INVALIDARG (invalid path)
  */
INT32 ossGetPathType ( const CHAR  *pPath, SDB_OSS_FILETYPE *pFileType ) ;

 /*
  * Get file size
  * Input
  * Input
  * path (string)
  *
  * Output
  * N/A
  * Return
  * size (size_t, when >=0)
         *      SDB_IO (IO error)
         *      SDB_FNE (Path not exist)
         *      SDB_PERM (permission error)
         *      SDB_INVALIDARG (invalid path)
         */
INT32 ossGetFileSizeByName ( const CHAR  *pFileName, INT64 *pFileSize );

 /*
  * Get file size
  * Input
  *      file descriptor
  *
  * Output
  *      N/A
  * Return
  *      size (size_t, when >=0)
  *      SDB_IO (IO error)
  *      SDB_PERM (permission error)
  *      SDB_INVALIDARG (invalid path)
  */
INT32 ossGetFileSize(OSSFILE *pFile, INT64 *pfsize);


/*
 * Extend file with specified size(bytes)
 * Input
 *      file descriptor
 *      increased size(bytes)
 * Output
 *
 * Return
 *      SDB_OK
 *      SDB_IO (IO error)
 *      SDB_INVALIDARG (invalid input arguments)
 *      SDB_INVALID_FILE_TYPE (invalid input arguments)
 */
INT32 ossExtendFile( OSSFILE *pFile,
                     const INT64 incrementSize ) ;

INT32 ossExtentBySparse( OSSFILE *pFile,
                         UINT64 incrementSize,
                         UINT32 onceWrite = 512 ) ;

INT32 ossTruncateFile ( OSSFILE *pFile, const INT64 fileLen ) ;

CHAR* ossGetRealPath(const CHAR  *pPath,
                     CHAR        *resolvedPath,
                     UINT32      length);

/*
 * Verify the supported file system type
 * and supported file block size
 *
 * Input
 *      one file name
 *
 * Output
 *
 * Return
 *      true or false
 *
 */
INT32 ossGetFSType ( const CHAR   *pFileName, OSS_FS_TYPE  *ossFSType );

/*
 * Rename a given path to a new name
 *
 * Input
 *      old path
 *      new path
 * Output
 *
 * Return
 *      rc
 *
 */
INT32 ossRenamePath ( const CHAR *pOldPath, const CHAR *pNewPath ) ;

/*
 * Lock or Unlock a file
 *
 * Input
 *    fileHandle
 *    OSS_FILE_LOCK
 * Output
 *
 * Return
 *    rc
 *
 */
INT32 ossLockFile ( OSSFILE *pFile, OSS_FILE_LOCK lockType ) ;

/*
 * Read specified length
 *
 * Input
 *   file descriptor (OSSFILE)
 *   length (integer)
 * OutPut
 *   buffer (char*)
 *   pBufferRead (integer ref)
 * Return
 *   rc
 * ps
 *   read may be less than len( because of eof ).
 */
INT32 ossReadN( OSSFILE *file,
                SINT64 len,
                CHAR *buf,
                SINT64 &read ) ;

/*
* Write specified length
* 
* Input
*   file descriptor (OSSFILE)
*   buffer (const char*)
*   length (integer)
* OutPut
*
* Return
*   rc
*/
INT32 ossWriteN( OSSFILE *file,
                 const CHAR *buf,
                 SINT64 len ) ;

INT32 ossSeekAndReadN( OSSFILE *file,
                       SINT64 offset,
                       SINT64 len,
                       CHAR *buf,
                       SINT64 &read ) ;

INT32 ossSeekAndWriteN( OSSFILE *file,
                        SINT64 offset,
                        const CHAR *pBufferWrite,
                        SINT64      iLenToWrite,
                        SINT64 &written ) ;

INT32 ossGetFileUserInfo( const CHAR *filename, OSSUID &uid, OSSGID &gid ) ;

INT32 ossGetUserInfo( const CHAR *username, OSSUID &uid, OSSGID &gid ) ;

INT32 ossGetUserInfo( OSSUID uid, CHAR *pUserName, UINT32 nameLen ) ;

#endif // OSSIO_HPP_

