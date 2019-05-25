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

   Source File Name = rtnTransaction.cpp

   Descriptive Name = Runtime Transaction

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   runtime transaction management for data node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "rtn.hpp"
#include "pmdCB.hpp"
#include "dpsMessageBlock.hpp"
#include "clsReplayer.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsOp2Record.hpp"

namespace engine
{

   #define RTN_TRANS_ROLLBACK_RETRY_TIMES             ( 20 )
   #define RTN_TRANS_ROLLBACK_RETRY_INTERVAL          OSS_ONE_SEC

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSBEGIN, "rtnTransBegin" )
   INT32 rtnTransBegin( _pmdEDUCB * cb )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSBEGIN ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32 rc = SDB_OK ;
      if ( !sdbGetTransCB()->isTransOn() )
      {
         rc = SDB_DPS_TRANS_DIABLED ;
         goto error;
      }
      if ( cb->getTransID() == DPS_INVALID_TRANS_ID )
      {
         cb->setTransID( sdbGetTransCB()->allocTransID() ) ;
         cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
         sdbGetTransCB()->addTransCB( cb->getTransID(), cb ) ;
      }
      PD_LOG( PDINFO, "Begin transaction operations(transID=%llu)",
              cb->getTransID() ) ;
      PD_TRACE_EXIT ( SDB_RTNTRANSBEGIN ) ;

   done:
      return rc;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSCOMMIT, "rtnTransCommit" )
   INT32 rtnTransCommit( _pmdEDUCB * cb, SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSCOMMIT ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32 rc = SDB_OK ;

      DPS_TRANS_ID curTransID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET firstTransLsn = DPS_INVALID_LSN_OFFSET ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      curTransID = cb->getTransID() ;
      preTransLsn = cb->getCurTransLsn() ;

      if ( curTransID == DPS_INVALID_TRANS_ID ||
           preTransLsn == DPS_INVALID_LSN_OFFSET )
      {
         sdbGetTransCB()->delTransCB( curTransID ) ;
         cb->setTransID( DPS_INVALID_TRANS_ID ) ;
         sdbGetTransCB()->transLockReleaseAll( cb ) ;
         goto done ;
      }

      if ( !dpsCB )
      {
         goto done ;
      }
      firstTransLsn = sdbGetTransCB()->getBeginLsn( curTransID ) ;
      SDB_ASSERT( firstTransLsn != DPS_INVALID_LSN_OFFSET,
                  "First transaction lsn can't be invalid" ) ;

      PD_LOG( PDEVENT, "Execute commit(transID=%llu, lastLsn=%llu)",
              curTransID, preTransLsn ) ;

      rc = dpsTransCommit2Record( curTransID, preTransLsn, firstTransLsn,
                                  record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build commit log:%d",rc ) ;
         goto error ;
      }

      info.setInfoEx( ~0, DMS_INVALID_CLID, DMS_INVALID_EXTENT, cb ) ;
      rc = dpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      dpsCB->writeData( info ) ;

      sdbGetTransCB()->delTransCB( curTransID ) ;
      cb->setTransID( DPS_INVALID_TRANS_ID ) ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      sdbGetTransCB()->transLockReleaseAll( cb ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNTRANSCOMMIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSROLLBACK, "rtnTransRollback" )
   INT32 rtnTransRollback( _pmdEDUCB * cb, SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSROLLBACK ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32 rc = SDB_OK;
      _dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN );
      DPS_LSN dpsLsn ;
      DPS_LSN_OFFSET curLsnOffset = DPS_INVALID_LSN_OFFSET ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_TRANS_ID curTransID = DPS_INVALID_TRANS_ID ;
      DPS_TRANS_ID rollbackID = DPS_INVALID_TRANS_ID ;
      UINT32 retryTimes = 0 ;
      BOOLEAN doRollback = FALSE ;
      _clsReplayer replayer( TRUE ) ;

      cb->startRollback() ;
      curLsnOffset = cb->getCurTransLsn() ;
      transID = cb->getTransID() ;
      rollbackID = sdbGetTransCB()->getRollbackID( transID ) ;

      if ( DPS_INVALID_TRANS_ID == transID ||
           DPS_INVALID_LSN_OFFSET == curLsnOffset )
      {
         goto done;
      }
      if ( !dpsCB )
      {
         goto done ;
      }

      PD_LOG ( PDEVENT, "Begin to rollback transaction[ID:%llu, "
               "lastLsn:%llu]...", transID, curLsnOffset ) ;
      doRollback = TRUE ;

      cb->setTransID( rollbackID ) ;

      while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )
      {
         dpsLogRecord record;
         mb.clear() ;
         dpsLsn.offset = curLsnOffset ;
         rc = dpsCB->search( dpsLsn, &mb ) ;
         PD_RC_CHECK( rc, PDERROR, "Rollback failed, "
                      "failed to get the log(offset=%llu, version=%d, rc=%d)",
                      curLsnOffset, dpsLsn.version, rc ) ;
         rc = record.load( mb.offset( 0 ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Rollback failed, failed to parse log",
                      "(lsn=%llu, rc=%d)", curLsnOffset, rc ) ;
         dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_TRANSID ) ;
         if ( !itr.valid() )
         {
            PD_LOG( PDERROR, "can not find DPS_LOG_PUBLIC_TRANSID "
                    "in record" ) ;
            rc = SDB_SYS ;
            break ;
         }
         curTransID = sdbGetTransCB()->getTransID(
            *((DPS_TRANS_ID *)itr.value()) ) ;
         PD_CHECK( curTransID == transID, SDB_DPS_CORRUPTED_LOG, error,
                   PDERROR, "Failed to rollback(lsn=%llu), the log is damaged",
                   curLsnOffset ) ;

         if ( pmdGetKRCB()->isCBValue( SDB_CB_CLS ) && !pmdIsPrimary() )
         {
            sdbGetTransCB()->addTransInfo( transID, curLsnOffset ) ;
            rc = SDB_CLS_NOT_PRIMARY ;
            goto error ;
         }

         {
            cb->setRelatedTransLSN( curLsnOffset ) ;
            dpsLogRecord::iterator tmpitr =
               record.find(DPS_LOG_PUBLIC_PRETRANS ) ;
            if ( !tmpitr.valid() )
            {
               curLsnOffset = DPS_INVALID_LSN_OFFSET ;
            }
            else
            {
               curLsnOffset = *(( DPS_LSN_OFFSET *)tmpitr.value());
            }
            cb->setCurTransLsn( curLsnOffset ) ;

            rc = replayer.rollback( ( dpsLogRecordHeader *)mb.offset(0),
                                    cb ) ;
            if ( rc )
            {
               ++retryTimes ;
               PD_LOG( PDERROR, "Rollback transaction[ID:%llu, lsn=%llu, "
                       "time=%u] failed, rc: %d", transID, dpsLsn.offset,
                       retryTimes, rc ) ;
               if ( retryTimes >= RTN_TRANS_ROLLBACK_RETRY_TIMES )
               {
                  PD_LOG( PDSEVERE, "Rollback transaction failed, need to "
                          "restart the system" ) ;
                  PMD_RESTART_DB( rc ) ;
                  goto error ;
               }
               ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
               curLsnOffset = cb->getRelatedTransLSN() ;
            }
            else
            {
               retryTimes = 0 ;
            }
         }
      }

   done:
      sdbGetTransCB()->delTransCB( transID ) ;
      cb->setTransID( DPS_INVALID_TRANS_ID ) ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      cb->setRelatedTransLSN( DPS_INVALID_LSN_OFFSET ) ;
      sdbGetTransCB()->transLockReleaseAll( cb ) ;
      cb->stopRollback() ;

      if ( doRollback )
      {
         PD_LOG ( PDEVENT, "Rollback transaction[ID:%llu] finished with "
                  "rc[%d]", transID, rc ) ;
      }

