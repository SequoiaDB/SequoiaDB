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
#include "dpsUtil.hpp"

namespace engine
{

   /// local define
   #define RTN_TRANS_ROLLBACK_RETRY_TIMES             ( 20 )
   #define RTN_TRANS_ROLLBACK_RETRY_INTERVAL          OSS_ONE_SEC

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSBEGIN, "rtnTransBegin" )
   INT32 rtnTransBegin( _pmdEDUCB * cb,
                        BOOLEAN isAutoCommit,
                        DPS_TRANS_ID specID )
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
         if ( DPS_INVALID_TRANS_ID != specID )
         {
            cb->setTransID( DPS_TRANS_SET_FIRSTOP( specID ) ) ;
         }
         else
         {
            cb->setTransID( sdbGetTransCB()->allocTransID( isAutoCommit ) ) ;
         }
         cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;

         if ( !sdbGetTransCB()->addTransCB( cb->getTransID(), cb ) )
         {
            PD_LOG( PDERROR, "Transaction(%s) is alredy exist",
                    dpsTransIDToString( cb->getTransID() ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      PD_LOG( PDINFO, "Begin transaction operations(ID:%s, IDAttr:%s)",
              dpsTransIDToString( cb->getTransID() ).c_str(),
              dpsTransIDAttrToString( cb->getTransID() ).c_str() ) ;

   done:
      PD_TRACE_EXIT ( SDB_RTNTRANSBEGIN ) ;
      return rc;
   error:
      goto done ;
   }

   INT32 rtnTransPreCommit( _pmdEDUCB *cb, UINT32 nodeNum,
                            const UINT64 *pNodes,
                            INT16 w,
                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      DPS_TRANS_ID curTransID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET firstTransLsn = DPS_INVALID_LSN_OFFSET ;
      UINT8 attr = DPS_TS_COMMIT_ATTR_PRE ;

      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      curTransID = cb->getTransID() ;
      preTransLsn = cb->getCurTransLsn() ;

      if ( curTransID == DPS_INVALID_TRANS_ID ||
           preTransLsn == DPS_INVALID_LSN_OFFSET )
      {
         goto done ;
      }

      if ( !dpsCB )
      {
         goto done ;
      }

      firstTransLsn = sdbGetTransCB()->getBeginLsn( curTransID ) ;
      SDB_ASSERT( firstTransLsn != DPS_INVALID_LSN_OFFSET,
                  "First transaction lsn can't be invalid" ) ;

      PD_LOG( PDINFO, "Execute pre-commit(ID:%s, LastLsn=%llu)",
              dpsTransIDToString( curTransID ).c_str(),
              preTransLsn ) ;

      rc = dpsTransCommit2Record( curTransID, preTransLsn, firstTransLsn,
                                  attr, &nodeNum, pNodes, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build pre-commit log:%d",rc ) ;
         goto error ;
      }

      info.setInfoEx( ~0, DMS_INVALID_CLID, DMS_INVALID_EXTENT, cb ) ;
      info.enableTrans() ;
      rc = dpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      dpsCB->writeData( info ) ;

      cb->setTransStatus( DPS_TRANS_WAIT_COMMIT ) ;

   done:
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSCOMMIT, "rtnTransCommit" )
   INT32 rtnTransCommit( _pmdEDUCB * cb, SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB_RTNTRANSCOMMIT ) ;
      SDB_ASSERT( cb, "cb can't be null" ) ;
      INT32 rc = SDB_OK ;
      UINT8 attr = 0 ;

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
         cb->setTransStatus( DPS_TRANS_COMMIT ) ;

         // make sure to commit meta-block statistics
         // NOTE: actually it is empty
         cb->getTransExecutor()->commitMBStats() ;

         sdbGetTransCB()->delTransCB( curTransID ) ;
         cb->setTransID( DPS_INVALID_TRANS_ID ) ;
         // release all transactions lock
         sdbGetTransCB()->transLockReleaseAll( cb ) ;
         // reduce the reservedLogSpace from dps for the transaction
         sdbGetTransCB()->releaseRBLogSpace( cb ) ;
         goto done ;
      }

      if ( !dpsCB )
      {
         goto done ;
      }

      if ( DPS_TRANS_WAIT_COMMIT == cb->getTransStatus() )
      {
         attr = DPS_TS_COMMIT_ATTR_SND ;
      }

      firstTransLsn = sdbGetTransCB()->getBeginLsn( curTransID ) ;
      SDB_ASSERT( firstTransLsn != DPS_INVALID_LSN_OFFSET,
                  "First transaction lsn can't be invalid" ) ;

      PD_LOG( PDINFO, "Execute commit(ID:%s, LastLsn=%llu)",
              dpsTransIDToString( curTransID ).c_str(),
              preTransLsn ) ;

      rc = dpsTransCommit2Record( curTransID, preTransLsn, firstTransLsn,
                                  attr, NULL, NULL, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build commit log:%d",rc ) ;
         goto error ;
      }

      info.setInfoEx( ~0, DMS_INVALID_CLID, DMS_INVALID_EXTENT, cb ) ;
      info.enableTrans() ;
      rc = dpsCB->prepare( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                   "log(rc=%d)", rc ) ;
      dpsCB->writeData( info ) ;

      cb->setTransStatus( DPS_TRANS_COMMIT ) ;

      // commit meta-block statistics
      // SHOULD commit this before release TX locks
      // the mbstat can be protected by TX locks
      cb->getTransExecutor()->commitMBStats() ;

      sdbGetTransCB()->delTransCB( curTransID ) ;
      cb->setTransID( DPS_INVALID_TRANS_ID ) ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      // clear all lsn mapping
      cb->getTransExecutor()->clearRecordMap() ;
      // release all transactions lock
      sdbGetTransCB()->transLockReleaseAll( cb ) ;

      // reduce the reservedLogSpace from dps for the transaction
      sdbGetTransCB()->releaseRBLogSpace( cb ) ;

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
      MAP_TRANS_PENDING_OBJ mapPendingObj ;

      cb->startTransRollback() ;
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

      PD_LOG ( PDEVENT, "Begin to rollback transaction[ID:%s, "
               "lastLsn:%llu]...", dpsTransIDToString( transID ).c_str(),
               curLsnOffset ) ;
      doRollback = TRUE ;

      cb->setTransID( rollbackID ) ;
      cb->setTransStatus( DPS_TRANS_ROLLBACK ) ;

      // read the log and rollback one by one
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
         PD_CHECK( curTransID == DPS_TRANS_GET_ID( transID ),
                   SDB_DPS_CORRUPTED_LOG, error,
                   PDERROR, "Failed to rollback(lsn=%llu, Log TransID:%llu, "
                   "Session TransID:%llu), the log is damaged",
                   curLsnOffset, curTransID, DPS_TRANS_GET_ID( transID ) ) ;

         // in cluster mode, when not primary, need add trans info to map
         if ( pmdGetKRCB()->isCBValue( SDB_CB_CLS ) && !pmdIsPrimary() )
         {
            sdbGetTransCB()->addTransInfo( transID,
                                           curLsnOffset,
                                           cb->getTransStatus() ) ;
            mapPendingObj.clear() ;
            rc = SDB_CLS_NOT_PRIMARY ;
            goto error ;
         }

         {
            cb->setRelatedTransLSN( curLsnOffset ) ;
            dpsLogRecord::iterator tmpitr =
               record.find(DPS_LOG_PUBLIC_PRETRANS ) ;
            if ( !tmpitr.valid() )
            {
               /// it is the first log.
               curLsnOffset = DPS_INVALID_LSN_OFFSET ;
            }
            else
            {
               curLsnOffset = *(( DPS_LSN_OFFSET *)tmpitr.value());
            }
            cb->setCurTransLsn( curLsnOffset ) ;

            /// when rollback failed, need to retry some times.
            /// But all the way do it failed, need to restart the db
            rc = replayer.rollbackTrans( ( dpsLogRecordHeader *)mb.offset(0),
                                         cb, mapPendingObj ) ;
            if ( rc )
            {
               ++retryTimes ;
               PD_LOG( PDERROR, "Rollback transaction[ID:%s, lsn=%llu, "
                       "time=%u] failed, rc: %d",
                       dpsTransIDToString( transID ).c_str(),
                       dpsLsn.offset,
                       retryTimes, rc ) ;
               if ( retryTimes >= RTN_TRANS_ROLLBACK_RETRY_TIMES )
               {
                  PD_LOG( PDSEVERE, "Rollback transaction failed, need to "
                          "restart the system" ) ;
                  PMD_RESTART_DB( rc ) ;
                  goto error ;
               }
               ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
               /// set the current lsn to last lsn
               curLsnOffset = cb->getRelatedTransLSN() ;
            }
            else
            {
               retryTimes = 0 ;
            }
         }
      }

   done:
      // complete the transaction whether success or not,
      // this avoid infinite recursion when rollback failed

      // rollback meta-block statistics
      cb->getTransExecutor()->rollbackMBStats() ;

      sdbGetTransCB()->delTransCB( transID ) ;
      cb->setTransID( DPS_INVALID_TRANS_ID ) ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      cb->setRelatedTransLSN( DPS_INVALID_LSN_OFFSET ) ;
      // clear all lsn mapping
      cb->getTransExecutor()->clearRecordMap() ;
      sdbGetTransCB()->transLockReleaseAll( cb ) ;

      // reduce the reservedLogSpace from dps for the transaction
      sdbGetTransCB()->releaseRBLogSpace( cb ) ;

      cb->stopTransRollback() ;

      if ( !mapPendingObj.empty() )
      {
         SDB_ASSERT( FALSE, "Transaction's pending object map is "
                     "not empty" ) ;
         PD_LOG( PDERROR, "Transaction(%s)'s pending object map"
                 " is not empty(size:%d)",
                 dpsTransIDToString( transID ).c_str(),
                 mapPendingObj.size() ) ;
      }

      if ( doRollback )
      {
         PD_LOG ( PDEVENT, "Rollback transaction(ID:%s, IDAttr:%s) finished "
                  "with rc[%d]", dpsTransIDToString( transID ).c_str(),
                  dpsTransIDAttrToString( transID ).c_str(),
                  rc ) ;
      }

