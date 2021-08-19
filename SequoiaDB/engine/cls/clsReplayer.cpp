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

   Source File Name = clsReplayer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsReplayer.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "dpsOp2Record.hpp"
#include "clsReplBucket.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtnLob.hpp"
#include "rtnAlter.hpp"
#include "utilCompressor.hpp"
#include "mthModifier.hpp"
#include "utilBsonHash.hpp"

using namespace bson ;

namespace engine
{

   #define CLS_REPLAY_CHECK_INTERVAL         ( 200 )  /// ms

   static BSONObj s_replayHint = BSON( "" << IXM_ID_KEY_NAME ) ;

   INT32 startIndexJob ( RTN_JOB_TYPE type,
                         const dpsLogRecordHeader *recordHeader,
                         _dpsLogWrapper *dpsCB,
                         BOOLEAN isRollBack ) ;

   // default pending count for duplicated key issue
   #define CLS_PARALLA_DEF_PENDING_COUNT     ( 1024 )
   // max pending count index ( power of 2 )
   #define CLS_PARALLA_MAX_PENDING_INDEX     ( 20 )

   /*
      _clsCLParallaInfo implement
   */
   _clsCLParallaInfo::_clsCLParallaInfo()
   {
      _parallaType = CLS_PARALLA_NULL ;
      _lastLSN = DPS_INVALID_LSN_OFFSET ;
      _pendingLSN = DPS_INVALID_LSN_OFFSET ;
      _pendingCount = 0 ;
      _pendingIndex = 0 ;
      _recParallaCount = 0 ;
   }

   _clsCLParallaInfo::~_clsCLParallaInfo()
   {
   }

   INT32 _clsCLParallaInfo::waitLastLSN( clsBucket *pBucket )
   {
      SDB_ASSERT( NULL != pBucket, "bucket is invalid" ) ;
      return pBucket->waitForLSN( _lastLSN ) ;
   }

   BOOLEAN _clsCLParallaInfo::checkParalla( UINT16 type,
                                            DPS_LSN_OFFSET curLSN,
                                            clsBucket *pBucket ) const
   {
      BOOLEAN canRecParalla = FALSE ;

      if ( _pendingCount == 0 &&
          ( LOG_TYPE_DATA_INSERT == type ||
            LOG_TYPE_DATA_UPDATE == type ||
            LOG_TYPE_DATA_DELETE == type ) )
      {
         canRecParalla = TRUE ;
      }

      return canRecParalla ;
   }

   void _clsCLParallaInfo::updateParalla( CLS_PARALLA_TYPE parallaType,
                                          UINT16 type,
                                          DPS_LSN_OFFSET curLSN )
   {
      _parallaType = parallaType ;
      _lastLSN = curLSN ;

      // resolve pending when current LSN is larger than pending LSN
      if ( _pendingCount > 0 &&
           DPS_INVALID_LSN_OFFSET != _pendingLSN &&
           DPS_INVALID_LSN_OFFSET != curLSN &&
           _pendingLSN <= curLSN )
      {
         -- _pendingCount ;
         if ( 0 == _pendingCount )
         {
            _pendingLSN = DPS_INVALID_LSN_OFFSET ;
#if defined(_DEBUG)
            PD_LOG( PDDEBUG, "Set collection %s not pending from LSN %llu",
                    _collection.c_str(), curLSN ) ;
#endif
         }
      }

      if ( CLS_PARALLA_REC == parallaType )
      {
         // will replay in record parallel mode
         // if the number of DPS records replayed in record parallel mode is
         // equal to last start pending count, reduce the pending index
         ++ _recParallaCount ;
         if ( _pendingIndex > 0 &&
              _recParallaCount > (UINT64)( ( CLS_PARALLA_DEF_PENDING_COUNT <<
                                             _pendingIndex ) ) )
         {
            -- _pendingIndex ;
         }
      }
      else
      {
         // will replay in collection parallel mode
         // reset record parallel count
         _recParallaCount = 0 ;
      }
   }

   CLS_PARALLA_TYPE _clsCLParallaInfo::getLastParallaType() const
   {
      return _parallaType ;
   }

   BOOLEAN _clsCLParallaInfo::isParallaTypeSwitch( CLS_PARALLA_TYPE type ) const
   {
      if ( CLS_PARALLA_NULL == _parallaType )
      {
         return FALSE ;
      }
      return _parallaType != type ? TRUE : FALSE ;
   }

