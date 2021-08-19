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

   Source File Name = utilFileStream.cpp

   Descriptive Name = File I/O stream

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilFileStream.hpp"
#include "ossMem.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
   utilFileStreamBase::utilFileStreamBase()
      : _file( NULL ),
        _fileManaged( FALSE ),
        _inited( FALSE )
   {
   }

   utilFileStreamBase::~utilFileStreamBase()
   {
      _close() ;
   }

   INT32 utilFileStreamBase::init( ossFile* file, BOOLEAN fileManaged )
   {
      SDB_ASSERT( !_inited, "already inited" ) ;
      SDB_ASSERT( NULL != file, "file can't be NULL" ) ;
      SDB_ASSERT( file->isOpened(), "file should opened" ) ;

      _file = file ;
      _fileManaged = fileManaged ;
      _inited = TRUE ;

      return SDB_OK ;
   }

   INT32 utilFileStreamBase::_close()
   {
      INT32 rc = SDB_OK ;
      
      if ( _fileManaged && NULL != _file )
      {
         rc = _file->close() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to close file[%s], rc=%d",
                    _file->getPath().c_str(), rc ) ;
         }

         SDB_OSS_DEL( _file ) ;
      }

      _file = NULL ;
      _fileManaged = FALSE ;
      _inited = FALSE ;

      return rc ;
   }

   INT32 utilFileStreamBase::_read( CHAR* buf, INT64 bufLen, INT64& readSize )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "should inited" ) ;
      SDB_ASSERT( NULL != _file, "_file can't be NULL" ) ;
      SDB_ASSERT( _file->isOpened(), "_file should be opened" ) ;

      rc = _file->readN( buf, bufLen, readSize ) ;
      if ( SDB_OK != rc && SDB_EOF != rc )
      {
         PD_LOG( PDERROR, "Failed to read from file[%s], bufLen=%lld, rc=%d",
                 _file->getPath().c_str(), bufLen, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilFileStreamBase::_write( const CHAR* buf, INT64 bufLen )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "should inited" ) ;
      SDB_ASSERT( NULL != _file, "_file can't be NULL" ) ;
      SDB_ASSERT( _file->isOpened(), "_file should be opened" ) ;

      rc = _file->writeN( buf, bufLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write to file[%s], bufLen=%lld",
                 _file->getPath().c_str(), bufLen ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilFileStreamBase::_flush()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "should inited" ) ;
      SDB_ASSERT( NULL != _file, "_file can't be NULL" ) ;
      SDB_ASSERT( _file->isOpened(), "_file should be opened" ) ;

      rc = _file->sync() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to sync file[%s], rc=%d",
                 _file->getPath().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   utilFileInStream::utilFileInStream()
   {
   }

   utilFileInStream::~utilFileInStream()
   {
   }

   INT32 utilFileInStream::read( CHAR* buf, INT64 bufLen, INT64& readSize )
   {
      return _read( buf, bufLen, readSize ) ;
   }

   INT32 utilFileInStream::close()
   {
      return _close() ;
   }

   utilFileOutStream::utilFileOutStream()
   {
   }

   utilFileOutStream::~utilFileOutStream()
   {
   }

   INT32 utilFileOutStream::write( const CHAR* buf, INT64 bufLen )
   {
      return _write( buf, bufLen ) ;
   }
   
   INT32 utilFileOutStream::flush()
   {
      return _flush() ;
   }

   INT32 utilFileOutStream::close()
   {
      return _close() ;
   }
}

