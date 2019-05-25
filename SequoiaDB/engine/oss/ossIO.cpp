/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = ossIO.cpp

   Descriptive Name = Operating System Services IO

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for IO operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "ossPath.hpp"
#include "../util/text.h"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#include <iostream>

#if defined (_LINUX)
   #include <sys/statfs.h>
   #include <pwd.h>
   #include <unistd.h>

   #define  _ossGetCWD  getcwd
   #define  _ossChDir   chdir
   #define  _ossAccess  access
#endif

#ifdef _WINDOWS
   #include "io.h"
   #include <direct.h>

   #define _ossGetCWD   _getcwd
   #define _ossChDir    _chdir
   #define _ossAccess   _access
#endif

#define MAX_OPEN_RETRY     10
#define MAX_CLOSE_RETRY    5
#define MAX_DELFILE_RETRY  5

namespace fs = boost::filesystem ;

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
 *
 * Notice: when mode=OSS_READONLY, read & write mode is set in this function.
 *         it can cause block when the file is a pipe.
 *         set mode=OSS_READONLY | OSS_SHAREREAD, to make sure it's really
 *         read only!!
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSOPEN, "ossOpen" )
INT32 ossOpen(const CHAR   *pFileName ,
              UINT32       iMode ,
              UINT32       iPermission ,
              OSSFILE      &pFile )
{
   PD_TRACE_ENTRY ( SDB_OSSOPEN );
   UINT32 permission = iPermission ;
   UINT32 retryLoops = 0 ;
   UINT32 err        = 0 ;
   INT32  rc         = SDB_OK ;

   UINT32 mode       = 0;
   UINT32 direct     = 0 ;

   SDB_ASSERT ( pFileName , "pFileName is NULL" ) ;

#if defined (_WINDOWS)
   UINT32 creationDisposition = 0 ;
   UINT32 desiredAccess       = 0 ;
   UINT32 sharedMode          = 0 ;
   UINT32 attributes          = FILE_ATTRIBUTE_NORMAL ;
   WCHAR  FileNameUnicode[OSS_MAX_PATHSIZE+1] ;

   if( 0 == MultiByteToWideChar ( CP_ACP, 0,
                                  pFileName, -1,
                                  FileNameUnicode,
                                  ossStrlen ( pFileName ) + 1 ))
   {
      err = ossGetLastError () ;

      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to convert  file name to unicode: %s, Error: %d",
             pFileName, err ) ;
      rc = SDB_INVALIDARG ;
      goto error;
   }

   switch ( iMode & OSS_CREATE )
   {
   case OSS_DEFAULT: // just open the existing file
      creationDisposition = OPEN_EXISTING ;
      break ;
   case OSS_CREATEONLY: // create the file when not exist, otherwise fail
      creationDisposition = CREATE_NEW ;
      break ;
   case OSS_REPLACE: // create/truncate the file
      creationDisposition = CREATE_ALWAYS ;
      break ;
   case OSS_CREATE: // open the file, create it if not exist
      creationDisposition = OPEN_ALWAYS ;
      break ;
   }

   switch ( iMode & OSS_READWRITE )
   {
   case OSS_WRITEONLY: // write only mode
      if ( iMode & OSS_SHAREREAD )
      {
         desiredAccess = GENERIC_READ | GENERIC_WRITE ;
      }
      else
      {
         desiredAccess = GENERIC_WRITE ;
      }
      break ;

   case OSS_READONLY: // read only mode
      if ( ( iMode & OSS_SHAREREAD ) == OSS_EXCLUSIVE )
      {
         desiredAccess = GENERIC_READ | GENERIC_WRITE ;
      }
      else
      {
         desiredAccess = GENERIC_READ ;
      }
      break ;

   case OSS_READWRITE: // both read/write request
      desiredAccess = GENERIC_READ | GENERIC_WRITE ;
      break ;
   }

   switch ( iMode & OSS_SHAREWRITE )
   {
   case OSS_EXCLUSIVE:
      sharedMode = 0 ;
      break ;
   case OSS_SHAREREAD:
      if ( iMode & OSS_WRITEONLY )
      {
         sharedMode = FILE_SHARE_READ | FILE_SHARE_WRITE |
                      FILE_SHARE_DELETE ;
      }
      else
      {
         sharedMode = FILE_SHARE_READ ;
      }
      break ;
   case OSS_SHAREWRITE:
      sharedMode = FILE_SHARE_READ | FILE_SHARE_WRITE |
                   FILE_SHARE_DELETE ;
      break ;
   }

   if ( iMode & OSS_DIRECTIO )
   {
      attributes |= FILE_FLAG_NO_BUFFERING ;
   }

   if ( iMode & OSS_WRITETHROUGH )
   {
      attributes |= FILE_FLAG_WRITE_THROUGH ;
   }

   if ( 0 == iPermission )
   {
      permission = OSS_DEFAULTFILE ;
   }

   while ( TRUE )
   {
      retryLoops ++ ;

      pFile.hFile = CreateFile ( FileNameUnicode ,
                                 desiredAccess,
                                 sharedMode,
                                 NULL,
                                 creationDisposition,
                                 attributes,
                                 NULL );

      if ( INVALID_HANDLE_VALUE == pFile.hFile )
      {
         err = ossGetLastError () ;
         if ( ERROR_SHARING_VIOLATION == err && retryLoops < MAX_OPEN_RETRY )
         {
            ossSleep ( 100 ) ;
            continue ;
         }
         switch ( err )
         {
         case ERROR_SHARING_VIOLATION :
            rc = SDB_PERM ;
            break ;
         default :
            rc = SDB_IO ;
            break ;
         }
         pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
                "Failed to open file: %s, Error: %d", pFileName, err ) ;
         break;
      }
      break ;
   } // while (TRUE)
done:
   PD_TRACE_EXITRC ( SDB_OSSOPEN, rc );
   return rc;
error :
   goto done ;
#elif defined (_LINUX)
    switch ( iMode & OSS_CREATE )
    {
    case OSS_DEFAULT: // just open the existing file
       mode = 0 ;
       break ;
    case OSS_CREATEONLY: // create the file when not exist, otherwise fail
       mode = O_CREAT | O_EXCL ;
       break ;
    case OSS_REPLACE: // create/truncate the file
       mode = O_CREAT | O_TRUNC ;
       break ;
    case OSS_CREATE: // open the file, create it if not exist
       mode = O_CREAT ;
       break ;
    }

    switch ( iMode & OSS_READWRITE )
    {
    case OSS_WRITEONLY: // write only mode
       if ( iMode & OSS_SHAREREAD ) // write operation with shared read mode
       {
          mode |= O_RDWR ;
       }
       else
       {
          mode |= O_WRONLY ;
       }
       break ;

    case OSS_READONLY: // read only mode
       if (( iMode & OSS_SHAREREAD ) == OSS_EXCLUSIVE )
       {
          mode |= O_RDWR ;
       }
       else
       {
          mode |= O_RDONLY ;
       }
       break ;
    case OSS_READWRITE: // both read/write request
       mode |= O_RDWR ;
       break ;
    }

    if ( iMode & OSS_DIRECTIO )
    {
       direct = O_DIRECT ;
    }

    if ( iMode & OSS_WRITETHROUGH )
    {
       mode |= O_SYNC ;
    }

    if ( iPermission == 0 )
    {
       permission = OSS_DEFAULTFILE ;
    }

    while ( TRUE )
    {
       retryLoops ++ ;
       do
       {
          pFile.fd = open ( pFileName, mode | direct, permission ) ;
       } while ( (-1==pFile.fd) && (EINTR== ( err=ossGetLastError () )) ) ;

       if ( -1 == pFile.fd )
       {
          if( ( EINVAL == err ) && ( O_DIRECT == direct) )
          {
             direct = 0 ;
             continue ; // remove direct io bit, retry open the file again
          }
          else if (( ETXTBSY == err ) && ( retryLoops < MAX_OPEN_RETRY ))
          {
             ossSleep ( 100 ) ; // sleep for 0.1 seconds
             continue ;
          }
          pdLog ( PDERROR, __func__, __FILE__, __LINE__,
                  "Failed to open file: %s, errno: %d", pFileName, err ) ;
          switch ( err )
          {
          case ENOENT:
             rc = SDB_FNE ;
             break ;
          case EEXIST:
             rc = SDB_FE ;
             break ;
          case EACCES:
             rc = SDB_PERM ;
             break ;
          case EMFILE :
             rc = SDB_TOO_MANY_OPEN_FD ;
             break ;
          default:
             rc = SDB_IO ;
             break ;
          }
          break ; // break out loop
       } //if ( -1 == pFile.fd )
       break ;
    } // while(TRUE)
    PD_TRACE_EXITRC ( SDB_OSSOPEN, rc );
    return rc ;
