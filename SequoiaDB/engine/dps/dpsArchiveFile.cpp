/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = dpsArchiveFile.cpp

   Descriptive Name = Data Protection Services Log Archive File

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/3/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsArchiveFile.hpp"
#include "dpsLogFile.hpp"

namespace engine
{
   dpsArchiveFile::dpsArchiveFile()
   {
      _logHeader = NULL ;
      _archiveHeader = NULL ;
      _readOnly = FALSE ;
      _inited = FALSE ;
   }

   dpsArchiveFile::~dpsArchiveFile()
   {
      close() ;
   }

   INT32 dpsArchiveFile::init( const string& path, BOOLEAN readOnly )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      INT32 mode = 0 ;

      SDB_ASSERT( !_inited, "already inited" ) ;

      _path = path ;
      _readOnly = readOnly ;

      rc = ossFile::exists( path, exist ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check existence of file[%s], rc=%d",
                 path.c_str(), rc ) ;
         goto error ;
      }

      if ( exist )
      {
         if ( readOnly )
         {
            mode = OSS_READONLY ;
         }
         else
         {
            mode = OSS_READWRITE ;
         }
      }
      else
      {
         if ( readOnly )
         {
            rc = SDB_FNE ;
            PD_LOG( PDERROR, "Readonly file[%s] does not exist",
                    path.c_str() ) ;
            goto error ;
         }

         mode = OSS_CREATEONLY | OSS_READWRITE ;
      }

      rc = _file.open( path, mode, OSS_DEFAULTFILE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc=%d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      rc = _initHeader() ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to init archive file header, rc=%d", rc ) ;
         goto error;
      }

      if ( exist )
      {
         rc = _readHeader() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to reader header of archive file[%s]," \
                    "rc=%d", _path.c_str(), rc ) ;
            goto error ;
         }

         if ( readOnly )
         {
            if (!_archiveHeader->isValid())
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid header of archive file[%s]",
                       _path.c_str() ) ;
               goto error ;
            }
         }
      }
      else
      {
         rc = _flushHeader() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to finish header of archive file[%s]," \
                    "rc=%d", _path.c_str(), rc ) ;
            goto error ;
         }
      }

      // skip file header
      rc = _file.seek( DPS_LOG_HEAD_LEN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to seek archive file[%s] at offset[%lld], " \
                 "rc=%d", _path.c_str(), 0, rc ) ;
         goto error ;
      }

      _inited = TRUE ;

   done:
      return rc ;
   error:
      close() ;
      goto done ;
   }

   void dpsArchiveFile::close()
   {
      _file.close() ;
      SAFE_OSS_DELETE( _logHeader ) ;
      _archiveHeader = NULL ;
      _path = "" ;
      _readOnly = FALSE ;
      _inited = FALSE ;
   }

   INT32 dpsArchiveFile::flushHeader()
   {
      return _flushHeader() ;
   }

   INT32 dpsArchiveFile::write( const CHAR* data, INT64 len )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "uninited file" ) ;
      SDB_ASSERT( !_readOnly, "readonly file" ) ;

      rc = _file.writeN( data, len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write to archive file[%s], rc=%d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFile::write( INT64 offset, const CHAR* data, INT64 len )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "uninited file" ) ;
      SDB_ASSERT( !_readOnly, "readonly file" ) ;

      rc = _file.seekAndWriteN( offset, data, len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write to archive file[%s], rc=%d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFile::write( ossFile& fromFile, BOOLEAN compress )
   {
      utilFileInStream in ;
      utilFileOutStream fileOut ;
      utilZlibOutStream zlibOut ;
      utilStream stream ;
      utilOutStream* out = NULL ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "uninited file" ) ;
      SDB_ASSERT( !_readOnly, "readonly file" ) ;

      rc = in.init( &fromFile, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init file instream, rc=%d", rc ) ;
         goto error ;
      }

      rc = fileOut.init( &_file, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init file outstream, rc=%d", rc ) ;
         goto error ;
      }

      rc = stream.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init stream, rc=%d", rc ) ;
         goto error ;
      }

      if ( compress )
      {
         rc = zlibOut.init( fileOut ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init zlib outstream, rc=%d", rc ) ;
            goto error ;
         }

         out = &zlibOut ;
      }
      else
      {
         out = &fileOut ;
      }

      rc = stream.copy( in, *out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to stream, rc=%d", rc ) ;
         goto error ;
      }

      rc = out->close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close outstream, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFile::extend( INT64 fileSize )
   {
      INT32 rc = SDB_OK;
      INT64 realSize = 0;

      SDB_ASSERT( _inited, "uninited file" ) ;
      SDB_ASSERT( !_readOnly, "readonly file" ) ;

      rc = _file.getFileSize( realSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get file size[%s], rc=%d",
                 _path.c_str(), rc ) ;
         goto error;
      }

      realSize -= DPS_LOG_HEAD_LEN;

      if ( realSize < fileSize )
      {
         INT64 increment = fileSize - realSize ;
         rc = _file.extend( increment ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend file[%s], rc=%d",
                    _path.c_str(), rc) ;
            goto error ;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dpsArchiveFile::_initHeader()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL == _logHeader, "_logHeader should be NULL" ) ;
      SDB_ASSERT( NULL == _archiveHeader, "_archiveHeader should be NULL" ) ;

      _logHeader = SDB_OSS_NEW _dpsLogHeader() ;
      if ( NULL == _logHeader )
      {
         rc = SDB_OOM;
         PD_LOG ( PDERROR, "Failed to create new _dpsLogHeader!" ) ;
         goto error;
      }

      _archiveHeader = ( dpsArchiveHeader* )
                       ( (CHAR*)_logHeader + DPS_ARCHIVE_HEADER_OFFSET ) ;
      _archiveHeader->init() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFile::_readHeader()
   {
      INT32 rc = SDB_OK ;
      INT64 readSize = 0 ;
      INT64 offset = 0 ;

      SDB_ASSERT( NULL != _logHeader, "_logHeader can't be NULL" ) ;

      // remember current position
      rc = _file.position( offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to postion archive file[%s], rc=%d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      // move to file header
      rc = _file.seek( 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to seek archive file[%s] at offset[%lld], " \
                 "rc=%d", _path.c_str(), 0, rc ) ;
         goto error ;
      }

      rc = _file.readN( (CHAR*)_logHeader, DPS_LOG_HEAD_LEN, readSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read archive file header, rc=%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( readSize == DPS_LOG_HEAD_LEN,
                  "readSize != DPS_LOG_HEAD_LEN" ) ;

      // go back to previous position
      rc = _file.seek( offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to seek archive file[%s] at offset[%lld], " \
                 "rc=%d", _path.c_str(), 0, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFile::_flushHeader()
   {
      INT32 rc = SDB_OK ;
      INT64 offset = 0 ;

      SDB_ASSERT( NULL != _logHeader, "_logHeader can't be NULL" ) ;
      SDB_ASSERT( !_readOnly, "can't finish readonly file header" ) ;

      // remember current position
      rc = _file.position( offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to postion archive file[%s], rc=%d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      // move to file header
      rc = _file.seek( 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to seek archive file[%s] at offset[%lld], " \
                 "rc=%d", _path.c_str(), 0, rc ) ;
         goto error ;
      }

      rc = _file.writeN( (CHAR*)_logHeader, DPS_LOG_HEAD_LEN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to finish header of archive file[%s], " \
                 "rc=%d", _path.c_str(), rc ) ;
         goto error ;
      }

      // go back to previous position
      rc = _file.seek( offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to seek archive file[%s] at offset[%lld], " \
                 "rc=%d", _path.c_str(), 0, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

