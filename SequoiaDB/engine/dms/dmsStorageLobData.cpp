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

   Source File Name = dmsStorageLobData.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageLobData.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "pmd.hpp"
#include "dmsLobDirectInBuffer.hpp"
#include "dmsLobDirectOutBuffer.hpp"
#include "dmsTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{

   const UINT32 DMS_LOBD_EXTEND_LEN = 4 * 1024 * 1024 ;

   /*
      _dmsStorageLobData implement
   */
   _dmsStorageLobData::_dmsStorageLobData( const CHAR *fileName,
                                           BOOLEAN enableSparse,
                                           BOOLEAN useDirectIO )
   :_fileSz( 0 ),
    _pageSz( 0 ),
    _logarithmic( 0 ),
    _flags( DMS_LOBD_FLAG_NULL )
   {
      _fileName.assign( fileName ) ;
      _segmentPages = 0 ;
      _segmentPagesSquare = 0 ;
      ossMemset( _fullPath, 0, sizeof( _fullPath ) ) ;
      if ( useDirectIO )
      {
         _flags |= DMS_LOBD_FLAG_DIRECT ;
      }
      if ( enableSparse )
      {
         _flags |= DMS_LOBD_FLAG_SPARSE ;
      }
   }

   _dmsStorageLobData::~_dmsStorageLobData()
   {
      close() ;
   }

   void _dmsStorageLobData::enableSparse( BOOLEAN sparse )
   {
      if ( sparse )
      {
         _flags |= DMS_LOBD_FLAG_SPARSE ;
      }
      else
      {
         _flags &= ( ~DMS_LOBD_FLAG_SPARSE ) ;
      }
   }

   const CHAR* _dmsStorageLobData::getFileName() const
   {
      return _fileName.c_str() ;
   }

   INT32 _dmsStorageLobData::rename( const CHAR *csName,
                                     const CHAR *suFileName,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      dmsStorageUnitHeader *pHeader = NULL ;

      if ( !isOpened() )
      {
         _fileName = suFileName ;
      }
      else
      {
         CHAR tmpPathFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossStrcpy( tmpPathFile, _fullPath ) ;

         CHAR *pos = ossStrstr( tmpPathFile, _fileName.c_str() ) ;
         if ( !pos )
         {
            PD_LOG( PDERROR, "File full path[%s] is not include su file[%s]",
                    _fullPath, _fileName.c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         *pos = '\0' ;

         rc = cb->allocBuff( sizeof(dmsStorageUnitHeader), &pBuff, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc buff failed, rc: %d", rc ) ;
            goto error ;
         }
         pHeader = ( dmsStorageUnitHeader*)pBuff ;
         /// get header
         rc = _getFileHeader( *pHeader, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get file header failed, rc: %d", rc ) ;
            goto error ;
         }

         utilCatPath( tmpPathFile, OSS_MAX_PATHSIZE, suFileName ) ;

#ifdef _WINDOWS
         /// modify header
         ossStrncpy( pHeader->_name, csName, DMS_SU_NAME_SZ ) ;
         pHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;

         rc = _writeFileHeader( *pHeader, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write file header failed, rc: %d", rc ) ;
            goto error ;
         }
         /// close
         close() ;
         /// rename
         rc = ossRenamePath( _fullPath, tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                    _fullPath, tmpPathFile, rc ) ;
            goto error ;
         }
         /// reopen
         _fileName = suFileName ;
         ossStrcpy( _fullPath, tmpPathFile ) ;
         {
            UINT32 mode = OSS_READWRITE | OSS_SHAREREAD ;
            if ( OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) )
            {
               mode |= OSS_DIRECTIO ;
            }
            rc = ossOpen( _fullPath, mode, OSS_RU|OSS_WU|OSS_RG, _file ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to open file:%s, rc:%d",
                       _fullPath, rc ) ;
               goto error ;
            }
            rc = ossGetFileSize( &_file, &_fileSz ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get size of file:%s, rc:%d",
                       _fileName.c_str(), rc ) ;
               goto error ;
            }
         }