#endif
}

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
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSCLOSE, "ossClose" )
INT32 ossClose(OSSFILE &pFile)
{
   INT32  rc        = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSCLOSE );
#if defined (_WINDOWS)
   BOOL fOK         = TRUE ;
   fOK = CloseHandle( (HANDLE) pFile.hFile );
   if ( !fOK )
   {
       rc = SDB_IO;
       goto error;
   }
   else
   {
      rc = SDB_OK ;
      pFile.hFile = 0 ;
   }
#elif defined (_LINUX)
   if( 0 == pFile.fd )
   {
      rc = SDB_OK;
      goto done;
   }
   if ( 0 != close ( pFile.fd ) )
   {
      rc = SDB_IO ;
      goto error ;
   }
   pFile.fd = 0 ;
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSCLOSE, rc );
   return rc ;
error :
   goto done ;
}

/*
 * Create a directory
 * Input
 * dir (string)
 * permission (integer)
 * Output
 * N/A
 * Return
 * SDB_OK
 * SDB_PERM
 * SDB_IO
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSMKDIR, "ossMkdir" )
INT32 ossMkdir ( const CHAR* pPathName, UINT32 iPermission )
{
   INT32   rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSMKDIR );
   SDB_ASSERT ( pPathName , "pPathName is NULL" ) ;
   try
   {
      fs::path dirpath ( pPathName ) ;
      fs::perms permission = fs::no_perms ;

      if ( !(iPermission & OSS_PERMALL) )
      {
         permission = fs::owner_all |
                      fs::group_read | fs::group_exe |
                      fs::others_read | fs::others_exe ;
      }
      else
      {
         if ( OSS_RU & iPermission )
         {
            permission |= fs::owner_read ;
         }
         if ( OSS_WU & iPermission )
         {
            permission |= fs::owner_write ;
         }
         if ( OSS_XU & iPermission )
         {
            permission |= fs::owner_exe ;
         }
         if ( OSS_RG & iPermission )
         {
            permission |= fs::group_read ;
         }
         if ( OSS_WG & iPermission )
         {
            permission |= fs::group_write ;
         }
         if ( OSS_XG & iPermission )
         {
            permission |= fs::group_exe ;
         }
         if ( OSS_RO & iPermission )
         {
            permission |= fs::others_read ;
         }
         if ( OSS_WO & iPermission )
         {
            permission |= fs::others_write ;
         }
         if ( OSS_XO & iPermission )
         {
            permission |= fs::others_exe ;
         }
      }

      if ( exists( dirpath ) )
      {
         rc = SDB_FE ;
         goto error ;
      }

      if ( fs::create_directories( dirpath ) )
      {
         rc = SDB_OK ;
      }
      else
      {
         SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                                  "Failed to create_directory(dirpath)" ) ;
      }

      fs::permissions( dirpath, permission ) ;
   }
   catch ( fs::filesystem_error& e )
   {
      if ( e.code() == boost::system::errc::permission_denied ||
           e.code() == boost::system::errc::operation_not_permitted )
      {
         rc = SDB_PERM ;
      }
      else
      {
         PD_LOG( PDERROR, "Create directory[%s] failed, errno: %d",
                 pPathName, e.code().value() ) ;
         rc = SDB_IO ;
      }
      goto error ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR, "Failed to make dir: %s", e.what() ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_OSSMKDIR, rc );
   return rc ;
error :
   goto done ;
}

INT32 ossGetCWD( CHAR *pPath, UINT32 maxSize )
{
   INT32 rc = SDB_OK ;

   if ( NULL == pPath || 0 == maxSize )
   {
      rc = SDB_INVALIDARG ;
   }
   else
   {
      if ( NULL == _ossGetCWD( pPath, maxSize ) )
      {
         INT32 err = errno ;
         PD_LOG ( PDINFO, "Failed to getcwd, errno: %d", err ) ;
         switch ( err )
         {
         case EINVAL:
         case ERANGE:
            rc = SDB_INVALIDARG ;
            break ;
         case EACCES:
         case EPERM:
            rc = SDB_PERM ;
            break ;
         default:
            rc = SDB_SYS ;
            break ;
         }
      }
   }

   return rc ;
}

INT32 ossChDir( const CHAR *pPath )
{
   INT32 rc = SDB_OK ;

   rc = ossAccess( pPath ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( 0 != _ossChDir( pPath ) )
   {
      INT32 err = errno ;
      PD_LOG( PDINFO, "Failed to chdir(%s), errno: %d", pPath, err ) ;
      switch ( err )
      {
      case ENOENT:
         rc = SDB_FNE ;
         break ;
      case ENOTDIR:
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EACCES:
      case EPERM:
         rc = SDB_PERM ;
         break ;
      case EIO:
         rc = SDB_IO ;
         break ;
      default:
         rc = SDB_SYS ;
         break ;
      }
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/*
 * Delete a directory
 * Input
 * dir (string)
 * Output
 * N/A
 * Return
 * SDB_OK (success)
 * SDB_PERM (permission denied)
 * SDB_FNE (dir not exist)
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDELETE, "ossDelete" )
INT32 ossDelete ( const CHAR *pPathName )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSDELETE );

   SDB_ASSERT ( pPathName , "pPathName is NULL" ) ;

   try
   {
      fs::path pathName ( pPathName ) ;

      if ( !exists( pathName ) )
      {
         rc = SDB_FNE ;
         goto error ;
      }

      if ( is_regular_file( pathName ) )       // is a file
      {
         if ( fs::remove_all( pathName ) )
         {
            rc = SDB_OK ;
         }
         else
         {
            SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                                     "Failed to remove_all(file)" ) ;
         }
      }
      else if (is_directory( pathName ) )      // is a directory
      {
         if ( fs::remove_all( pathName ) )
         {
            rc = SDB_OK ;
         }
         else
         {
            SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                                     "Failed to remove_all(dir)" ) ;
         }
      }
      else
      {
           SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALID_FILE_TYPE,
                                    "Invalid file type" ) ;
      }
   }
   catch ( fs::filesystem_error& e )
   {
      if ( e.code() == boost::system::errc::permission_denied )
         rc = SDB_PERM ;
      else
         rc = SDB_IO ;
      goto error ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR, "Failed to remove file or dir: %s", e.what() ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSDELETE, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSFILECOPY, "ossFileCopy" )
INT32 ossFileCopy( const CHAR * pSrcFile,
                   const CHAR * pDstFile,
                   UINT32 iPermission,
                   BOOLEAN isReplace )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSFILECOPY );

   OSSFILE srcFile ;
   OSSFILE dstFile ;
   BOOLEAN srcFileOpen = FALSE ;
   BOOLEAN dstFileOpen = FALSE ;
   UINT32 dstMode = OSS_CREATEONLY | OSS_READWRITE ;
   CHAR *pBuff = NULL ;
   UINT32 buffLen = 0 ;
   INT64 hasRead = 0 ;

   if ( !pSrcFile || !pDstFile )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pBuff = (CHAR*)SDB_OSS_MALLOC( 4096 ) ;
   if ( !pBuff )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   buffLen = 4096 ;
   ossMemset( pBuff, 0, buffLen ) ;

   rc = ossOpen( pSrcFile, OSS_READONLY | OSS_SHAREREAD,
                 OSS_DEFAULTFILE, srcFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Open source file[%s] failed, rc: %d", pSrcFile, rc ) ;
      goto error ;
   }
   srcFileOpen = TRUE ;

   if ( isReplace )
   {
      dstMode = OSS_REPLACE | OSS_READWRITE ;
   }
   rc = ossOpen( pDstFile, dstMode, iPermission, dstFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Open dest file[%s] failed, rc: %d", pDstFile, rc ) ;
      goto error ;
   }
   dstFileOpen = TRUE ;

   while ( TRUE )
   {
      rc = ossReadN( &srcFile, buffLen, pBuff, hasRead ) ;
      if ( SDB_EOF == rc || 0 == hasRead )
      {
         rc = SDB_OK ;
         break ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Read file[%s] failed, rc: %d", pSrcFile, rc ) ;
         goto error ;
      }
      rc = ossWriteN( &dstFile, pBuff, hasRead ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Write file[%s] failed, rc: %d", pDstFile, rc ) ;
         goto error ;
      }
   }

done:
   if ( pBuff )
   {
      SDB_OSS_FREE( pBuff ) ;
   }
   if ( srcFileOpen )
   {
      ossClose( srcFile ) ;
   }
   if ( dstFileOpen )
   {
      ossClose( dstFile ) ;
   }
   PD_TRACE_EXITRC ( SDB_OSSFILECOPY, rc );
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSACCESS, "ossAccess" )
INT32 ossAccess ( const CHAR *pPathName, int flags )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSACCESS );

   if ( NULL == pPathName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 0 != _ossAccess( pPathName, flags ) )
   {
      INT32 err = errno ;
      switch ( err )
      {
      case ENOENT:
         rc = SDB_FNE ;
         break ;
      case EACCES:
      case EPERM:
         rc = SDB_PERM ;
         break ;
      case ENOTDIR:
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EIO:
         rc = SDB_IO ;
         break ;
      default :
         rc = SDB_SYS ;
         break ;
      }
      goto error ;
   }

done:
   PD_TRACE_EXITRC ( SDB_OSSACCESS, rc );
   return rc ;
error:
   goto done ;
}

/*
 * Read a file from current file descriptor location
 * Input
 *    file descriptor (OSSFILE)
 *    buffer (char*)
 *    length to read (integer)
 *    length read (integer*)
 * Output
 *    pBufferRead (integer pointer)
 *    pLenRead (actual read length)
 * Return
 *    Success: read length (>=0)
 *    Failed:
 *       SDB_INVALIDARG (invalid parameter passed in)
 *       SDB_INVALIDSIZE (read size is not valid)
 *       SDB_PERM (not open for reading)
 *       SDB_IO (IO error)
 *       SDB_OOM (out of memory)
 *       SDB_EOF (hit end of the file)
 *       SDB_INTERRUPT (interrupt received)
 *       SDB_UNKNOWN(other unknown error)
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSREAD, "ossRead" )
INT32 ossRead( OSSFILE* pFile,
               CHAR*    pBufferRead,
               SINT64   iLenToRead,
               SINT64*  pLenRead )
{
   INT32   rc       = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSREAD );
   UINT32  err      = 0 ;
   SINT64  readSize = 0 ;
#if defined (_WINDOWS)
   DWORD   readBytes = 0 ;
#endif
   SDB_ASSERT ( pFile       , "pFile is NULL" ) ;
   SDB_ASSERT ( pBufferRead , "pBufferRead is NULL" ) ;
   SDB_ASSERT ( pLenRead    , "pLenRead is NULL" ) ;

   if ( 0 == iLenToRead )
      goto done ;
   *pLenRead = 0;

   /* validate parameters */
   if ( 0 >= iLenToRead )
   {
       SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDSIZE,
                                "Invalid iLenToRead" ) ;
   }
