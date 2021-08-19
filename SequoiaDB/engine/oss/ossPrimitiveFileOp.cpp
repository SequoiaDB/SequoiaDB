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

   Source File Name = ossPrimitiveFileOp.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#if defined (_LINUX) || defined (_AIX)
   #include <fcntl.h>
   #include <unistd.h>
   #include <sys/stat.h>
#elif defined (_WINDOWS)
   #include <stdlib.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <Fcntl.h>
   #include <io.h>
#endif
#include <string.h>
#include "ossPrimitiveFileOp.hpp"
#include "ossUtil.hpp"

/*
   ossPrimitiveFileOp implement
*/
ossPrimitiveFileOp::ossPrimitiveFileOp()
{
   _fileHandle = OSS_INVALID_HANDLE_FD_VALUE ;
   _bIsStdout = FALSE ;
   ossMemset( _fileName, 0, sizeof( _fileName ) ) ;
}

ossPrimitiveFileOp::~ossPrimitiveFileOp()
{
   Close() ;
}

BOOLEAN ossPrimitiveFileOp::isValid() const
{
   return ( OSS_INVALID_HANDLE_FD_VALUE != _fileHandle ) ;
}

BOOLEAN ossPrimitiveFileOp::isExist() const
{
   if ( _bIsStdout ||
        ( 0 != _fileName[0] &&
          0 == oss_access( _fileName, 0 ) ) )
   {
      return TRUE ;
   }
   return FALSE ;
}

void ossPrimitiveFileOp::Close()
{
   if ( isValid() && ( ! _bIsStdout ) )
   {
      oss_close( _fileHandle ) ;
      _fileHandle = OSS_INVALID_HANDLE_FD_VALUE ;
   }
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSPFOP_OPEN, "ossPrimitiveFileOp::Open" )
int ossPrimitiveFileOp::Open( const CHAR * pFilePath, UINT32_64 options )
{
   INT32 rc = 0 ;
   PD_TRACE_ENTRY ( SDB_OSSPFOP_OPEN );

#if defined (_LINUX) || defined (_AIX)
   INT32 mode = O_RDWR ;

   if ( options & OSS_PRIMITIVE_FILE_OP_READ_ONLY )
   {
      mode = O_RDONLY ;
   }
   else if ( options & OSS_PRIMITIVE_FILE_OP_WRITE_ONLY )
   {
      mode = O_WRONLY ;
   }

   if ( options & OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING )
   {
   }
   else if ( options & OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS )
   {
      mode |= O_CREAT ;
   }

   if ( options & OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC )
   {
      mode |= O_TRUNC ;
   }

   do
   {
      _fileHandle = oss_open( pFilePath, mode, 0644 ) ;
   } while (( -1 == _fileHandle ) && ( EINTR == errno )) ;
#elif defined (_WINDOWS)
   INT32 mode = _O_RDWR ;

   if ( options & OSS_PRIMITIVE_FILE_OP_READ_ONLY )
   {
      mode = _O_RDONLY | _O_BINARY ;
   }
   else if ( options & OSS_PRIMITIVE_FILE_OP_WRITE_ONLY )
   {
      mode = _O_WRONLY | _O_BINARY ;
   }

   if ( options & OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING )
   {
      mode |= ( _O_CREAT | _O_APPEND | _O_BINARY ) ;
   }
   else if ( options & OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS )
   {
      mode |= ( _O_CREAT | _O_APPEND | _O_BINARY ) ;
   }

   if ( options & OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC )
   {
      mode |= _O_TRUNC ;
   }

   _fileHandle = oss_open( pFilePath, mode, _S_IREAD | _S_IWRITE ) ;
#endif
   if ( _fileHandle <= OSS_INVALID_HANDLE_FD_VALUE )
   {
      rc = errno ;
      goto exit ;
   }
   ossStrncpy( _fileName, pFilePath, OSS_MAX_PATHSIZE ) ;

exit :
   PD_TRACE_EXITRC ( SDB_OSSPFOP_OPEN, rc );
   return rc ;
}

void ossPrimitiveFileOp::openStdout()
{
#if defined (_LINUX) || defined (_AIX)
   setFileHandle(STDOUT_FILENO) ;
#elif defined (_WINDOWS)
   HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE) ;
   setFileHandle((handleType)handle) ;
#endif
   _bIsStdout = TRUE ;
}

