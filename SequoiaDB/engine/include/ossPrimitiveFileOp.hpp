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

#define OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE 2048

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

   ossPrimitiveFileOp() ;
   ~ossPrimitiveFileOp() ;

   int Open
   (
      const char * pFilePath,
      UINT32_64    options =   OSS_PRIMITIVE_FILE_OP_READ_WRITE
                             | OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS
   ) ;

   void openStdout() ;

   void Close() ;

   BOOLEAN isValid() const ;
   BOOLEAN isExist() const ;

   int Read( const size_t size, void * const pBuf, int * const pBytesRead ) ;

   int Write( const void * pBuf, size_t len = 0 ) ;

   int fWrite( const char * fmt, ... ) ;

   offsetType getCurrentOffset (void) const ;

   void seekToOffset( offsetType offset ) ;

   void seekToEnd( void ) ;

   int getSize( offsetType * const pFileSize ) ;

   handleType getHandle( void ) const
   {
      return _fileHandle ;
   }
} ;

#endif // OSS_PRIMITIVE_FILE_OP_HPP