#if defined (_WINDOWS)
   err = ReadFile( (HANDLE)pFile->hFile,
                   pBufferRead,
                  (DWORD)iLenToRead,
                  &readBytes,
                   NULL ) ;
   if ( !err )
   {
      err = ossGetLastError() ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to ReadFile() : %x, Error: %d",
             pFile->hFile, err ) ;
      switch ( err )
      {
      case ERROR_HANDLE_EOF:
        rc = SDB_EOF ;
        break ;
     case ERROR_NOT_ENOUGH_MEMORY:
         rc = SDB_OOM ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
     }
   }
   if ( 0 == readBytes )
      rc = SDB_EOF ;
   if ( pLenRead )
      *pLenRead = readBytes ;
#elif defined (_LINUX)
   readSize = read ( pFile->fd, pBufferRead, iLenToRead ) ;
   if( -1 == readSize )
   {
      err = ossGetLastError() ;
      PD_LOG ( PDERROR, "Failed to read() : %x, Error: %d", pFile->fd, err ) ;
      switch ( err )
      {
      case EINTR:
         rc = SDB_INTERRUPT ;
         break ;
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case ENOMEM:
         rc = SDB_OOM ;
         break ;
      case EBADF:
         rc = SDB_PERM ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else if ( 0 == readSize )
   {
      rc = SDB_EOF ;
   }
   else
   {
      (*pLenRead) = readSize ;
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSREAD, rc );
   return rc ;
error :
   goto done ;
}

/**
 * Write a file at current file descriptor location
 * Input
 *    file descriptor (OSSFILE)
 *    buffer (char*)
 *    length to write (integer)
 *    length written (integer*)
 * Output
 *    pLenWritten (length actually write)
 * Return
 *    Success: write length (>=0)
 *    Failed:
 *      SDB_INVALIDARG (invalid parameter passed in)
 *      SDB_INVALIDSIZE (write size is not valid)
 *      SDB_PERM (not open for write permission)
 *      SDB_IO (IO error)
 *      SDB_INTERRUPT (interrupt received)
 **/
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSWRITE, "ossWrite" )
INT32 ossWrite( OSSFILE  *pFile,
                const CHAR     *pBufferWrite,
                SINT64   iLenToWrite,
                SINT64   *pLenWritten )
{
   INT32   rc        = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSWRITE );
   UINT32  err       = 0 ;
   SINT64  writeSize = 0 ;
#if defined (_WINDOWS)
   DWORD writeBytes = 0 ;
#endif
   SDB_ASSERT ( pFile         , "pFile is NULL" ) ;
   SDB_ASSERT ( pBufferWrite  , "pBufferWrite is NULL" ) ;
   SDB_ASSERT ( pLenWritten   , "pLenWritten is NULL" ) ;
   if ( 0 == iLenToWrite )
      goto done ;
   /* validate parameters */
   if ( 0 > iLenToWrite )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDSIZE,
                               "Invalid iLenToWrite" ) ;
   }