ossPrimitiveFileOp::offsetType ossPrimitiveFileOp::getCurrentOffset () const
{
   offsetType returnValue ;
   returnValue.offset = oss_lseek( _fileHandle, 0, SEEK_CUR ) ;
   return returnValue ;
}

void ossPrimitiveFileOp::seekToEnd( void )
{
   oss_lseek( _fileHandle, 0, SEEK_END ) ;
}

void ossPrimitiveFileOp::seekToOffset( ossPrimitiveFileOp::offsetType param )
{
   if ( ( oss_off_t )-1 != param.offset )
   {
      oss_lseek( _fileHandle, param.offset, SEEK_SET ) ;
   }
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSPFOP_READ, "ossPrimitiveFileOp::Read" )
int ossPrimitiveFileOp::Read
(
   const size_t size,
   void * const pBuffer,
   INT32 * const  pBytesRead
)
{
   INT32     retval    = 0 ;
   PD_TRACE_ENTRY ( SDB_OSSPFOP_READ );
#if defined (_LINUX) || defined (_AIX)
   ssize_t bytesRead = 0 ;
#elif defined (_WINDOWS)
   SINT64  bytesRead = 0 ;
#endif

   if ( isValid() )
   {
      do
      {
         bytesRead = oss_read( _fileHandle, pBuffer, size ) ;
      } while (( -1 == bytesRead ) && ( EINTR == errno )) ;
      if ( -1 == bytesRead )
      {
         goto err_read ;
      }
   }
   else
   {
      goto err_read ;
   }

   if ( pBytesRead )
   {
      *pBytesRead = bytesRead ;
   }
exit :
   PD_TRACE_EXITRC ( SDB_OSSPFOP_READ, retval );
   return retval ;

err_read :
   *pBytesRead = 0 ;
   retval      = errno ;
   goto exit ;
}

// returns 0 for success, others for errno
PD_TRACE_DECLARE_FUNCTION ( SDB_OSSPFOP_WRITE, "ossPrimitiveFileOp::Write" )
int ossPrimitiveFileOp::Write( const void * pBuffer, size_t size )
{
   int rc = 0 ;
   PD_TRACE_ENTRY ( SDB_OSSPFOP_WRITE );

   size_t currentSize = 0 ;
   if ( 0 == size )
   {
      size = strlen( ( CHAR * )pBuffer ) ;
   }

   if ( isValid() )
   {
      do
      {
         rc = oss_write( _fileHandle, &((char*)pBuffer)[currentSize],
                         size-currentSize ) ;
         if ( rc >= 0 )
            currentSize += rc ;
      } while ((( -1 == rc ) && ( EINTR == errno )) ||
               (( -1 != rc ) && ( currentSize != size ))) ;
      if ( -1 == rc )
      {
         rc = errno ;
         goto exit ;
      }
      rc = 0 ;
   }
exit :
   PD_TRACE_EXITRC ( SDB_OSSPFOP_WRITE, rc );
   return rc ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSPFOP_FWRITE, "ossPrimitiveFileOp::fWrite" )
int ossPrimitiveFileOp::fWrite( const CHAR * format, ... )
{
   int rc = 0 ;
   PD_TRACE_ENTRY ( SDB_OSSPFOP_FWRITE );
   va_list ap ;
   CHAR buf[OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE] = { 0 } ;

   va_start( ap, format ) ;
   ossVsnprintf( buf, sizeof( buf ), format, ap ) ;
   va_end( ap ) ;

   rc = Write( buf ) ;

   PD_TRACE_EXITRC ( SDB_OSSPFOP_FWRITE, rc );
   return rc ;
}

void ossPrimitiveFileOp::setFileHandle( handleType handle )
{
   _fileHandle = handle ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSPFOP_GETSIZE, "ossPrimitiveFileOp::getSize" )
int ossPrimitiveFileOp::getSize( offsetType * const pFileSize )
{
   int             rc        = 0 ;
   PD_TRACE_ENTRY ( SDB_OSSPFOP_GETSIZE );
   oss_struct_stat buf       = { 0 } ;

   if ( -1 == oss_fstat( _fileHandle, &buf ) )
   {
      rc = errno ;
      goto err_exit ;
   }

   pFileSize->offset = buf.st_size ;

exit :
   PD_TRACE_EXITRC ( SDB_OSSPFOP_GETSIZE, rc );
   return rc ;

err_exit :
   pFileSize->offset = 0 ;
   goto exit ;
}
