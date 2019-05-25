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

   Source File Name = clsReplSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

*******************************************************************************/

#include "clsReplSession.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "rtnRecover.hpp"
#include "pmdStartup.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   const UINT32 CLS_SYNC_DEF_LEN = 1024 ;
   const UINT16 CLS_SYNC_INTERVAL = 2000 ;
   const UINT16 CLS_CONSULT_INTERVAL = 5000 ;

   #define CLS_REPL_MAX_TIME           (2)

   /*
      _clsReplDstSession implement
   */
   BEGIN_OBJ_MSG_MAP( _clsReplDstSession , _pmdAsyncSession )
      ON_MSG( MSG_CLS_SYNC_RES, handleSyncRes )
      ON_MSG( MSG_CLS_SYNC_NOTIFY, handleNotify )
      ON_MSG( MSG_CLS_CONSULTATION_RES, handleConsultRes )
   END_OBJ_MSG_MAP()

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN__CLSDSTREPSN, "_clsReplDstSession::_clsReplDstSession" )
   _clsReplDstSession::_clsReplDstSession ( UINT64 sessionID )
      :_pmdAsyncSession ( sessionID ),
       _mb( CLS_SYNC_DEF_LEN ),
       _status( CLS_SESSION_STATUS_SYNC ),
       _quit( FALSE ),
       _addFSSession(0),
       _timeout( 0 ),
       _consultLsn(),
       _lastRecvConsultLsn()
   {
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN__CLSDSTREPSN );
      _logger = pmdGetKRCB()->getDPSCB() ;
      _sync = sdbGetReplCB()->syncMgr() ;
      _repl = sdbGetReplCB() ;
      _pReplBucket = _repl->getBucket() ;

      _requestID = 1 ;
      _syncFailedNum = 0 ;
      _isFirstToSync = TRUE ;

      _syncSrc.value = MSG_INVALID_ROUTEID ;
      _lastSyncNode.value = MSG_INVALID_ROUTEID ;

      _fullSyncIgnoreTimes = 0 ;

      PD_TRACE_EXIT ( SDB__CLSDSTREPSN__CLSDSTREPSN );
   }

   _clsReplDstSession::~_clsReplDstSession ()
   {
   }

   SDB_SESSION_TYPE _clsReplDstSession::sessionType() const
   {
      return SDB_SESSION_REPL_DST ;
   }

   EDU_TYPES _clsReplDstSession::eduType () const
   {
      return EDU_TYPE_REPLAGENT ;
   }

   BOOLEAN _clsReplDstSession::timeout( UINT32 interval )
   {
      return _quit ;
   }

   void _clsReplDstSession::onRecieve( const NET_HANDLE netHandle,
                                       MsgHeader *msg )
   {
      if ( MSG_INVALID_ROUTEID != _syncSrc.value )
      {
         _repl->aliveNode( _syncSrc ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN_ONTIMER, "_clsReplDstSession::onTimer" )
   void _clsReplDstSession::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN_ONTIMER ) ;
      _timeout += interval ;

      _selector.timeout( interval ) ;

      if ( _quit )
      {
         goto done ;
      }
      else if ( CLS_SESSION_STATUS_CONSULT == _status &&
                _timeout < CLS_CONSULT_INTERVAL )
      {
         goto done ;
      }
      else if ( _timeout < CLS_SYNC_INTERVAL )
      {
         goto done ;
      }
      _timeout = 0 ;

      if ( !_sync->isReadyToReplay() && pmdGetStartup().isOK() )
      {
         _isFirstToSync = TRUE ;
         _status = CLS_SESSION_STATUS_SYNC ;
         ++_requestID ;
         goto done ;
      }
      else if ( _isFirstToSync &&
                CLS_SESSION_STATUS_FULL_SYNC != _status &&
                pmdGetKRCB()->getEDUMgr()->getWritingEDUCount() > 0 )
      {
         PD_LOG( PDWARNING, "Session[%s]: Has some writing edus don't "
                 "exit, can't to sync", sessionName() ) ;
         goto done ;
      }

      _isFirstToSync = FALSE ;

      if ( MSG_INVALID_ROUTEID != _syncSrc.value &&
           !_repl->isAlive ( _syncSrc ) )
      {
         PD_LOG ( PDWARNING, "Session[%s]: Peer node sharing-break",
                  sessionName() ) ;
         _selector.addToBlakList ( _syncSrc ) ;
         _selector.clearSrc () ;
         _syncSrc = _selector.src() ;
      }

      if ( CLS_BUCKET_WAIT_ROLLBACK == _pReplBucket->getStatus() )
      {
         _pReplBucket->waitEmptyAndRollback() ;
         DPS_LSN expectLSN = _pReplBucket->completeLSN() ;
         INT32 rcTmp = _logger->move( expectLSN.offset, expectLSN.version ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to move lsn to "
                    "[%u, %llu], rc: %d", sessionName(), expectLSN.version,
                    expectLSN.offset, rcTmp ) ;
            _status = CLS_SESSION_STATUS_FULL_SYNC ;
         }
         else
         {
            PD_LOG( PDEVENT, "Session[%s]: Move lsn to[%u, %llu]",
                    sessionName(), expectLSN.version, expectLSN.offset ) ;
         }
      }

      if ( CLS_SESSION_STATUS_SYNC == _status )
      {
         _sendSyncReq() ;
      }
      else if ( CLS_SESSION_STATUS_CONSULT == _status )
      {
         _sendConsultReq() ;
      }
      else if ( CLS_SESSION_STATUS_FULL_SYNC == _status )
      {
         _fullSync () ;
      }
      else
      {
      }

   done:
      PD_TRACE1 ( SDB__CLSDSTREPSN_ONTIMER, PD_PACK_INT(_quit) ) ;
      PD_TRACE_EXIT ( SDB__CLSDSTREPSN_ONTIMER );
      return  ;
   }

   void _clsReplDstSession::_onAttach()
   {
      if ( !pmdGetStartup().isOK() )
      {
         PD_LOG( PDEVENT, "Session[%s]: The db data is abnormal, "
                 "need to synchronize full data", sessionName() ) ;
         _status = CLS_SESSION_STATUS_FULL_SYNC ;
      }

      _pReplBucket->reset() ;
   }

   void _clsReplDstSession::_onDetach()
   {
      if ( _pReplBucket->waitEmptyAndRollback() )
      {
         DPS_LSN expectLSN = _pReplBucket->completeLSN() ;
         INT32 rcTmp = _logger->move( expectLSN.offset, expectLSN.version ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to move lsn to "
                    "[%u, %llu], rc: %d", sessionName(), expectLSN.version,
                    expectLSN.offset, rcTmp ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Session[%s]: Move lsn to [%u, %llu]",
                    sessionName(), expectLSN.version, expectLSN.offset ) ;
         }
      }
      _pReplBucket->close() ;

      if ( CLS_SESSION_STATUS_FULL_SYNC != _status &&
           PMD_IS_DB_UP() )
      {
         pmdGetKRCB()->getClsCB()->startInnerSession( CLS_REPL,
                                                      CLS_TID_REPL_SYC ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN_HNDSYNCRES, "_clsReplDstSession::handleSyncRes" )
   INT32 _clsReplDstSession::handleSyncRes( NET_HANDLE handle, MsgHeader* header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN_HNDSYNCRES );

      _repl->getSyncEmptyEvent()->reset() ;

      MsgReplSyncRes *msg = ( MsgReplSyncRes * )header ;
      if ( CLS_SESSION_STATUS_SYNC != _status )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: Status[%d] not expect[%d], "
                  "ignore", sessionName(), _status ,
                  CLS_SESSION_STATUS_SYNC ) ;
         goto done ;
      }
      else if ( !_sync->isReadyToReplay() )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: Not ready to replay, ignore",
                  sessionName() ) ;
         goto done ;
      }
      else if ( msg->identity.value != _syncSrc.value )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: Node id[%d] is not expect[%d], "
                  "ignore", sessionName(), msg->identity.columns.nodeID,
                  _syncSrc.columns.nodeID ) ;
         goto done ;
      }
      else if ( msg->header.header.requestID != _requestID )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: RequestID[%lld] not "
                  "expected[%lld], ignore", sessionName(),
                  msg->header.header.requestID, _requestID ) ;
         goto done ;
      }
      else
      {
         _selector.clearTime() ;
         ++_requestID ;

         if ( !PMD_IS_DB_NORMAL() )
         {
            PD_LOG ( PDDEBUG, "Session[%s]: Repl status[%d] is not "
                     "normal, ignore", sessionName(), PMD_DB_STATUS() ) ;
            goto done ;
         }
      }

      if ( SDB_OK == msg->header.res )
      {
         _lastSyncNode.value = msg->identity.value ;

         UINT32 num = 0 ;
         rc = _replayLog( (const CHAR *)
               ( ( ossValuePtr )(&(msg->header)) + sizeof( MsgReplSyncRes ) ),
               msg->header.header.messageLength - sizeof( MsgReplSyncRes ),
               num ) ;
         PD_LOG ( PDDEBUG, "Session[%s]: ReplyLog num:%d, rc: %d",
                  sessionName(), num, rc ) ;

         if ( 0 != num )
         {
            _syncFailedNum = 0 ;
            rc = SDB_OK ;

            _sendSyncReq() ;
            _selector.clearBlacklist () ;
         }
         else if ( SDB_OK == rc )
         {
            if ( _repl->getPrimary().value != _syncSrc.value )
            {
               _syncFailedNum = 0 ;

               _selector.addToBlakList( _syncSrc ) ;
               _selector.clearSrc() ;

               _timeout = CLS_SYNC_INTERVAL ;
            }

            if ( _pReplBucket->maxReplSync() > 0 )
            {
               DPS_LSN completeLSN = _pReplBucket->completeLSN() ;
               if ( !completeLSN.invalid() &&
                    _completeLSN.offset != completeLSN.offset )
               {
                  _sendSyncReq( &completeLSN ) ;
               }
               else if ( !completeLSN.invalid() &&
                         completeLSN.offset != _logger->expectLsn().offset )
               {
                  if ( SDB_OK == _pReplBucket->waitSubmit( OSS_ONE_SEC ) )
                  {
                     _sendSyncReq () ;
                  }
                  else
                  {
                     _timeout += OSS_ONE_SEC ;
                  }
               }
            }
         }
      }

      if ( SDB_OK != msg->header.res || SDB_OK != rc )
      {
         _syncFailedNum++ ;
         if ( _syncFailedNum < _repl->groupSize() - 1 )
         {
            _selector.addToBlakList( _syncSrc ) ;
            _selector.clearSrc() ;
            _lastSyncNode.value = MSG_INVALID_ROUTEID ;

            _sendSyncReq() ;
         }
         else
         {
            if ( _pReplBucket->maxReplSync() > 0 )
            {
               PD_LOG( PDEVENT, "Session[%s]: Repl bucket info[%s]",
                       sessionName(),
                       _pReplBucket->toBson().toString().c_str() ) ;
            }
            _sendConsultReq() ;
         }
      }
      else
      {
         if ( sdbGetTransCB()->isNeedSyncTrans() )
         {
            sdbGetTransCB()->syncTransInfoFromLocal( msg->oldestTransLsn ) ;
            sdbGetTransCB()->setIsNeedSyncTrans( FALSE ) ;
         }
      }

   done:
      _repl->getSyncEmptyEvent()->signalAll() ;
      PD_TRACE_EXITRC ( SDB__CLSDSTREPSN_HNDSYNCRES, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN_HNDNTF, "_clsReplDstSession::handleNotify" )
   INT32 _clsReplDstSession::handleNotify( NET_HANDLE handle, MsgHeader* header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN_HNDNTF );
      if ( CLS_SESSION_STATUS_SYNC != _status )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: Status[%d] not expected, ignore",
                  sessionName(), _status, CLS_SESSION_STATUS_SYNC ) ;
         goto done ;
      }

      _sendSyncReq() ;
   done:
      PD_TRACE_EXIT ( SDB__CLSDSTREPSN_HNDNTF );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN_HNDSSTRES, "_clsReplDstSession::handleConsultRes" )
   INT32 _clsReplDstSession::handleConsultRes( NET_HANDLE handle,
                                               MsgHeader *header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN_HNDSSTRES );

      if ( (UINT32)header->messageLength < sizeof( _MsgReplConsultationRes ) )
      {
         PD_LOG( PDWARNING, "Session[%s]: Consultation responses message "
                 "length[%d] is less than %d", header->messageLength,
                 sizeof( _MsgReplConsultationRes ) ) ;
         goto done ;
      }
      else if ( CLS_SESSION_STATUS_CONSULT != _status )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: Status[%d] not expected[%d], "
                  "ignore", sessionName(), _status,
                  CLS_SESSION_STATUS_CONSULT ) ;
         goto done ;
      }
      else if ( !_sync->isReadyToReplay() )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: Not ready to replay, ignore",
                  sessionName() ) ;
         goto done ;
      }
      else if ( header->requestID != _requestID )
      {
         PD_LOG ( PDDEBUG, "Session[%s]: RequestID[%lld] not exptected "
                  "[%lld], ignore", sessionName(), header->requestID,
                  _requestID ) ;
         goto done ;
      }
      else
      {
         ++_requestID ;
      }

      {
      _MsgReplConsultationRes *msg = ( _MsgReplConsultationRes * )header ;
      if ( SDB_OK != msg->header.res )
      {
         PD_LOG( PDEVENT, "Session[%s]: Consult failed[%d], need to "
                 "synchronize full data", sessionName(), msg->header.res ) ;
         _fullSync() ;
         goto done ;
      }
      else
      {
         PD_LOG ( PDEVENT, "Session[%s]: Consult returnTo "
                  "LSN[ver:%d, offset:%lld]", sessionName(),
                  msg->returnTo.version, msg->returnTo.offset ) ;

         BOOLEAN bValid = TRUE ;
         _selector.clearTime() ;
         _mb.clear() ;
         DPS_LSN curLsn = _logger->getCurrentLsn() ;

         if ( msg->returnTo.invalid() )
         {
            PD_LOG ( PDWARNING, "Session[%s]: Consult returnTo lsn is "
                     "invalid, need to synchronize full data",
                     sessionName() ) ;
            _fullSync() ;
            goto done ;
         }

         _lastRecvConsultLsn = msg->returnTo ;

         if ( SDB_OK != _logger->search( msg->returnTo, &_mb ) )
         {
            bValid = FALSE ;
         }
         else if ( msg->hashValue != ossHash( _mb.offset(0), _mb.length() ) )
         {
            bValid = FALSE ;
            _consultLsn = msg->returnTo ;
         }

         if ( !bValid )
         {
            PD_LOG ( PDINFO, "Session[%s]: Consult Lsn[%d,%lld], "
                     "curLsn[%d,%lld]", sessionName(), _consultLsn.version,
                     _consultLsn.offset, curLsn.version, curLsn.offset ) ;

            DPS_LSN search = _consultLsn ;
            UINT32 count = 0 ;
            do
            {
               _mb.clear() ;
               if ( SDB_OK != _logger->searchHeader( search, &_mb ) )
               {
                  PD_LOG ( PDWARNING, "Session[%s]: No find the lsn less "
                           "than(offset:%lld, version:%d), need to "
                           "synchronize full data", sessionName(),
                           msg->returnTo.offset, msg->returnTo.version ) ;
                  _fullSync () ;
                  goto done ;
               }
               _consultLsn.offset = ((dpsLogRecordHeader *)
                                      ( _mb.offset(0) ))->_lsn ;
               _consultLsn.version = ((dpsLogRecordHeader *)
                                      ( _mb.offset(0) ))->_version ;
               search.offset = ((dpsLogRecordHeader *)
                                (_mb.offset(0)))->_preLsn ;
               ++count ;
            } while ( _consultLsn.compareOffset( msg->returnTo ) >= 0 ||
                      _consultLsn.compareVersion( msg->returnTo.version ) > 0 ||
                      count <= 1 ) ;

            _sendConsultReq () ;
            goto done ;
         }
         else
         {
            UINT32 rollbackNum = 0 ;
            dpsLogRecordHeader *pLogHeader = NULL ;
            DPS_LSN rollback = _logger->getCurrentLsn() ;
            DPS_LSN point ;
            point.offset = ((dpsLogRecordHeader *)( _mb.offset(0) ))->_lsn ;
            point.version = ((dpsLogRecordHeader *)( _mb.offset(0) ))->_version ;

            if ( 0 != point.compareVersion( msg->returnTo.version ) )
            {
               PD_LOG( PDERROR, "Session[%s]: The local searched "
                       "lsn[%u,%lld] is not the same with consult "
                       "lsn[%u,%lld], need to synchronize full data",
                       sessionName(), point.version, point.offset,
                       msg->returnTo.version, msg->returnTo.offset ) ;
               _fullSync() ;
               goto done ;
            }

            while ( TRUE )
            {
               _mb.clear() ;
               rc = _logger->search( rollback, &_mb ) ;
               SDB_ASSERT( SDB_OK == rc,
                           "search should always be successful" ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Session[%s]: Search lsn[%lld, %d] "
                          "failed, rc: %d, need to synchronize full data",
                          sessionName(), rollback.offset, rollback.version,
                          rc ) ;
                  _fullSync() ;
                  goto done ;
               }
               pLogHeader = ( dpsLogRecordHeader* )_mb.offset( 0 ) ;

               if ( 0 == point.compareOffset( rollback.offset ) )
               {
                  break ;
               }
               else if ( SDB_OK != _rollback( _mb.offset( 0 ) ) )
               {
                  PD_LOG( PDERROR, "Session[%s]: Rollback lsn[%lld, %d] "
                          "failed, need to synchronize full data",
                          sessionName(), rollback.offset, rollback.version  ) ;
                  _fullSync() ;
                  goto done ;
               }
               else
               {
                  ++rollbackNum ;
                  rollback.offset = pLogHeader->_preLsn ;
               }
            }

            if ( SDB_OK != _logger->move( point.offset + pLogHeader->_length,
                                          point.version ) )
            {
               PD_LOG( PDERROR, "Session[%s]: Rollback log failed, "
                       "need to synchronize full data", sessionName() ) ;
               _fullSync() ;
               goto done ;
            }
            PD_LOG( PDEVENT, "Session[%s]: Complete consult and "
                    "rollbacked %u records, current lsn[%u, %lld], "
                    "expect lsn[%u, %lld]", sessionName(), rollbackNum,
                    point.version, point.offset, point.version,
                    point.offset + pLogHeader->_length ) ;

            _status = CLS_SESSION_STATUS_SYNC ;
            ++_requestID ;
            _syncFailedNum = 0 ;
            _consultLsn.reset() ;
            _lastRecvConsultLsn.reset() ;
         }
      }
      }
   done:
      if ( CLS_SESSION_STATUS_SYNC == _status &&
           sdbGetTransCB()->isNeedSyncTrans() )
      {
         DPS_LSN beginLsn = _logger->getStartLsn() ;
         sdbGetTransCB()->syncTransInfoFromLocal( beginLsn.offset ) ;
         sdbGetTransCB()->setIsNeedSyncTrans( FALSE ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSDSTREPSN_HNDSSTRES ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN__FULLSYNC, "_clsReplDstSession::_fullSync" )
   void _clsReplDstSession::_fullSync()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN__FULLSYNC ) ;
      clsCB *pClsCB = pmdGetKRCB()->getClsCB() ;
      pmdOptionsCB *optionCB = pmdGetKRCB()->getOptionCB() ;
      BOOLEAN needNotify = FALSE ;

      _status = CLS_SESSION_STATUS_FULL_SYNC ;

      if ( SDB_DB_NORMAL != PMD_DB_STATUS() &&
           SDB_DB_FULLSYNC != PMD_DB_STATUS() )
      {
         PD_LOG( PDINFO, "Repl status is[%d], can't inital full sync",
                 PMD_DB_STATUS() ) ;
         goto done ;
      }
      else if ( sdbGetTransCB()->getTransCBSize() != 0 )
      {
         PD_LOG( PDINFO, "Has %d edus in rollback or commit, can't intial "
                 "full sync", sdbGetTransCB()->getTransCBSize() ) ;
         goto done ;
      }

      SDB_ASSERT( 0 < sdbGetReplCB()->groupSize(), "impossible" ) ;

      if ( PMD_OPT_VALUE_NONE == optionCB->getDataErrorOp() )
      {
         if ( PMD_IS_DB_AVAILABLE() )
         {
            PMD_SET_DB_STATUS( SDB_DB_FULLSYNC ) ;
         }
         if ( 0 == ( _fullSyncIgnoreTimes % 150 ) )
         {
            PD_LOG( PDERROR, "The node's data is error, forbidden "
                    "fullsync by configure" ) ;
            needNotify = TRUE ;
         }
         ++_fullSyncIgnoreTimes ;
         goto done ;
      }
      else if ( PMD_OPT_VALUE_SHUTDOWN == optionCB->getDataErrorOp() )
      {
         PD_LOG( PDSEVERE, "The node's data is error, shutdown by configure" ) ;
         PMD_SHUTDOWN_DB( SDB_CLS_FULL_SYNC ) ;
         needNotify = TRUE ;
         goto done ;
      }

      _fullSyncIgnoreTimes = 0 ;
      if ( SDB_DB_FULLSYNC == PMD_DB_STATUS() )
      {
         PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
      }

      if ( 1 >=  pClsCB->getReplCB()->groupSize() || pmdIsPrimary() )
      {
         PD_LOG( PDWARNING, "Session[%s]: Group size is one or the node "
                 "begin to primary, begin to rebuild database",
                 sessionName() ) ;

         rtnDBRebuilder rebuilder ;

         PMD_SET_DB_STATUS( SDB_DB_REBUILDING ) ;
         pClsCB->getReplCB()->getFaultEvent()->signalAll( SDB_RTN_IN_REBUILD ) ;
         eduCB()->getEDUMgr()->interruptWritingEDUS() ;
         pClsCB->getShardRouteAgent()->disconnectAll() ;
         rc = rebuilder.doOpr( eduCB() ) ;
         pClsCB->getReplCB()->getFaultEvent()->reset() ;
         PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         _status = CLS_SESSION_STATUS_SYNC ;
         ++_requestID ;
         pClsCB->getReplCB()->voteMachine()->force( CLS_ELECTION_STATUS_SEC ) ;
      }
      else
      {
         _quit = TRUE ;

         if ( _addFSSession.compareAndSwap( 0, 1 ) )
         {
            pClsCB->startInnerSession( CLS_REPL, CLS_TID_REPL_FS_SYC ) ;
            PD_LOG( PDEVENT, "Session[%s]: Start the synchronization of full",
                    sessionName() ) ;
         }
      }

   done:
      if ( needNotify )
      {
         BOOLEAN hasSend = FALSE ;
         /* Notify the node status to the primary node */
         MsgRouteID primaryID = _repl->getPrimary() ;
         if ( MSG_INVALID_ROUTEID != primaryID.value )
         {
            MsgClsNodeStatusNotify ntyMsg ;
            ntyMsg.status = SDB_DB_FULLSYNC ;
            if ( SDB_OK == routeAgent()->syncSend( primaryID,
                                                   (void*)&ntyMsg ) )
            {
               hasSend = TRUE ;
            }
         }
         if ( !hasSend )
         {
            _fullSyncIgnoreTimes = 0 ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSDSTREPSN__FULLSYNC ) ;
      return ;
   error:
      PD_LOG ( PDSEVERE, "Session[%s]: Local database rebuild failed "
               "with %d", sessionName(), rc ) ;
      PMD_SHUTDOWN_DB( rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN__RLBCK, "_clsReplDstSession::_rollback" )
   INT32 _clsReplDstSession::_rollback( const CHAR *log )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN__RLBCK );
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;

      const dpsLogRecordHeader *header = (const dpsLogRecordHeader *)log;
      PD_LOG( PDDEBUG, "Session[%s]: Begin to rollback lsn:[%lld, %d]",
              sessionName(), header->_lsn, header->_version ) ;
      rc = _replayer.rollback( header, eduCB() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to rollback, lsn:[%lld, %d], rc = %d",
                  header->_lsn, header->_version, rc ) ;
         goto error ;
      }

      if ( pTransCB && pTransCB->isTransOn() &&
           !pTransCB->isNeedSyncTrans() )
      {
         dpsLogRecord record ;
         record.load( log ) ;
         if ( !pTransCB->rollbackTransInfoFromLog( record ) )
         {
            pTransCB->setIsNeedSyncTrans( TRUE ) ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSDSTREPSN__RLBCK, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN__SNDSYNCREQ, "_clsReplDstSession::_sendSyncReq" )
   void _clsReplDstSession::_sendSyncReq( DPS_LSN *pCompleteLSN )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN__SNDSYNCREQ );

      _syncSrc = _selector.selected() ;

      if ( MSG_INVALID_ROUTEID != _syncSrc.value )
      {
         _timeout = 0 ;

         _MsgReplSyncReq msg ;
         msg.header.TID = CLS_TID( _sessionID ) ;
         msg.header.requestID = _requestID ;
         msg.next = _logger->expectLsn() ;
         msg.needData = PMD_IS_DB_NORMAL() ? 1 : 0 ;

         if ( msg.needData &&
              _lastSyncNode.value != _syncSrc.value &&
              !_logger->getCurrentLsn().invalid() )
         {
            msg.next = _logger->getCurrentLsn() ;
         }

         if ( pCompleteLSN )
         {
            _completeLSN = *pCompleteLSN ;
            msg.completeNext = _completeLSN ;
         }
         else if ( _pReplBucket->maxReplSync() > 0 )
         {
            _completeLSN = _pReplBucket->completeLSN() ;
            if ( !_completeLSN.invalid() )
            {
               msg.completeNext = _completeLSN ;
            }
         }

         msg.identity = routeAgent()->localID() ;
         routeAgent()->syncSend( _syncSrc, &msg ) ;
         PD_LOG( PDDEBUG, "Session[%s]: Send sync req to [node: %d, "
                 "group:%d], lsn: [%llu][%u], complete lsn: [%lld][%u]",
                 sessionName(), _syncSrc.columns.nodeID,
                 _syncSrc.columns.groupID, msg.next.offset, msg.next.version,
                 msg.completeNext.offset, msg.completeNext.version ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSDSTREPSN__SNDSYNCREQ ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN__SNDCSTREQ, "_clsReplDstSession::_sendConsultReq" )
   void _clsReplDstSession::_sendConsultReq()
   {
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN__SNDCSTREQ );

      if ( CLS_SESSION_STATUS_CONSULT != _status )
      {
         ++_requestID ;
         _status = CLS_SESSION_STATUS_CONSULT ;
      }

      if ( !_pReplBucket->isEmpty() )
      {
         PD_LOG( PDDEBUG, "Session[%s]: Repl bucket is not empty, "
                 "size: %d, can't send consult req", sessionName(),
                 _pReplBucket->size() ) ;
         goto done ;
      }
      _pReplBucket->reset() ;

      if ( _consultLsn.invalid () )
      {
         _consultLsn = _logger->getCurrentLsn() ;
      }

      _syncSrc = _selector.selected() ;
      if ( MSG_INVALID_ROUTEID != _syncSrc.value )
      {
         _timeout = 0 ;

         _MsgReplConsultation msg ;
         msg.header.TID = CLS_TID( _sessionID ) ;
         msg.header.requestID = _requestID ;
         msg.current =  _consultLsn ;
         msg.lastConsult = _lastRecvConsultLsn ;
         msg.identity = routeAgent()->localID() ;

         if ( !_consultLsn.invalid() )
         {
            _mb.clear() ;
            if ( SDB_OK == _logger->search( _consultLsn, &_mb ) )
            {
               msg.hashValue = ossHash( _mb.offset(0), _mb.length() ) ;
            }
         }

         routeAgent()->syncSend( _syncSrc, &msg ) ;
         PD_LOG( PDEVENT, "Session[%s]: Send consult req to [node: %d, "
                 "group:%d], [LSN: %d:%lld]", sessionName(),
                 _syncSrc.columns.nodeID, _syncSrc.columns.groupID,
                 msg.current.version,  msg.current.offset ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSDSTREPSN__SNDCSTREQ );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSTREPSN__REPLG, "_clsReplDstSession::_replayLog" )
   INT32 _clsReplDstSession::_replayLog( const CHAR *logs, const UINT32 &len,
                                         UINT32 &num )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSTREPSN__REPLG ) ;

      dpsLogRecordHeader *recordHeader = NULL ;
      const CHAR *log = logs ;
      num = 0 ;
      BOOLEAN needRollback = FALSE ;
      DPS_LSN expectLSN ;

      while ( log < logs + len )
      {
         if ( eduCB()->isInterrupted() )
         {
            PD_LOG ( PDEVENT, "Session[%s]: ReplayLog is interrupted",
                     sessionName() ) ;
            break ;
         }

         recordHeader = (dpsLogRecordHeader *)log ;
         needRollback = FALSE ;

         PD_LOG( PDDEBUG, "Session[%s]: Replay record [lsn offset: %lld, "
                 "version: %d, len:%d, preLsn:%lld]", sessionName(),
                 recordHeader->_lsn, recordHeader->_version,
                 recordHeader->_length, recordHeader->_preLsn ) ;

         expectLSN = _logger->expectLsn() ;

         if ( expectLSN.compareOffset( recordHeader->_lsn ) > 0 )
         {
            dpsLogRecordHeader *searchHeader = NULL ;
            DPS_LSN searchLSN ;
            searchLSN.offset = recordHeader->_lsn ;
            searchLSN.version = recordHeader->_version ;
            _mb.clear() ;
            rc = _logger->search( searchLSN, &_mb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Find lsn[%u,%lld] in local "
                       "Failed, rc: %d", sessionName(), searchLSN.version,
                       searchLSN.offset, rc ) ;
               goto error ;
            }
            searchHeader = (dpsLogRecordHeader*)_mb.offset(0) ;
            if ( recordHeader->_version != searchHeader->_version ||
                 recordHeader->_length != searchHeader->_length )
            {
               PD_LOG( PDERROR, "Session[%s]: Local lsn[%u,%lld, len: %u] "
                       "is not the same with remote[%s] lsn[%u,%lld, len: %u]",
                       sessionName(), searchHeader->_version,
                       searchHeader->_lsn, searchHeader->_length,
                       routeID2String( _lastSyncNode ).c_str(),
                       recordHeader->_version, recordHeader->_lsn,
                       recordHeader->_length ) ;
               rc = SDB_CLS_REPLAY_LOG_FAILED ;
               goto error ;
            }
            if ( ossHash( log, recordHeader->_length ) !=
                 ossHash( _mb.offset(0), searchHeader->_length ) )
            {
               PD_LOG( PDERROR, "Session[%s]: Local lsn[%u, %lld] 's "
                       "hash value is not the same with remote[%s] "
                       "lsn[%u, %lld]", sessionName(), searchHeader->_version,
                       searchHeader->_lsn,
                       routeID2String( _lastSyncNode ).c_str(),
                       recordHeader->_version, recordHeader->_lsn ) ;
               rc = SDB_CLS_REPLAY_LOG_FAILED ;
               goto error ;
            }

            log += recordHeader->_length ;
            continue ;
         }