#else
         /// rename filename
         rc = ossRenamePath( _fullPath, tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                    _fullPath, tmpPathFile, rc ) ;
            goto error ;
         }

         _fileName = suFileName ;
         ossStrcpy( _fullPath, tmpPathFile ) ;

         /// modify header
         ossStrncpy( pHeader->_name, csName, DMS_SU_NAME_SZ ) ;
         pHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;

         rc = _writeFileHeader( *pHeader, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write file header failed, rc: %d", rc ) ;
            goto error ;
         }
#endif //_WINDOWS
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_OPEN, "_dmsStorageLobData::open" )
   INT32 _dmsStorageLobData::open( const CHAR *path,
                                   BOOLEAN createNew,
                                   UINT32 totalDataPages,
                                   const dmsStorageInfo &info,
                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_OPEN ) ;
      UINT32 mode = OSS_READWRITE | OSS_SHAREREAD ;
      SDB_ASSERT( path, "path can't be NULL" ) ;
      INT64 fileSize = 0 ;
      INT64 rightSize = 0 ;
      BOOLEAN reGetSize = FALSE ;

      _fileSz = 0 ;

      if ( createNew )
      {
         mode |= OSS_CREATEONLY ;
      }

      if ( OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) )
      {
         mode |= OSS_DIRECTIO ;
      }

      rc = utilBuildFullPath( path, _fileName.c_str(), OSS_MAX_PATHSIZE,
                              _fullPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "path + name is too long: %s, %s",
                  path, _fileName.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = ossOpen( _fullPath, mode, OSS_RU|OSS_WU|OSS_RG, _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc:%d",
                 _fullPath, rc ) ;
         goto error ;
      }

      if ( createNew )
      {
         PD_LOG( PDEVENT, "create lobd file[%s] succeed, mode: 0x%08x",
                 _fullPath, mode ) ;
      }

      rc = ossGetFileSize( &_file, &fileSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get size of file:%s, rc:%d",
                 _fileName.c_str(), rc ) ;
         goto error ;
      }

      if ( 0 == fileSize )
      {
         if ( !createNew )
         {
            PD_LOG ( PDERROR, "lobd file is empty: %s", _fileName.c_str() ) ;
            rc = SDB_DMS_INVALID_SU ;
            goto error ;
         }
         rc = _initFileHeader( info, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to init file header:%s, rc:%d",
                    _fileName.c_str(), rc ) ;
            goto error ;
         }
         // then we get the size again to make sure it's what we need
         rc = ossGetFileSize( &_file, &fileSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get size of file:%s, rc:%d",
                    _fileName.c_str(), rc ) ;
            goto error ;
         }
      }

      if ( fileSize < (INT64)sizeof( dmsStorageUnitHeader ) )
      {
         PD_LOG ( PDERROR, "Invalid storage unit size: %s",
                  _fileName.c_str() ) ;
         PD_LOG ( PDERROR, "Expected more than %d bytes, actually read %lld "
                  "bytes", sizeof( dmsStorageUnitHeader ), fileSize ) ;
         rc = SDB_DMS_INVALID_SU ;
         goto error ;
      }

      rc = _validateFile( info, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to validate file:%s, rc:%d",
                 _fullPath, rc ) ;
         goto error ;
      }

      /// make sure the file size is multiple of segments
      if ( 0 != ( _fileSz - sizeof( dmsStorageUnitHeader ) ) % getSegmentSize() )
      {
         PD_LOG ( PDWARNING, "Unexpected length[%llu] of file: %s", _fileSz,
                  _fileName.c_str() ) ;

         /// need to truncate the file
         rightSize = ( ( _fileSz - sizeof( dmsStorageUnitHeader ) ) /
                       getSegmentSize() ) * getSegmentSize() +
                       sizeof( dmsStorageUnitHeader ) ;
         rc = ossTruncateFile( &_file, (INT64)rightSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                    _fileName.c_str(), rightSize, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Truncate file[%s] to size[%llu] succeed",
                 _fileName.c_str(), rightSize ) ;
         // then we get the size again to make sure it's what we need
         rc = ossGetFileSize( &_file, &_fileSz ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _fileName.c_str(), rc ) ;
            goto error ;
         }
      }

      rightSize = (INT64)totalDataPages * _pageSz +
                  sizeof( dmsStorageUnitHeader ) ;
      /// make sure the file is correct with meta data
      if ( _fileSz > rightSize )
      {
         PD_LOG( PDWARNING, "File[%s] size[%llu] is grater than storage "
                 "data size[%llu]", _fileName.c_str(), _fileSz,
                 rightSize ) ;

         rc = ossTruncateFile( &_file, rightSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Truncate file[%s] to size[%lld] failed, rc: %d",
                    _fileName.c_str(), rightSize, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Truncate file[%s] to size[%lld] succeed",
                 _fileName.c_str(), rightSize ) ;
         reGetSize = TRUE ;
      }
      else if ( _fileSz < rightSize )
      {
         PD_LOG( PDWARNING, "File[%s] size[%llu] is less than storage "
                 "data size[%llu]", _fileName.c_str(), _fileSz,
                 rightSize ) ;

         INT64 extentSize = rightSize - _fileSz ;
         INT64 tmpFileSz = _fileSz ;
         rc = extend( extentSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Extend file[%s] to size[%lld] from size[%lld] "
                    "failed, rc: %d", _fileName.c_str(), rightSize,
                    tmpFileSz, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Extend file[%s] to size[%lld] from size[%lld] "
                 "succeed", _fileName.c_str(), rightSize, tmpFileSz ) ;
         reGetSize = TRUE ;
      }

      if ( reGetSize )
      {
         // then we get the size again to make sure it's what we need
         rc = ossGetFileSize( &_file, &_fileSz ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _fileName.c_str(), rc ) ;
            goto error ;
         }
         reGetSize = FALSE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_OPEN, rc ) ;
      return rc ;
   error:
      close() ;
      _postOpen( rc ) ;
      goto done ;
   }

   BOOLEAN _dmsStorageLobData::isOpened()const
   {
      return _file.isOpened() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_CLOSE, "_dmsStorageLobData::close" )
   INT32 _dmsStorageLobData::close()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_CLOSE ) ;
      if ( _file.isOpened() )
      {
         rc = ossClose( _file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close file:%s, rc:%d",
                    _fullPath, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_CLOSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_PREPAREWRITE, "_dmsStorageLobData::prepareWrite" )
   INT32 _dmsStorageLobData::prepareWrite( INT32 pageID,
                                           const CHAR *pData,
                                           UINT32 len,
                                           UINT32 offset,
                                           IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_PREPAREWRITE ) ;

      _dmsLobDirectOutBuffer buffer( pData, len, offset, isDirectIO(),
                                     cb, pageID, 0, this ) ;
      rc = buffer.prepare() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Prepare for write failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_PREPAREWRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_WRITE, "_dmsStorageLobData::write" )
   INT32 _dmsStorageLobData::write( INT32 pageID, const CHAR *pData,
                                    UINT32 len, UINT32 offset,
                                    UINT32 newestMask,
                                    IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_WRITE ) ;
      SDB_ASSERT( DMS_LOB_INVALID_PAGEID != pageID &&
                  NULL != pData &&
                  len + offset <= _pageSz, "invalid operation" ) ;

      _dmsLobDirectOutBuffer buffer( pData, len, offset, isDirectIO(),
                                     cb, pageID, newestMask, this ) ;
      const _dmsLobDirectBuffer::tuple *t = NULL ;

      INT64 written = 0 ;
      INT64 writeOffset = 0 ;

      rc = buffer.doit( &t ) ;
      if ( rc )
      {
         goto error ;
      }

      writeOffset = getSeek( pageID, t->offset ) ;
      if ( writeOffset + t->size > _fileSz )
      {
         PD_LOG( PDERROR, "Offset[%lld] grater than file size[%lld] in "
                 "file[%s]", t->offset, _fileSz, _fileName.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeekAndWriteN( &_file, writeOffset,
                             t->buf, t->size,
                             written ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write data, page:%d, rc:%d",
                 pageID, rc ) ;
         goto error ;
      }

      buffer.done() ;

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_WRITERAW, "_dmsStorageLobData::writeRaw" )
   INT32 _dmsStorageLobData::writeRaw( INT64 offset, const CHAR *pData,
                                       UINT32 len, IExecutor *cb,
                                       BOOLEAN isAligned,
                                       UINT32 newestMask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_WRITERAW ) ;
      SDB_ASSERT( NULL != pData && offset >= 0, "invalid operation" ) ;

      INT32  pageID = -1 ;
      if ( offset >= (INT64)sizeof( _dmsStorageUnitHeader ) )
      {
         offset -= sizeof( _dmsStorageUnitHeader ) ;
         pageID = offset >> _logarithmic ;
         offset %= _pageSz ;
      }

      BOOLEAN needAligned = FALSE ;
      if ( isDirectIO() && !isAligned )
      {
         needAligned = TRUE ;
      }

      _dmsLobDirectOutBuffer buffer( pData, len, offset,
                                     needAligned, cb, pageID,
                                     newestMask, this ) ;
      const _dmsLobDirectBuffer::tuple *t = NULL ;
      INT64 writeOffset = 0 ;
      INT64 written = 0 ;

      rc = buffer.doit( &t ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( pageID >= 0 )
      {
         writeOffset = getSeek( pageID, t->offset ) ;
      }
      else
      {
         writeOffset = offset ;
      }

      if ( (INT64)( writeOffset + t->size ) > _fileSz )
      {
         PD_LOG( PDERROR, "Offset[%lld] grater than file size[%lld] in "
                 "file[%s]", offset, _fileSz, _fileName.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeekAndWriteN( &_file, writeOffset,
                             t->buf, t->size,
                             written ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write data, offset:%lld, len:%d, rc:%d",
                 offset, len, rc ) ;
         goto error ;
      }

      buffer.done() ;

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_WRITERAW, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_PREPAREREAD, "_dmsStorageLobData::prepareRead" )
   INT32 _dmsStorageLobData::prepareRead( INT32 pageID,
                                          CHAR *pData,
                                          UINT32 len,
                                          UINT32 offset,
                                          IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_PREPAREREAD ) ;

      _dmsLobDirectInBuffer buffer( pData, len, offset,
                                    isDirectIO(), cb ) ;
      rc = buffer.prepare() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Prepare for read failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_PREPAREREAD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_READ, "_dmsStorageLobData::read" )
   INT32 _dmsStorageLobData::read( INT32 pageID, CHAR *pData,
                                   UINT32 len, UINT32 offset,
                                   UINT32 &readLen,
                                   IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_READ ) ;
      SDB_ASSERT( DMS_LOB_INVALID_PAGEID != pageID &&
                  NULL != pData &&
                  len + offset <= _pageSz, "invalid operation" ) ;

      SINT64 readFromFile = 0 ;
      INT64 readOffset = 0 ;
      _dmsLobDirectInBuffer buffer( pData, len, offset,
                                    isDirectIO(), cb ) ;
      const _dmsLobDirectBuffer::tuple *t = NULL ;

      rc = buffer.doit( &t ) ;
      if ( rc )
      {
         goto error ;
      }

      readOffset = getSeek( pageID, t->offset ) ;
      if ( readOffset + t->size > _fileSz )
      {
         PD_LOG( PDERROR, "Offset[%lld] grater than file size[%lld] in "
                 "file[%s]", readOffset, _fileSz, _fileName.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeekAndReadN( &_file, readOffset,
                            t->size, t->buf,
                            readFromFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read page[%d], rc: %d",
                 pageID, rc ) ;
         goto error ;
      }

      buffer.done() ;
      readLen = len ;

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_READ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_READRAW, "_dmsStorageLobData::readRaw" )
   INT32 _dmsStorageLobData::readRaw( INT64 offset, UINT32 len, CHAR *buf,
                                      UINT32 &readLen, IExecutor *cb,
                                      BOOLEAN isAligned )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_READRAW ) ;
      SDB_ASSERT( NULL != buf && offset >= 0, "invalid arguments" ) ;

      SINT64 readFromFile = 0 ;
      INT32  pageID = -1 ;
      if ( offset >= (INT64)sizeof( _dmsStorageUnitHeader ) )
      {
         offset -= sizeof( _dmsStorageUnitHeader ) ;
         pageID = offset >> _logarithmic ;
         offset %= _pageSz ;
      }

      BOOLEAN needAligned = FALSE ;
      if ( isDirectIO() && !isAligned )
      {
         needAligned = TRUE ;
      }
      _dmsLobDirectInBuffer buffer( buf, len, offset,
                                    needAligned, cb ) ;
      const _dmsLobDirectBuffer::tuple *t = NULL ;
      INT64 readOffset = 0 ;

      rc = buffer.doit( &t ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( pageID >= 0 )
      {
         readOffset = getSeek( pageID, t->offset ) ;
      }
      else
      {
         readOffset = offset ;
      }

      if ( ( SINT64 )(readOffset + t->size) > _fileSz )
      {
         PD_LOG( PDERROR, "Offset[%lld] grater than file size[%lld] in "
                 "file[%s]", readOffset, _fileSz, _fileName.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeekAndReadN( &_file, readOffset,
                            t->size, t->buf,
                            readFromFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read data[offset: %lld, len: %d], rc: %d",
                 readOffset, t->size, rc ) ;
         goto error ;
      }

      buffer.done() ;
      readLen = readFromFile ;

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_READRAW, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_EXTEND, "_dmsStorageLobData::extend" )
   INT32 _dmsStorageLobData::extend( INT64 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_EXTEND ) ;
      do
      {
         rc = _extend( len ) ;
         if ( SDB_INVALIDARG == rc &&
              OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_SPARSE ) )
         {
            PD_LOG( PDWARNING, "this filesystem may not support sparse file"
                    ", we should try again" ) ;
            OSS_BIT_CLEAR( _flags, DMS_LOBD_FLAG_SPARSE ) ;
            rc = SDB_OK ;
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extend file:%d", rc ) ;
            goto error ;
         }
         else
         {
            break ;
         }

      } while (  TRUE ) ;
   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_EXTEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA__EXTEND, "_dmsStorageLobData::_extend" )
   INT32 _dmsStorageLobData::_extend( INT64 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA__EXTEND ) ;
      SDB_ASSERT( 0 < len, "invalid extend size" ) ;
      SDB_ASSERT( 0 == len % OSS_FILE_DIRECT_IO_ALIGNMENT, "impossible" ) ;
      OSSFILE file ;
      UINT32 mode = OSS_READWRITE | OSS_SHAREREAD  ;
      /// free in done.
      CHAR *extendBuf = NULL ;
      UINT32 bufSize = DMS_LOBD_EXTEND_LEN ;

      if ( OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) )
      {
         mode |= OSS_DIRECTIO ;
      }

      rc = ossOpen( _fullPath, mode, OSS_RU|OSS_WU|OSS_RG, file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file when extend:%d", rc ) ;
         goto error ;
      }

