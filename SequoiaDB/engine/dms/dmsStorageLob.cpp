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

   Source File Name = dmsStorageLob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageLob.hpp"
#include "dpsOp2Record.hpp"
#include "pmd.hpp"
#include "dpsTransCB.hpp"
#include "dmsTrace.hpp"
#include "pdTrace.hpp"
#include "monClass.hpp"

namespace engine
{
   /// define for lobm to check its segment size
   #define DMS_SEGMENT_SZ16K      (16*1024)           /// 16KB
   #define DMS_SEGMENT_SZ32K      (32*1024)           /// 32KB
   #define DMS_SEGMENT_SZ64K      (64*1024)           /// 64KB
   #define DMS_SEGMENT_SZ128K     (128*1024)          /// 128KB
   #define DMS_SEGMENT_SZ256K     (256*1024)          /// 256KB
   #define DMS_SEGMENT_SZ512K     (512*1024)          /// 512KB
   #define DMS_SEGMENT_SZ1M       (1*1024*1024)       /// 1MB
   #define DMS_SEGMENT_SZ2M       (2*1024*1024)       /// 2MB
   #define DMS_SEGMENT_SZ4M       (4*1024*1024)       /// 4MB
   #define DMS_SEGMENT_SZ8M       (8*1024*1024)       /// 8MB

   #define DMS_LOB_EXTEND_THRESHOLD_SIZE     ( 65536 )   // 64K

   #define DMS_LOB_PAGE_IN_USED( page )\
           ( DMS_SME_ALLOCATED == getSME()->getBitMask( page ) )

   #define DMS_LOB_GET_HASH_FROM_BLK( blk, hash )\
           do\
           {\
              const BYTE *d1 = (blk)->_oid ;\
              const BYTE *d2 = ( const BYTE * )( &( (blk)->_sequence ) ) ;\
              (hash) = ossHash( d1, sizeof( (blk)->_oid ),\
                                d2, sizeof( (blk)->_sequence ) ) ;\
           } while( FALSE )

   /*
      _dmsStorageLob implement
   */
   _dmsStorageLob::_dmsStorageLob( IStorageService *service,
                                   dmsSUDescriptor *suDescriptor,
                                   const CHAR *lobmFileName,
                                   const CHAR *lobdFileName,
                                   dmsStorageDataCommon *pDataSu,
                                   utilCacheUnit *pCacheUnit )
   :_dmsStorageBase( service, suDescriptor, lobmFileName ),
    _dmsBME( NULL ),
    _dmsData( (dmsStorageData *)pDataSu ),      // TODO: temporary cast
    _data( lobdFileName,
           suDescriptor->getStorageInfo()._enableSparse,
           suDescriptor->getStorageInfo()._directIO ),
    _delayOpenLatch( MON_LATCH_DMSSTORAGELOB_DELAYOPENLATCH ),
    _pCacheUnit( pCacheUnit ),
    _pSyncMgrTmp( NULL ),
    _pStatMgrTmp( NULL )
   {
      ossMemset( _path, 0, sizeof( _path ) ) ;
      ossMemset( _metaPath, 0, sizeof( _metaPath ) ) ;
      _needDelayOpen = FALSE ;

      _dmsData->_attachLob( this ) ;
      _isRename = FALSE ;
      _dataSegmentSize = 0 ;
   }

   _dmsStorageLob::~_dmsStorageLob()
   {
      _dmsData->_detachLob() ;
      _dmsData = NULL ;
      _dmsBME = NULL ;

      for ( UINT32 i = 0 ; i < _vecBucketLacth.size() ; ++i )
      {
         SDB_OSS_DEL _vecBucketLacth[ i ] ;
      }
      _vecBucketLacth.clear() ;
   }

   void _dmsStorageLob::syncMemToMmap ()
   {
      if ( _dmsData && _data.isOpened() )
      {
         _dmsData->syncMemToMmap() ;
         _dmsData->flushMME( isSyncDeep() ) ;
      }
   }

   INT32 _dmsStorageLob::_renameMetaOrDataFile( const CHAR* metaFilePath,
                                                const CHAR* dataFilePath )
   {
      INT32 rc = SDB_OK ;
      CHAR fullPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( NULL == metaFilePath || NULL == dataFilePath )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "metaFilePath or dataFilePath can't be NULL, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      // rename lob meta file
      rc = utilBuildFullPath( metaFilePath, _suFileName, OSS_MAX_PATHSIZE,
                              fullPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "File path are too long: %s/%s, rc: %d",
                  metaFilePath, _suFileName, rc ) ;
         goto error ;
      }