   void _clsCLParallaInfo::setPending( DPS_LSN_OFFSET lsn )
   {
      if ( DPS_INVALID_LSN_OFFSET == _pendingLSN )
      {
         _pendingLSN = lsn ;
         _pendingCount = CLS_PARALLA_DEF_PENDING_COUNT << _pendingIndex ;
      }
      else if ( _pendingLSN < lsn )
      {
         // pending again
         // NOTE: might not happen during CL parallel mode
         _pendingLSN = lsn ;
         _pendingCount += CLS_PARALLA_DEF_PENDING_COUNT ;
      }
      else
      {
         // might rollbacked
         _pendingCount += CLS_PARALLA_DEF_PENDING_COUNT ;
      }
      if ( _pendingIndex < CLS_PARALLA_MAX_PENDING_INDEX )
      {
         ++ _pendingIndex ;
      }
#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "Set collection %s pending from LSN %llu count %u "
              "[ index %u ]", _collection.c_str(), _pendingLSN, _pendingCount,
              _pendingIndex ) ;
#endif
   }

   /*
      _clsReplayer implement
   */
   _clsReplayer::_clsReplayer( BOOLEAN useDps, BOOLEAN isReplSync )
   {
      _dmsCB = sdbGetDMSCB() ;
      _dpsCB = NULL ;
      if ( useDps )
      {
         _dpsCB = sdbGetDPSCB() ;
      }
      _monDBCB = pmdGetKRCB()->getMonDBCB () ;

      _isReplSync = isReplSync ;
   }

   _clsReplayer::~_clsReplayer()
   {

   }

   void _clsReplayer::enableDPS ()
   {
      _dpsCB = sdbGetDPSCB() ;
   }

   void _clsReplayer::disableDPS ()
   {
      _dpsCB = NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__CALCBUCKETID, "_clsReplayer::_calcBucketID" )
   INT32 _clsReplayer::_calcBucketID( dpsLogRecordHeader *recordHeader,
                                      clsBucket *pBucket,
                                      CLS_PARALLA_TYPE &parallaType,
                                      UINT32 &bucketID,
                                      UINT32 &clHash,
                                      utilCLUniqueID &clUniqueID,
                                      UINT32 &waitBucketID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__CALCBUCKETID ) ;

      SDB_ASSERT( NULL != recordHeader, "Record is invalid" ) ;
      SDB_ASSERT( NULL != pBucket, "bucket is invalid" ) ;

      bucketID = ~0 ;

      switch( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
         case LOG_TYPE_DATA_DELETE :
         case LOG_TYPE_DATA_UPDATE :
         {
            rc = _calcDataBucketID( recordHeader, pBucket, parallaType,
                                    bucketID, clHash, clUniqueID,
                                    waitBucketID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to calculate bucket ID for "
                         "DATA record type [%u] LSN [%llu], rc: %d",
                         recordHeader->_type, recordHeader->_lsn, rc ) ;
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         case LOG_TYPE_LOB_UPDATE :
         case LOG_TYPE_LOB_REMOVE :
         {
            parallaType = CLS_PARALLA_CL ;
            rc = _calcLobBucketID( recordHeader, pBucket, bucketID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to calculate bucket ID for "
                         "LOB record type [%u] LSN [%llu], rc: %d",
                         recordHeader->_type, recordHeader->_lsn, rc ) ;
            break ;
         }
         case LOG_TYPE_DUMMY :
            bucketID = 0 ;
            break ;
         default :
            break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREP__CALCBUCKETID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__CALCDATABUCKETID, "_clsReplayer::_calcDataBucketID" )
   INT32 _clsReplayer::_calcDataBucketID( dpsLogRecordHeader *recordHeader,
                                          clsBucket *pBucket,
                                          CLS_PARALLA_TYPE &parallaType,
                                          UINT32 &bucketID,
                                          UINT32 &clHash,
                                          utilCLUniqueID &clUniqueID,
                                          UINT32 &waitBucketID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__CALCDATABUCKETID ) ;

      const CHAR *fullname = NULL ;
      BSONObj curObj, waitObj ;

      switch( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
            parallaType = CLS_PARALLA_CL ;
            rc = dpsRecord2Insert( (CHAR *)recordHeader, &fullname, curObj ) ;
            break ;
         case LOG_TYPE_DATA_DELETE :
            parallaType = CLS_PARALLA_CL ;
            rc = dpsRecord2Delete( (CHAR *)recordHeader, &fullname, curObj ) ;
            break ;
         case LOG_TYPE_DATA_UPDATE :
         {
            BSONObj oldObj ;
            BSONObj modifier ;   //new change obj
            parallaType = CLS_PARALLA_CL ;
            rc = dpsRecord2Update( (CHAR *)recordHeader, &fullname,
                                   curObj, oldObj, waitObj, modifier, NULL,
                                   NULL, NULL, NULL ) ;
            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "Invalid DATA operator type" ) ;
            PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                      "Invalid DATA operator type [%u] LSN [%llu]",
                      recordHeader->_type, recordHeader->_lsn ) ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Parse dps log[type: %d, lsn: %lld, len: %d]"
                   "failed, rc: %d", recordHeader->_type,
                   recordHeader->_lsn, recordHeader->_length, rc ) ;

      if ( CLS_PARALLA_CL == parallaType )
      {
         rc = _checkCLParalla( recordHeader, pBucket, fullname, clUniqueID,
                               parallaType ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to check parallel for "
                      "collection [%s], rc: %d", fullname, rc ) ;
      }

      if ( CLS_PARALLA_REC == parallaType )
      {
         BSONElement idEle = curObj.getField( DMS_ID_KEY_NAME ) ;
         if ( !idEle.eoo() )
         {
            clHash = BSON_HASHER::hashStr( fullname ) ;
            UINT32 valueHash = BSON_HASHER::hash( idEle.value(),
                                                  idEle.valuesize() ) ;
            valueHash = BSON_HASHER::hashCombine( clHash, valueHash ) ;
            bucketID = pBucket->calcIndex( valueHash ) ;

            // check if have wait object ( UPDATE's new matcher )
            if ( waitObj.hasField( DMS_ID_KEY_NAME ) )
            {
               BSONElement waitEle = waitObj.getField( DMS_ID_KEY_NAME ) ;
               if ( 0 != waitEle.woCompare( idEle, FALSE ) )
               {
                  // not the same OID
                  UINT32 waitHash = BSON_HASHER::hash( waitEle.value(),
                                                       waitEle.valuesize() ) ;
                  waitHash = BSON_HASHER::hashCombine( clHash, waitHash ) ;
                  waitBucketID = pBucket->calcIndex( waitHash ) ;
               }
            }
         }
      }
      else if ( CLS_PARALLA_CL == parallaType )
      {
         clHash = BSON_HASHER::hashStr( fullname ) ;
         bucketID = pBucket->calcIndex( clHash ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREP__CALCDATABUCKETID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__CALCLOBBUCKETID, "_clsReplayer::_calcLobBucketID" )
   INT32 _clsReplayer::_calcLobBucketID( dpsLogRecordHeader *recordHeader,
                                         clsBucket *pBucket,
                                         UINT32 &bucketID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__CALCLOBBUCKETID ) ;

      const CHAR *fullname = NULL ;
      const OID *oid = NULL ;
      UINT32 sequence = 0 ;

      switch ( recordHeader->_type )
      {
         case LOG_TYPE_LOB_WRITE :
         {
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullname, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullname, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullname, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "Invalid LOB operator type" ) ;
            PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                      "Invalid LOB operator type [%u] LSN [%llu]",
                      recordHeader->_type, recordHeader->_lsn ) ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Parse dps log[type: %d, lsn: %lld, len: %d]"
                   "failed, rc: %d", recordHeader->_type,
                   recordHeader->_lsn, recordHeader->_length, rc ) ;

      if ( NULL != oid )
      {
         CHAR tmpData[ sizeof( *oid ) + sizeof( sequence ) ] = { 0 } ;
         ossMemcpy( tmpData, ( const CHAR * )( oid->getData()),
                    sizeof( *oid ) ) ;
         ossMemcpy( &tmpData[ sizeof( *oid ) ], ( const CHAR* )&sequence,
                    sizeof( sequence ) ) ;
         UINT32 valueHash = BSON_HASHER::hash( tmpData, sizeof( tmpData ) ) ;
         bucketID = pBucket->calcIndex( valueHash ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREP__CALCLOBBUCKETID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__CHECKCLPARALLA, "_clsReplayer::_checkCLParalla" )
   INT32 _clsReplayer::_checkCLParalla( dpsLogRecordHeader *recordHeader,
                                        clsBucket *pBucket,
                                        const CHAR *fullname,
                                        utilCLUniqueID &clUniqueID,
                                        CLS_PARALLA_TYPE &parallaType )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__CHECKCLPARALLA ) ;

      dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      const CHAR *pShortName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      clsCLParallaInfo *pInfo = NULL ;
      BOOLEAN noIDIndex = FALSE ;

      rc = rtnResolveCollectionNameAndLock( fullname, _dmsCB, &su, &pShortName,
                                            suID ) ;
      if ( SDB_OK != rc )
      {
         // ignore error
         rc = SDB_OK ;
         goto done ;
      }

      // Currently parallel replaying on capped collection is forbidden,
      // because we need to be sure the records are exactly the same with
      // the ones on primary node, including their positions.
      if ( DMS_STORAGE_CAPPED == su->type() )
      {
         goto done ;
      }

      rc = su->data()->getMBContext( &mbContext, pShortName, -1 ) ;
      if ( SDB_OK != rc )
      {
         // ignore error
         rc = SDB_OK ;
         goto done ;
      }

      // check unique ID of collection
      clUniqueID = mbContext->mb()->_clUniqueID ;
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         // do not have unique ID ( not upgrade yet ) or local unique ID,
         // could use record parallel replay in below condition
         // - has no more than 1 unique indexes ( number <= 1 )
         // - and has no text indexes
         PD_LOG( PDDEBUG, "Collection [%s] does not have unique ID",
                 fullname ) ;
         if ( ( LOG_TYPE_DATA_INSERT == recordHeader->_type ||
                LOG_TYPE_DATA_UPDATE == recordHeader->_type ||
                LOG_TYPE_DATA_DELETE == recordHeader->_type ) &&
                ( 0 == mbContext->mbStat()->_textIdxNum ) &&
                ( mbContext->mbStat()->_uniqueIdxNum <= 1 ) )
         {
            parallaType = CLS_PARALLA_REC ;
         }
         goto done ;
      }
      else
      {
         // collection have valid unique ID
         pInfo = pBucket->getOrCreateInfo( fullname,
                                           mbContext->mb()->_clUniqueID ) ;
         PD_CHECK( NULL != pInfo, SDB_OOM, error, PDERROR,
                   "Failed to get collection parallel information for "
                   "collection [%s]", fullname ) ;

         // For collection who has text indices, parallel replay should
         // also be forbidden. Otherwise, the records in the capped
         // collection will not be exactly the same.
         if ( 0 != mbContext->mbStat()->_textIdxNum )
         {
            /// can't upgrade to recParalla
         }
         else if ( pInfo->checkParalla( recordHeader->_type,
                                        recordHeader->_lsn,
                                        pBucket ) )
         {
            parallaType = CLS_PARALLA_REC ;
         }
      }

      // check if have $id index
      noIDIndex = OSS_BIT_TEST( mbContext->mb()->_attributes,
                                DMS_MB_ATTR_NOIDINDEX ) ? TRUE : FALSE ;

      su->data()->releaseMBContext( mbContext ) ;
      mbContext = NULL ;

      _dmsCB->suUnlock( suID, SHARED ) ;
      suID = DMS_INVALID_SUID ;
      su = NULL ;

      if ( NULL != pInfo &&
           pInfo->isParallaTypeSwitch( parallaType ) )
      {
         // if parallel type is changed, wait for last LSN of the same
         // collection
         rc = pInfo->waitLastLSN( pBucket ) ;
         if ( SDB_OK != rc )
         {
            INT32 bucketStatus = (INT32)pBucket->getStatus() ;
            PD_LOG( PDWARNING, "Failed to wait repl bucket to switch LSN, its "
                    "status[%s(%d)] is error",
                    clsGetReplBucketStatusDesp( bucketStatus ),
                    bucketStatus ) ;
            goto error ;
         }
      }

      if ( CLS_PARALLA_REC == parallaType )
      {
         // we need to check ID index
         // - record without ID index could not rollback, so we need to
         //   wait all previous records with ID index to be finished
         // - in another way, record with ID index could cause rollback
         //   so we need to wait all previous records without ID index to be
         //   finished
         if ( noIDIndex )
         {
            // without ID index case
            rc = pBucket->waitForIDLSNComp( recordHeader->_lsn ) ;
            if ( SDB_OK != rc )
            {
               INT32 bucketStatus = (INT32)pBucket->getStatus() ;
               PD_LOG( PDWARNING, "Failed to wait repl bucket to complement "
                       "the last record parallel mode with ID index, its "
                       "status[%s(%d)] is error",
                       clsGetReplBucketStatusDesp( bucketStatus ),
                       bucketStatus ) ;
               goto error ;
            }
         }
         else
         {
            // with ID index case
            rc = pBucket->waitForNIDLSNComp( recordHeader->_lsn ) ;
            if ( SDB_OK != rc )
            {
               INT32 bucketStatus = (INT32)pBucket->getStatus() ;
               PD_LOG( PDWARNING, "Failed to wait repl bucket to complement "
                       "the last record parallel mode without ID index, its "
                       "status[%s(%d)] is error",
                       clsGetReplBucketStatusDesp( bucketStatus ),
                       bucketStatus ) ;
               goto error ;
            }
         }
      }

      if ( NULL != pInfo )
      {
         /// update paralla info
         pInfo->updateParalla( parallaType, recordHeader->_type,
                               recordHeader->_lsn ) ;
      }

   done:
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         _dmsCB->suUnlock( suID, SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSREP__CHECKCLPARALLA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLYBUCKET, "_clsReplayer::replayByBucket" )
   INT32 _clsReplayer::replayByBucket( dpsLogRecordHeader *recordHeader,
                                       pmdEDUCB *eduCB, clsBucket *pBucket )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__CLSREP_REPLYBUCKET ) ;

      CLS_PARALLA_TYPE parallaType = CLS_PARALLA_NULL ;
      UINT32 bucketID = ~0 ;
      UINT32 clHash = 0 ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      UINT32 waitBucketID = ~0 ;

      SDB_ASSERT( NULL != recordHeader, "Record is invalid" ) ;
      SDB_ASSERT( NULL != pBucket, "bucket is invalid" ) ;

      try
      {
         rc = _calcBucketID( recordHeader, pBucket, parallaType, bucketID,
                             clHash, clUniqueID, waitBucketID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to calculate bucket ID, "
                      "rc: %d", rc ) ;

         if ( (UINT32)~0 != bucketID )
         {
            rc = pBucket->pushData( bucketID, (CHAR *)recordHeader,
                                    recordHeader->_length, parallaType, clHash,
                                    clUniqueID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to push log to bucket, rc: %d",
                         rc ) ;
            if ( (UINT32)~0 != waitBucketID && bucketID != waitBucketID )
            {
               PD_LOG( PDDEBUG, "Bucket [%u]: push wait for LSN [%llu] in "
                       "bucket [%u]", waitBucketID, recordHeader->_lsn,
                       bucketID ) ;
               rc = pBucket->pushWait( waitBucketID, recordHeader->_lsn ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to push wait to bucket, "
                            "rc: %d", rc ) ;
            }
         }
         else
         {
            // wait bucket all complete and check status
            rc = pBucket->waitEmptyWithCheck() ;
            if ( rc )
            {
               INT32 bucketStatus = (INT32)pBucket->getStatus() ;
               PD_LOG( PDWARNING, "Wait repl bucket empty failed, its "
                       "status[%s(%d)] is error",
                       clsGetReplBucketStatusDesp( bucketStatus ),
                       bucketStatus ) ;
               goto error ;
            }

            // judge lsn valid
            if ( !pBucket->_expectLSN.invalid() &&
                 0 != pBucket->_expectLSN.compareOffset( recordHeader->_lsn ) )
            {
               PD_LOG( PDWARNING, "Expect lsn[%lld], real complete lsn[%lld]",
                       recordHeader->_lsn, pBucket->_expectLSN.offset ) ;
               rc = SDB_CLS_REPLAY_LOG_FAILED ;
               goto error ;
            }

            pBucket->clearParallaInfo() ;

            // should not ignore duplicated keys on user indexes
            rc = replay( recordHeader, eduCB, TRUE, FALSE ) ;
            // re-calc complete lsn
            if ( SDB_OK == rc && !pBucket->_expectLSN.invalid() )
            {
               pBucket->_expectLSN.offset += recordHeader->_length ;
               pBucket->_expectLSN.version = recordHeader->_version ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "received unexcepted error when parsing inner bson "
                 "object dps repl log, error:%s", e.what() ) ;
         goto error ;
      }

   done:
      if ( SDB_OK != rc )
      {
         dpsLogRecord record ;
         CHAR tmpBuff[4096] = {0} ;
         INT32 rcTmp = record.load( (const CHAR*)recordHeader ) ;
         if ( SDB_OK == rcTmp )
         {
            record.dump( tmpBuff, sizeof(tmpBuff)-1, DPS_DMP_OPT_FORMATTED ) ;
         }
         PD_LOG( PDERROR, "sync bucket: replay log [type:%d, lsn:%lld, "
                 "data: %s] failed, rc: %d", recordHeader->_type,
                 recordHeader->_lsn, tmpBuff, rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREP_REPLYBUCKET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // for all UPDATE/DELETE, make sure we use hint = {"":"$id"}, so that we can
   // bypass expensive optimizer to improve performance
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLAY, "_clsReplayer::replay" )
   INT32 _clsReplayer::replay( dpsLogRecordHeader *recordHeader,
                               pmdEDUCB *eduCB,
                               BOOLEAN incMonCount,
                               BOOLEAN ignoreDupKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_REPLAY );
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;

      dpsTransCB *transCB = sdbGetTransCB() ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      BOOLEAN startedRollback = FALSE ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn ) ;
      }

      // check transaction rollback
      rc = dpsGetTransIDFromRecord( (CHAR *)recordHeader,
                                    transID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( DPS_INVALID_TRANS_ID != transID &&
           transCB->isRollback( transID ) )
      {
         eduCB->startTransRollback() ;
         startedRollback = TRUE ;
      }

      try
      {
      switch ( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
         {
            INT32 flag = 0 ;
            utilInsertResult insertResult ;
            const CHAR *fullname = NULL ;
            BSONObj obj ;
            rc = dpsRecord2Insert( (CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            if ( 0 == ossStrcmp( fullname, DMS_STAT_COLLECTION_CL_NAME ) ||
                 0 == ossStrcmp( fullname, DMS_STAT_INDEX_CL_NAME ) )
            {
               // for statistics tables, replace directly to avoid duplicated
               // key issue ( which might not be cleared by delayed or
               // interrupted async index tasks )
               OSS_BIT_SET( flag, FLG_INSERT_REPLACEONDUP ) ;
            }
            rc = rtnReplayInsert( fullname, obj, flag, eduCB, _dmsCB, _dpsCB,
                                  1, &insertResult ) ;
            if ( SDB_OK == rc && incMonCount )
            {
               _monDBCB->monOperationCountInc ( MON_INSERT_REPL ) ;
            }
            // ignore duplicated key in these case
            // 1. ignore duplicated key by caller
            // 2. conflict objects has the same OID
            else if ( SDB_IXM_DUP_KEY == rc &&
                      ( ignoreDupKey ||
                        insertResult.isSameID() ) )
            {
               PD_LOG( PDINFO, "Record[%s] already exist when insert",
                       obj.toPoolString().c_str() ) ;
               rc = SDB_OK ;
            }

            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            BSONObj match ;     //old match
            BSONObj oldObj ;
            BSONObj newMatch ;
            BSONObj modifier ;   //new change obj
            const CHAR *fullname = NULL ;
            utilUpdateResult upResult ;
            UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
            rc = dpsRecord2Update( (CHAR *)recordHeader,
                                   &fullname,
                                   match,
                                   oldObj,
                                   newMatch,
                                   modifier, NULL, NULL, NULL, &logWriteMod ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            /// possibly get a empty modifier
            if ( !modifier.isEmpty() )
            {
               rc = rtnUpdate( fullname, match, modifier,
                               s_replayHint, 0, eduCB, _dmsCB, _dpsCB, 1,
                               &upResult, NULL, logWriteMod ) ;
            }
            if ( SDB_OK == rc )
            {
               if ( upResult.updateNum() > 0 )
               {
                  if ( incMonCount )
                  {
                     _monDBCB->monOperationCountInc ( MON_UPDATE_REPL ) ;
                  }
               }
               else if ( _isReplSync )
               {
                  SDB_ASSERT( upResult.updateNum() > 0,
                              "Updated number must > 0" ) ;
               }
               else
               {
                  PD_LOG( PDDEBUG, "LSN [%llu] matcher %s updated 0 record on "
                          "collection [%s]", recordHeader->_lsn,
                          match.toPoolString().c_str(), fullname ) ;
               }
            }
            // ignore duplicated key in REPLACE case
            // 1. ignore duplicated key by caller
            // 2. conflict objects has the same OID
            else if ( SDB_IXM_DUP_KEY == rc &&
                      DMS_LOG_WRITE_MOD_FULL == logWriteMod &&
                      ( ignoreDupKey || upResult.isSameID() ) )
            {
               PD_LOG( PDINFO, "Record[%s] already exist when update",
                       match.toPoolString().c_str() ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            const CHAR *fullname = NULL ;
            BSONObj obj ;
            utilDeleteResult delResult ;
            rc = dpsRecord2Delete( (CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            else
            {
               BSONElement idEle = obj.getField( DMS_ID_KEY_NAME ) ;
               if ( idEle.eoo() )
               {
                  PD_LOG( PDWARNING, "replay: failed to parse "
                          "oid from bson:[%s]",obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               {
                  BSONObjBuilder selectorBuilder ;
                  selectorBuilder.append( idEle ) ;
                  BSONObj selector = selectorBuilder.obj() ;
                  rc = rtnDelete( fullname, selector, s_replayHint, 0, eduCB,
                                  _dmsCB, _dpsCB, 1, &delResult ) ;
               }
            }
            if ( SDB_OK == rc )
            {
               if ( delResult.deletedNum() > 0 )
               {
                  if ( incMonCount )
                  {
                     _monDBCB->monOperationCountInc ( MON_DELETE_REPL ) ;
                  }
               }
               else if ( _isReplSync )
               {
                  SDB_ASSERT( delResult.deletedNum() > 0,
                              "Deleted number must > 0" ) ;
               }
               else
               {
                  PD_LOG( PDDEBUG, "LSN [%llu] matcher %s deleted 0 record on "
                          "collection [%s]", recordHeader->_lsn,
                          obj.toPoolString().c_str(), fullname ) ;
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_POP:
         {
            const CHAR *fullName = NULL ;
            INT64 logicalID = 0 ;
            INT8 direction = 1 ;
            rc = dpsRecord2Pop( (CHAR *)recordHeader, &fullName,
                                logicalID, direction );
            if ( rc )
            {
               goto error ;
            }

            rc = rtnPopCommand( fullName, logicalID, eduCB, _dmsCB,
                                _dpsCB, direction ) ;
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG( PDERROR, "Logical id[%lld] is invalid when pop from "
                       "collection[%s]", logicalID, fullName ) ;
               rc = SDB_OK ;
            }
            else if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               PD_LOG( PDERROR, "Collection space not exist when pop from "
                       "collection[%s]", fullName ) ;
               rc = SDB_OK ;
            }
            else if ( SDB_DMS_NOTEXIST == rc )
            {
               PD_LOG( PDERROR, "Collection[%s] not exist when pop",
                       fullName ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CS_CRT :
         {
            const CHAR *cs = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            INT32 pageSize = 0 ;
            INT32 lobPageSize = 0 ;
            INT32 type = 0 ;
            rc = dpsRecord2CSCrt( (CHAR *)recordHeader, &cs, csUniqueID,
                                  pageSize, lobPageSize, type ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( type < DMS_STORAGE_NORMAL || type >= DMS_STORAGE_DUMMY )
            {
               PD_LOG( PDERROR, "Invalid cs type[%c] in replica log", type ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            rc = rtnCreateCollectionSpaceCommand( cs, eduCB, _dmsCB, _dpsCB,
                                                  csUniqueID,
                                                  pageSize, lobPageSize,
                                                  (DMS_STORAGE_TYPE)type ) ;
            if ( SDB_DMS_CS_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection space[%s] already exist when "
                       "create", cs ) ;

               rc = _dmsCB->changeUniqueID( cs, csUniqueID, BSONObj(),
                                            eduCB, _dpsCB ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR,
                          "Fail to change cs[%s] unique id, rc: %d", cs, rc ) ;
               }
            }
            break ;
         }
         case LOG_TYPE_CS_DELETE :
         {
            const CHAR *cs = NULL ;
            rc = dpsRecord2CSDel( (CHAR *)recordHeader,
                                  &cs ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            while ( TRUE )
            {
               rc = rtnDropCollectionSpaceCommand( cs, eduCB, _dmsCB, _dpsCB,
                                                   TRUE ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep ( 100 ) ;
                  continue ;
               }
               break ;
            }
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection space[%s] not exist when "
                       "drop", cs ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CL_CRT :
         {
            const CHAR *cl = NULL ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
            UINT32 attribute = 0 ;
            UINT8 compType = UTIL_COMPRESSOR_INVALID ;
            BSONObj extOptions ;
            string cs ;
            utilCSUniqueID csUniqID = UTIL_UNIQUEID_NULL ;

            rc = dpsRecord2CLCrt( (CHAR *)recordHeader, &cl, clUniqueID,
                                  attribute, compType, extOptions ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            cs = dmsGetCSNameFromFullName( cl ) ;
            csUniqID = utilGetCSUniqueID( clUniqueID ) ;

            rc = rtnCreateCollectionCommand( cl, attribute, eduCB, _dmsCB,
                                             _dpsCB, clUniqueID,
                                             (UTIL_COMPRESSOR_TYPE)compType,
                                             0, TRUE,
                                             ( extOptions.isEmpty() ?
                                               NULL : &extOptions ) ) ;
            if ( SDB_DMS_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection [%s] already exist when "
                       "create", cl ) ;

               BSONArrayBuilder clArr ;
               clArr << BSON( FIELD_NAME_NAME <<
                              dmsGetCLShortNameFromFullName( cl ).c_str() <<
                              FIELD_NAME_UNIQUEID <<
                              (INT64)clUniqueID ) ;
               rc = _dmsCB->changeUniqueID( cs.c_str(), csUniqID, clArr.arr(),
                                            eduCB, _dpsCB ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Fail to change cl[%s] unique id, rc: %d",
                          cl, rc ) ;
                  goto error ;
               }

            }
            break ;
         }
         case LOG_TYPE_CL_DELETE :
         {
            const CHAR *cl = NULL ;
            rc = dpsRecord2CLDel( (CHAR *)recordHeader,
                                   &cl ) ;
            rc = rtnDropCollectionCommand( cl, eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection [%s] not exist when drop", cl ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_IX_CRT :
         {
            /// rebuild the index can be very time-consuming.
            /// we create a sub thread to handle it.
            startIndexJob ( RTN_JOB_CREATE_INDEX, recordHeader,
                            _dpsCB, FALSE ) ;
            break ;
         }
         case LOG_TYPE_IX_DELETE :
         {
            startIndexJob ( RTN_JOB_DROP_INDEX, recordHeader,
                            _dpsCB, FALSE ) ;
            break ;
         }
         case LOG_TYPE_CL_RENAME :
         {
            const CHAR *cs = NULL ;
            const CHAR *oldCl = NULL ;
            const CHAR *newCl = NULL ;
            rc = dpsRecord2CLRename( (CHAR *)recordHeader,
                                      &cs, &oldCl, &newCl ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnRenameCollectionCommand( cs, oldCl, newCl,
                                             eduCB, _dmsCB, _dpsCB, FALSE ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               INT32 rcTmp = SDB_OK ;
               CHAR newCLFullName [ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
               ossSnprintf( newCLFullName, sizeof( newCLFullName ),
                            "%s.%s", cs, newCl ) ;
               rcTmp = rtnTestCollectionCommand( newCLFullName, _dmsCB ) ;
               if ( SDB_OK == rcTmp )
               {
                  /// When old cl doesn't exist, but new cl has already exist,
                  /// we should ignore error
                  rc = SDB_OK ;
                  PD_LOG( PDWARNING, "Rename cl[%s.%s] to [%s.%s], old"
                          " cl not exist, new cl exist, ignore error.",
                          cs, oldCl, cs, newCl ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "Failed to rename cl[%s.%s] to [%s.%s], "
                          "rc: %d, test new cl rc: %d",
                          cs, oldCl, cs, newCl, rc, rcTmp ) ;
               }
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to rename cl[%s.%s] to [%s.%s], rc: %d",
                       cs, oldCl, cs, newCl, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CS_RENAME :
         {
            const CHAR *oldName = NULL ;
            const CHAR *newName = NULL ;
            rc = dpsRecord2CSRename( (const CHAR *)recordHeader,
                                     &oldName,
                                     &newName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
#ifdef _WINDOWS
            while ( TRUE )
            {
               rc = rtnRenameCollectionSpaceCommand( oldName, newName,
                                                     eduCB, _dmsCB, _dpsCB,
                                                     FALSE ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep ( 100 ) ;
                  continue ;
               }
               break ;
            }
#else
            rc = rtnRenameCollectionSpaceCommand( oldName, newName,
                                                  eduCB, _dmsCB, _dpsCB,
                                                  FALSE ) ;
#endif
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               INT32 rcTmp = SDB_OK ;
               rcTmp = rtnTestCollectionSpaceCommand( newName, _dmsCB ) ;
               if ( SDB_OK == rcTmp )
               {
                  /// When old cs doesn't exist, but new cs has already exist,
                  /// we should ignore error
                  rc = SDB_OK ;
                  PD_LOG( PDWARNING, "Replay log: rename cs[%s] to [%s], old"
                          " cs not exist, new cs exist, ignore error.",
                          oldName, newName ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "Failed to rename cs[%s] to [%s], rc: %d, "
                          "test new cs rc: %d", oldName, newName, rc, rcTmp ) ;
               }
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to rename cs[%s] to [%s], rc: %d",
                       oldName, newName, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            const CHAR *clname = NULL ;
            rc = dpsRecord2CLTrunc( (const CHAR *)recordHeader, &clname ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            // truncate will reset index flag of dropping indexes,
            // so we need to wait for collection jobs ( for drop indexes )
            while ( rtnGetIndexJobHolder()->hasCLJob( clname ) )
            {
               ossSleep( CLS_REPLAY_CHECK_INTERVAL ) ;
            }
            rc = rtnTruncCollectionCommand( clname, eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to truncate collection[%s], rc: %d",
                       clname, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_INVALIDATE_CATA :
         {
            UINT8 type = 0 ;
            const CHAR *csName = NULL ;
            const CHAR *clFullName = NULL ;
            const CHAR *ixName = NULL ;

            rc = dpsRecord2InvalidCata( (CHAR *)recordHeader,
                                        type,
                                        &clFullName,
                                        &ixName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            // Check if only contains name of collection space
            if ( NULL != clFullName &&
                 NULL == ossStrchr( clFullName, '.' ) )
            {
               csName = clFullName ;
               clFullName = NULL ;
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_STAT ) )
            {
               rtnAnalyzeParam param ;
               param._mode = SDB_ANALYZE_MODE_CLEAR ;
               param._needCheck = FALSE ;

               if ( csName && 0 == ossStrcmp( csName, "SYS" ) )
               {
                  // Reload all statistics
                  csName = NULL ;
               }

               rtnAnalyze( csName, clFullName, ixName, param,
                           eduCB, _dmsCB, NULL, NULL ) ;
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_CATA ) )
            {
               catAgent *pCatAgent = NULL ;

               /// when sdbrestore, the shardCB is NULL
               if ( sdbGetShardCB() &&
                    NULL != ( pCatAgent = sdbGetShardCB()->getCataAgent() ) )
               {
                  pCatAgent->lock_w() ;
                  if ( NULL != clFullName )
                  {
                     pCatAgent->clear( clFullName ) ;
                  }
                  else if ( NULL != csName )
                  {
                     pCatAgent->clearBySpaceName( csName, NULL, NULL ) ;
                  }
                  else
                  {
                     PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
                     pCatAgent->release_w() ;
                     rc = SDB_SYS ;
                     goto error ;
                  }
                  pCatAgent->release_w() ;
               }
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_PLAN ) )
            {
               SDB_RTNCB * rtnCB = sdbGetRTNCB() ;

               if ( NULL != clFullName )
               {
                  rtnCB->getAPM()->invalidateCLPlans( clFullName ) ;
               }
               else if ( NULL != csName )
               {
                  rtnCB->getAPM()->invalidateSUPlans( csName ) ;
               }
               else
               {
                  rtnCB->getAPM()->invalidateAllPlans() ;
               }
            }

            rc = SDB_OK ;
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnWriteLob( fullName, *oid, sequence,
                              offset, len, data, eduCB,
                              1, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_EXISTS == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnRemoveLobPiece( fullName, *oid,
                                    sequence, eduCB,
                                    1, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnUpdateLob( fullName, *oid, sequence,
                               offset, len, data, eduCB,
                               1, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
               goto error ;
            }

            break ;
         }
         case LOG_TYPE_ALTER :
         {
            const CHAR * objectName = NULL ;
            RTN_ALTER_OBJECT_TYPE objectType = RTN_ALTER_INVALID_OBJECT ;
            BSONObj alterObject ;

            rc = dpsRecord2Alter( (CHAR *)recordHeader, &objectName,
                                  (INT32 &)objectType, alterObject ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            while ( TRUE )
            {
               rc = rtnAlterCommand( objectName, objectType, alterObject,
                                     eduCB, _dpsCB ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep( 100 ) ;
                  continue ;
               }
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to alter object, rc: %d", rc ) ;

            break ;
         }
         case LOG_TYPE_ADDUNIQUEID :
         {
            const CHAR * csname = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj ;

            rc = dpsRecord2AddUniqueID( (CHAR *)recordHeader, &csname,
                                         csUniqueID, clInfoObj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnChangeUniqueID( csname, csUniqueID, clInfoObj,
                                    eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               PD_LOG( PDWARNING, "Collection space[%s] not exist "
                       "when add unique id ", csname ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to add unique id, rc: %d", rc ) ;

            break ;
         }
         case LOG_TYPE_DUMMY :
         {
            rc = SDB_OK ;
            break ;
         }
         case LOG_TYPE_TS_COMMIT :
         {
            rc = SDB_OK ;
            break ;
         }

         case LOG_TYPE_TS_ROLLBACK :
         {
            rc = SDB_OK ;
            break ;
         }
         default :
         {
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            PD_LOG( PDWARNING, "unexpected log type: %d",
                    recordHeader->_type ) ;
            break ;
         }
      }
      }
      catch ( std::exception &e )
      {
         rc = SDB_CLS_REPLAY_LOG_FAILED ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( startedRollback )
      {
         eduCB->stopTransRollback() ;
      }
      eduCB->resetLsn() ;
      if ( SDB_OK != rc )
      {
         ftReportErr( rc ) ;

         dpsLogRecord record ;
         CHAR tmpBuff[4096] = {0} ;
         INT32 rcTmp = record.load( (const CHAR*)recordHeader ) ;
         if ( SDB_OK == rcTmp )
         {
            record.dump( tmpBuff, sizeof(tmpBuff)-1, DPS_DMP_OPT_FORMATTED ) ;
         }
         PD_LOG( PDERROR, "sync: replay log [type:%d, lsn:%lld, data: %s] "
                 "failed, rc: %d", recordHeader->_type, recordHeader->_lsn,
                 tmpBuff, rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREP_REPLAY, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplayer::rollbackTrans( const dpsLogRecordHeader *recordHeader,
                                      pmdEDUCB *eduCB,
                                      MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;
      MAP_TRANS_PENDING_OBJ::iterator it ;
      INT64 contextID = -1 ;
      BOOLEAN needReport = TRUE ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn, TRUE ) ;
      }

      if ( LOG_TYPE_DATA_INSERT == recordHeader->_type )
      {
         rc = _rollbackTransInsert( recordHeader, eduCB, mapPendingObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback insert record of "
                      "LSN [%llu], rc: %d", recordHeader->_lsn, rc ) ;
      }
      else if ( LOG_TYPE_DATA_UPDATE == recordHeader->_type )
      {
         rc = _rollbackTransUpdate( recordHeader, eduCB, mapPendingObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback update record of "
                      "LSN [%llu], rc: %d", recordHeader->_lsn, rc ) ;
      }
      else if ( LOG_TYPE_DATA_DELETE == recordHeader->_type )
      {
         rc = _rollbackTransDelete( recordHeader, eduCB, mapPendingObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback delete record of "
                      "LSN [%llu], rc: %d", recordHeader->_lsn, rc ) ;
      }
      else
      {
         needReport = FALSE ;
         rc = rollback( recordHeader, eduCB ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         sdbGetRTNCB()->contextDelete( contextID, eduCB ) ;
      }
      return rc ;
   error:
      if ( needReport )
      {
         ftReportErr( rc ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_ROLBCK, "_clsReplayer::rollback" )
   INT32 _clsReplayer::rollback( const dpsLogRecordHeader *recordHeader,
                                 _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_ROLBCK );
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn, TRUE ) ;
      }

      try
      {
         switch ( recordHeader->_type )
         {
         case LOG_TYPE_DATA_INSERT :
         {
            BSONObj obj ;
            const CHAR *fullname = NULL ;
            utilDeleteResult delResult ;

            rc = dpsRecord2Insert( (const CHAR *)recordHeader,
                                   &fullname, obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            {
               BSONElement idEle = obj.getField( DMS_ID_KEY_NAME ) ;
               if ( idEle.eoo() )
               {
                  PD_LOG( PDWARNING, "replay: failed to parse"
                          " oid from bson:[%s]",obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               {
                  BSONObjBuilder selectorBuilder ;
                  selectorBuilder.append( idEle ) ;
                  BSONObj selector = selectorBuilder.obj() ;
                  rc = rtnDelete( fullname, selector, s_replayHint, 0, eduCB,
                                  _dmsCB, _dpsCB, 1, &delResult ) ;
               }

               if ( SDB_OK == rc )
               {
                  if ( delResult.deletedNum() == 0 )
                  {
                     PD_LOG( PDWARNING, "Rollback insert record of LSN [%llu], "
                             "no record is rollbacked", recordHeader->_lsn ) ;
                  }
                  else if ( delResult.deletedNum() > 1 )
                  {
                     PD_LOG( PDWARNING, "Rollback insert record of LSN [%llu], "
                             "more than one record [%lld] are rollbacked",
                             recordHeader->_lsn, delResult.deletedNum() ) ;
                  }
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            const CHAR *fullname = NULL ;
            BSONObj oldMatch ;
            BSONObj modifier ;  //old modifier
            BSONObj newMatch ;     //new matcher
            BSONObj newObj ;
            UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
            utilUpdateResult upResult ;

            rc = dpsRecord2Update( (const CHAR *)recordHeader,
                                    &fullname,
                                    oldMatch,
                                    modifier,
                                    newMatch,
                                    newObj, NULL, NULL, NULL, &logWriteMod ) ;
            if ( !modifier.isEmpty() )
            {
               rc = rtnUpdate( fullname, newMatch, modifier, s_replayHint,
                               0, eduCB, _dmsCB, _dpsCB, 1, NULL,
                               NULL, logWriteMod ) ;
               if ( SDB_OK == rc )
               {
                  if ( upResult.updateNum() == 0 )
                  {
                     PD_LOG( PDWARNING, "Rollback update record of LSN [%llu], "
                             "no record is rollbacked", recordHeader->_lsn ) ;
                  }
                  else if ( upResult.updateNum() > 1 )
                  {
                     PD_LOG( PDWARNING, "Rollback update record of LSN [%llu], "
                             "more than one record [%lld] are rollbacked",
                             recordHeader->_lsn, upResult.updateNum() ) ;
                  }
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            BSONObj obj ;
            const CHAR *fullname = NULL ;
            rc = dpsRecord2Delete( (const CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnInsert( fullname, obj, 1, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
            if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG( PDINFO, "Record[%s] exist when rollback delete",
                       obj.toString().c_str() ) ;
#ifdef _DEBUG
               SDB_ASSERT ( (rc == SDB_OK),
                            "Rollback should not hit dup key" );
#endif
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CS_CRT :
         {
            const CHAR *cs = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            INT32 pageSize = 0 ;
            INT32 lobPageSize = 0 ;
            INT32 type = 0 ;
            rc = dpsRecord2CSCrt( (const CHAR *)recordHeader, &cs, csUniqueID,
                                  pageSize, lobPageSize, type ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnDropCollectionSpaceCommand( cs, eduCB, _dmsCB,
                                                _dpsCB, TRUE ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               PD_LOG( PDWARNING, "Collection space[%s] not exist when "
                       "rollback create cs", cs ) ;
            }
            break ;
         }
         case LOG_TYPE_CS_DELETE :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_CL_CRT :
         {
            const CHAR *fullname = NULL ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
            UINT32 attribute = 0 ;
            UINT8 compType = UTIL_COMPRESSOR_INVALID ;
            BSONObj extOptions ;
            rc = dpsRecord2CLCrt( (const CHAR *)recordHeader,
                                  &fullname, clUniqueID,
                                  attribute, compType, extOptions ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnDropCollectionCommand( fullname, eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               PD_LOG( PDWARNING, "Collection[%s] not exist when rollback "
                       "create cl", fullname ) ;
            }
            break ;
         }
         case LOG_TYPE_CL_DELETE :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_IX_CRT :
         {
            startIndexJob ( RTN_JOB_DROP_INDEX, recordHeader,
                            _dpsCB, TRUE ) ;
            break ;
         }
         case LOG_TYPE_IX_DELETE :
         {
            startIndexJob ( RTN_JOB_CREATE_INDEX, recordHeader,
                            _dpsCB, TRUE ) ;
            break ;
         }
         case LOG_TYPE_CL_RENAME :
         {
            const CHAR *cs = NULL ;
            const CHAR *oldCl = NULL ;
            const CHAR *newCl = NULL ;
            rc = dpsRecord2CLRename( (CHAR *)recordHeader,
                                      &cs, &oldCl, &newCl ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnRenameCollectionCommand( cs, newCl, oldCl,
                                             eduCB, _dmsCB, _dpsCB, FALSE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename cs[%s] cl %s to %s, rc: %d",
                       cs, oldCl, newCl, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CS_RENAME :
         {
            const CHAR *oldName = NULL ;
            const CHAR *newName = NULL ;
            rc = dpsRecord2CSRename( (const CHAR *)recordHeader,
                                     &oldName,
                                     &newName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnRenameCollectionSpaceCommand( newName, oldName,
                                                  eduCB, _dmsCB, _dpsCB,
                                                  FALSE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename %s to %s, rc: %d",
                       oldName, newName, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_ALTER :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_ADDUNIQUEID :
         {
            const CHAR * csname = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj, emptyObj ;

            rc = dpsRecord2AddUniqueID( (CHAR *)recordHeader, &csname,
                                         csUniqueID, clInfoObj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnChangeUniqueID( csname, UTIL_UNIQUEID_NULL,
                                    utilSetUniqueID( clInfoObj ),
                                    eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               PD_LOG( PDWARNING, "Collection space[%s] not exist "
                       "when add unique id ", csname ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to add unique id, rc: %d", rc ) ;

            break ;
         }
         case LOG_TYPE_TS_COMMIT :
         case LOG_TYPE_TS_ROLLBACK :
         case LOG_TYPE_DUMMY :
         case LOG_TYPE_INVALIDATE_CATA :
         {
            rc = SDB_OK ;
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnRemoveLobPiece( fullName, *oid, sequence,
                                    eduCB, 1, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnUpdateLob( fullName, *oid, sequence,
                               offset, oldLen, oldData, eduCB,
                               1, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnWriteLob( fullName, *oid, sequence,
                              offset, len, data, eduCB,
                              1, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_EXISTS == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_POP:
         {
            // Pop is not able to rollback, return error.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         default :
         {
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            PD_LOG( PDWARNING, "unexpected log type: %d",
                    recordHeader->_type ) ;
            break ;
         }
         }
      }
      catch ( std::exception &e )
      {
         /// reuse error code.
         rc = SDB_CLS_REPLAY_LOG_FAILED ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      eduCB->resetLsn() ;
      if ( SDB_OK != rc )
      {
         ftReportErr( rc ) ;

         dpsLogRecord record ;
         CHAR tmpBuff[4096] = {0} ;
         INT32 rcTmp = record.load( (const CHAR*)recordHeader ) ;
         if ( SDB_OK == rcTmp )
         {
            record.dump( tmpBuff, sizeof(tmpBuff)-1, DPS_DMP_OPT_FORMATTED ) ;
         }
         PD_LOG( PDERROR, "sync: rollback log [type:%d, lsn:%lld, data: %s] "
                 "failed, rc: %d", recordHeader->_type, recordHeader->_lsn,
                 tmpBuff, rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREP_ROLBCK, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplayer::replayCrtCS( const CHAR *cs, utilCSUniqueID csUniqueID,
                                    INT32 pageSize,
                                    INT32 lobPageSize, DMS_STORAGE_TYPE type,
                                    _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != cs, "cs should not be NULL" ) ;
      INT32 rc = SDB_OK ;

      rc = rtnTestCollectionSpaceCommand( cs, _dmsCB, &csUniqueID ) ;

      if ( SDB_DMS_CS_REMAIN == rc )
      {
         rc = rtnDropCollectionSpaceCommand( cs, eduCB,
                                             _dmsCB, _dpsCB ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
         }
         if ( SDB_DMS_CS_NOTEXIST != rc )
         {
            PD_LOG( PDERROR,
                    "Drop cs[%s] before create cs failed, rc: %d",
                    cs, rc ) ;
         }
      }
      else if ( SDB_DMS_CS_UNIQUEID_CONFLICT == rc )
      {
         // try to drop cs if it is empty cs
         rc = _dmsCB->dropEmptyCollectionSpace( cs, eduCB, _dpsCB ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Drop emtpy collection space[%s]", cs ) ;
            rc = SDB_DMS_CS_NOTEXIST ;
         }
         else if ( SDB_DMS_CS_NOT_EMPTY != rc )
         {
            PD_LOG( PDWARNING,
                    "Try to drop collection space[%s] failed, rc: %d",
                    cs, rc ) ;
         }
      }

      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionSpaceCommand( cs, eduCB, _dmsCB,
                                               _dpsCB, csUniqueID, pageSize,
                                               lobPageSize, type, TRUE ) ;
      }

      if ( rc )
      {
         ftReportErr( rc ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayCrtCollection( const CHAR *collection,
                                            utilCLUniqueID clUniqueID,
                                            UINT32 attributes,
                                            _pmdEDUCB *eduCB,
                                            UTIL_COMPRESSOR_TYPE compType,
                                            const BSONObj *extOptions )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      INT32 rc = SDB_OK ;

      rc = rtnTestCollectionCommand( collection, _dmsCB, &clUniqueID ) ;

      if ( SDB_DMS_REMAIN == rc )
      {
         rc = rtnDropCollectionCommand( collection, eduCB, _dmsCB, _dpsCB ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         if ( SDB_DMS_NOTEXIST != rc )
         {
            PD_LOG( PDERROR,
                    "Drop cl[%s] before create cl failed, rc: %d",
                    collection, rc ) ;
         }
      }

      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionCommand( collection, attributes, eduCB,
                                          _dmsCB, _dpsCB, clUniqueID, compType,
                                          0, TRUE, extOptions ) ;
      }

      if ( rc )
      {
         ftReportErr( rc ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayIXCrt( const CHAR *collection,
                                    BSONObj &index,
                                    _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      INT32 rc = rtnCreateIndexCommand( collection, index, eduCB,
                                        _dmsCB, _dpsCB, TRUE ) ;
      if ( rc )
      {
         ftReportErr( rc ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayInsert( const CHAR *collection,
                                     BSONObj &obj,
                                     _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      INT32 rc = rtnReplayInsert( collection, obj, FLG_INSERT_CONTONDUP, eduCB,
                                  _dmsCB, _dpsCB ) ;
      if ( rc )
      {
         ftReportErr( rc ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayWriteLob( const CHAR *fullName,
                                       const bson::OID &oid,
                                       UINT32 sequence,
                                       UINT32 offset,
                                       UINT32 len,
                                       const CHAR *data,
                                       _pmdEDUCB *eduCB )
   {
      INT32 rc = rtnWriteLob( fullName, oid, sequence,
                              offset, len, data, eduCB,
                              1, _dpsCB ) ;
      if ( rc )
      {
         ftReportErr( rc ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__ROLEBACKTRANSINSERT, "_clsReplayer::_rollbackTransInsert" )
   INT32 _clsReplayer::_rollbackTransInsert(
                                       const dpsLogRecordHeader *recordHeader,
                                       pmdEDUCB *eduCB,
                                       MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__ROLEBACKTRANSINSERT ) ;

      SDB_ASSERT( NULL != recordHeader, "record header is invalid" ) ;
      SDB_ASSERT( LOG_TYPE_DATA_INSERT == recordHeader->_type,
                  "should be INSERT log" ) ;

      BSONObj insertObject, deleteSelector ;
      const CHAR *clFullName = NULL ;
      BOOLEAN pendingRemoved = FALSE ;
      dpsTransPendingKey pendingKey ;
      dpsTransPendingValue pendingValue ;

      rc = dpsRecord2Insert( (const CHAR *)recordHeader, &clFullName,
                             insertObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get insert record of LSN [%llu], "
                   "rc: %d", recordHeader->_lsn, rc ) ;

      // no need to copy, used for find
      pendingKey.setKey( clFullName, insertObject, FALSE ) ;

      rc = dpsRemoveTransPending( mapPendingObj, pendingKey, NULL,
                                  &pendingValue, pendingRemoved ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove transaction rollback "
                   "pending object, rc: %d", rc ) ;

      if ( pendingRemoved )
      {
         // pending object is removed, means duplicated key issue happened
         // with earlier rollback records
         if ( LOG_TYPE_DATA_DELETE != pendingValue._opType )
         {
            // if the corresponding record still exists, delete it
            // NOTE: OID might be changed by subsequent records in this
            //       transaction, use the saved OID (current OID on disk)
            //       rather than OID from record
            deleteSelector = pendingValue._obj ;
         }
         else
         {
            // if the corresponding record doesn't exist, ignore it
            // NOTE: record is not inserted by earlier rollback records
            deleteSelector = BSONObj() ;
         }

         if ( mapPendingObj.empty() )
         {
            // no pending object, clear rollback pending flag
            eduCB->clearTransRBPending() ;
         }
      }
      else
      {
         // no pending object is found, do the rollback DELETE
         BSONElement idElement = insertObject.getField( DMS_ID_KEY_NAME ) ;
         PD_CHECK( EOO != idElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to find OID from BSON [%s]",
                   insertObject.toPoolString().c_str() ) ;
         deleteSelector = BSON( DMS_ID_KEY_NAME << idElement ) ;
      }

      /// delete disk
      if ( !deleteSelector.isEmpty() )
      {
         utilDeleteResult delResult ;
         rc = rtnDelete( clFullName, deleteSelector, s_replayHint, 0, eduCB,
                         _dmsCB, _dpsCB, 1, &delResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete record to rollback "
                      "LSN [%llu], rc: %d", recordHeader->_lsn, rc ) ;

         if ( delResult.deletedNum() == 0 )
         {
            PD_LOG( PDWARNING, "Rollback insert record of LSN [%llu], "
                    "no record is rollbacked", recordHeader->_lsn ) ;
         }
         else if ( delResult.deletedNum() > 1 )
         {
            PD_LOG( PDWARNING, "Rollback insert record of LSN [%llu], "
                    "more than one record [%lld] are rollbacked",
                    recordHeader->_lsn, delResult.deletedNum() ) ;
         }
      }
      else if ( pendingRemoved )
      {
         if ( DPS_INVALID_LSN_OFFSET == eduCB->getCurTransLsn() )
         {
            SDB_ASSERT( mapPendingObj.empty(), "pending map should be emtpy" ) ;
            SDB_ASSERT( !eduCB->isTransRBPending(),
                        "transaction should not be rollback pending" ) ;

            rc = _logTransRollback( eduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to log rollback, rc: %d", rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__ROLEBACKTRANSINSERT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__ROLEBACKTRANSUPDATE, "_clsReplayer::_rollbackTransUpdate" )
   INT32 _clsReplayer::_rollbackTransUpdate(
                                       const dpsLogRecordHeader *recordHeader,
                                       pmdEDUCB *eduCB,
                                       MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__ROLEBACKTRANSUPDATE ) ;

      SDB_ASSERT( NULL != recordHeader, "record header is invalid" ) ;
      SDB_ASSERT( LOG_TYPE_DATA_UPDATE == recordHeader->_type,
                  "should be UPDATE log" ) ;

      const CHAR *clFullName = NULL ;
      BSONObj oldMatch ;   // old matcher
      BSONObj oldObject ;  // old modifier
      BSONObj newMatch ;   // new matcher
      BSONObj newObject ;  // new modifier
      BSONObj rollbackObject ;

      dpsTransPendingKey newPendingKey, oldPendingKey ;
      dpsTransPendingValue newPendingValue, oldPendingValue ;
      BOOLEAN isPendingKeyExist = FALSE ;
      UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;

      rc = dpsRecord2Update( (const CHAR *)recordHeader,
                              &clFullName, oldMatch, oldObject,
                              newMatch, newObject, NULL, NULL, NULL,
                              &logWriteMod ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get update record of LSN [%llu], "
                   "rc: %d", recordHeader->_lsn, rc ) ;

      if ( oldObject.isEmpty() )
      {
         PD_LOG( PDDEBUG, "Update record of LSN [%llu] has empty modifier",
                 recordHeader->_lsn ) ;
         goto done ;
      }

      // construct old pending key, no need to copy, used for find
      // check for new match (NOT old) of update record, which OID to be
      // rollbacked
      oldPendingKey.setKey( clFullName, newMatch, FALSE ) ;

      rc = dpsRemoveTransPending( mapPendingObj, oldPendingKey, &newObject,
                                  &oldPendingValue, isPendingKeyExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove transaction rollback "
                   "pending object, rc: %d", rc ) ;

      if ( isPendingKeyExist )
      {
         // pending key exists means a duplicated key issue happened
         // we need to rollback to the old object of update DPS record
         // but current record on the disk is not correct since the duplicated
         // key issue, so we need to recover the old object from the removed
         // pending key
         rc = _replayUpdateModifier( oldObject, newObject, rollbackObject,
                                     logWriteMod ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to replay update modifier on "
                      "old pending object, rc: %d", rc ) ;

         if ( LOG_TYPE_DATA_DELETE != oldPendingValue._opType )
         {
            // old pending value is not created by DELETE
            // ( means created by UPDATE ), the DMS object is not correct,
            // need to replace whole object with the new one
            newMatch = oldPendingValue._obj ;
            oldObject = BSON( "$replace" << rollbackObject ) ;
            logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
         }

         if ( mapPendingObj.empty() )
         {
            // no pending object, clear rollback pending flag
            eduCB->clearTransRBPending() ;
         }
      }

      if ( isPendingKeyExist &&
           LOG_TYPE_DATA_DELETE == oldPendingValue._opType )
      {
         // in below case, we perform INSERT
         // if pending key exists but pending value is empty
         // ( created by DELETE ), means the object does not exists in DMS,
         // so we need to insert the expected rollback object back
         rc = rtnInsert( clFullName, rollbackObject, 1, 0, eduCB,
                         _dmsCB, _dpsCB, 1 ) ;
      }
      else
      {
         // in below case, we perform UPDATE
         // if pending key does not exist, perform the original update rollback
         // and if pending value is not empty, means a incorrect record exists
         // in DMS, so we need to update the
         utilUpdateResult upResult ;
         rc = rtnUpdate( clFullName, newMatch, oldObject,
                         s_replayHint, 0, eduCB, _dmsCB, _dpsCB, 1,
                         &upResult, NULL, logWriteMod ) ;

         if ( upResult.updateNum() == 0 )
         {
            PD_LOG( PDWARNING, "Rollback update record of LSN [%llu], "
                    "no record is rollbacked", recordHeader->_lsn ) ;
         }
         else if ( upResult.updateNum() > 1 )
         {
            PD_LOG( PDWARNING, "Rollback update record of LSN [%llu], "
                    "more than one record [%lld] are rollbacked",
                    recordHeader->_lsn, upResult.updateNum() ) ;
         }
      }
      if ( SDB_IXM_DUP_KEY == rc )
      {
         BOOLEAN added = FALSE ;

         if ( !isPendingKeyExist )
         {
            // old pending key does not exists, we need to construct the new
            // pending key from corresponding DMS object, perform the update on
            // corresponding object and store result in new pending key
            BSONObj currentObject ;
            rc = _queryRecord( clFullName, newMatch, eduCB, currentObject ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to query object [%s] from "
                         "collection [%s], rc: %d",
                         newMatch.toPoolString().c_str(), clFullName, rc ) ;

            rc = _replayUpdateModifier( oldObject, currentObject,
                                        rollbackObject, logWriteMod ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to replay update modifier on "
                         "DMS object, rc: %d", rc ) ;

            newPendingKey.setKey( clFullName, rollbackObject, TRUE ) ;

            // old pending key does not exit, save the current DMS object (OID)
            // as new pending value
            newPendingValue.setValue( newMatch, LOG_TYPE_DATA_UPDATE ) ;
         }
         else
         {
            // old pending key exits, and we failed to rollback again
            // rollback object is already generated in earlier phase, use it
            // as new pending key
            newPendingKey.setKey( clFullName, rollbackObject, TRUE ) ;

            // restore the old pending value back
            newPendingValue.setValue( oldPendingValue._obj,
                                      oldPendingValue._opType ) ;
         }

         // save pending object into pending map
         rc = dpsAddTransPending( mapPendingObj, newPendingKey, newPendingValue,
                                  added ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add transaction pending object, "
                      "rc: %d", rc ) ;

         // have pending object, set rollback pending flag
         eduCB->setTransRBPending() ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback update record of "
                      "LSN [%llu], rc: %d", recordHeader->_lsn, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__ROLEBACKTRANSUPDATE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__ROLEBACKTRANSDELETE, "_clsReplayer::_rollbackTransDelete" )
   INT32 _clsReplayer::_rollbackTransDelete(
                                       const dpsLogRecordHeader *recordHeader,
                                       pmdEDUCB *eduCB,
                                       MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__ROLEBACKTRANSUPDATE ) ;

      SDB_ASSERT( NULL != recordHeader, "record header is invalid" ) ;
      SDB_ASSERT( LOG_TYPE_DATA_DELETE == recordHeader->_type,
                  "should be DELETE log" ) ;

      BSONObj deleteObject ;
      const CHAR *clFullName = NULL ;

      rc = dpsRecord2Delete( (const CHAR *)recordHeader, &clFullName,
                             deleteObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get delete record of LSN [%llu], "
                   "rc: %d", recordHeader->_lsn, rc ) ;

      rc = rtnInsert( clFullName, deleteObject, 1, 0, eduCB, _dmsCB, _dpsCB,
                      1 ) ;
      if ( SDB_IXM_DUP_KEY == rc )
      {
         // A duplicated key issue happened, means the key had been recreated
         // during this transaction by other transactions
         // NOTE: if the key was not first inserted by this transaction,
         //       other transaction will report duplicated keys since the old
         //       version exists
         dpsTransPendingKey newKey( clFullName, deleteObject, TRUE ) ;
         dpsTransPendingValue newValue( BSONObj(), LOG_TYPE_DATA_DELETE ) ;
         BOOLEAN added = FALSE ;

         // save pending object into pending map
         rc = dpsAddTransPending( mapPendingObj, newKey, newValue, added ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add transaction pending object, "
                      "rc: %d", rc ) ;

         // have pending object, set rollback pending flag
         eduCB->setTransRBPending() ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to execute insert to rollback "
                      "delete record of LSN [%llu], rc: %d",
                      recordHeader->_lsn, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__ROLEBACKTRANSUPDATE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__LOGTRANSROLLBACK, "_clsReplayer::_logTransRollback" )
   INT32 _clsReplayer::_logTransRollback( pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__LOGTRANSROLLBACK ) ;

      dpsMergeInfo info ;

      // no need for notify LSN
      info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, NULL ) ;

      dpsLogRecord &record = info.getMergeBlock().record() ;

      DPS_TRANS_ID transID = eduCB->getTransID() ;
      DPS_LSN_OFFSET preTransLSN = eduCB->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedTransLSN = eduCB->getRelatedTransLSN() ;

      PD_CHECK( DPS_INVALID_LSN_OFFSET == preTransLSN, SDB_SYS, error, PDERROR,
                "Failed to log transaction rollback for transaction [%llu], "
                "preTransLSN is not empty [%llu]", transID, preTransLSN ) ;

      rc = dpsTransRollback2Record( transID, preTransLSN, relatedTransLSN,
                                    record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to transform record into "
                   "transaction rollback record, rc: %d", rc ) ;

      rc = _dpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare transaction rollback "
                   "record, rc: %d", rc ) ;

      // write dps
      _dpsCB->writeData( info ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__LOGTRANSROLLBACK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLAYRBPENDING, "_clsReplayer::replayRBPending" )
   INT32 _clsReplayer::replayRBPending( const dpsLogRecordHeader *recordHeader,
                                        BOOLEAN removeOnly,
                                        pmdEDUCB *eduCB,
                                        MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP_REPLAYRBPENDING ) ;

      switch ( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
         {
            rc = _replayInsertRBPending( recordHeader, removeOnly, eduCB,
                                         mapPendingObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to replay INSERT rollback "
                         "pending DPS record [LSN: %llu]",
                         recordHeader->_lsn ) ;
            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            rc = _replayUpdateRBPending( recordHeader, removeOnly, eduCB,
                                         mapPendingObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to replay UPDATE rollback "
                         "pending DPS record [LSN: %llu]",
                         recordHeader->_lsn ) ;
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            rc = _replayDeleteRBPending( recordHeader, removeOnly, eduCB,
                                         mapPendingObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to replay DELETE rollback "
                         "pending DPS record [LSN: %llu]",
                         recordHeader->_lsn ) ;
            break ;
         }
         default :
         {
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Failed to replay rollback pending DPS record "
                      "[LSN: %llu], unsupported type [%d]",
                      recordHeader->_lsn, recordHeader->_type ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSREP_REPLAYRBPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__REPLAYINSERTRBPENDING, "_clsReplayer::_replayInsertRBPending" )
   INT32 _clsReplayer::_replayInsertRBPending(
                                       const dpsLogRecordHeader *recordHeader,
                                       BOOLEAN removeOnly,
                                       pmdEDUCB *eduCB,
                                       MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__REPLAYINSERTRBPENDING ) ;

      SDB_ASSERT( NULL != recordHeader, "record header is invalid" ) ;
      SDB_ASSERT( LOG_TYPE_DATA_INSERT == recordHeader->_type,
                  "should be INSERT log" ) ;

      BSONObj insertObject ;
      const CHAR *clFullName = NULL ;
      BOOLEAN pendingRemoved = FALSE ;
      dpsTransPendingKey pendingKey ;

      rc = dpsRecord2Insert( (const CHAR *)recordHeader, &clFullName,
                             insertObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get insert record of LSN [%llu], "
                   "rc: %d", recordHeader->_lsn, rc ) ;

      // no need to copy, used for find
      pendingKey.setKey( clFullName, insertObject, FALSE ) ;

      rc = dpsRemoveTransPending( mapPendingObj, pendingKey, NULL, NULL,
                                  pendingRemoved ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove transaction rollback "
                   "pending object, rc: %d", rc ) ;

      // this might remove nothing which is not related to any
      // pending objects
      // no need to check

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__REPLAYINSERTRBPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__REPLAYUPDATERBPENDING, "_clsReplayer::_replayUpdateRBPending" )
   INT32 _clsReplayer::_replayUpdateRBPending(
                                       const dpsLogRecordHeader *recordHeader,
                                       BOOLEAN removeOnly,
                                       pmdEDUCB *eduCB,
                                       MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__REPLAYUPDATERBPENDING ) ;

      SDB_ASSERT( NULL != recordHeader, "record header is invalid" ) ;
      SDB_ASSERT( LOG_TYPE_DATA_UPDATE == recordHeader->_type,
                  "should be UPDATE log" ) ;

      const CHAR *clFullName = NULL ;
      BSONObj oldMatch ;   // old matcher
      BSONObj oldObject ;  // old modifier
      BSONObj newMatch ;   // new matcher
      BSONObj newObject ;  // new modifier
      BSONObj rollbackObject ;

      dpsTransPendingKey newPendingKey, oldPendingKey ;
      dpsTransPendingValue newPendingValue, oldPendingValue ;
      BOOLEAN isPendingKeyExist = FALSE, added = FALSE ;
      UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;

      rc = dpsRecord2Update( (const CHAR *)recordHeader,
                              &clFullName, oldMatch, oldObject,
                              newMatch, newObject, NULL, NULL, NULL,
                              &logWriteMod ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get update record of LSN [%llu], "
                   "rc: %d", recordHeader->_lsn, rc ) ;

      if ( oldObject.isEmpty() )
      {
         PD_LOG( PDDEBUG, "Update record of LSN [%llu] has empty modifier",
                 recordHeader->_lsn ) ;
         goto done ;
      }

      // construct old pending key, no need to copy, used for find
      // check for new match (NOT old) of update record, which OID to be
      // rollbacked
      oldPendingKey.setKey( clFullName, newMatch, FALSE ) ;

      rc = dpsRemoveTransPending( mapPendingObj, oldPendingKey, &newObject,
                                  &oldPendingValue, isPendingKeyExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove transaction rollback "
                   "pending object, rc: %d", rc ) ;

      if ( removeOnly )
      {
         // this might remove nothing which is not related to any
         // pending objects
         // no need to check
         goto done ;
      }

      if ( isPendingKeyExist )
      {
         // pending key exists means a duplicated key issue happened
         // we need to rollback to the old object of update DPS record
         // but current record on the disk is not correct since the duplicated
         // key issue, so we need to recover the old object from the removed
         // pending key
         rc = _replayUpdateModifier( oldObject, newObject, rollbackObject,
                                     logWriteMod ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to replay update modifier on "
                      "old pending object, rc: %d", rc ) ;

         // old pending key exits, and we failed to rollback again
         // rollback object is already generated in earlier phase, use it
         // as new pending key
         newPendingKey.setKey( clFullName, rollbackObject, TRUE ) ;

         // restore the old pending value back
         newPendingValue.setValue( oldPendingValue._obj,
                                   oldPendingValue._opType ) ;
      }
      else
      {
         // old pending key does not exists, we need to construct the new
         // pending key from corresponding DMS object, perform the update on
         // corresponding object and store result in new pending key
         BSONObj currentObject ;
         rc = _queryRecord( clFullName, newMatch, eduCB, currentObject ) ;
         if ( SDB_DMS_EOC == rc )
         {
            // EOC means later rollback will remove this pending object
            // which might change the object in DMS, so we could not find
            // the object with previous DPS record
            // in this case, we only keep the OID as pending key for later DPS
            // record to remove this pending object
            rollbackObject = oldMatch ;
            rc = SDB_OK ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to query object [%s] from "
                         "collection [%s], rc: %d",
                         newMatch.toPoolString().c_str(), clFullName, rc ) ;

            rc = _replayUpdateModifier( oldObject, currentObject,
                                        rollbackObject, logWriteMod ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to replay update modifier on "
                         "DMS object, rc: %d", rc ) ;
         }

         newPendingKey.setKey( clFullName, rollbackObject, TRUE ) ;

         // old pending key does not exit, save the current DMS object (OID)
         // as new pending value
         newPendingValue.setValue( newMatch, LOG_TYPE_DATA_UPDATE ) ;
      }

      // save pending object into pending map
      rc = dpsAddTransPending( mapPendingObj, newPendingKey, newPendingValue,
                               added ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add transaction pending object, "
                   "rc: %d", rc ) ;

      PD_CHECK( added, SDB_SYS, error, PDERROR,
                "Failed to add pending key [ collection %s, key %s ]",
                newPendingKey._collection.c_str(),
                newPendingKey._obj.toPoolString().c_str() ) ;

      // have pending object, set rollback pending flag
      eduCB->setTransRBPending() ;

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__REPLAYUPDATERBPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__REPLAYDELETERBPENDING, "_clsReplayer::_replayDeleteRBPending" )
   INT32 _clsReplayer::_replayDeleteRBPending(
                                       const dpsLogRecordHeader *recordHeader,
                                       BOOLEAN removeOnly,
                                       pmdEDUCB *eduCB,
                                       MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__REPLAYDELETERBPENDING ) ;

      SDB_ASSERT( NULL != recordHeader, "record header is invalid" ) ;
      SDB_ASSERT( LOG_TYPE_DATA_DELETE == recordHeader->_type,
                  "should be DELETE log" ) ;

      BSONObj deleteObject ;
      const CHAR *clFullName = NULL ;
      BOOLEAN added = FALSE ;
      dpsTransPendingKey pendingKey ;
      dpsTransPendingValue pendingValue ;

      if ( removeOnly )
      {
         // this might remove nothing which is not related to any
         // pending objects
         // no need to check
         goto done ;
      }

      rc = dpsRecord2Delete( (const CHAR *)recordHeader, &clFullName,
                             deleteObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get delete record of LSN [%llu], "
                   "rc: %d", recordHeader->_lsn, rc ) ;

      pendingKey.setKey( clFullName, deleteObject, TRUE ) ;
      pendingValue.setValue( BSONObj(), LOG_TYPE_DATA_DELETE ) ;

      // save pending object into pending map
      rc = dpsAddTransPending( mapPendingObj, pendingKey, pendingValue, added ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add transaction pending object, "
                   "rc: %d", rc ) ;

      PD_CHECK( added, SDB_SYS, error, PDERROR,
                "Failed to add pending key [ collection %s, key %s ]",
                pendingKey._collection.c_str(),
                pendingKey._obj.toPoolString().c_str() ) ;

      // have pending object, set rollback pending flag
      eduCB->setTransRBPending() ;

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__REPLAYDELETERBPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__REPLAYUPDATEMODIFIER, "_clsReplayer::_replayUpdateModifier" )
   INT32 _clsReplayer::_replayUpdateModifier( const BSONObj &updater,
                                              const BSONObj &oldObject,
                                              BSONObj &newObject,
                                              UINT32 logWriteMode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__REPLAYUPDATEMODIFIER ) ;

      mthModifier modifier ;
      rc = modifier.loadPattern( updater, NULL, TRUE, NULL, FALSE,
                                 logWriteMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load modify pattern [%s], rc: %d",
                   updater.toPoolString().c_str(), rc ) ;

      rc = modifier.modify( oldObject, newObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to modify [%s] by [%s], rc: %d",
                   oldObject.toPoolString().c_str(),
                   updater.toPoolString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSREP__REPLAYUPDATEMODIFIER, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP__QUERYRECORD, "_clsReplayer::_queryRecord" )
   INT32 _clsReplayer::_queryRecord( const CHAR *collection,
                                     const BSONObj &matcher,
                                     pmdEDUCB *eduCB,
                                     BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREP__QUERYRECORD ) ;

      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      INT64 contextID = -1 ;
      BSONObj currentObject ;
      rtnContextBuf buffer ;
      mthModifier modifier ;

      rc = rtnQuery( collection, BSONObj(), matcher, BSONObj(),
                     s_replayHint, 0, eduCB, 0, 1, _dmsCB,
                     sdbGetRTNCB(), contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query object [%s] from "
                   "collection [%s], rc: %d", matcher.toPoolString().c_str(),
                   collection, rc ) ;

      rc = rtnGetMore( contextID, 1, buffer, eduCB, sdbGetRTNCB() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         PD_LOG( PDDEBUG, "Hit end of query object [%s] from collection [%s]",
                 matcher.toPoolString().c_str(), collection ) ;
         goto done ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to get object [%s] from "
                      "collection [%s], rc: %d",
                      matcher.toPoolString().c_str(), collection, rc ) ;
      }

      /// get object
      rc = buffer.nextObj( currentObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse object [%s] from "
                   "collection [%s], rc: %d",
                   matcher.toPoolString().c_str(), collection, rc ) ;

      object = currentObject.getOwned() ;

   done :
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSREP__QUERYRECORD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   static BOOLEAN _isTextIdx( const BSONObj &index )
   {
      BSONObj idxDef = index.getObjectField( IXM_FIELD_NAME_KEY ) ;
      BSONElement ele = idxDef.firstElement() ;

      return ( 0 == ossStrcmp( ele.valuestrsafe(), IXM_TEXT_KEY_TYPE ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STARTINXJOB, "startIndexJob" )
   INT32 startIndexJob ( RTN_JOB_TYPE type,
                         const dpsLogRecordHeader *recordHeader,
                         _dpsLogWrapper *dpsCB,
                         BOOLEAN isRollBack )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_STARTINXJOB );
      const CHAR *fullname = NULL ;
      BSONObj index ;
      std::string indexName ;
      rtnIndexJob *indexJob = NULL ;
      clsCatalogSet *pCatSet = NULL ;
      BOOLEAN useSync = FALSE ;

      if ( LOG_TYPE_IX_CRT != recordHeader->_type &&
           LOG_TYPE_IX_DELETE != recordHeader->_type )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( LOG_TYPE_IX_CRT == recordHeader->_type )
      {
         rc = dpsRecord2IXCrt( (CHAR *)recordHeader,
                               &fullname,
                               index ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = dpsRecord2IXDel( (CHAR *)recordHeader,
                               &fullname,
                               index ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      if ( pmdGetKRCB()->isRestore() || _isTextIdx( index ) )
      {
         useSync = TRUE ;
      }
      else
      {
         sdbGetShardCB()->getAndLockCataSet( fullname, &pCatSet, TRUE ) ;
         if ( pCatSet && CLS_REPLSET_MAX_NODE_SIZE == pCatSet->getW() )
         {
            useSync = TRUE ;
         }
         sdbGetShardCB()->unlockCataSet( pCatSet ) ;
      }

      indexJob = SDB_OSS_NEW rtnIndexJob( type, fullname,
                                          index, dpsCB,
                                          recordHeader->_lsn,
                                          isRollBack ) ;
      if ( NULL == indexJob )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for indexJob" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = indexJob->init () ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Index job[%s] init failed, rc = %d",
                  indexJob->name(), rc ) ;
         goto error ;
      }

      indexName = indexJob->getIndexName() ;
      /// When is $id or useSync
      if ( useSync ||
           0 == ossStrcmp( indexJob->getIndexName(), IXM_ID_KEY_NAME ) )
      {
         indexJob->doit() ;
         SDB_OSS_DEL indexJob ;
         indexJob = NULL ;
      }
      else
      {
         EDUID jobEduID = PMD_INVALID_EDUID ;
         // if use RTN_JOB_MUTEX_STOP_RET, when create index have complete,
         // drop index should not drop really, so it's error, need to use
         // RTN_JOB_MUTEX_STOP_CONT
         rc = rtnGetJobMgr()->startJob( indexJob, RTN_JOB_MUTEX_STOP_CONT,
                                        &jobEduID ) ;

         /// When create index, should wait the index has created into
         /// meta data
         if ( PMD_INVALID_EDUID != jobEduID &&
              LOG_TYPE_IX_CRT == recordHeader->_type )
         {
            BOOLEAN indexExist = FALSE ;

            while ( NULL != rtnGetJobMgr()->findJob( jobEduID ) )
            {
               /// when index job is running
               if ( SDB_OK != rtnIndexJob::checkIndexExist( fullname,
                                                            indexName.c_str(),
                                                            indexExist ) ||
                    TRUE == indexExist )
               {
                  break ;
               }
               ossSleep( CLS_REPLAY_CHECK_INTERVAL ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_STARTINXJOB, rc );
      return rc ;
   error:
      goto done ;
   }

}