#if defined (_WINDOWS)
   SDB_ASSERT ( INVALID_HANDLE_VALUE != pFile->hFile,
                "pFile->hFile is invalid" ) ;

   err = WriteFile( (HANDLE) pFile->hFile,
                    pBufferWrite,
                    (DWORD)iLenToWrite,
                    &writeBytes,
                     NULL) ;
   if ( !err )
   {
      err = ossGetLastError();
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to WriteFile() : %x, Error: %d",
             pFile->hFile, err ) ;

      switch (err)
      {
      case ERROR_HANDLE_EOF:
         rc = SDB_EOF ;
         break ;
      case ERROR_NOT_ENOUGH_MEMORY:
         rc = SDB_OOM ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   if ( pLenWritten )
      *pLenWritten = writeBytes ;
#elif defined (_LINUX)
   SDB_ASSERT ( pFile->fd, "pFile->fd is NULL" ) ;

   writeSize = write ( pFile->fd, pBufferWrite, iLenToWrite ) ;
   if ( -1 == writeSize )
   {
      (*pLenWritten) = 0 ;
      err = ossGetLastError() ;
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Failed to write() : %x, Error: %d",
              pFile->fd, err ) ;
      switch ( err )
      {
      case EINTR:
         rc = SDB_INTERRUPT ;
         break ;
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EBADF:
         rc = SDB_PERM ;
         break ;
      case ENOSPC :
         rc = SDB_NOSPC ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else
   {
      (*pLenWritten) = writeSize ;
   }
#endif

done :
   PD_TRACE_EXITRC ( SDB_OSSWRITE, rc );
   return rc ;
error :
   goto done ;
}

/*
 * Replisition read/write file offset
 * Input
 *    File descriptor (OSSFILE)
 *    offset (off_t)
 *    whence (OSS_SEEK)
 * Output
 *    N/A
 * Return
 *    SDB_OK (success)
 *    SDB_INVALIDSIZE (seek location is not valid)
 *    SDB_INVALIDARGS (bad arguments)
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSEEK, "ossSeek" )
INT32 ossSeek ( OSSFILE  *pFile,
                INT64 offset,
                OSS_SEEK whence,
                INT64* position )
{
   INT32   rc      = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSEEK );

   SDB_ASSERT ( pFile , "pFile is NULL" ) ;

#if defined (_LINUX)
   INT64 seekOff = 0 ;
   seekOff = lseek ( pFile->fd, (INT64)offset, whence ) ;
   if ( -1 == seekOff )
   {
       INT32 err = ossGetLastError () ;
       pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
               "Failed to lseek() : %x, Error: %d",
               pFile->fd, err ) ;
       switch ( err )
       {
       case EINTR:
          rc = SDB_INTERRUPT ;
          break ;
       case EINVAL:
          rc = SDB_INVALIDARG ;
          break ;
       case EBADF:
          rc = SDB_PERM ;
          break ;
       default:
          rc = SDB_IO ;
          break ;
       }
       goto error ;
   }

   if ( NULL != position )
   {
      *position = seekOff ;
   }
#elif defined (_WINDOWS)
   LARGE_INTEGER li ;
   LARGE_INTEGER result ;
   BOOL ret ;
   li.QuadPart = offset ;
   result.QuadPart = 0 ;

   ret = SetFilePointerEx( (HANDLE) pFile->hFile,
                           li,
                           &result,
                           (DWORD)whence ) ;
   if ( !ret )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
               "Failed to SetFilePointerEx() : %x, Error: %d",
               pFile->hFile, ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }

   if ( NULL != position )
   {
      *position = (INT64)result.QuadPart ;
   }
#endif

done :
   PD_TRACE_EXITRC ( SDB_OSSSEEK, rc );
   return rc;
error :
   goto done ;
}

/**
 * Read a file from current file descriptor location
 * at offset from the start of the file at
 * Input
 *    file descriptor (OSSFILE)
 *    offset (off_t)
 *    buffer (char*)
 *    length to read (integer)
 *    length read (integer*)
 * Output
 *    N/A
 * Return
 *    Success: read length (>=0)
 * Failed:
 *    SDB_INVALIDARG (invalid parameter passed in)
 *    SDB_INVALIDSIZE (read size is not valid)
 *    SDB_IO (IO error)
 *    SDB_INTERRUPT (interrupt received)
 **/
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSEEKANDREAD, "ossSeekAndRead" )
INT32 ossSeekAndRead( OSSFILE   *pFile,
                      INT64     offset,
                      CHAR      *pBufferRead,
                      SINT64    iLenToRead,
                      SINT64   *pLenRead )
{
   INT32   rc       = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSEEKANDREAD );
   INT32   err      = 0 ;
   SINT64  readSize = 0 ;
#if defined (_WINDOWS)
   LARGE_INTEGER li ;
#endif
   SDB_ASSERT ( pFile        , "pFile is NULL" ) ;
   SDB_ASSERT ( pBufferRead  , "pBufferRead is NULL" ) ;
   SDB_ASSERT ( pLenRead     , "pLenRead is NULL" ) ;

   if ( 0 == iLenToRead )
      goto done ;
   /* validate parameters */
   if ( ( 0 > offset ) || ( 0 > iLenToRead ) )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDSIZE,
                               "Failed to SetFilePointer()" ) ;
   }
#if defined (_WINDOWS)
   {
      OSSFILE_XLOCK
      DWORD readBytes = 0 ;
      li.QuadPart = offset ;
      li.LowPart = SetFilePointer ( (HANDLE) pFile->hFile,
                                    li.LowPart,
                                    &li.HighPart,
                                    FILE_BEGIN ) ;

      if ( li.LowPart == INVALID_SET_FILE_POINTER &&
           NO_ERROR != ossGetLastError () )
      {
         SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                                  "Failed to SetFilePointer()" ) ;
      }

      if ( SDB_OK == rc )
      {
         err = ReadFile ( (HANDLE)pFile->hFile,
                           pBufferRead,
                          (DWORD)iLenToRead,
                          &readBytes,
                           NULL ) ;
      }

      if ( !err )
      {
         err = ossGetLastError() ;

         pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
                      "Failed to ReadFile() : %x, Error: %d",
                      pFile->hFile, err ) ;

         switch ( err )
         {
         case ERROR_HANDLE_EOF:
            rc = SDB_EOF ;
            break ;
         case ERROR_NOT_ENOUGH_MEMORY:
            rc = SDB_OOM ;
            break ;
         default:
            rc = SDB_IO ;
            break ;
         }
      }
      if ( 0 == readBytes )
         rc = SDB_EOF ;
      if ( pLenRead )
         *pLenRead = readBytes ;
   }
#elif defined (_LINUX)
   readSize = pread ( pFile->fd, pBufferRead, iLenToRead, offset ) ;
   if ( -1 == readSize )
   {
      err = ossGetLastError() ;
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Failed to pread() : %x, Error: %d",
              pFile->fd, err ) ;
      switch ( err )
      {
      case EINTR:
         rc = SDB_INTERRUPT ;
         break ;
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case ENOMEM:
         rc = SDB_OOM ;
         break ;
      case EBADF:
         rc = SDB_PERM ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else if( 0 == readSize )
   {
      rc = SDB_EOF ;
   }
   else
   {
      (*pLenRead) = readSize ;
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSSEEKANDREAD, rc );
   return rc ;
error :
   goto done ;

 }

/*
 * Write a file from current file descriptor location
 * Input
 *    file descriptor (OSSFILE)
 *    offset (off_t)
 *    buffer (char*)
 *    length to write (integer)
 *    length written (integer*)
 * Output
 *    N/A
 * Return
 *    Success: write length (>=0)
 *    Failed:
 *       SDB_INVALIDARG (invalid parameter passed in)
 *       SDB_INVALIDSIZE (write size is not valid)
 *       SDB_IO (IO error)
 *       SDB_INTERRUPT (interrupt received)
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSEEKANDWRITE, "ossSeekAndWrite" )
INT32 ossSeekAndWrite ( OSSFILE    *pFile,
                        INT64       offset,
                        const CHAR *pBufferWrite,
                        SINT64      iLenToWrite,
                        SINT64     *pLenWritten )
{
   INT32   rc        = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSEEKANDWRITE );
   UINT32  err       = 0 ;
   SINT64  writeSize = 0 ;

#if defined (_WINDOWS)
   LARGE_INTEGER li;
#endif

   SDB_ASSERT ( pFile        , "pFile is NULL" ) ;
   SDB_ASSERT ( pBufferWrite , "pBufferWrite is NULL" ) ;
   SDB_ASSERT ( pLenWritten  , "pLenWritten is NULL" ) ;
   if ( 0 == iLenToWrite )
      goto done ;
   /* validate parameters */
   if ( (0 > offset) || (0 > iLenToWrite) )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDSIZE,
                               "Invalid parameter, offset or iLenToWrite" ) ;
   }

#if defined (_WINDOWS)
   {
      OSSFILE_XLOCK
      DWORD writeBytes = 0 ;
      li.QuadPart = offset ;
      li.LowPart  = SetFilePointer ( (HANDLE) pFile->hFile,
                                      li.LowPart,
                                      &li.HighPart,
                                      FILE_BEGIN ) ;
      if ( li.LowPart == INVALID_SET_FILE_POINTER &&
           NO_ERROR != ossGetLastError () )
      {
         SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                                  "Failed to SetFilePointer()" ) ;
      }
      if ( 0 == rc )
      {
         err = WriteFile ( (HANDLE)pFile->hFile,
                           pBufferWrite,
                           (DWORD)iLenToWrite,
                           &writeBytes,
                            NULL ) ;
      }

      if ( !err )
      {
         err = ossGetLastError() ;

         pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
                "Failed to pread() : %x, Error: %d",
                pFile->hFile, err ) ;

         switch (err)
         {
         case ERROR_HANDLE_EOF:
            rc = SDB_EOF ;
            break ;
         case ERROR_NOT_ENOUGH_MEMORY:
            rc = SDB_OOM ;
            break ;
         default:
            rc = SDB_IO ;
            break ;
         }
      }
      if ( pLenWritten )
         *pLenWritten = writeBytes ;
   }