#ifdef _DEBUG
      {
         SINT64 sizeBeforeExtend = 0 ;
         rc = ossGetFileSize( &file, &sizeBeforeExtend ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get size of file:%s, rc:%d",
                    _fileName.c_str(), rc ) ;
            goto error ;
         }

         if ( 0 != _fileSz &&
              ( sizeBeforeExtend - sizeof( _dmsStorageUnitHeader ) ) %
              getSegmentSize() != 0 )
         {
            PD_LOG( PDERROR, "invalid file size:%lld, file:%s",
                    sizeBeforeExtend, _fileName.c_str() ) ;
            rc = SDB_SYS ;
            SDB_ASSERT( FALSE, "impossible" ) ;
            goto error ;
         }
      }
#endif // _DEBUG

      if ( !OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_SPARSE ) &&
           !OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) )
      {
         rc = ossExtendFile( &file, len ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extend file:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         SINT64 extendSize = len ;

         rc = ossSeek( &file, 0, OSS_SEEK_END ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to seek to the end of file:%d", rc ) ;
            goto error ;
         }

         if ( OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_SPARSE ) )
         {
            rc = ossSeek( &file,
                          len - OSS_FILE_DIRECT_IO_ALIGNMENT,
                          OSS_SEEK_CUR ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to seek offset[%lld](OSS_SEEK_CUR), rc:%d",
                       len - OSS_FILE_DIRECT_IO_ALIGNMENT, rc ) ;
               goto error ;
            }
            else
            {
               /// we only need to write some bytes at the end of file.
               bufSize = OSS_FILE_DIRECT_IO_ALIGNMENT ;
               extendSize = OSS_FILE_DIRECT_IO_ALIGNMENT ;
            }
         }

         if ( OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) )
         {
            extendBuf = ( CHAR * )ossAlignedAlloc(
                                          OSS_FILE_DIRECT_IO_ALIGNMENT,
                                          bufSize ) ;
         }
         else
         {
            extendBuf = (CHAR*) SDB_OSS_MALLOC ( bufSize ) ;
         }

         if ( NULL == extendBuf )
         {
            PD_LOG( PDERROR, "failed to allcate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( extendBuf, 0, bufSize ) ;

         do
         {
            SINT64 writeSize = bufSize <= extendSize ?
                               bufSize : extendSize ;
            rc = ossWriteN( &file, extendBuf, writeSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write file:%d", rc ) ;
               goto error ;
            }
            extendSize -= writeSize ;
         } while ( 0 < extendSize ) ;
      }

#ifdef _DEBUG
      {
         SINT64 sizeAfterExtend = 0 ;
         rc = ossGetFileSize( &file, &sizeAfterExtend ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get size of file:%s, rc:%d",
                    _fileName.c_str(), rc ) ;
            goto error ;
         }

         if ( ( sizeAfterExtend - sizeof( _dmsStorageUnitHeader ) ) %
              getSegmentSize() != 0 )
         {
            PD_LOG( PDERROR, "invalid file size:%lld, file:%s",
                    sizeAfterExtend, _fileName.c_str() ) ;
            rc = SDB_SYS ;
            SDB_ASSERT( FALSE, "impossible" ) ;
            goto error ;
         }
      }
