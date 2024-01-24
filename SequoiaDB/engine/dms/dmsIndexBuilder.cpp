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

   Source File Name = dmsIndexBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexBuilder.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsIndexOnlineBuilder.hpp"
#include "dmsIndexSortingBuilder.hpp"
#include "dmsIndexExtBuilder.hpp"
#include "dmsScanner.hpp"
#include "dmsCB.hpp"
#include "ixm.hpp"
#include "pdSecure.hpp"

using namespace bson ;

namespace engine
{

   /*
      _dmsIndexBuilder implement
    */
   _dmsIndexBuilder::_dmsIndexBuilder( _dmsStorageUnit* su,
                                       _dmsMBContext* mbContext,
                                       _pmdEDUCB* eduCB,
                                       dmsExtentID indexExtentID,
                                       dmsExtentID indexLogicID,
                                       dmsIndexBuildGuardPtr &guardPtr,
                                       dmsDupKeyProcessor *dkProcessor,
                                       dmsIdxTaskStatus* pIdxStatus )
   : _su( su ),
     _suIndex ( su->index() ),
     _suData ( su->data() ),
     _mbContext ( mbContext ),
     _eduCB ( eduCB ),
     _buildGuardPtr( guardPtr ),
     _indexExtentID ( indexExtentID ),
     _indexLID( indexLogicID ),
     _dkProcessor( dkProcessor ),
     _pIdxStatus( pIdxStatus )
   {
      _indexCB = NULL ;
      _scanRID.reset() ;
      _unique = FALSE ;
      _dropDups = FALSE ;
      _pOprHandler = NULL ;
      _pResult = NULL ;
      _remoteOperator = NULL ;
      _checker = NULL ;
   }

   _dmsIndexBuilder::~_dmsIndexBuilder()
   {
      if ( _buildGuardPtr )
      {
         _buildGuardPtr->buildExit() ;
      }
      _suIndex = NULL ;
      _suData = NULL ;
      _mbContext = NULL ;
      SAFE_OSS_DELETE( _indexCB ) ;
      _releaseScannerChecker() ;
   }

   void _dmsIndexBuilder::setOprHandler( IDmsOprHandler *pOprHander )
   {
      _pOprHandler = pOprHander ;
   }

   void _dmsIndexBuilder::setWriteResult( utilWriteResult *pResult )
   {
      _pResult = pResult ;
   }