#if defined ( _DEBUG )
      // only for debug, wait the group other node sync complete
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
      TRANS_MAP tmpTransMap ;
      DPS_LSN dpsLsn;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_TRANS_ID rollbackID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET curLsnOffset = DPS_INVALID_LSN_OFFSET ;
      UINT32 retryTimes = 0 ;
      _clsReplayer replayer( TRUE );
      _dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN ) ;

      pTransCB->cloneTransMap( tmpTransMap ) ;
      cb->startTransRollback() ;

      PD_LOG ( PDEVENT, "Begin to rollback all unfinished transactions[%d]...",
               tmpTransMap.size() ) ;

      while ( tmpTransMap.size() != 0 )
      {
         TRANS_MAP::iterator iterMap = tmpTransMap.begin();
         dpsTransBackInfo &transInfo = iterMap->second ;
         MAP_TRANS_PENDING_OBJ mapPendingObj ;
         transID = iterMap->first ;
         rollbackID = pTransCB->getRollbackID( transID ) ;
         curLsnOffset = transInfo._lsn ;
         cb->setTransID( rollbackID ) ;

         PD_LOG( PDEVENT, "Begin to rollback transaction[ID:%s, "
                 "LastLSN: %llu]...", dpsTransIDToString( transID ).c_str(),
                 curLsnOffset ) ;

         if ( DPS_INVALID_LSN_OFFSET != transInfo._curLSNWithRBPending )
         {
            PD_LOG( PDEVENT, "Transaction[ID:%s] is rollback pending, "
                    "restart from previous non pending LSN: %llu, "
                    "current pending LSN: %llu",
                    dpsTransIDToString( transID ).c_str(),
                    curLsnOffset, transInfo._curLSNWithRBPending ) ;
         }

         while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )
         {
            if ( !pTransCB->isDoRollback() )
            {
               PD_LOG ( PDEVENT, "Rollback is interrupted" ) ;
               rc = SDB_INTERRUPT ;
               goto error ;
            }
            dpsLogRecordHeader *recordHeader = NULL ;
            dpsLogRecord record ;
            mb.clear() ;
            dpsLsn.offset = curLsnOffset;
            rc = pDpsCB->search( dpsLsn, &mb ) ;
            if ( rc )
            {
               // don't return,
               // stop rollback current transaction,
               // go on to rollback other transaction
               PD_LOG ( PDERROR, "Rollback failed, failed to get the "
                        "log( offset =%llu, version=%d, rc=%d)",
                        curLsnOffset, dpsLsn.version, rc ) ;
               break ;
            }
            recordHeader = (dpsLogRecordHeader *)( mb.offset( 0 ) ) ;
            rc = record.load( mb.offset( 0 ) ) ;
            if ( rc )
            {
               // don't return,
               // stop rollback current transaction,
               // go on to rollback other transaction
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
               // don't return,
               // stop rollback current transaction,
               // go on to rollback other transaction
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

               // rollback pending record had been replayed earlier,
               // but interrupted by primary switch or reboot,
               // just reconstruct pending objects and move to next
               if ( DPS_INVALID_LSN_OFFSET != transInfo._curLSNWithRBPending &&
                    DPS_INVALID_LSN_OFFSET != curLsnOffset &&
                    curLsnOffset >= transInfo._curLSNWithRBPending )
               {
                  BOOLEAN removeOnly =
                        transInfo._curNonPendingLSN.count( curLsnOffset ) > 0 ;
                  // found a DPS log already rollbacked, but which may create
                  // or resolve a pending object, so try to rebuild pending
                  // objects from original DPS log
                  PD_LOG( PDDEBUG, "Rollback transaction [ID: %s] meets "
                          "older rollbacked record LSN [%llu], "
                          "created pending object: %s, "
                          "current pending LSN: [%llu]",
                          dpsTransIDToString( transID ).c_str(),
                          recordHeader->_lsn,
                          removeOnly ? "FALSE" : "TRUE",
                          transInfo._curLSNWithRBPending ) ;
                  rc = replayer.replayRBPending( recordHeader, removeOnly, cb,
                                                 mapPendingObj ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to replay rollback "
                               "pending record LSN [%llu], rc: %d",
                               recordHeader->_lsn, rc ) ;
                  continue ;
               }

               /// when rollback failed, need to retry some times.
               /// But all the way do it failed, need to restart the db
               rc = replayer.rollbackTrans( ( dpsLogRecordHeader *)mb.offset(0),
                                            cb, mapPendingObj ) ;
               if ( rc )
               {
                  ++retryTimes ;
                  PD_LOG( PDERROR, "Rollback transaction[ID:%s, "
                          "lsn=%llu, time=%u] failed,  rc: %d",
                          dpsTransIDToString( transID ).c_str(),
                          dpsLsn.offset, retryTimes, rc ) ;
                  if ( retryTimes >= RTN_TRANS_ROLLBACK_RETRY_TIMES )
                  {
                     PD_LOG( PDSEVERE, "Rollback transaction failed, need to "
                             "restart the system" ) ;
                     PMD_RESTART_DB( rc ) ;
                     goto error ;
                  }
                  ossSleep( RTN_TRANS_ROLLBACK_RETRY_INTERVAL ) ;
                  /// restore cur lsn to last lsn and retry
                  curLsnOffset = cb->getRelatedTransLSN() ;
               }
               else
               {
                  retryTimes = 0 ;
                  pTransCB->updateTransInfo( transInfo,
                                             DPS_TRANS_ROLLBACK,
                                             curLsnOffset,
                                             cb->isTransRBPending() ) ;
               }
            }
         } /// while ( curLsnOffset != DPS_INVALID_LSN_OFFSET )

         if ( !mapPendingObj.empty() )
         {
            SDB_ASSERT( FALSE, "Transaction's pending object map is "
                        "not empty" ) ;
            PD_LOG( PDERROR, "Transaction(%s)'s pending object map"
                    " is not empty(size:%d)",
                    dpsTransIDToString( transID ).c_str(),
                    mapPendingObj.size() ) ;
         }
         else if ( cb->isTransRBPending() )
         {
            SDB_ASSERT( FALSE, "Transaction's rollback pending" ) ;
            PD_LOG( PDERROR, "Transaction(%s)'s rollback pending",
                    dpsTransIDToString( transID ).c_str() ) ;
         }

         /// remove the transaction
         pTransMap->erase( iterMap->first ) ;
         tmpTransMap.erase( iterMap ) ;
         PD_LOG( PDEVENT, "Rollback transaction(ID:%s, IDAttr:%s) finished "
                 "with rc[%d]", dpsTransIDToString( transID ).c_str(),
                 dpsTransIDAttrToString( transID ).c_str(),
                 rc ) ;
      } /// while ( tmpTransMap.size() != 0 )

   done:
      // clear transaction metablock statistics
      sdbGetDMSCB()->fixTransMBStats() ;

      // clear all lsn mapping
      cb->getTransExecutor()->clearRecordMap() ;
      pTransCB->transLockReleaseAll( cb ) ;
      pTransCB->stopRollbackTask() ;

      cb->stopTransRollback() ;

      PD_LOG ( PDEVENT, "Rollback all unfinished transactions finished with "
               "rc[%d]", rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRANSSAVEWAITCOMMIT, "rtnTransSaveWaitCommit" )
   INT32 rtnTransSaveWaitCommit ( _pmdEDUCB * cb, SDB_DPSCB * dpsCB,
                                  BOOLEAN & savedAsWaitCommit )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNTRANSSAVEWAITCOMMIT ) ;

      SDB_ASSERT( cb, "cb can't be null" ) ;

      DPS_LSN_OFFSET curLsnOffset = cb->getCurTransLsn() ;
      DPS_TRANS_ID transID = cb->getTransID() ;

      savedAsWaitCommit = FALSE ;

      if ( DPS_INVALID_TRANS_ID == transID ||
           DPS_INVALID_LSN_OFFSET == curLsnOffset ||
           !dpsCB ||
           !pmdGetKRCB()->isCBValue( SDB_CB_CLS ) ||
           pmdIsPrimary() )
      {
         // no trans ID/LSN, or no dps, or not cluster mode,
         // or is primary, in those cases, should not clear, goto done
         goto done ;
      }

      // in cluster mode, when not primary, need add trans info to map
      sdbGetTransCB()->addTransInfo( transID, curLsnOffset,
                                     cb->getTransStatus() ) ;

      // just clear meta-block statistics
      cb->getTransExecutor()->clearMBStats() ;

      sdbGetTransCB()->delTransCB( transID ) ;
      cb->setTransID( DPS_INVALID_TRANS_ID ) ;
      cb->setCurTransLsn( DPS_INVALID_LSN_OFFSET ) ;
      cb->setRelatedTransLSN( DPS_INVALID_LSN_OFFSET ) ;

      // clear all lsn mapping
      cb->getTransExecutor()->clearRecordMap() ;
      sdbGetTransCB()->transLockReleaseAll( cb ) ;

      // reduce the reservedLogSpace from dps for the transaction
      sdbGetTransCB()->releaseRBLogSpace( cb ) ;

      savedAsWaitCommit = TRUE ;

      PD_LOG ( PDEVENT, "Save transaction(ID:%s, IDAttr:%s) as wait-commit "
               "finished", dpsTransIDToString( transID ).c_str(),
               dpsTransIDAttrToString( transID ).c_str() ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNTRANSSAVEWAITCOMMIT, rc ) ;
      return rc ;
   }

   INT32 rtnTransTryOrTestLockCL( const CHAR *pCollection,
                                  INT32 lockType,
                                  BOOLEAN isTest,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      const CHAR *pCollectionShortName = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      UINT16 collectionID = DMS_INVALID_MBID ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB  can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb  can't be NULL" ) ;
      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID );
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name"
                   "(collection:%s, rc=%d)", pCollection, rc ) ;
      rc = su->data()->findCollection( pCollectionShortName, collectionID ) ;
      logicCSID = su->LogicalCSID() ;
      dmsCB->suUnlock ( suID );
      PD_RC_CHECK( rc, PDERROR, "Failed to find the collection"
                   "(collection:%s, rc=%d)", pCollection, rc ) ;
      switch( lockType )
      {
      case DPS_TRANSLOCK_S:
         if ( isTest )
         {
            rc = pTransCB->transLockTestS( cb, logicCSID, collectionID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryS( cb, logicCSID, collectionID ) ;
         }
            break;
      case DPS_TRANSLOCK_X:
         if ( isTest )
         {
            rc = pTransCB->transLockTestX( cb, logicCSID, collectionID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryX( cb, logicCSID, collectionID ) ;
         }
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

   INT32 rtnTransTryOrTestLockCS( const CHAR *pSpace,
                                  INT32 lockType,
                                  BOOLEAN isTest,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      UINT32 logicCSID = ~0;
      dmsStorageUnit *su = NULL;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      SDB_ASSERT ( pSpace, "space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB  can't be NULL" ) ;
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
         if ( isTest )
         {
            rc = pTransCB->transLockTestS( cb, logicCSID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryS( cb, logicCSID ) ;
         }
         break;
      case DPS_TRANSLOCK_X:
         if ( isTest )
         {
            rc = pTransCB->transLockTestX( cb, logicCSID ) ;
         }
         else
         {
            rc = pTransCB->transLockTryX( cb, logicCSID ) ;
         }
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
