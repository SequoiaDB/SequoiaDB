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
#include "utilCompressor.hpp"

using namespace bson ;

namespace engine
{

   INT32 startIndexJob ( RTN_JOB_TYPE type,
                         const dpsLogRecordHeader *recordHeader,
                         _dpsLogWrapper *dpsCB,
                         BOOLEAN isRollBack ) ;

   _clsReplayer::_clsReplayer( BOOLEAN useDps )
   {
      _dmsCB = sdbGetDMSCB() ;
      _dpsCB = NULL ;
      if ( useDps )
      {
         _dpsCB = sdbGetDPSCB() ;
      }
      _monDBCB = pmdGetKRCB()->getMonDBCB () ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLYBUCKET, "_clsReplayer::replayByBucket" )
   INT32 _clsReplayer::replayByBucket( dpsLogRecordHeader *recordHeader,
                                       pmdEDUCB *eduCB, clsBucket *pBucket )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_REPLYBUCKET ) ;
      const CHAR *fullname = NULL ;
      BSONObj obj ;
      BSONElement idEle ;
      const bson::OID *oidPtr = NULL ;
      UINT32 sequence = 0 ;
      BOOLEAN paralla = FALSE ;
      BOOLEAN updateSameOID = FALSE ;
      UINT32 bucketID = ~0 ;

      SDB_ASSERT( recordHeader && pBucket, "Invalid param" ) ;
      try
      {
      switch( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
            rc = dpsRecord2Insert( (CHAR *)recordHeader, &fullname, obj ) ;
            break ;
         case LOG_TYPE_DATA_DELETE :
            rc = dpsRecord2Delete( (CHAR *)recordHeader, &fullname, obj ) ;
            break ;
         case LOG_TYPE_DATA_UPDATE :
         {
            BSONObj match ;     //old match
            BSONObj oldObj ;
            BSONObj newMatch ;
            BSONObj modifier ;   //new change obj
            rc = dpsRecord2Update( (CHAR *)recordHeader, &fullname,
                                   match, oldObj, newMatch, modifier ) ;
            if ( SDB_OK == rc &&
                 0 == match.woCompare( newMatch, BSONObj(), false ) )
            {
               obj = match ;
               updateSameOID = TRUE ;
            }
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            paralla = TRUE ;
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            oidPtr = oid ;
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            paralla = TRUE ;
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            oidPtr = oid ;
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            paralla = TRUE ;
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            oidPtr = oid ;
            break ;
         }
         case LOG_TYPE_DUMMY :
            bucketID = 0 ;
            break ;
         default :
            break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Parse dps log[type: %d, lsn: %lld, len: %d]"
                   "falied, rc: %d", recordHeader->_type,
                   recordHeader->_lsn, recordHeader->_length, rc ) ;

      if ( LOG_TYPE_DATA_INSERT == recordHeader->_type ||
           LOG_TYPE_DATA_DELETE == recordHeader->_type ||
           ( LOG_TYPE_DATA_UPDATE == recordHeader->_type && updateSameOID ) )
      {
         dmsStorageUnit *su = NULL ;
         const CHAR *pShortName = NULL ;
         dmsStorageUnitID suID = DMS_INVALID_SUID ;
         rc = rtnResolveCollectionNameAndLock( fullname, _dmsCB, &su,
                                               &pShortName, suID ) ;
         if ( SDB_OK == rc )
         {
            dmsMBContext *mbContext = NULL ;
            rc = su->data()->getMBContext( &mbContext, pShortName, SHARED ) ;
            if ( SDB_OK == rc )
            {
               paralla = ( mbContext->mbStat()->_uniqueIdxNum <= 1 &&
                           0 == mbContext->mbStat()->_textIdxNum ) ?
                         TRUE : FALSE ;
               su->data()->releaseMBContext( mbContext ) ;
            }

            if ( DMS_STORAGE_CAPPED == su->type() )
            {
               paralla = FALSE ;
            }
            _dmsCB->suUnlock( suID, SHARED ) ;
         }
      }

      if ( paralla )
      {
         idEle = obj.getField( DMS_ID_KEY_NAME ) ;
         if ( !idEle.eoo() )
         {
            bucketID = pBucket->calcIndex( idEle.value(),
                                           idEle.valuesize() ) ;
         }
         else if ( NULL != oidPtr )
         {
            CHAR tmpData[ sizeof( *oidPtr ) + sizeof( sequence ) ] = { 0 } ;
            ossMemcpy( tmpData, ( const CHAR * )( oidPtr->getData()),
                       sizeof( *oidPtr ) ) ;
            ossMemcpy( &tmpData[ sizeof( *oidPtr ) ], ( const CHAR* )&sequence,
                       sizeof( sequence ) ) ;
            bucketID = pBucket->calcIndex( tmpData, sizeof( tmpData ) ) ;
         }
      }

      if ( (UINT32)~0 != bucketID )
      {
         rc = pBucket->pushData( bucketID, (CHAR *)recordHeader,
                                 recordHeader->_length ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push log to bucket, rc: %d",
                      rc ) ;
      }
      else
      {
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

         if ( !pBucket->_expectLSN.invalid() &&
              0 != pBucket->_expectLSN.compareOffset( recordHeader->_lsn ) )
         {
            PD_LOG( PDWARNING, "Expect lsn[%lld], real complete lsn[%lld]",
                    recordHeader->_lsn, pBucket->_expectLSN.offset ) ;
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         rc = replay( recordHeader, eduCB ) ;
         if ( SDB_OK == rc && !pBucket->_expectLSN.invalid() )
         {
            pBucket->_expectLSN.offset += recordHeader->_length ;
            pBucket->_expectLSN.version = recordHeader->_version ;
         }
      }
      }
      catch (std::exception &e)
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
      PD_TRACE_EXITRC ( SDB__CLSREP_REPLYBUCKET, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLAY, "_clsReplayer::replay" )
   INT32 _clsReplayer::replay( dpsLogRecordHeader *recordHeader,
                               pmdEDUCB *eduCB, BOOLEAN incCount )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_REPLAY );
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn ) ;
      }

      try
      {
      switch ( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
         {
            const CHAR *fullname = NULL ;
            BSONObj obj ;
            rc = dpsRecord2Insert( (CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnReplayInsert( fullname, obj, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
            if ( SDB_OK == rc && incCount )
            {
               _monDBCB->monOperationCountInc ( MON_INSERT_REPL ) ;
            }
            else if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG( PDINFO, "Record[%s] already exist when insert",
                       obj.toString().c_str() ) ;
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
            BSONObj hint = BSON(""<<IXM_ID_KEY_NAME);
            rc = dpsRecord2Update( (CHAR *)recordHeader,
                                   &fullname,
                                   match,
                                   oldObj,
                                   newMatch,
                                   modifier ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            if ( !modifier.isEmpty() )
            {
               rc = rtnUpdate( fullname, match, modifier,
                               hint, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
            }
            if ( SDB_OK == rc && incCount )
            {
               _monDBCB->monOperationCountInc ( MON_UPDATE_REPL ) ;
            }
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            const CHAR *fullname = NULL ;
            BSONObj obj ;
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
                  BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
                  rc = rtnDelete( fullname, selector, hint, 0, eduCB, _dmsCB,
                                  _dpsCB, 1 ) ;
               }
            }
            if ( SDB_OK == rc && incCount )
            {
               _monDBCB->monOperationCountInc ( MON_DELETE_REPL ) ;
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
                                _dpsCB, 1, direction ) ;
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG( PDINFO, "Logical id[ %lld ] is invalid when pop",
                       logicalID ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CS_CRT :
         {
            const CHAR *cs = NULL ;
            INT32 pageSize = 0 ;
            INT32 lobPageSize = 0 ;
            INT32 type = 0 ;
            rc = dpsRecord2CSCrt( (CHAR *)recordHeader,
                                  &cs, pageSize, lobPageSize, type ) ;
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
                                                  pageSize, lobPageSize,
                                                  (DMS_STORAGE_TYPE)type ) ;
            if ( SDB_DMS_CS_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection space[%s] already exist when "
                       "create", cs ) ;
               rc = SDB_OK ;
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
            UINT32 attribute = 0 ;
            UINT8 compType = UTIL_COMPRESSOR_INVALID ;
            BSONObj extOptions ;
            rc = dpsRecord2CLCrt( (CHAR *)recordHeader, &cl, attribute,
                                  compType, extOptions ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnCreateCollectionCommand( cl, attribute, eduCB, _dmsCB,
                                             _dpsCB,
                                             (UTIL_COMPRESSOR_TYPE)compType,
                                             0, TRUE,
                                             ( extOptions.isEmpty() ?
                                               NULL : &extOptions ) ) ;
            if ( SDB_DMS_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection [%s] already exist when "
                       "create", cl ) ;
               rc = SDB_OK ;
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
            dmsStorageUnitID suID = DMS_INVALID_CS ;
            dmsStorageUnit *su = NULL ;
            rc = dpsRecord2CLRename( (CHAR *)recordHeader,
                                      &cs, &oldCl, &newCl ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnCollectionSpaceLock ( cs, _dmsCB, FALSE, &su, suID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get collection space %s, rc: %d",
                       cs, rc ) ;
               goto error ;
            }
            rc = su->data()->renameCollection ( oldCl, newCl,
                                                eduCB, _dpsCB ) ;
            _dmsCB->suUnlock ( suID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename %s to %s, rc: %d",
                       oldCl, newCl, rc ) ;
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
            while( TRUE )
            {
               rc = _dmsCB->renameCollectionSpace( oldName, newName,
                                                   eduCB, _dpsCB ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep ( 100 ) ;
                  continue ;
               }
               break ;
            }
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
            const CHAR *clname = NULL ;
            rc = dpsRecord2CLTrunc( (const CHAR *)recordHeader, &clname ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
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
                  csName = NULL ;
               }

               rtnAnalyze( csName, clFullName, ixName, param,
                           eduCB, _dmsCB, NULL, NULL ) ;
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_CATA ) )
            {
               catAgent *pCatAgent = sdbGetShardCB()->getCataAgent() ;
               if ( pCatAgent )
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
      eduCB->resetLsn() ;
      if ( SDB_OK != rc )
      {
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
                  BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
                  rc = rtnDelete( fullname, selector, hint, 0, eduCB, _dmsCB,
                                  _dpsCB, 1 ) ;
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
            BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
            rc = dpsRecord2Update( (const CHAR *)recordHeader,
                                    &fullname,
                                    oldMatch,
                                    modifier,
                                    newMatch,
                                    newObj ) ;
            if ( !modifier.isEmpty() )
            {
               rc = rtnUpdate( fullname, newMatch, modifier,
                               hint, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
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
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CS_CRT :
         {
            const CHAR *cs = NULL ;
            INT32 pageSize = 0 ;
            INT32 lobPageSize = 0 ;
            INT32 type = 0 ;
            rc = dpsRecord2CSCrt( (const CHAR *)recordHeader,
                                  &cs, pageSize, lobPageSize, type ) ;
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
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_CL_CRT :
         {
            const CHAR *fullname = NULL ;
            UINT32 attribute = 0 ;
            UINT8 compType = UTIL_COMPRESSOR_INVALID ;
            BSONObj extOptions ;
            rc = dpsRecord2CLCrt( (const CHAR *)recordHeader, &fullname,
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
            dmsStorageUnitID suID = DMS_INVALID_CS ;
            dmsStorageUnit *su = NULL ;
            const CHAR *cs = NULL ;
            const CHAR *oldName = NULL ;
            const CHAR *newName = NULL ;
            rc = dpsRecord2CLRename( (const CHAR *)recordHeader,
                                     &cs,
                                     &oldName,
                                     &newName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnCollectionSpaceLock ( cs, _dmsCB, FALSE, &su, suID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get collection space %s, rc: %d",
                       cs, rc ) ;
               goto error ;
            }
            rc = su->data()->renameCollection ( newName,
                                                oldName,
                                                eduCB,
                                                _dpsCB ) ;
            _dmsCB->suUnlock ( suID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename %s to %s, rc: %d",
                       newName, oldName, rc ) ;
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
            while( TRUE )
            {
               rc = _dmsCB->renameCollectionSpace( newName, oldName,
                                                   eduCB, _dpsCB ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep ( 100 ) ;
                  continue ;
               }
               break ;
            }
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename %s to %s, rc: %d",
                       newName, oldName, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_TS_COMMIT :
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
         rc = SDB_CLS_REPLAY_LOG_FAILED ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      eduCB->resetLsn() ;
      if ( SDB_OK != rc )
      {
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

   INT32 _clsReplayer::replayCrtCS( const CHAR *cs, INT32 pageSize,
                                    INT32 lobPageSize, DMS_STORAGE_TYPE type,
                                    _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != cs, "cs should not be NULL" ) ;
      INT32 rc = rtnTestCollectionSpaceCommand( cs, _dmsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionSpaceCommand( cs, eduCB, _dmsCB,
                                               _dpsCB, pageSize,
                                               lobPageSize, type, TRUE ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayCrtCollection( const CHAR *collection,
                                            UINT32 attributes,
                                            _pmdEDUCB *eduCB,
                                            UTIL_COMPRESSOR_TYPE compType,
                                            const BSONObj *extOptions )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      INT32 rc = rtnTestCollectionCommand( collection, _dmsCB ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionCommand( collection, attributes, eduCB,
                                          _dmsCB, _dpsCB, compType,
                                          0, TRUE, extOptions ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayIXCrt( const CHAR *collection,
                                    BSONObj &index,
                                    _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      return rtnCreateIndexCommand( collection, index, eduCB,
                                    _dmsCB, _dpsCB, TRUE ) ;
   }

   INT32 _clsReplayer::replayInsert( const CHAR *collection,
                                     BSONObj &obj,
                                     _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      return rtnReplayInsert( collection, obj, FLG_INSERT_CONTONDUP, eduCB,
                              _dmsCB, _dpsCB ) ;
   }

   INT32 _clsReplayer::replayWriteLob( const CHAR *fullName,
                                       const bson::OID &oid,
                                       UINT32 sequence,
                                       UINT32 offset,
                                       UINT32 len,
                                       const CHAR *data,
                                       _pmdEDUCB *eduCB )
   {
      return rtnWriteLob( fullName, oid, sequence,
                          offset, len, data, eduCB,
                          1, _dpsCB ) ;
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
      rtnIndexJob *indexJob = NULL ;
      clsCatalogSet *pCatSet = NULL ;
      BOOLEAN useSync = FALSE ;

      if ( LOG_TYPE_IX_CRT != recordHeader->_type &&
           LOG_TYPE_IX_DELETE != recordHeader->_type )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( LOG_TYPE_IX_CRT == recordHeader->_type)
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

      if ( pmdGetKRCB()->isRestore() )
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

      if ( useSync ||
           0 == ossStrcmp( indexJob->getIndexName(), IXM_ID_KEY_NAME ) )
      {
         indexJob->doit() ;
         SDB_OSS_DEL indexJob ;
         indexJob = NULL ;
      }
      else
      {
         rc = rtnGetJobMgr()->startJob( indexJob, RTN_JOB_MUTEX_STOP_CONT,
                                        NULL ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_STARTINXJOB, rc );
      return rc ;
   error:
      goto done ;
   }

}