   INT32 _dmsIndexBuilder::_init()
   {
      INT32 rc = SDB_OK ;

      BOOLEAN needTruncate = FALSE ;

      SDB_ASSERT( _suIndex != NULL, "_suIndex can't be NULL" ) ;
      SDB_ASSERT( _suData != NULL, "_suData can't be NULL" ) ;
      SDB_ASSERT( _mbContext != NULL, "_mbContext can't be NULL" ) ;

      // lock mb
      rc = _mbLockAndCheck( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      // make sure the extent is valid
      PD_CHECK ( DMS_INVALID_EXTENT != _indexExtentID,
                 SDB_INVALIDARG, error, PDERROR,
                 "Index Extent ID %d is invalid for collection %d",
                 _indexExtentID, (INT32)_mbContext->mbID() ) ;

      _indexCB = SDB_OSS_NEW ixmIndexCB ( _indexExtentID,
                                          _suIndex, _mbContext ) ;
      if ( NULL == _indexCB )
      {
         PD_LOG ( PDERROR, "failed to allocate _indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // verify the index control block is initialized
      PD_CHECK ( _indexCB->isInitialized(), SDB_DMS_INIT_INDEX,
                 error, PDERROR, "Failed to initialize index" ) ;

      // Someone else may have dropped the index before we take the mb lock.
      // Check for that.
      if ( _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         PD_LOG( PDERROR, "Index logical id in indexCB is not as expected. "
                          "The index may have been recreated") ;
         goto error ;
      }

      rc = _indexCB->getIndexID ( _indexOID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get indexID, rc = %d", rc ) ;

      // already creating, continue
      if ( IXM_INDEX_FLAG_CREATING == _indexCB->getFlag() )
      {
         _scanRID = _indexCB->getScanRID() ;
      }
      else if ( IXM_INDEX_FLAG_NORMAL == _indexCB->getFlag() ||
                IXM_INDEX_FLAG_INVALID == _indexCB->getFlag() )
      {
         // truncate index, do not remove root
         rc = _indexCB->truncate ( FALSE, IXM_INDEX_FLAG_CREATING ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index, rc: %d", rc ) ;
            goto error ;
         }
         needTruncate = TRUE ;
      }
      else
      {
         PD_LOG ( PDERROR, "Index status[%d(%s)] is unexpected",
                  (INT32)_indexCB->getFlag(),
                  ixmGetIndexFlagDesp( _indexCB->getFlag() ) ) ;
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto error ;
      }

      _unique = _indexCB->unique() ;
      _dropDups = _indexCB->dropDups() ;

      if ( _indexCB->isGlobal() )
      {
         if ( _eduCB->isAffectGIndex() )
         {
            // only primary need to insert remote index
            rc = _eduCB->getOrCreateRemoteOperator( &_remoteOperator ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get remote operator, rc: %d",
                         rc ) ;
         }
      }

      rc = _suIndex->getIndex( _mbContext, _indexCB, _eduCB, _idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get index, rc: %d", rc ) ;

      if ( needTruncate )
      {
         dmsTruncateIdxOptions options ;
         rc = _idxPtr->truncate( options, _eduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate index store, rc: %d", rc ) ;
      }

      // set key pattern to key generator
      rc = _keyGen.setKeyPattern( _indexCB->keyPattern() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set key pattern, rc: %d", rc ) ;

      rc = _createScannerChecker() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create scanner checker, "
                   "rc: %d", rc ) ;

      // WARNING: should not rewrite return code from _onInit()
      // if SDB_DMS_EOC is returned, it will be processed with caller
      rc = _onInit() ;
      if ( rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDERROR, "Post init operation failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      SAFE_DELETE( _indexCB ) ;
      _releaseScannerChecker() ;
      goto done ;
   }

   INT32 _dmsIndexBuilder::_onInit()
   {
      INT32 rc = SDB_OK ;

      /// start rebuilding
      // _currentExtentID = _mbContext->mb()->_firstExtentID ;
      // if ( DMS_INVALID_EXTENT == _currentExtentID )
      // {
      //    /// when the collection is empty, we complete the index creating
      //    /// fast( when unlock the context and scan the data, because the
      //    /// scanExtLID always is -1, so when scan finished, the new instor
      //    /// will not insert to the index )
      //    _indexCB->setFlag ( IXM_INDEX_FLAG_NORMAL ) ;
      //    _indexCB->scanExtLID ( DMS_INVALID_EXTENT ) ;
      //    rc = SDB_DMS_EOC ;
      //    goto error ;
      // }

      if ( _pIdxStatus && DMS_TASK_STATUS_RUN == _pIdxStatus->status() )
      {
         _pIdxStatus->setTotalRecNum( _mbContext->mbStat()->_totalRecords.fetch() ) ;
         _pIdxStatus->resetPcsedRecNum() ;
      }

      return rc ;
   }

   INT32 _dmsIndexBuilder::_finish()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_OK == ( rc = _checkIndexAfterLock( EXCLUSIVE ) ) )
      {
         _indexCB->setFlag ( IXM_INDEX_FLAG_NORMAL ) ;
         _indexCB->scanExtLID ( DMS_INVALID_EXTENT ) ;
         _indexCB->setScanExtOffset( DMS_INVALID_OFFSET ) ;
         _mbContext->mbUnlock() ;
      }

      _releaseScannerChecker() ;

      return rc ;
   }

   INT32 _dmsIndexBuilder::build()
   {
      INT32 rc = SDB_OK ;

      dpsTransExecutor *pTransExe = _eduCB->getTransExecutor() ;

      /// first get transisolation
      INT32 oldIsolation = pTransExe->getTransIsolation() ;
      UINT32 oldMask = pTransExe->getTransConfMask() ;
      /// set isolation to ru
      pTransExe->setTransIsolation( TRANS_ISOLATION_RU, FALSE ) ;

      rc = _init() ;
      if ( SDB_DMS_EOC == rc )
      {
         /// no data, has already finished
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         goto init_error ;
      }

      rc = _build() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _finish() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      /// restore transisolation
      pTransExe->setTransIsolation( oldIsolation,
                                    ( oldMask & TRANS_CONF_MASK_ISOLATION ) ) ;
      return rc ;
   init_error:
      goto done ;
   error:
      // do NOT set flag when SDB_DMS_INVALID_INDEXCB,
      // because something else may changed the indexCB,
      // it's possible other threads dropped index so that
      // the extent is reused. So we should NOT touch the block here
      if ( SDB_DMS_INVALID_INDEXCB != rc )
      {
         if ( SDB_OK == _checkIndexAfterLock( SHARED ) )
         {
            _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            _mbContext->mbUnlock() ;
         }
      }
      goto done ;
   }

   INT32 _dmsIndexBuilder::_beforeExtent()
   {
      INT32 rc = SDB_OK ;

      // make sure the index cb is still valid
      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

      if ( _pIdxStatus && DMS_TASK_STATUS_RUN == _pIdxStatus->status() )
      {
         // in case _totalRecords has changed
         _pIdxStatus->setTotalRecNum( _mbContext->mbStat()->_totalRecords.fetch() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_afterExtent( const dmsRecordID &lastRID,
                                         UINT64 scannedNum,
                                         BOOLEAN isEOF )
   {
      INT32 rc = SDB_OK ;

      if ( isEOF )
      {
         // done scan, set scanned extent to maximum value
         // so coming write operators will update this index
         _indexCB->setScanRID( dmsRecordID::maxRID() ) ;
         _buildGuardPtr->buildEnd() ;
      }
      else
      {
         _indexCB->setScanRID( lastRID ) ;
      }

      if ( _pIdxStatus && DMS_TASK_STATUS_RUN == _pIdxStatus->status() )
      {
         _pIdxStatus->incPcsedRecNum( scannedNum ) ;
      }

      // check if scanner is interrupted
      rc = _checkInterrupt() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_getKeySet( ossValuePtr recordDataPtr,
                                       BSONObjSet& keySet )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj obj ( (CHAR*)recordDataPtr ) ;
         BSONElement arrEle ;

         rc = _keyGen.getKeys( obj, keySet, &arrEle ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to get keys from object %s",
                       PD_SECURE_OBJ( obj ) ) ;

         rc = _indexCB->checkKeys( obj, keySet, arrEle, _pResult ) ;
         if ( SDB_OK != rc &&
              NULL != _pResult &&
              _pResult->getCurID().isEmpty() )
         {
            _pResult->setCurrentID( obj ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to check keys for object %s, "
                      "rc: %d", PD_SECURE_OBJ( obj ), rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_insertKey( const BSONObj &key,
                                       const dmsRecordID &rid,
                                       const Ordering& ordering )
   {
      INT32 rc = SDB_OK ;

      // check key size
      INT32 keySize = key.objsize() + 1 ;
      if ( keySize > _su->index()->indexKeySizeMax() )
      {
         PD_LOG ( PDERROR, "key size [%d] must be less than or equal to [%d]",
                  keySize, _su->index()->indexKeySizeMax() ) ;
         rc = SDB_IXM_KEY_TOO_LARGE ;
         goto error ;
      }

      // Callback to validate in memory tree
      if ( _pOprHandler && _indexCB->unique() )
      {
         rc = _pOprHandler->onInsertIndex( _mbContext, _indexCB,
                                           _indexCB->unique(),
                                           _indexCB->enforced(),
                                           key, rid, _eduCB, _pResult ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Insert index callback failed, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = _idxPtr->index( key, rid, !( _indexCB->unique() ), _eduCB, _pResult ) ;
      if ( SDB_OK != rc )
      {
         // during index rebuild, it's possible some other
         // sessions inserted records that already stored in
         // index, and then when we scan the disk we read it
         // again. In  this case let's simply skip it
         if ( SDB_IXM_IDENTICAL_KEY == rc )
         {
            rc = SDB_OK ;
            PD_LOG ( PDWARNING, "Identical key is detected "
                     "during index rebuild, ignore" ) ;
         }
         else if ( SDB_IXM_DUP_KEY == rc && NULL != _dkProcessor )
         {
            // try to fix with duplicated key processor
            INT32 tmpRC = SDB_OK ;

            PD_LOG( PDWARNING, "Failed to insert into index with duplicated "
                    "key record at ( extent %d, offset %d ), rc: %d",
                    rid._extent, rid._offset, rc ) ;
            tmpRC = _dkProcessor->processDupKeyRecord( _suData,
                                                       _mbContext,
                                                       rid, 0, _eduCB ) ;
            if ( tmpRC != SDB_OK )
            {
               // failed to process
               PD_LOG( PDWARNING, "Failed to process duplicated key "
                       "record at ( extent %d, offset %d ), rc: %d",
                       rid._extent, rid._offset, tmpRC ) ;
            }
            else
            {
               // succeed to process, reset result
               rc = SDB_OK ;
               if ( NULL != _pResult )
               {
                  _pResult->reset() ;
               }
            }
         }
         else
         {
            // for any other index insert error, let's return error
            PD_LOG ( PDERROR, "Failed to insert into index, rc: %d", rc ) ;
            goto error ;
         }
      }

      if ( NULL != _remoteOperator )
      {
         // avoid primary change when creating global index
         PD_CHECK( pmdIsPrimary(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                   "Failed to insert global index due to primary change" ) ;

         // insert index to remote index cl
         rc = _remoteOperator->insert( _indexCB->getIndexCLName(), key,
                                       0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert to remote:cl=%s,"
                      "insertor=%s,rc=%d", _indexCB->getIndexCLName(),
                      PD_SECURE_OBJ( key ), rc ) ;
      }

   done:
      if ( rc && _pResult && _pResult->isMaskEnabled( UTIL_RESULT_MASK_ID ) )
      {
         if ( _pResult->getCurID().isEmpty() &&
              _pResult->getCurRID().isValid() )
         {
            dmsRecordData curData ;
            if ( SDB_OK == _suData->fetch( _mbContext, _pResult->getCurRID(),
                                           curData, _eduCB ) )
            {
               BSONObj curObj( curData.data() ) ;
               _pResult->setCurrentID( curObj ) ;
            }
         }
         if ( _pResult->getPeerID().isEmpty() &&
              _pResult->getPeerRID().isValid() )
         {
            dmsRecordData peerData ;
            if ( SDB_OK == _suData->fetch( _mbContext, _pResult->getPeerRID(),
                                           peerData, _eduCB ) )
            {
               BSONObj peerObj( peerData.data() ) ;
               _pResult->setPeerID( peerObj ) ;
            }
         }
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_insertKey( ossValuePtr recordDataPtr,
                                       const dmsRecordID &rid,
                                       const Ordering& ordering )
   {
      BSONObjSet keySet ;
      BSONObjSet::iterator it ;
      INT32 rc = SDB_OK ;

      rc = _getKeySet( recordDataPtr, keySet ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for ( it = keySet.begin() ; it != keySet.end() ; it++ )
      {
         rc = _insertKey( *it, rid, ordering ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_mbLockAndCheck( INT32 lockType )
   {
      INT32 rc = SDB_OK ;

      while ( TRUE )
      {
         rc = _mbContext->mbLock( lockType ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
            goto error ;
         }

         if ( _mbContext->mbStat()->_blockIndexCreatingCount > 0 )
         {
            // other operation need to block index creating
            _mbContext->mbUnlock() ;
            ossSleep( DMS_INDEX_WAITBLOCK_INTERVAL ) ;
            continue ;
         }

         break ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_checkIndexAfterLock( INT32 lockType )
   {
      INT32 rc = SDB_OK ;

      rc = _mbLockAndCheck( lockType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         goto error ;
      }

      // make sure the index cb is still valid
      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _mbContext->mbUnlock() ;
      goto done ;
   }

   INT32 _dmsIndexBuilder::_checkInterrupt()
   {
      INT32 rc = SDB_OK ;

      // check scanner
      if ( ( NULL != _checker ) &&
           ( _checker->needInterrupt() ) )
      {
         PD_LOG( PDWARNING, "Scanner for building index [%s] on "
                 "collection [%s.%s] is interrupted", _indexCB->getName(),
                 _suData->getSuName(), _mbContext->mb()->_collectionName ) ;
         rc = SDB_DMS_SCANNER_INTERRUPT ;
         goto error ;
      }

      // check task
      if ( ( NULL != _pIdxStatus ) &&
           ( DMS_TASK_STATUS_CANCELED == _pIdxStatus->status() ) )
      {
         PD_LOG( PDWARNING, "Task for building index [%s] on "
                 "collection [%s.%s] has been canceled", _indexCB->getName(),
                 _suData->getSuName(), _mbContext->mb()->_collectionName ) ;
         rc = SDB_TASK_HAS_CANCELED ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_createScannerChecker()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL == _checker, "checker should not be valid" ) ;
      const CHAR *csName = _suData->getSuName() ;
      const CHAR *clShortName = _mbContext->mb()->_collectionName ;
      rc = sdbGetDMSCB()->createScannerChecker( _suData->logicalID(),
                                                _mbContext->clLID(),
                                                csName,
                                                clShortName,
                                                "build index",
                                                _eduCB,
                                                &_checker ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create scanner checker for "
                   "collection [%s.%s], rc: %d", csName, clShortName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _dmsIndexBuilder::_releaseScannerChecker()
   {
      if ( NULL != _checker )
      {
         sdbGetDMSCB()->releaseScannerChecker( _checker ) ;
         _checker = NULL ;
      }
   }


   _dmsIndexBuilder* _dmsIndexBuilder::createInstance( dmsSUDescriptor* su,
                                                       _dmsMBContext* mbContext,
                                                       _pmdEDUCB* eduCB,
                                                       dmsExtentID indexExtentID,
                                                       dmsExtentID indexLogicID,
                                                       INT32 sortBufferSize,
                                                       UINT16 indexType,
                                                       dmsIndexBuildGuardPtr &guardPtr,
                                                       IDmsOprHandler *pOprHandler,
                                                       utilWriteResult *pResult,
                                                       dmsDupKeyProcessor *dkProcessor,
                                                       dmsIdxTaskStatus* pIdxStatus )
   {
      _dmsIndexBuilder* builder = NULL ;

      SDB_ASSERT( su != NULL, "su can't be NULL" ) ;
      SDB_ASSERT( mbContext != NULL, "mbContext can't be NULL" ) ;
      SDB_ASSERT( eduCB != NULL, "eduCB can't be NULL" ) ;

      dmsStorageUnit *storageUnit = static_cast<dmsStorageUnit*>( su ) ;

      if ( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT, indexType ) )
      {
         builder = SDB_OSS_NEW _dmsIndexExtBuilder( storageUnit,
                                                    mbContext,
                                                    eduCB,
                                                    indexExtentID,
                                                    indexLogicID,
                                                    guardPtr,
                                                    dkProcessor ) ;
         if ( NULL == builder)
         {
            PD_LOG ( PDERROR, "failed to allocate _dmsIndexExtBuilder" ) ;
         }
      }
      else
      {
         PD_LOG ( PDDEBUG, "index sort buffer size: %dMB", sortBufferSize ) ;

         if ( sortBufferSize < 0 )
         {
            PD_LOG ( PDERROR, "invalid sort buffer size: %d", sortBufferSize ) ;
         }
         else if ( 0 == sortBufferSize || !( sdbGetDMSCB()->hasIxmKeySorterCreator() ) )
         {
            builder = SDB_OSS_NEW _dmsIndexOnlineBuilder( storageUnit,
                                                          mbContext,
                                                          eduCB,
                                                          indexExtentID,
                                                          indexLogicID,
                                                          guardPtr,
                                                          dkProcessor,
                                                          pIdxStatus ) ;
            if ( NULL == builder)
            {
               PD_LOG ( PDERROR, "failed to allocate _dmsIndexOnlineBuilder" ) ;
            }
         }
         else
         {
            builder = SDB_OSS_NEW _dmsIndexSortingBuilder( storageUnit,
                                                           mbContext,
                                                           eduCB,
                                                           indexExtentID,
                                                           indexLogicID,
                                                           sortBufferSize,
                                                           guardPtr,
                                                           dkProcessor,
                                                           pIdxStatus ) ;
            if ( NULL == builder)
            {
               PD_LOG ( PDERROR, "failed to allocate _dmsIndexSortingBuilder" ) ;
            }
         }

         if ( builder )
         {
            builder->setOprHandler( pOprHandler ) ;
            builder->setWriteResult( pResult ) ;
         }
      }

      return builder ;
   }

   void _dmsIndexBuilder::releaseInstance( _dmsIndexBuilder* builder )
   {
      SDB_ASSERT( builder != NULL, "builder can't be NULL" ) ;

      SDB_OSS_DEL builder ;
   }
}

