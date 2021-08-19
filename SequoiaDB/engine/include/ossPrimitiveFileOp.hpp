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

   Source File Name = ossPrimitiveFileOp.hpp

   Descriptive Name = Operating System Services Primitive File Operation Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure for primitive file
   operation, including basic open/read/write/seek operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_PRIMITIVE_FILE_OP_HPP
#define OSS_PRIMITIVE_FILE_OP_HPP
#include "core.hpp"
#include "oss.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined (_LINUX) || defined (_AIX)
   #define OSS_F_GETLK        F_GETLK64
   #define OSS_F_SETLK        F_SETLK64
   #define OSS_F_SETLKW       F_SETLKW64

   #define oss_struct_statfs  struct statfs64
   #define oss_statfs         statfs64
   #define oss_fstatfs        fstatfs64
   #define oss_struct_statvfs struct statvfs64
   #define oss_statvfs        statvfs64
   #define oss_fstatvfs       fstatvfs64
   #define oss_struct_stat    struct stat64
   #define oss_struct_flock   struct flock64
   #define oss_stat           stat64
   #define oss_lstat          lstat64
   #define oss_fstat          fstat64
   #define oss_open           open64
   #define oss_lseek          lseek64
   #define oss_ftruncate      ftruncate64
   #define oss_off_t          off64_t
   #define oss_close          close
   #define oss_chmod          chmod
   #define oss_read           read
   #define oss_write          write
   #define oss_access         access
#elif defined (_WINDOWS)
   #define oss_struct_stat    struct _stati64
   #define oss_stat           _stati64
   #define oss_fstat          _fstati64
   #define oss_lstat          oss_stat
   #define oss_open           _open
   #define oss_lseek          _lseeki64
   #define oss_close          _close
   #define oss_chmod          _chmod
   #define oss_read           _read
   #define oss_write          _write
   #define oss_off_t          INT64
   #define oss_access         _access
#endif

// size of the internal buffer used by ossPrimitiveFileOp::fwrite
// Note:
//   Do NOT change this value to a bigger size,
//   ossPrimitiveFileOp::fwrite allocates the buffer from stack !
#define OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE 2048

// open() options
#define OSS_PRIMITIVE_FILE_OP_READ_ONLY     (((UINT32_64)1) << 0)
#define OSS_PRIMITIVE_FILE_OP_WRITE_ONLY    (((UINT32_64)1) << 1)
#define OSS_PRIMITIVE_FILE_OP_READ_WRITE    (((UINT32_64)1) << 2)
#define OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING (((UINT32_64)1) << 3)
#define OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS   (((UINT32_64)1) << 4)
#define OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC    (((UINT32_64)1) << 5)

#if defined (_LINUX) || defined (_AIX)
   #define OSS_PRIMITIVE_FILE_SEP "/"
   #define OSS_INVALID_HANDLE_FD_VALUE (-1)
#elif defined (_WINDOWS)
   #define OSS_PRIMITIVE_FILE_SEP "\\"
   #define OSS_INVALID_HANDLE_FD_VALUE (-1)
#endif

class ossPrimitiveFileOp : public SDBObject
{
public :
   class offsetType : public SDBObject
   {
   public :
   #if defined (_LINUX) || defined (_AIX)
      typedef oss_off_t ossOffset_t ;
   #elif defined (_WINDOWS)
      typedef SINT64 ossOffset_t ;
   #endif

      ossOffset_t offset ;
   } ;

#if defined (_LINUX) || defined (_AIX)
   typedef  int    handleType ;
#elif defined (_WINDOWS)
   typedef  int handleType ;
#endif

private :
   handleType _fileHandle ;
   ossPrimitiveFileOp( const ossPrimitiveFileOp & ) {}
   const ossPrimitiveFileOp &operator=( const ossPrimitiveFileOp & ) ;
   BOOLEAN _bIsStdout ;
   CHAR    _fileName[ OSS_MAX_PATHSIZE + 1 ] ;

protected :
   void setFileHandle( handleType handle ) ;

public :

   // Default constructor.  Construct an uninitialized OSS file handle
   ossPrimitiveFileOp() ;
   ~ossPrimitiveFileOp() ;

   // Create or open an existing file.
   // pFilePath [in]
   //    Full path to the file.
   // options [in]
   //    Open operation mask, logical OR from following :
   //       OSS_PRIMITIVE_FILE_OP_READ_WRITE
   //          - default. open for read/write.
   //       OSS_PRIMITIVE_FILE_OP_READ_ONLY
   //          - open for read only.
   //       OSS_PRIMITIVE_FILE_OP_WRITE_ONLY
   //          - open for write only.
   //       OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING
   //          - Open an existing file, fails if the file doesn't exist.
   //       OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS
   //          - Open an existing file, creates one if the file doesn't exist.
   // Return:
   //    0,     success
   //    errno, failure
   int Open
   (
      const char * pFilePath,
      UINT32_64    options =   OSS_PRIMITIVE_FILE_OP_READ_WRITE
                             | OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS
   ) ;

   // open stdout for output.
   void openStdout() ;

   // close the file
   void Close() ;

   // validate an oss file handle
   BOOLEAN isValid() const ;
   BOOLEAN isExist() const ;

   // read data from the file
   // size [in]
   //    the size of pBuf
   // pBuf[out]
   //    a preallocated buffer for reading data.
   // pBytesRead [out]
   //    the  number of bytes read from the file.
   // Return:
   //    0,     success
   //    errno, failure
   int Read( const size_t size, void * const pBuf, int * const pBytesRead ) ;

   // write data to the file
   // pBuf[in]
   //   Data to be written
   // len[in]
   //   Number of bytes to be written. If the pBuf is a NULL termianted
   //   string this parameter can be ignored.
   int Write( const void * pBuf, size_t len = 0 ) ;

   // write formated information to the file. It can only handle a fixed
   // size buffer ( 2K, OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE )
   // fmt
   //   a fprintf style format specifier.
   // ...
   //   variable argument list assocatiated with the above format flags
   // Note:
   //   It calls vsnprintf internally, and the fixed size buffer are allocated
   //   from the stack.
   int fWrite( const char * fmt, ... ) ;

   // get the current file offset.
   offsetType getCurrentOffset (void) const ;

   // seek to the position obtained by currentOffset.
   void seekToOffset( offsetType offset ) ;

   // seek to the end
   void seekToEnd( void ) ;

   // get the current file size.
   // pFileSize[out]
   // Return:
   //    0,     success
   //    errno, failure
   int getSize( offsetType * const pFileSize ) ;

   // get the raw file descriptor.  For OSS inernal use only.
   handleType getHandle( void ) const
   {
      return _fileHandle ;
   }
} ;

#endif // OSS_PRIMITIVE_FILE_OP_HPP