#elif defined (_LINUX)
   writeSize = pwrite ( pFile->fd, pBufferWrite, iLenToWrite, offset ) ;
   if( -1 == writeSize )
   {
      err = ossGetLastError () ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to pwrite() : %x, Error: %d",
             pFile->fd, err ) ;
      switch ( err )
      {
      case EINTR:
         rc = SDB_INTERRUPT ;
         break ;
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case ENOMEM:
         rc = SDB_OOM ;
         break;
     case ENOSPC :
         rc = SDB_NOSPC ;
         break ;
      case EBADF:
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else
   {
      (*pLenWritten) = writeSize ;
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSSEEKANDWRITE, rc );
   return rc ;
error :
   goto done ;
}

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
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSFSYNC, "ossFsync" )
INT32 ossFsync( OSSFILE* pFile )
{
   INT32   rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSFSYNC );
   UINT32  err = 0 ;

   SDB_ASSERT ( pFile , "pFile is NULL" ) ;

#if defined (_WINDOWS)
   BOOL   fOk = TRUE ;
   fOk =  FlushFileBuffers( (HANDLE) pFile->hFile ) ;
   if ( !fOk )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                               "Failed to FlushFileBuffers()" ) ;
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSFSYNC, rc );
   return rc ;
error :
   goto done ;
#elif defined (_LINUX)
   rc = fsync ( pFile->fd ) ;
   if( rc )
   {
      err = ossGetLastError () ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to pwrite() : %x, Error: %d",
             pFile->fd, err ) ;
      switch ( err )
      {
      case EROFS:
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EBADF:
      case EIO:
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   PD_TRACE_EXITRC ( SDB_OSSFSYNC, rc );
   return rc ;
#endif
}

/*
 * Type of a given path
 * Input
 *      path (string)
 * Output
 *      N/A
 * Return
 *      type (integer, when >=0)
 *      SDB_IO (IO error)
 *      SDB_FNE (Path not exist)
 *      SDB_PERM (permission error)
 *      SDB_INVALIDARG (invalid path)
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETPATHTYPE, "ossGetPathType" )
INT32 ossGetPathType ( const CHAR  *pPath, SDB_OSS_FILETYPE *pFileType )
{
   INT32 rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETPATHTYPE );
   INT32 err = 0 ;
#if defined (_WINDOWS)
   WCHAR FileNameUnicode[OSS_MAX_PATHSIZE+1] ;
   DWORD dwAttrs ;
#elif defined (_LINUX)
   struct stat sb ;
#endif

   SDB_ASSERT ( pPath     , "pPath is NULL" ) ;
   SDB_ASSERT ( pFileType , "pFileType is NULL" ) ;

#if defined (_LINUX)
   if ( 0 == ossStrncmp ( SDB_DEV_NULL, pPath, ossStrlen(SDB_DEV_NULL) ) )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDSIZE,
                               "Invalid pPath, /dev/null" ) ;
   }
#endif

#if defined (_WINDOWS)
   if ( 0 == MultiByteToWideChar ( CP_ACP, 0, pPath, -1,
                                   FileNameUnicode,
                                   ossStrlen( pPath ) + 1 ) )
   {
      err = ossGetLastError () ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to convert  file name to unicode: %s, Error: %d",
             pPath, err ) ;

      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDARG,
                               "Failed to MultiByteToWideChar()" ) ;
   }

   if (  INVALID_FILE_ATTRIBUTES == GetFileAttributes( FileNameUnicode )
                        && ERROR_FILE_NOT_FOUND ==  ossGetLastError () )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_FNE,
                               "File doesn't exist" ) ;
   }

   dwAttrs = GetFileAttributes( FileNameUnicode );
   if  ( dwAttrs & FILE_ATTRIBUTE_ARCHIVE )
   {
      (*pFileType) = SDB_OSS_FIL ;
   }
   else if ( dwAttrs & FILE_ATTRIBUTE_DIRECTORY )
   {
      (*pFileType) = SDB_OSS_DIR ;
   }
   else
   {
      err = ossGetLastError () ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to GetFileAttributes: %s, dwAttrs: %d, Error: %d",
             FileNameUnicode, dwAttrs, err ) ;

      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                               "Failed to GetFileAttributes()" ) ;
   }

#elif defined (_LINUX)

   err = stat ( pPath, &sb ) ;
   if ( -1 == err )
   {
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to stat  file name : %s, Error: %d",
             pPath, err ) ;

      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDARG,
                               "Failed to stat()" ) ;
   }
   else
   {
      switch (sb.st_mode & S_IFMT)
      {
      case S_IFBLK:
      case S_IFCHR:
         (*pFileType) = SDB_OSS_DEV ;
         break ;
      case S_IFDIR:
         (*pFileType) = SDB_OSS_DIR ;
         break ;
      case S_IFIFO:
         (*pFileType) = SDB_OSS_PIP ;
         break ;
      case S_IFLNK:
         (*pFileType) = SDB_OSS_SYM ;
         break ;
      case S_IFREG:
         (*pFileType) = SDB_OSS_FIL ;
         break ;
      case S_IFSOCK:
         (*pFileType) = SDB_OSS_SCK ;
         break ;
      default:
         (*pFileType) = SDB_OSS_UNK ;
         break ;
      }
  }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSGETPATHTYPE, rc );
   return rc ;
error :
   (*pFileType) = SDB_OSS_UNK ;
   goto done ;
}

/*
 * Get file size
 * Input
 * Input
 *      path (string)
 *
 * Output
 *      N/A
 * Return
 *      size (size_t, when >=0)
 *      SDB_IO (IO error)
 *      SDB_FNE (Path not exist)
 *      SDB_PERM (permission error)
 *      SDB_INVALIDARG (invalid path)
 */
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETFSBYNM, "ossGetFileSizeByName" )
INT32 ossGetFileSizeByName ( const CHAR* pFileName, INT64 *pFileSize )
{
   INT32    rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETFSBYNM );
   INT32    err = 0 ;

#if defined (_WINDOWS)
   BOOL     fOk = TRUE ;
   WIN32_FILE_ATTRIBUTE_DATA   fileInfo ;
   WCHAR FileNameUnicode[OSS_MAX_PATHSIZE+1] ;
#elif defined (_LINUX)
   struct stat sb;
#endif
   SDB_ASSERT ( pFileName , "pFileName is NULL" ) ;
   SDB_ASSERT ( pFileSize , "pFileSize is NULL" ) ;

#if defined (_WINDOWS)

   if ( 0 == MultiByteToWideChar ( CP_ACP, 0,
                                   pFileName, -1,
                                   FileNameUnicode,
                                   ossStrlen( pFileName ) + 1 ))
   {
      err = ossGetLastError () ;

      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to convert  file name to unicode: %s, Error: %d",
             pFileName, err ) ;
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_INVALIDARG,
                               "Failed to MultiByteToWideChar()" ) ;
   }

   if (  INVALID_FILE_ATTRIBUTES == GetFileAttributes( FileNameUnicode )
                        && ERROR_FILE_NOT_FOUND ==  ossGetLastError () )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_FNE,
                               "File doesn't exist" ) ;
   }

   fOk = GetFileAttributesEx( FileNameUnicode,
                              GetFileExInfoStandard,
                              (void*)&fileInfo ) ;

   SDB_VALIDATE_GOTOERROR ( fOk, SDB_IO,
                            "Failed to GetFileAttributesEx()" ) ;

   (*pFileSize) = ( (INT64)fileInfo.nFileSizeHigh << 32 ) +
                    fileInfo.nFileSizeLow ;
done :
   PD_TRACE_EXITRC ( SDB_OSSGETFSBYNM, rc );
   return rc ;
error :
   goto done ;
