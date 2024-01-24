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
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
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
      destroy() ;
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
      /// first destroy
      destroy() ;

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

   void _dmsDirtyList::destroy()
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
      _hasIncWriteCount = FALSE ;
      _pBase = NULL ;
      _ptr   = ( ossValuePtr ) 0 ;
   }

   _dmsExtRW::_dmsExtRW( const _dmsExtRW &extRW )
   : _extentID( extRW._extentID ),
     _collectionID( extRW._collectionID ),
     _attr( extRW._attr ),
     _hasIncWriteCount( FALSE ),
     _ptr( extRW._ptr ),
     _pBase( extRW._pBase )
   {
      /*
      If _hasIncWriteCount is FALSE, we must inc writePtrCount,
      _pBase._mbStatInfo[collectionID]._writePtrCount.

      If we don't inc writePtrCount, then the following problems will occur.
      eg:
      {
         _dmsExtRW a ;
         // now writePtrCount is 0, _hasIncWriteCount is TRUE
         a.writePtr() ;
         // now writePtrCount is 1, _hasIncWriteCount is FALSE
         _dmsExtRW b( a ) ;
         ...
         ~a() ;
         // now writePtrCount is 0
         ~b() ;
         // now writePtrCount is (UINT32)0 - 1
      }
      */

      if ( extRW._hasIncWriteCount )
      {
         _pBase->incWritePtrCount( extRW._collectionID ) ;
         _hasIncWriteCount = TRUE ;
      }
   }

   _dmsExtRW::~_dmsExtRW()
   {
      if ( _pBase && isDirty() )
      {
         _pBase->markDirty( _collectionID, _extentID, DMS_CHG_AFTER ) ;
      }

      if ( _hasIncWriteCount )
      {
         _pBase->decWritePtrCount( _collectionID ) ;
         _hasIncWriteCount = FALSE ;
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

   const CHAR* _dmsExtRW::readPtr( UINT32 offset, UINT32 len ) const
   {
      if ( (ossValuePtr)0 == _ptr )
      {
         std::string text = "readPtr is NULL: " ;
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
         std::string text = "writePtr is NULL: " ;
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
      if ( !_hasIncWriteCount )
      {
         _pBase->incWritePtrCount( _collectionID ) ;
         _hasIncWriteCount = TRUE ;
      }
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
   #define DMS_SYS_EXTEND_THRESHOLD_SIZE  ( 4194304 )    // 4MB

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
   _dmsStorageBase::_dmsStorageBase( IStorageService *service,
                                     dmsSUDescriptor *suDescriptor,
                                     const CHAR *pSuFileName )
   {
      SDB_ASSERT( service, "Service can't be NULL" ) ;
      SDB_ASSERT( suDescriptor, "SU descriptor can't be NULL" ) ;
      SDB_ASSERT( pSuFileName, "SU file name can't be NULL" ) ;

      _service            = service ;
      _suDescriptor       = suDescriptor ;
      _dmsHeader          = NULL ;
      _dmsSME             = NULL ;
      _dataSegID          = 0 ;

      _pageNum            = 0 ;
      _maxSegID           = -1 ;
      _segmentPages       = 0 ;
      _segmentPagesSquare = 0 ;
      _pageSizeSquare     = 0 ;
      _isTempSU           = FALSE ;
      _isSysSU            = FALSE ;
      _transSupport       = TRUE ;
      _blockScanSupport   = TRUE ;
      _pageSize           = 0 ;
      _lobPageSize        = 0 ;
      _segmentSize        = 0 ;

      ossStrncpy( _suFileName, pSuFileName, DMS_SU_FILENAME_SZ ) ;
      _suFileName[ DMS_SU_FILENAME_SZ ] = 0 ;
      ossMemset( _fullPathName, 0, sizeof(_fullPathName) ) ;

      _resetInfoByName( suDescriptor->getSUName() ) ;

      _pSyncMgr           = NULL ;
      _pStatMgr           = NULL ;
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
      _dirtyList.destroy() ;
      _suDescriptor = NULL ;
      _service = NULL ;
   }

   BOOLEAN _dmsStorageBase::isClosed() const
   {
      return _isClosed ;
   }

   void _dmsStorageBase::setTransSupport( BOOLEAN supported )
   {
      _transSupport = supported ;
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

   UINT64 _dmsStorageBase::getCreateTime() const
   {
      if ( _dmsHeader )
      {
         return _dmsHeader->_createTime ;
      }
      return 0 ;
   }

   UINT64 _dmsStorageBase::getUpdateTime() const
   {
      if ( _dmsHeader )
      {
         return _dmsHeader->_updateTime ;
      }
      return 0 ;
   }

   void _dmsStorageBase::restoreForCrash()
   {
      _isCrash = FALSE ;
      /// set force sync
      _forceSync = TRUE ;
      _onRestore() ;
   }

   BOOLEAN _dmsStorageBase::isCrashed() const
   {
      return _isCrash ;
   }

   void _dmsStorageBase::setCrashed()
   {

      ossScopedLock lock( &_commitLatch ) ;

      _isCrash = TRUE ;
      _commitFlag = 0 ;
      _dmsHeader->_commitFlag = 0 ;

      /// flush header
      flushHeader( TRUE ) ;
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

      if ( !_syncEnable )
      {
         return FALSE ;
      }
      else if ( _forceSync )
      {
         force = TRUE ;
         return TRUE ;
      }

      if ( 0 != _commitFlag && _dirtyList.dirtyNumber() <= 0 )
      {
         return FALSE ;
      }
      else if ( _syncRecordNum > 0 && _writeReordNum >= _syncRecordNum )
      {
         PD_LOG( PDDEBUG, "Write record number[%u] more than threshold[%u]",
                 _writeReordNum, _syncRecordNum ) ;
         force = TRUE ;
         return TRUE ;
      }
      else
      {
         UINT64 oldestTick = _getOldestWriteTick() ;
         UINT64 oldTimeSpan = 0 ;

         if ( (UINT64)~0 != oldestTick )
         {
            oldTimeSpan = pmdGetTickSpanTime( oldestTick ) ;
         }

         if ( _syncInterval > 0 &&
              oldTimeSpan >= _syncNoWriteTime &&
              oldTimeSpan > 60 * _syncInterval )
         {
            /// If has a collection that don't write over very mush times,
            /// need to flush by force
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

      /// first set commitFlag to valid
      /// then flush dirty
      /// then check commitFlag, when valid, need to reflush header
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
      if ( _suDescriptor )
      {
         return _suDescriptor->getSUName() ;
      }
      return "" ;
   }

   INT32 _dmsStorageBase::openStorage( const CHAR *pPath,
                                       IDataSyncManager *pSyncMgr,
                                       IDataStatManager *pStatMgr,
                                       BOOLEAN createNew )
   {
      INT32 rc               = SDB_OK ;
      UINT64 fileSize        = 0 ;
      UINT64 currentOffset   = 0 ;
      UINT32 mode = OSS_READWRITE|OSS_EXCLUSIVE ;
      UINT64 rightSize = 0 ;
      ossSpinSLatch *pExtendLatch = NULL ;

      SDB_ASSERT( pPath, "path can't be NULL" ) ;

      if ( NULL == _suDescriptor || NULL == pSyncMgr )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _pSyncMgr = pSyncMgr ;
      _pStatMgr = pStatMgr ;

      /// init lock
      pExtendLatch = SDB_OSS_NEW ossSpinSLatch() ;
      if ( !pExtendLatch )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc extend latch failed" ) ;
         goto error ;
      }
      _segmentLatch = sharedMutexPtr( pExtendLatch ) ;

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

      // open the file, create one if not exist
      rc = ossMmapFile::open ( _fullPathName, mode, OSS_RU|OSS_WU|OSS_RG ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc && !createNew && _canRecreateNew() )
         {
            mode |= OSS_CREATEONLY ;
            PD_LOG ( PDWARNING, "Try to recreate storage unit file[%s], "
                     "mode:0x%08x", _fullPathName, mode ) ;
            // open the file, create one if not exist
            rc = ossMmapFile::open ( _fullPathName, mode, OSS_RU|OSS_WU|OSS_RG ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recreate storeage unit file: %s, "
                         "rc: %d", _fullPathName, rc ) ;
            createNew = TRUE ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to open %s, rc=%d", _fullPathName, rc ) ;
            goto error ;
         }
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

      // is it a brand new file
      if ( 0 == fileSize )
      {
         // if it's a brand new file but we don't ask for creating new storage
         // unit, then we exit with invalid su error
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
         // then we get the size again to make sure it's what we need
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

      // map metadata
      // header, 64K
      rc = map ( DMS_HEADER_OFFSET, DMS_HEADER_SZ, (void**)&_dmsHeader ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map header: %s", _suFileName ) ;
         goto error ;
      }

      /// lobPageSize is 0 if it was created by db with older version.
      /// we reassign it with 256K -- yunwu
      if ( 0 == _dmsHeader->_lobdPageSize )
      {
         _dmsHeader->_lobdPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      // after we load SU, let's verify it's expected file
      rc = _validateHeader( _dmsHeader ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Storage Unit Header is invalid: %s, rc: %d",
                  _suFileName, rc ) ;
         goto error ;
      }
      else if ( !createNew )
      {
         if ( _suDescriptor->getStorageInfo()._dataIsOK && 0 == _dmsHeader->_commitFlag )
         {
            /// upgrade from old version( _dmsHeader->_commitLsn = 0 )
            if ( 0 == _dmsHeader->_commitLsn )
            {
               ossTimestamp t ;
               ossGetCurrentTime( t ) ;
               _dmsHeader->_commitTime = t.time * 1000 + t.microtm / 1000 ;
               _dmsHeader->_commitLsn = _suDescriptor->getStorageInfo()._curLSNOnStart ;
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
      else
      {
         _dmsHeader->_createTime = ossGetCurrentMilliseconds() ;
         _dmsHeader->_updateTime = _dmsHeader->_createTime ;
      }

      // SME, 16MB
      rc = map ( DMS_SME_OFFSET, DMS_SME_SZ, (void**)&_dmsSME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map SME: %s", _suFileName ) ;
         goto error ;
      }

      // initialize SME Manager, which is used to do fast-lookup and release
      // for extents. Note _pageSize is initialized in _validateHeader, so
      // we are safe to use page size here
      rc = _smeMgr.init ( this, _dmsSME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to initialize SME, rc = %d", rc ) ;
         goto error ;
      }

      rc = _onMapMeta( (UINT64)( DMS_SME_OFFSET + DMS_SME_SZ ) ) ;
      PD_RC_CHECK( rc, PDERROR, "map file[%s] meta failed, rc: %d",
                   _suFileName, rc ) ;

      // make sure the file size is multiple of segments
      if ( !_checkFileSizeValidBySegment( fileSize, rightSize ) )
      {
         PD_LOG ( PDWARNING, "Unexpected length[%llu] of file: %s", fileSize,
                  _suFileName ) ;
         if ( fileSize > rightSize )
         {
            /// need to truncate the file, to remove
            /// the invalid part of the segment
            rc = ossTruncateFile( &_file, (INT64)rightSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                       _suFileName, rightSize, rc ) ;
               goto error ;
            }
            PD_LOG( PDEVENT, "Truncate file[%s] to size[%llu] succeed",
                    _suFileName, rightSize ) ;
            // then we get the size again to make sure it's what we need
            rc = ossMmapFile::size ( fileSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                        _suFileName, rc ) ;
               goto error ;
            }
         }
      }

      rightSize = 0 ;
      /// make sure the file is correct with meta data
      if ( !_checkFileSizeValid( fileSize, rightSize ) )
      {
         if ( fileSize > rightSize )
         {
            PD_LOG( PDWARNING, "File[%s] size[%llu] is greater than storage "
                    "unit pages[%u]", _suFileName, fileSize,
                    _dmsHeader->_storageUnitSize ) ;
            rc = ossTruncateFile( &_file, (INT64)rightSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                       _suFileName, rightSize, rc ) ;
               goto error ;
            }
            PD_LOG( PDEVENT, "Truncate file[%s] to size[%llu] succeed",
                    _suFileName, rightSize ) ;
         }
         else if ( fileSize < rightSize )
         {
            PD_LOG( PDWARNING, "File[%s] size[%llu] is less than storage "
                    "unit pages[%u]", _suFileName, fileSize,
                    _dmsHeader->_storageUnitSize ) ;
            rc = ossExtendFile( &_file, (INT64)( rightSize - fileSize ) ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extend file[%s] to size[%llu] from size[%llu] "
                       "failed, rc: %d", _suFileName, rightSize,
                       fileSize, rc ) ;
               goto error ;
            }
            PD_LOG( PDEVENT, "Extend file[%s] to size[%llu] from size[%llu] "
                    "succeed", _suFileName, rightSize, fileSize ) ;
         }

         // then we get the size again to make sure it's what we need
         rc = ossMmapFile::size ( fileSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
      }

      // loop and map each segment into separate mem range
      _dataSegID = segmentSize() ;
      currentOffset = _dataOffset() ;
      while ( currentOffset < fileSize )
      {
         rc = map( currentOffset, _getSegmentSize(), NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to map data segment at offset %llu",
                     currentOffset ) ;
            goto error ;
         }
         currentOffset += _getSegmentSize() ;
      }
      _maxSegID = (INT32)segmentSize() - 1 ;

      // create dirtyList to record dirty pages. Note dirty list doesn't
      // contain header and metadata segments, only for data segments
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

      /// regsiter
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
      // set closed flag
      _isClosed = TRUE ;

      // be sure the extend job has quit
      if ( _segmentLatch.get() )
      {
         ossLatch( _segmentLatch.get(), SHARED ) ;
         ossUnlatch( _segmentLatch.get(), SHARED );
      }

      // unregister
      if ( _pSyncMgr )
      {
         _pSyncMgr->unregSync( this ) ;
         _pSyncMgr = NULL ;
      }

      _pStatMgr = NULL ;

      // be sure the sync jos has quit
      lock() ;
      unlock() ;

      if ( ossMmapFile::_opened )
      {
         _onClosed() ;

         if ( _dirtyList.dirtyNumber() > 0 )
         {
            flushAll( _syncDeep ) ;
         }
         /// set commit flag valid
         _commitFlag = 1 ;
         /// make header valid
         _markHeaderValid( _syncDeep, TRUE, TRUE, 0 ) ;
         /// close file
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

      // clean all dirty
      _dirtyList.cleanAll() ;

      // close
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

      _onHeaderUpdated() ;

#ifdef _WINDOWS
      /// modify the header
      ossStrncpy( _dmsHeader->_name, csName, DMS_SU_NAME_SZ ) ;
      _dmsHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
      flushHeader( TRUE ) ;

      {
         IDataSyncManager *pSyncMgr = _pSyncMgr ;
         IDataStatManager *pStatMgr = _pStatMgr ;
         /// close
         closeStorage() ;

         /// rename
         rc = ossRenamePath( _fullPathName, tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                    _fullPathName, tmpPathFile, rc ) ;
            goto error ;
         }

         /// open
         *pos = '\0' ;
         ossStrncpy( _suFileName, suFileName, DMS_SU_FILENAME_SZ ) ;
         _suFileName[ DMS_SU_FILENAME_SZ ] = '\0' ;
         ossStrncpy( _pStorageInfo->_suName, csName, DMS_SU_NAME_SZ ) ;
         _pStorageInfo->_suName[ DMS_SU_NAME_SZ ] = 0 ;
         rc = openStorage( tmpPathFile, pSyncMgr, pStatMgr, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open storage file failed, rc: %d", rc ) ;
            goto error ;
         }
      }
#else
      /// rename filename
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

      /// modify the header
      ossStrncpy( _dmsHeader->_name, csName, DMS_SU_NAME_SZ ) ;
      _dmsHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
      flushHeader( TRUE ) ;

#endif // _WINDOWS

      _resetInfoByName( csName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::updateCSUniqueIDFromInfo()
   {
      // if the cs without lob, lobd file isn't exist, then _dmsHeader is NULL.
      if ( _dmsHeader && _suDescriptor )
      {
         _dmsHeader->_csUniqueID = _suDescriptor->getCSUniqueID() ;
         _onHeaderUpdated() ;
         flushHeader( TRUE ) ;
      }

      return SDB_OK ;
   }

   INT32 _dmsStorageBase::setLobPageSize ( UINT32 lobPageSize )
   {
      INT32 rc = SDB_OK ;

      _lobPageSize = lobPageSize ;
      if ( _suDescriptor )
      {
         _suDescriptor->getStorageInfo()._lobdPageSize = lobPageSize ;
      }

      if ( _dmsHeader )
      {
         _dmsHeader->_lobdPageSize = lobPageSize ;
         _onHeaderUpdated() ;
         flushHeader( TRUE ) ;
      }

      return rc ;
   }

   INT32 _dmsStorageBase::_postOpen( INT32 cause )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_DMS_INVALID_SU == cause &&
           _fullPathName[0] != '\0' )
      {
         rc = dmsRenameInvalidFile( _fullPathName ) ;
         if ( rc )
         {
            goto error ;
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

      // move to beginning of the file
      rc = ossSeek ( &_file, 0, OSS_SEEK_SET ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to seek to beginning of the file, rc: %d",
                  rc ) ;
         goto error ;
      }

      // allocate buffer for dmsHeader
      _dmsHeader = SDB_OSS_NEW dmsStorageUnitHeader ;
      if ( !_dmsHeader )
      {
         PD_LOG ( PDSEVERE, "Failed to allocate memory to for dmsHeader" ) ;
         PD_LOG ( PDSEVERE, "Requested memory: %d bytes", DMS_HEADER_SZ ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // initialize a new header with empty size
      _initHeader ( _dmsHeader ) ;

      // write the buffer into file
      rc = _writeFile ( &_file, (const CHAR *)_dmsHeader, DMS_HEADER_SZ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write to file duirng SU init, rc: %d",
                  rc ) ;
         goto error ;
      }
      SDB_OSS_DEL _dmsHeader ;
      _dmsHeader = NULL ;

      // then SME
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

      if ( !_isSysSU )
      {
         pHeader->_segmentSize= DMS_SEGMENT_SZ_BASE ;
      }
      else
      {
         pHeader->_segmentSize= DMS_SYS_SEGMENT_SZ ;
      }
   }

   void _dmsStorageBase::_initHeader( dmsStorageUnitHeader * pHeader )
   {
      ossStrncpy( pHeader->_eyeCatcher, _getEyeCatcher(),
                  DMS_HEADER_EYECATCHER_LEN ) ;
      pHeader->_version = _curVersion() ;
      _initHeaderPageSize( pHeader, &( _suDescriptor->getStorageInfo() ) ) ;
      pHeader->_storageUnitSize = _dataOffset() / pHeader->_pageSize ;
      ossStrncpy ( pHeader->_name, _suDescriptor->getSUName(), DMS_SU_NAME_SZ ) ;
      pHeader->_sequence = _suDescriptor->getStorageInfo()._sequence ;
      pHeader->_numMB    = 0 ;
      pHeader->_MBHWM    = 0 ;
      pHeader->_pageNum  = 0 ;
      pHeader->_secretValue = _suDescriptor->getStorageInfo()._secretValue ;
      pHeader->_createLobs = 0 ;
      pHeader->_commitFlag = 0 ;
      pHeader->_commitLsn  = ~0 ;
      pHeader->_commitTime = 0 ;
      pHeader->_csUniqueID = _suDescriptor->getCSUniqueID() ;
      pHeader->_idxInnerHWM = 0 ;
      pHeader->_clInnderHWM = 0 ;
   }

   INT32 _dmsStorageBase::_checkPageSize( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      // check page size
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
      /// change segmentsize
      else if ( 0 != pHeader->_segmentSize &&
                !DMS_IS_VALID_SEGMENT( pHeader->_segmentSize ) )
      {
         PD_LOG( PDERROR, "Invalid segment size: %d in file[%s]",
                 pHeader->_segmentSize, getSuFileName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // must be set storage info page size here, because lob meta page size
      // is 256B, so can't be assign to storage page size in later code
      if ( (UINT32)_suDescriptor->getStorageInfo()._pageSize != pHeader->_pageSize )
      {
         _suDescriptor->getStorageInfo()._pageSize = pHeader->_pageSize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_validateHeader( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      // check eye catcher
      if ( 0 != ossStrncmp ( pHeader->_eyeCatcher, _getEyeCatcher(),
                             DMS_HEADER_EYECATCHER_LEN ) )
      {
         CHAR szTmp[ DMS_HEADER_EYECATCHER_LEN + 1 ] = {0} ;
         ossStrncpy( szTmp, pHeader->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;
         PD_LOG ( PDERROR, "Invalid eye catcher: %s", szTmp ) ;
         rc = SDB_INVALID_FILE_TYPE ;
         goto error ;
      }

      // check version
      rc = _checkVersion( pHeader ) ;
      if ( rc )
      {
         goto error ;
      }
      // check page size
      rc = _checkPageSize( pHeader ) ;
      if ( rc )
      {
         goto error ;
      }
      _pageSize = pHeader->_pageSize ;
      _lobPageSize = pHeader->_lobdPageSize ;
      /// lobm _segmentSize update in _checkPageSize
      if ( 0 == _segmentSize )
      {
         _segmentSize = pHeader->_segmentSize ;
      }

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
      else if ( 0 != ossStrncmp ( _suDescriptor->getSUName(), pHeader->_name,
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

      if ( _suDescriptor->getStorageInfo()._secretValue != pHeader->_secretValue )
      {
         _suDescriptor->getStorageInfo()._secretValue = pHeader->_secretValue ;
      }
      if ( _suDescriptor->getStorageInfo()._sequence != pHeader->_sequence )
      {
         _suDescriptor->getStorageInfo()._sequence = pHeader->_sequence ;
      }
      if ( (UINT32)_suDescriptor->getStorageInfo()._lobdPageSize != pHeader->_lobdPageSize )
      {
         _suDescriptor->getStorageInfo()._lobdPageSize =  pHeader->_lobdPageSize ;
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

      if ( _suDescriptor->getCSUniqueID() != pHeader->_csUniqueID )
      {
         _suDescriptor->setCSUniqueID( pHeader->_csUniqueID ) ;
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

   BOOLEAN _dmsStorageBase::_checkFileSizeValidBySegment( const UINT64 fileSize,
                                                          UINT64 &rightSize )
   {
      if (  0 == ( fileSize - _dataOffset() ) % _getSegmentSize() )
      {
         rightSize = fileSize ;
         return TRUE ;
      }
      else
      {
         rightSize = ( ( fileSize - _dataOffset() ) /
                       _getSegmentSize() ) * _getSegmentSize() +
                       _dataOffset() ;
         return FALSE ;
      }
   }

   BOOLEAN _dmsStorageBase::_checkFileSizeValid( const UINT64 fileSize,
                                                 UINT64 &rightSize )
   {
      UINT64 rightSz = (UINT64)_dmsHeader->_storageUnitSize * pageSize() ;
      if ( fileSize == rightSz )
      {
         rightSize = fileSize ;
         return TRUE ;
      }
      else
      {
         rightSize = rightSz ;
         return FALSE ;
      }
   }

   INT32 _dmsStorageBase::_preExtendSegment ()
   {
      INT32 rc = _extendSegments( 1 ) ;
      // release lock
      sharedMutexPtr tmpPtr( _segmentLatch ) ;
      ossUnlatch( tmpPtr.get(), EXCLUSIVE ) ;

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
      UINT64 fileSize       = 0 ;
      UINT64 newFileSize    = 0 ;
      UINT64 rightSize      = 0 ;
      UINT64 incFileSize    = 0 ;
      UINT32 incPageNum     = 0 ;
      UINT32 mapSegNum      = 0 ;
      UINT32 beginExtentID  = 0 ;
      UINT32 endExtentID    = 0 ;

      // now other normal applications still able to access metadata in
      // read-only mode

      // get file size for map or rollback
      rc = ossGetFileSize ( &_file, (INT64 *)&fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get file size, rc = %d", rc ) ;

      // check file size is valid or not
      if ( !_checkFileSizeValid( fileSize, rightSize ) )
      {
         if ( fileSize > rightSize )
         {
            PD_LOG( PDWARNING, "File[%s] size[%llu] is not match with storage "
                    "unit pages[%u]", _suFileName, fileSize,
                    _dmsHeader->_storageUnitSize ) ;
            fileSize = rightSize ;
            rc = ossTruncateFile( &_file, (INT64)fileSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                       _suFileName, fileSize, rc ) ;
               goto error ;
            }
            PD_LOG( PDEVENT, "Truncate file[%s] to size[%llu]", _suFileName,
                    fileSize ) ;
         }
         else if ( fileSize < rightSize )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid file[%s] size[%llu], less than the "
                    "expected size[%llu], rc: %d",
                    _suFileName, fileSize, rightSize, rc ) ;
            goto error ;
         }
      }
      // sanity check, fileSize must be equal with rightSize,
      // all subsequent steps depend on this
      if ( rightSize != fileSize )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed in sanity check in file[%s], "
                 "fileSize: %llu, rightSize : %llu, rc = %d",
                 _suFileName, fileSize, rightSize, rc ) ;
         goto error ;
      }
      SDB_ASSERT( fileSize == rightSize, "Invalid file size" ) ;

      // calculate the info for extend/map/deposit, numSeg will be updated
      // to the actual number of increasing segments
      _calcExtendInfo( fileSize, numSeg, incFileSize, incPageNum ) ;

      // we'll check if adding new segments will exceed the limit
      beginExtentID = _dmsHeader->_pageNum ;
      endExtentID   = beginExtentID + incPageNum ;
      if ( endExtentID > DMS_MAX_PG )
      {
         PD_LOG( PDERROR, "Extent page[%u] exceed max pages[%u] in su[%s]",
                 endExtentID, DMS_MAX_PG, _suFileName ) ;
         rc = SDB_DMS_NOSPC ;
         goto error ;
      }
      // We'll also verify the SME shows DMS_SME_FREE for all needed pages
      for ( UINT32 i = beginExtentID; i < endExtentID; i++ )
      {
         if ( DMS_SME_FREE != _dmsSME->getBitMask( i ) )
         {
            rc = SDB_DMS_CORRUPTED_SME ;
            goto error ;
         }
      }

      // now we only hold extendsegment latch, no other sessions can extend
      // but other sessions can freely create new extents in existing segments
      // This should be safe because no one knows we are increasing the size of
      // file, so other sessions will not attempt to access the new space
      // then we need to increase the size of file first
      // MAKE SURE NOT HOLD ANY METADATA LATCH DURING SUCH EXPENSIVE DISK
      // OPERATION extendSeg latch is held here so that it's not possible //
      // two sessions doing same extend

      // try to extend file and map file
      newFileSize = fileSize + incFileSize ;
      if ( newFileSize > fileSize )
      {
         // extend file size
      retry:
         rc = ossExtend( &_file, fileSize, incFileSize,
                         _suDescriptor->getStorageInfo()._enableSparse ) ;
         if ( rc )
         {
            INT32 rc1 = SDB_OK ;
            PD_LOG ( PDWARNING, "Failed to extend storage unit for %llu "
                     "bytes, sparse:%s, rc: %d", incFileSize,
                     _suDescriptor->getStorageInfo()._enableSparse ? "TRUE" : "FALSE", rc ) ;

            // truncate the file when it's failed to extend file
            rc1 = ossTruncateFile ( &_file, fileSize ) ;
            if ( rc1 )
            {
               PD_LOG ( PDSEVERE, "Failed to revert the increase of segment, "
                        "rc = %d", rc1 ) ;
               // if we increased the file size but got error, and we are not able
               // to decrease it, something BIG wrong, let's panic
               ossPanic () ;
            }

            if ( SDB_INVALIDARG == rc && _suDescriptor->getStorageInfo()._enableSparse )
            {
               _suDescriptor->getStorageInfo()._enableSparse = FALSE ;
               goto retry ;
            }
            // we need to manage how to truncate the file to original size here
            goto error ;
         }

         // map all new segments into memory
         while ( fileSize < newFileSize )
         {
            rc = map ( fileSize, _getSegmentSize(), NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to map storage unit from "
                        "offset %llu", fileSize ) ;
               goto error ;
            }
            _maxSegID += 1 ;
            _dirtyList.setSize( ossMmapFile::segmentSize() - _dataSegID ) ;
            fileSize += _getSegmentSize() ;
            mapSegNum++ ;
         }
      }
      // sanity check
      if ( newFileSize != fileSize )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed in sanity check in file[%s], "
                 "fileSize: %llu, newFileSize: %llu, rc = %d",
                 _suFileName, fileSize, newFileSize, rc ) ;
         goto error ;
      }
      if ( numSeg != mapSegNum )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed in sanity check in file[%s], "
                 "numSeg: %d, mapSegNum : %d, rc = %d",
                 _suFileName, numSeg, mapSegNum, rc ) ;
         goto error ;
      }

      // deposit new pages to SME and then update info in file header
      while ( beginExtentID < endExtentID )
      {
         UINT32 incPages = 0 ;
         if ( 0 == beginExtentID % _segmentPages )
         {
            // when at the beginning of a segment, we have the
            // follow two cases to handle
            if ( ( endExtentID - beginExtentID ) < _segmentPages )
            {
               // cast 1: increased pages inside the range of a segmentPages
               //
               //              segmentPages
               // --------|-------------------------------|
               //
               //    beginExtID                endExtID
               // --------|-----------------------|
               incPages = endExtentID - beginExtentID ;
            }
            else
            {
               // cast 2: increased pages outside the range of a segmentPages
               //
               //              segmentPages           segmentPages
               // --------|-------------------|~|-------------------|
               //
               //    beginExtID                  endExtID
               // --------|-------------------------|
               incPages = _segmentPages ;
            }
            // we need to set _storageUnitSize before deposit()
            // If not, FreeSize calculated by snapshot cs may be
            // larger than TotalSize.
            _dmsHeader->_storageUnitSize += incPages ;
            // add pages to a new segmentSpace
            rc = _smeMgr.depositPages( (dmsExtentID)beginExtentID, incPages ) ;
         }
         else
         {
            UINT32 freePages = _segmentPages - ( beginExtentID % _segmentPages ) ;
            // when not at the beginning of a segment, we have
            // the follow two cases to handle
            if ( ( endExtentID - beginExtentID ) < freePages )
            {
               // cast 1: increased pages inside the range of a segmentPages
               //
               //              segmentPages
               // --------|-------------------------------|
               //
               //          beginExtID              endExtID
               // --------------|---------------------|
               incPages = endExtentID - beginExtentID ;
            }
            else
            {
               // cast 2: increased pages outside the range of a segmentPages
               //
               //              segmentPages           segmentPages
               // --------|-------------------|~|-------------------|
               //
               //          beginExtID                  endExtID
               // --------------|-------------------------|
               incPages = freePages ;
            }
            _dmsHeader->_storageUnitSize += incPages ;
            // append pages to the last segmentSpace
            rc = _smeMgr.appendPages( (dmsExtentID)beginExtentID, incPages ) ;
         }
         if ( rc )
         {
            _dmsHeader->_storageUnitSize -= incPages ;
            PD_LOG ( PDSEVERE, "Failed to deposit pages[%d] into SMEMgr, "
                     "segmentPages: %d, _storageUnitSize: %llu, "
                     "_pageNum: %llu, rc = %d",
                     incPages, _segmentPages,
                     _dmsHeader->_storageUnitSize,
                     _dmsHeader->_pageNum, rc ) ;
            ossPanic() ;
            goto error ;
         }
         beginExtentID += incPages ;
         // update header
         _dmsHeader->_pageNum += incPages ;
         _pageNum = _dmsHeader->_pageNum ;
      }
      // sanity check
      if ( endExtentID != beginExtentID )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed in sanity check in file[%s], "
                 "beginExtentID: %d, endExtentID : %d, rc = %d",
                 _suFileName, beginExtentID, endExtentID, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEBASE__EXTENDSEG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   UINT32 _dmsStorageBase::_extendThreshold () const
   {
      if ( _isSysSU )
      {
         return DMS_SYS_EXTEND_THRESHOLD_SIZE >> _pageSizeSquare ;
      }
      else if ( _suDescriptor )
      {
         return _suDescriptor->getStorageInfo()._extentThreshold >> _pageSizeSquare ;
      }
      return (UINT32)( DMS_EXTEND_THRESHOLD_SIZE >> _pageSizeSquare ) ;
   }

   UINT32 _dmsStorageBase::_getSegmentSize() const
   {
      if ( 0 == _segmentSize )
      {
         return DMS_SEGMENT_SZ ;
      }
      return _segmentSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE__FINDFREESPACE, "_dmsStorageBase::_findFreeSpace" )
   INT32 _dmsStorageBase::_findFreeSpace( UINT16 numPages, dmsExtentID & foundPage,
                                          dmsContext *context )
   {
      UINT32 totalDataPageNum = 0 ;
      INT32 rc                = SDB_OK ;
      INT32 rc1               = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEBASE__FINDFREESPACE ) ;

      while ( TRUE )
      {
         totalDataPageNum = _pageNum ;
         rc = _smeMgr.reservePages( numPages, foundPage ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( DMS_INVALID_EXTENT != foundPage )
         {
            break ;
         }

         // if not able to find any, that means all pages are occupied
         // then we should call extendSegments
         if ( ossTestAndLatch( _segmentLatch.get(), EXCLUSIVE ) )
         {
            // double check to avoid extending segment multiple times,
            // _pageNum will be updated if extending segment has happened
            if ( totalDataPageNum != _pageNum  )
            {
               ossUnlatch( _segmentLatch.get(), EXCLUSIVE ) ;
               continue ;
            }

            // begin for extent
            rc = context ? context->pause() : SDB_OK ;
            if ( rc )
            {
               ossUnlatch( _segmentLatch.get(), EXCLUSIVE ) ;
               PD_LOG( PDERROR, "Failed to pause context[%s], rc: %d",
                       context->toString().c_str(), rc ) ;
               goto error ;
            }

            rc = _extendSegments( 1 ) ;

            // end to resume
            rc1 = context ? context->resume() : SDB_OK ;

            ossUnlatch( _segmentLatch.get(), EXCLUSIVE ) ;

            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to extend storage unit, rc=%d", rc );
               goto error ;
            }
            SDB_ASSERT( _pStatMgr, "should not be null" ) ;
            if ( NULL != _pStatMgr )
            {
               // update totalPageAllocate counter
               _pStatMgr->incPageAllocate( numPages ) ;
            }
            PD_RC_CHECK( rc1, PDERROR, "Failed to resume context[%s], rc: %d",
                         context->toString().c_str(), rc1 ) ;

            PD_LOG ( PDDEBUG, "Successfully extend storage unit for %d pages",
                     numPages ) ;
         }
         else
         {
            BOOLEAN needAlloc = TRUE ;
            // begin for extent
            rc = context ? context->pause() : SDB_OK ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pause context[%s], rc: %d",
                         context->toString().c_str(), rc ) ;
            ossLatch( _segmentLatch.get(), SHARED ) ;
            ossUnlatch( _segmentLatch.get(), SHARED );
            // end to resume
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

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEBASE__FINDFREESPACE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageBase::_releaseSpace( SINT32 pageStart, UINT16 numPages )
   {
      INT32 rc = SDB_OK ;
      rc = _smeMgr.releasePages( pageStart, numPages ) ;
      if ( SDB_OK == rc )
      {
         SDB_ASSERT( _pStatMgr, "should not be null" ) ;
         if ( NULL != _pStatMgr )
         {
            // update totalPageRelease counter
            _pStatMgr->incPageRelease( numPages ) ;
         }
      }
      return rc ;
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

   INT32 _dmsStorageBase::flushPages( dmsExtentID pageID, UINT16 pageNum,
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
      /// When in openStorage, the _dataSegID is not init and the
      /// _dirtyList is also not init
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

      /// first flush meta
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEBASE__RESETINOFBYNAME, "_dmsStorageBase::_resetInfoByName" )
   void _dmsStorageBase::_resetInfoByName( const CHAR *csName )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEBASE__RESETINOFBYNAME ) ;

      _isTempSU = FALSE ;
      _isSysSU = FALSE ;
      _blockScanSupport = TRUE ;

      if ( 0 == ossStrcmp( csName, SDB_DMSTEMP_NAME ) )
      {
         _isTempSU = TRUE ;
         _isSysSU = TRUE ;
         _blockScanSupport = FALSE ;
      }
      else if ( 0 == ossStrncmp( csName, "SYS", 3 ) )
      {
         _isSysSU = TRUE ;
         _blockScanSupport = FALSE ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEBASE__RESETINOFBYNAME ) ;
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
            /// flush header
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
      BOOLEAN setHeadCommFlgValid = TRUE ;

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
               _onMarkHeaderValid( lastLSN, sync, lastTime,
                                   setHeadCommFlgValid ) ;

               tmpCommitFlag = _isCrash ? 0 : _commitFlag ;
               if ( hasFlushedData ||
                    tmpCommitFlag != _dmsHeader->_commitFlag ||
                    lastLSN != _dmsHeader->_commitLsn )
               {
                  if ( setHeadCommFlgValid )
                  {
                     _dmsHeader->_commitFlag = tmpCommitFlag ;
                  }
                  _dmsHeader->_commitLsn = lastLSN ;
                  _dmsHeader->_commitTime = lastTime ;
                  /// flush header
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

   void _dmsStorageBase::_calcExtendInfo( const UINT64 fileSize,
                                          UINT32 &numSeg,
                                          UINT64 &incFileSize,
                                          UINT32 &incPageNum )
   {
      SDB_ASSERT( _segmentPages > 0, "Not initialized segment pages" ) ;
      incFileSize = (UINT64)_getSegmentSize() * numSeg ;
      incPageNum  = _segmentPages * numSeg ;
   }

}