      if ( SDB_OK == ossAccess( fullPath ) )
      {
         rc = dmsRenameInvalidFile( fullPath ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      ossMemset( fullPath, 0 ,sizeof( fullPath ) ) ;

      // rename lob data file
      rc = utilBuildFullPath( dataFilePath, _data.getFileName(),
                              OSS_MAX_PATHSIZE, fullPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "File path are too long: %s/%s, rc: %d",
                  dataFilePath, _data.getFileName(), rc ) ;
         goto error ;
      }

      if ( SDB_OK == ossAccess( fullPath ) )
      {
         rc = dmsRenameInvalidFile( fullPath ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageLob::_checkIfMetaOrDataFileExist( const CHAR* metaFilePath,
                                                      const CHAR* dataFilePath,
                                                      BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      CHAR fileFullPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      exist = FALSE ;

      if ( NULL == metaFilePath || NULL == dataFilePath )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "metaFilePath or dataFilePath can't be NULL, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      rc = utilBuildFullPath( metaFilePath, _suFileName,
                              OSS_MAX_PATHSIZE, fileFullPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "File path are too long: %s/%s, rc: %d",
                  metaFilePath, _suFileName, rc ) ;
         goto error ;
      }

      if ( SDB_OK == ossAccess( fileFullPath ) )
      {
         exist = TRUE ;
         goto done ;
      }

      ossMemset( fileFullPath, 0, sizeof( fileFullPath ) ) ;

      rc = utilBuildFullPath( dataFilePath, _data.getFileName(),
                              OSS_MAX_PATHSIZE, fileFullPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "File path are too long: %s/%s, rc: %d",
                  metaFilePath, _suFileName, rc ) ;
         goto error ;
      }

      if ( SDB_OK == ossAccess( fileFullPath ) )
      {
         exist = TRUE ;
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_OPEN, "_dmsStorageLob::open" )
   INT32 _dmsStorageLob::open( const CHAR *path,
                               const CHAR *metaPath,
                               IDataSyncManager *pSyncMgr,
                               IDataStatManager *pStatMgr,
                               BOOLEAN createNew )
   {
      INT32 rc              = SDB_OK ;
      BOOLEAN exist         = FALSE ;
      BOOLEAN needCalcCount = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_OPEN ) ;

      // copy path
      ossStrncpy( _path, path, OSS_MAX_PATHSIZE ) ;
      ossStrncpy( _metaPath, metaPath, OSS_MAX_PATHSIZE ) ;
      _pSyncMgrTmp = pSyncMgr ;
      _pStatMgrTmp = pStatMgr ;

      // if not create lobs
      if ( 0 == _dmsData->getHeader()->_createLobs )
      {
         rc = _checkIfMetaOrDataFileExist( metaPath, path, exist ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( exist )
         {
            PD_LOG( PDERROR, "Invalid meta or data file. "
                    "They must not exist" ) ;

            rc = _renameMetaOrDataFile( metaPath, path ) ;
            if ( rc )
            {
               goto error ;
            }

            rc = SDB_DMS_INVALID_SU ;
            goto error ;
         }
         _needDelayOpen = TRUE ;
      }
      else
      {
         for( UINT16 i = 0 ; i < DMS_MME_SLOTS ; i++ )
         {
            if ( !DMS_IS_MB_INUSE( _dmsData->_dmsMME->_mbList[i]._flag ) )
            {
               continue ;
            }
            if ( _dmsData->_mbStatInfo[i]._totalLobs.fetch() > 0 &&
                 ( _dmsData->_mbStatInfo[i]._totalLobSize.fetch() <= 0 ||
                   _dmsData->_mbStatInfo[i]._totalValidLobSize.fetch() <= 0 ) )
            {
               needCalcCount = TRUE ;
               break ;
            }
         }

         rc = _openLob( path, metaPath, createNew ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         /// when open exist lob files, need to analysis the lob count
         if ( ( !createNew && getHeader()->_version <= DMS_LOB_VERSION_1 ) ||
              needCalcCount )
         {
            rc = _calcCount() ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_RENAME, "_dmsStorageLob::rename" )
   INT32 _dmsStorageLob::rename( const CHAR *csName,
                                 const CHAR *lobmFileName,
                                 const CHAR *lobdFileName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_RENAME ) ;

      _isRename = TRUE ;
      if ( _needDelayOpen )
      {
         ossStrncpy( _suFileName, lobmFileName, DMS_SU_FILENAME_SZ ) ;
         _suFileName[ DMS_SU_FILENAME_SZ ] = '\0' ;
      }
      else
      {
         rc = renameStorage( csName, lobmFileName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename lob meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

      /// rename lob data
      rc = _data.rename( csName, lobdFileName, pmdGetThreadEDUCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename lob data failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      _isRename = FALSE ;
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_RENAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__DELAYOPEN, "_dmsStorageLob::_delayOpen" )
   INT32 _dmsStorageLob::_delayOpen()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__DELAYOPEN ) ;

      _delayOpenLatch.get() ;

      if ( !_needDelayOpen )
      {
         goto done ;
      }

      if ( isOpened() )
      {
         goto done ;
      }

      rc = _openLob( _path, _metaPath, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Delay open[%s] failed, rc: %d",
                 getSuName(), rc ) ;
         goto error ;
      }

      _needDelayOpen = FALSE ;

      // set data header
      _dmsData->updateCreateLobs( 1 ) ;

   done:
      _delayOpenLatch.release() ;
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__DELAYOPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__OPENLOB, "_dmsStorageLob::_openLob" )
   INT32 _dmsStorageLob::_openLob( const CHAR *path,
                                   const CHAR *metaPath,
                                   BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__OPENLOB ) ;

      ossSpinSLatch *pLatch = NULL ;
      /// create bucket latch
      for ( UINT32 i = 0 ; i < DMS_BUCKETS_LATCH_SIZE ; ++i )
      {
         pLatch = SDB_OSS_NEW ossSpinSLatch ;
         if ( !pLatch )
         {
            PD_LOG( PDERROR, "Create bucket latch[%d] failed", i ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _vecBucketLacth.push_back( pLatch ) ;
      }

      /// Init cache unit
      rc = _pCacheUnit->init( getLobData(), _suDescriptor->getStorageInfo()._lobdPageSize,
                              _suDescriptor->getStorageInfo()._pageAllocTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init cache unit failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( _suDescriptor->getStorageInfo()._cacheMergeSize > 0 )
      {
         rc = _pCacheUnit->enableMerge( _suDescriptor->getStorageInfo()._directIO,
                                        _suDescriptor->getStorageInfo()._cacheMergeSize ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Enable cache merge for lob[%s] failed, rc: %d",
                    getSuName(), rc ) ;
            /// ignored this error
            rc = SDB_OK ;
         }
      }

      rc = openStorage( metaPath, _pSyncMgrTmp, _pStatMgrTmp, createNew ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lobm file:%s, rc:%d",
                 _suFileName, rc ) ;
         if ( createNew && SDB_FE != rc )
         {
            goto rmlobm ;
         }
         goto error ;
      }

      rc = _data.open( path, createNew, _dataSegmentSize,
                       getHeader()->_pageNum, _suDescriptor->getStorageInfo(),
                       pmdGetThreadEDUCB() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lobd file:%s, rc:%d",
                 _data.getFileName(), rc ) ;
         if ( createNew )
         {
            if ( SDB_FE != rc )
            {
               goto rmboth ;
            }
            goto rmlobm ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__OPENLOB, rc ) ;
      return rc ;
   error:
      _data.close() ;
      close() ;
      goto done ;
   rmlobm:
      removeStorage() ;
      goto error ;
   rmboth:
      removeStorageFiles() ;
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_REMOVESTORAGEFILES, "_dmsStorageLob::removeStorageFiles" )
   void _dmsStorageLob::removeStorageFiles()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_REMOVESTORAGEFILES ) ;
      rc = removeStorage() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove file:%d", rc ) ;
      }

      rc = _data.remove() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove file:%d", rc ) ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGELOB_REMOVESTORAGEFILES ) ;
      return ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_GETLOBMETA, "_dmsStorageLob::getLobMeta" )
   INT32 _dmsStorageLob::getLobMeta( const bson::OID &oid,
                                     dmsMBContext *mbContext,
                                     pmdEDUCB *cb,
                                     _dmsLobMeta &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_GETLOBMETA ) ;
      UINT32 readSz = 0 ;
      dmsLobRecord piece ;
      piece.set( &oid, DMS_LOB_META_SEQUENCE, 0,
                 sizeof( meta ), NULL ) ;
      rc = read( piece, mbContext, cb,
                 ( CHAR * )( &meta ), readSz ) ;
      if ( SDB_OK == rc )
      {
         if ( sizeof( meta ) != readSz )
         {
            PD_LOG( PDERROR, "read length is %d, big error!", readSz ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         goto done ;
      }
      else if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
      {
         rc = SDB_FNE ;
         goto error ;
      }
      else
      {
         PD_LOG( PDERROR, "failed to read meta of lob, rc:%d",
                 rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_GETLOBMETA, rc ) ;
      return rc ;
   error:
      meta.clear() ;
      goto done ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_WRITELOBMETA, "_dmsStorageLob::writeLobMeta" )
   INT32 _dmsStorageLob::writeLobMeta( const bson::OID &oid,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB *cb,
                                       const _dmsLobMeta &meta,
                                       BOOLEAN isNew,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_WRITELOBMETA ) ;
      dmsLobRecord piece ;
      piece.set( &oid, DMS_LOB_META_SEQUENCE, 0,
                 sizeof( meta ), ( const CHAR * )( &meta ) ) ;
      if ( isNew )
      {
         rc = write( piece, mbContext, cb, dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = update( piece, mbContext, cb, dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITELOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_WRITEWITHDPSLOG, "_dmsStorageLob::_writeWithDpslog" )
   INT32 _dmsStorageLob::_writeWithDpslog( const dmsLobRecord &record,
                                           const CHAR *pFullName,
                                           dmsMBContext *mbContext,
                                           BOOLEAN canUnLock,
                                           BOOLEAN updateWhenExist,
                                           _pmdEDUCB *cb,
                                           dpsMergeInfo &info,
                                           SDB_DPSCB *dpscb,
                                           BOOLEAN *hasUpdated )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_WRITEWITHDPSLOG ) ;
      SDB_ASSERT( pFullName, "FullName can't be NULL" ) ;

      dmsWriteGuard guard( _service, _dmsData, mbContext, cb, TRUE, FALSE, TRUE );

      if ( !mbContext->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold lock with EXCLUSIVE" ) ;
         goto error ;
      }

      if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Write record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_INSERT ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      
      {
         guard.begin();
         std::shared_ptr< ILob > lobPtr ;
         rc = mbContext->getCollPtr()->getLobPtr( lobPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get lob storage ptr, rc: %d", rc ) ;
         if ( updateWhenExist )
         {
            ILob::updatedInfo info ;
            rc = lobPtr->writeOrUpdate( record, cb, &info ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to write or update on lob, rc: %d", rc ) ;
            // written
            if ( !info.hasUpdated )
            {
               mbContext->mbStat()->_totalLobPages.inc() ;
               if ( DMS_IS_LOBMETA_RECORD( record ) )
               {
                  mbContext->mbStat()->_totalLobs.inc() ;
                  INT64 lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( record._dataLen ) ;
                  mbContext->mbStat()->addTotalLobSize( lobPieceLen ) ;
                  _statVaildLobSize( mbContext, (_dmsLobMeta *)record._data, NULL ) ;
               }
               else
               {
                  mbContext->mbStat()->addTotalLobSize( record._dataLen ) ;
               }

               _incWriteRecord() ;
            }
            // updated
            else
            {
               mbContext->mbStat()->addTotalLobSize( info.increasedSize ) ;
            }

            if ( hasUpdated )
            {
               *hasUpdated = info.hasUpdated ;
            }
         }
         else
         {
            rc = lobPtr->write( record, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to write or update on lob, rc: %d", rc ) ;
            mbContext->mbStat()->_totalLobPages.inc();
            if ( DMS_IS_LOBMETA_RECORD( record ) )
            {
               mbContext->mbStat()->_totalLobs.inc() ;
               INT64 lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( record._dataLen ) ;
               mbContext->mbStat()->addTotalLobSize( lobPieceLen ) ;
               _statVaildLobSize( mbContext, (_dmsLobMeta *)record._data, NULL ) ;
            }
            else
            {
               mbContext->mbStat()->addTotalLobSize( record._dataLen ) ;
            }
         }
         guard.commit();
      }

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(),
                         *record._oid, record._sequence, cb ) ;
         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare dps log, rc:%d", rc ) ;
            goto error ;
         }
         if ( canUnLock )
         {
            mbContext->mbUnlock() ;
         }
         dpscb->writeData( info ) ;
      }
      else
      {
         cb->setDataExInfo( pFullName, _dmsData->logicalID(),
                            mbContext->clLID(), *record._oid, record._sequence ) ;
      }

      /// update last lsn
      if ( cb->getLsnCount() > 0 )
      {
         mbContext->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                     DMS_FILE_LOB,
                                                     cb->isDoRollback() ) ;
      }

   done:
      if ( canUnLock )
      {
         mbContext->mbUnlock() ;
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITEWITHDPSLOG, rc ) ;
      return rc ;
   error:
      guard.abort();
      goto done ;
   }

   INT32 _dmsStorageLob::write( const dmsLobRecord &record,
                                dmsMBContext *mbContext,
                                pmdEDUCB *cb,
                                SDB_DPSCB *dpscb )
   {
      return _writeInner( record, mbContext, cb, dpscb, FALSE, NULL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_UPDATEWITHDPSLOG, "_dmsStorageLob::_updateWithDpslog" )
   INT32 _dmsStorageLob::_updateWithDpslog( const dmsLobRecord &record,
                                          const CHAR *pFullName,
                                          dmsMBContext *mbContext,
                                          BOOLEAN canUnLock,
                                          _pmdEDUCB *cb,
                                          SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_UPDATEWITHDPSLOG ) ;
      SDB_ASSERT( pFullName, "FullName can't be NULL" ) ;

      dmsWriteGuard guard( _service, _dmsData, mbContext, cb, TRUE, FALSE, TRUE );

      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      CHAR *oldData = NULL ;
      UINT32 oldLen = 0 ;
      UINT32 pageSize = _data.pageSize() ;
      std::shared_ptr< ILob > lobPtr ;

      if ( !mbContext->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold lock with EXCLUSIVE" ) ;
         goto error ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in update", getSuName() ) ;
         goto error ;
      }
      else if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Update record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_INSERT ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      guard.begin();

      rc = mbContext->getCollPtr()->getLobPtr( lobPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get lob storage ptr, rc: %d", rc ) ;

      /// read old data when we need dps or update dmsLobMeta
      if ( NULL != dpscb || DMS_IS_LOBMETA_RECORD( record ) )
      {
         /// alloc memory
         rc = cb->allocBuff( record._dataLen, &oldData ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc read buffer[%u] failed, rc: %d",
                    record._dataLen, rc ) ;
            goto error ;
         }

         rc = lobPtr->read( record, cb, oldData, oldLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to read old lob data, rc: %d", rc ) ;

         /// need to unlock the mbContext, so the sync control is not hold
         /// the mbContext
         if ( canUnLock )
         {
            mbContext->mbUnlock() ;
         }

         if ( NULL != dpscb )
         {
            rc = dpsLobU2Record( pFullName,
                                 record._oid,
                                 record._sequence,
                                 record._offset,
                                 record._hash,
                                 record._dataLen,
                                 record._data,
                                 oldLen,
                                 oldData,
                                 pageSize,
                                 DMS_LOB_INVALID_PAGEID,
                                 transID,
                                 preTransLsn,
                                 relatedLsn,
                                 logRecord ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
               goto error ;
            }

            rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
               goto error ;
            }

            rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u), rc: %d",
                     logRecord.head()._length, rc ) ;
               info.clear() ;
               goto error ;
            }
         }
      }

      rc = lobPtr->update( record, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update lob, rc: %d", rc ) ;

      guard.commit();

      mbContext->mbStat()->addTotalLobSize( static_cast< INT32 >( record._dataLen - oldLen ) ) ;

      _incWriteRecord() ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(),
                         *record._oid, record._sequence, cb ) ;
         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare dps log, rc:%d", rc ) ;
            goto error ;
         }
         dpscb->writeData( info ) ;
      }
      else
      {
         cb->setDataExInfo( pFullName, _dmsData->logicalID(),
                            mbContext->clLID(), *record._oid, record._sequence ) ;
      }

      if ( cb->getLsnCount() > 0 )
      {
         /// not in mbContext lock, so use update with compare
         mbContext->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                     DMS_FILE_LOB,
                                                     cb->isDoRollback() ) ;
      }

      if ( DMS_IS_LOBMETA_RECORD( record ) )
      {
         _statVaildLobSize( mbContext, ( _dmsLobMeta* )record._data,
                            ( _dmsLobMeta* )oldData ) ;
      }

   done:
      if ( canUnLock )
      {
         mbContext->mbUnlock() ;
      }

      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }
      if ( oldData )
      {
         cb->releaseBuff( oldData ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_UPDATEWITHDPSLOG, rc ) ;
      return rc ;
   error:
      guard.abort();
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_UPDATE, "_dmsStorageLob::update" )
   INT32 _dmsStorageLob::update( const dmsLobRecord &record,
                                 dmsMBContext *mbContext,
                                 pmdEDUCB *cb,
                                 SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_UPDATE ) ;
      SDB_ASSERT( NULL != mbContext && NULL != cb, "can not be null" ) ;

      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in update, rc: %d", rc ) ;
      }

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      rc = _updateWithDpslog( record, fullName, mbContext, locked, cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update on lob, rc: %d", rc ) ;

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_UPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_WRITEINNER, "_dmsStorageLob::_writeInner" )
   INT32 _dmsStorageLob::_writeInner( const dmsLobRecord &record,
                                      dmsMBContext *mbContext,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb,
                                      BOOLEAN updateWhenExist,
                                      BOOLEAN *pHasUpdated )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_WRITEINNER ) ;
      SDB_ASSERT( NULL != mbContext && NULL != cb, "can not be null" ) ;

      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      UINT32 pageSize = 0 ;
      BOOLEAN locked = FALSE ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed, rc: %d", rc ) ;
      }
      // get page size
      pageSize = _data.pageSize() ;

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( NULL != dpscb )
      {
         rc = dpsLobW2Record( fullName,
                              record._oid,
                              record._sequence,
                              record._offset,
                              record._hash,
                              record._dataLen,
                              record._data,
                              pageSize,
                              DMS_LOB_INVALID_PAGEID,
                              transID,
                              preTransLsn,
                              relatedLsn,
                              logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u), rc: %d",
                    logRecord.head()._length, rc ) ;
            info.clear() ;
            goto error ;
         }
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      rc = _writeWithDpslog( record, fullName, mbContext, locked, updateWhenExist, cb, info, dpscb,
                             pHasUpdated );
      PD_RC_CHECK( rc, PDERROR, "Failed to write lob with dps log, rc: %d", rc ) ;

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITEINNER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageLob::writeOrUpdate( const dmsLobRecord &record,
                                        dmsMBContext *mbContext,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpscb,
                                        BOOLEAN *pHasUpdated )
   {
      return _writeInner( record, mbContext, cb, dpscb, TRUE, pHasUpdated ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_READ, "_dmsStorageLob::read" )
   INT32 _dmsStorageLob::read( const dmsLobRecord &record,
                               dmsMBContext *mbContext,
                               pmdEDUCB *cb,
                               CHAR *buf,
                               UINT32 &readLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_READ ) ;
      BOOLEAN locked = FALSE ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in read, rc: %d", rc ) ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in read", getSuName() ) ;
         goto error ;
      }
      else if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Read record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      {
         std::shared_ptr< ILob > lobPtr ;
         rc = mbContext->getCollPtr()->getLobPtr( lobPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get lob storage ptr, rc: %d", rc ) ;
         rc = lobPtr->read( record, cb, buf, readLen ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_LOB_SEQUENCE_NOT_EXIST ;
            goto error ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to read lob record, rc: %d", rc ) ;
         }
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_READ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_REMOVE, "_dmsStorageLob::remove" )
   INT32 _dmsStorageLob::remove( const dmsLobRecord &record,
                                 dmsMBContext *mbContext,
                                 pmdEDUCB *cb,
                                 SDB_DPSCB *dpscb,
                                 BOOLEAN onlyRemoveNewPage,
                                 const CHAR *pOldData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_REMOVE ) ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      UINT32 resevedLength = 0 ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      CHAR *readData = NULL ;
      UINT32 oldLen = 0 ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;
      BOOLEAN hasRemoved = FALSE ;
      UINT32 pageSize = 0 ;
      ossSpinSLatch *pLatch = NULL ;
      dmsLobRecord oldRecord ;
      UINT32 readLen = 0 ;
      BOOLEAN isMetaPage = FALSE ;
      std::shared_ptr< ILob > lobPtr ;

      dmsWriteGuard guard( _service, _dmsData, mbContext, cb, TRUE, FALSE, TRUE );

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in remove, rc: %d", rc ) ;
      }

      // get page size
      pageSize = _data.pageSize() ;

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      /// First to checkSyncControl and reserveLogSpace by a pageSize
      /// And this is outside of collection latch
      if ( dpscb )
      {
         oldLen = getLobdPageSize() ;
         rc = dpsLobRm2Record( fullName,
                               record._oid,
                               record._sequence,
                               record._offset,
                               record._hash,
                               oldLen,
                               readData,
                               pageSize,
                               DMS_LOB_INVALID_PAGEID,
                               transID,
                               preTransLsn,
                               relatedLsn,
                               logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u), rc: %d",
                    logRecord.head()._length, rc ) ;
            goto error ;
         }
         resevedLength = logRecord.head()._length ;
         /// clear log info
         oldLen = 0 ;
         info.clear() ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in remove", getSuName() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      rc = mbContext->getCollPtr()->getLobPtr( lobPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get lob storage ptr, rc: %d", rc ) ;

      /// When dpscb is NULL or not page 0, not to alloc the page when page is
      /// not in cache( use len = 0 )
      isMetaPage = DMS_LOB_META_SEQUENCE == record._sequence ;

      if ( isMetaPage && NULL == pOldData )
      {
         // for meta page, we need meta data to calculate valid size
         // if old data is passed, we can use the old data to calculate
         // otherwise, read from file
         readLen = DMS_LOB_META_LENGTH ;
      }
      else
      {
         readLen = getLobdPageSize() ;
      }

      rc = cb->allocBuff( readLen, &readData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to allocate read buffer[%u], rc: %d", readLen, rc ) ;

      {
         guard.begin();
         ossScopedLock lock( _getBucketLatch( _getBucket( record._hash ) ), SHARED ) ;
         rc = lobPtr->read( record, cb, readData, readLen ) ;
         if ( rc == SDB_DMS_EOC )
         {
            rc = SDB_OK ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to read lob to remove, rc: %d", rc ) ;
         oldLen = readLen ;
         if ( isMetaPage && NULL != pOldData )
         {
            // use the passed old data
            oldRecord._data = pOldData ;
         }
         else
         {
            oldRecord._data = readData ;
         }
      }

      /// lock bucket
      if ( dpscb )
      {
         pLatch = _getBucketLatch( _getBucket( record._hash ) ) ;
         pLatch->get() ;
      }

      {
         rc = lobPtr->remove( record, cb ) ;
         hasRemoved = TRUE ;
         if ( DMS_LOB_META_SEQUENCE == record._sequence )
         {
            mbContext->mbStat()->_totalLobs.dec() ;
            INT64 lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( oldLen ) ;
            mbContext->mbStat()->subTotalLobSize( lobPieceLen ) ;
         }
         else
         {
            mbContext->mbStat()->subTotalLobSize( oldLen ) ;
         }
      }

      /// release the mbContext
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }

      if ( dpscb )
      {
         rc = dpsLobRm2Record( fullName,
                               record._oid,
                               record._sequence,
                               record._offset,
                               record._hash,
                               oldLen,
                               readData,
                               pageSize,
                               DMS_LOB_INVALID_PAGEID,
                               transID,
                               preTransLsn,
                               relatedLsn,
                               logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(), *record._oid, record._sequence,
                         cb ) ;
         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare dps log, rc:%d", rc ) ;
            goto error ;
         }

         /// release bucket lock
         pLatch->release() ;
         pLatch = NULL ;
         /// write
         dpscb->writeData( info ) ;
      }
      else
      {
         cb->setDataExInfo( fullName, _dmsData->logicalID(),
                            mbContext->clLID(), *record._oid, record._sequence ) ;
      }

      guard.commit();

      if ( cb->getLsnCount() > 0 )
      {
         /// not with mbContext, so need update with compare
         mbContext->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                     DMS_FILE_LOB,
                                                     cb->isDoRollback() ) ;
      }

      // calculate lob valid size
      if ( isMetaPage )
      {
         SDB_ASSERT( NULL != oldRecord._data, "should have meta data" ) ;
         _statVaildLobSize( mbContext, NULL,
                            (const dmsLobMeta *)( oldRecord._data ) ) ;
      }

   done:
      if ( pLatch )
      {
         pLatch->release() ;
         pLatch = NULL ;
      }
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      if ( 0 != resevedLength )
      {
         transCB->releaseLogSpace( resevedLength, cb ) ;
      }
      if ( readData )
      {
         cb->releaseBuff( readData ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_REMOVE, rc ) ;
      return rc ;
   error:
      if ( hasRemoved )
      {
         ossPanic() ;
      }
      guard.abort();
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__PUSH2BUCKET, "_dmsStorageLob::_push2Bucket" )
   INT32 _dmsStorageLob::_push2Bucket( UINT32 bucket,
                                       DMS_LOB_PAGEID pageId,
                                       pmdEDUCB *cb,
                                       _dmsLobDataMapBlk &blk,
                                       const dmsLobRecord *pRecord )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__PUSH2BUCKET ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      ossScopedLock lock( _getBucketLatch( bucket ), EXCLUSIVE ) ;

      DMS_LOB_PAGEID &pageInBucket = _dmsBME->_buckets[bucket] ;
      /// empty bucket
      if ( DMS_LOB_INVALID_PAGEID == pageInBucket )
      {
         DMS_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_ADDRESSING, 1 ) ;
         pageInBucket = pageId ;
         blk._prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
         blk._nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
      }
      /// neet to find the last one
      else
      {
         dmsExtRW extRW ;
         DMS_LOB_PAGEID tmpPage = pageInBucket ;
         const _dmsLobDataMapBlk *lastBlk = NULL ;
         do
         {
            DMS_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_ADDRESSING, 1 ) ;
            /// Modify the list, well set to the null collection,
            /// because other collection's page data is not change
            extRW = extent2RW( tmpPage, -1 ) ;
            extRW.setNothrow( TRUE ) ;
            lastBlk = extRW.readPtr<_dmsLobDataMapBlk>() ;
            if ( !lastBlk )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                       tmpPage ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            /// check exist
            if ( pRecord &&
                 blk._clLogicalID == lastBlk->_clLogicalID &&
                 lastBlk->equals( pRecord->_oid->getData(),
                                  pRecord->_sequence ) )
            {
               PD_LOG( PDERROR, "Lob piece found, piece[%s], page:%d",
                       pRecord->toString().c_str(), tmpPage ) ;
               rc = SDB_LOB_SEQUENCE_EXISTS ;
               goto error ;
            }

            if ( DMS_LOB_INVALID_PAGEID == lastBlk->_nextPageInBucket )
            {
               _dmsLobDataMapBlk *writeBlk = NULL ;
               writeBlk = extRW.writePtr<_dmsLobDataMapBlk>() ;
               if ( !writeBlk )
               {
                  PD_LOG( PDERROR, "Get extent[%d] write address failed",
                          tmpPage ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               writeBlk->_nextPageInBucket = pageId ;
               blk._prevPageInBucket = tmpPage ;
               blk._nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
               break ;
            }
            else
            {
               tmpPage = lastBlk->_nextPageInBucket ;
               continue ;
            }
         } while ( TRUE ) ;
      }
      /// set page to normal
      blk.setNormal() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__PUSH2BUCKET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ONCREATE, "_dmsStorageLob::_onCreate" )
   INT32 _dmsStorageLob::_onCreate( OSSFILE *file, UINT64 curOffSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ONCREATE ) ;
      SDB_ASSERT( DMS_BME_OFFSET == curOffSet, "invalid offset" ) ;
      _dmsBucketsManagementExtent *bme = NULL ;

      bme = SDB_OSS_NEW _dmsBucketsManagementExtent() ;
      if ( NULL == bme )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _writeFile ( file, (const CHAR *)(bme),
                        sizeof( _dmsBucketsManagementExtent ) ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to write new bme to file rc: %d",
                  rc ) ;
         goto error ;
      }
   done:
      if ( NULL != bme )
      {
         SDB_OSS_DEL bme ;
         bme = NULL ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ONCREATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ONMAPMETA, "_dmsStorageLob::_onMapMeta" )
   INT32 _dmsStorageLob::_onMapMeta( UINT64 curOffSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ONMAPMETA ) ;
      rc = map ( DMS_BME_OFFSET, DMS_BME_SZ, (void**)&_dmsBME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map BME: %s", getSuFileName() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ONMAPMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   UINT32 _dmsStorageLob::_getSegmentSize() const
   {
      SDB_ASSERT( 0 != _segmentSize, "Not initialized" ) ;
      return _segmentSize ;
   }

   UINT32 _dmsStorageLob::_getMetaSizeOfDataSegment() const
   {
      SDB_ASSERT( _dataSegmentSize > 0, "Not initialized" ) ;
      SDB_ASSERT( _lobPageSize > 0, "Not initialized" ) ;
      SDB_ASSERT( _pageSize > 0, "Not initialized" ) ;
      return _dataSegmentSize / _lobPageSize * _pageSize ;
   }

   UINT32 _dmsStorageLob::_getDataSegmentPages() const
   {
      UINT32 dataSegmentPages = _data.segmentPages() ;
      SDB_ASSERT( dataSegmentPages > 0, "Not initialized" ) ;
      return dataSegmentPages ;
   }

   UINT32 _dmsStorageLob::_extendThreshold() const
   {
      if ( _suDescriptor )
      {
         return _suDescriptor->getStorageInfo()._extentThreshold >> _data.pageSizeSquareRoot() ;
      }
      return (UINT32)( DMS_LOB_EXTEND_THRESHOLD_SIZE >> pageSizeSquareRoot() ) ;
   }

   UINT64 _dmsStorageLob::_dataOffset()
   {
      return DMS_BME_OFFSET + DMS_BME_SZ ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__EXTENDSEGMENTS, "_dmsStorageLob::_extendSegments" )
   INT32 _dmsStorageLob::_extendSegments( UINT32 numSeg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__EXTENDSEGMENTS ) ;

      if ( _data.isOpened() )
      {
         INT64 extentLen = _data.getSegmentSize() ;
         rc = _data.extend( extentLen * numSeg ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend lobd file:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Lob data file is not opened" ) ;
         goto error ;
      }

      rc = this->_dmsStorageBase::_extendSegments( numSeg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extend lobm file:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__EXTENDSEGMENTS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsStorageLob::_getEyeCatcher() const
   {
      return DMS_LOBM_EYECATCHER ;
   }

   UINT32 _dmsStorageLob::_curVersion() const
   {
      return DMS_LOB_CUR_VERSION ;
   }

   INT32 _dmsStorageLob::_checkVersion( dmsStorageUnitHeader *pHeader )
   {
      INT32 rc = SDB_OK ;
      if ( pHeader->_version > _curVersion() )
      {
         PD_LOG( PDERROR, "Incompatible version: %u", pHeader->_version ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      }
      else if ( pHeader->_secretValue != _suDescriptor->getStorageInfo()._secretValue )
      {
         PD_LOG( PDERROR, "Secret value[%llu] not the same with data su[%llu]",
                 pHeader->_secretValue, _suDescriptor->getStorageInfo()._secretValue ) ;
         rc = SDB_DMS_SECRETVALUE_NOT_SAME ;
      }
      return rc ;
   }

   void _dmsStorageLob::_onClosed()
   {
      if ( !_isRename )
      {
         _data.close() ;
      }
   }

   INT32 _dmsStorageLob::_onOpened()
   {
      BOOLEAN needFlushMME = FALSE ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         _dmsData->_mbStatInfo[i]._lobLastWriteTick = ~0 ;
         _dmsData->_mbStatInfo[i]._lobCommitFlag.init( 1 ) ;

         if ( DMS_IS_MB_INUSE ( _dmsData->_dmsMME->_mbList[i]._flag ) )
         {
            /*
               Check the collection is valid
            */
            if ( !isCrashed() )
            {
               if ( 0 == _dmsData->_dmsMME->_mbList[i]._lobCommitFlag )
               {
                  /// upgrade from the old version which has no
                  /// _commitLSN/_idxCommitLSN/_lobCommitLSN in mb block,
                  /// so the value of _commitLSN/_idxCommitLSN/_lobCommitLSN is 0
                  if ( 0 == _dmsData->_dmsMME->_mbList[i]._lobCommitLSN )
                  {
                     _dmsData->_dmsMME->_mbList[i]._lobCommitLSN =
                        _suDescriptor->getStorageInfo()._curLSNOnStart ;
                  }
                  _dmsData->_dmsMME->_mbList[i]._lobCommitFlag = 1 ;
                  needFlushMME = TRUE ;
               }
               _dmsData->_mbStatInfo[i]._lobCommitFlag.init( 1 ) ;
            }
            else
            {
               _dmsData->_mbStatInfo[i]._lobCommitFlag.init(
                  _dmsData->_dmsMME->_mbList[i]._lobCommitFlag ) ;
            }
            _dmsData->_mbStatInfo[i]._lobIsCrash =
               ( 0 == _dmsData->_mbStatInfo[i]._lobCommitFlag.peek() ) ?
                                      TRUE : FALSE ;
            _dmsData->_mbStatInfo[i]._lobLastLSN.init(
               _dmsData->_dmsMME->_mbList[i]._lobCommitLSN ) ;
         }
      }

      if ( needFlushMME )
      {
         _dmsData->flushMME( isSyncDeep() ) ;
      }

      return SDB_OK ;
   }

   INT32 _dmsStorageLob::_onFlushDirty( BOOLEAN force, BOOLEAN sync )
   {
      INT32 rc = SDB_OK ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _dmsData->_mbStatInfo[i]._lobCommitFlag.init( 1 ) ;
      }
      if ( !isOpened() )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         /// flush cache to file
         if ( _pCacheUnit && _pCacheUnit->dirtyPages() > 0 )
         {
            _pCacheUnit->lockPageCleaner() ;
            UINT64 beginDirtyPages = 0 ;
            UINT32 syncPages = 0 ;
            while( _pCacheUnit->dirtyPages() > 0 )
            {
               beginDirtyPages = _pCacheUnit->dirtyPages() ;
               syncPages = _pCacheUnit->syncPages( pmdGetThreadEDUCB(),
                                                   TRUE, FALSE ) ;
               if ( 0 == syncPages ||
                    beginDirtyPages < _pCacheUnit->dirtyPages() + syncPages )
               {
                  break ;
               }
            }
            _pCacheUnit->unlockPageCleaner() ;
         }
         _data.flush() ;
      }
      return rc ;
   }

   INT32 _dmsStorageLob::_onMarkHeaderValid( UINT64 &lastLSN,
                                             BOOLEAN sync,
                                             UINT64 lastTime,
                                             BOOLEAN &setHeadCommFlgValid )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needFlush = FALSE ;
      UINT64 tmpLSN = 0 ;
      UINT32 tmpCommitFlag = 0 ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsData->_dmsMME->_mbList[i]._flag ) &&
              _dmsData->_mbStatInfo[i]._lobCommitFlag.peek() )
         {
            tmpLSN = _dmsData->_mbStatInfo[i]._lobLastLSN.peek() ;
            tmpCommitFlag = _dmsData->_mbStatInfo[i]._lobIsCrash ?
               0 : _dmsData->_mbStatInfo[i]._lobCommitFlag.peek() ;

            if ( tmpLSN != _dmsData->_dmsMME->_mbList[i]._lobCommitLSN ||
                 tmpCommitFlag != _dmsData->_dmsMME->_mbList[i]._lobCommitFlag )
            {
               _dmsData->_dmsMME->_mbList[i]._lobCommitLSN = tmpLSN ;
               _dmsData->_dmsMME->_mbList[i]._lobCommitTime = lastTime ;
               _dmsData->_dmsMME->_mbList[i]._lobCommitFlag = tmpCommitFlag ;
               needFlush = TRUE ;
            }

            /// update last lsn
            if ( (UINT64)~0 == lastLSN ||
                 ( (UINT64)~0 != tmpLSN && lastLSN < tmpLSN ) )
            {
               lastLSN = tmpLSN ;
            }
         }
      }

      if ( needFlush )
      {
         rc = _dmsData->flushMME( sync ) ;
      }
      return rc ;
   }

   INT32 _dmsStorageLob::_onMarkHeaderInvalid( INT32 collectionID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needSync = FALSE ;

      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         _dmsData->_mbStatInfo[ collectionID ]._lobLastWriteTick =
            pmdGetDBTick() ;
         if ( !_dmsData->_mbStatInfo[ collectionID ]._lobIsCrash &&
              _dmsData->_mbStatInfo[ collectionID
              ]._lobCommitFlag.compareAndSwap( 1, 0 ) )
         {
            needSync = TRUE ;
            _dmsData->_dmsMME->_mbList[ collectionID ]._lobCommitFlag = 0 ;
         }
      }
      else if ( -1 == collectionID )
      {
         for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
         {
            _dmsData->_mbStatInfo[ i ]._lobLastWriteTick = pmdGetDBTick() ;
            if ( DMS_IS_MB_INUSE ( _dmsData->_dmsMME->_mbList[i]._flag ) &&
                 !_dmsData->_mbStatInfo[ i ]._lobIsCrash &&
                 _dmsData->_mbStatInfo[ i
                 ]._lobCommitFlag.compareAndSwap( 1, 0 ) )
            {
               needSync = TRUE ;
               _dmsData->_dmsMME->_mbList[ i ]._lobCommitFlag = 0 ;
            }
         }
      }

      if ( needSync )
      {
         rc = _dmsData->flushMME( isSyncDeep() ) ;
      }
      return rc ;
   }

   UINT64 _dmsStorageLob::_getOldestWriteTick() const
   {
      UINT64 oldestWriteTick = ~0 ;
      UINT64 lastWriteTick = 0 ;

      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         lastWriteTick = _dmsData->_mbStatInfo[i]._lobLastWriteTick ;
         /// The collection is commit valid, should ignored
         if ( 0 == _dmsData->_mbStatInfo[i]._lobCommitFlag.peek() &&
              lastWriteTick < oldestWriteTick )
         {
            oldestWriteTick = lastWriteTick ;
         }
      }
      return oldestWriteTick ;
   }

   void _dmsStorageLob::_onRestore()
   {
      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         _dmsData->_mbStatInfo[i]._lobIsCrash = FALSE ;
      }
   }

   void _dmsStorageLob::_initHeaderPageSize( dmsStorageUnitHeader * pHeader,
                                             dmsStorageInfo * pInfo )
   {
      SDB_ASSERT( pInfo->_lobdPageSize > 0, "Not initialized lobd page size" ) ;
      /// assign values to lobm header
      pHeader->_pageSize     = DMS_PAGE_SIZE64B ;
      pHeader->_lobdPageSize = pInfo->_lobdPageSize ;
      // set to 0 again since v3.4.5/v3.6.1/v5.0.4,
      // for we hope the newly created lob can be
      // compatible with the old version
      pHeader->_segmentSize  = 0 ;
   }

   INT32 _dmsStorageLob::_checkPageSize( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;
      // the size of lobm for the matched lobm segment.
      // e.g. when lobd segment is 128M, lobd page is 256K,
      // metaSizeOfDataSeg is 32k
      UINT32 metaSizeOfDataSeg = 0 ;

      if ( pHeader->_pageSize != DMS_PAGE_SIZE64B &&
           pHeader->_pageSize != DMS_PAGE_SIZE256B )
      {
         PD_LOG( PDERROR, "Lob meta page size[%d] must be %d or %d in file[%s]",
                 pHeader->_pageSize,
                 DMS_PAGE_SIZE64B, DMS_PAGE_SIZE256B,
                 getSuFileName() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( DMS_PAGE_SIZE4K != pHeader->_lobdPageSize &&
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

      /// when lobm page size is 64B, lobm segment size is in range of
      /// [16K, 2M]; when lobm page size is 256B, lobm segment size is in range
      /// of [64K, 8M]
      if ( 0 != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ16K != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ32K != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ64K != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ128K != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ256K != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ512K != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ1M != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ2M != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ4M != pHeader->_segmentSize &&
           DMS_SEGMENT_SZ8M != pHeader->_segmentSize )
      {
         PD_LOG ( PDEVENT, "Invalid lobm segment size: %d in file[%s], lobm "
                  "segment size must be one of 0/16K/32K/64K/128K/256K/512K"
                  "1M/2M/4M/8M",
                  pHeader->_segmentSize, getSuFileName() ) ;
         /// we come here, we get an old version lob which lobm is not init
         /// because of a bug. Let's set its lobm segment size to 0, for it
         /// must be a lob created before v3.4.3/v5.0.2
         pHeader->_segmentSize = 0 ;
      }

      /// now, pHeader is a pointer got from lobm's file header,
      /// let's use it to calculate lobd _segmentSize and lobm
      /// _segmentSize
      if ( 0 == pHeader->_segmentSize )
      {
         /// we come here, we get a lob which lobd segment size is 128M
         _dataSegmentSize  = DMS_SEGMENT_SZ ;
         metaSizeOfDataSeg = DMS_SEGMENT_SZ / pHeader->_lobdPageSize *
                               pHeader->_pageSize ;
      }
      else
      {
         /// we come here, we get a lob which is created in
         /// v3.4.3/v3.4.4/v5.0.2/v5.0.3/v3.6
         /// only in these 5 versions, lobm segment size was writed into lobm
         /// file header
         /// the lob created since v3.4.5/v3.6.1/v5.0.4 will set
         /// pHeader->_segmentSize to 0 again
         _dataSegmentSize  = pHeader->_segmentSize / pHeader->_pageSize *
                             pHeader->_lobdPageSize ;
         metaSizeOfDataSeg = pHeader->_segmentSize ;
      }

      // since since v3.4.5/v3.6.1/v5.0.4, lobm segment size is aligned by 2MB,
      // so it will be 2MB at most case. It will be 4MB/8MB only when
      // logPageSize is 4k/8K and lobm page size is 256B
      _segmentSize = ossRoundUpToMultipleX( metaSizeOfDataSeg,
                                            DMS_SEGMENT_SZ2M ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsStorageLob::_checkFileSizeValidBySegment( const UINT64 fileSize,
                                                         UINT64 &rightSize )
   {
      if (  0 == ( fileSize - _dataOffset() ) % _getMetaSizeOfDataSegment() )
      {
         rightSize = fileSize ;
         return TRUE ;
      }
      else
      {
         rightSize = ( ( fileSize - _dataOffset() ) /
                       _getMetaSizeOfDataSegment() ) * _getMetaSizeOfDataSegment() +
                       _dataOffset() ;
         return FALSE ;
      }
   }

   BOOLEAN _dmsStorageLob::_checkFileSizeValid( const UINT64 fileSize,
                                                UINT64 &rightSize )
   {
      UINT64 rightSz = (UINT64)_dmsHeader->_storageUnitSize * pageSize() ;
      UINT64 alignSz = ossRoundUpToMultipleX( ( rightSz - _dataOffset() ),
                                              _getSegmentSize() ) +
                                              _dataOffset() ;
      if ( fileSize == alignSz )
      {
         rightSize = fileSize ;
         return TRUE ;
      }
      else
      {
         rightSize = alignSz ;
         return FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_CALCCOUNT, "_dmsStorageLob::_calcCount" )
   INT32 _dmsStorageLob::_calcCount()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_CALCCOUNT ) ;
      DMS_LOB_PAGEID current = 0 ;
      INT64 lobPieceLen = 0 ;
      dmsExtRW extRW ;

      /// clear all lob count
      for( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _dmsData->_mbStatInfo[i]._totalLobs.poke(0) ;
         _dmsData->_mbStatInfo[i]._totalLobSize.poke(0) ;
         _dmsData->_mbStatInfo[i]._totalValidLobSize.poke(0) ;
      }

      /// re-count
      while ( current < (INT32)pageNum() )
      {
         if ( DMS_LOB_PAGE_IN_USED( current ) )
         {
            const _dmsLobDataMapBlk *blk = NULL ;
            dmsMB *mb = NULL ;
            extRW = extent2RW( current, -1 ) ;
            extRW.setNothrow( TRUE ) ;
            blk = extRW.readPtr<_dmsLobDataMapBlk>() ;
            if ( !blk )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                       "pageid:%d", current ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( blk->_mbID >= DMS_MME_SLOTS || blk->_mbID < 0 ||
                 blk->isUndefined() )
            {
               ++current ;
               continue ;
            }
            mb = &(_dmsData->_dmsMME->_mbList[ blk->_mbID ] ) ;
            if ( mb->_logicalID != blk->_clLogicalID )
            {
               ++current ;
               continue ;
            }

            /// Stat lob info
            /// Traversing the lobd file to count _totalValidLobSize, the io
            /// overhead is very large. So, directly accumulate blk->_dataLen
            /// as _totalValidLobSize.
            if ( DMS_LOB_META_SEQUENCE != blk->_sequence )
            {
               _dmsData->_mbStatInfo[blk->_mbID]._totalLobSize.add(blk->_dataLen) ;
               _dmsData->_mbStatInfo[blk->_mbID]._totalValidLobSize.add(blk->_dataLen) ;
            }
            else
            {
               /// dmsLobMate size take the value: 1k.
               lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( blk->_dataLen ) ;
               _dmsData->_mbStatInfo[blk->_mbID]._totalLobs.add(1) ;
               _dmsData->_mbStatInfo[blk->_mbID]._totalLobSize.add(lobPieceLen) ;
               _dmsData->_mbStatInfo[blk->_mbID]._totalValidLobSize.add(lobPieceLen) ;
            }
         }
         ++current ;
      }
      /// flush MME
      _dmsData->flushMME( TRUE ) ;

      /// update the header
      _dmsHeader->_version = DMS_LOB_CUR_VERSION ;
      flushHeader( TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_CALCCOUNT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageLob::rebuildBME()
   {
      INT32 rc = SDB_OK ;
      DMS_LOB_PAGEID current = 0 ;
      dmsExtRW extRW ;
      UINT32 totalReleased = 0 ;
      UINT32 totalPushed = 0 ;
      UINT32 totalLobs = 0 ;
      UINT64 lobPieceLen = 0 ;
      UINT64 totalLobSize = 0 ;
      UINT64 totalValidLobSize = 0 ;
      UINT32 __hash = 0 ;
      UINT32 testBucketNo = 0 ;

      /// reset bme
      ossMemset( (void*)_dmsBME, 0xFF, sizeof(dmsBucketsManagementExtent) ) ;

      /// clear all lob count
      for( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _dmsData->_mbStatInfo[i]._totalLobs.poke(0) ;
         _dmsData->_mbStatInfo[i]._totalLobPages.poke(0) ;
         _dmsData->_mbStatInfo[i]._totalLobSize.poke(0) ;
         _dmsData->_mbStatInfo[i]._totalValidLobSize.poke(0) ;
      }

      /// rebuild
      while ( current < (INT32)pageNum() )
      {
         if ( DMS_LOB_PAGE_IN_USED( current ) )
         {
            _dmsLobDataMapBlk *blk = NULL ;
            dmsMB *mb = NULL ;
            extRW = extent2RW( current, -1 ) ;
            extRW.setNothrow( TRUE ) ;
            blk = extRW.writePtr<_dmsLobDataMapBlk>() ;
            if ( !blk )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                       "pageid:%d", current ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( blk->_mbID >= DMS_MME_SLOTS || blk->_mbID < 0 ||
                 blk->isUndefined() )
            {
               _releaseSpace( current, 1 ) ;
               ++totalReleased ;
               ++current ;
               continue ;
            }
            mb = &(_dmsData->_dmsMME->_mbList[ blk->_mbID ] ) ;
            if ( mb->_logicalID != blk->_clLogicalID )
            {
               _releaseSpace( current, 1 ) ;
               ++totalReleased ;
               ++current ;
               continue ;
            }
            else
            {
               dmsLobRecord record ;
               record.set( ( const bson::OID* )blk->_oid, blk->_sequence, 0,
                           blk->_dataLen, NULL ) ;
               /// add page to bucket
               DMS_LOB_GET_HASH_FROM_BLK( blk, __hash ) ;
               testBucketNo = _getBucket( __hash ) ;
               rc = _push2Bucket( testBucketNo, current, NULL, *blk, &record ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Push page[%d] to bucket failed, rc: %d",
                          current, rc ) ;
                  goto error ;
               }
               ++totalPushed ;
               /// stat lob info
               /// Traversing the lobd file to count _totalValidLobSize, the io
               /// overhead is very large. So, directly accumulate blk->_dataLen
               /// as _totalValidLobSize.
               _dmsData->_mbStatInfo[blk->_mbID]._totalLobPages.add(1) ;
               if ( DMS_LOB_META_SEQUENCE == blk->_sequence )
               {
                  ++totalLobs ;
                  /// dmsLobMate size take the value: 1k.
                  lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( blk->_dataLen ) ;
                  totalLobSize += lobPieceLen ;
                  totalValidLobSize += lobPieceLen ;
                  _dmsData->_mbStatInfo[blk->_mbID]._totalLobs.add(1) ;
                  _dmsData->_mbStatInfo[blk->_mbID]._totalLobSize.add(lobPieceLen) ;
                  _dmsData->_mbStatInfo[blk->_mbID]._totalValidLobSize.add(lobPieceLen) ;
               }
               else
               {
                  /// dmsLobMate size take the value: 1k.
                  totalLobSize += blk->_dataLen ;
                  totalValidLobSize += blk->_dataLen ;
                  _dmsData->_mbStatInfo[blk->_mbID]._totalLobSize.add(blk->_dataLen) ;
                  _dmsData->_mbStatInfo[blk->_mbID]._totalValidLobSize.add(blk->_dataLen) ;
               }
            }
         }
         ++current ;
      }

      /// update the header
      if ( _dmsHeader->_version <= DMS_LOB_VERSION_1 )
      {
         _dmsHeader->_version = DMS_LOB_CUR_VERSION ;
      }
      flushMeta( TRUE ) ;

      PD_LOG( PDEVENT, "Rebuild bme of file[%s] succeed[ReleasedPage:%u, "
              "PushedPage:%u, TotalLobs:%u, TotalLobSzie:%llu, "
              "TotalValidLobSize:%llu]", getSuFileName(), totalReleased,
              totalPushed, totalLobs, totalLobSize, totalValidLobSize ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_READPAGE, "_dmsStorageLob::readPage" )
   INT32 _dmsStorageLob::readPage( DMS_LOB_PAGEID &pos,
                                   BOOLEAN onlyMetaPage,
                                   _pmdEDUCB *cb,
                                   dmsMBContext *mbContext,
                                   dmsLobInfoOnPage &page )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_READPAGE ) ;
      DMS_LOB_PAGEID current = pos ;
      BOOLEAN locked = FALSE ;

      if ( DMS_LOB_INVALID_PAGEID == current )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in read, rc: %d", rc ) ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get lock:%d", rc ) ;
            goto error ;
         }
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in read", getSuName() ) ;
         goto error ;
      }

      do
      {
         if ( pageNum() <= ( UINT32 )current )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
         else if ( DMS_LOB_PAGE_IN_USED( current ) )
         {
            dmsExtRW extRW ;
            const _dmsLobDataMapBlk *blk = NULL ;
            extRW = extent2RW( current, -1 ) ;
            extRW.setNothrow( TRUE ) ;
            blk = extRW.readPtr<_dmsLobDataMapBlk>() ;
            if ( !blk )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                       "pageid:%d", current ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            /// first check undefined
            if ( blk->isUndefined() )
            {
               ++current ;
               continue ;
            }
            /// then check clLID
            else if ( mbContext->clLID() != blk->_clLogicalID ||
                      ( onlyMetaPage &&
                        DMS_LOB_META_SEQUENCE != blk->_sequence ) )
            {
               ++current ;
               continue ;
            }

            ossMemcpy( &( page._oid ), blk->_oid, sizeof( page._oid ) ) ;
            page._sequence = blk->_sequence ;
            page._len = blk->_dataLen ;
            ++current ;
            break ;
         }
         else
         {
            /// not allocated.
            ++current ;
         }
      } while ( TRUE ) ;

      /// point to the next.
      pos = current ;
   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_READPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_TRUNCATE, "_dmsStorageLob::truncate" )
   INT32 _dmsStorageLob::truncate( dmsMBContext *mbContext,
                                   _pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_TRUNCATE ) ;

      BOOLEAN locked = FALSE ;
      BOOLEAN needPanic = FALSE ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in remove", getSuName() ) ;
         goto error ;
      }

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( NULL != dpscb )
      {
         rc = dpsLobTruncate2Record( fullName, logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build dps log:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to reserved log space(length=%u)",
                    logRecord.head()._length ) ;
            info.clear() ;
            goto error ;
         }
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      needPanic = TRUE ;
      {
        std::shared_ptr<ILob> lobPtr;
        rc = mbContext->getCollPtr()->getLobPtr(lobPtr);
        PD_RC_CHECK(rc, PDERROR, "Failed to get lob storage ptr, rc: %d", rc);
        rc = lobPtr->truncate(cb);
        PD_RC_CHECK(rc, PDERROR, "Failed to truncate all lob pieces, rc: %d", rc);
      }

      // clear the stat info
      mbContext->mbStat()->_totalLobPages.poke(0) ;
      mbContext->mbStat()->_totalLobs.poke(0) ;
      mbContext->mbStat()->resetTotalLobSize() ;
      mbContext->mbStat()->resetTotalValidLobSize() ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(),
                         mbContext->clLID(),
                         OID(), DMS_LOB_META_SEQUENCE, cb ) ;

         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to prepare dps log:%d", rc ) ;
            goto error ;
         }
         mbContext->mbStat()->updateLastLSN(
            info.getMergeBlock().record().head()._lsn,
            DMS_FILE_LOB ) ;

         if ( locked )
         {
            mbContext->mbUnlock() ;
            locked = FALSE ;
         }
         dpscb->writeData( info ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         mbContext->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_LOB ) ;
         cb->setDataExInfo( fullName, _dmsData->logicalID(),
                            mbContext->clLID(), OID(), DMS_LOB_META_SEQUENCE ) ;
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_TRUNCATE, rc ) ;
      return rc ;
   error:
      if ( needPanic )
      {
         PD_LOG( PDSEVERE, "we must panic db now, we got a irreparable "
                 "error" ) ;
         ossPanic() ;
      }
      goto done ;
   }

   void _dmsStorageLob::_calcExtendInfo( const UINT64 fileSize,
                                         UINT32 &numSeg,
                                         UINT64 &incFileSize,
                                         UINT32 &incPageNum )
   {
      UINT32 totalSegNum    = 0 ;
      UINT32 newTotalSegNum = 0 ;
      UINT32 segmentPages   = this->segmentPages() ;
      UINT64 rightSz   = (UINT64)_dmsHeader->_storageUnitSize * pageSize() ;
      UINT64 newFileSz = ossRoundUpToMultipleX(
            ( rightSz - _dataOffset() + _getMetaSizeOfDataSegment() * numSeg ),
            _getSegmentSize() ) + _dataOffset() ;

      incFileSize = ( newFileSz > fileSize ) ? ( newFileSz - fileSize ) : 0 ;
      incPageNum  = _getDataSegmentPages() * numSeg ;
      // currently, numSeg is the number of increasing lobd segments,
      // for one lobm segment can hold many lobd segments,
      // so we need to change numSeg to be the actual number
      // of increasing lobm segments
      totalSegNum = _dmsHeader->_pageNum / segmentPages ;
      if ( 0 != ( _dmsHeader->_pageNum % segmentPages ) )
      {
         totalSegNum += 1 ;
      }
      newTotalSegNum = ( _dmsHeader->_pageNum + incPageNum ) / segmentPages ;
      if ( 0 != ( ( _dmsHeader->_pageNum + incPageNum ) % segmentPages ) )
      {
         newTotalSegNum += 1 ;
      }
      numSeg = newTotalSegNum - totalSegNum ;
   }

   void _dmsStorageLob::_statVaildLobSize( dmsMBContext *mbContext,
                                           const dmsLobMeta *metaNew,
                                           const dmsLobMeta *metaOld )
   {
      INT64 newLen = 0 ;
      INT64 oldLen = 0 ;
      INT64 incLen = 0 ;

      if ( NULL != metaNew )
      {
         newLen = metaNew->_lobLen ;
      }
      if ( NULL != metaOld )
      {
         oldLen = metaOld->_lobLen ;
      }

      incLen = newLen - oldLen ;
      if ( 0 != incLen )
      {
         mbContext->mbStat()->addTotalValidLobSize( incLen ) ;
      }
   }
}