#if defined ( _DEBUG )
      if ( dpsCB && SDB_ROLE_CATALOG != pmdGetDBRole() )
      {
         dpsCB->completeOpr( cb, CLS_REPLSET_MAX_NODE_SIZE ) ;
      }
#endif // _DEBUG

      PD_TRACE_EXITRC ( SDB_RTNTRANSROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnTransRollbackAll( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      SDB_DPSCB *pDpsCB = sdbGetDPSCB() ;
      TRANS_MAP *pTransMap = pTransCB->getTransMap();
      DPS_LSN dpsLsn;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_TRANS_ID rollbackID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET curLsnOffset = DPS_INVALID_LSN_OFFSET ;
      UINT32 retryTimes = 0 ;
      _clsReplayer replayer( TRUE );
      _dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN ) ;

      cb->startRollback() ;

      PD_LOG ( PDEVENT, "Begin to rollback all unfinished transactions[%d]...",
               pTransMap->size() ) ;

      while ( pTransMap->size() != 0 )
      {
         TRANS_MAP::iterator iterMap = pTransMap->begin();
         transID = iterMap->first ;
         rollbackID = pTransCB->getRollbackID( transID ) ;
         curLsnOffset = iterMap->second ;
         cb->setTransID( rollbackID ) ;

         PD_LOG( PDEVENT, "Begin to rollback transaction[ID: %llu, "
                 "lastLSN: %llu]...", transID, curLsnOffset ) ;

         while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )
         {
            if ( !pTransCB->isDoRollback() )
            {
               PD_LOG ( PDEVENT, "Rollback is interrupted" ) ;
               rc = SDB_INTERRUPT ;
               goto error ;
            }
            dpsLogRecord record ;
            mb.clear() ;
            dpsLsn.offset = curLsnOffset;
            rc = pDpsCB->search( dpsLsn, &mb ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Rollback failed, failed to get the "
                        "log( offset =%llu, version=%d, rc=%d)",
                        curLsnOffset, dpsLsn.version, rc ) ;
               break ;
            }
            rc = record.load( mb.offset( 0 )) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Rollback failed, "
                        "failed to parse log(lsn=%llu, rc=%d)",
                        curLsnOffset, rc ) ;
               break;
            }
            dpsLogRecord::iterator itr =
                            record.find( DPS_LOG_PUBLIC_TRANSID ) ;
            if ( !itr.valid() )
            {
               PD_LOG( PDERROR, "failed to find transid in record." ) ;
               rc = SDB_SYS ;
               break ;
            }

            if ( transID != pTransCB->getTransID(
                                     *(DPS_TRANS_ID *)(itr.value()) ))
            {
               PD_LOG ( PDERROR, "Failed to rollback(lsn=%llu), "
                        "the log is damaged", curLsnOffset ) ;
               break ;
            }

            {
               cb->setRelatedTransLSN( curLsnOffset ) ;
               dpsLogRecord::iterator tmpitr =
                               record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
               if ( !tmpitr.valid() )
               {
                  curLsnOffset = DPS_INVALID_LSN_OFFSET;
               }
               else
               {
                  curLsnOffset = *((DPS_LSN_OFFSET *)tmpitr.value() );
               }
               cb->setCurTransLsn( curLsnOffset ) ;

               rc = replayer.rollback( ( dpsLogRecordHeader *)mb.offset(0),
                                       cb ) ;
               if ( rc )
               {
                  ++retryTimes ;
                  PD_LOG( PDERROR, "Rollback transaction[ID:%llu, lsn=%llu, "
                          "time=%u] failed,  rc: %d", transID,
                          dpsLsn.offset, retryTimes, rc ) ;
                  if ( retryTimes >= RTN_TRANS_ROLLBACK_RETRY_TIMES )
                  {
                     PD_LOG( PDSEVERE, "Rollback transaction failed, need to "
                             "restart the system" ) ;
                     PMD_RESTART_DB( rc ) ;
                     goto error ;
                  }
                  ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
                  curLsnOffset = cb->getRelatedTransLSN() ;
               }
               else
               {
                  retryTimes = 0 ;
                  iterMap->second = curLsnOffset ;
               }
            }
         } /// while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )

         pTransMap->erase( iterMap ) ;
         PD_LOG( PDEVENT, "Rollback transaction[ID:%lld] finished with rc[%d]",
                 transID, rc ) ;
      } /// while ( pTransMap->size() != 0 )

   done:
      pTransCB->transLockReleaseAll( cb ) ;
      pTransCB->stopRollbackTask() ;
      cb->stopRollback() ;

      PD_LOG ( PDEVENT, "Rollback all unfinished transactions finished with "
               "rc[%d]", rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnTransTryLockCL( const CHAR *pCollection, INT32 lockType,
                            _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      const CHAR *pCollectionShortName = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      UINT16 collectionID = DMS_INVALID_MBID;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB  can't be NULL" ) ;
      SDB_ASSERT ( dpsCB, "dpsCB  can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb  can't be NULL" ) ;
      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID );
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name"
                  "(collection:%s, rc=%d)", pCollection, rc ) ;
      rc = su->data()->findCollection ( pCollectionShortName, collectionID ) ;
      logicCSID = su->LogicalCSID();
      dmsCB->suUnlock ( suID );
      PD_RC_CHECK( rc, PDERROR, "Failed to find the collection"
                   "(collection:%s, rc=%d)", pCollection, rc );
      switch( lockType )
      {
      case DPS_TRANSLOCK_S:
            rc = pTransCB->transLockTryS( cb, logicCSID, collectionID );
            break;
      case DPS_TRANSLOCK_X:
            rc = pTransCB->transLockTryX( cb, logicCSID, collectionID );
            break;
      default:
            rc = SDB_INVALIDARG;
            PD_RC_CHECK( rc, PDERROR, "invalid lock-type" ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnTransTryLockCS( const CHAR *pSpace, INT32 lockType,
                            _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      SDB_ASSERT ( pSpace, "space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB  can't be NULL" ) ;
      SDB_ASSERT ( dpsCB, "dpsCB  can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb  can't be NULL" ) ;
      UINT32 length = ossStrlen ( pSpace );
      PD_CHECK( (length > 0 && length <= DMS_SU_NAME_SZ), SDB_INVALIDARG,
                error, PDERROR, "invalid length of collectionspace name:%s",
                pSpace );

      rc = dmsCB->nameToSUAndLock( pSpace, suID, &su );
      PD_CHECK(( su != NULL && suID != DMS_INVALID_SUID), SDB_DMS_CS_NOTEXIST,
               error, PDERROR, "lock collection space(%s) failed(rc=%d)",
               pSpace, rc );
      logicCSID = su->LogicalCSID();
      dmsCB->suUnlock ( suID ) ;
      switch( lockType )
      {
      case DPS_TRANSLOCK_S:
            rc = pTransCB->transLockTryS( cb, logicCSID );
            break;
      case DPS_TRANSLOCK_X:
            rc = pTransCB->transLockTryX( cb, logicCSID );
            break;
      default:
            rc = SDB_INVALIDARG;
            PD_RC_CHECK( rc, PDERROR, "invalid lock-type" );
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnTransReleaseLock( const CHAR *pCollection,
                              _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      const CHAR *pCollectionShortName = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      UINT16 collectionID = DMS_INVALID_MBID;
      CHAR *pDot = NULL;
      CHAR *pDot1 = NULL;
      pDot = (CHAR *)ossStrchr( pCollection, '.' );
      pDot1 = (CHAR *)ossStrrchr( pCollection, '.' );
      PD_CHECK( (pDot == pDot1 && pCollection != pDot), SDB_INVALIDARG,
                error, PDERROR, "invalid format for collection name:%s, "
                "expected format:<collectionspace>[.<collectionname>]",
                pCollection );
      if ( pDot )
      {
         rc = rtnResolveCollectionNameAndLock( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID );
         PD_RC_CHECK( rc, PDERROR,
                     "Failed to resolve collection name(collection:%s, rc=%d)",
                      pCollection, rc );
         rc = su->data()->findCollection( pCollectionShortName, collectionID ) ;
         logicCSID = su->LogicalCSID();
         dmsCB->suUnlock( suID );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to find collection(collection:%s, rc=%d)",
                      pCollection, rc );
      }
      else
      {
         rc = dmsCB->nameToSUAndLock( pCollection, suID, &su );
         PD_CHECK( ( su != NULL && suID != DMS_INVALID_SUID),
                   SDB_DMS_CS_NOTEXIST, error, PDERROR,
                   "lock collection space(%s) failed(rc=%d)",
                   pCollection, rc ) ;
         logicCSID = su->LogicalCSID();
         dmsCB->suUnlock( suID );
      }
      pTransCB->transLockRelease( cb, logicCSID, collectionID );
   done:
      return rc;
   error:
      goto done;
   }

}
