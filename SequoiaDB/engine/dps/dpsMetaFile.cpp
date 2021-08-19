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

   Source File Name = dpsMetaFile.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsMetaFile.hpp"
#include "utilStr.hpp"

namespace engine
{
   _dpsMetaFile::_dpsMetaFile()
   {
      ossMemset( _path, 0, sizeof( _path ) ) ;
      _invalidateStatus = FALSE ;
   }

   _dpsMetaFile::~_dpsMetaFile()
   {
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
   }

   BOOLEAN _dpsMetaFile::isCacheLSNValid() const
   {
      return DPS_INVALID_LSN_OFFSET != _content._oldestLSNOffset ?
             TRUE : FALSE ;
   }

   INT32 _dpsMetaFile::init( const CHAR *parentDir )
   {
      INT32 rc = SDB_OK ;
      if ( NULL == parentDir )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = utilBuildFullPath( parentDir, DPS_METAFILE_NAME, OSS_MAX_PATHSIZE,
                              _path ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Build full path failed(parentDir=%s), rc: %d",
                 parentDir, rc ) ;
         goto error ;
      }

      if ( SDB_OK == ossAccess( _path ) )
      {
         rc = _restore() ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }

         PD_LOG( PDWARNING, "Restore meta file(%s) failed, rc: %d",
                 _path, rc ) ;
         PD_LOG( PDWARNING, "Try to delete meta file(%s) and recreate it",
                 _path ) ;
         ossDelete( _path ) ;
      }

      rc = _initNewFile() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Init new meta file(%s) failed, rc: %d",
                 _path, rc ) ;
         goto error ;
      }

      PD_LOG( PDINFO, "Create dps meta file(%s) succeed", _path ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsMetaFile::stop( DPS_LSN_OFFSET oldestTransLSN,
                             UINT32 beginFile,
                             UINT32 workFile,
                             const DPS_LSN &curLSN,
                             UINT32 curLsnLength,
                             const DPS_LSN &memBeginLSN )
   {
      INT32 rc = SDB_OK ;

      _content._oldestLSNOffset = oldestTransLSN ;
      _content._beginFile = beginFile ;
      _content._workFile = workFile ;
      _content._curLsnVersion = curLSN.version ;
      _content._curLsnOffset = curLSN.offset ;
      _content._curLsnLength = curLsnLength ;
      _content._memBeginLsnVer = memBeginLSN.version ;
      _content._memBeginLsnOffset = memBeginLSN.offset ;

      rc = writeContent() ;
      if ( SDB_OK == rc )
      {
         _invalidateStatus = _content.isStatusValid() ? FALSE : TRUE ;
         PD_LOG( PDEVENT, "Save meta info( OldestTransLSN: %lld, "
                                          "BeginFile: %d, "
                                          "WorkFile: %d, "
                                          "CurLsn: %d.%lld, "
                                          "CurLsnLength: %u, "
                                          "MemBeginLsn: %d.%lld ) succeed",
                 _content._oldestLSNOffset,
                 _content._beginFile,
                 _content._workFile,
                 _content._curLsnVersion,
                 _content._curLsnOffset,
                 _content._curLsnLength,
                 _content._memBeginLsnVer,
                 _content._memBeginLsnOffset ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Save meta info( OldestTransLSN: %lld, "
                                          "BeginFile: %d, "
                                          "WorkFile: %d, "
                                          "CurLsn: %d.%lld, "
                                          "CurLsnLength: %u, "
                                          "MemBeginLsn: %d.%lld ) "
                                          "failed, rc: %d",
                 _content._oldestLSNOffset,
                 _content._beginFile,
                 _content._workFile,
                 _content._curLsnVersion,
                 _content._curLsnOffset,
                 _content._curLsnLength,
                 _content._memBeginLsnVer,
                 _content._memBeginLsnOffset,
                 rc ) ;
      }

      return rc ;
   }

   INT32 _dpsMetaFile::_restore()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isOpened = FALSE ;
      SINT64 read = 0 ;

      rc = ossOpen( _path, OSS_READWRITE, OSS_RWXU, _file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file(%s), rc:%d",
                  _path, rc ) ;
      isOpened = TRUE ;

      rc = ossReadN( &_file, sizeof( _header ), (CHAR *)&_header, read ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta file(%s) header, rc: %d",
                  _path, rc ) ;

      // read size must equal header's size.
      PD_CHECK( read == sizeof( _header ), SDB_SYS, error, PDERROR,
                "Failed to read meta file(%s) header, rc: %d",
                _path, SDB_SYS ) ;

      if ( 0 != ossMemcmp( _header._eyeCatcher, DPS_METAFILE_HEADER_EYECATCHER,
                           DPS_METAFILE_HEADER_EYECATCHER_LEN ) )
      {
         // header's magic number must be DPS_METAFILE_HEADER_EYECATCHER
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Meta header is invalid, rc: %d", rc ) ;
         goto error ;
      }

      // try to read lsn
      rc = readContent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta file(%s) content, rc: %d",
                   _path, rc ) ;
      _invalidateStatus = _content.isStatusValid() ? FALSE : TRUE ;

      PD_LOG( PDEVENT, "Load meta info( OldestTransLSN: %lld, "
                                       "BeginFile: %d, "
                                       "WorkFile: %d, "
                                       "CurLsn: %d.%lld, "
                                       "CurLsnLength: %u, "
                                       "MemBeginLsn: %d.%lld ) succeed",
              _content._oldestLSNOffset,
              _content._beginFile,
              _content._workFile,
              _content._curLsnVersion,
              _content._curLsnOffset,
              _content._curLsnLength,
              _content._memBeginLsnVer,
              _content._memBeginLsnOffset ) ;

   done:
      return rc ;
   error:
      if ( isOpened )
      {
         ossClose( _file ) ;
      }
      goto done ;
   }

   INT32 _dpsMetaFile::_initNewFile()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isCreated = FALSE ;
      SINT64 written = 0 ;

      _content.reset() ;

      rc = ossOpen( _path, OSS_CREATEONLY | OSS_READWRITE,
                    OSS_DEFAULTFILE, _file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create meta file(%s), rc: %d",
                   _path, rc ) ;
      isCreated = TRUE ;

      // increase the file size to the total meta file size
      rc = ossExtendFile( &_file, DPS_METAFILE_HEADER_LEN +
                          DPS_METAFILE_CONTENT_LEN  ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extend meta file(size=%d), rc: %d",
                   DPS_METAFILE_HEADER_LEN + DPS_METAFILE_CONTENT_LEN, rc ) ;

      // write header
      rc = ossSeekAndWriteN( &_file, 0, (const CHAR *)&_header,
                             sizeof( _header ), written ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write header, rc: %d", rc ) ;

      // write content
      rc = ossSeekAndWriteN( &_file, DPS_METAFILE_HEADER_LEN,
                             (const CHAR *)&_content,
                             sizeof( _content ), written ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write content, rc: %d", rc ) ;

      //sync the file
      rc = ossFsync( &_file ) ;
      PD_RC_CHECK( rc, PDERROR, "Fsync failed file(%s), rc: %d", _path, rc ) ;

   done:
      return rc ;
   error:
      if ( isCreated )
      {
         ossClose( _file ) ;
      }
      goto done ;
   }

   INT32 _dpsMetaFile::writeContent()
   {
      INT32 rc = SDB_OK ;
      SINT64 written = 0 ;

      SINT64 toWrite = sizeof( _content ) ;
      rc = ossSeekAndWriteN( &_file, DPS_METAFILE_HEADER_LEN,
                             ( const CHAR* ) &_content, toWrite, written ) ;
      PD_RC_CHECK( rc, PDERROR, "Write content failed, rc: %d", rc ) ;

      rc = ossFsync( &_file ) ;
      PD_RC_CHECK( rc, PDERROR, "Fsync file(%s) failed, rc: %d",
                   _path, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsMetaFile::readContent()
   {
      INT32 rc = SDB_OK ;
      SINT64 read = 0 ;

      SINT64 toRead = sizeof( _content ) ;
      rc = ossSeekAndReadN( &_file, DPS_METAFILE_HEADER_LEN, toRead,
                            ( CHAR * ) &_content, read ) ;
      PD_RC_CHECK( rc, PDERROR, "Read content failed, rc: %d", rc ) ;

      PD_CHECK( toRead == read, SDB_SYS, error, PDERROR,
                "Read content failed( read=%lld,expect=%lld )",
                read, toRead ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsMetaFile::writeOldestLSNOffset( DPS_LSN_OFFSET offset )
   {
      _content._oldestLSNOffset = offset ;
      return writeContent() ;
   }

   INT32 _dpsMetaFile::invalidateStatus()
   {
      INT32 rc = SDB_OK ;

      if ( !_invalidateStatus )
      {
         _content.resetStatus() ;
         rc =  writeContent() ;
         if ( SDB_OK == rc )
         {
            _invalidateStatus = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Invalidate dps meta status failed, rc: %d", rc ) ;
         }
      }

      return rc ;
   }

}