#ifdef _DEBUG
         if ( 0 != expectLSN.compareOffset( recordHeader->_lsn ) ||
              0 < expectLSN.compareVersion( recordHeader->_version ) )
         {
            PD_LOG ( PDWARNING, "Session[%s]: ReplayLog, cur lsn[%d,%lld] "
                     "can't fit expect lsn[%d,%lld]", sessionName(),
                     recordHeader->_version, recordHeader->_lsn,
                     expectLSN.version, expectLSN.offset ) ;
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
#endif

         rc = _replay( recordHeader ) ;
         if ( SDB_OK != rc )
         {
            SDB_ASSERT( SDB_OOM == rc || SDB_NOSPC == rc,
                        "Unexpect error occured" ) ;
            PD_LOG( PDERROR, "Session[%s]: Failed to replay log, rc: %d",
                    sessionName(), rc ) ;
            goto error ;
         }

         rc = _logger->recordRow( log, recordHeader->_length ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Row record failed[rc:%d]",
                    sessionName(), rc ) ;
            needRollback = TRUE ;
            goto error ;
         }

         log += recordHeader->_length ;
         ++num ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSDSTREPSN__REPLG, rc );
      return rc ;
   error:
      if ( _pReplBucket->waitEmptyAndRollback() )
      {
         DPS_LSN completeLSN = _pReplBucket->completeLSN() ;
         rcTmp = _logger->move( completeLSN.offset, completeLSN.version ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to move lsn to "
                    "[%u, %llu], rc: %d, need to synchronize full data",
                    sessionName(), completeLSN.version, completeLSN.offset,
                    rcTmp ) ;
            _fullSync() ;
         }
         else
         {
            PD_LOG( PDEVENT, "Session[%s]: Move lsn to[%u, %llu]",
                    sessionName(), completeLSN.version, completeLSN.offset ) ;
         }
      }
      else if ( needRollback )
      {
         rcTmp = _rollback( log ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to rollback[%lld, "
                    "type: %d], rc: %d, need to synchronize full data",
                    sessionName(), recordHeader->_lsn, recordHeader->_type,
                    rcTmp ) ;
            _fullSync() ;
         }
      }
      goto done ;
   }

   INT32 _clsReplDstSession::_replay( dpsLogRecordHeader *header )
   {
      if ( _pReplBucket->maxReplSync() > 0 )
      {
         return _replayer.replayByBucket( header, eduCB(), _pReplBucket ) ;
      }
      else
      {
         return _replayer.replay( header, eduCB() ) ;
      }
   }

   /*
      _clsReplSrcSession implement
   */
   BEGIN_OBJ_MSG_MAP( _clsReplSrcSession , _pmdAsyncSession )
      ON_MSG( MSG_CLS_SYNC_REQ, handleSyncReq )
      ON_MSG( MSG_CLS_SYNC_VIR_REQ, handleVirSyncReq )
      ON_MSG( MSG_CLS_CONSULTATION_REQ, handleConsultReq )
   END_OBJ_MSG_MAP()

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCREPSN__CLSREPSN, "_clsReplSrcSession::_clsReplSrcSession" )
   _clsReplSrcSession::_clsReplSrcSession ( UINT64 sessionID )
      :_pmdAsyncSession ( sessionID ),
       _mb( CLS_SYNC_DEF_LEN ),
       _quit( FALSE ),
       _timeout( 0 )
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCREPSN__CLSREPSN );

      _logger = pmdGetKRCB()->getDPSCB() ;
      _sync = sdbGetReplCB()->syncMgr() ;
      _repl = sdbGetReplCB() ;

      _lastProcRequestID = 0 ;

      PD_TRACE_EXIT ( SDB__CLSSRCREPSN__CLSREPSN );
   }

   _clsReplSrcSession::~_clsReplSrcSession ()
   {
   }

   SDB_SESSION_TYPE _clsReplSrcSession::sessionType() const
   {
      return SDB_SESSION_REPL_SRC ;
   }

   EDU_TYPES _clsReplSrcSession::eduType () const
   {
      return EDU_TYPE_REPLAGENT ;
   }

   void _clsReplSrcSession::onRecieve ( const NET_HANDLE netHandle,
                                        MsgHeader * msg )
   {
      _timeout = 0 ;
      _repl->aliveNode( msg->routeID ) ;
   }

   BOOLEAN _clsReplSrcSession::timeout( UINT32 interval )
   {
      return _quit ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCREPSN_ONTIMER, "_clsReplSrcSession::onTimer" )
   void _clsReplSrcSession::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCREPSN_ONTIMER ) ;
      _timeout += interval ;

      if ( !_quit && CLS_DST_SESSION_NO_MSG_TIME < _timeout )
      {
         PD_LOG ( PDEVENT, "Session[%s] peer node a long time no msg, "
                  "quit", sessionName() ) ;
         _quit = TRUE ;
      }

      PD_TRACE1 ( SDB__CLSSRCREPSN_ONTIMER, PD_PACK_INT(_quit) ) ;
      PD_TRACE_EXIT ( SDB__CLSSRCREPSN_ONTIMER );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCREPSN_HNDSYNCREQ, "_clsReplSrcSession::handleSyncReq" )
   INT32 _clsReplSrcSession::handleSyncReq( NET_HANDLE handle, MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSRCREPSN_HNDSYNCREQ );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      MsgReplSyncReq *msg = ( MsgReplSyncReq * )header ;

      if ( DPS_INVALID_LSN_OFFSET != msg->completeNext.offset )
      {
         _sync->complete( msg->identity, msg->completeNext,
                          CLS_TID( _sessionID ) ) ;
      }
      else
      {
         _sync->complete( msg->identity, msg->next,
                          CLS_TID( _sessionID ) ) ;
      }

      if ( pmdGetStartup().isOK() )
      {
         rc = _syncLog( handle, msg ) ;
      }

      PD_TRACE_EXITRC ( SDB__CLSSRCREPSN_HNDSYNCREQ, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCREPSN_HNDVIRSYNCREQ, "_clsReplSrcSession::handleVirSyncReq" )
   INT32 _clsReplSrcSession::handleVirSyncReq( NET_HANDLE handle,
                                               MsgHeader* header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSRCREPSN_HNDVIRSYNCREQ );
      MsgReplVirSyncReq *msg = ( MsgReplVirSyncReq * )header ;
      _sync->complete( msg->from, msg->next, CLS_TID( _sessionID ) ) ;
      PD_TRACE_EXITRC ( SDB__CLSSRCREPSN_HNDVIRSYNCREQ, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCREPSN_HNDCSTREQ, "_clsReplSrcSession::handleConsultReq" )
   INT32 _clsReplSrcSession::handleConsultReq( NET_HANDLE handle,
                                               MsgHeader* header )
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCREPSN_HNDCSTREQ );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      _MsgReplConsultation *msg = ( _MsgReplConsultation * )header ;
      _MsgReplConsultationRes res ;
      BOOLEAN needReply = TRUE ;
      DPS_LSN fLsn ;
      DPS_LSN mLsn ;
      DPS_LSN eLsn ;

      res.header.header.TID = msg->header.TID ;
      res.header.header.routeID = msg->header.routeID ;
      res.header.header.requestID = msg->header.requestID ;
      time_t bTime = time(NULL) ;

      if ( (UINT32)header->messageLength < sizeof( _MsgReplConsultation ) )
      {
         PD_LOG( PDWARNING, "Session[%s]: Recv consult request message "
                 "length[%d] is less than %d", sessionName(), header->messageLength,
                 sizeof( _MsgReplConsultation ) ) ;
         needReply = FALSE ; /// not reply
         goto done ;
      }
      else if ( header->requestID <= _lastProcRequestID &&
                eduCB()->getQueSize() > 0 )
      {
         PD_LOG( PDINFO, "Session[%s]: Consult's request id[%llu] is no "
                 "greater than processed request id[%llu], and message queue "
                 "is not empty[%u], so dispath the request", sessionName(),
                 header->requestID, _lastProcRequestID,
                 eduCB()->getQueSize() ) ;
         needReply = FALSE ; /// not reply
         goto done ;
      }
      _lastProcRequestID = header->requestID ;

      if ( msg->current.invalid() )
      {
         res.header.res = SDB_CLS_CONSULT_FAILED ;
         goto done ;
      }

      _logger->getLsnWindow( fLsn, mLsn, eLsn, NULL, NULL ) ;
      PD_LOG( PDEVENT, "Session[%s]: Recv a consult req. "
              "[remote offset:%lld, remote ver:%d, local foffset:%lld, "
              "local fver:%d, local eoffset:%lld, local ever:%d]",
              sessionName(), msg->current.offset, msg->current.version,
              fLsn.offset, fLsn.version, eLsn.offset, eLsn.version ) ;

      if ( eLsn.invalid() )
      {
         res.header.res = SDB_CLS_CONSULT_FAILED ;
         goto done ;
      }
      else if ( 0 < fLsn.compare( msg->current ) )
      {
         res.header.res = SDB_CLS_CONSULT_FAILED ;
         goto done ;
      }
      else if ( eLsn.compareVersion ( msg->current.version ) <= 0 &&
                eLsn.compareOffset ( msg->current.offset ) < 0 )
      {
         res.header.res = SDB_OK ;
         res.returnTo = eLsn ;
         goto done ;
      }
      else
      {
         _mb.clear() ;
         DPS_LSN search = msg->current ;
         dpsLogRecordHeader *logHeader = NULL ;
         UINT32 baseVersion = DPS_INVALID_LSN_VERSION ;

         if ( SDB_OK != _logger->search( search, &_mb ) )
         {
            if ( msg->lastConsult.invalid() )
            {
               search.offset = eLsn.offset ;
               baseVersion = eLsn.version ;
            }
            else
            {
               search = msg->lastConsult ;
               baseVersion = search.version ;
            }
         }
         else
         {
            logHeader = (dpsLogRecordHeader*)_mb.offset(0) ;
            if ( logHeader->_version != msg->current.version ||
                 msg->hashValue != ossHash( _mb.offset(0), _mb.length() ) )
            {
               search.offset = logHeader->_preLsn ;
               baseVersion = logHeader->_version ;
            }
            else
            {
               res.header.res = SDB_OK ;
               res.returnTo = search ;
               res.hashValue = msg->hashValue ;
               goto done ;
            }
         }

         DPS_LSN returnTo ;
         do
         {
            _mb.clear() ;
            if ( SDB_OK != _logger->searchHeader( search, &_mb ) )
            {
               break ;
            }
            else
            {
               logHeader = (dpsLogRecordHeader*)_mb.offset(0) ;
               returnTo.offset = logHeader->_lsn ;
               returnTo.version = logHeader->_version ;
               search.offset = logHeader->_preLsn;
            }
         }while ( returnTo.compareOffset( msg->current.offset ) > 0 ||
                  ( returnTo.version == baseVersion &&
                    time( NULL ) - bTime <= CLS_REPL_MAX_TIME ) ) ;

         if ( returnTo.invalid() )
         {
            res.returnTo = fLsn ;
         }
         else
         {
            res.returnTo = returnTo ;
         }
         res.header.res = SDB_OK ;
      }

   done:
      if ( needReply )
      {
         PD_LOG( PDEVENT, "Session[%s]: Consult result[%d], "
                 "return lsn[%u,%lld]", sessionName(), res.header.res,
                 res.returnTo.version, res.returnTo.offset ) ;
         _mb.clear() ;
         if ( 0 == res.hashValue && !res.returnTo.invalid() &&
              SDB_OK == _logger->search( res.returnTo, &_mb ) )
         {
            res.hashValue = ossHash( _mb.offset(0), _mb.length() ) ;
         }
         routeAgent()->syncSend( handle, &res ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSSRCREPSN_HNDCSTREQ );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCREPSN__SYNCLOG, "_clsReplSrcSession::_syncLog" )
   INT32 _clsReplSrcSession::_syncLog( const NET_HANDLE &handle,
                                       const MsgReplSyncReq *req )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSRCREPSN__SYNCLOG );

      DPS_LSN fLsn ;
      DPS_LSN mLsn ;
      DPS_LSN eLsn ;
      DPS_LSN expect ;
      DPS_LSN search = req->next ;
      MsgReplSyncRes msg ;

      if ( DPS_INVALID_LSN_OFFSET == req->next.offset )
      {
         rc = SDB_CLS_SYNC_FAILED ;
         goto done ;
      }

      if ( MSG_INVALID_ROUTEID == _repl->getPrimary().value )
      {
         PD_LOG ( PDINFO, "Session[%s]: Don't know who is primary node, "
                  "not reply", sessionName() ) ;
         goto done ;
      }

      if ( req->header.requestID <= _lastProcRequestID &&
           eduCB()->getQueSize() > 0 )
      {
         PD_LOG( PDINFO, "Session[%s]: Sync's request id[%llu] is no "
                 "greater than processed request id[%llu], and message queue "
                 "is not empty[%u], so dispath the request", sessionName(),
                 req->header.requestID, _lastProcRequestID,
                 eduCB()->getQueSize() ) ;
         goto done ;
      }
      _lastProcRequestID = req->header.requestID ;

      msg.oldestTransLsn = pmdGetKRCB()->getTransCB()->getOldestBeginLsn();
      msg.header.header.routeID = req->header.routeID ;
      msg.header.header.TID = req->header.TID ;
      msg.header.header.requestID = req->header.requestID ;
      msg.identity = routeAgent()->localID() ;

      _logger->getLsnWindow( fLsn, mLsn, eLsn, &expect, NULL ) ;
      if ( 0 < fLsn.compareOffset( req->next.offset ) )
      {
         PD_LOG( PDWARNING, "Session[%s]: Remote lsn is too old. "
                 "remote [offset:%lld, version:%d], local [fLsn offset:%lld, "
                 "fLsn version:%d, eLsn offset:%lld, eLsn version:%d]",
                 sessionName(), req->next.offset, req->next.version,
                 fLsn.offset, fLsn.version, eLsn.offset, eLsn.version ) ;
         msg.header.res = SDB_CLS_SYNC_FAILED ;
         routeAgent()->syncSend( handle, &msg ) ;
         rc = SDB_CLS_SYNC_FAILED ;
         goto done ;
      }
      else if ( 0 > eLsn.compare( req->next ) )
      {
         rc = SDB_OK ;
         if ( pmdIsPrimary() )
         {
            if (  0 != expect.compareOffset( req->next.offset ) ||
                  0 > expect.compareVersion( req->next.version ) )
            {
               msg.header.res = SDB_CLS_SYNC_FAILED ;
               rc = SDB_CLS_SYNC_FAILED ;
               PD_LOG( PDWARNING, "Session[%s]: Remote lsn is not match "
                       "local.[remote offset:%lld, remote ver:%d]"
                       "[end offset:%lld, end version:%d][expect offset:%lld,"
                       "expect version:%d]", sessionName(),req->next.offset,
                       req->next.version, eLsn.offset,
                       eLsn.version, expect.offset, expect.version ) ;
            }
            else
            {
               PD_LOG( PDDEBUG, "Session[%s]: Local has no more new data",
                       sessionName() ) ;
               msg.header.res = SDB_OK ;
               rc = SDB_OK ;
            }
         }
         else
         {
            PD_LOG( PDDEBUG, "Session[%s]: Local has no more new data.",
                    sessionName() ) ;
            msg.header.res = SDB_OK ;
            rc = SDB_OK ;
         }
         routeAgent()->syncSend( handle, &msg ) ;
         goto done ;
      }
      else if ( 0 == req->needData )
      {
         msg.header.res = SDB_OK ;
         routeAgent()->syncSend( handle, &msg ) ;
         goto done ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Session[%s]: Begin to find log. remote "
                 "[offset:%lld, version:%d], local [fLsn offset:%lld, "
                 "fLsn version:%d] [eLsn offset:%lld, eLsn version:%d]",
                 sessionName(), req->next.offset, req->next.version,
                 fLsn.offset, fLsn.version, eLsn.offset, eLsn.version ) ;
         _mb.clear() ;

         _logger->search( search, &_mb, DPS_SEARCH_ALL, -1,
                          CLS_REPL_MAX_TIME, CLS_SYNC_MAX_LEN ) ;
      }

      msg.header.header.messageLength = sizeof( _MsgReplSyncRes) +
                                           _mb.length() ;
      if ( 0 != _mb.length() )
      {
         rc = SDB_OK ;
         msg.header.res = SDB_OK ;
         routeAgent()->syncSend( handle, ( MsgHeader *)(&msg),
                                 _mb.offset( 0 ), _mb.length() ) ;
      }
      else
      {
         rc = SDB_CLS_SYNC_FAILED ;
         msg.header.res = SDB_CLS_SYNC_FAILED ;
         PD_LOG( PDWARNING, "Session[%s]: Can not find [ver:%d, "
                 "offset:%lld]", sessionName(), search.version,
                 search.offset ) ;
         routeAgent()->syncSend( handle, &msg ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSRCREPSN__SYNCLOG, rc );
      return rc ;
   }

}