#elif defined (_LINUX)
   rc = stat ( pFileName, &sb ) ;
   if ( -1 == rc )
   {
      err = ossGetLastError() ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to stat() : %s, Error: %d",
             pFileName, err ) ;
      switch(err)
      {
      case EACCES:
         rc = SDB_PERM ;
         break ;
      case ENOENT:
         rc = SDB_FNE ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else
   {
      if( !(sb.st_mode & S_IFREG) )
      {
         PD_LOG ( PDERROR,
                  "File mode is not regular: %d: %s",
                  sb.st_mode, pFileName ) ;
         return SDB_INVALID_FILE_TYPE ;
      }
      (*pFileSize) = sb.st_size ;
   }
   PD_TRACE_EXITRC ( SDB_OSSGETFSBYNM, rc );
   return rc ;
#endif
}

/*
 * Get file size
 * Input
 * Input
 *      file descriptor
 *
 * Output
 *      pfsize
 * Return
 *      SDB_OK
 *      SDB_IO (IO error)
 *      SDB_INVALIDARG (invalid input arguments)
 *      SDB_INVALID_FILE_TYPE (invalid input arguments)
 */
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETFILESIZE, "ossGetFileSize" )
INT32 ossGetFileSize ( OSSFILE *pFile, INT64 *pfsize )
{
   INT32    rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETFILESIZE );
   INT32    err = 0 ;

#if defined (_WINDOWS)
   LARGE_INTEGER li ;
#elif defined (_LINUX)
   struct stat sb ;
#endif
   SDB_ASSERT ( pFile , "input file is NULL" ) ;
   SDB_ASSERT ( pfsize, "output size buffer is NULL" ) ;

#if defined (_WINDOWS)

   if ( !GetFileSizeEx ( pFile->hFile, &li ) )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_IO,
                               "Failed to GetFileSizeEx()" ) ;
   }
   *pfsize = li.QuadPart ;
done:
   PD_TRACE_EXITRC ( SDB_OSSGETFILESIZE, rc );
   return rc ;
error :
   goto done ;
#elif defined (_LINUX)
   rc = fstat ( pFile->fd, &sb ) ;
   if ( -1 == rc )
   {
      err = ossGetLastError () ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to fstat() : %x, Error: %d",
             pFile->fd, err ) ;
      switch ( err )
      {
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EBADF:
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else
   {
      switch ( sb.st_mode & S_IFMT )
      {
      case S_IFLNK:
      case S_IFREG:
         (*pfsize) = (INT64) sb.st_size ;
          break ;
      case S_IFCHR:
      case S_IFBLK:
      case S_IFDIR:
      case S_IFIFO:
      case S_IFSOCK:
      default:
         rc = SDB_INVALID_FILE_TYPE ;
         break ;
      }
   }
   PD_TRACE_EXITRC ( SDB_OSSGETFILESIZE, rc );
   return rc ;
#endif
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSTRUNCATEFILE, "ossTruncateFile" )
INT32 ossTruncateFile ( OSSFILE *pFile, const INT64 fileLen )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSTRUNCATEFILE ) ;
   SDB_ASSERT ( pFile, "input file is NULL" ) ;
#if defined (_LINUX)
   rc = ftruncate ( pFile->fd, fileLen ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to truncate file to %lld bytes, errno = %d",
               fileLen, ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }
   else
   {
      rc = SDB_OK ;
   }
#elif defined (_WINDOWS)
   rc = ossSeek ( pFile, fileLen, OSS_SEEK_SET ) ;
   PD_RC_CHECK ( rc, PDERROR, "Failed to seek to offset %lld, errno = %d",
                 fileLen, ossGetLastError() ) ;
   rc = SetEndOfFile ( (HANDLE)pFile->hFile ) ;
   if ( 0 == rc )
   {
      PD_LOG ( PDERROR, "Failed to truncate file to offset %lld, errno = %d",
               fileLen, ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }
   else
   {
      rc = SDB_OK ;
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSTRUNCATEFILE, rc ) ;
   return rc ;
error :
   goto done ;
}

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
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSEXTFILE, "ossExtendFile" )
INT32 ossExtendFile ( OSSFILE *pFile,
                      const INT64 incrementSize )
{
#ifdef  OSS_EXTEND_DELTA
#undef  OSS_EXTEND_DELTA
#endif
#define OSS_EXTEND_DELTA      (4194304)      // 4MB

   INT32    rc         = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSEXTFILE );
   INT32    loop       = 0 ;
   INT32    remainder  = 0 ;
   CHAR    *pBuffer    = NULL ;

   SDB_ASSERT ( pFile, "input file is NULL" ) ;

   rc = ossSeek( pFile, 0, OSS_SEEK_END ) ;

   SDB_VALIDATE_GOTOERROR ( SDB_OK == rc, rc,
                            "Failed to seek to end of file" ) ;

   loop       = incrementSize / ( OSS_EXTEND_DELTA ) ;
   remainder  = incrementSize % ( OSS_EXTEND_DELTA ) ;
   pBuffer    = (CHAR*) SDB_OSS_MALLOC ( OSS_EXTEND_DELTA ) ;

   SDB_VALIDATE_GOTOERROR ( pBuffer, SDB_OOM,
                            "Failed to allocate memory" ) ;
   ossMemset( pBuffer, 0, OSS_EXTEND_DELTA ) ;

   for ( INT32 i = 0; i < loop ; i++ )
   {
      rc = ossWriteN ( pFile, pBuffer, OSS_EXTEND_DELTA ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to extend file, errno = %d",
                    ossGetLastError() ) ;
   }
#undef  OSS_EXTEND_DELTA
   if ( 0 != remainder )
   {
      rc = ossWriteN( pFile, pBuffer, remainder ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to extend file, errno = %d",
                    ossGetLastError() ) ;
   }
done :
   if ( pBuffer )
   {
      SDB_OSS_FREE ( pBuffer ) ;
   }
   PD_TRACE_EXITRC ( SDB_OSSEXTFILE, rc );
   return rc;
error :
   goto done ;
}

INT32 ossExtentBySparse( OSSFILE *pFile,
                         UINT64 incrementSize,
                         UINT32 onceWrite )
{
   INT32    rc         = SDB_OK ;
   CHAR    *pBuffer    = NULL ;

   SDB_ASSERT ( pFile, "input file is NULL" ) ;

   if ( onceWrite > incrementSize )
   {
      onceWrite = incrementSize ;
   }

   rc = ossSeek( pFile, 0, OSS_SEEK_END ) ;

   SDB_VALIDATE_GOTOERROR ( SDB_OK == rc, rc,
                            "Failed to seek to end of file" ) ;

   rc = ossSeek( pFile, incrementSize - onceWrite, OSS_SEEK_CUR ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to seek offset[%lld](OSS_SEEK_CUR), rc:%d",
              incrementSize - onceWrite, rc ) ;
      goto error ;
   }

   pBuffer = ( CHAR* )SDB_OSS_MALLOC( onceWrite ) ;
   if ( !pBuffer )
   {
      PD_LOG( PDERROR, "Alloc memory[%d] failed", onceWrite ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pBuffer, 0 , onceWrite ) ;

   rc = ossWriteN( pFile, pBuffer, onceWrite ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to write file, rc: %d", rc ) ;
      goto error ;
   }

done :
   if ( pBuffer )
   {
      SDB_OSS_FREE ( pBuffer ) ;
   }
   return rc;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETREALPATH, "ossGetRealPath" )
