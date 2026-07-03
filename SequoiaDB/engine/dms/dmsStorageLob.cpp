/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
#include "utilBitmap.hpp"

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


   /*
      _dmsStorageLob implement
   */
   _dmsStorageLob::_dmsStorageLob( const CHAR *lobmFileName,
                                   const CHAR *lobdFileName,
                                   dmsStorageInfo *info,
                                   dmsStorageDataCommon *pDataSu,
                                   utilCacheUnit *pCacheUnit )
   :_dmsStorageBase( lobmFileName, info ),
    _dmsBME( NULL ),
    _dmsData( (dmsStorageData *)pDataSu ),      // TODO: temporary cast
    _data( lobdFileName, info->_enableSparse, info->_directIO ),
    _path( NULL ),
    _metaPath( NULL ),
    _delayOpenLatch( MON_LATCH_DMSSTORAGELOB_DELAYOPENLATCH ),
    _pCacheUnit( pCacheUnit ),
    _pSyncMgrTmp( NULL ),
    _pStatMgrTmp( NULL ),
    _vecBucketLacth( NULL )
   {
      _needDelayOpen = FALSE ;

      _dmsData->_attachLob( this ) ;
      _isRename = FALSE ;
      _dataSegmentSize = 0 ;
      /// default to current version, refreshed from header in _onOpened
      _hashVersion = DMS_LOB_CUR_VERSION ;
   }

   _dmsStorageLob::~_dmsStorageLob()
   {
      _dmsData->_detachLob() ;
      _dmsData = NULL ;
      _dmsBME = NULL ;

      if ( _vecBucketLacth )
      {
         SDB_OSS_DEL [] _vecBucketLacth ;
         _vecBucketLacth = NULL ;
      }

      _releasePath() ;
   }

   void _dmsStorageLob::_releasePath()
   {
      if ( _metaPath && _metaPath != _path )
      {
         SDB_OSS_FREE( _metaPath ) ;
      }
      if ( _path )
      {
         SDB_OSS_FREE( _path ) ;
      }

      _metaPath = NULL ;
      _path = NULL ;
   }

   UINT32 _dmsStorageLob::getBucketID( const _dmsLobDataMapBlk &blk,
                                       UTIL_LOB_HASH_TYPE hashType )
   {
      UINT32 hashCode = utilLobHash( blk._oid, blk._sequence, hashType ) ;
      return _getBucket( hashCode ) ;
   }

   void _dmsStorageLob::syncMemToMmap ( BOOLEAN *pHasWritten )
   {
      if ( _dmsData && _data.isOpened() )
      {
         _dmsData->syncMemToMmap( pHasWritten ) ;
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
      _releasePath() ;

      _path = ossStrdup( path ) ;
      if ( !_path )
      {
         PD_LOG( PDERROR, "Allocate lob path failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      if ( 0 == ossStrcmp( path, metaPath ) )
      {
         /// the same
         _metaPath = _path ;
      }
      else
      {
         _metaPath = ossStrdup( metaPath ) ;
         if ( !_metaPath )
         {
            PD_LOG( PDERROR, "Allocate lob meta path failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

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
         dmsMBStatInfo *pMBStat = NULL ;
         UINT16 i = _dmsData->_nextUsedMBSlot( 0 ) ;
         while ( DMS_INVALID_MBID != i )
         {
            pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
            if ( pMBStat->_totalLobs > 0 &&
                 ( pMBStat->_totalLobSize <= 0 ||
                   pMBStat->_totalValidLobSize <= 0 ) )
            {
               needCalcCount = TRUE ;
               break ;
            }

            i = _dmsData->_nextUsedMBSlot( i + 1 ) ;
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

      /// create bucket latch
      _vecBucketLacth = SDB_OSS_NEW ossSpinSLatch[ DMS_BUCKETS_LATCH_SIZE ] ;
      if ( !_vecBucketLacth )
      {
         PD_LOG( PDERROR, "Create bucket latch failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      /// Init cache unit
      rc = _pCacheUnit->init( getLobData(), _pStorageInfo->_lobdPageSize,
                              _pStorageInfo->_pageAllocTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init cache unit failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( _pStorageInfo->_cacheMergeSize > 0 )
      {
         rc = _pCacheUnit->enableMerge( _pStorageInfo->_directIO,
                                        _pStorageInfo->_cacheMergeSize ) ;
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
                       getHeader()->_pageNum, *_pStorageInfo,
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
                 sizeof( meta ), NULL, hashType() ) ;
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
                 sizeof( meta ), ( const CHAR * )( &meta ), hashType() ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_WRITEWITHPAGE, "_dmsStorageLob::_writeWithPage" )
   INT32 _dmsStorageLob::_writeWithPage( const dmsLobRecord &record,
                                         DMS_LOB_PAGEID &pageID,
                                         const CHAR *pFullName,
                                         dmsMBContext *mbContext,
                                         BOOLEAN canUnLock,
                                         _pmdEDUCB *cb,
                                         dpsMergeInfo &info,
                                         SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_WRITEWITHPAGE ) ;
      SDB_ASSERT( DMS_LOB_INVALID_PAGEID != pageID, "Page can't be invalid" ) ;
      SDB_ASSERT( pFullName, "FullName can't be NULL" ) ;

      BOOLEAN pageFilled = FALSE ;
      utilCacheContext cContext ;

      if ( DMS_LOB_INVALID_PAGEID == pageID )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid pageID" ) ;
         goto error ;
      }
      if ( !mbContext->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold lock with EXCLUSIVE" ) ;
         goto error ;
      }
      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in write", getSuName() ) ;
         goto error ;
      }
      else if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Write record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mbStat()->_flag,
                                           DMS_ACCESS_TYPE_INSERT ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mbStat()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      _pCacheUnit->prepareWrite( pageID, record._offset,
                                 record._dataLen, cb,
                                 cContext ) ;
      rc = cContext.write( record._data, record._offset,
                           record._dataLen, cb,
                           UTIL_WRITE_NEWEST_BOTH ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write data to collection:%s, rc:%d",
                 pFullName, rc ) ;
         goto error ;
      }

      rc = _fillPage( record, pageID, cb, mbContext ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to fill page, rc:%d", rc ) ;
         goto error ;
      }
      pageFilled = TRUE ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(),
                         pageID, cb ) ;
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
                            mbContext->clLID(), pageID ) ;
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
      /// submit the data
      cContext.submit( cb ) ;
      /// when write, set the page is newest( is the first write )
      cContext.makeNewest() ;

      pageID = DMS_LOB_INVALID_PAGEID ;

      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITEWITHPAGE, rc ) ;
      return rc ;
   error:
      /// rollback the data
      cContext.release() ;
      /// rollback the page
      if ( DMS_LOB_INVALID_PAGEID != pageID )
      {
         PD_LOG( PDEVENT, "Rollback lob piece[%s]",
                 record.toString().c_str(), pageID ) ;
         _rollback( record, pageID, cb, mbContext, pageFilled ) ;
      }
      goto done ;
   }

   INT32 _dmsStorageLob::write( const dmsLobRecord &record,
                                dmsMBContext *mbContext,
                                pmdEDUCB *cb,
                                SDB_DPSCB *dpscb )
   {
      return _writeInner( record, mbContext, cb, dpscb, FALSE, NULL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_UPDATEWITHPAGE, "_dmsStorageLob::_updateWithPage" )
   INT32 _dmsStorageLob::_updateWithPage( const dmsLobRecord &record,
                                          DMS_LOB_PAGEID pageID,
                                          const CHAR *pFullName,
                                          dmsMBContext *mbContext,
                                          BOOLEAN canUnLock,
                                          _pmdEDUCB *cb,
                                          SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_UPDATEWITHPAGE ) ;
      SDB_ASSERT( DMS_LOB_INVALID_PAGEID != pageID, "Page can't be invalid" ) ;
      SDB_ASSERT( pFullName, "FullName can't be NULL" ) ;

      dmsExtRW extRW ;
      _dmsLobDataMapBlk *blk = NULL ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      CHAR *oldData = NULL ;
      UINT32 oldLen = 0 ;
      utilCacheContext cContext ;
      UINT32 newestMask = 0 ;
      UINT32 orgBlkLen = 0 ;
      UINT32 pageIncSize = 0 ;
      UINT32 pageSize = _data.pageSize() ;

      if ( DMS_LOB_INVALID_PAGEID == pageID )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "PageID is invalid" ) ;
         goto error ;
      }
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

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mbStat()->_flag,
                                           DMS_ACCESS_TYPE_INSERT ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mbStat()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      /// prepare write
      {
         UINT32 newDataLen = 0 ;
         extRW = extent2RW( pageID, mbContext->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         blk = extRW.writePtr<_dmsLobDataMapBlk>() ;
         if ( !blk )
         {
            PD_LOG( PDERROR, "Get extent[%d] address failed", pageID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         orgBlkLen = blk->_dataLen ;
         newDataLen = record._dataLen + record._offset ;
         if ( newDataLen > 0 && orgBlkLen > newDataLen )
         {
            newDataLen = orgBlkLen ;
         }
         if ( newDataLen > 0 )
         {
            if ( DMS_LOB_META_SEQUENCE == record._sequence &&
                 orgBlkLen < DMS_LOB_META_LENGTH )
            {
               if ( newDataLen > DMS_LOB_META_LENGTH )
               {
                  pageIncSize = newDataLen - DMS_LOB_META_LENGTH ;
               }
            }
            else
            {
               pageIncSize = newDataLen - orgBlkLen ;
            }
         }
         _pCacheUnit->prepareWrite( pageID, 0, newDataLen, cb, cContext ) ;
      }

      /// read old data when we need dps or update dmsLobMeta
      if ( NULL != dpscb || DMS_IS_LOBMETA_RECORD( record ) )
      {
         UINT32 readOffset = 0 ;
         UINT32 readLen = 0 ;

         if ( record._offset >= orgBlkLen )
         {
            /// do nothing
         }
         else if ( record._offset + record._dataLen > orgBlkLen )
         {
            readOffset = record._offset ;
            readLen = orgBlkLen - record._offset ;
         }
         else
         {
            readOffset = record._offset ;
            readLen = record._dataLen ;
         }

         /// alloc memory
         rc = cb->allocBuff( readLen > 0 ? readLen : 1, &oldData, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc read buffer[%u] failed, rc: %d",
                    readLen, rc ) ;
            goto error ;
         }
         rc = cContext.readAndCache( oldData, readOffset, readLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to read data from file, rc:%d", rc ) ;
            goto error ;
         }
         /// need to unlock the mbContext, so the sync control is not hold
         /// the mbContext
         if ( canUnLock )
         {
            mbContext->mbUnlock() ;
         }

         oldLen = cContext.submit( cb ) ;
         SDB_ASSERT( oldLen == readLen, "impossible" ) ;

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
                                 pageID,
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

      if ( record._dataLen + record._offset > orgBlkLen )
      {
         newestMask |= UTIL_WRITE_NEWEST_TAIL ;

         if ( 0 == record._offset )
         {
            newestMask |= UTIL_WRITE_NEWEST_HEADER ;
         }
      }

      rc = cContext.write( record._data, record._offset,
                           record._dataLen, cb, newestMask ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write data to collection:%s, rc:%d",
                 pFullName, rc ) ;
         goto error ;
      }

      if ( record._dataLen + record._offset > orgBlkLen )
      {
         blk->_dataLen = record._dataLen + record._offset ;
      }

      if ( blk->isNew() )
      {
         blk->setOld() ;
      }

      /// Maintain the page crc. Only a full overwrite ( from offset 0 that
      /// covers the whole valid content ) can be checksummed without an
      /// extra read; any other partial update changes the page content, so
      /// the crc must be cleared to avoid a stale value ( see design ).
      if ( pmdGetOptionCB()->lobChecksumWriteOn() &&
           0 == record._offset && record._dataLen >= orgBlkLen &&
           NULL != record._data && record._dataLen > 0 )
      {
         blk->setCrc( utilCrc32c( record._data, record._dataLen ) ) ;
      }
      else
      {
         blk->clearCrc() ;
      }

      mbContext->mbStat()->addTotalLobSize( pageIncSize ) ;

      _incWriteRecord() ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(),
                         pageID, cb ) ;
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
                            mbContext->clLID(), pageID ) ;
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
      /// submit the data
      cContext.submit( cb ) ;
      /// make the page newest
      cContext.makeNewest( newestMask ) ;

      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }
      if ( oldData )
      {
         cb->releaseBuff( oldData ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_UPDATEWITHPAGE, rc ) ;
      return rc ;
   error:
      /// rollback the data
      cContext.release() ;
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

      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in update, rc: %d", rc ) ;
      }

      /// make full name
      _clFullName( mbContext->mbStat()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      rc = _find( record, mbContext->clLID(), cb, page ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find piece[%s], rc:%d",
                 record.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         PD_LOG( PDERROR, "Can not find piece[%s]",
                 record.toString().c_str() ) ;
         rc = SDB_LOB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

      rc = _updateWithPage( record, page, fullName, mbContext,
                            locked, cb, dpscb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update record[%s] to page[%d] in collection[%s] "
                 "failed, rc: %d", record.toString().c_str(), page,
                 fullName, rc ) ;
         goto error ;
      }

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

      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      DMS_LOB_PAGEID foundPage = DMS_LOB_INVALID_PAGEID ;
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
      _clFullName( mbContext->mbStat()->_collectionName, fullName,
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
                              page,
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

      rc = _allocatePage( record, mbContext, page ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to allocate page in collection:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

#if defined (_DEBUG)
      SDB_ASSERT( DMS_LOB_PAGE_IN_USED( page ), "must be used" ) ;
#endif

      /// When using update
      if ( updateWhenExist )
      {
         rc = _find( record, mbContext->clLID(), cb, foundPage ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find piece[%s], rc:%d",
                    record.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      /// write
      if ( DMS_LOB_INVALID_PAGEID == foundPage )
      {
         rc = _writeWithPage( record, page, fullName, mbContext,
                              locked, cb, info, dpscb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write record[%s] to collection[%s] failed, "
                    "rc: %d", record.toString().c_str(), fullName, rc ) ;
            goto error ;
         }
         if ( pHasUpdated )
         {
            *pHasUpdated = FALSE ;
         }
      }
      /// update
      else
      {
         /// release page
         _releasePage( page, mbContext ) ;
         page = DMS_LOB_INVALID_PAGEID ;
         /// relase log space
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
         info.clear() ;

         rc = _updateWithPage( record, foundPage, fullName, mbContext,
                               locked, cb, dpscb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update record[%s] to page[%d] in "
                    "collection[%s] failed, rc: %d",
                    record.toString().c_str(), foundPage,
                    fullName, rc ) ;
            goto error ;
         }
         if ( pHasUpdated )
         {
            *pHasUpdated = TRUE ;
         }
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
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITEINNER, rc ) ;
      return rc ;
   error:
      if ( DMS_LOB_INVALID_PAGEID != page )
      {
         _releasePage( page, mbContext ) ;
      }
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
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      BOOLEAN locked = FALSE ;
      utilCacheContext cContext ;
      /// crc verify is done after submit ( data filled in buf ), so capture
      /// the expected crc under mb lock here
      BOOLEAN verifyCrc = FALSE ;
      UINT32  expectCrc = 0 ;

      if ( _needDelayOpen )
      {
         ossScopedLock lock( &_delayOpenLatch ) ;

         /// when not opened, return error, and not delay open
         if ( _needDelayOpen )
         {
            rc = SDB_LOB_SEQUENCE_NOT_EXIST ;
            goto error ;
         }
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

      rc = _find( record, mbContext->clLID(), cb, page ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find page of record[%s], rc:%d",
                 record.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

#if defined (_DEBUG)
      SDB_ASSERT( DMS_LOB_PAGE_IN_USED( page ), "must be used" ) ;
#endif
      /// A whole page read ( offset 0, len == page data len ) with a valid
      /// page crc can be verified. Capture the expected crc under lock.
      if ( pmdGetOptionCB()->lobChecksumReadOn() && 0 == record._offset )
      {
         dmsExtRW extRW = extent2RW( page, mbContext->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         const _dmsLobDataMapBlk *blk = extRW.readPtr<_dmsLobDataMapBlk>() ;
         if ( blk && blk->isCrcValid() &&
              record._dataLen == blk->_dataLen )
         {
            verifyCrc = TRUE ;
            expectCrc = blk->_crc ;
         }
      }

      _pCacheUnit->prepareRead( page, record._offset, record._dataLen,
                                cb, cContext ) ;
      rc = cContext.read( buf, record._offset, record._dataLen, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to read data from file, rc:%d", rc ) ;
         goto error ;
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      /// submit the read data
      readLen = cContext.submit( cb ) ;
      /// verify page crc after data is filled into buf
      if ( SDB_OK == rc && verifyCrc && readLen == record._dataLen )
      {
         UINT32 actualCrc = utilCrc32c( buf, readLen ) ;
         if ( actualCrc != expectCrc )
         {
            PD_LOG( PDERROR, "Lob data page crc mismatch, piece[%s], page[%d], "
                    "len[%u], expect[0x%08x], actual[0x%08x]",
                    record.toString().c_str(), page, readLen,
                    expectCrc, actualCrc ) ;
            rc = SDB_DMS_CORRUPTED_EXTENT ;
         }
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_READ, rc ) ;
      return rc ;
   error:
      /// rollback the read data
      cContext.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ALLOCATEPAGE, "_dmsStorageLob::_allocatePage" )
   INT32 _dmsStorageLob::_allocatePage( const dmsLobRecord &record,
                                        dmsMBContext *context,
                                        DMS_LOB_PAGEID &page )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ALLOCATEPAGE ) ;
      SDB_ASSERT( NULL != record._oid && 0 <= record._sequence &&
                  record._dataLen + record._offset <= getLobdPageSize(),
                  "invalid lob record" ) ;

      rc = _findFreeSpace( 1, page, context ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find free space, rc:%d", rc ) ;
         goto error ;
      }

      /// add lob page
      context->mbStat()->_totalLobPages += 1 ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ALLOCATEPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__FILLPAGE, "_dmsStorageLob::_fillPage" )
   INT32 _dmsStorageLob::_fillPage( const dmsLobRecord &record,
                                    DMS_LOB_PAGEID page,
                                    pmdEDUCB *cb,
                                    dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__FILLPAGE ) ;
      _dmsLobDataMapBlk *blk = NULL ;
      INT64 lobPieceLen      = 0 ;
      dmsExtRW extRW ;

      extRW = extent2RW( page, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      blk = extRW.writePtr<_dmsLobDataMapBlk>() ;
      if ( !blk )
      {
         PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                 page ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// must first set clLogiclID
      blk->_clLogicalID = context->clLID() ;
      blk->_mbID = context->mbID() ;
      blk->_newFlag = DMS_LOB_PAGE_FLAG_NEW ;

      ossMemset( blk->_pad1, 0, sizeof( blk->_pad1 ) ) ;
      ossMemset( blk->_pad2, 0, sizeof( blk->_pad2 ) ) ;
      ossMemcpy( blk->_oid, record._oid, DMS_LOB_OID_LEN ) ;
      blk->_sequence = record._sequence ;
      blk->_dataLen = record._dataLen + record._offset ;
      blk->_prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
      blk->_nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
      blk->setRemoved() ;
      /// crc is (re)generated below when checksum is enabled, clear it
      /// first so a reused page never carries a stale crc
      blk->clearCrc() ;

      /// A new page written from offset 0 holds the whole valid content
      /// [0, _dataLen) in record._data, so the crc can be computed without
      /// any extra read ( see design: only checksum fully written pages ).
      if ( pmdGetOptionCB()->lobChecksumWriteOn() &&
           0 == record._offset && NULL != record._data &&
           record._dataLen > 0 )
      {
         blk->setCrc( utilCrc32c( record._data, record._dataLen ) ) ;
      }

#if defined (_DEBUG)
      {
         UINT32 __hash = _calcHash( *blk ) ;
         if ( __hash != record._hash )
         {
            dmsLobDataMapBlk memBlk ;
            ossMemcpy( &memBlk, blk, sizeof( memBlk ) ) ;
            SDB_ASSERT( __hash == record._hash, "must be same" ) ;
         }
      }
#endif

      rc = _push2Bucket( _getBucket( record._hash ),
                         page, cb, *blk, &record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push page[%d] to bucket[%d], rc: %d",
                 page, _getBucket( record._hash ), rc ) ;
         goto error ;
      }

      /// add stat
      if ( DMS_IS_LOBMETA_RECORD( record ) )
      {
         context->mbStat()->_totalLobs++ ;
         lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( blk->_dataLen ) ;
         context->mbStat()->addTotalLobSize( lobPieceLen ) ;
         _statVaildLobSize( context, ( _dmsLobMeta* )record._data, NULL ) ;
      }
      else
      {
         context->mbStat()->addTotalLobSize( blk->_dataLen ) ;
      }

      _incWriteRecord() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__FILLPAGE, rc ) ;
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
      UINT32 bucketNumber = 0 ;
      dmsExtRW extRW ;
      _dmsLobDataMapBlk *blk = NULL ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      UINT32 resevedLength = 0 ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      CHAR *oldData = NULL ;
      UINT32 oldLen = 0 ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;
      BOOLEAN hasRemoved = FALSE ;
      utilCacheContext cContext ;
      UINT32 dirtyStart = 0 ;
      UINT32 dirtyLen = 0 ;
      UINT64 beginLSN = 0 ;
      UINT64 endLSN = 0 ;
      UINT32 pageSize = 0 ;
      ossSpinSLatch *pLatch = NULL ;
      dmsLobRecord oldRecord ;
      UINT32 readLen = 0 ;
      BOOLEAN needSubmit = FALSE, isMetaPage = FALSE ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in remove, rc: %d", rc ) ;
      }

      // get page size
      pageSize = _data.pageSize() ;

      /// make full name
      _clFullName( mbContext->mbStat()->_collectionName, fullName,
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
                               oldData,
                               pageSize,
                               page,
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

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mbStat()->_flag,
                                           DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mbStat()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      rc = _find( record, mbContext->clLID(), cb, page, &bucketNumber ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find record[%s], rc:%d",
                 record.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         goto done ;
      }

      extRW = extent2RW( page, mbContext->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      blk = extRW.writePtr<_dmsLobDataMapBlk>() ;
      if ( !blk )
      {
         PD_LOG( PDERROR, "Get extent[%d] address failed", page ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( onlyRemoveNewPage )
      {
         if ( !blk->isNew() )
         {
            goto done ;
         }
      }

      /// When dpscb is NULL or not page 0, not to alloc the page when page is
      /// not in cache( use len = 0 )
      isMetaPage = DMS_LOB_META_SEQUENCE == blk->_sequence ;
      oldLen = blk->_dataLen ;
      if ( dpscb )
      {
         // for DPS, we need whole page to write DPS log
         readLen = oldLen ;
      }
      else if ( isMetaPage )
      {
         // for meta page, we need meta data to calculate valid size
         // if old data is passed, we can use the old data to calculate
         // otherwise, read from file
         if ( NULL == pOldData )
         {
            readLen = DMS_LOB_META_LENGTH ;
         }
      }

      _pCacheUnit->prepareWrite( page, 0, readLen, cb, cContext ) ;
      if ( readLen > 0 )
      {
         rc = cb->allocBuff( readLen, &oldData, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc read buffer[%u] failed, rc: %d",
                    readLen, rc ) ;
            goto error ;
         }
         rc = cContext.read( oldData, 0, readLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to read data from file, rc:%d", rc ) ;
            goto error ;
         }

         needSubmit = TRUE ;
      }
      else if ( isMetaPage && NULL != pOldData )
      {
         // use the passed old data
         oldRecord._data = pOldData ;
      }

      /// lock bucket
      if ( dpscb )
      {
         pLatch = _getBucketLatch( bucketNumber ) ;
         pLatch->get() ;
      }

      /// remove and release the page
      rc = _removePage( page, blk, &bucketNumber, cb, mbContext,
                        pLatch ? TRUE : FALSE, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to remove page:%d, rc:%d", page, rc ) ;
         goto error ;
      }
      hasRemoved = TRUE ;

      /// release the mbContext
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }

      /// submit the read data
      if ( needSubmit )
      {
         /// submit the read data
         UINT32 submitLen = cContext.submit( cb ) ;
         SDB_ASSERT( submitLen == readLen, "impossible" ) ;
         oldRecord._data = oldData ;
         needSubmit = FALSE ;
      }

      if ( dpscb )
      {
         rc = dpsLobRm2Record( fullName,
                               record._oid,
                               record._sequence,
                               record._offset,
                               record._hash,
                               oldLen,
                               oldData,
                               pageSize,
                               page,
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
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(), page, cb ) ;
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
                            mbContext->clLID(), page ) ;
      }

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

      /// discard the page
      cContext.discardPage( dirtyStart, dirtyLen, beginLSN, endLSN ) ;
      /// release the context and then lock mbContext again
      cContext.release() ;

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
      if ( oldData )
      {
         cb->releaseBuff( oldData ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_REMOVE, rc ) ;
      return rc ;
   error:
      if ( hasRemoved )
      {
         ossPanic() ;
      }
      goto done ;
   }

   INT32 _dmsStorageLob::_releasePage( DMS_LOB_PAGEID page,
                                       dmsMBContext *context )
   {
      context->mbStat()->_totalLobPages -= 1 ;
      return _releaseSpace( page, 1 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__FIND, "_dmsStorageLob::_find" )
   INT32 _dmsStorageLob::_find( const _dmsLobRecord &record,
                                UINT32 clID,
                                pmdEDUCB *cb,
                                DMS_LOB_PAGEID &page,
                                UINT32 *bucket )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__FIND ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      UINT32 bucketNumber = _getBucket( record._hash ) ;
      DMS_LOB_PAGEID pageInBucket = DMS_LOB_INVALID_PAGEID ;
      dmsExtRW extRW ;
      const _dmsLobDataMapBlk *blk = NULL ;

      ossScopedLock lock( _getBucketLatch( bucketNumber ), SHARED ) ;

      pageInBucket = _dmsBME->_buckets[bucketNumber] ;
      while ( DMS_LOB_INVALID_PAGEID != pageInBucket )
      {
         DMS_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_ADDRESSING, 1 ) ;
         extRW = extent2RW( pageInBucket, -1 ) ;
         extRW.setNothrow( TRUE ) ;
         blk = extRW.readPtr<_dmsLobDataMapBlk>() ;
         if ( !blk )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                    "pageid:%d", pageInBucket ) ;
            rc = SDB_SYS ;
            goto error ;
         }

#if defined (_DEBUG)
         {
            UINT32 __hash = _calcHash( *blk ) ;
            UINT32 testBucketNo = _getBucket( __hash ) ;
            if ( testBucketNo != bucketNumber )
            {
               dmsLobDataMapBlk memBlk ;
               ossMemcpy( &memBlk, blk, sizeof( memBlk ) ) ;
               SDB_ASSERT( testBucketNo == bucketNumber, "must be same" ) ;
            }
         }
#endif
         if ( clID == blk->_clLogicalID &&
              blk->equals( record._oid->getData(), record._sequence ) )
         {
            page = pageInBucket ;
            break ;
         }
         else
         {
            pageInBucket = blk->_nextPageInBucket ;
            continue ;
         }
      }

      if ( DMS_LOB_INVALID_PAGEID != pageInBucket && NULL != bucket )
      {
         *bucket = bucketNumber ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__FIND, rc ) ;
      return rc ;
   error:
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
      CHAR *pBuffer = NULL ;
      UINT32 hasWriteSize = 0 ;

      SDB_ASSERT( 0 == DMS_BME_SZ % DMS_SEGMENT_SZ2M, "Invalid buffer size" ) ;

      pBuffer = ( CHAR* )SDB_OSS_MALLOC( DMS_SEGMENT_SZ2M ) ;
      if ( !pBuffer )
      {
         PD_LOG( PDERROR, "Allocate buffer(%u) failed", DMS_SEGMENT_SZ2M ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      /// init the buffer
      ossMemset( pBuffer, DMS_LOB_INVALID_PAGEID, DMS_SEGMENT_SZ2M ) ;

      /// write data
      while( hasWriteSize < DMS_BME_SZ )
      {
         rc = _writeFile ( file, (const CHAR *)pBuffer, DMS_SEGMENT_SZ2M ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to write new bme buffer to file rc: %d", rc ) ;
            goto error ;
         }
         hasWriteSize += DMS_SEGMENT_SZ2M ;
      }

   done:
      if ( NULL != pBuffer )
      {
         SDB_OSS_FREE( pBuffer ) ;
         pBuffer = NULL ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ONCREATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ONMAPMETA, "_dmsStorageLob::_onMapMeta" )
   INT32 _dmsStorageLob::_onMapMeta( UINT64 curOffSet, BOOLEAN isCreateNew )
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
      if ( _pStorageInfo )
      {
         return _pStorageInfo->_extentThreshold >> _data.pageSizeSquareRoot() ;
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
      BOOLEAN needDoPost = FALSE ;

      if ( _data.isOpened() )
      {
         INT64 extentLen = _data.getSegmentSize() ;
         rc = _data.extend( extentLen * numSeg ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend lobd file, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Lob data file is not opened" ) ;
         goto error ;
      }

      needDoPost = TRUE ;

      rc = this->_dmsStorageBase::_extendSegments( numSeg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to extend lobm file, rc: %d", rc ) ;
         goto error ;
      }
   done:
      if ( needDoPost )
      {
         INT32 rcTmp = _data.postExtend( pageNum(), rc ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Do lob data post extend failed, rc: %d", rcTmp ) ;
            rc = rc ? rc : rcTmp ;
         }
      }
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
      else if ( pHeader->_secretValue != _pStorageInfo->_secretValue )
      {
         PD_LOG( PDERROR, "Secret value[%llu] not the same with data su[%llu]",
                 pHeader->_secretValue, _pStorageInfo->_secretValue ) ;
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
      UINT16 i = 0 ;
      dmsMBStatInfo *pMBStat = NULL ;

      /// cache the lobm version ( like _pageSize ), so the hash hot path
      /// does not need to read the mmap header
      _hashVersion = getHeader()->_version ;

      i = _dmsData->_nextUsedMBSlot( 0 ) ;
      while ( DMS_INVALID_MBID != i )
      {
         pMBStat = &( _dmsData->_mbStatInfo[i] ) ;

         pMBStat->_lobLastWriteTick = ~0 ;
         pMBStat->_lobCommitFlag.init( 1 ) ;

         /*
            Check the collection is valid
         */
         if ( !_needDelayOpen && !isCrashed() )
         {
            if ( 0 == _dmsData->_dmsMME->_mbList[i]._lobCommitFlag )
            {
               /// upgrade from the old version which has no
               /// _commitLSN/_idxCommitLSN/_lobCommitLSN in mb block,
               /// so the value of _commitLSN/_idxCommitLSN/_lobCommitLSN is 0
               if ( 0 == _dmsData->_dmsMME->_mbList[i]._lobCommitLSN )
               {
                  _dmsData->_dmsMME->_mbList[i]._lobCommitLSN =
                     _pStorageInfo->_curLSNOnStart ;
               }
               _dmsData->_dmsMME->_mbList[i]._lobCommitFlag = 1 ;
               pMBStat->_lobCommitFlagSync = _dmsData->_dmsMME->_mbList[i]._lobCommitFlag ;
               needFlushMME = TRUE ;
            }
            pMBStat->_lobCommitFlag.init( 1 ) ;
         }
         else
         {
            pMBStat->_lobCommitFlag.init( _dmsData->_dmsMME->_mbList[i]._lobCommitFlag ) ;
         }
         if ( !_needDelayOpen )
         {
            /// not set crash, keep init value when delay open
            pMBStat->_lobIsCrash = ( 0 == pMBStat->_lobCommitFlag.peek() ) ? TRUE : FALSE ;
         }
         pMBStat->_lobLastLSN.init( _dmsData->_dmsMME->_mbList[i]._lobCommitLSN ) ;

         i = _dmsData->_nextUsedMBSlot( i + 1 ) ;
      }

      if ( needFlushMME )
      {
         /// need skip mem sync
         _dmsData->flushMME( isSyncDeep(), TRUE ) ;
      }

      return SDB_OK ;
   }

   INT32 _dmsStorageLob::_onFlushDirty( BOOLEAN force, BOOLEAN sync )
   {
      INT32 rc = SDB_OK ;
      _utilStackBitmap<DMS_MME_SLOTS> bitmap ;
      dmsMBStatInfo *pMBStat = NULL ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
         if ( pMBStat->_lobCommitFlag.compareAndSwap( 0, 1 ) )
         {
            bitmap.setBit( i ) ;
         }
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
               if ( 0 == syncPages )
               {
                  break ;
               }
               else if ( !force && beginDirtyPages < _pCacheUnit->dirtyPages() + syncPages )
               {
                  /// sync page interrupt
                  rc = SDB_INVALIDARG ;
                  break ;
               }
            }
            _pCacheUnit->unlockPageCleaner() ;
         }

         _data.flush() ;

         if ( rc )
         {
            for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
            {
               if ( bitmap.testBit( i ) )
               {
                  pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
                  pMBStat->_lobCommitFlag.compareAndSwap( 1, 0 ) ;
               }
            }
         }
      }
      return rc ;
   }

   void _dmsStorageLob::incWritePtrCount( INT32 collectionID )
   {
      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         dmsMBStatInfo *pMBStat = &( _dmsData->_mbStatInfo[ collectionID ] ) ;
         pMBStat->_curLobWriteCount.inc() ;
      }
   }

   void _dmsStorageLob::decWritePtrCount( INT32 collectionID )
   {
      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         dmsMBStatInfo *pMBStat = &( _dmsData->_mbStatInfo[ collectionID ] ) ;
         pMBStat->_curLobWriteCount.dec() ;
      }
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
      dmsMBStatInfo *pMBStat = NULL ;

      UINT16 i = _dmsData->_nextUsedMBSlot( 0 ) ;
      while ( DMS_INVALID_MBID != i )
      {
         pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
         tmpLSN = pMBStat->_lobLastLSN.fetch() ;
         if ( !pMBStat->_lobCommitFlag.compare( 0 ) )
         {
            tmpCommitFlag = pMBStat->_lobIsCrash ? 0 : pMBStat->_lobCommitFlag.fetch() ;

            if ( !tmpCommitFlag )
            {
               setHeadCommFlgValid = FALSE ;
               pMBStat->_lobCommitFlag.swap( 0 ) ;
            }

            if ( tmpLSN != _dmsData->_dmsMME->_mbList[i]._lobCommitLSN ||
                 tmpCommitFlag != _dmsData->_dmsMME->_mbList[i]._lobCommitFlag )
            {
               _dmsData->_dmsMME->_mbList[i]._lobCommitLSN = tmpLSN ;
               _dmsData->_dmsMME->_mbList[i]._lobCommitTime = lastTime ;

               pMBStat->_lobCommitTime = _dmsData->_dmsMME->_mbList[i]._lobCommitTime ;

               if ( tmpCommitFlag &&
                    pMBStat->_curLobWriteCount.fetch() > 0 &&
                    !isClosed() )
               {
                  /// has some write operator in the collection
                  setHeadCommFlgValid = FALSE ;
                  pMBStat->_lobCommitFlag.swap( 0 ) ;
               }
               else
               {
                  _dmsData->_dmsMME->_mbList[i]._lobCommitFlag = tmpCommitFlag ;
                  pMBStat->_lobCommitFlagSync = _dmsData->_dmsMME->_mbList[i]._lobCommitFlag ;
               }

               /// double check
               if ( pMBStat->_lobCommitFlag.compare( 0 ) &&
                    _dmsData->_dmsMME->_mbList[i]._lobCommitFlag )
               {
                  _dmsData->_dmsMME->_mbList[i]._lobCommitFlag = 0 ;
                  pMBStat->_lobCommitFlagSync = _dmsData->_dmsMME->_mbList[i]._lobCommitFlag ;
                  setHeadCommFlgValid = FALSE ;
               }

               needFlush = TRUE ;
            }
         }
         else
         {
            setHeadCommFlgValid = FALSE ;
         }

         /// update last lsn
         if ( (UINT64)~0 == lastLSN ||
              ( (UINT64)~0 != tmpLSN && lastLSN < tmpLSN ) )
         {
            lastLSN = tmpLSN ;
         }

         i = _dmsData->_nextUsedMBSlot( i + 1 ) ;
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
      dmsMBStatInfo *pMBStat = NULL ;

      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         pMBStat = &( _dmsData->_mbStatInfo[ collectionID ] ) ;
         pMBStat->_lobLastWriteTick = pmdGetDBTick() ;
         if ( !pMBStat->_lobIsCrash &&
              pMBStat->_lobCommitFlag.compareAndSwap( 1, 0 ) )
         {
            needSync = TRUE ;
            _dmsData->_dmsMME->_mbList[ collectionID ]._lobCommitFlag = 0 ;
            pMBStat->_lobCommitFlagSync = _dmsData->_dmsMME->_mbList[ collectionID ]._lobCommitFlag ;
         }
      }
      else if ( -1 == collectionID )
      {
         UINT16 i = _dmsData->_nextUsedMBSlot( 0 ) ;
         while( DMS_INVALID_MBID != i )
         {
            pMBStat = &( _dmsData->_mbStatInfo[ i ] ) ;
            pMBStat->_lobLastWriteTick = pmdGetDBTick() ;
            if ( !pMBStat->_lobIsCrash &&
                 pMBStat->_lobCommitFlag.compareAndSwap( 1, 0 ) )
            {
               needSync = TRUE ;
               _dmsData->_dmsMME->_mbList[ i ]._lobCommitFlag = 0 ;
               pMBStat->_lobCommitFlagSync = _dmsData->_dmsMME->_mbList[ i ]._lobCommitFlag ;
            }

            i = _dmsData->_nextUsedMBSlot( i + 1 ) ;
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
      dmsMBStatInfo *pMBStat = NULL ;

      UINT16 i = _dmsData->_nextUsedMBSlot( 0 ) ;
      while ( DMS_INVALID_MBID != i )
      {
         pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
         lastWriteTick = pMBStat->_lobLastWriteTick ;
         /// The collection is commit valid, should ignored
         if ( 0 == pMBStat->_lobCommitFlag.peek() &&
              lastWriteTick < oldestWriteTick )
         {
            oldestWriteTick = lastWriteTick ;
         }

         i = _dmsData->_nextUsedMBSlot( i + 1 ) ;
      }
      return oldestWriteTick ;
   }

   void _dmsStorageLob::_onRestore()
   {
      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         dmsMBStatInfo *pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
         pMBStat->_lobIsCrash = FALSE ;
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
      dmsMBStatInfo *pMBStat = NULL ;

      /// clear all lob count
      for( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
         pMBStat->_totalLobs = 0 ;
         pMBStat->_totalLobSize = 0 ;
         pMBStat->_totalValidLobSize = 0 ;
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
               pMBStat = &( _dmsData->_mbStatInfo[blk->_mbID] ) ;
               pMBStat->_totalLobSize += blk->_dataLen ;
               pMBStat->_totalValidLobSize += blk->_dataLen ;
            }
            else
            {
               pMBStat = &( _dmsData->_mbStatInfo[blk->_mbID] ) ;
               /// dmsLobMate size take the value: 1k.
               lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( blk->_dataLen ) ;
               pMBStat->_totalLobs += 1 ;
               pMBStat->_totalLobSize += lobPieceLen ;
               pMBStat->_totalValidLobSize += lobPieceLen ;
            }
         }
         ++current ;
      }
      /// flush MME
      _dmsData->flushMME( TRUE ) ;

      /// update the header. Only upgrade an old file to VERSION_2 ( which
      /// still uses the djb2 hash ). Never relabel an old djb2 file as
      /// VERSION_3 ( md5 ), and never downgrade a VERSION_3 file, otherwise
      /// the bucket hash algorithm would mismatch the existing buckets.
      if ( _dmsHeader->_version < DMS_LOB_VERSION_2 )
      {
         _dmsHeader->_version = DMS_LOB_VERSION_2 ;
         _hashVersion = _dmsHeader->_version ;
      }
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
      dmsMBStatInfo *pMBStat = NULL ;

      /// reset bme
      ossMemset( (void*)_dmsBME, 0xFF, sizeof(dmsBucketsManagementExtent) ) ;

      /// clear all lob count
      for( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         pMBStat = &( _dmsData->_mbStatInfo[i] ) ;
         pMBStat->_totalLobs = 0 ;
         pMBStat->_totalLobPages = 0 ;
         pMBStat->_totalLobSize = 0 ;
         pMBStat->_totalValidLobSize = 0 ;
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
                           blk->_dataLen, NULL, hashType() ) ;
               /// add page to bucket
               __hash = _calcHash( *blk ) ;
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
               pMBStat = &( _dmsData->_mbStatInfo[blk->_mbID] ) ;
               pMBStat->_totalLobPages += 1 ;
               if ( DMS_LOB_META_SEQUENCE == blk->_sequence )
               {
                  ++totalLobs ;
                  /// dmsLobMate size take the value: 1k.
                  lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( blk->_dataLen ) ;
                  totalLobSize += lobPieceLen ;
                  totalValidLobSize += lobPieceLen ;
                  pMBStat->_totalLobs += 1 ;
                  pMBStat->_totalLobSize += lobPieceLen ;
                  pMBStat->_totalValidLobSize += lobPieceLen ;
               }
               else
               {
                  /// dmsLobMate size take the value: 1k.
                  totalLobSize += blk->_dataLen ;
                  totalValidLobSize += blk->_dataLen ;
                  pMBStat->_totalLobSize += blk->_dataLen ;
                  pMBStat->_totalValidLobSize += blk->_dataLen ;
               }
            }
         }
         ++current ;
      }

      /// update the header. An old file rebuilt here was hashed with djb2,
      /// so only upgrade it to VERSION_2 ( still djb2 ), never to VERSION_3
      /// ( md5 ). A VERSION_3 file keeps its version and md5 hash.
      if ( _dmsHeader->_version <= DMS_LOB_VERSION_1 )
      {
         _dmsHeader->_version = DMS_LOB_VERSION_2 ;
         _hashVersion = _dmsHeader->_version ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_FREECACHE, "_dmsStorageLob::freeCache" )
   INT32 _dmsStorageLob::freeCache( UINT32 segmentID )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      DMS_LOB_PAGEID startPageID = 0 ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_FREECACHE ) ;

      rcTmp = _ossMmapFile::freeCache( segmentID ) ;
      if ( rcTmp != SDB_OK )
      {
         PD_LOG( PDWARNING, "Failed to invalidate cache of segment[%d], "
                 "rc: %d", segmentID, rcTmp ) ;
         rc = rcTmp ;
      }

      startPageID = segment2Extent( segmentID, 0 ) ;
      if ( startPageID != DMS_INVALID_EXTENT )
      {
         rcTmp = _data.freeCache( startPageID, segmentPages() ) ;
         if ( rcTmp != SDB_OK )
         {
            PD_LOG( PDWARNING, "Failed to invalidate cache of segment[%d], "
                  "rc: %d", segmentID, rcTmp ) ;
            rc = rcTmp ;
         }
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_FREECACHE, rc ) ;
      return rc ;
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
         ossScopedLock lock( &_delayOpenLatch ) ;

         /// when not opened, return error, and not delay open
         if ( _needDelayOpen )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get mblock, rc: %d", rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__REMOVEPAGE, "_dmsStorageLob::_removePage" )
   INT32 _dmsStorageLob::_removePage( DMS_LOB_PAGEID page,
                                      _dmsLobDataMapBlk *blk,
                                      const UINT32 *bucket,
                                      pmdEDUCB *cb,
                                      dmsMBContext *mbContext,
                                      BOOLEAN hasLockBucket,
                                      BOOLEAN needRelease,
                                      const dmsLobRecord *pRecord )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__REMOVEPAGE ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      UINT32 bucketNumber = 0 ;
      INT64 lobPieceLen   = 0 ;

      if ( NULL != bucket )
      {
         bucketNumber = *bucket ;
      }
      else
      {
         UINT32 __hash1 = _calcHash( *blk ) ;
         bucketNumber = _getBucket( __hash1 ) ;
      }

      /// lock
      if ( !hasLockBucket )
      {
         _getBucketLatch( bucketNumber )->get() ;
      }

      if ( DMS_LOB_INVALID_PAGEID == blk->_prevPageInBucket )
      {
         SDB_ASSERT( _dmsBME->_buckets[bucketNumber] == page,
                     "must be this page" ) ;
         _dmsBME->_buckets[bucketNumber] = blk->_nextPageInBucket ;
         if ( DMS_LOB_INVALID_PAGEID != blk->_nextPageInBucket )
         {
            dmsExtRW nextRW ;
            _dmsLobDataMapBlk *nextBlk = NULL ;
            nextRW = extent2RW( blk->_nextPageInBucket, -1 ) ;
            nextRW.setNothrow( TRUE ) ;
            nextBlk = nextRW.writePtr<_dmsLobDataMapBlk>() ;
            if ( !nextBlk )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                       "pageid:%d", blk->_nextPageInBucket ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            nextBlk->_prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
         }
      }
      else
      {
         dmsExtRW prevRW ;
         _dmsLobDataMapBlk *prevBlk = NULL ;
         prevRW = extent2RW( blk->_prevPageInBucket, -1 ) ;
         prevRW.setNothrow( TRUE ) ;
         prevBlk = prevRW.writePtr<_dmsLobDataMapBlk>() ;
         if ( !prevBlk )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                    blk->_prevPageInBucket ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         prevBlk->_nextPageInBucket = blk->_nextPageInBucket ;

         if ( DMS_LOB_INVALID_PAGEID != blk->_nextPageInBucket )
         {
            dmsExtRW nextRW ;
            _dmsLobDataMapBlk *nextBlk = NULL ;
            nextRW = extent2RW( blk->_nextPageInBucket, -1 ) ;
            nextRW.setNothrow( TRUE ) ;
            nextBlk = nextRW.writePtr<_dmsLobDataMapBlk>() ;
            if ( !nextBlk )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                       blk->_nextPageInBucket ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            nextBlk->_prevPageInBucket = blk->_prevPageInBucket ;
         }
      }

      if ( DMS_LOB_META_SEQUENCE == blk->_sequence )
      {
         mbContext->mbStat()->_totalLobs -= 1 ;
         lobPieceLen = DMS_GET_LOB_PIECE_LENGTH( blk->_dataLen ) ;
         mbContext->mbStat()->subTotalLobSize( lobPieceLen ) ;
         if ( NULL != pRecord )
         {
            _statVaildLobSize( mbContext, NULL, (_dmsLobMeta *)pRecord->_data ) ;
         }
      }
      else
      {
         mbContext->mbStat()->subTotalLobSize( blk->_dataLen ) ;
      }
      /// monitor lob page which is removed
      DMS_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_TRUNCATE, 1 ) ;

      _incWriteRecord() ;
      blk->reset() ;
      blk->setRemoved() ;

      /// release the page
      if ( needRelease )
      {
         _releasePage( page, mbContext ) ;
      }
   done:
      if ( !hasLockBucket )
      {
         _getBucketLatch( bucketNumber )->release() ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__REMOVEPAGE, rc ) ;
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

      DMS_LOB_PAGEID current = -1 ;
      BOOLEAN locked = FALSE ;
      BOOLEAN needPanic = FALSE ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      utilCacheContext cContext ;
      UINT32 dirtyStart = 0 ;
      UINT32 dirtyLen = 0 ;
      UINT64 beginLSN = 0 ;
      UINT64 endLSN = 0 ;

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in remove", getSuName() ) ;
         goto error ;
      }

      /// make full name
      _clFullName( mbContext->mbStat()->_collectionName, fullName,
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

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mbStat()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mbStat()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      needPanic = TRUE ;
      while ( ( UINT32 )++current < pageNum() )
      {
         if ( !DMS_LOB_PAGE_IN_USED( current ) )
         {
            continue ;
         }

         dmsExtRW extRW ;
         const _dmsLobDataMapBlk *readBlk = NULL ;
         _dmsLobDataMapBlk *blk = NULL ;
         extRW = extent2RW( current, mbContext->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         readBlk = extRW.readPtr<_dmsLobDataMapBlk>() ;
         if ( !readBlk )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                    current ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         /// need first check undefined
         if ( readBlk->isUndefined() )
         {
            continue ;
         }
         /// then check clLID
         else if ( mbContext->clLID() != readBlk->_clLogicalID )
         {
            continue ;
         }

         /// change to write mode
         blk = extRW.writePtr<_dmsLobDataMapBlk>() ;
         if ( !blk )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                    current ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _removePage( current, blk, NULL, cb, mbContext, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove page:%d, rc:%d", current, rc ) ;
            goto error ;
         }
         /// when the page is dirty, dicard the page, size is 0, will not
         /// alloc the page when page is not in memory
         _pCacheUnit->prepareWrite( current, 0, 0, cb, cContext ) ;
         cContext.discardPage( dirtyStart, dirtyLen, beginLSN, endLSN ) ;
         cContext.release() ;
      }

      // clear the stat info
      mbContext->mbStat()->_totalLobPages = 0 ;
      mbContext->mbStat()->_totalLobs = 0 ;
      mbContext->mbStat()->resetTotalLobSize() ;
      mbContext->mbStat()->resetTotalValidLobSize() ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(),
                         mbContext->clLID(),
                         DMS_INVALID_EXTENT, cb ) ;

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
                            mbContext->clLID(), DMS_INVALID_EXTENT ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ROLLBACK, "_dmsStorageLob::_rollback" )
   INT32 _dmsStorageLob::_rollback( const dmsLobRecord &record,
                                    DMS_LOB_PAGEID page,
                                    pmdEDUCB *cb,
                                    dmsMBContext *mbContext,
                                    BOOLEAN pageFilled )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ROLLBACK ) ;
      BOOLEAN locked = FALSE ;
      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         goto done ;
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
         PD_LOG( PDERROR, "File[%s] is not open in write", getSuName() ) ;
         goto error ;
      }

      if ( pageFilled )
      {
         dmsExtRW extRW ;
         _dmsLobDataMapBlk *blk = NULL ;
         extRW = extent2RW( page, mbContext->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         blk = extRW.writePtr<_dmsLobDataMapBlk>() ;
         if ( !blk )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                    "pageid:%d", page ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         rc = _removePage( page, blk, NULL, cb, mbContext, FALSE, TRUE, &record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove page:%d, rc:%d", page, rc ) ;
            goto error ;
         }
      }
      else
      {
         _releasePage( page, mbContext ) ;
      }
   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ROLLBACK, rc ) ;
      return rc ;
   error:
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
