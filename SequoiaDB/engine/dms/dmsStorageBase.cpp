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

   Source File Name = dmsStorageBase.cpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageBase.hpp"
#include "dmsStorageData.hpp"
#include "dmsStorageJob.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdStartup.hpp"
#include "utilStr.hpp"
#include "pmdEnv.hpp"

using namespace bson ;

namespace engine
{

   #define DMS_SME_FREE_STR             "Free"
   #define DMS_SME_ALLOCATED_STR        "Occupied"

   void smeMask2String( CHAR state, CHAR * pBuffer, INT32 buffSize )
   {
      SDB_ASSERT( DMS_SME_FREE == state || DMS_SME_ALLOCATED == state,
                  "SME Mask must be 1 or 0" ) ;
      SDB_ASSERT( pBuffer && buffSize > 0 , "Buffer can not be NULL" ) ;

      if ( DMS_SME_FREE == state )
      {
         ossStrncpy( pBuffer, DMS_SME_FREE_STR, buffSize - 1 ) ;
      }
      else
      {
         ossStrncpy( pBuffer, DMS_SME_ALLOCATED_STR, buffSize - 1 ) ;
      }
      pBuffer[ buffSize - 1 ] = 0 ;
   }

   /*
      _dmsDirtyList implement
   */
   _dmsDirtyList::_dmsDirtyList()
   :_dirtyBegin( 0x7FFFFFFF ), _dirtyEnd( 0 )
   {
      _pData = NULL ;
      _capacity = 0 ;
      _size  = 0 ;
      _fullDirty = FALSE ;
   }

   _dmsDirtyList::~_dmsDirtyList()
   {
      destory() ;
   }