CHAR  *ossGetRealPath( const CHAR  *pPath,
                       CHAR        *resolvedPath,
                       UINT32       length )
{
   PD_TRACE_ENTRY ( SDB_OSSGETREALPATH );
   SDB_ASSERT ( pPath , "pPath is NULL" ) ;
   SDB_ASSERT ( resolvedPath, "resolvedPath is NULL" ) ;
   CHAR pathBuffer [ OSS_MAX_PATHSIZE + 1 ] = {'\0'} ;
   CHAR tempBuffer [ OSS_MAX_PATHSIZE + 1 ] = {'\0'} ;
   ossStrncpy ( pathBuffer, pPath, sizeof(pathBuffer) ) ;
   CHAR *ret = NULL ;
   CHAR *pPos = NULL ;

   resolvedPath[ 0 ] = '\0' ;

   while ( TRUE )
   {
      if ( '\0' == pathBuffer[0] )
      {
         ret = pathBuffer ;
         break ;
      }
      ret =
#if defined (_LINUX)
         realpath ( pathBuffer, tempBuffer ) ;
#elif defined (_WINDOWS)
         _fullpath ( tempBuffer, pathBuffer, sizeof(tempBuffer) ) ;
#endif
      if ( ret )
      {
         ossStrncpy ( resolvedPath, tempBuffer, length ) ;
         break ;
      }
      CHAR *pNewPos = ossStrrchr ( pathBuffer, OSS_FILE_SEP_CHAR ) ;
      if ( pNewPos )
      {
         *pNewPos = '\0' ;
      }
      else
      {
         ossStrncpy( resolvedPath, tempBuffer, length ) ;
         ret = pathBuffer ;
         break ;
      }
      if ( pPos )
      {
         *pPos = OSS_FILE_SEP_CHAR ;
      }
      pPos = pNewPos ;
   }
   if ( ret && pPos )
   {
      *pPos = OSS_FILE_SEP_CHAR ;
      ossStrncat ( resolvedPath, pPos, length - ossStrlen(resolvedPath) ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSGETREALPATH );
   return ret ? resolvedPath: NULL ;
}


/*
 * Verify the supported file system type
 * and supported file block size
 *
 * Input
 *      one file name (with full path on WINDOWS)
 *
 * Output
 *
 * Return
 *      true or false
 *
 */
 // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETFSTYPE, "ossGetFSType" )
INT32 ossGetFSType ( const CHAR  *pFileName, OSS_FS_TYPE  *ossFSType )
{
   INT32   rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETFSTYPE );
   INT32   err = 0 ;

#if defined (_WINDOWS)
   WCHAR volumeName[OSS_MAX_PATHSIZE + 1]     = { 0 } ;
   WCHAR fileSystemName[OSS_MAX_PATHSIZE + 1] = { 0 } ;
   WCHAR FileNameUnicode[OSS_MAX_PATHSIZE+1]  = { 0 } ;
   DWORD serialNumber                         = 0 ;
   DWORD maxComponentLen                      = 0 ;
   DWORD fileSystemFlags                      = 0 ;
   CHAR  NewFileName[OSS_MAX_PATHSIZE + 1]    = { 0 } ;
   WCHAR NTFSName[OSS_MAX_PATHSIZE + 1]       = { 0 } ;
#elif defined (_LINUX)
   struct stat   sb ;
   struct statfs sfs ;
#endif
    SDB_ASSERT ( pFileName, "pFileName is NULL" ) ;

#if defined (_WINDOWS)

   if ( (pFileName[1] != ':') )
   {
       pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
                "Failed to find  the root directory in : %s, Error: %d",
                pFileName, err ) ;
       goto error;
   }

   if ( pFileName[1] == ':' )
   {
       ossStrncpy(NewFileName,pFileName,2);
       ossStrncat(NewFileName,"\\",1);
   }

   if( 0 == MultiByteToWideChar ( CP_ACP, 0,
                                  NewFileName, -1,
                                  FileNameUnicode,
                                  ossStrlen( NewFileName ) + 1) )
   {
       err = ossGetLastError () ;
       pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Failed to convert  file name to unicode: %s, Error: %d",
              pFileName, err ) ;
       goto error;
   }

   if (  INVALID_FILE_ATTRIBUTES == GetFileAttributes( FileNameUnicode )
                        && ERROR_FILE_NOT_FOUND ==  ossGetLastError () )
   {
      SDB_VALIDATE_GOTOERROR ( FALSE, SDB_FNE,
                               "File doesn't exist" ) ;
   }

   err = GetVolumeInformation( FileNameUnicode,
                               volumeName,
                               ARRAYSIZE(volumeName),
                               &serialNumber,
                               &maxComponentLen,
                               &fileSystemFlags,
                               fileSystemName,
                               ARRAYSIZE(fileSystemName) );
   if ( 0 == err )
   {
      err = ossGetLastError() ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to GetVolumeInformation() : %s, Error: %d",
             pFileName, err ) ;
      goto error ;
   }
   else
   {
      if( 0 == MultiByteToWideChar ( CP_ACP, 0,
                                     "NTFS", -1,
                                     NTFSName,
                                     ossStrlen( "NTFS" ) + 1) )
      {
          err = ossGetLastError () ;
          pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
                 "Failed to convert  'NTFS' to unicode: %s", pFileName ) ;
          goto error;
      }

      if ( 0 != (err = wcscmp(fileSystemName, NTFSName)) )
      {
         pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
                "Not supported file system : %s, Error: %d",
                pFileName, err ) ;
         goto error ;
      }
      else
      {
         *ossFSType = OSS_FS_NTFS ;
         goto done;
      }
   }
#elif defined (_LINUX)
   err = stat ( pFileName, &sb ) ;
   if ( -1 == err )
   {
      err = ossGetLastError() ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to stat() : %s, Error: %d",
             pFileName, err ) ;
      goto error ;
   }

   err = statfs ( pFileName, &sfs ) ;
   if ( -1 == err )
   {
      err = ossGetLastError() ;
      pdLog( PDERROR, __FUNC__, __FILE__, __LINE__,
             "Failed to statfs() : %s, Error: %d",
             pFileName, err ) ;
      goto error ;
   }
   else if ( (OSS_FS_EXT2_OLD != sfs.f_type) &&
             (OSS_FS_EXT2 != sfs.f_type) &&
             (OSS_FS_EXT3 != sfs.f_type) )
   {
      *ossFSType = OSS_FS_TYPE_UNKNOWN ;
      goto done;
   }
   else
   {
      switch (sfs.f_type)
      {
      case 	0xEF51:
         *ossFSType = OSS_FS_EXT2_OLD ;
         break;
      case  0xEF53:
         *ossFSType = OSS_FS_EXT2 ;
         break;
      default:
         *ossFSType = OSS_FS_TYPE_UNKNOWN ;
         break;
      }
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSGETFSTYPE, rc );
   return rc ;

error :
   *ossFSType = OSS_FS_TYPE_UNKNOWN ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSRENMPATH, "ossRenamePath" )
INT32 ossRenamePath ( const CHAR *pOldPath, const CHAR *pNewPath )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSRENMPATH );
   INT32 err = 0 ;
   err = rename ( pOldPath, pNewPath ) ;
   if ( err != 0 )
   {
      err = ossGetLastError() ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSRENMPATH, rc );
   return rc ;
error :
#if defined (_LINUX)
   switch ( err )
   {
   case ENOENT:
      rc = SDB_FNE ;
      break ;
   case EACCES:
   case EPERM:
      rc = SDB_PERM ;
      break ;
   case ENOMEM:
      rc = SDB_OOM ;
      break ;
   default:
      rc = SDB_IO ;
      break ;
   }
#elif defined (_WINDOWS)
   switch ( err )
   {
   case ERROR_FILE_NOT_FOUND:
   case ERROR_PATH_NOT_FOUND:
      rc = SDB_FNE ;
      break ;
   case ERROR_ACCESS_DENIED:
      rc = SDB_PERM ;
      break ;
   default :
      rc = SDB_IO ;
      break ;
   }
#endif
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSLOCKFILE, "ossLockFile" )
INT32 ossLockFile ( OSSFILE *pFile, OSS_FILE_LOCK lockType )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSLOCKFILE ) ;
   SDB_ASSERT ( pFile, "input file is NULL" ) ;
#if defined (_LINUX)
   struct flock lock ;
   ossMemset ( &lock, 0, sizeof(lock) ) ;
   switch ( lockType )
   {
   case OSS_LOCK_SH :
      lock.l_type = F_RDLCK ;
      break ;
   case OSS_LOCK_EX :
      lock.l_type = F_WRLCK ;
      break ;
   case OSS_LOCK_UN :
      lock.l_type = F_UNLCK ;
      break ;
   }
   lock.l_whence = SEEK_SET ;
   lock.l_start = 0 ;
   lock.l_len = 0 ;
   rc = fcntl ( pFile->fd, F_SETLK, &lock ) ;
   if ( -1 == rc )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to lock the file, errno = %d", rc ) ;
      if ( EACCES == rc || EAGAIN == rc )
      {
         rc = SDB_PERM ;
         goto error ;
      }
      rc = SDB_IO ;
      goto error ;
   }
   rc = SDB_OK ;