#endif // _DEBUG
      _fileSz += len ;

   done:
      if ( file.isOpened() )
      {
         INT32 rcTmp = SDB_OK ;
         rcTmp = ossClose( file ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to close file after extend:%d", rcTmp ) ;
         }
      }

      if ( NULL != extendBuf )
      {
         if ( OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) )
         {
            SDB_OSS_ORIGINAL_FREE( extendBuf ) ;
         }
         else
         {
            SDB_OSS_FREE( extendBuf ) ;
         }
      }
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA__EXTEND, rc ) ;
      return rc ;
   truncate:
      {
         INT32 rcTmp = SDB_OK ;
         rcTmp = ossTruncateFile( &file, _fileSz ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDSEVERE, "Failed to revert the increase of segment, "
                     "rc = %d", rcTmp ) ;
            ossPanic() ;
         }
         goto done ;
      }
   error:
      {
         if ( !file.isOpened() )
         {
            goto done ;
         }
         else
         {
            SINT64 nowSize = 0 ;
            INT32 rcTmp = ossGetFileSize( &file, &nowSize ) ;
            if ( SDB_OK != rcTmp )
            {
               PD_LOG( PDERROR, "failed to get file size:%d", rcTmp ) ;
               goto truncate ;
            }
            else if ( nowSize != _fileSz )
            {
               goto truncate ;
            }
            else
            {
               goto done ;
            }
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_REMOVE, "_dmsStorageLobData::remove" )
   INT32 _dmsStorageLobData::remove()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_REMOVE ) ;

      if ( _fullPath[ 0 ] == 0 )
      {
         goto done ;
      }

      rc = close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close file:%s, rc:%d",
                 _fileName.c_str(), rc ) ;
         goto error ;
      }

      rc = ossDelete( _fullPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove file:%s, rc:%d",
                 _fullPath, rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "remove file:%s", _fullPath ) ;
      _fullPath[ 0 ] = 0 ;

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_REMOVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageLobData::_postOpen( INT32 cause )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_DMS_INVALID_SU == cause &&
           _fullPath[0] != '\0' )
      {
         CHAR tmpFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossSnprintf( tmpFile, OSS_MAX_PATHSIZE, "%s.err.%u",
                      _fullPath, ossGetCurrentProcessID() ) ;
         if ( SDB_OK == ossAccess( tmpFile, 0 ) )
         {
            rc = ossDelete( tmpFile ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Remove file[%s] failed, rc: %d",
                       tmpFile, rc ) ;
               goto error ;
            }
         }
         /// rename the file
         rc = ossRenamePath( _fullPath, tmpFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to [%s] failed, rc: %d",
                    _fullPath, tmpFile, rc ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Rename file[%s] to [%s] succeed",
                    _fullPath, tmpFile ) ;
         }
      }

   done:
      return cause ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA__INITFILEHEADER, "_dmsStorageLobData::_initFileHeader" )
   INT32 _dmsStorageLobData::_initFileHeader( const dmsStorageInfo &info,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA__INITFILEHEADER ) ;
      CHAR *pBuff = NULL ;
      dmsStorageUnitHeader *pHeader = NULL ;

      rc = cb->allocBuff( sizeof( dmsStorageUnitHeader ),
                          &pBuff, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc memory[size:%d] failed, rc: %d",
                 sizeof( dmsStorageUnitHeader ), rc ) ;
         goto error ;
      }

      pHeader = ( dmsStorageUnitHeader* )pBuff ;
      pHeader->reset() ;
      /// Init
      ossStrncpy( pHeader->_eyeCatcher, DMS_LOBD_EYECATCHER,
                  DMS_LOBD_EYECATCHER_LEN ) ;
      pHeader->_version = DMS_LOB_CUR_VERSION ;
      pHeader->_pageSize = 0 ;
      pHeader->_storageUnitSize = 0 ;
      ossStrncpy ( pHeader->_name, info._suName, DMS_SU_NAME_SZ ) ;
      pHeader->_sequence = info._sequence ;
      pHeader->_numMB    = 0 ;
      pHeader->_MBHWM    = 0 ;
      pHeader->_pageNum  = 0 ;
      pHeader->_secretValue = info._secretValue ;

      /// extend and write
      rc = extend( sizeof( dmsStorageUnitHeader ) ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to extend header, rc: %d", rc ) ;
         goto error ;
      }

      rc = writeRaw( 0, pBuff, sizeof( dmsStorageUnitHeader ),
                     cb, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write header, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA__INITFILEHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA__VALIDATEFILE, "_dmsStorageLobData::_validateFile" )
   INT32 _dmsStorageLobData::_validateFile( const dmsStorageInfo &info,
                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA__VALIDATEFILE ) ;
      CHAR *pData = NULL ;
      dmsStorageUnitHeader *pHeader = NULL ;

      rc = cb->allocBuff( sizeof(dmsStorageUnitHeader), &pData ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc memory[%d] failed, rc: %d",
                 sizeof(dmsStorageUnitHeader), rc ) ;
         goto error ;
      }
      pHeader = (dmsStorageUnitHeader*)pData ;

      rc = ossGetFileSize( &_file, &_fileSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get size of file:%s, rc:%d",
                 _fileName.c_str(), rc ) ;
         goto error ;
      }

      rc = _getFileHeader( *pHeader, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get file header, rc:%d", rc ) ;
         goto error ;
      }

      if ( 0 != ossStrncmp( pHeader->_eyeCatcher, DMS_LOBD_EYECATCHER,
                            DMS_LOBD_EYECATCHER_LEN ) )
      {
         CHAR szTmp[ DMS_HEADER_EYECATCHER_LEN + 1 ] = {0} ;
         ossStrncpy( szTmp, pHeader->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;
         PD_LOG( PDERROR, "invalid eye catcher:%s, file:%s",
                 szTmp, _fileName.c_str() ) ;
         rc = SDB_INVALID_FILE_TYPE ;
         goto error ;
      }

      if ( pHeader->_version > DMS_LOB_CUR_VERSION )
      {
         PD_LOG( PDERROR, "invalid version of header:%d, file:%s",
                 pHeader->_version, _fileName.c_str() ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
         goto error ;
      }

      if ( 0 != pHeader->_pageSize ||
           0 != pHeader->_storageUnitSize ||
           0 != pHeader->_numMB ||
           0 != pHeader->_MBHWM ||
           0 != pHeader->_pageNum )
      {
         PD_LOG( PDERROR, "invalid field value which not in used" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( 0 != ossStrncmp ( info._suName, pHeader->_name,
                             DMS_SU_NAME_SZ ) )
      {
         CHAR szTmp[ DMS_SU_NAME_SZ + 1 ] = {0} ;
         ossStrncpy( szTmp, pHeader->_name, DMS_SU_NAME_SZ ) ;
         PD_LOG( PDERROR, "invalid su name:%s in file:%s",
                 szTmp, _fileName.c_str() ) ;
         rc = SDB_SYS ;
      }

      if ( info._sequence != pHeader->_sequence )
      {
         PD_LOG( PDERROR, "invalid sequence:%d != %d in file:%s",
                 pHeader->_sequence, info._sequence, _fileName.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( info._secretValue != pHeader->_secretValue )
      {
         PD_LOG( PDERROR, "invalid secret value: %lld, self: %lld, file:%s",
                 info._secretValue, pHeader->_secretValue, _fileName.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _pageSz = info._lobdPageSize ;
      if ( !ossIsPowerOf2( _pageSz, &_logarithmic ) )
      {
         PD_LOG( PDERROR, "Page size[%d] is not power of 2", _pageSz ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _segmentPages = getSegmentSize() >> _logarithmic ;
      if ( !ossIsPowerOf2( _segmentPages, &_segmentPagesSquare ) )
      {
         PD_LOG( PDERROR, "Segment pages[%u] must be the power of 2",
                 _segmentPages ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( pData )
      {
         cb->releaseBuff( pData ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA__VALIDATEFILE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA__FETFILEHEADER, "_dmsStorageLobData::_getFileHeader" )
   INT32 _dmsStorageLobData::_getFileHeader( _dmsStorageUnitHeader &header,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA__FETFILEHEADER ) ;
      UINT32 readLen = 0 ;

      rc = readRaw( 0, sizeof( header ), (CHAR *)&header, readLen,
                    cb, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Read header failed, rc: %d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( sizeof( _dmsStorageUnitHeader ) == readLen, "impossible" ) ;

   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA__FETFILEHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSLOBDATA_WRFILEHEADER, "_dmsStorageLobData::_writeFileHeader" )
   INT32 _dmsStorageLobData::_writeFileHeader( const _dmsStorageUnitHeader &header,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSLOBDATA_WRFILEHEADER ) ;

      rc = writeRaw( 0, (const CHAR *)&header, sizeof( header ),
                     cb, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write header, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSLOBDATA_WRFILEHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTORAGELOBDATA_FLUSH, "_dmsStorageLobData::flush" )
   INT32 _dmsStorageLobData::flush()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSSTORAGELOBDATA_FLUSH ) ;
      if ( isOpened() )
      {
         rc = ossFsync( &_file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fsync file[%s], rc:%d",
                    _fileName.c_str(), rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_DMSSTORAGELOBDATA_FLUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