   INT32 _dmsDirtyList::init( UINT32 capacity )
   {
      INT32 rc = SDB_OK ;
      UINT32 arrayNum = 0 ;

      SDB_ASSERT( capacity > 0 , "Capacity must > 0" ) ;

      if ( capacity <= _capacity )
      {
         cleanAll() ;
         goto done ;
      }
      destory() ;

      arrayNum = ( capacity + 7 ) >> 3 ;
      _pData = ( CHAR* )SDB_OSS_MALLOC( arrayNum ) ;
      if ( !_pData )
      {
         PD_LOG( PDERROR, "Alloc dirty list failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( _pData, 0, arrayNum ) ;
      _capacity = arrayNum << 3 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsDirtyList::destory()
   {
      if ( _pData )
      {
         SDB_OSS_FREE( _pData ) ;
         _pData = NULL ;
      }
      _capacity = 0 ;
      _size = 0 ;
      _fullDirty = FALSE ;

      _dirtyBegin.init( 0x7FFFFFFF ) ;
      _dirtyEnd.init( 0 ) ;
   }

   void _dmsDirtyList::setSize( UINT32 size )
   {
      SDB_ASSERT( size <= _capacity, "Size must <= Capacity" ) ;
      _size = size <= _capacity ? size : _capacity ;
   }

   void _dmsDirtyList::cleanAll()
   {
      _dirtyBegin.init( 0x7FFFFFFF ) ;
      _dirtyEnd.init( 0 ) ;
      _fullDirty = FALSE ;

      UINT32 arrayNum = ( _size + 7 ) >> 3 ;
      for ( UINT32 i = 0 ; i < arrayNum ; ++i )
      {
         if ( _pData[ i ] != 0 )
         {
            _pData[ i ] = 0 ;
         }
      }
   }

   INT32 _dmsDirtyList::nextDirtyPos( UINT32 &fromPos ) const
   {
      INT32 pos = -1 ;

      while ( fromPos < _size )
      {
         if ( !_fullDirty && 0 == ( fromPos & 7 ) &&
              0 == _pData[ fromPos >> 3 ] )
         {
            fromPos += 8 ;
         }
         else if ( !isDirty( fromPos ) )
         {
            ++fromPos ;
         }
         else
         {
            pos = fromPos++ ;
            break ;
         }
      }
      return pos ;
   }

   UINT32 _dmsDirtyList::dirtyNumber() const
   {
      UINT32 dirtyNum = 0 ;

      if ( _fullDirty )
      {
         dirtyNum = _size ;
      }
      else
      {
         UINT32 beginPos = _dirtyBegin.peek() ;
         UINT32 endPos = _dirtyEnd.peek() + 1 ;

         while ( beginPos < endPos )
         {
            if ( 0 == ( beginPos & 7 ) &&
                 0 == _pData[ beginPos >> 3 ] )
            {
               beginPos += 8 ;
            }
            else
            {
               if ( isDirty( beginPos ) )
               {
                  ++dirtyNum ;
               }
               ++beginPos ;
            }
         }
      }
      return dirtyNum ;
   }

   UINT32 _dmsDirtyList::dirtyGap() const
   {
      if ( _fullDirty )
      {
         return _size ;
      }
      else
      {
         UINT32 beginPos = _dirtyBegin.peek() ;
         UINT32 endPos = _dirtyEnd.peek() ;
         if ( beginPos > endPos )
         {
            return 0 ;
         }
         return endPos - beginPos + 1 ;
      }
   }

   /*
      _dmsExtRW implement
   */
   _dmsExtRW::_dmsExtRW()
   {
      _extentID = -1 ;
      _collectionID = -1 ;
      _attr = 0 ;
      _pBase = NULL ;
      _ptr   = ( ossValuePtr ) 0 ;
   }

   _dmsExtRW::~_dmsExtRW()
   {
      if ( _pBase && isDirty() )
      {
         _pBase->markDirty( _collectionID, _extentID, DMS_CHG_AFTER ) ;
      }
   }

   BOOLEAN _dmsExtRW::isEmpty() const
   {
      return _pBase ? FALSE : TRUE ;
   }

   void _dmsExtRW::setNothrow( BOOLEAN nothrow )
   {
      if ( nothrow )
      {
         _attr |= DMS_RW_ATTR_NOTHROW ;
      }
      else
      {
         OSS_BIT_CLEAR( _attr, DMS_RW_ATTR_NOTHROW ) ;
      }
   }

   BOOLEAN _dmsExtRW::isNothrow() const
   {
      return ( _attr & DMS_RW_ATTR_NOTHROW ) ? TRUE : FALSE ;
   }

   const CHAR* _dmsExtRW::readPtr( UINT32 offset, UINT32 len )
   {
      if ( (ossValuePtr)0 == _ptr )
      {
         std::string text = "Point is NULL: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_SYS ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_SYS, text ) ;
      }
      return ( const CHAR* )_ptr + offset ;
   }

   CHAR* _dmsExtRW::writePtr( UINT32 offset, UINT32 len )
   {
      if ( (ossValuePtr)0 == _ptr )
      {
         std::string text = "Point is NULL: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_SYS ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_SYS, text ) ;
      }
      _markDirty() ;
      _pBase->markDirty( _collectionID, _extentID, DMS_CHG_BEFORE ) ;
      return ( CHAR* )_ptr + offset ;
   }

   _dmsExtRW _dmsExtRW::derive( INT32 extentID )
   {
      if ( _pBase )
      {
         return _pBase->extent2RW( extentID, _collectionID ) ;
      }
      return _dmsExtRW() ;
   }

   BOOLEAN _dmsExtRW::isDirty() const
   {
      return ( _attr & DMS_RW_ATTR_DIRTY ) ? TRUE : FALSE ;
   }

   void _dmsExtRW::_markDirty()
   {
      _attr |= DMS_RW_ATTR_DIRTY ;
   }

   std::string _dmsExtRW::toString() const
   {
      std::stringstream ss ;
      ss << "ExtentRW(" << _collectionID << "," << _extentID << ")" ;
      return ss.str() ;
   }

   #define DMS_EXTEND_THRESHOLD_SIZE      ( 33554432 )   // 32MB

   /*
      Sync Config Default Value
   */
   #define DMS_SYNC_RECORDNUM_DFT         ( 0 )
   #define DMS_SYNC_DIRTYRATIO_DFT        ( 50 )
   #define DMS_SYNC_INTERVAL_DFT          ( 10000 )
   #define DMS_SYNC_NOWRITE_DFT           ( 5000 )

   /*
      _dmsStorageBase : implement
   */
   _dmsStorageBase::_dmsStorageBase( const CHAR *pSuFileName,
                                     dmsStorageInfo *pInfo )
   {
      SDB_ASSERT( pSuFileName, "SU file name can't be NULL" ) ;

      _pStorageInfo       = pInfo ;
      _dmsHeader          = NULL ;
      _dmsSME             = NULL ;
      _dataSegID          = 0 ;

      _pageNum            = 0 ;
      _maxSegID           = -1 ;
      _segmentPages       = 0 ;
      _segmentPagesSquare = 0 ;
      _pageSizeSquare     = 0 ;
      _isTempSU           = FALSE ;
      _blockScanSupport   = TRUE ;
      _pageSize           = 0 ;
      _lobPageSize        = 0 ;

      ossStrncpy( _suFileName, pSuFileName, DMS_SU_FILENAME_SZ ) ;
      _suFileName[ DMS_SU_FILENAME_SZ ] = 0 ;
      ossMemset( _fullPathName, 0, sizeof(_fullPathName) ) ;

      if ( 0 == ossStrcmp( pInfo->_suName, SDB_DMSTEMP_NAME ) )
      {
         _isTempSU = TRUE ;
         _blockScanSupport = FALSE ;
      }

      _pSyncMgr           = NULL ;
      _isClosed           = TRUE ;
      _commitFlag         = 0 ;
      _isCrash            = FALSE ;
      _forceSync          = FALSE ;

      _syncInterval       = DMS_SYNC_INTERVAL_DFT ;
      _syncRecordNum      = DMS_SYNC_RECORDNUM_DFT ;
      _syncDirtyRatio     = DMS_SYNC_DIRTYRATIO_DFT ;
      _syncNoWriteTime    = DMS_SYNC_NOWRITE_DFT ;
      _syncDeep           = FALSE ;

      _lastWriteTick      = 0 ;
      _writeReordNum      = 0 ;
      _lastSyncTime       = 0 ;
      _syncEnable         = TRUE ;
   }

   _dmsStorageBase::~_dmsStorageBase()
   {
      SDB_ASSERT( !ossMmapFile::_opened, "Must Call closeStorage before "
                  "delete the object" ) ;
      closeStorage() ;
      _pStorageInfo = NULL ;
      _dirtyList.destory() ;
   }

   BOOLEAN _dmsStorageBase::isClosed() const
   {
      return _isClosed ;
   }

   void _dmsStorageBase::setSyncConfig( UINT32 syncInterval,
                                        UINT32 syncRecordNum,
                                        UINT32 syncDirtyRatio )
   {
      _syncInterval = syncInterval ;
      _syncRecordNum = syncRecordNum ;
      _syncDirtyRatio = syncDirtyRatio ;
   }

   void _dmsStorageBase::setSyncDeep( BOOLEAN syncDeep )
   {
      _syncDeep = syncDeep ;
   }

   void _dmsStorageBase::setSyncNoWriteTime( UINT32 millsec )
   {
      _syncNoWriteTime = millsec ;
   }

   BOOLEAN _dmsStorageBase::isSyncDeep() const
   {
      return _syncDeep ;
   }

   UINT32 _dmsStorageBase::getSyncInterval() const
   {
      return _syncInterval ;
   }

   UINT32 _dmsStorageBase::getSyncRecordNum() const
   {
      return _syncRecordNum ;
   }

   UINT32 _dmsStorageBase::getSyncDirtyRatio() const
   {
      return _syncDirtyRatio ;
   }

   UINT32 _dmsStorageBase::getSyncNoWriteTime() const
   {
      return _syncNoWriteTime ;
   }

   UINT32 _dmsStorageBase::getCommitFlag() const
   {
      if ( _dmsHeader )
      {
         return _dmsHeader->_commitFlag ;
      }
      return 1 ;
   }

   UINT64 _dmsStorageBase::getCommitLSN() const
   {
      if ( _dmsHeader )
      {
         return _dmsHeader->_commitLsn ;
      }
      return 0 ;
   }

   UINT64 _dmsStorageBase::getCommitTime() const
   {
      if ( _dmsHeader )
      {
         return _dmsHeader->_commitTime ;
      }
      return 0 ;
   }

   void _dmsStorageBase::restoreForCrash()
   {
      _isCrash = FALSE ;
      _forceSync = TRUE ;
      _onRestore() ;
   }

   BOOLEAN _dmsStorageBase::isCrashed() const
   {
      return _isCrash ;
   }

   void _dmsStorageBase::enableSync( BOOLEAN enable )
   {
      lock() ;
      _syncEnable = enable ;
      unlock() ;
   }

   BOOLEAN _dmsStorageBase::canSync( BOOLEAN &force ) const
   {
      force = FALSE ;
      UINT64 oldestTick = _getOldestWriteTick() ;
      UINT64 oldTimeSpan = 0 ;

      if ( !_syncEnable )
      {
         return FALSE ;
      }
      else if ( _forceSync )
      {
         force = TRUE ;
         return TRUE ;
      }

      if ( (UINT64)~0 != oldestTick )
      {
         oldTimeSpan = pmdGetTickSpanTime( oldestTick ) ;
      }

      if ( 0 != _commitFlag && _dirtyList.dirtyNumber() <= 0 )
      {
         return FALSE ;
      }
      else if ( _syncInterval > 0 &&
                oldTimeSpan >= _syncNoWriteTime &&
                oldTimeSpan > 60 * _syncInterval )
      {
         force = TRUE ;
         return TRUE ;
      }
      else if ( _syncRecordNum > 0 && _writeReordNum >= _syncRecordNum )
      {
         PD_LOG( PDDEBUG, "Write record number[%u] more than threshold[%u]",
                 _writeReordNum, _syncRecordNum ) ;
         force = TRUE ;
         return TRUE ;
      }
      else if ( pmdGetTickSpanTime( _lastWriteTick ) < _syncNoWriteTime )
      {
         return FALSE ;
      }
      else if ( _syncInterval > 0 )
      {
         ossTimestamp tm ;
         ossGetCurrentTime( tm ) ;
         UINT64 curTime = tm.time * 1000 + tm.microtm / 1000 ;

         if ( curTime - _lastSyncTime >= _syncInterval )
         {
            PD_LOG( PDDEBUG, "Time interval threshold tiggered, "
                    "CurTime:%llu, LastSyncTime:%llu, SyncInterval:%u",
                    curTime, _lastSyncTime, _syncInterval ) ;
            return TRUE ;
         }
      }
      return FALSE ;
   }

   void _dmsStorageBase::lock()
   {
      _persistLatch.get() ;
   }

   void _dmsStorageBase::unlock()
   {
      _persistLatch.release() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE_SYNC, "_dmsStorageBase::sync" )
   INT32 _dmsStorageBase::sync( BOOLEAN force,
                                BOOLEAN sync,
                                IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE_SYNC ) ;
      ossTimestamp t ;
      UINT32 num = 0 ;

      if ( !_syncEnable )
      {
         goto done ;
      }

      ossGetCurrentTime( t ) ;
      _lastSyncTime = t.time * 1000 + t.microtm / 1000 ;

      _commitFlag = 1 ;
      _forceSync = FALSE ;

      if ( _dirtyList.isFullDirty() )
      {
         num = 1 ;
         rc = flushAll( sync ) ;
         PD_LOG( PDDEBUG, "Flushed all pages to file[%s], rc: %d",
                 _suFileName, rc ) ;
      }
      else
      {
         rc = flushDirtySegments( &num, force, sync ) ;
         PD_LOG( PDDEBUG, "Flushed %u segments to file[%s], rc: %d",
                 num, _suFileName, rc ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      rc = _markHeaderValid( sync, force, num > 0 ? TRUE : FALSE,
                             _lastSyncTime ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEBASE_SYNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsStorageBase::getSuFileName () const
   {
      return _suFileName ;
   }

   const CHAR* _dmsStorageBase::getSuName () const
   {
      if ( _pStorageInfo )
      {
         return _pStorageInfo->_suName ;
      }
      return "" ;
   }

   INT32 _dmsStorageBase::openStorage( const CHAR *pPath,
                                       IDataSyncManager *pSyncMgr,
                                       BOOLEAN createNew )
   {
      INT32 rc               = SDB_OK ;
      UINT64 fileSize        = 0 ;
      UINT64 currentOffset   = 0 ;
      UINT32 mode = OSS_READWRITE|OSS_EXCLUSIVE ;
      UINT64 rightSize = 0 ;
      BOOLEAN reGetSize = FALSE ;

      SDB_ASSERT( pPath, "path can't be NULL" ) ;

      if ( NULL == _pStorageInfo || NULL == pSyncMgr )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _pSyncMgr = pSyncMgr ;

      if ( createNew )
      {
         mode |= OSS_CREATEONLY ;
         _isCrash = FALSE ;
      }

      rc = utilBuildFullPath( pPath, _suFileName, OSS_MAX_PATHSIZE,
                              _fullPathName ) ;

      if ( rc )
      {
         PD_LOG ( PDERROR, "Path+filename are too long: %s; %s", pPath,
                  _suFileName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      PD_LOG ( PDDEBUG, "Open storage unit file %s", _fullPathName ) ;

      rc = ossMmapFile::open ( _fullPathName, mode, OSS_RU|OSS_WU|OSS_RG ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to open %s, rc=%d", _fullPathName, rc ) ;
         goto error ;
      }
      if ( createNew )
      {
         PD_LOG( PDEVENT, "Create storage unit file[%s] succeed, "
                 "mode: 0x%08x", _fullPathName, mode ) ;
      }

      rc = ossMmapFile::size ( fileSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                  _fullPathName, rc ) ;
         goto error ;
      }

      if ( 0 == fileSize )
      {
         if ( !createNew )
         {
            PD_LOG ( PDERROR, "Storage unit file[%s] is empty", _suFileName ) ;
            rc = SDB_DMS_INVALID_SU ;
            goto error ;
         }
         rc = _initializeStorageUnit () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to initialize Storage Unit, rc=%d", rc ) ;
            goto error ;
         }
         rc = ossMmapFile::size ( fileSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
      }

      if ( fileSize < _dataOffset() )
      {
         PD_LOG ( PDERROR, "Invalid storage unit size: %s", _suFileName ) ;
         PD_LOG ( PDERROR, "Expected more than %d bytes, actually read %lld "
                  "bytes", _dataOffset(), fileSize ) ;
         rc = SDB_DMS_INVALID_SU ;
         goto error ;
      }

      rc = map ( DMS_HEADER_OFFSET, DMS_HEADER_SZ, (void**)&_dmsHeader ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map header: %s", _suFileName ) ;
         goto error ;
      }

      if ( 0 == _dmsHeader->_lobdPageSize )
      {
         _dmsHeader->_lobdPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      rc = _validateHeader( _dmsHeader ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Storage Unit Header is invalid: %s, rc: %d",
                  _suFileName, rc ) ;
         goto error ;
      }
      else if ( !createNew )
      {
         if ( _pStorageInfo->_dataIsOK && 0 == _dmsHeader->_commitFlag )
         {
            if ( 0 == _dmsHeader->_commitLsn )
            {
               ossTimestamp t ;
               ossGetCurrentTime( t ) ;
               _dmsHeader->_commitTime = t.time * 1000 + t.microtm / 1000 ;
               _dmsHeader->_commitLsn = _pStorageInfo->_curLSNOnStart ;
            }
            _dmsHeader->_commitFlag = 1 ;
         }
         _commitFlag = _dmsHeader->_commitFlag ;
         _isCrash = ( 0 == _commitFlag ) ? TRUE : FALSE ;

         ossTimestamp commitTm ;
         commitTm.time = _dmsHeader->_commitTime / 1000 ;
         commitTm.microtm = ( _dmsHeader->_commitTime % 1000 ) * 1000 ;
         CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( commitTm, strTime ) ;

         PD_LOG( PDEVENT, "Storage file[%s] is %s[%u], CommitLSN: %lld, "
                 "CommitTime: %s[%llu]", _suFileName,
                 ( _isCrash ? "Invalid" : "Valid" ), _commitFlag,
                 _dmsHeader->_commitLsn,
                 strTime, _dmsHeader->_commitTime ) ;
      }

      rc = map ( DMS_SME_OFFSET, DMS_SME_SZ, (void**)&_dmsSME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map SME: %s", _suFileName ) ;
         goto error ;
      }

      rc = _smeMgr.init ( this, _dmsSME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to initialize SME, rc = %d", rc ) ;
         goto error ;
      }

      rc = _onMapMeta( (UINT64)( DMS_SME_OFFSET + DMS_SME_SZ ) ) ;
      PD_RC_CHECK( rc, PDERROR, "map file[%s] meta failed, rc: %d",
                   _suFileName, rc ) ;

      if ( 0 != ( fileSize - _dataOffset() ) % _getSegmentSize() )
      {
         PD_LOG ( PDWARNING, "Unexpected length[%llu] of file: %s", fileSize,
                  _suFileName ) ;

         rightSize = ( ( fileSize - _dataOffset() ) /
                       _getSegmentSize() ) * _getSegmentSize() +
                       _dataOffset() ;
         rc = ossTruncateFile( &_file, (INT64)rightSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                    _suFileName, rightSize, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Truncate file[%s] to size[%llu] succeed",
                 _suFileName, rightSize ) ;
         rc = ossMmapFile::size ( fileSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
      }

      rightSize = (UINT64)_dmsHeader->_storageUnitSize * pageSize() ;
      if ( fileSize > rightSize )
      {
         PD_LOG( PDWARNING, "File[%s] size[%llu] is grater than storage "
                 "unit pages[%u]", _suFileName, fileSize,
                 _dmsHeader->_storageUnitSize ) ;

         rc = ossTruncateFile( &_file, (INT64)rightSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                    _suFileName, rightSize, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Truncate file[%s] to size[%lld] succeed",
                 _suFileName, rightSize ) ;
         reGetSize = TRUE ;
      }
      else if ( fileSize < rightSize )
      {
         PD_LOG( PDWARNING, "File[%s] size[%llu] is less than storage "
                 "unit pages[%u]", _suFileName, fileSize,
                 _dmsHeader->_storageUnitSize ) ;

         rc = ossExtendFile( &_file, (INT64)( rightSize - fileSize ) ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Extend file[%s] to size[%lld] from size[%lld] "
                    "failed, rc: %d", _suFileName, rightSize,
                    fileSize, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Extend file[%s] to size[%lld] from size[%lld] "
                 "succeed", _suFileName, rightSize, fileSize ) ;
         reGetSize = TRUE ;
      }

      if ( reGetSize )
      {
         rc = ossMmapFile::size ( fileSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
         reGetSize = FALSE ;
      }

      _dataSegID = segmentSize() ;
      currentOffset = _dataOffset() ;
      while ( currentOffset < fileSize )
      {
         rc = map( currentOffset, _getSegmentSize(), NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to map data segment at offset %lld",
                     currentOffset ) ;
            goto error ;
         }
         currentOffset += _getSegmentSize() ;
      }
      _maxSegID = (INT32)segmentSize() - 1 ;

      rc = _dirtyList.init( maxSegmentNum() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Init dirty list failed in file[%s], rc: %d",
                  _suFileName, rc ) ;
         goto error ;
      }
      _dirtyList.setSize( segmentSize() - _dataSegID ) ;

      rc = _onOpened() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed on post open operation for file[%s], rc: %d",
                 _suFileName, rc ) ;
         goto error ;
      }

      if ( !isTempSU() )
      {
         _pSyncMgr->registerSync( this ) ;
      }
      else
      {
         _pSyncMgr = NULL ;
      }
      _isClosed = FALSE ;

   done:
      return rc ;
   error:
      ossMmapFile::close () ;
      _postOpen( rc ) ;
      goto done ;
   }

   void _dmsStorageBase::closeStorage ()
   {
      _isClosed = TRUE ;

      ossLatch( &_segmentLatch, SHARED ) ;
      ossUnlatch( &_segmentLatch, SHARED );

      if ( _pSyncMgr )
      {
         _pSyncMgr->unregSync( this ) ;
         _pSyncMgr = NULL ;
      }

      lock() ;
      unlock() ;

      if ( ossMmapFile::_opened )
      {
         _onClosed() ;

         if ( _dirtyList.dirtyNumber() > 0 )
         {
            flushAll( _syncDeep ) ;
         }
         _commitFlag = 1 ;
         _markHeaderValid( _syncDeep, TRUE, TRUE, 0 ) ;
         ossMmapFile::close() ;

         _dmsHeader = NULL ;
         _dmsSME = NULL ;
      }
      _maxSegID = -1 ;
   }

   INT32 _dmsStorageBase::removeStorage()
   {
      INT32 rc = SDB_OK ;

      if ( _fullPathName[0] == 0 )
      {
         goto done ;
      }

      _dirtyList.cleanAll() ;

      closeStorage() ;

      rc = ossDelete( _fullPathName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove storeage unit file: %s, "
                   "rc: %d", _fullPathName, rc ) ;

      PD_LOG( PDEVENT, "Remove storage unit file[%s] succeed", _fullPathName ) ;
      _fullPathName[ 0 ] = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::renameStorage( const CHAR *csName,
                                         const CHAR *suFileName )
   {
      INT32 rc = SDB_OK ;
      CHAR tmpPathFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      ossStrcpy( tmpPathFile, _fullPathName ) ;

      CHAR *pos = ossStrstr( tmpPathFile, _suFileName ) ;
      if ( !pos )
      {
         PD_LOG( PDERROR, "File full path[%s] is not include su file[%s]",
                 _fullPathName, _suFileName ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *pos = '\0' ;
      utilCatPath( tmpPathFile, OSS_MAX_PATHSIZE, suFileName ) ;

      if ( !_dmsHeader || _isClosed )
      {
         PD_LOG( PDERROR, "File is not open" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

#ifdef _WINDOWS
      ossStrncpy( _dmsHeader->_name, csName, DMS_SU_NAME_SZ ) ;
      _dmsHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
      flushHeader( TRUE ) ;

      {
         IDataSyncManager *pSyncMgr = _pSyncMgr ;
         closeStorage() ;

         rc = ossRenamePath( _fullPathName, tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                    _fullPathName, tmpPathFile, rc ) ;
            goto error ;
         }

         *pos = '\0' ;
         ossStrncpy( _suFileName, suFileName, DMS_SU_FILENAME_SZ ) ;
         _suFileName[ DMS_SU_FILENAME_SZ ] = '\0' ;
         ossStrncpy( _pStorageInfo->_suName, csName, DMS_SU_NAME_SZ ) ;
         _pStorageInfo->_suName[ DMS_SU_NAME_SZ ] = 0 ;
         rc = openStorage( tmpPathFile, pSyncMgr, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open storage file failed, rc: %d", rc ) ;
            goto error ;
         }
      }
#else
      rc = ossRenamePath( _fullPathName, tmpPathFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                 _fullPathName, tmpPathFile, rc ) ;
         goto error ;
      }

      ossStrncpy( _suFileName, suFileName, DMS_SU_FILENAME_SZ ) ;
      _suFileName[ DMS_SU_FILENAME_SZ ] = '\0' ;

      ossStrcpy( _fullPathName, tmpPathFile ) ;

      ossStrncpy( _dmsHeader->_name, csName, DMS_SU_NAME_SZ ) ;
      _dmsHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
      flushHeader( TRUE ) ;

#endif // _WINDOWS

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_postOpen( INT32 cause )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_DMS_INVALID_SU == cause &&
           _fullPathName[0] != '\0' )
      {
         CHAR tmpFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossSnprintf( tmpFile, OSS_MAX_PATHSIZE, "%s.err.%u",
                      _fullPathName, ossGetCurrentProcessID() ) ;
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
         rc = ossRenamePath( _fullPathName, tmpFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to [%s] failed, rc: %d",
                    _fullPathName, tmpFile, rc ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Rename file[%s] to [%s] succeed",
                    _fullPathName, tmpFile ) ;
         }
      }

   done:
      return cause ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_writeFile( OSSFILE *file, const CHAR * pData,
                                      INT64 dataLen )
   {
      INT32 rc = SDB_OK;
      SINT64 written = 0;
      SINT64 needWrite = dataLen;
      SINT64 bufOffset = 0;

      while ( 0 < needWrite )
      {
         rc = ossWrite( file, pData + bufOffset, needWrite, &written );
         if ( rc && SDB_INTERRUPT != rc )
         {
            PD_LOG( PDWARNING, "Failed to write data, rc: %d", rc ) ;
            goto error ;
         }
         needWrite -= written ;
         bufOffset += written ;

         rc = SDB_OK ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_initializeStorageUnit ()
   {
      INT32   rc        = SDB_OK ;
      _dmsHeader        = NULL ;
      _dmsSME           = NULL ;

      rc = ossSeek ( &_file, 0, OSS_SEEK_SET ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to seek to beginning of the file, rc: %d",
                  rc ) ;
         goto error ;
      }

      _dmsHeader = SDB_OSS_NEW dmsStorageUnitHeader ;
      if ( !_dmsHeader )
      {
         PD_LOG ( PDSEVERE, "Failed to allocate memory to for dmsHeader" ) ;
         PD_LOG ( PDSEVERE, "Requested memory: %d bytes", DMS_HEADER_SZ ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _initHeader ( _dmsHeader ) ;

      rc = _writeFile ( &_file, (const CHAR *)_dmsHeader, DMS_HEADER_SZ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write to file duirng SU init, rc: %d",
                  rc ) ;
         goto error ;
      }
      SDB_OSS_DEL _dmsHeader ;
      _dmsHeader = NULL ;

      _dmsSME = SDB_OSS_NEW dmsSpaceManagementExtent ;
      if ( !_dmsSME )
      {
         PD_LOG ( PDSEVERE, "Failed to allocate memory to for dmsSME" ) ;
         PD_LOG ( PDSEVERE, "Requested memory: %d bytes", DMS_SME_SZ ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _writeFile ( &_file, (CHAR *)_dmsSME, DMS_SME_SZ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write to file duirng SU init, rc: %d",
                  rc ) ;
         goto error ;
      }
      SDB_OSS_DEL _dmsSME ;
      _dmsSME = NULL ;

      rc = _onCreate( &_file, (UINT64)( DMS_HEADER_SZ + DMS_SME_SZ )  ) ;
      PD_RC_CHECK( rc, PDERROR, "create storage unit failed, rc: %d", rc ) ;

   done :
      return rc ;
   error :
      if (_dmsHeader)
      {
         SDB_OSS_DEL _dmsHeader ;
         _dmsHeader = NULL ;
      }
      if (_dmsSME)
      {
         SDB_OSS_DEL _dmsSME ;
         _dmsSME = NULL ;
      }
      goto done ;
   }

   void _dmsStorageBase::_initHeaderPageSize( dmsStorageUnitHeader * pHeader,
                                              dmsStorageInfo * pInfo )
   {
      pHeader->_pageSize      = pInfo->_pageSize ;
      pHeader->_lobdPageSize  = pInfo->_lobdPageSize ;
   }

   void _dmsStorageBase::_initHeader( dmsStorageUnitHeader * pHeader )
   {
      ossStrncpy( pHeader->_eyeCatcher, _getEyeCatcher(),
                  DMS_HEADER_EYECATCHER_LEN ) ;
      pHeader->_version = _curVersion() ;
      _initHeaderPageSize( pHeader, _pStorageInfo ) ;
      pHeader->_storageUnitSize = _dataOffset() / pHeader->_pageSize ;
      ossStrncpy ( pHeader->_name, _pStorageInfo->_suName, DMS_SU_NAME_SZ ) ;
      pHeader->_sequence = _pStorageInfo->_sequence ;
      pHeader->_numMB    = 0 ;
      pHeader->_MBHWM    = 0 ;
      pHeader->_pageNum  = 0 ;
      pHeader->_secretValue = _pStorageInfo->_secretValue ;
      pHeader->_createLobs = 0 ;
      pHeader->_commitFlag = 0 ;
      pHeader->_commitLsn  = ~0 ;
      pHeader->_commitTime = 0 ;
   }

   INT32 _dmsStorageBase::_checkPageSize( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      if ( DMS_PAGE_SIZE4K  != pHeader->_pageSize &&
           DMS_PAGE_SIZE8K  != pHeader->_pageSize &&
           DMS_PAGE_SIZE16K != pHeader->_pageSize &&
           DMS_PAGE_SIZE32K != pHeader->_pageSize &&
           DMS_PAGE_SIZE64K != pHeader->_pageSize )
      {
         PD_LOG ( PDERROR, "Invalid page size: %u, page size must be one of "
                  "4K/8K/16K/32K/64K", pHeader->_pageSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( DMS_DO_NOT_CREATE_LOB != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE4K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE8K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE16K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE32K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE64K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE128K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE256K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE512K != pHeader->_lobdPageSize )
      {
         PD_LOG ( PDERROR, "Invalid lob page size: %d in file[%s], lob page "
                  "size must be one of 4K/8K/16K/32K/64K/128K/256K/512K",
                  pHeader->_lobdPageSize, getSuFileName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( (UINT32)_pStorageInfo->_pageSize != pHeader->_pageSize )
      {
         _pStorageInfo->_pageSize = pHeader->_pageSize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_validateHeader( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      if ( 0 != ossStrncmp ( pHeader->_eyeCatcher, _getEyeCatcher(),
                             DMS_HEADER_EYECATCHER_LEN ) )
      {
         CHAR szTmp[ DMS_HEADER_EYECATCHER_LEN + 1 ] = {0} ;
         ossStrncpy( szTmp, pHeader->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;
         PD_LOG ( PDERROR, "Invalid eye catcher: %s", szTmp ) ;
         rc = SDB_INVALID_FILE_TYPE ;
         goto error ;
      }

      rc = _checkVersion( pHeader ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkPageSize( pHeader ) ;
      if ( rc )
      {
         goto error ;
      }
      _pageSize = pHeader->_pageSize ;
      _lobPageSize = pHeader->_lobdPageSize ;

      if ( 0 != _dataOffset() % pHeader->_pageSize )
      {
         rc = SDB_SYS ;
         PD_LOG( PDSEVERE, "Dms storage meta size[%llu] is not a mutiple of "
                 "pagesize[%u]", _dataOffset(), pHeader->_pageSize ) ;
      }
      else if ( DMS_MAX_PG < pHeader->_pageNum )
      {
         PD_LOG ( PDERROR, "Invalid storage unit page number: %u",
                  pHeader->_pageNum ) ;
         rc = SDB_SYS ;
      }
      else if ( pHeader->_storageUnitSize - pHeader->_pageNum !=
                _dataOffset() / pHeader->_pageSize )
      {
         PD_LOG( PDERROR, "Invalid storage unit size: %u",
                 pHeader->_storageUnitSize ) ;
         rc = SDB_SYS ;
      }
      else if ( 0 != ossStrncmp ( _pStorageInfo->_suName, pHeader->_name,
                                  DMS_SU_NAME_SZ ) )
      {
         PD_LOG ( PDERROR, "Invalid storage unit name: %s", pHeader->_name ) ;
         rc = SDB_SYS ;
      }

      if ( rc )
      {
         goto error ;
      }

      if ( !ossIsPowerOf2( pHeader->_pageSize, &_pageSizeSquare ) )
      {
         PD_LOG( PDERROR, "Page size must be the power of 2" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _pStorageInfo->_secretValue != pHeader->_secretValue )
      {
         _pStorageInfo->_secretValue = pHeader->_secretValue ;
      }
      if ( _pStorageInfo->_sequence != pHeader->_sequence )
      {
         _pStorageInfo->_sequence = pHeader->_sequence ;
      }
      if ( (UINT32)_pStorageInfo->_lobdPageSize != pHeader->_lobdPageSize )
      {
         _pStorageInfo->_lobdPageSize =  pHeader->_lobdPageSize ;
      }
      _pageNum = pHeader->_pageNum ;
      _segmentPages = _getSegmentSize() >> _pageSizeSquare ;

      if ( !ossIsPowerOf2( _segmentPages, &_segmentPagesSquare ) )
      {
         PD_LOG( PDERROR, "Segment pages[%u] must be the power of 2",
                 _segmentPages ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      PD_LOG ( PDDEBUG, "Validated storage unit file %s\n"
               "page size: %d\ndata size: %d pages\nname: %s\nsequence: %d",
               getSuFileName(), pHeader->_pageSize, pHeader->_pageNum,
               pHeader->_name, pHeader->_sequence ) ;

   done :
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_preExtendSegment ()
   {
      INT32 rc = _extendSegments( 1 ) ;
      ossUnlatch( &_segmentLatch, EXCLUSIVE ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Pre-extend segment failed, rc: %d", rc ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE__EXTENDSEG, "_dmsStorageBase::_extendSegments" )
   INT32 _dmsStorageBase::_extendSegments( UINT32 numSeg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE__EXTENDSEG ) ;
      INT64 fileSize = 0 ;

      UINT32 beginExtentID = _dmsHeader->_pageNum ;
      UINT32 endExtentID   = beginExtentID + _segmentPages * numSeg ;

      if ( endExtentID > DMS_MAX_PG )
      {
         PD_LOG( PDERROR, "Extent page[%u] exceed max pages[%u] in su[%s]",
                 endExtentID, DMS_MAX_PG, _suFileName ) ;
         rc = SDB_DMS_NOSPC ;
         goto error ;
      }

      for ( UINT32 i = beginExtentID; i < endExtentID; i++ )
      {
         if ( DMS_SME_FREE != _dmsSME->getBitMask( i ) )
         {
            rc = SDB_DMS_CORRUPTED_SME ;
            goto error ;
         }
      }

      rc = ossGetFileSize ( &_file, &fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get file size, rc = %d", rc ) ;

      if ( fileSize > (INT64)_dmsHeader->_storageUnitSize * pageSize() )
      {
         PD_LOG( PDWARNING, "File[%s] size[%llu] is not match with storage "
                 "unit pages[%u]", _suFileName, fileSize,
                 _dmsHeader->_storageUnitSize ) ;

         fileSize = (UINT64)_dmsHeader->_storageUnitSize * pageSize() ;
         rc = ossTruncateFile( &_file, fileSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Truncate file[%s] to size[%lld] failed, rc: %d",
                    _suFileName, fileSize, rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Truncate file[%s] to size[%lld]", _suFileName,
                 fileSize ) ;
      }

   retry:
      if ( _pStorageInfo->_enableSparse )
      {
         rc = ossExtentBySparse( &_file, (UINT64)_getSegmentSize() * numSeg ) ;
      }
      else
      {
         rc = ossExtendFile( &_file, _getSegmentSize() * numSeg ) ;
      }

      if ( rc )
      {
         INT32 rc1 = SDB_OK ;
         PD_LOG ( PDERROR, "Failed to extend storage unit for %lld "
                  "bytes, sparse:%s, rc: %d",
                  _getSegmentSize() * (UINT64)numSeg,
                  _pStorageInfo->_enableSparse ? "TRUE" : "FALSE", rc ) ;

         rc1 = ossTruncateFile ( &_file, fileSize ) ;
         if ( rc1 )
         {
            PD_LOG ( PDSEVERE, "Failed to revert the increase of segment, "
                     "rc = %d", rc1 ) ;
            ossPanic () ;
         }

         if ( SDB_INVALIDARG == rc && _pStorageInfo->_enableSparse )
         {
            _pStorageInfo->_enableSparse = FALSE ;
            goto retry ;
         }
         goto error ;
      }

      for ( UINT32 i = 0; i < numSeg ; i++ )
      {
         rc = map ( fileSize, _getSegmentSize(), NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to map storage unit from offset %lld",
                     _getSegmentSize() * i + _dmsHeader->_storageUnitSize ) ;
            goto error ;
         }
         _maxSegID += 1 ;
         _dirtyList.setSize( segmentSize() - _dataSegID ) ;

         rc = _smeMgr.depositASegment( (dmsExtentID)beginExtentID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to deposit new segment into SMEMgr, "
                     "rc = %d", rc ) ;
            ossPanic() ;
            goto error ;
         }
         beginExtentID += _segmentPages ;
         fileSize += _getSegmentSize() ;

         _dmsHeader->_storageUnitSize += _segmentPages ;
         _dmsHeader->_pageNum += _segmentPages ;
         _pageNum = _dmsHeader->_pageNum ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEBASE__EXTENDSEG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   UINT32 _dmsStorageBase::_extendThreshold () const
   {
      if ( _pStorageInfo )
      {
         return _pStorageInfo->_extentThreshold >> _pageSizeSquare ;
      }
      return (UINT32)( DMS_EXTEND_THRESHOLD_SIZE >> _pageSizeSquare ) ;
   }

   UINT32 _dmsStorageBase::_getSegmentSize() const
   {
      return DMS_SEGMENT_SZ ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE__FINDFREESPACE, "_dmsStorageBase::_findFreeSpace" )
   INT32 _dmsStorageBase::_findFreeSpace( UINT16 numPages, SINT32 & foundPage,
                                          dmsContext *context )
   {
      UINT32 segmentSize = 0 ;
      INT32 rc = SDB_OK ;
      INT32 rc1 = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE__FINDFREESPACE ) ;

      while ( TRUE )
      {
         rc = _smeMgr.reservePages( numPages, foundPage, &segmentSize ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( DMS_INVALID_EXTENT != foundPage )
         {
            break ;
         }

         if ( ossTestAndLatch( &_segmentLatch, EXCLUSIVE ) )
         {
            if ( segmentSize != _smeMgr.segmentNum() )
            {
               ossUnlatch( &_segmentLatch, EXCLUSIVE ) ;
               continue ;
            }

            rc = context ? context->pause() : SDB_OK ;
            if ( rc )
            {
               ossUnlatch( &_segmentLatch, EXCLUSIVE ) ;
               PD_LOG( PDERROR, "Failed to pause context[%s], rc: %d",
                       context->toString().c_str(), rc ) ;
               goto error ;
            }

            rc = _extendSegments( 1 ) ;

            rc1 = context ? context->resume() : SDB_OK ;

            ossUnlatch( &_segmentLatch, EXCLUSIVE ) ;

            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to extend storage unit, rc=%d", rc );
               goto error ;
            }
            PD_RC_CHECK( rc1, PDERROR, "Failed to resume context[%s], rc: %d",
                         context->toString().c_str(), rc1 ) ;

            PD_LOG ( PDDEBUG, "Successfully extend storage unit for %d pages",
                     numPages ) ;
         }
         else
         {
            BOOLEAN needAlloc = TRUE ;
            rc = context ? context->pause() : SDB_OK ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pause context[%s], rc: %d",
                         context->toString().c_str(), rc ) ;
            ossLatch( &_segmentLatch, SHARED ) ;
            ossUnlatch( &_segmentLatch, SHARED );
            rc = context ? context->resume() : SDB_OK ;
            PD_RC_CHECK( rc, PDERROR, "Failed to resum context[%s], rc: %d",
                         context->toString().c_str(), rc ) ;
            _onAllocSpaceReady( context, needAlloc ) ;
            if ( !needAlloc )
            {
               break ;
            }
         }
      }

      if ( _extendThreshold() > 0 &&
           _smeMgr.totalFree() < _extendThreshold() &&
           ossTestAndLatch( &_segmentLatch, EXCLUSIVE ) )
      {
         if ( _smeMgr.totalFree() >= _extendThreshold() ||
              SDB_OK != startExtendSegmentJob( NULL, this ) )
         {
            ossUnlatch( &_segmentLatch, EXCLUSIVE ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEBASE__FINDFREESPACE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_releaseSpace( SINT32 pageStart, UINT16 numPages )
   {
      return _smeMgr.releasePages( pageStart, numPages ) ;
   }

   UINT32 _dmsStorageBase::_totalFreeSpace ()
   {
      return _smeMgr.totalFree() ;
   }

   INT32 _dmsStorageBase::flushHeader( BOOLEAN sync )
   {
      return flush( 0, sync ) ;
   }

   INT32 _dmsStorageBase::flushSME( BOOLEAN sync )
   {
      return _ossMmapFile::flush( 1, sync ) ;
   }

   INT32 _dmsStorageBase::flushMeta( BOOLEAN sync, UINT32 *pExceptID,
                                     UINT32 num )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      syncMemToMmap() ;

      for ( UINT32 i = 0 ; i < _dataSegID ; ++i )
      {
         if ( pExceptID )
         {
            BOOLEAN bExcept = FALSE ;
            for ( UINT32 j = 0 ; j < num ; ++j )
            {
               if ( pExceptID[ j ] == i )
               {
                  bExcept = TRUE ;
                  break ;
               }
            }
            if ( bExcept )
            {
               continue ;
            }
         }
         rcTmp = _ossMmapFile::flush( i, sync ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Flush segment %u to disk failed, rc: %d",
                    i, rcTmp ) ;
            if ( SDB_OK == rc )
            {
               rc = rcTmp ;
            }
         }
      }
      return rc ;
   }

   INT32 _dmsStorageBase::flushPages( SINT32 pageID, UINT16 pageNum,
                                      BOOLEAN sync )
   {
      INT32 rc = SDB_OK ;

      if( DMS_INVALID_EXTENT == pageID )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         UINT32 offset = 0 ;
         UINT32 segmentID = extent2Segment( pageID, &offset ) ;
         INT32 length = (INT32)pageNum << pageSizeSquareRoot() ;
         offset <<= pageSizeSquareRoot() ;
         rc = _ossMmapFile::flushBlock( segmentID, offset, length, sync ) ;
      }

      return rc ;
   }

   INT32 _dmsStorageBase::flushSegment( UINT32 segmentID, BOOLEAN sync )
   {
      if ( _dataSegID > 0 && segmentID >= _dataSegID )
      {
         _dirtyList.cleanDirty( segmentID - _dataSegID ) ;
      }
      return _ossMmapFile::flush( segmentID, sync ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE_FLUSHALL, "_dmsStorageBase::flushAll" )
   INT32 _dmsStorageBase::flushAll( BOOLEAN sync )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE_FLUSHALL ) ;

      syncMemToMmap() ;

      rc = _onFlushDirty( TRUE, sync ) ;
      if ( rc )
      {
         goto done ;
      }

      _dirtyList.cleanAll() ;
      _writeReordNum = 0 ;
      rc = _ossMmapFile::flushAll( sync ) ;
      if ( rc )
      {
         goto done ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEBASE_FLUSHALL, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE_FLUSHDIRTYSEGS, "_dmsStorageBase::flushDirtySegments" )
   INT32 _dmsStorageBase::flushDirtySegments ( UINT32 *pNum,
                                               BOOLEAN force,
                                               BOOLEAN sync )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE_FLUSHDIRTYSEGS ) ;

      UINT32 numbers = 0 ;
      INT32 segmentID = 0 ;
      UINT32 fromPos = 0 ;

      rc = flushMeta( sync ) ;
      if ( rc )
      {
         goto done ;
      }
      rc = _onFlushDirty( force, sync ) ;
      if ( rc )
      {
         goto done ;
      }

      _writeReordNum = 0 ;
      segmentID = _dirtyList.nextDirtyPos( fromPos ) ;
      while( segmentID >= 0 )
      {
         if ( _isClosed )
         {
            rc = SDB_APP_INTERRUPT ;
            break ;
         }
         else if ( !force && 0 == _commitFlag )
         {
            rc = SDB_APP_INTERRUPT ;
            break ;
         }
         rc = flushSegment( (UINT32)segmentID + _dataSegID, sync ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to flush segment[%u] to file[%s], "
                    "rc: %d", segmentID, _suFileName, rc ) ;
         }
         ++numbers ;
         segmentID = _dirtyList.nextDirtyPos( fromPos ) ;
      }

   done :
      if ( pNum )
      {
         *pNum = numbers ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGEBASE_FLUSHDIRTYSEGS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE__MARKHEADEERINVALID, "_dmsStorageBase::_markHeaderInvalid" )
   void _dmsStorageBase::_markHeaderInvalid( INT32 collectionID,
                                             BOOLEAN isAll )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE__MARKHEADEERINVALID ) ;
      if ( _dmsHeader )
      {
         if ( isAll )
         {
            _onMarkHeaderInvalid( -1 ) ;
         }
         else if ( collectionID >= 0 )
         {
            _onMarkHeaderInvalid( collectionID ) ;
         }
      }

      if ( _dmsHeader && !_isCrash && _commitFlag )
      {
         ossScopedLock lock( &_commitLatch ) ;

         if ( _commitFlag )
         {
            _commitFlag = 0 ;
            _dmsHeader->_commitFlag = 0 ;
            flushHeader( _syncDeep ) ;
         }
      }
      PD_TRACE_EXIT( SDB__DMSSTORAGEBASE__MARKHEADEERINVALID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE__MARKHEADEERVALID, "_dmsStorageBase::_markHeaderValid" )
   INT32 _dmsStorageBase::_markHeaderValid( BOOLEAN sync,
                                            BOOLEAN force,
                                            BOOLEAN hasFlushedData,
                                            UINT64 lastTime )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE__MARKHEADEERVALID ) ;

      if ( _dmsHeader )
      {
         if ( 0 == lastTime )
         {
            ossTimestamp t ;
            ossGetCurrentTime( t ) ;
            lastTime = t.time * 1000 + t.microtm / 1000 ;
         }

         if ( _commitFlag || force )
         {
            UINT64 lastLSN = ~0 ;
            UINT32 tmpCommitFlag = 0 ;
            ossScopedLock lock( &_commitLatch ) ;
            if ( _commitFlag || force )
            {
               _onMarkHeaderValid( lastLSN, sync, lastTime ) ;

               tmpCommitFlag = _isCrash ? 0 : _commitFlag ;
               if ( hasFlushedData ||
                    tmpCommitFlag != _dmsHeader->_commitFlag ||
                    lastLSN != _dmsHeader->_commitLsn )
               {
                  _dmsHeader->_commitFlag = tmpCommitFlag ;
                  _dmsHeader->_commitLsn = lastLSN ;
                  _dmsHeader->_commitTime = lastTime ;
                  rc = flushHeader( sync ) ;
               }
            }
         }
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEBASE__MARKHEADEERVALID, rc ) ;
      return rc ;
   }

   void _dmsStorageBase::_incWriteRecord()
   {
      ++_writeReordNum ;
   }

   void _dmsStorageBase::_disableBlockScan()
   {
      _blockScanSupport = FALSE ;
   }

   /*
      DMS TOOL FUNCTIONS:
   */
   BOOLEAN dmsAccessAndFlagCompatiblity ( UINT16 collectionFlag,
                                          DMS_ACCESS_TYPE accessType )
   {
      if ( !pmdGetStartup().isOK() )
      {
         return TRUE ;
      }
      else if ( DMS_IS_MB_FREE(collectionFlag) ||
                DMS_IS_MB_DROPPED(collectionFlag) )
      {
         return FALSE ;
      }
      else if ( DMS_IS_MB_NORMAL(collectionFlag) )
      {
         return TRUE ;
      }
      else if ( DMS_IS_MB_OFFLINE_REORG(collectionFlag) )
      {
         if ( DMS_IS_MB_OFFLINE_REORG_TRUNCATE(collectionFlag) &&
            ( accessType == DMS_ACCESS_TYPE_TRUNCATE ) )
         {
            return TRUE ;
         }
         else if ( ( DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY ( collectionFlag ) ||
                     DMS_IS_MB_OFFLINE_REORG_REBUILD( collectionFlag ) ) &&
                  ( ( accessType == DMS_ACCESS_TYPE_QUERY ) ||
                    ( accessType == DMS_ACCESS_TYPE_FETCH ) ) )
         {
            return TRUE ;
         }
         return FALSE ;
      }
      else if ( DMS_IS_MB_ONLINE_REORG(collectionFlag) )
      {
         return TRUE ;
      }
      else if ( DMS_IS_MB_LOAD ( collectionFlag ) &&
                DMS_ACCESS_TYPE_TRUNCATE != accessType )
      {
         return TRUE ;
      }

      return FALSE ;
    }
}