#elif defined (_WINDOWS)
   OVERLAPPED ovp ;
   BOOLEAN result ;
   ossMemset ( &ovp, 0, sizeof(ovp) ) ;
   if ( OSS_LOCK_SH == lockType )
   {
      result = LockFileEx ( pFile->hFile, LOCKFILE_FAIL_IMMEDIATELY,
                            0, 0, 0, &ovp ) ;
   }
   else if ( OSS_LOCK_EX == lockType )
   {
      result = LockFileEx ( pFile->hFile, LOCKFILE_EXCLUSIVE_LOCK |
                            LOCKFILE_FAIL_IMMEDIATELY, 0, 0, 0, &ovp ) ;
   }
   else
   {
      result = UnlockFileEx ( pFile->hFile, 0, 0, 0, &ovp ) ;
   }
   if ( !result )
   {
      rc = ossGetLastError() ;
      PD_LOG ( PDERROR,
               "Failed to lock the file, errno = %d", rc ) ;
      if ( ERROR_SHARING_VIOLATION == rc )
      {
         rc = SDB_PERM ;
         goto error ;
      }
      rc = SDB_IO ;
      goto error ;
   }
   rc = SDB_OK ;
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSLOCKFILE, rc );
   return rc ;
error :
   goto done ;
}

INT32 ossReadN( OSSFILE *file,
                SINT64 len,
                CHAR *buf,
                SINT64 &read )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != file && NULL != buf, "can not be null" ) ;

   SINT64 total = len ;
   SINT64 totalRead = 0 ;

   while ( 0 < total )
   {
      SINT64 onceRead = 0 ;
      rc = ossRead( file, buf + totalRead, total, &onceRead ) ;
      if ( SDB_OK == rc )
      {
         totalRead += onceRead ;
         total -= onceRead ;
         continue ;
      }
      else if ( SDB_INTERRUPT == rc )
      {
         rc = SDB_OK ;
         continue ;
      }
      else if ( SDB_EOF == rc && 0 < totalRead )
      {
         rc = SDB_OK ;
         break ;
      }
      else
      {
         goto error ;
      }
   }

   read = totalRead ;
done:
   return rc ;
error:
   goto done ;
}

INT32 ossSeekAndReadN( OSSFILE *file,
                       SINT64 offset,
                       SINT64 len,
                       CHAR *buf,
                       SINT64 &read )
{
   INT32 rc = SDB_OK ;
   SINT64 total = len ;
   SINT64 seek = offset ;
   read = 0 ;

   while ( 0 < total )
   {
      SINT64 onceRead = 0 ;
      rc = ossSeekAndRead( file, seek, buf + read, total, &onceRead ) ;
      if ( SDB_OK == rc )
      {
         read += onceRead ;
         total -= onceRead ;
         seek += onceRead ;
         continue ;
      }
      else if ( SDB_INTERRUPT == rc )
      {
         rc = SDB_OK ;
         continue ;
      }
      else if ( SDB_EOF == rc && 0 < read )
      {
         rc = SDB_OK ;
         break ;
      }
      else
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 ossWriteN( OSSFILE *file,
                 const CHAR *buf,
                 SINT64 len )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != file && NULL != buf, "can not be null" ) ;

   SINT64 total = len ;
   while ( 0 < total )
   {
      SINT64 onceWrite = 0 ;
      rc = ossWrite( file, buf + len - total, total, &onceWrite ) ;
      if ( SDB_OK == rc )
      {
         total -= onceWrite ;
         continue ;
      }
      else if ( SDB_INTERRUPT == rc )
      {
         rc = SDB_OK ;
         continue ;
      }
      else
      {
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 ossSeekAndWriteN( OSSFILE *file,
                        SINT64 offset,
                        const CHAR *pBufferWrite,
                        SINT64      iLenToWrite,
                        SINT64 &written )
{
   INT32 rc = SDB_OK ;
   SINT64 total = iLenToWrite ;
   SINT64 seek = offset ;
   written = 0 ;

   while ( 0 < total )
   {
      SINT64 onceWrite = 0 ;
      rc = ossSeekAndWrite( file, seek, pBufferWrite + written , total,
                            &onceWrite ) ;
      if ( SDB_OK == rc )
      {
         written += onceWrite ;
         total -= onceWrite ;
         seek += onceWrite ;
         continue ;
      }
      else if ( SDB_INTERRUPT == rc )
      {
         rc = SDB_OK ;
         continue ;
      }
      else
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 ossGetFileUserInfo( const CHAR * filename, OSSUID & uid, OSSGID & gid )
{
   INT32 rc = SDB_OK ;

#if defined( _LINUX )
   struct stat sb ;
   rc = stat ( filename, &sb ) ;
   if ( -1 == rc )
   {
      INT32 err = ossGetLastError() ;
      PD_LOG( PDERROR, "Failed to stat() : %s, Error: %d", filename, err ) ;
      switch(err)
      {
      case EACCES:
         rc = SDB_PERM ;
         break ;
      case ENOENT:
         rc = SDB_FNE ;
         break ;
      default:
         rc = SDB_IO ;
         break ;
      }
   }
   else
   {
      uid = sb.st_uid ;
      gid = sb.st_gid ;
   }
#else
#endif // _LINUX
   return rc ;
}

INT32 ossGetUserInfo( const CHAR * username, OSSUID & uid, OSSGID & gid )
{
   INT32 rc = SDB_OK ;

#if defined( _LINUX )
   struct passwd pwdinfo ;
   struct passwd *pwd = NULL ;
   UINT32 buffLen = 0 ;
   CHAR *pBuff = NULL ;

   buffLen = sysconf( _SC_GETPW_R_SIZE_MAX ) * sizeof( CHAR ) ;
   pBuff = ( CHAR* )SDB_OSS_MALLOC( buffLen ) ;
   if ( !pBuff )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   getpwnam_r( username, &pwdinfo, pBuff, buffLen, &pwd ) ;
   if ( !pwd )
   {
      std::cout << "getpwnam_r failed: " << ossGetLastError() << std::endl ;
      rc = SDB_SYS ;
      goto error ;
   }

   uid = pwd->pw_uid ;
   gid = pwd->pw_gid ;

done:
   if ( pBuff )
   {
      SDB_OSS_FREE( pBuff ) ;
   }
   return rc ;
error:
   goto done ;

#else
   return SDB_OK ;
#endif // _LINUX
}

INT32 ossGetUserInfo( OSSUID uid, CHAR *pUserName, UINT32 nameLen )
{
   INT32 rc = SDB_OK ;
   if ( nameLen > 0 )
   {
      pUserName[ nameLen - 1 ] = 0 ;
   }

#if defined( _LINUX )
   struct passwd pwdinfo ;
   struct passwd *pwd = NULL ;
   UINT32 buffLen = 0 ;
   CHAR *pBuff = NULL ;

   buffLen = sysconf( _SC_GETPW_R_SIZE_MAX ) * sizeof( CHAR ) ;
   pBuff = ( CHAR* )SDB_OSS_MALLOC( buffLen ) ;
   if ( !pBuff )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   getpwuid_r( uid, &pwdinfo, pBuff, buffLen, &pwd ) ;
   if ( !pwd )
   {
      std::cout << "getpwuid_r failed: " << ossGetLastError() << std::endl ;
      rc = SDB_SYS ;
      goto error ;
   }

   if ( nameLen > 0 )
   {
      ossStrncpy( pUserName, pwd->pw_name, nameLen -1 ) ;
   }

done:
   if ( pBuff )
   {
      SDB_OSS_FREE( pBuff ) ;
   }
   return rc ;
error:
   goto done ;

#else
   return SDB_OK ;
#endif // _LINUX
}


