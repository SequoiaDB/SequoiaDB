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

   Source File Name = clsShardSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsShardSession.hpp"
#include "pmd.hpp"
#include "clsMgr.hpp"
#include "msgMessage.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtnDataSet.hpp"
#include "rtnContextShdOfLob.hpp"
#include "rtnContextExplain.hpp"
#include "rtnContextMainCL.hpp"
#include "rtnContextDel.hpp"
#include "rtnCommandSnapshot.hpp"
#include "utilCompressor.hpp"
#include "pmdStartup.hpp"
#include "clsCommand.hpp"
#include "dpsOp2Record.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsUtil.hpp"
#include "rtnLob.hpp"
#include "clsMainCLMonAggregator.hpp"

using namespace bson ;

namespace engine
{

#define SHD_SESSION_TIMEOUT         (60)
#define SHD_INTERRUPT_CHECKPOINT    (10)
#define SHD_NOTPRIMARY_WAITTIME     (20000)     //ms
#define SHD_TRANSROLLBACK_WAITTIME  (60000)     //ms
#define SHD_WAITTIME_INTERVAL       (200)       //ms

#define SHD_TRANS_WAITCOMMIT_TIMEOUT      ( 60000 )   // 60 sec
#define SHD_TRANS_WAITCOMMIT_INTERVAL     ( 3000 )    // 3 sec

#define SHD_RET_BUILDER_DFT_SIZE          ( 80 )

   BEGIN_OBJ_MSG_MAP( _clsShdSession, _pmdAsyncSession )
      ON_MSG ( MSG_BS_UPDATE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_INSERT_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_DELETE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_QUERY_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_GETMORE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_KILL_CONTEXT_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_MSG_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_INTERRUPTE, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_BEGIN_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_COMMIT_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_ROLLBACK_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_COMMITPRE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_UPDATE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_DELETE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_TRANS_INSERT_REQ, _onOPMsg )
      ON_MSG ( MSG_COM_SESSION_INIT_REQ, _onOPMsg )
#if defined (_DEBUG)
      ON_MSG ( MSG_AUTH_VERIFY_REQ, _onOPMsg )
      ON_MSG ( MSG_AUTH_CRTUSR_REQ, _onOPMsg )
      ON_MSG ( MSG_AUTH_DELUSR_REQ, _onOPMsg )
#endif
      ON_MSG ( MSG_BS_LOB_OPEN_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_LOB_WRITE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_LOB_READ_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_LOB_LOCK_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_LOB_CLOSE_REQ, _onOPMsg )
      ON_MSG ( MSG_BS_LOB_REMOVE_REQ, _onOPMsg )
      ON_MSG ( MSG_CAT_GRP_CHANGE_NTY, _onCatalogChangeNtyMsg )

      ON_EVENT( PMD_EDU_EVENT_TRANS_STOP, _onTransStopEvnt )
   END_OBJ_MSG_MAP() ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSDSESS__CLSSHDSESS, "_clsShdSession::_clsShdSession" )
   _clsShdSession::_clsShdSession ( UINT64 sessionID, _schedTaskInfo *pTaskInfo )
      :_pmdAsyncSession ( sessionID ),
       _retBuilder( SHD_RET_BUILDER_DFT_SIZE )
   {
      PD_TRACE_ENTRY ( SDB__CLSSDSESS__CLSSHDSESS ) ;
      _pCollectionName  = NULL ;
      _clVersion        = 0 ;
      _isMainCL         = FALSE ;
      _hasUpdateCataInfo= FALSE ;
      pmdKRCB *pKRCB = pmdGetKRCB () ;
      _pReplSet  = sdbGetReplCB () ;
      _pShdMgr   = sdbGetShardCB () ;
      _pCatAgent = pKRCB->getClsCB ()->getCatAgent () ;
      _pFreezingWindow = _pShdMgr->getFreezingWindow() ;
      _pDmsCB    = pKRCB->getDMSCB () ;
      _pDpsCB    = pKRCB->getDPSCB () ;
      _pRtnCB    = pKRCB->getRTNCB () ;
      _pTaskInfo = pTaskInfo ;
      _primaryID.value = MSG_INVALID_ROUTEID ;
      ossMemset( _detailName, 0, sizeof( _detailName ) ) ;
      _logout    = TRUE ;
      _delayLogin= FALSE ;

      _inPacketLevel = 0 ;
      _pendingContextID = -1 ;
      _pendingStartFrom = 0 ;

      _transWaitTimeout = 0 ;
      _transWaitID = DPS_INVALID_TRANS_ID ;

      PD_TRACE_EXIT ( SDB__CLSSDSESS__CLSSHDSESS ) ;
   }

   _clsShdSession::~_clsShdSession ()
   {
      _pReplSet  = NULL ;
      _pShdMgr   = NULL ;
      _pCatAgent = NULL ;
      _pFreezingWindow = NULL ;
      _pDmsCB    = NULL ;
      _pRtnCB    = NULL ;
      _pDpsCB    = NULL ;
      _pCollectionName = NULL ;
      _clVersion = 0 ;
      _cmdCollectionName.clear() ;
   }

   const CHAR* _clsShdSession::sessionName() const
   {
      if ( 0 != _detailName[0] )
      {
         return _detailName ;
      }
      return _pmdAsyncSession::sessionName() ;
   }

   void _clsShdSession::clear()
   {
      ossMemset( _detailName, 0, sizeof( _detailName ) ) ;
      _username = "" ;
      _passwd = "" ;
      _logout = TRUE ;
      _delayLogin = FALSE ;

      _pmdAsyncSession::clear() ;
   }

   SDB_SESSION_TYPE _clsShdSession::sessionType() const
   {
      return SDB_SESSION_SHARD ;
   }

   EDU_TYPES _clsShdSession::eduType () const
   {
      return EDU_TYPE_SHARDAGENT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS_ONRV, "_clsShdSession::onRecieve" )
   void _clsShdSession::onRecieve ( const NET_HANDLE netHandle,
                                    MsgHeader * msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS_ONRV ) ;
      ossGetCurrentTime( _lastRecvTime ) ;
      PD_TRACE_EXIT ( SDB__CLSSHDSESS_ONRV ) ;
   }

   void _clsShdSession::onDispatchMsgBegin( const NET_HANDLE netHandle,
                                            const MsgHeader *pHeader )
   {
      _pTaskInfo->beginATask() ;
   }

   void _clsShdSession::onDispatchMsgEnd( INT64 costUsecs )
   {
      monSvcTaskInfo *pInfo = NULL ;
      _pTaskInfo->doneATask() ;

      if ( costUsecs > 0 )
      {
         pInfo = eduCB()->getMonAppCB()->getSvcTaskInfo() ;
         if ( pInfo )
         {
            /// it doesn't matter wether type is MON_TOTAL_READ_TIME or
            /// MON_TOTAL_WRITE_TIME
            pInfo->monOperationTimeInc( MON_TOTAL_WRITE_TIME,
                                        costUsecs ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS_TMOUT, "_clsShdSession::timeout" )
   BOOLEAN _clsShdSession::timeout ( UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS_TMOUT ) ;
      BOOLEAN ret = FALSE ;
      ossTimestamp curTime ;
      ossGetCurrentTime ( curTime ) ;

      if ( curTime.time - _lastRecvTime.time > SHD_SESSION_TIMEOUT &&
           _pEDUCB->contextNum() == 0 &&
           ( _pEDUCB->getTransID() == DPS_INVALID_TRANS_ID ||
           !(sdbGetReplCB()->primaryIsMe())))
      {
         // will be release
         ret = TRUE ;
         goto done ;
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSSHDSESS_TMOUT ) ;
      return ret ;
   }

   void _clsShdSession::onTimer( UINT64 timerID, UINT32 interval )
   {
      if ( DPS_TRANS_WAIT_COMMIT == _pEDUCB->getTransStatus() )
      {
         if ( _transWaitID != _pEDUCB->getTransID() )
         {
            _transWaitTimeout = interval ;
            _transWaitID = _pEDUCB->getTransID() ;
         }
         else
         {
            _transWaitTimeout += interval ;
         }

         if ( _transWaitTimeout >= SHD_TRANS_WAITCOMMIT_TIMEOUT )
         {
            PD_LOG( PDWARNING, "Transaction(%s) is timeout in "
                    "status(WaitCommit), begin to consult with other nodes",
                    dpsTransIDToString( _pEDUCB->getTransID() ).c_str() ) ;

            // only test wait-sync for pre-commit log
            _rollbackTrans( NULL, OSS_ONE_SEC, 0 ) ;

            if ( _transWaitTimeout >= SHD_TRANS_WAITCOMMIT_INTERVAL )
            {
               _transWaitTimeout -= SHD_TRANS_WAITCOMMIT_INTERVAL ;
            }
         }
      }
      else
      {
         _transWaitTimeout = 0 ;
         _transWaitID = DPS_INVALID_TRANS_ID ;
      }
   }

   BOOLEAN _clsShdSession::isSetLogout() const
   {
      return _logout ;
   }

   BOOLEAN _clsShdSession::isDelayLogin() const
   {
      return _delayLogin ;
   }

   void _clsShdSession::setLogout()
   {
      _logout = TRUE ;
   }

   const dpsTransConfItem& _clsShdSession::getTransConf() const
   {
      return _transConf ;
   }

   const string& _clsShdSession::getSource() const
   {
      return _source ;
   }

   void _clsShdSession::setDelayLogin( const clsIdentifyInfo &info )
   {
      _delayLogin = TRUE ;

      UINT32 ip = 0, port = 0 ;
      ossUnpack32From64( info._id, ip, port ) ;
      setIdentifyInfo( ip, port, info._nid, info._tid, info._eduid ) ;
      _username = info._username ;
      _passwd = info._passwd ;
      _source = info._source ;
      /// update audit mask
      setAuditConfig( info._auditMask, info._auditConfigMask ) ;
      /// update trans conf
      _transConf = info._transConf ;

      if ( !info._objSchedInfo.isEmpty() )
      {
         _objDelayInfo = info._objSchedInfo.getOwned() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONDETACH, "_clsShdSession::_onDetach" )
   void _clsShdSession::_onDetach ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONDETACH ) ;
      if ( _pEDUCB )
      {
         INT64 contextID = -1 ;
         while ( -1 != ( contextID = _pEDUCB->contextPeek() ) )
         {
            _pRtnCB->contextDelete ( contextID, NULL ) ;
         }

         // wait pre-commit for 5 minutes, after that ignore sync error
         INT32 rcTmp = _rollbackTrans( NULL, -1, 5 * 60 * OSS_ONE_SEC, TRUE ) ;
         if ( rcTmp)
         {
            PD_LOG ( PDERROR, "Failed to rollback(rc=%d)", rcTmp ) ;
         }

         rtnRemoteMessenger *messenger =
            pmdGetKRCB()->getRTNCB()->getRemoteMessenger() ;
         if ( messenger )
         {
            messenger->removeSession( _pEDUCB ) ;
         }
      }

      /// has session init
      if ( 0 != _detailName[0] )
      {
         UINT32 ip = 0 ;
         UINT32 port = 0 ;
         ossUnpack32From64( identifyID(), ip, port ) ;

         /// audit
         CHAR szTmpIP[ 50 ] = { 0 } ;
         ossIP2Str( ip, szTmpIP, sizeof(szTmpIP) - 1 ) ;
         CHAR szTmpID[ 20 ] = { 0 } ;
         ossSnprintf( szTmpID, sizeof(szTmpID) - 1, "%llu", eduID() ) ;

         PD_AUDIT( AUDIT_ACCESS, _client.getUsername(), szTmpIP, (UINT16)port,
                   "LOGOUT", AUDIT_OBJ_SESSION, szTmpID, SDB_OK,
                   "User[UserName:%s, FromIP:%s, FromPort:%u, "
                   "FromSession:%llu, FromTID:%u, Source:%s] logout succeed",
                   _client.getUsername(), szTmpIP, port,
                   identifyEDUID(), identifyTID(), _source.c_str() ) ;
      }

      _pmdAsyncSession::_onDetach () ;
      PD_TRACE_EXIT ( SDB__CLSSHDSESS__ONDETACH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__WAITSYNC, "_clsShdSession::_waitSync" )
   INT32 _clsShdSession::_waitSync( const DPS_LSN_OFFSET &offset,
                                    INT32 w,
                                    INT32 waitSyncTimeout )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSSHDSESS__WAITSYNC ) ;

      replCB * replCB = sdbGetClsCB()->getReplCB() ;
      INT32 timePassed = 0, waitTime = OSS_ONE_SEC ;

      if ( 0 == waitSyncTimeout )
      {
         waitSyncTimeout = 10 ;
         waitTime = 10 ;
      }
      else if ( waitSyncTimeout < 0 )
      {
         waitSyncTimeout = OSS_SINT32_MAX ;
      }

      // start rollback to avoid interrupt during wait-sync
      _pEDUCB->startTransRollback() ;

      while ( replCB->groupSize() > 1 &&
              pmdIsPrimary() &&
              timePassed < waitSyncTimeout )
      {
         // just wait for one replica node in this special case
         rc = replCB->sync( offset, _pEDUCB, w, waitTime ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }
         else if ( SDB_TIMEOUT == rc )
         {
            // could wait again
            timePassed += waitTime ;
            continue ;
         }
         else if ( SDB_CLS_WAIT_SYNC_FAILED == rc ||
                   SDB_DATABASE_DOWN == rc )
         {
            // wait sync may not pass 1 second, so sleep and retry
            ossSleep( OSS_ONE_SEC ) ;
            timePassed += OSS_ONE_SEC ;
         }
         else
         {
            PD_LOG( PDWARNING, "Failed to wait sync for lsn [%llu], rc: %d",
                    offset, rc ) ;
            break ;
         }
      }

      _pEDUCB->stopTransRollback() ;

      PD_TRACE_EXITRC( SDB__CLSSHDSESS__WAITSYNC, rc ) ;

      return rc ;
   }

   INT32 _clsShdSession::_rollbackTrans( BOOLEAN *pHasRollback,
                                         INT32 checkStatusTimeout,
                                         INT32 waitSyncTimeout,
                                         BOOLEAN ignoreWaitSyncError )
   {
      INT32 rc = SDB_OK ;

      if ( DPS_INVALID_LSN_OFFSET == _pEDUCB->getCurTransLsn() )
      {
         // readonly transaction goes to rollback directly
         goto rollback ;
      }

      if ( DPS_TRANS_WAIT_COMMIT == _pEDUCB->getTransStatus() )
      {
         DPS_LSN lsn ;
         _dpsMessageBlock mb ;
         lsn.offset = _pEDUCB->getCurTransLsn() ;
         clsGTSAgent *pGTSAgent = _pShdMgr->getGTSAgent() ;
         DPS_TRANS_STATUS status = DPS_TRANS_UNKNOWN ;

         DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
         DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
         DPS_LSN_OFFSET firstTransLsn = DPS_INVALID_LSN_OFFSET ;
         UINT8 attr = 0 ;
         UINT32 nodeNum = 0 ;
         const UINT64 *pNodes = NULL ;
         INT32 timeCounter = 0 ;

         // for wait-commit status, we need to make sure pre-commit log
         // is replicated to at least one other replicate node ( group with
         // multiple nodes )
         rc = _waitSync( lsn.offset, 2, waitSyncTimeout ) ;
         if ( SDB_OK != rc )
         {
            if ( ignoreWaitSyncError )
            {
               PD_LOG( PDWARNING, "Failed to wait pre-commit log for "
                       "transaction [%s], lsn [%llu], rc: %d, ignore now",
                       dpsTransIDToString( transID ).c_str(),
                       lsn.offset, rc ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to wait pre-commit log for "
                       "transaction [%s], lsn [%llu], rc: %d",
                       dpsTransIDToString( transID ).c_str(),
                       lsn.offset, rc ) ;
               goto error ;
            }
         }

         /// load lsn
         rc = _pDpsCB->search( lsn, &mb ) ;
         SDB_ASSERT( SDB_OK == rc, "Search lsn is error" ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Search lsn(%llu) failed, rc: %d",
                    lsn.offset, rc ) ;
            goto rollback ;
         }

         rc = dpsRecord2TransCommit( mb.offset( 0 ), transID, preTransLsn,
                                     firstTransLsn, attr, nodeNum,
                                     &pNodes ) ;
         SDB_ASSERT( SDB_OK == rc &&
                     DPS_TS_COMMIT_ATTR_PRE == attr &&
                     nodeNum > 0,
                     "Invalid log" ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Log is invalid" ) ;
            goto rollback ;
         }

         do
         {
            rc = pGTSAgent->checkTransStatus( _pEDUCB->getTransID(),
                                              nodeNum, pNodes,
                                              _pEDUCB, status ) ;
            if ( rc )
            {
               ossSleep( OSS_ONE_SEC ) ;
               timeCounter += OSS_ONE_SEC ;

               if ( checkStatusTimeout >= 0 &&
                    timeCounter > checkStatusTimeout )
               {
                  goto error ;
               }
               continue ;
            }

            if ( DPS_TRANS_COMMIT == status )
            {
               goto commit ;
            }
            else
            {
               goto rollback ;
            }
         } while( pmdIsPrimary() ) ;
         {
            BOOLEAN savedAsWaitCommit = FALSE ;
            rc = rtnTransSaveWaitCommit( _pEDUCB, _pDpsCB, savedAsWaitCommit ) ;
            if ( rc )
            {
               goto error ;
            }
            else if ( savedAsWaitCommit )
            {
               goto done ;
            }
         }
      }
      else if ( DPS_TRANS_DOING == _pEDUCB->getTransStatus() )
      {
         _pEDUCB->setTransStatus( DPS_TRANS_DOING_INTERRUPT ) ;
         sdbGetTransCB()->updateTransStatus( _pEDUCB->getTransID(),
                                             DPS_TRANS_DOING_INTERRUPT ) ;
      }

   rollback:
      if ( pHasRollback )
      {
         *pHasRollback = TRUE ;
      }
      rc = rtnTransRollback( _pEDUCB, _pDpsCB ) ;
      if ( rc )
      {
         goto error ;
      }
      goto done ;

   commit:
      if ( pHasRollback )
      {
         *pHasRollback = FALSE ;
      }
      rc = rtnTransCommit( _pEDUCB, _pDpsCB ) ;
      if ( rc )
      {
         goto error ;
      }
      goto done ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__DFMSGFUNC, "_clsShdSession::_defaultMsgFunc" )
   INT32 _clsShdSession::_defaultMsgFunc ( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__DFMSGFUNC ) ;
      rc = _onOPMsg( handle, msg ) ;
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__DFMSGFUNC, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__REPLY, "_clsShdSession::_reply" )
   INT32 _clsShdSession::_reply ( MsgOpReply * header, const CHAR * buff,
                                  UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__REPLY ) ;

      if ( (UINT32)(header->header.messageLength) !=
           sizeof (MsgOpReply) + size )
      {
         PD_LOG ( PDERROR, "Session[%s] reply message length error[%u != %u]",
                  sessionName() ,header->header.messageLength,
                  sizeof ( MsgOpReply ) + size ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      //Send message
      if ( size > 0 )
      {
         rc = routeAgent()->syncSend ( _netHandle, (MsgHeader *)header,
                                       (void*)buff, size ) ;
      }
      else
      {
         rc = routeAgent()->syncSend ( _netHandle, (void *)header ) ;
      }

      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Session[%s] send reply message failed[rc:%d]",
            sessionName(), rc ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__REPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //message fuctions
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONOPMSG, "_clsShdSession::_onOPMsg" )
   INT32 _clsShdSession::_onOPMsg ( NET_HANDLE handle, MsgHeader * msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONOPMSG ) ;

      BOOLEAN loop = TRUE ;
      INT32 loopTime = 0 ;
      INT32 rc = SDB_OK ;
      INT32 opCode = msg->opCode;

      SINT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      rtnContextBuf buffObj ;
      _pCollectionName = NULL ;
      _clVersion = 0 ;
      _cmdCollectionName.clear() ;
      _isMainCL        = FALSE ;
      _hasUpdateCataInfo = FALSE ;
      BOOLEAN isNeedRollback = FALSE ;
      BOOLEAN isAutoCommit = FALSE ;
      monClassQuery *monQuery = NULL ;
      ossTick startTime ;
      monClassQueryTmpData tmpData ;
      tmpData = *(eduCB()->getMonAppCB()) ;

      _primaryID.value = MSG_INVALID_ROUTEID ;

      if ( isDelayLogin() )
      {
         _login() ;
      }

      while ( loop )
      {
         _retBuilder.reset() ;

         if ( MSG_PACKET == opCode )
         {
            rc = _onPacketMsg( handle, msg, contextID, buffObj,
                               startFrom, opCode ) ;
            if ( 0 == _inPacketLevel )
            {
               goto reply ;
            }
            else
            {
               goto done ;
            }
         }

         rc = _checkClusterActive( msg ) ;
         if ( rc )
         {
            break ;
         }

         MON_START_OP( _pEDUCB->getMonAppCB() ) ;
         _pEDUCB->getMonAppCB()->setLastOpType( opCode ) ;

         if ( _pEDUCB->getMonQueryCB() == NULL )
         {
            monQuery = pmdGetKRCB()->getMonMgr()->
                       registerMonitorObject<monClassQuery>() ;

            if ( monQuery )
            {
               monQuery->sessionID = _pEDUCB->getID() ;
               monQuery->opCode = opCode ;
               monQuery->tid = _pEDUCB->getTID() ;

               ISession *pSession = _pEDUCB->getSession() ;
               if ( pSession )
               {
                  monQuery->relatedTID = pSession->identifyTID() ;
                  monQuery->relatedNID = pSession->identifyNID() ;
               }

               _pEDUCB->setMonQueryCB( monQuery ) ;
            }
         }
         startTime.sample() ;

         switch ( opCode )
         {
            case MSG_BS_UPDATE_REQ :
            {
               MsgOpUpdate *pUpdateMsg = ( MsgOpUpdate* )msg ;
               utilUpdateResult upResult ;
               isNeedRollback = TRUE ;
               rc = _onUpdateReqMsg ( handle, msg, upResult ) ;
               upResult.toBSON( _retBuilder ) ;
               if ( FLG_UPDATE_RETURNNUM & pUpdateMsg->flags )
               {
                  /// compatiable with old version
                  contextID = upResult.modifiedNum() ;
               }
               break ;
            }
            case MSG_BS_INSERT_REQ :
            {
               MsgOpInsert *pInsert = (MsgOpInsert*)msg ;
               utilInsertResult inResult ;
               isNeedRollback = TRUE ;
               rc = _onInsertReqMsg ( handle, msg, inResult ) ;
               inResult.toBSON( _retBuilder ) ;
               if ( pInsert->flags & FLG_INSERT_RETURNNUM )
               {
                  /// compatiable with old version
                  contextID = ossPack32To64( inResult.insertedNum(),
                                             inResult.duplicatedNum() ) ;
               }
               break ;
            }
            case MSG_BS_DELETE_REQ :
            {
               MsgOpDelete *pDeleteMsg = ( MsgOpDelete* )msg ;
               utilDeleteResult delResult ;
               isNeedRollback = TRUE ;
               rc = _onDeleteReqMsg ( handle, msg, delResult ) ;
               delResult.toBSON( _retBuilder ) ;
               if ( FLG_DELETE_RETURNNUM & pDeleteMsg->flags )
               {
                  /// compatiable with old version
                  contextID = delResult.deletedNum() ;
               }
               break ;
            }
            case MSG_BS_QUERY_REQ :
               rc = _onQueryReqMsg ( handle, msg, buffObj, startFrom,
                                     contextID, isNeedRollback,
                                     &_retBuilder ) ;
               break ;
            case MSG_BS_GETMORE_REQ :
               rc = _onGetMoreReqMsg ( msg, buffObj, startFrom,
                                       contextID, isNeedRollback ) ;
               break ;
            case MSG_BS_TRANS_UPDATE_REQ :
            {
               MsgOpUpdate *pUpdateMsg = ( MsgOpUpdate* )msg ;
               utilUpdateResult upResult ;
               isNeedRollback = TRUE ;
               rc = _onTransUpdateReqMsg ( handle, msg, upResult ) ;
               upResult.toBSON( _retBuilder ) ;
               if ( FLG_UPDATE_RETURNNUM & pUpdateMsg->flags )
               {
                  /// compatiable with old version
                  contextID = upResult.modifiedNum() ;
               }
               break ;
            }
            case MSG_BS_TRANS_INSERT_REQ :
            {
               MsgOpInsert *pInsert = (MsgOpInsert*)msg ;
               utilInsertResult inResult ;
               isNeedRollback = TRUE ;
               rc = _onTransInsertReqMsg ( handle, msg, inResult ) ;
               inResult.toBSON( _retBuilder ) ;
               if ( pInsert->flags & FLG_INSERT_RETURNNUM )
               {
                  contextID = ossPack32To64( inResult.insertedNum(),
                                             inResult.duplicatedNum() ) ;
               }
               break ;
            }
            case MSG_BS_TRANS_DELETE_REQ :
            {
               MsgOpDelete *pDeleteMsg = ( MsgOpDelete* )msg ;
               utilDeleteResult delResult ;
               isNeedRollback = TRUE ;
               rc = _onTransDeleteReqMsg ( handle, msg, delResult ) ;
               delResult.toBSON( _retBuilder ) ;
               if ( FLG_DELETE_RETURNNUM & pDeleteMsg->flags )
               {
                  /// compatiable with old version
                  contextID = delResult.deletedNum() ;
               }
               break ;
            }
            case MSG_BS_TRANS_QUERY_REQ :
               rc = _onTransQueryReqMsg( handle, msg, buffObj, startFrom,
                                         contextID, isNeedRollback ) ;
               break ;
            case MSG_BS_KILL_CONTEXT_REQ :
               rc = _onKillContextsReqMsg ( handle, msg ) ;
               break ;
            case MSG_BS_MSG_REQ :
               rc = _onMsgReq ( handle, msg ) ;
               break ;
            case MSG_BS_INTERRUPTE :
               rc = _onInterruptMsg( handle, msg ) ;
               break ;
#if defined (_DEBUG)
            // for authentication message through sharding port, we simply
            // return OK
            case MSG_AUTH_VERIFY_REQ :
            case MSG_AUTH_CRTUSR_REQ :
            case MSG_AUTH_DELUSR_REQ :
               rc = SDB_OK ;
               break ;
#endif
            case MSG_BS_TRANS_BEGIN_REQ :
               rc = _onTransBeginMsg( handle, msg ) ;
               break;

            case MSG_BS_TRANS_COMMIT_REQ :
               isNeedRollback = TRUE ;
               rc = _onTransCommitMsg( handle, msg );
               break;

            case MSG_BS_TRANS_ROLLBACK_REQ:
               rc = _onTransRollbackMsg( handle, msg ) ;
               break;

            case MSG_BS_TRANS_COMMITPRE_REQ:
               isNeedRollback = TRUE ;
               rc = _onTransCommitPreMsg( handle, msg );
               break;

            case MSG_COM_SESSION_INIT_REQ:
               rc = _onSessionInitReqMsg( msg );
               break;

            case MSG_BS_LOB_OPEN_REQ:
               rc = _onOpenLobReq( msg, contextID, buffObj ) ;
               break ;
            case MSG_BS_LOB_WRITE_REQ:
               rc = _onWriteLobReq( msg ) ;
               break ;
            case MSG_BS_LOB_READ_REQ:
               rc = _onReadLobReq( msg, buffObj ) ;
               break ;
            case MSG_BS_LOB_LOCK_REQ:
               rc = _onLockLobReq( msg ) ;
               break ;
            case MSG_BS_LOB_CLOSE_REQ:
               rc = _onCloseLobReq( msg ) ;
               break ;
            case MSG_BS_LOB_REMOVE_REQ:
               rc = _onRemoveLobReq( msg ) ;
               break ;
            case MSG_BS_LOB_UPDATE_REQ:
               rc = _onUpdateLobReq( msg ) ;
               break ;
            case MSG_BS_LOB_GETRTDETAIL_REQ:
               rc = _onGetLobRTDetailReq( msg, buffObj ) ;
               break ;
            default:
               rc = SDB_CLS_UNKNOW_MSG ;
               break ;
         }

         //Need to update catalog info
         // SDB_CLS_NO_CATALOG_INFO: between update and check, this cata
         // will be removed by others, so need to retry all the way
         if ( SDB_CLS_NO_CATALOG_INFO == rc ||
              ( ( SDB_CLS_DATA_NODE_CAT_VER_OLD == rc ||
                  SDB_CLS_COORD_NODE_CAT_VER_OLD == rc
                 ) && loopTime < 1
               )
             )
         {
            loopTime++ ;
            PD_LOG ( PDWARNING, "Catalog is empty or older[rc:%d] in "
                     "session[%s]", rc, sessionName() ) ;
            rc = _pShdMgr->syncUpdateCatalog( _pCollectionName ) ;
            if ( SDB_OK == rc )
            {
               _hasUpdateCataInfo = TRUE ;
               continue ;
            }
         }
         else if ( (SDB_DMS_CS_NOTEXIST == rc || SDB_DMS_NOTEXIST == rc) &&
                   _pCollectionName )
         {
            if ( _pReplSet->primaryIsMe() )
            {
               // catalog has the collection, so need to create, no compression
               if ( _isMainCL )
               {
                  /// if main collection, need update catalog info first
                  if ( !_hasUpdateCataInfo )
                  {
                     rc = _pShdMgr->syncUpdateCatalog( _pCollectionName ) ;
                     if ( SDB_OK == rc )
                     {
                        ++loopTime ;
                        _hasUpdateCataInfo = TRUE ;
                     }
                  }
               }
               else if ( SDB_DMS_CS_NOTEXIST == rc )
               {
                  rc = _createCSByCatalog( _pCollectionName ) ;
               }
               else if ( SDB_DMS_NOTEXIST == rc )
               {
                  rc = _createCLByCatalog( _pCollectionName ) ;
               }

               if ( SDB_OK == rc )
               {
                  continue ;
               }
            }
            else
            {
               // if slave data node doesn't have the cs/cl, but catalog has the
               // cs/cl, it may be that rename operation hasn't been replayed.
               rc = _pShdMgr->syncUpdateCatalog( _pCollectionName ) ;
               if ( SDB_OK == rc )
               {
                  rc = SDB_CLS_DATA_NOT_SYNC ;
               }
            }
         }
         else if ( SDB_RTN_EXIST_INDOUBT_TRANS == rc )
         {
            _rollbackTrans() ;
            if ( DPS_TRANS_WAIT_COMMIT != eduCB()->getTransStatus() )
            {
               continue ;
            }
         }

         if ( SDB_CLS_DATA_NODE_CAT_VER_OLD == rc )
         {
            rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
         }

         if ( _pEDUCB->getMonQueryCB() )
         {
            monQuery = _pEDUCB->getMonQueryCB() ;
            ossTick endTime ;
            endTime.sample() ;
            monQuery->responseTime += endTime - startTime ;
            monQuery->rowsReturned += buffObj.recordNum() ;
            tmpData.diff(*(_pEDUCB->getMonAppCB())) ;
            monQuery->incMetrics(tmpData) ;
            if ( !monQuery->anchorToContext )
            {
               pmdGetKRCB()->getMonMgr()->removeMonitorObject( monQuery ) ;
               _pEDUCB->setMonQueryCB( NULL ) ;
            }
         }

         loop = FALSE ;
      }

      if ( isNoReplyMsg( opCode ) )
      {
         //not to reply
         goto done ;
      }

      if ( rc < -SDB_MAX_ERROR || rc > SDB_MAX_WARNING )
      {
         PD_LOG ( PDERROR, "Session[%s] OP[type:%u] return code error[rc:%d]",
                  sessionName(), opCode, rc ) ;
         rc = SDB_SYS ;
      }

      if ( rc )
      {
         ftReportErr( rc, _pEDUCB->isWritingDB() ) ;
      }

      /// auto-commit process
      if ( eduCB()->isAutoCommitTrans() )
      {
         isAutoCommit = TRUE ;
         if ( SDB_OK == rc || SDB_DMS_EOC == rc )
         {
            INT32 rcTmp = rtnTransCommit( eduCB(), _pDpsCB ) ;
            rc = rcTmp ? rcTmp : rc ;
         }
      }

      try
      {
         if ( SDB_OK != rc )
         {
            BOOLEAN inTrans = _pEDUCB->isTransaction() ;
            BOOLEAN hasRollbacked = FALSE ;

            /// when coord catalog info is old, can't rollback, coord will retry
            if ( inTrans &&
                 ( isAutoCommit ||
                   ( isNeedRollback && SDB_CLS_COORD_NODE_CAT_VER_OLD != rc &&
                     _pEDUCB->getTransExecutor()->isTransAutoRollback() )
                  )
                )
            {
               PD_LOG ( PDDEBUG, "Rolling back operation(op=%d, rc=%d) on data",
                        opCode, rc ) ;
               INT32 rcTmp = _rollbackTrans() ;
               if ( rcTmp )
               {
                  PD_LOG ( PDERROR, "Failed to rollback(rc=%d)", rcTmp ) ;
               }
               hasRollbacked = TRUE ;
            }

            if ( SDB_APP_INTERRUPT == rc &&
                 SDB_OK != _pEDUCB->getInterruptRC() )
            {
               rc = _pEDUCB->getInterruptRC() ;
               PD_LOG ( PDDEBUG, "Interrupted EDU [%llu] with return code %d",
                        _pEDUCB->getID(), rc ) ;
            }

            if ( 0 == buffObj.size() )
            {
               utilBuildErrorBson( _retBuilder, rc,
                                   _pEDUCB->getInfo( EDU_INFO_ERROR ),
                                   inTrans ? &hasRollbacked : NULL ) ;
               _errorInfo = _retBuilder.done() ;
               buffObj = rtnContextBuf( _errorInfo ) ;
            }
            else
            {
               SDB_ASSERT( 1 == buffObj.recordNum(), "Record number must be 1" ) ;

               // buffObj may be hold by _retBuilder, need a new builder
               BSONObjBuilder errorBuilder ;

               BSONObj errObj( buffObj.data() ) ;
               BSONObjIterator itr( errObj ) ;
               while( itr.more() )
               {
                  BSONElement e = itr.next() ;
                  if ( 0 != ossStrcmp( FIELD_NAME_ROLLBACK, e.fieldName() ) )
                  {
                     errorBuilder.append( e ) ;
                  }
               }
               errorBuilder.appendBool( FIELD_NAME_ROLLBACK, hasRollbacked ) ;
               _errorInfo = errorBuilder.obj() ;
               buffObj = rtnContextBuf( _errorInfo ) ;
            }

            if ( rc != SDB_DMS_EOC )
            {
               PD_LOG ( (SDB_CLS_COORD_NODE_CAT_VER_OLD==rc ? PDINFO : PDERROR),
                        "Session[%s] process OP[type:%u] failed[rc:%d]",
                        sessionName(), opCode, rc ) ;
            }

            if ( SDB_CLS_NOT_PRIMARY == rc ||
                 SDB_CLS_NOT_SECONDARY == rc )
            {
               if ( 0 == _primaryID.columns.nodeID )
               {
                  _primaryID.value = _pReplSet->getPrimary().value ;
               }
               if ( 0 != _primaryID.columns.nodeID )
               {
                  // return the node id by startFrom
                  startFrom = _primaryID.columns.nodeID ;
               }
            }
         }
         /// succeed and has result info
         else if ( !_retBuilder.isEmpty() && 0 == buffObj.size() )
         {
            _errorInfo = _retBuilder.done() ;
            buffObj = rtnContextBuf( _errorInfo ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Failed to build retObj:exception=%s",
                 e.what() ) ;
      }

   reply:
      if ( _inPacketLevel > 0 )
      {
         _pendingContextID = contextID ;
         _pendingBuff = buffObj ;
         _pendingStartFrom = startFrom ;
      }
      else
      {
         //Build reply message
         _replyHeader.header.opCode = MAKE_REPLY_TYPE( opCode ) ;
         _replyHeader.header.messageLength = sizeof ( MsgOpReply ) ;
         _replyHeader.header.requestID = msg->requestID ;
         _replyHeader.header.TID = msg->TID ;
         _replyHeader.header.routeID.value = 0 ;

         _replyHeader.header.messageLength += buffObj.size() ;
         _replyHeader.flags = rc ;
         _replyHeader.contextID = contextID ;
         _replyHeader.numReturned = buffObj.recordNum() ;
         _replyHeader.startFrom = startFrom ;

         rc = _reply ( &_replyHeader, buffObj.data(), buffObj.size() ) ;
      }

   done:
      eduCB()->writingDB( FALSE ) ;
      MON_END_OP( _pEDUCB->getMonAppCB() ) ;
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONOPMSG, rc ) ;
      return rc ;
   }

   INT32 _clsShdSession::_createCSByCatalog( const CHAR * clFullName )
   {
      INT32 rc = SDB_OK ;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      INT32 index = 0 ;
      while ( clFullName[ index ] && index < DMS_COLLECTION_SPACE_NAME_SZ )
      {
         if ( '.' == clFullName[ index ] )
         {
            break ;
         }
         csName[ index ] = clFullName[ index ] ;
         ++index ;
      }

      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      UINT32 pageSize = DMS_PAGE_SIZE_DFT ;
      UINT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      DMS_STORAGE_TYPE type = DMS_STORAGE_NORMAL ;
      rc = _pShdMgr->rGetCSInfo( csName, csUniqueID,
                                 &pageSize, &lobPageSize, &type ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Session[%s]: Get collection space[%s] page "
                 "size from catalog failed, rc: %d", sessionName(),
                 csName, rc ) ;
         goto error ;
      }
      rc = rtnCreateCollectionSpaceCommand( csName, _pEDUCB, _pDmsCB, _pDpsCB,
                                            csUniqueID, pageSize,
                                            lobPageSize, type ) ;
      if ( SDB_DMS_CS_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( SDB_DMS_CS_UNIQUEID_CONFLICT == rc )
      {
         rc = _renameCSByCatalog( csName, csUniqueID ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Session[%s]: Rename collection space[%s] by "
                    "catalog failed, rc: %d", sessionName(), csName, rc ) ;
         }
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Session[%s]: Create collection space[%s] by "
                 "catalog failed, rc: %d", sessionName(), csName, rc ) ;
      }
      else
      {
         PD_LOG( PDEVENT, "Session[%s]: Create collection space[%s] by "
                 "catalog succeed", sessionName(), csName ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_createCLByCatalog( const CHAR *clFullName,
                                             const CHAR *pParent,
                                             BOOLEAN mustOnSelf )
   {
      INT32 rc                = SDB_OK ;
      UINT32 attribute        = 0 ;
      BOOLEAN isMainCL        = FALSE;
      UINT32 groupCount       = 0 ;
      BSONObj shardingKey ;
      CLS_SUBCL_LIST subCLList ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      UTIL_COMPRESSOR_TYPE compType = UTIL_COMPRESSOR_INVALID ;
      BSONObj extOptions ;
      BSONObjBuilder builder ;

      /// get sharding key
   retry:
      _pCatAgent->lock_r() ;
      clsCatalogSet *set = _pCatAgent->collectionSet( clFullName ) ;
      if ( NULL == set )
      {
         _pCatAgent->release_r() ;

         rc = _pShdMgr->syncUpdateCatalog( clFullName ) ;
         if ( SDB_OK == rc )
         {
            goto retry ;
         }
         else
         {
            PD_LOG( PDERROR, "Session[%s] Update collection[%s]'s "
                    "catalog info failed, rc: %d", sessionName(),
                    clFullName, rc ) ;
            goto error ;
         }
      }

      if ( set->isSharding() && set->ensureShardingIndex() )
      {
         shardingKey = set->getShardingKey().getOwned() ;
      }

      attribute = set->getAttribute() ;
      isMainCL = set->isMainCL() ;
      groupCount = set->groupCount() ;
      clUniqueID = set->clUniqueID() ;
      compType = set->getCompressType() ;
      if ( OSS_BIT_TEST( attribute, DMS_MB_ATTR_CAPPED ) )
      {
         builder.append( FIELD_NAME_SIZE, set->getMaxSize() ) ;
         builder.append( FIELD_NAME_MAX, set->getMaxRecNum() ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, set->getOverWrite() ) ;
         extOptions = builder.done() ;
      }

      if ( isMainCL )
      {
         set->getSubCLList( subCLList ) ;
      }

      _pCatAgent->release_r() ;

      if ( isMainCL )
      {
         CLS_SUBCL_LIST_IT iter = subCLList.begin() ;
         while ( iter != subCLList.end() )
         {
            rc = _createCLByCatalog( (*iter).c_str(), clFullName, FALSE ) ;
            if ( rc )
            {
               break ;
            }
            ++iter ;
         }
      }
      else
      {
         if( 0 == groupCount )
         {
            /// first clear
            _pCatAgent->lock_w() ;
            _pCatAgent->clear( clFullName ) ;
            _pCatAgent->release_w() ;

            if ( pParent && FALSE == mustOnSelf )
            {
               /// ignore this sub-collection
               goto done ;
            }

            rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
            PD_LOG( PDERROR, "Session[%s]: Collection[%s] is not on this group",
                    sessionName(), clFullName ) ;
            goto error ;
         }

         rc = rtnCreateCollectionCommand( clFullName, shardingKey, attribute,
                                          _pEDUCB, _pDmsCB, _pDpsCB, clUniqueID,
                                          compType, 0, FALSE, &extOptions ) ;
         if ( SDB_DMS_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( SDB_DMS_UNIQUEID_CONFLICT == rc )
         {
            rc = _renameCLByCatalog( clFullName, clUniqueID ) ;
            if ( rc )
            {
               if ( NULL == pParent )
               {
                  PD_LOG( PDWARNING, "Session[%s]: "
                          "Rename collection[%s] by catalog failed, rc: %d",
                          sessionName(), clFullName, rc ) ;
               }
               else
               {
                  PD_LOG( PDWARNING, "Session[%s]: Rename sub-collection[%s] "
                          "of main-collection[%s] by catalog failed, rc: %d",
                          sessionName(), clFullName, pParent, rc ) ;
               }
            }
         }
         else if ( SDB_OK != rc )
         {
            if ( NULL == pParent )
            {
               PD_LOG( PDWARNING, "Session[%s]: Create collection[%s] by "
                       "catalog failed, rc: %d", sessionName(), clFullName,
                       rc ) ;
            }
            else
            {
               PD_LOG( PDWARNING, "Session[%s]: Create sub-collection[%s] "
                       "of main-collection[%s] by catalog failed, rc: %d",
                       sessionName(), clFullName, pParent, rc ) ;
            }
         }
         else
         {
            if ( NULL == pParent )
            {
               PD_LOG( PDEVENT, "Session[%s]: Create collection[%s] by "
                       "catalog succeed", sessionName(), clFullName ) ;
            }
            else
            {
               PD_LOG( PDEVENT, "Session[%s]: Create sub-collection[%s] "
                       "of main-collection[%s] by catalog succeed",
                       sessionName(), clFullName, pParent ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__RENAMECSBYC, "_clsShdSession::_renameCSByCatalog" )
   INT32 _clsShdSession::_renameCSByCatalog( const CHAR* csName,
                                             utilCSUniqueID csUniqueID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__RENAMECSBYC ) ;

      INT32 rc                   = SDB_OK ;
      utilCSUniqueID tmpUniqueID = UTIL_UNIQUEID_NULL ;
      dmsStorageUnitID suID      = DMS_INVALID_SUID ;
      dmsStorageUnit *su         = NULL ;
      INT64 contextID            = 0 ;
      rtnContextRenameCS *pCtx   = NULL ;
      CHAR csNameInData[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      rtnContextBuf buffObj ;

      PD_CHECK( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid collection space[%s] unique id[%u]",
                csName, csUniqueID ) ;

      /// 1) get csName in data by UniqueID
      rc = _pDmsCB->idToSUAndLock( csUniqueID, suID, &su, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to loop up su by cs unique id[%u], rc: %d",
                   csUniqueID, rc ) ;

      ossStrncpy( csNameInData, su->CSName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;

      _pDmsCB->suUnlock( suID, SHARED ) ;
      suID = DMS_INVALID_SUID ;
      su = NULL ;

      PD_CHECK( 0 != ossStrcmp( csName, csNameInData ), SDB_OK, done, PDINFO,
                "Collection space name[%s] is the same, don't need to rename",
                csNameInData ) ;

      /// 2) rename cs phase 1
      rc = _pRtnCB->contextNew( RTN_CONTEXT_RENAMECS, (rtnContext **)&pCtx,
                                contextID, _pEDUCB );
      PD_RC_CHECK( rc, PDERROR, "Failed to create context, "
                   "rename collection space[%s] to[%s], rc: %d",
                   csNameInData, csName, rc ) ;

      rc = pCtx->open( csNameInData, csName, _pEDUCB, FALSE );
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, "
                   "rename collection space[%s] to[%s], rc: %d",
                   csNameInData, csName, rc ) ;

      // 3) check catalog again, in case that someone rename back
      rc = _pShdMgr->rGetCSInfo( csName, tmpUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection space[%s] from catalog, rc: %d",
                   csName, rc ) ;

      PD_CHECK( csUniqueID == tmpUniqueID,
                SDB_DMS_CS_UNIQUEID_CONFLICT, error, PDERROR,
                "Unexpected collection space[%s] unique id[%u], expect[%u]",
                csName, tmpUniqueID, csUniqueID ) ;

      /// 4) rename cs phase 2
      rc = pCtx->getMore( -1, buffObj, _pEDUCB ) ;
      if ( SDB_DMS_EOC == rc )
      {
         PD_LOG( PDEVENT,
                 "Rename collection space[%s] to [%s] by catalog succeed",
                 csNameInData, csName ) ;
         rc = SDB_OK ;
      }
      else if ( SDB_DMS_CS_NOTEXIST == rc ||
                SDB_DPS_TRANS_LOCK_INCOMPATIBLE == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get more, "
                   "rename collection space[%s] to[%s], rc: %d",
                   csNameInData, csName, rc ) ;

   done:
      if ( su )
      {
         _pDmsCB->suUnlock( suID, SHARED ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;
      }
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, _pEDUCB ) ;
         contextID = -1 ;
         pCtx = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__RENAMECSBYC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__RENAMECLBYC, "_clsShdSession::_renameCLByCatalog" )
   INT32 _clsShdSession::_renameCLByCatalog( const CHAR* clFullName,
                                             utilCLUniqueID clUniqueID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__RENAMECLBYC ) ;

      INT32 rc                   = SDB_OK ;
      utilCSUniqueID csUniqueID  = utilGetCSUniqueID( clUniqueID ) ;
      utilCLUniqueID tmpUniqueID = UTIL_UNIQUEID_NULL ;
      dmsStorageUnitID suID      = DMS_INVALID_SUID ;
      dmsStorageUnit *su         = NULL ;
      INT64 contextID            = 0 ;
      rtnContextRenameCL *pCtx   = NULL ;
      dmsMBContext *pMBContext   = NULL ;
      clsCatalogSet *pCatSet     = NULL ;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]       = { 0 } ;
      CHAR csNameInData[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      CHAR clName[ DMS_COLLECTION_NAME_SZ + 1 ]             = { 0 } ;
      CHAR clNameInData[ DMS_COLLECTION_NAME_SZ + 1 ]       = { 0 } ;
      rtnContextBuf buffObj ;

      PD_CHECK( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) &&
                UTIL_IS_VALID_CSUNIQUEID( csUniqueID ),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid collection[%s] unique id[%llu, %u]",
                clFullName, clUniqueID, csUniqueID ) ;

      /// 1) resolve cl name
      rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ),
                                     csName, DMS_COLLECTION_SPACE_NAME_SZ,
                                     clName, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to resolve collection[%s], rc: %d",
                   clFullName, rc) ;

      /// 2) get csName clName in data by UniqueID
      rc = _pDmsCB->idToSUAndLock( csUniqueID, suID, &su, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to loop up su by cs unique id[%u], rc: %d",
                   csUniqueID, rc ) ;

      rc = su->data()->getMBContextByID( &pMBContext, clUniqueID, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get mb context by cl unique id[%llu], rc: %d",
                   clUniqueID, rc ) ;

      ossStrncpy( csNameInData, su->CSName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncpy( clNameInData,
                  pMBContext->mb()->_collectionName,
                  DMS_COLLECTION_NAME_SZ ) ;

      su->data()->releaseMBContext( pMBContext ) ;
      pMBContext = NULL ;

      _pDmsCB->suUnlock( suID, SHARED ) ;
      suID = DMS_INVALID_SUID ;
      su = NULL ;

      PD_CHECK( 0 == ossStrcmp( csName, csNameInData ),
                SDB_DMS_UNIQUEID_CONFLICT, error, PDERROR,
                "Unexpected cs name[%s], expect[%s]",
                csNameInData, csName ) ;
      PD_CHECK( 0 != ossStrcmp( clName, clNameInData ),
                SDB_OK, done, PDINFO,
                "CL name[%s] is the same, don't need to rename",
                clFullName ) ;

      /// 3) rename cl phase 1
      rc = _pRtnCB->contextNew( RTN_CONTEXT_RENAMECL, (rtnContext **)&pCtx,
                                contextID, _pEDUCB );
      PD_RC_CHECK( rc, PDERROR, "Failed to create context, "
                   "rename collection[%s.%s] to [%s.%s], rc: %d",
                   csName, clNameInData, csName, clName, rc ) ;

      rc = pCtx->open( csName, clNameInData, clName, _pEDUCB, 1, FALSE );
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, "
                   "rename collection[%s.%s] to [%s.%s], rc: %d",
                   csName, clNameInData, csName, clName, rc ) ;

      /// 4) check catalog again, in case that someone rename back
      _pCatAgent->lock_w() ;
      _pCatAgent->clear( clFullName ) ;
      _pCatAgent->release_w() ;

      rc = _pShdMgr->getAndLockCataSet( clFullName, &pCatSet, TRUE ) ;
      if ( SDB_OK == rc && pCatSet )
      {
         tmpUniqueID = pCatSet->clUniqueID() ;
      }
      _pShdMgr->unlockCataSet( pCatSet ) ;

      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update collection[%s]'s catalog info, rc: %d",
                   clFullName, rc ) ;
      PD_CHECK( clUniqueID == tmpUniqueID,
                SDB_DMS_UNIQUEID_CONFLICT, error, PDERROR,
                "Unexpected collection[%s] unique id[%u], expect[%u]",
                clFullName, tmpUniqueID, clUniqueID ) ;

      /// 5) rename cl phase 2
      rc = pCtx->getMore( -1, buffObj, _pEDUCB ) ;
      if ( SDB_DMS_EOC == rc )
      {
         PD_LOG( PDEVENT,
                 "Rename collection[%s.%s] to [%s.%s] by catalog succeed",
                 csName, clNameInData, csName, clName ) ;
         rc = SDB_OK ;
      }
      else if ( SDB_DMS_NOTEXIST == rc ||
                SDB_DPS_TRANS_LOCK_INCOMPATIBLE == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get more, "
                   "rename collection[%s.%s] to [%s.%s], rc: %d",
                   csName, clNameInData, csName, clName, rc ) ;

   done:
      if ( pMBContext )
      {
         su->data()->releaseMBContext( pMBContext ) ;
         pMBContext = NULL ;
      }
      if ( su )
      {
         _pDmsCB->suUnlock( suID, SHARED ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;
      }
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, _pEDUCB ) ;
         contextID = -1 ;
         pCtx = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__RENAMECLBYC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_processSubCLResult( INT32 result,
                                              const CHAR *clFullName,
                                              const CHAR *pParent )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_OK == result )
      {
         goto done ;
      }
      else if ( ( SDB_DMS_CS_NOTEXIST == result ||
                  SDB_DMS_NOTEXIST == result ) && pmdIsPrimary() )
      {
         if ( SDB_DMS_CS_NOTEXIST == result )
         {
            rc = _createCSByCatalog( clFullName ) ;
         }
         else
         {
            rc = _createCLByCatalog( clFullName, pParent ) ;
         }
      }
      else
      {
         rc = result ;
      }

   done:
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONUPREQMSG, "_clsShdSession::_onUpdateReqMsg" )
   INT32 _clsShdSession::_onUpdateReqMsg( NET_HANDLE handle, MsgHeader * msg,
                                          utilUpdateResult &upResult )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONUPREQMSG ) ;
      PD_LOG ( PDDEBUG, "session[%s] _onUpdateReqMsg", sessionName() ) ;

      INT32 rc = SDB_OK ;
      MsgOpUpdate *pUpdate = (MsgOpUpdate*)msg ;
      INT32 flags = 0 ;
      CHAR mainCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      CHAR *pCollectionName = NULL ;
      CHAR *pMatcherBuffer = NULL ;
      CHAR *pUpdatorBuffer = NULL ;
      CHAR *pHintBuffer = NULL ;
      INT16 w = 0 ;
      INT16 clientW = pUpdate->w ;
      INT16 replSize = 0 ;

      rc = msgExtractUpdate( (CHAR*)msg, &flags, &pCollectionName,
                             &pMatcherBuffer, &pUpdatorBuffer, &pHintBuffer );
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Extract update message failed[rc:%d] in "
                  "session[%s]", rc, sessionName() ) ;
         goto error ;
      }
      _pCollectionName = pCollectionName ;

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      /// When update virtual cs
      if ( 0 == ossStrncmp( pCollectionName, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         try
         {
            BSONObj objUpdator( pUpdatorBuffer ) ;
            rc = _updateVCS( pCollectionName, objUpdator ) ;
            if ( rc )
            {
               goto error ;
            }
            goto done ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( pCollectionName, pUpdate->version,
                                    &_isMainCL, &replSize, mainCLName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _calculateW( &replSize, &clientW, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj dummy ;
         BSONObj matcher( pMatcherBuffer );
         BSONObj updator( pUpdatorBuffer );
         BSONObj hint( pHintBuffer );

         // Construct query options
         // matcher, selector, order, hint, collection, skip, limit, flag
         rtnQueryOptions options( matcher, dummy, dummy, hint, pCollectionName,
                                  0, -1, flags ) ;
         options.setMainCLName( mainCLName ) ;
         options.setUpdator( updator ) ;

         // add last op info
         MON_SAVE_OP_OPTION( eduCB()->getMonAppCB(), msg->opCode, options ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] Update: selctor: %s\nupdator: %s\n"
                  "hint: %s", sessionName(), selector.toString().c_str(),
                  updator.toString().c_str(), hint.toString().c_str() ) ; */

         if ( _isMainCL )
         {
            rc = _updateToMainCL( options, updator, _pEDUCB, _pDmsCB, _pDpsCB,
                                  w, upResult ) ;
         }
         else
         {
            BSONObj shardingKey ;

            rc = _getShardingKey( pCollectionName, shardingKey ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get sharding key of "
                       "collection[%s], rc=%d", pCollectionName, rc ) ;
               goto error ;
            }

            rc = rtnUpdate( options, updator, _pEDUCB, _pDmsCB, _pDpsCB, w,
                            &upResult,
                            shardingKey.isEmpty() ? NULL : &shardingKey ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s] Failed to create selector and updator "
                  "for update: %s", sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONUPREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONINSTREQMSG, "_clsShdSession::_onInsertReqMsg" )
   INT32 _clsShdSession::_onInsertReqMsg ( NET_HANDLE handle, MsgHeader * msg,
                                           utilInsertResult &inResult )
   {
      PD_LOG ( PDDEBUG, "session[%s] _onInsertReqMsg", sessionName() ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONINSTREQMSG ) ;
      INT32 flags = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pInsertorBuffer = NULL ;
      INT32 recordNum = 0 ;
      MsgOpInsert *pInsert = (MsgOpInsert*)msg ;
      INT16 w = 0 ;
      INT16 clientW = pInsert->w ;
      INT16 replSize = 0 ;

      rc = msgExtractInsert ( (CHAR*)msg,  &flags, &pCollectionName,
                              &pInsertorBuffer, recordNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s] extract insert msg failed[rc:%d]",
                  sessionName(), rc ) ;
         goto error ;
      }

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      if ( (flags & FLG_INSERT_CONTONDUP) && (flags & FLG_INSERT_REPLACEONDUP) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,"Conflict insert flag(CONTONDUP and REPLACEONDUP):"
                 "flag=%d,rc=%d", flags, rc ) ;
         goto error ;
      }

      _pCollectionName = pCollectionName ;

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( pCollectionName,
                                    pInsert->version,
                                    &_isMainCL, &replSize ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _calculateW( &replSize, &clientW, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
         goto error ;
      }
      try
      {
         BSONObj insertor ( pInsertorBuffer ) ;

         rtnQueryOptions options ;
         options.setCLFullName( pCollectionName ) ;
         options.setInsertor( insertor ) ;

         // add last op info
         MON_SAVE_OP_OPTION( eduCB()->getMonAppCB(), msg->opCode, options ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] Insert: %s\nCollection: %s",
                  sessionName(), insertor.toString().c_str(),
                  pCollectionName ) ; */

         if ( _isMainCL )
         {
            rc = _insertToMainCL( insertor, recordNum, flags, w, TRUE,
                                  inResult ) ;
            if ( SDB_OK == rc )
            {
               rc = _insertToMainCL( insertor, recordNum, flags, w, FALSE,
                                     inResult ) ;
            }
         }
         else
         {

            rc = rtnInsert ( pCollectionName, insertor, recordNum, flags,
                             _pEDUCB, _pDmsCB, _pDpsCB, w,
                             &inResult ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s] Failed to create insertor for "
                  "insert: %s", sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONINSTREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONDELREQMSG, "_clsShdSession::_onDeleteReqMsg" )
   INT32 _clsShdSession::_onDeleteReqMsg ( NET_HANDLE handle, MsgHeader * msg,
                                           utilDeleteResult &delResult )
   {
      PD_LOG ( PDDEBUG, "session[%s] _onDeleteReqMsg", sessionName() ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONDELREQMSG ) ;
      INT32 flags = 0 ;
      CHAR mainCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      CHAR *pCollectionName = NULL ;
      CHAR *pMatcherBuffer = NULL ;
      CHAR *pHintBuffer = NULL ;
      MsgOpDelete * pDelete = (MsgOpDelete*)msg ;
      INT16 w = 0 ;
      INT16 clientW = pDelete->w ;
      INT16 replSize = 0 ;

      rc = msgExtractDelete ( (CHAR *)msg , &flags, &pCollectionName,
                              &pMatcherBuffer, &pHintBuffer ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s] extract delete msg failed[rc:%d]",
                  sessionName(), rc ) ;
         goto error ;
      }
      _pCollectionName = pCollectionName ;

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( pCollectionName, pDelete->version,
                                    &_isMainCL, &replSize, mainCLName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _calculateW( &replSize, &clientW, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj dummy ;
         BSONObj matcher ( pMatcherBuffer ) ;
         BSONObj hint ( pHintBuffer ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] Delete: deletor: %s\nhint: %s",
                  sessionName(), deletor.toString().c_str(),
                  hint.toString().c_str() ) ; */

         // matcher, selector, order, hint, collection, skip, limit, flag
         rtnQueryOptions options( matcher, dummy, dummy, hint, pCollectionName,
                                  0, -1, flags ) ;
         options.setMainCLName( mainCLName ) ;

         // add last op info
         MON_SAVE_OP_OPTION( eduCB()->getMonAppCB(), msg->opCode, options ) ;

         if ( _isMainCL )
         {
            rc = _deleteToMainCL( options, _pEDUCB, _pDmsCB, _pDpsCB, w,
                                  delResult ) ;
         }
         else
         {
            rc = rtnDelete( options, _pEDUCB, _pDmsCB, _pDpsCB, w,
                            &delResult ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s] Failed to create deletor for "
                  "DELETE: %s", sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONDELREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONQYREQMSG, "_clsShdSession::_onQueryReqMsg" )
   INT32 _clsShdSession::_onQueryReqMsg ( NET_HANDLE handle, MsgHeader * msg,
                                          rtnContextBuf &buffObj,
                                          INT32 &startingPos,
                                          INT64 &contextID,
                                          BOOLEAN &needRollback,
                                          BSONObjBuilder *pBuilder )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONQYREQMSG ) ;

      INT32 rc = SDB_OK ;
      INT32 flags = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pQueryBuff = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderByBuffer = NULL ;
      CHAR *pHintBuffer = NULL ;
      INT64 numToSkip = -1 ;
      INT64 numToReturn = -1 ;
      MsgOpQuery *pQuery = (MsgOpQuery*)msg ;
      INT16 clientW = pQuery->w ;
      INT16 replSize = 0 ;
      INT16 w = 1 ;
      _rtnCommand *pCommand = NULL ;
      monClassQuery *monQuery = NULL ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      CHAR mainCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

      rc = msgExtractQuery ( (CHAR *)msg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQueryBuff,
                             &pFieldSelector, &pOrderByBuffer, &pHintBuffer ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s] extract query msg failed[rc:%d]",
                  sessionName(), rc ) ;
         goto error ;
      }

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      if ( !rtnIsCommand ( pCollectionName ) )
      {
         rtnContextBase *pContext = NULL ;
         _pCollectionName = pCollectionName ;

         if ( flags & FLG_QUERY_MODIFY )
         {
            needRollback = TRUE ;
            rc = _checkWriteStatus() ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
               goto error ;
            }

            rc = _checkCLStatusAndGetSth( pCollectionName, pQuery->version,
                                          &_isMainCL, &replSize,
                                          mainCLName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = _calculateW( &replSize, &clientW, w ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = _checkPrimaryWhenRead( FLG_QUERY_PRIMARY, flags ) ;
            if ( rc )
            {
               goto error ;
            }

            if ( !OSS_BIT_TEST( flags, FLG_QUERY_PRIMARY ) )
            {
               rc = _checkSecondaryWhenRead( FLG_QUERY_SECONDARY, flags ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }

            rc = _checkCLStatusAndGetSth( pCollectionName, pQuery->version,
                                          &_isMainCL, NULL, mainCLName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         try
         {
            BSONObj matcher ( pQueryBuff ) ;
            BSONObj selector ( pFieldSelector ) ;
            BSONObj orderBy ( pOrderByBuffer ) ;
            BSONObj hint ( pHintBuffer ) ;

            // Construct query options
            // matcher, selector, order, hint, collection, skip, limit, flag
            rtnQueryOptions options( matcher, selector, orderBy, hint,
                                     pCollectionName, numToSkip, numToReturn,
                                     flags ) ;
            options.setMainCLName( mainCLName ) ;

            // add last op info
            MON_SAVE_OP_OPTION( eduCB()->getMonAppCB(), msg->opCode, options ) ;

            /*
            PD_LOG ( PDDEBUG, "Session[%s] Query: matcher: %s\nselector: "
                     "%s\norderBy: %s\nhint:%s", sessionName(),
                     matcher.toString().c_str(), selector.toString().c_str(),
                     orderBy.toString().c_str(), hint.toString().c_str() ) ; */

            if ( !_isMainCL )
            {
               rc = rtnQuery( options, _pEDUCB, _pDmsCB, _pRtnCB, contextID,
                              &pContext, TRUE, FALSE ) ;
            }
            else
            {
               rc = _queryToMainCL( options, _pEDUCB, contextID, &pContext,
                                    w, needRollback ) ;
            }

            if ( rc )
            {
               goto error ;
            }

            if ( eduCB()->isAutoCommitTrans() )
            {
               pContext->setTransContext( TRUE ) ;
            }
            /// set write info
            if ( pContext && pContext->isWrite() )
            {
               pContext->setWriteInfo( _pDpsCB, w ) ;
            }

            monQuery = pContext->getMonQueryCB() ;

            if ( monQuery && pContext->getPlanRuntime() )
            {
               monQuery->accessPlanID = pContext->
                                         getPlanRuntime()->
                                         getAccessPlanID() ;
            }

            // query with return data
            if ( ( flags & FLG_QUERY_WITH_RETURNDATA ) && NULL != pContext )
            {
               rc = pContext->getMore( -1, buffObj, _pEDUCB ) ;
               if ( rc || pContext->eof() )
               {
                  _pRtnCB->contextDelete( contextID, _pEDUCB ) ;
                  contextID = -1 ;
               }
               startingPos = ( INT32 )buffObj.getStartFrom() ;

               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
               }
               else if ( rc )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to query with return "
                          "data, rc: %d", sessionName(), rc ) ;
                  goto error ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Session[%s] Failed to create matcher and "
                     "selector for QUERY: %s", sessionName(), e.what () ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         _pCollectionName = NULL ;
         _cmdCollectionName.clear() ;

         rc = rtnParserCommand( pCollectionName, &pCommand ) ;

         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Parse command[%s] failed[rc:%d]",
                     pCollectionName, rc ) ;
            goto error ;
         }

         rc = rtnInitCommand( pCommand , flags, numToSkip, numToReturn,
                              pQueryBuff, pFieldSelector, pOrderByBuffer,
                              pHintBuffer ) ;
         if ( pCommand->hasBuff() )
         {
            buffObj = pCommand->getBuff() ;
         }
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( NULL != pCommand->collectionFullName() )
         {
            _cmdCollectionName.assign( pCommand->collectionFullName() ) ;
            _pCollectionName = _cmdCollectionName.c_str() ;
         }

         MON_SAVE_CMD_DETAIL( _pEDUCB->getMonAppCB(), pCommand->type(),
                              "Command:%s, Collection:%s, Match:%s, "
                              "Selector:%s, OrderBy:%s, Hint:%s, Skip:%llu, "
                              "Limit:%lld, Flag:0x%08x(%u)",
                              pCollectionName, _cmdCollectionName.c_str(),
                              BSONObj(pQueryBuff).toString().c_str(),
                              BSONObj(pFieldSelector).toString().c_str(),
                              BSONObj(pOrderByBuffer).toString().c_str(),
                              BSONObj(pHintBuffer).toString().c_str(),
                              numToSkip, numToReturn, flags, flags ) ;

         if ( pCommand->writable () )
         {
            rc = _checkWriteStatus() ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = _checkPrimaryWhenRead( FLG_QUERY_PRIMARY, flags ) ;
            if ( rc )
            {
               goto error ;
            }

            if ( !OSS_BIT_TEST( flags, FLG_QUERY_PRIMARY ) )
            {
               rc = _checkSecondaryWhenRead( FLG_QUERY_SECONDARY, flags ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
         }

         //check cata
         if ( pCommand->collectionFullName() )
         {
            rc = _checkCLStatusAndGetSth( pCommand->collectionFullName(),
                                          pQuery->version, &_isMainCL,
                                          &replSize, mainCLName, &clUniqueID ) ;

            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( pCommand->writable() )
            {
               rc = _calculateW( &replSize, &clientW, w ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
                  goto error ;
               }
            }

            pCommand->setMainCLName( mainCLName ) ;

            if ( CMD_CREATE_COLLECTION == pCommand->type() )
            {
               _rtnCreateCollection *pCrtCL = (_rtnCreateCollection*)pCommand ;
               pCrtCL->setCLUniqueID( clUniqueID ) ;
            }
         }
         else if ( pCommand->spaceName() )
         {
            rc = _checkReplStatus() ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to check repl status, rc: %d", rc ) ;
               goto error ;
            }
            /// wait freezing window
            if ( pCommand->writable() )
            {
               rc = _pFreezingWindow->waitForOpr( pCommand->spaceName(),
                                                  _pEDUCB,
                                                  _pEDUCB->isWritingDB() ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Wait freezing window for "
                          "collectionspace(%s) failed, rc: %d",
                          pCommand->spaceName(), rc ) ;
                  goto error ;
               }
            }
         }

         if ( CMD_LOAD_COLLECTIONSPACE == pCommand->type() )
         {
            _rtnLoadCollectionSpace *pLoadcs = (_rtnLoadCollectionSpace*)pCommand ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj ;

            rc = _pShdMgr->rGetCSInfo( pLoadcs->spaceName(), csUniqueID,
                                       NULL, NULL, NULL, &clInfoObj ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Get collection space[%s] unique "
                       "id from catalog failed, rc: %d", sessionName(),
                       pLoadcs->spaceName(), rc ) ;
               goto error ;
            }

            pLoadcs = (_rtnLoadCollectionSpace*)pCommand ;
            pLoadcs->setCSUniqueID( csUniqueID ) ;
            pLoadcs->setCLInfo( clInfoObj ) ;
         }

         PD_LOG ( PDDEBUG, "Command: %s", pCommand->name () ) ;

         /// sometimes we can not get catainfo from command
         /// request. here if w < 1, we set it with 1.
         if ( w < 1 )
         {
            w = 1 ;
         }

         if ( _isMainCL )
         {
            rc = _runOnMainCL( pCollectionName, pCommand, flags, numToSkip,
                               numToReturn, pQueryBuff, pFieldSelector,
                               pOrderByBuffer, pHintBuffer, w,
                               contextID, pBuilder ) ;
         }
         else
         {
            if ( CMD_CREATE_COLLECTION == pCommand->type() )
            {
               rc = _testCollectionBeforeCreate( pCommand->collectionFullName(),
                                                 clUniqueID ) ;
               if ( SDB_DMS_CS_NOTEXIST == rc ||
                    SDB_DMS_CS_UNIQUEID_CONFLICT == rc ||
                    SDB_DMS_UNIQUEID_CONFLICT == rc ||
                    SDB_DMS_CS_REMAIN == rc ||
                    SDB_DMS_REMAIN == rc )
               {
                  goto error ;
               }
            }
            else if ( CMD_SNAPSHOT_COLLECTIONS == pCommand->type() )
            {
               _rtnMonInnerBase *pMonBase = (_rtnMonInnerBase *)pCommand ;
               clsShowMainCLMode mode = SHOW_MODE_SUB ;
               IRtnMonProcessor *pProcessor = NULL ;
               // Disable main cl mode for SEQUOIADBMAINSTREAM-5578
               // rc = clsParseShowMainCLModeHint( BSONObj(pHintBuffer), mode ) ;
               // PD_RC_CHECK( rc, PDERROR, "Failed to parse hint, rc=%d", rc );
               pProcessor = SDB_OSS_NEW clsMainCLMonAggregator( mode ) ;
               if ( !pProcessor )
               {
                  rc = SDB_OOM ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to alloc monitor data processor" ) ;
               }
               pMonBase->setDataProcessor( pProcessor ) ;
            }
            //run command
            rc = rtnRunCommand( pCommand, getServiceType(),
                                _pEDUCB, _pDmsCB, _pRtnCB,
                                _pDpsCB, w, &contextID ) ;
            if ( pCommand->hasBuff() )
            {
               buffObj = pCommand->getBuff() ;
            }
            if ( rc && pBuilder && pCommand->getResult() )
            {
               pCommand->getResult()->toBSON( *pBuilder ) ;
            }
            if ( rc && CMD_CREATE_COLLECTION == pCommand->type() )
            {
               /// create collection failed, so we need to clear cache
               _pCatAgent->lock_w () ;
               _pCatAgent->clear ( pCommand->collectionFullName() ) ;
               _pCatAgent->release_w () ;
            }
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Run command[%s] failed, rc: %d",
                    pCommand->name(), rc ) ;
            goto error ;
         }

      }

   done:
      if ( pCommand )
      {
         rtnReleaseCommand( &pCommand ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONQYREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONGETMOREREQMSG, "_clsShdSession::_onGetMoreReqMsg" )
   INT32 _clsShdSession::_onGetMoreReqMsg( MsgHeader * msg,
                                           rtnContextBuf &buffObj,
                                           INT32 & startingPos,
                                           INT64 &contextID,
                                           BOOLEAN &needRollback )
   {
      PD_LOG ( PDDEBUG, "session[%s] _onGetMoreReqMsg", sessionName() ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONGETMOREREQMSG ) ;
      INT32 numToRead = 0 ;
      rtnContext *pContext = NULL ;

      rc = msgExtractGetMore ( (CHAR*)msg, &numToRead, &contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s] extract GETMORE msg failed[rc:%d]",
                  sessionName(), rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, NumToRead:%d",
                          contextID, numToRead ) ;
      /*
      PD_LOG ( PDDEBUG, "GetMore: contextID:%lld\nnumToRead: %d", contextID,
               numToRead ) ; */

      pContext = _pRtnCB->contextFind ( contextID, eduCB() ) ;
      if ( !pContext )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }
      needRollback = pContext->needRollback() ;

      /// trans context
      if ( pContext->isTransContext() && !eduCB()->isTransaction() )
      {
         rc = rtnTransBegin( eduCB(), TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      rc = rtnGetMore ( pContext, numToRead, buffObj, eduCB(), _pRtnCB ) ;
      if ( rc )
      {
         contextID = -1 ;
         goto error ;
      }

      startingPos = ( INT32 )buffObj.getStartFrom() ;

   done:
      if ( SDB_DMS_EOC == rc )
      {
         needRollback = FALSE ;
      }

      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONGETMOREREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONKILLCTXREQMSG, "_clsShdSession::_onKillContextsReqMsg" )
   INT32 _clsShdSession::_onKillContextsReqMsg ( NET_HANDLE handle,
                                                 MsgHeader * msg )
   {
      PD_LOG ( PDDEBUG, "session[%s] _onKillContextsReqMsg", sessionName() ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONKILLCTXREQMSG ) ;
      INT32 contextNum = 0 ;
      INT64 *pContextIDs = NULL ;

      rc = msgExtractKillContexts ( (CHAR*)msg, &contextNum, &pContextIDs ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s] extract KILLCONTEXT msg failed[rc:%d]",
                  sessionName(), rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextNum:%d, ContextID:%lld",
                          contextNum, pContextIDs[0] ) ;

      /*
      if ( contextNum > 0 )
      {
         PD_LOG ( PDDEBUG, "KillContext: contextNum:%d\ncontextID: %lld",
                  contextNum, pContextIDs[0] ) ;
      } */

      rc = rtnKillContexts ( contextNum, pContextIDs, _pEDUCB, _pRtnCB ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDSESS__ONKILLCTXREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_onMsgReq ( NET_HANDLE handle, MsgHeader * msg )
   {
      return rtnMsg( (MsgOpMsg*)msg ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__ONINRPTMSG, "_clsShdSession::_onInterruptMsg" )
   INT32 _clsShdSession::_onInterruptMsg ( NET_HANDLE handle, MsgHeader * msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDSESS__ONINRPTMSG ) ;
      //delete all contextID
      if ( _pEDUCB )
      {
         INT64 contextID = -1 ;
         while ( -1 != ( contextID = _pEDUCB->contextPeek() ) )
         {
            _pRtnCB->contextDelete ( contextID, NULL ) ;
         }

         INT32 rcTmp = _rollbackTrans();
         if ( rcTmp )
         {
            PD_LOG ( PDERROR, "Failed to rollback(rc=%d)", rcTmp ) ;
         }
      }

      PD_TRACE_EXIT ( SDB__CLSSHDSESS__ONINRPTMSG ) ;
      return SDB_OK ;
   }

   INT32 _clsShdSession::_onTransBeginMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      MsgOpTransBegin *pTransBegin = ( MsgOpTransBegin* )msg ;

      if ( DPS_TRANS_WAIT_COMMIT == eduCB()->getTransStatus() )
      {
         rc = SDB_RTN_EXIST_INDOUBT_TRANS ;
         goto error ;
      }

      rc = _checkPrimaryStatus() ;
      if ( rc )
      {
         goto error ;
      }

      /// Old trans begin msg is only a MsgHeader
      if ( msg->messageLength > (INT32)sizeof( MsgHeader ) &&
           DPS_INVALID_TRANS_ID != pTransBegin->transID &&
           0 != DPS_TRANS_GET_NODEID( pTransBegin->transID ) )
      {
         rc = rtnTransBegin( _pEDUCB, FALSE, pTransBegin->transID ) ;
      }
      else
      {
         rc = rtnTransBegin( _pEDUCB ) ;
      }

      if ( SDB_OK == rc )
      {
         /// unset all trans context
         rtnUnsetTransContext( eduCB(), _pRtnCB ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_onTransCommitMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      CHAR tmpID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      CHAR tmpAttr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;

      if ( !_pReplSet->primaryIsMe() )
      {
         return SDB_CLS_NOT_PRIMARY ;
      }
      if ( _pEDUCB->getTransID() == DPS_INVALID_TRANS_ID )
      {
         return SDB_DPS_TRANS_NO_TRANS ;
      }
      dpsTransIDToString( eduCB()->getTransID(),
                          tmpID, DPS_TRANS_STR_LEN ) ;
      dpsTransIDAttrToString( eduCB()->getTransID(),
                              tmpAttr, DPS_TRANS_STR_LEN ) ;
      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                          "TransactionID: %s(%s)", tmpID, tmpAttr ) ;
      return rtnTransCommit( _pEDUCB, _pDpsCB ) ;
   }

   INT32 _clsShdSession::_onTransRollbackMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      CHAR tmpID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      CHAR tmpAttr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;

      if ( !_pReplSet->primaryIsMe() )
      {
         return SDB_CLS_NOT_PRIMARY ;
      }

      dpsTransIDToString( eduCB()->getTransID(),
                          tmpID, DPS_TRANS_STR_LEN ) ;
      dpsTransIDAttrToString( eduCB()->getTransID(),
                              tmpAttr, DPS_TRANS_STR_LEN ) ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_ROLLBACK_REQ,
                          "TransactionID: %s(%s)", tmpID, tmpAttr ) ;
      return _rollbackTrans() ;
   }

   INT32 _clsShdSession::_onTransCommitPreMsg( NET_HANDLE handle,
                                               MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      CHAR tmpID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      CHAR tmpAttr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;

      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      MsgOpTransCommitPre *pCommitPreMsg = ( MsgOpTransCommitPre* )msg ;

      INT16 replSize = optCB->transReplSize() ;
      INT16 w = 0 ;

      if ( !_pReplSet->primaryIsMe() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }
      if ( _pEDUCB->getTransID() == DPS_INVALID_TRANS_ID )
      {
         rc = SDB_DPS_TRANS_NO_TRANS ;
         goto error ;
      }

      dpsTransIDToString( eduCB()->getTransID(),
                          tmpID, DPS_TRANS_STR_LEN ) ;
      dpsTransIDAttrToString( eduCB()->getTransID(),
                              tmpAttr, DPS_TRANS_STR_LEN ) ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_COMMITPRE_REQ,
                          "TransactionID: %s(%s)", tmpID, tmpAttr ) ;

      rc = _calculateW( &replSize, NULL, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to calculate w, rc: %d", rc ) ;
         goto error ;
      }

      rc = rtnTransPreCommit( _pEDUCB, pCommitPreMsg->nodeNum,
                              pCommitPreMsg->nodes, w, _pDpsCB ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_checkTransAutoCommit( const MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      if ( !_pEDUCB->isTransaction() )
      {
         if ( _pEDUCB->getTransExecutor()->isTransAutoCommit() )
         {
            rc = _checkPrimaryStatus() ;
            if ( SDB_OK == rc )
            {
               rc = rtnTransBegin( _pEDUCB, TRUE ) ;
            }
         }
         else
         {
            rc = SDB_DPS_TRANS_NO_TRANS ;
         }
      }

      return rc ;
   }

   INT32 _clsShdSession::_onTransUpdateReqMsg ( NET_HANDLE handle,
                                                MsgHeader *msg,
                                                utilUpdateResult &upResult )
   {
      INT32 rc = _checkTransAutoCommit( msg ) ;
      if ( SDB_OK == rc )
      {
         rc = _onUpdateReqMsg( handle, msg, upResult ) ;
      }
      return rc ;
   }

   INT32 _clsShdSession::_onTransInsertReqMsg ( NET_HANDLE handle,
                                                MsgHeader *msg,
                                                utilInsertResult &inResult )
   {
      INT32 rc = _checkTransAutoCommit( msg ) ;
      if ( SDB_OK == rc )
      {
         rc = _onInsertReqMsg( handle, msg, inResult ) ;
      }
      return rc ;
   }

   INT32 _clsShdSession::_onTransDeleteReqMsg ( NET_HANDLE handle,
                                                MsgHeader *msg,
                                                utilDeleteResult &delResult )
   {
      INT32 rc = _checkTransAutoCommit( msg ) ;
      if ( SDB_OK == rc )
      {
         rc = _onDeleteReqMsg( handle, msg, delResult ) ;
      }
      return rc ;
   }

   INT32 _clsShdSession::_onTransQueryReqMsg( NET_HANDLE handle,
                                              MsgHeader * msg,
                                              rtnContextBuf & buffObj,
                                              INT32 & startingPos,
                                              INT64 & contextID,
                                              BOOLEAN &needRollback )
   {
      INT32 rc = _checkTransAutoCommit( msg ) ;
      if ( SDB_OK == rc )
      {
         rc = _onQueryReqMsg( handle, msg, buffObj, startingPos,
                              contextID, needRollback, NULL ) ;
      }
      return rc ;
   }

   void _clsShdSession::_login()
   {
      UINT32 ip = 0, port = 0 ;
      UINT32 auditMask = 0, auditConfigMask = 0 ;

      _delayLogin = FALSE ;
      _logout     = FALSE ;

      if ( !_username.empty() )
      {
         _client.authenticate( _username.c_str(), _passwd.c_str() ) ;
      }

      if ( !_objDelayInfo.isEmpty() )
      {
         _updateVCS( CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO,
                     _objDelayInfo ) ;
         _objDelayInfo = BSONObj() ;
      }

      getAuditConfig( auditMask, auditConfigMask ) ;
      pdUpdateCurAuditMask( AUDIT_LEVEL_USER, auditMask, auditConfigMask ) ;

      /// update trans conf
      if ( 0 != _transConf.getTransConfMask() )
      {
         eduCB()->getTransExecutor()->copyFrom( _transConf ) ;
      }

      eduCB()->setSource( _source.c_str() ) ;

      ossUnpack32From64( identifyID(), ip, port ) ;
      /// set detail name
      CHAR szTmpIP[ 50 ] = { 0 } ;
      ossIP2Str( ip, szTmpIP, sizeof(szTmpIP) - 1 ) ;
      ossSnprintf( _detailName, SESSION_NAME_LEN, "%s,R-IP:%s,R-Port:%u",
                   _pmdAsyncSession::sessionName(), szTmpIP,
                   port ) ;
      eduCB()->setName( _detailName ) ;

      /// audit
      CHAR szTmpID[ 20 ] = { 0 } ;
      ossSnprintf( szTmpID, sizeof(szTmpID) - 1, "%llu", eduID() ) ;
      PD_AUDIT_OP( AUDIT_ACCESS, MSG_AUTH_VERIFY_REQ, AUDIT_OBJ_SESSION,
                   szTmpID, SDB_OK, "User[UserName:%s, FromIP:%s, "
                   "FromPort:%u, FromSession:%llu, FromTID:%u, Source:%s] "
                   "login succeed", _client.getUsername(),
                   szTmpIP, port, identifyEDUID(), identifyTID(),
                   _source.c_str() ) ;
   }

   INT32 _clsShdSession::_onSessionInitReqMsg ( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      MsgComSessionInitReq *pMsgReq = (MsgComSessionInitReq*)msg ;
      MsgRouteID localRouteID = routeAgent()->localID() ;

      /// check wether the route id is matched
      if ( pMsgReq->dstRouteID.value != localRouteID.value )
      {
         rc = SDB_INVALID_ROUTEID;
         PD_LOG ( PDERROR, "Session init failed: route id does not match."
                  "Message info: [%s], Local route id: %s",
                  msg2String( msg ).c_str(),
                  routeID2String( localRouteID ).c_str() ) ;
      }
      else if ( (UINT32)msg->messageLength > sizeof( MsgComSessionInitReq ) )
      {
         /// set user name info
         try
         {
            BSONObj obj( pMsgReq->data ) ;
            BSONElement user = obj.getField( SDB_AUTH_USER ) ;
            BSONElement passwd = obj.getField( SDB_AUTH_PASSWD ) ;

            _client.authenticate( user.valuestrsafe(),
                                  passwd.valuestrsafe() ) ;

            BSONElement eMask = obj.getField( FIELD_NAME_AUDIT_MASK ) ;
            BSONElement eConfigMask = obj.getField( FIELD_NAME_AUDIT_CONFIG_MASK ) ;

            /// set audit mask
            if ( eMask.isNumber() && eConfigMask.isNumber() )
            {
               setAuditConfig( (UINT32)eMask.numberInt(),
                               (UINT32)eConfigMask.numberInt() ) ;
            }

            _transConf.fromBson( obj ) ;

            if ( isSetLogout() )
            {
               BSONElement remoteIP = obj.getField( FIELD_NAME_REMOTE_IP ) ;
               BSONElement remotePort = obj.getField( FIELD_NAME_REMOTE_PORT ) ;

               _source = obj.getField( FIELD_NAME_SOURCE ).str() ;

               if ( String == remoteIP.type() && NumberInt == remotePort.type() )
               {
                  _client.setFromInfo( remoteIP.valuestr(),
                                       (UINT16)remotePort.numberInt() ) ;
               }
               /// set the remote info into this session
               setIdentifyInfo( pMsgReq->localIP, pMsgReq->localPort,
                                pMsgReq->srcRouteID,
                                pMsgReq->localTID, pMsgReq->localSessionID ) ;
               /// inner login
               _login() ;
            }
            else
            {
               UINT32 auditMask = 0, auditConfigMask = 0 ;
               getAuditConfig( auditMask, auditConfigMask ) ;
               pdUpdateCurAuditMask( AUDIT_LEVEL_USER, auditMask,
                                     auditConfigMask ) ;
               /// update trans conf
               if ( 0 != _transConf.getTransConfMask() )
               {
                  eduCB()->getTransExecutor()->copyFrom( _transConf ) ;
               }
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            /// do not report error
         }
      }
      return rc ;
   }

   INT32 _clsShdSession::_onTransStopEvnt( pmdEDUEvent *event )
   {
      // rollback transaction
      INT32 rcTmp = _rollbackTrans() ;
      if ( rcTmp )
      {
         PD_LOG ( PDERROR, "Failed to rollback(rc=%d)", rcTmp ) ;
      }

      // disconnect remote
      MsgHeader msg ;
      msg.messageLength = sizeof( MsgHeader ) ;
      msg.opCode = MSG_BS_DISCONNECT ;
      msg.TID = CLS_TID( _sessionID ) ;
      msg.routeID.value = MSG_INVALID_ROUTEID ;
      msg.requestID = 0L ;
      routeAgent()->syncSend( _netHandle, &msg ) ;

      // close handle
      routeAgent()->close( _netHandle ) ;

      PD_LOG( PDEVENT, "On transaction stop event: close handle %d",
              _netHandle ) ;

      return SDB_OK ;
   }

   INT32 _clsShdSession::_insertToMainCL( BSONObj &objs, INT32 objNum,
                                          INT32 flags, INT16 w,
                                          BOOLEAN onlyCheck,
                                          utilInsertResult &inResult )
   {
      INT32 rc = SDB_OK ;
      ossValuePtr pCurPos = 0 ;
      INT32 totalObjsNum = 0 ;

      try
      {
         PD_CHECK( !objs.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                  "Insert record can't be empty" );
         pCurPos = (ossValuePtr)objs.objdata();
         while ( totalObjsNum < objNum )
         {
            BSONObj subObjsInfo( (const CHAR *)pCurPos );
            INT32 subObjsNum = 0;
            UINT32 subObjsSize = 0;
            const CHAR *pSubCLName = NULL;
            BSONElement beSubObjsNum;
            BSONElement beSubObjsSize;
            BSONElement beSubCLName;
            BSONObj insertor;
            beSubObjsNum = subObjsInfo.getField( FIELD_NAME_SUBOBJSNUM );
            PD_CHECK( beSubObjsNum.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get the field(%s)", FIELD_NAME_SUBOBJSNUM );
            subObjsNum = beSubObjsNum.numberInt();

            beSubObjsSize = subObjsInfo.getField( FIELD_NAME_SUBOBJSSIZE );
            PD_CHECK( beSubObjsSize.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get the field(%s)", FIELD_NAME_SUBOBJSSIZE );
            subObjsSize = beSubObjsSize.numberInt();

            beSubCLName = subObjsInfo.getField( FIELD_NAME_SUBCLNAME );
            PD_CHECK( beSubCLName.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Failed to get the field(%s)",
                      FIELD_NAME_SUBCLNAME );
            pSubCLName = beSubCLName.valuestr();

            pCurPos += ossAlignX( (ossValuePtr)subObjsInfo.objsize(), 4 );
            ++totalObjsNum;
            insertor = BSONObj( (CHAR *)pCurPos ) ;

            if ( onlyCheck )
            {
               rc = _pFreezingWindow->waitForOpr( pSubCLName, _pEDUCB,
                                                  _pEDUCB->isWritingDB() ) ;
               PD_RC_CHECK( rc, PDERROR, "Wait freezing window for "
                            "collection(%s) failed, rc: %d",
                            pSubCLName, rc ) ;
            }
            else
            {
               while ( TRUE )
               {
                  /// insert to sub collection
                  rc = rtnInsert ( pSubCLName, insertor, subObjsNum, flags,
                                   _pEDUCB, _pDmsCB, _pDpsCB, w,
                                   &inResult ) ;
                  if ( rc )
                  {
                     rc = _processSubCLResult( rc, pSubCLName,
                                               _pCollectionName ) ;
                     if ( SDB_OK == rc )
                     {
                        continue ;
                     }
                  }
                  break ;
               }
               if( rc )
               {
                  PD_LOG( PDERROR, "Session[%s]: Failed to insert to "
                          "sub-collection[%s] of main-collection[%s], rc: %d",
                          sessionName(), pSubCLName, _pCollectionName, rc ) ;
                  goto error ;
               }
            }

            /// continue next sub collection
            pCurPos += subObjsSize ;
            totalObjsNum += subObjsNum ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      if ( onlyCheck )
      {
         // need recheck version of main-collection
         rc = _checkCLVersion( _pCollectionName, _clVersion ) ;
         PD_RC_CHECK( rc, PDDEBUG, "Failed to check message version [%d] of "
                      "collection [%s], rc: %d", _clVersion, _pCollectionName,
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_getShardingKey( const CHAR* clName,
                                          BSONObj &shardingKey )
   {
      INT32 rc = SDB_OK ;
      _clsCatalogSet* set = NULL ;

      for( ;; )
      {
         _pCatAgent->lock_r() ;

         set = _pCatAgent->collectionSet( clName ) ;
         if ( NULL == set )
         {
            _pCatAgent->release_r() ;

            rc = _pShdMgr->syncUpdateCatalog( clName ) ;
            if ( SDB_OK == rc )
            {
               continue ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to update catalog of collection[%s], rc=%d",
                       clName, rc ) ;
               goto error ;
            }
         }
         else
         {
            break ;
         }
      }

      SDB_ASSERT( NULL != set, "_clsCatalogSet should not be NULL" ) ;

      if ( set->isSharding() )
      {
         shardingKey = set->getShardingKey() ;
      }

      _pCatAgent->release_r() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_includeShardingOrder( const CHAR *pCollectionName,
                                                const BSONObj &orderBy,
                                                BOOLEAN &result )
   {
      INT32 rc = SDB_OK;
      BSONObj shardingKey;
      _clsCatalogSet *pCataSet = NULL;
      BOOLEAN catLocked = FALSE;
      result = FALSE;
      BOOLEAN isRange = FALSE;
      try
      {
         if ( orderBy.isEmpty() )
         {
            goto done;
         }

         _pCatAgent->lock_r () ;
         catLocked = TRUE;
         pCataSet = _pCatAgent->collectionSet( pCollectionName );
         if ( NULL == pCataSet )
         {
            _pCatAgent->release_r () ;
            catLocked = FALSE;

            rc = SDB_CLS_NO_CATALOG_INFO ;
            PD_LOG( PDERROR, "can not find collection:%s", pCollectionName ) ;
            goto error ;
         }
         isRange = pCataSet->isRangeSharding();
         shardingKey = pCataSet->getShardingKey().getOwned() ;
         _pCatAgent->release_r () ;
         catLocked = FALSE;
         if ( !isRange )
         {
            goto done;
         }
         if ( !shardingKey.isEmpty() )
         {
            result = TRUE;
            BSONObjIterator iterOrder( orderBy );
            BSONObjIterator iterSharding( shardingKey );
            while( iterOrder.more() && iterSharding.more() )
            {
               BSONElement beOrder = iterOrder.next();
               BSONElement beSharding = iterSharding.next();
               if ( 0 != beOrder.woCompare( beSharding ) )
               {
                  result = FALSE;
                  break;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "occur unexpected error:%s",
                      e.what() );
      }
   done:
      if ( catLocked )
      {
         _pCatAgent->release_r () ;
      }
      return rc;
   error:
      goto done;
   }

   INT32 _clsShdSession::_queryToMainCL( rtnQueryOptions &options,
                                         pmdEDUCB *cb,
                                         SINT64 &contextID,
                                         _rtnContextBase **ppContext,
                                         INT16 w,
                                         BOOLEAN isWrite )
   {
      INT32 rc = SDB_OK ;
      CLS_SUBCL_LIST strSubCLList ;
      BSONObj boNewMatcher ;
      BOOLEAN includeShardingOrder = FALSE ;
      SINT64 tmpContextID = -1 ;
      rtnContext * pContext = NULL ;

      SDB_ASSERT( options.getCLFullName(), "collection name can't be NULL!" ) ;
      SDB_ASSERT( cb, "educb can't be NULL!" ) ;

      rc = _includeShardingOrder( options.getCLFullName(), options.getOrderBy(),
                                  includeShardingOrder ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check order-key(rc=%d)", rc ) ;

      rc = _getSubCLList( options.getQuery(), options.getCLFullName(),
                          isWrite, boNewMatcher, strSubCLList ) ;
      if ( rc != SDB_OK )
      {
         goto error;
      }

      options.setQuery( boNewMatcher ) ;

      if ( includeShardingOrder )
      {
         rc = _sortSubCLListByBound( options.getCLFullName(), strSubCLList ) ;
         if ( rc )
         {
            /// can't optimize
            if ( SDB_CLS_NO_CATALOG_INFO == rc || SDB_SYS == rc )
            {
               includeShardingOrder = FALSE ;
            }
            else
            {
               goto error ;
            }
         }
      }

      if ( options.testFlag( FLG_QUERY_EXPLAIN ) )
      {
         rtnContextMainCLExplain *pContextMainCL = NULL ;

         rc = _pRtnCB->contextNew( RTN_CONTEXT_MAINCL_EXP,
                                   (rtnContext **)&pContextMainCL,
                                   tmpContextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create new main-collection "
                      "explain context, rc: %d", rc ) ;

         pContext = pContextMainCL ;

         rc = pContextMainCL->open( options, strSubCLList,
                                    includeShardingOrder, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open main-collection context, "
                      "rc: %d", rc ) ;
      }
      else
      {
         rtnContextMainCL *pContextMainCL = NULL ;

         rc = _pRtnCB->contextNew( RTN_CONTEXT_MAINCL,
                                   (rtnContext **)&pContextMainCL,
                                   tmpContextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create new main-collection "
                      "context, rc: %d", rc ) ;

         pContext = pContextMainCL ;

         if ( options.canPrepareMore() )
         {
            pContextMainCL->setPrepareMoreData( TRUE ) ;
         }

         /// must set before open
         pContextMainCL->setWriteInfo( _pDpsCB, w ) ;

         rc = pContextMainCL->open( options, strSubCLList,
                                    includeShardingOrder, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open main-collection context, "
                      "rc: %d", rc ) ;
      }

      // Get start timestamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         pContext->getMonCB()->recordStartTimestamp() ;
      }

      contextID = tmpContextID ;
      if ( ppContext )
      {
         *ppContext = pContext ;
      }
      tmpContextID = -1 ;
      pContext = NULL ;

   done :
      return rc ;

   error :
      if ( -1 != tmpContextID )
      {
         _pRtnCB->contextDelete( tmpContextID, cb ) ;
         tmpContextID = -1 ;
      }
      goto done ;
   }

   INT32 _clsShdSession::_sortSubCLListByBound( const CHAR *pCollectionName,
                                                CLS_SUBCL_LIST &strSubCLList )
   {
      INT32 rc = SDB_OK ;
      ossPoolSet<string> setNameFilter ;
      CLS_SUBCL_LIST strSubCLListTmp ;
      _clsCatalogSet *pCataSet = NULL ;

      _pCatAgent->lock_r () ;
      pCataSet = _pCatAgent->collectionSet( pCollectionName ) ;
      if ( NULL == pCataSet )
      {
         _pCatAgent->release_r () ;
         rc = SDB_CLS_NO_CATALOG_INFO ;
         PD_LOG( PDWARNING, "Can't find collection(%s)'s catalog information",
                 pCollectionName ) ;
         goto error ;
      }
      pCataSet->getSubCLList( strSubCLListTmp, SUBCL_SORT_BY_BOUND ) ;
      _pCatAgent->release_r () ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Get sub collection list failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         CLS_SUBCL_LIST_IT it = strSubCLList.begin() ;
         while( it != strSubCLList.end() )
         {
            setNameFilter.insert( *it ) ;
            ++it ;
         }

         ossPoolSet<string>::iterator itSet ;
         strSubCLList.clear() ;
         it = strSubCLListTmp.begin() ;
         while( it != strSubCLListTmp.end() )
         {
            itSet = setNameFilter.find( *it ) ;
            /// Found
            if ( itSet != setNameFilter.end() )
            {
               strSubCLList.push_back( *it ) ;
               setNameFilter.erase( itSet ) ;
            }
            ++it ;
         }

         /// has some sub cl not found
         if ( !setNameFilter.empty() )
         {
            rc = SDB_SYS ;
            itSet = setNameFilter.begin() ;
            while( itSet != setNameFilter.end() )
            {
               strSubCLList.push_back( *itSet ) ;
               ++itSet ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_getSubCLList( const BSONObj &matcher,
                                        const CHAR *pCollectionName,
                                        BOOLEAN isWrite,
                                        BSONObj &boNewMatcher,
                                        CLS_SUBCL_LIST &strSubCLList )
   {
      INT32 rc = SDB_OK;

      try
      {
         BSONObjBuilder bobNewMatcher;
         BSONObjIterator iter( matcher );
         while( iter.more() )
         {
            BSONElement beTmp = iter.next();
            if ( beTmp.type() == Array &&
                 0 == ossStrcmp(beTmp.fieldName(), CAT_SUBCL_NAME ) )
            {
               BSONObj boSubCLList = beTmp.embeddedObject();
               BSONObjIterator iterSubCL( boSubCLList );
               while( iterSubCL.more() )
               {
                  BSONElement beSubCL = iterSubCL.next();
                  string strSubCLName = beSubCL.str();
                  if ( !strSubCLName.empty() )
                  {
                     strSubCLList.push_back( strSubCLName ) ;

                     /// wait for freezing window
                     if ( isWrite )
                     {
                        rc = _pFreezingWindow->waitForOpr( strSubCLName.c_str(),
                                                           _pEDUCB,
                                                           _pEDUCB->isWritingDB() ) ;
                        PD_RC_CHECK( rc, PDERROR, "Wait freezing window for "
                                     "collection(%s) failed, rc: %d",
                                     strSubCLName.c_str(), rc ) ;
                     }
                  }
               }
            }
            else
            {
               bobNewMatcher.append( beTmp );
            }
         }
         boNewMatcher = bobNewMatcher.obj();
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "occur unexpected error:%s",
                      e.what() );
      }

      if ( strSubCLList.empty() )
      {
         rc = _getSubCLList( pCollectionName, isWrite, strSubCLList ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         // need recheck version of main-collection
         rc = _checkCLVersion( pCollectionName, _clVersion ) ;
         PD_RC_CHECK( rc, PDDEBUG, "Failed to check message version [%d] of "
                      "collection [%s], rc: %d", _clVersion, pCollectionName,
                      rc ) ;
      }

      PD_CHECK( !strSubCLList.empty(), SDB_INVALID_MAIN_CL, error, PDERROR,
                "main-collection has no sub-collection!" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_getSubCLList( const CHAR *pCollectionName,
                                        BOOLEAN isWrite,
                                        CLS_SUBCL_LIST &subCLList )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCLName = NULL ;
      CLS_SUBCL_LIST strSubCLListTmp ;
      clsCatalogSet *pCataSet = NULL ;
      CLS_SUBCL_LIST_IT iter ;

      _pCatAgent->lock_r () ;
      pCataSet = _pCatAgent->collectionSet( pCollectionName ) ;
      if ( NULL == pCataSet )
      {
         _pCatAgent->release_r () ;
         rc = SDB_CLS_NO_CATALOG_INFO ;
         PD_LOG( PDERROR, "can not find collection:%s", pCollectionName ) ;
         goto error ;
      }
      pCataSet->getSubCLList( strSubCLListTmp ) ;
      _pCatAgent->release_r() ;

      /// check all sub collection is valid
      iter = strSubCLListTmp.begin() ;
      while( iter != strSubCLListTmp.end() )
      {
         pSubCLName = (*iter).c_str() ;

         _pCatAgent->lock_r() ;
         pCataSet = _pCatAgent->collectionSet( pSubCLName ) ;
         if ( NULL == pCataSet )
         {
            _pCatAgent->release_r() ;

            rc = _pShdMgr->syncUpdateCatalog( pSubCLName ) ;
            if ( SDB_OK == rc )
            {
               continue ;
            }
            else
            {
               goto error ;
            }
         }
         /// not on the node, ignore
         else if ( 0 == pCataSet->groupCount() )
         {
            _pCatAgent->release_r() ;

            _pCatAgent->lock_w() ;
            _pCatAgent->clear( pSubCLName ) ;
            _pCatAgent->release_w() ;

            ++iter ;
            continue ;
         }
         _pCatAgent->release_r() ;

         /// push to list
         subCLList.push_back( *iter ) ;
         if ( isWrite )
         {
            rc = _pFreezingWindow->waitForOpr( (*iter).c_str(), _pEDUCB,
                                               _pEDUCB->isWritingDB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Wait freezing window for "
                         "collection(%s) failed, rc: %d",
                         (*iter).c_str(), rc ) ;
         }

         ++iter ;
      }

      if ( subCLList.empty() )
      {
         /// is empty main collection
         _pCatAgent->lock_w() ;
         _pCatAgent->clear( pCollectionName ) ;
         _pCatAgent->release_w() ;
      }
      else
      {
         // need recheck version of main-collection
         rc = _checkCLVersion( _pCollectionName, _clVersion ) ;
         PD_RC_CHECK( rc, PDDEBUG, "Failed to check message version [%d] of "
                      "collection [%s], rc: %d", _clVersion, _pCollectionName,
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_updateToMainCL( rtnQueryOptions &options,
                                          const BSONObj &updator,
                                          pmdEDUCB *cb,
                                          SDB_DMSCB *pDmsCB,
                                          SDB_DPSCB *pDpsCB,
                                          INT16 w,
                                          utilUpdateResult &upResult )
   {
      INT32 rc = SDB_OK;
      BSONObj boNewMatcher;
      const CHAR *pSubCLName = NULL ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iterSubCLSet ;

      rc = _getSubCLList( options.getQuery(), options.getCLFullName(), TRUE,
                          boNewMatcher, strSubCLList ) ;
      if ( rc != SDB_OK )
      {
         goto error;
      }

      /// update sub collections
      iterSubCLSet = strSubCLList.begin() ;
      while( iterSubCLSet != strSubCLList.end() )
      {
         pSubCLName = (*iterSubCLSet).c_str() ;
         BSONObj shardingKey ;

         // Construct query options for sub-collection
         rtnQueryOptions subCLOptions( options ) ;
         subCLOptions.setMainCLQuery( options.getCLFullName(), pSubCLName ) ;
         subCLOptions.setQuery( boNewMatcher ) ;

         rc = _getShardingKey( pSubCLName, shardingKey ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get sharding key of collection[%s], rc=%d",
                    options.getCLFullName(), rc ) ;
            goto error ;
         }

         rc = rtnUpdate( subCLOptions, updator, cb, pDmsCB, pDpsCB, w,
                         &upResult,
                         shardingKey.isEmpty() ? NULL : &shardingKey ) ;
         if ( rc )
         {
            rc = _processSubCLResult( rc, pSubCLName, options.getCLFullName() ) ;
            if ( SDB_OK == rc )
            {
               continue ;
            }
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Update on sub-collection[%s] of "
                    "main-collection[%s] failed, rc: %d",
                    sessionName(), pSubCLName, options.getCLFullName(), rc ) ;
            goto error ;
         }

         /// continue next sub collection
         ++iterSubCLSet ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_deleteToMainCL ( rtnQueryOptions &options,
                                           pmdEDUCB *cb,
                                           SDB_DMSCB *dmsCB,
                                           SDB_DPSCB *dpsCB,
                                           INT16 w,
                                           utilDeleteResult &delResult )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCLName = NULL ;
      BSONObj boNewMatcher ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iterSubCLSet ;

      rc = _getSubCLList( options.getQuery(), options.getCLFullName(), TRUE,
                          boNewMatcher, strSubCLList ) ;
      if ( rc != SDB_OK )
      {
         goto error;
      }

      iterSubCLSet = strSubCLList.begin() ;
      while( iterSubCLSet != strSubCLList.end() )
      {
         pSubCLName = (*iterSubCLSet).c_str() ;

         // Construct query options for sub-collection
         rtnQueryOptions subCLOptions( options ) ;
         subCLOptions.setMainCLQuery( options.getCLFullName(), pSubCLName ) ;
         subCLOptions.setQuery( boNewMatcher ) ;

         rc = rtnDelete( subCLOptions, cb, dmsCB, dpsCB, w, &delResult ) ;
         if ( rc )
         {
            rc = _processSubCLResult( rc, pSubCLName, options.getCLFullName() ) ;
            if ( SDB_OK == rc )
            {
               continue ;
            }
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Delete on sub-collection[%s] of "
                    "main-collection[%s] failed, rc: %d", sessionName(),
                    pSubCLName, options.getCLFullName(), rc ) ;
            goto error ;
         }

         /// continue next sub collection
         ++iterSubCLSet;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsShdSession::_runOnMainCL( const CHAR *pCommandName,
                                       _rtnCommand *pCommand,
                                       INT32 flags,
                                       INT64 numToSkip,
                                       INT64 numToReturn,
                                       const CHAR *pQuery,
                                       const CHAR *pField,
                                       const CHAR *pOrderBy,
                                       const CHAR *pHint,
                                       INT16 w,
                                       SINT64 &contextID,
                                       BSONObjBuilder *pBuilder )
   {
      INT32 rc = SDB_OK;
      BOOLEAN writable = FALSE ;
      SDB_ASSERT( pCommandName && pCommand, "pCommand can't be null!" );
      switch( pCommand->type() )
      {
      case CMD_GET_COUNT:
      case CMD_GET_INDEXES:
      case CMD_LIST_LOB:
      case CMD_GET_CL_DETAIL:
         rc = _getOnMainCL( pCommandName, pCommand->collectionFullName(),
                            flags, numToSkip, numToReturn, pQuery, pField,
                            pOrderBy, pHint, w, contextID );
         break;

      case CMD_CREATE_INDEX:
         writable = TRUE ;
         rc = _createIndexOnMainCL( pCommandName,
                                    pCommand->collectionFullName(),
                                    pQuery, pHint, w, contextID, FALSE,
                                    pBuilder );
         break;

      case CMD_ALTER_COLLECTION :
         writable = TRUE ;
         rc = _alterMainCL( pCommand, _pEDUCB, _pDpsCB, pBuilder ) ;
         break ;
      case CMD_DROP_INDEX:
         writable = TRUE ;
         rc = _dropIndexOnMainCL( pCommandName, pCommand->collectionFullName(),
                                  pQuery, w, contextID );
         break;
      case CMD_TEST_COLLECTION:
         rc = _testMainCollection( pCommand->collectionFullName() ) ;
         break ;
      case CMD_LINK_COLLECTION:
      case CMD_UNLINK_COLLECTION:
         rc = rtnRunCommand( pCommand, CMD_SPACE_SERVICE_SHARD,
                             _pEDUCB, _pDmsCB, _pRtnCB,
                             _pDpsCB, w, &contextID ) ;
         break;

      case CMD_DROP_COLLECTION:
         /// wait sync in context, not set writable
         rc = _dropMainCL( pCommand->collectionFullName(), w,
                           contextID );
         break;

      case CMD_RENAME_COLLECTION:
         /// wait sync in context, not set writable
         rc = _renameMainCL( pCommand->collectionFullName(), w, contextID );
         break;

      case CMD_TRUNCATE:
         writable = TRUE ;
         rc = _truncateMainCL( pCommand->collectionFullName() ) ;
         break ;

      case CMD_ANALYZE :
         writable = pCommand->writable() ;
         rc = _analyzeMainCL( pCommand ) ;
         break ;

      case CMD_SNAPSHOT_RESET :
         writable = pCommand->writable() ;
         rc = _resetSnapshotMainCL( pCommand ) ;
         break ;

      default:
         rc = SDB_MAIN_CL_OP_ERR;
         break;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "failed to run command on main-collection(rc=%d)",
                   rc );

      /// wait for sync
      if ( writable && w > 1 )
      {
         _pDpsCB->completeOpr( eduCB(), w ) ;
      }

   done:
      return rc;
   error:
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, eduCB() ) ;
         contextID = -1 ;
      }
      goto done;
   }

   INT32 _clsShdSession::_getOnMainCL( const CHAR *pCommand,
                                       const CHAR *pCollection,
                                       INT32 flags,
                                       INT64 numToSkip,
                                       INT64 numToReturn,
                                       const CHAR *pQuery,
                                       const CHAR *pField,
                                       const CHAR *pOrderBy,
                                       const CHAR *pHint,
                                       INT16 w,
                                       SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCLName = NULL ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iterSubCLSet ;
      BSONObj boNewMatcher ;
      rtnContextMainCL *pContextMainCL = NULL ;
      BSONObj boMatcher ;
      BSONObj orderBy ;
      BSONObj boEmpty ;
      BSONObj boHint ;
      _rtnCommand *pCommandTmp = NULL;
      INT64 subNumToReturn = numToReturn ;
      INT64 subNumToSkip = 0 ;
      SDB_ASSERT( pCommand, "pCommand can't be null!" );
      SDB_ASSERT( pCollection,
                  "collection name can't be null!"  );

      try
      {
         boMatcher = BSONObj( pQuery );
         orderBy = BSONObj( pOrderBy ) ;
         BSONObj boHintTmp = BSONObj( pHint );
         BSONObjBuilder bobHint;
         BSONObjIterator iter( boHintTmp );
         while( iter.more() )
         {
            BSONElement beTmp = iter.next();
            if ( 0 != ossStrcmp( beTmp.fieldName(), FIELD_NAME_COLLECTION ))
            {
               bobHint.append( beTmp );
            }
         }
         boHint = bobHint.obj();
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s] Failed to create matcher: %s",
                  sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _getSubCLList( boMatcher, pCollection, FALSE, boNewMatcher,
                          strSubCLList );
      PD_RC_CHECK( rc, PDERROR, "failed to get sub-collection list(rc=%d)",
                   rc );

      /// reset num to skip and num to return
      if ( strSubCLList.size() <= 1 )
      {
         subNumToSkip = numToSkip ;
         numToSkip = 0 ;
      }
      else
      {
         if ( numToSkip > 0 && numToReturn > 0 )
         {
            subNumToReturn = numToSkip + numToReturn ;
         }
      }

      rc = _pRtnCB->contextNew( RTN_CONTEXT_MAINCL,
                                (rtnContext **)&pContextMainCL,
                                contextID, _pEDUCB );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to create new main-collection context(rc=%d)",
                  rc );

      if ( OSS_BIT_TEST(flags, FLG_QUERY_PREPARE_MORE ) &&
           !OSS_BIT_TEST(flags, FLG_QUERY_MODIFY ) )
      {
         pContextMainCL->setPrepareMoreData( TRUE ) ;
      }

      rc = pContextMainCL->open( orderBy, numToReturn, numToSkip ) ;
      PD_RC_CHECK( rc, PDERROR, "open main-collection context failed(rc=%d)",
                   rc );

      iterSubCLSet = strSubCLList.begin() ;
      while( iterSubCLSet != strSubCLList.end() )
      {
         pSubCLName = (*iterSubCLSet).c_str() ;

         SINT64 subContextID = -1 ;
         BSONObj boSubHint ;
         try
         {
            BSONObjBuilder bobSubHint;
            bobSubHint.appendElements( boHint );
            bobSubHint.append( FIELD_NAME_COLLECTION, *iterSubCLSet ) ;
            boSubHint = bobSubHint.obj();
         }
         catch( std::exception &e )
         {
            PD_LOG ( PDERROR, "Session[%s] Failed to create hint: %s",
                     sessionName(), e.what () ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         do
         {
            rc = rtnParserCommand( pCommand, &pCommandTmp );
            if ( rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Parse command[%s] failed, "
                       "rc: %d", sessionName(), pCommand, rc ) ;
               break ;
            }

            rc = rtnInitCommand( pCommandTmp, flags, subNumToSkip,
                                 subNumToReturn, boNewMatcher.objdata(),
                                 pField, pOrderBy,
                                 boSubHint.objdata() ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to init command[%s], "
                       "rc: %d", sessionName(), pCommand, rc ) ;
               break ;
            }

            pCommandTmp->setMainCLName( pCollection ) ;

            rc = rtnRunCommand( pCommandTmp, CMD_SPACE_SERVICE_SHARD, _pEDUCB,
                                _pDmsCB, _pRtnCB, _pDpsCB, w, &subContextID );
            if ( rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to run command[%s] on "
                       "sub-collection[%s], rc: %d", sessionName(), pCommand,
                       pSubCLName, rc ) ;
               break ;
            }
         } while( FALSE ) ;

         if ( pCommandTmp )
         {
            rtnReleaseCommand( &pCommandTmp ) ;
            pCommandTmp = NULL;
         }

         if ( rc )
         {
            rc = _processSubCLResult( rc, pSubCLName, pCollection ) ;
            if ( SDB_OK == rc )
            {
               continue ;
            }
            goto error ;
         }

         pContextMainCL->addSubContext( subContextID ) ;
         ++iterSubCLSet ;
      }

   done:
      if ( pCommandTmp )
      {
         rtnReleaseCommand( &pCommandTmp );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _clsShdSession::_createIndexOnMainCL( const CHAR *pCommand,
                                               const CHAR *pCollection,
                                               const CHAR *pQuery,
                                               const CHAR *pHint,
                                               INT16 w,
                                               SINT64 &contextID,
                                               BOOLEAN syscall,
                                               BSONObjBuilder *pBuilder )
   {
      INT32 rc = SDB_OK;
      const CHAR *pSubCLName = NULL ;
      BSONObj boMatcher ;
      BSONObj boNewMatcher ;
      BSONObj boIndex ;
      BSONObj boHint ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iter ;
      INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ;
      BOOLEAN lockDms = FALSE ;
      utilWriteResult wrResult ;

      try
      {
         boMatcher = BSONObj( pQuery ) ;
         boHint = BSONObj( pHint ) ;

         rc = rtnGetObjElement( boMatcher, FIELD_NAME_INDEX, boIndex ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get object index, rc: %d", rc ) ;

         rc = rtnConvertIndexDef( boIndex ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to convert index definition" ) ;

         if ( boMatcher.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
         {
            rc = rtnGetIntElement( boMatcher, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                   sortBufferSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get index sort buffer, matcher: %s",
                        boMatcher.toString().c_str() ) ;
               goto error ;
            }
         }
         else if ( boHint.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
         {
            rc = rtnGetIntElement( boHint, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                   sortBufferSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get index sort buffer, hint: %s",
                        boHint.toString().c_str() ) ;
               goto error ;
            }
         }
         if ( sortBufferSize < 0 )
         {
            PD_LOG ( PDERROR, "invalid index sort buffer size: %d",
                     sortBufferSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "occur unexpected error(%s)",
                      e.what() );
      }

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = pmdGetKRCB()->getDMSCB()->writable ( _pEDUCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      lockDms = TRUE ;

      rc = _getSubCLList( boMatcher, pCollection, TRUE, boNewMatcher,
                          strSubCLList );
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list, rc: %d",
                   rc ) ;

      iter = strSubCLList.begin() ;
      while( iter != strSubCLList.end() )
      {
         INT32 rcTmp = SDB_OK ;
         pSubCLName = iter->c_str() ;

         wrResult.resetInfo() ;
         rcTmp = rtnCreateIndexCommand( pSubCLName, boIndex, _pEDUCB,
                                        _pDmsCB, _pDpsCB, syscall,
                                        sortBufferSize,
                                        &wrResult ) ;
         if ( rcTmp )
         {
            rcTmp = _processSubCLResult( rcTmp, pSubCLName,
                                         pCollection ) ;
            if ( SDB_OK == rcTmp )
            {
               continue ;
            }
         }

         if ( rcTmp && SDB_OK != rcTmp && SDB_IXM_REDEF != rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s]: Create index[%s] for "
                    "sub-collection[%s] of main-collection[%s] failed, "
                    "rc: %d", sessionName(), boIndex.toString().c_str(),
                    pSubCLName, _pCollectionName, rcTmp ) ;

            if ( SDB_OK == rc )
            {
               if ( pBuilder )
               {
                  wrResult.toBSON( *pBuilder ) ;
               }
               rc = rcTmp ;
            }
         }
         ++iter ;
      }

      // Clear cached main-collection plans
      // Note: cached sub-collection plans are cleared inside create-index
      // of sub-collections
      _pRtnCB->getAPM()->invalidateCLPlans( _pCollectionName ) ;

      // Tell secondary nodes to clear cached main-collection plans
      sdbGetClsCB()->invalidatePlan( _pCollectionName ) ;

   done:
      if ( lockDms )
      {
         _pDmsCB->writeDown( _pEDUCB ) ;
         lockDms = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_dropIndexOnMainCL( const CHAR *pCommand,
                                             const CHAR *pCollection,
                                             const CHAR *pQuery,
                                             INT16 w,
                                             SINT64 &contextID,
                                             BOOLEAN syscall )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCLName = NULL ;
      BSONObj boMatcher ;
      BSONObj boNewMatcher ;
      BSONObj boIndex ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iter ;
      BSONElement ele;
      BOOLEAN isExist = FALSE ;
      BOOLEAN lockDms = FALSE ;

      try
      {
         boMatcher = BSONObj( pQuery );
         rc = rtnGetObjElement( boMatcher, FIELD_NAME_INDEX,
                                boIndex );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get object index(rc=%d)", rc );
         ele = boIndex.firstElement() ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "occur unexpected error(%s)",
                      e.what() );
      }

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = _pDmsCB->writable ( _pEDUCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      lockDms = TRUE ;

      rc = _getSubCLList( boMatcher, pCollection, TRUE, boNewMatcher,
                          strSubCLList );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get sub-collection list, rc: %d", rc ) ;

      iter = strSubCLList.begin();
      while( iter != strSubCLList.end() )
      {
         INT32 rcTmp = SDB_OK ;
         pSubCLName = iter->c_str() ;

         rcTmp = rtnDropIndexCommand( pSubCLName, ele, _pEDUCB,
                                      _pDmsCB, _pDpsCB, syscall ) ;
         if ( rcTmp )
         {
            rcTmp = _processSubCLResult( rcTmp, pSubCLName, pCollection ) ;
            if ( SDB_OK == rcTmp )
            {
               continue ;
            }
         }

         if ( SDB_OK == rcTmp )
         {
            isExist = TRUE ;
         }
         else
         {
            if ( SDB_OK == rc || SDB_IXM_NOTEXIST == rc )
            {
               rc = rcTmp ;
            }
            PD_LOG( PDERROR, "Session[%s]: Drop index[%s] for "
                    "sub-collection[%s] of main-collection[%s] "
                    "failed, rc: %d", sessionName(), ele.toString().c_str(),
                    pSubCLName, _pCollectionName, rcTmp ) ;
         }
         ++iter ;
      }

      if ( SDB_IXM_NOTEXIST == rc && isExist )
      {
         rc = SDB_OK ;
      }

      // Clear cached main-collection plans
      // Note: cached sub-collection plans are cleared inside create-index
      // of sub-collections
      _pRtnCB->getAPM()->invalidateCLPlans( _pCollectionName ) ;

      // Tell secondary nodes to clear cached main-collection plans
      sdbGetClsCB()->invalidatePlan( _pCollectionName ) ;

   done:
      if ( lockDms )
      {
         _pDmsCB->writeDown( _pEDUCB ) ;
         lockDms = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_dropMainCL( const CHAR *pCollection,
                                      INT16 w,
                                      SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      CLS_SUBCL_LIST subCLLst ;
      contextID = -1 ;
      rtnContextDelMainCL *delContext = NULL ;

      rc = _getSubCLList( pCollection, TRUE, subCLLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Failed to get sub collection "
                   "list, rc: %d", sessionName(), rc ) ;

      rc = _pRtnCB->contextNew( RTN_CONTEXT_DELMAINCL,
                                (rtnContext **)&delContext,
                                contextID, _pEDUCB );
      PD_RC_CHECK( rc, PDERROR, "Failed to create context, drop "
                   "main collection[%s] failed, rc: %d", pCollection,
                   rc ) ;
      rc = delContext->open( pCollection, subCLLst, _pEDUCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, drop "
                   "main collection[%s] failed, rc: %d", pCollection,
                   rc ) ;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsShdSession::_renameMainCL( const CHAR *pCollection,
                                        INT16 w,
                                        SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      contextID = -1 ;
      rtnContextRenameMainCL *renameContext = NULL ;

      rc = _pRtnCB->contextNew( RTN_CONTEXT_RENAMEMAINCL,
                                (rtnContext **)&renameContext,
                                contextID, _pEDUCB );
      PD_RC_CHECK( rc, PDERROR, "Failed to create context, rename "
                   "main collection[%s] failed, rc: %d", pCollection,
                   rc ) ;

      rc = renameContext->open( pCollection, _pEDUCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, drop "
                   "main collection[%s] failed, rc: %d", pCollection,
                   rc ) ;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsShdSession::_onCatalogChangeNtyMsg( MsgHeader * msg )
   {
      _pShdMgr->updateCatGroup() ;
      return SDB_OK ;
   }

   INT32 _clsShdSession::_onOpenLobReq( MsgHeader *msg,
                                        SINT64 &contextID,
                                        rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      BSONObj lob ;
      BSONElement fullName ;
      BSONElement mode ;
      INT16 w = 0 ;
      INT16 replSize = 0 ;
      const CHAR *pData = NULL ;
      UINT32 dataLen = 0 ;
      _rtnContextShdOfLob *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = msgExtractOpenLobRequest( ( const CHAR * )msg, &header, lob ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract open msg:%d", rc ) ;
         goto error ;
      }
      fullName = lob.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != fullName.type() )
      {
         PD_LOG( PDERROR, "invalid lob obj:%s",
                 lob.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _pCollectionName = fullName.valuestr() ;

      mode = lob.getField( FIELD_NAME_LOB_OPEN_MODE ) ;
      if ( NumberInt != mode.type() )
      {
         PD_LOG( PDERROR, "invalid lob obj:%s",
                 lob.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "Option:%s", lob.toPoolString().c_str() ) ;

      if ( !SDB_IS_LOBREADONLY_MODE( mode.Int() ) )
      {
         rc = _checkWriteStatus() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _checkPrimaryWhenRead( FLG_LOBREAD_PRIMARY, header->flags ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "failed to check read status:%d", rc ) ;
            goto error ;
         }

         if ( !OSS_BIT_TEST( header->flags, FLG_LOBREAD_PRIMARY ) )
         {
            rc = _checkSecondaryWhenRead( FLG_LOBREAD_SECONDARY,
                                          header->flags ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to check read secondary status, "
                       "rc: %d", rc ) ;
               goto error ;
            }
         }
      }

      rc = _checkCLStatusAndGetSth( fullName.valuestr(),
                                    header->version,
                                    &_isMainCL,
                                    &replSize ) ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( !SDB_IS_LOBREADONLY_MODE( mode.Int() ) )
      {
         rc = _calculateW( &replSize, &( header->w ), w ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         w = 1 ;
      }

      rc = rtnCB->contextNew( RTN_CONTEXT_SHARD_OF_LOB,
                              (rtnContext**)(&context),
                              contextID, _pEDUCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open context:%d", rc ) ;
         goto error ;
      }

      rc = context->open( lob, header->flags, header->version, w,
                          _pDpsCB, _pEDUCB, &pData, dataLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob context:%d", rc ) ;
         goto error ;
      }

      /// if sequence 0 is not on this node, we have nothing to send back.
      if ( pData && dataLen > 0 )
      {
         buffObj = rtnContextBuf( pData, dataLen, 1 ) ;
      }
   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, _pEDUCB ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _clsShdSession::_onWriteLobReq( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      const MsgLobTuple *tuple = NULL ;
      UINT32 tSize = 0 ;
      const MsgLobTuple *curTuple = NULL ;
      UINT32 tupleNum = 0 ;
      const CHAR *data = NULL ;
      rtnContext *context = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      INT16 w = 0 ;
      INT16 wWhenOpen = 0 ;
      BOOLEAN orUpdate = FALSE ;

      rc = msgExtractLobRequest( ( const CHAR * )msg,
                                 &header, obj,
                                 &tuple, &tSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract write msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist", header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid type of context:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextShdOfLob * )context ;
      _pCollectionName = lobContext->getFullName() ;
      wWhenOpen = lobContext->getW() ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, CollectionName:%s, TupleSize:%u",
                          header->contextID, _pCollectionName, tSize ) ;

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( lobContext->getFullName(),
                                    header->version,
                                    &_isMainCL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _calculateW( &wWhenOpen, NULL, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
         goto error ;
      }

      if ( header->flags & FLG_LOBWRITE_OR_UPDATE )
      {
         orUpdate = TRUE ;
      }

      while ( TRUE )
      {
         BOOLEAN got = FALSE ;
         rc = msgExtractTuplesAndData( &tuple, &tSize,
                                       &curTuple, &data,
                                       &got ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract next tuple:%d", rc ) ;
            goto error ;
         }

         if ( !got )
         {
            break ;
         }

         rc = lobContext->write( curTuple->columns.sequence,
                                 curTuple->columns.offset,
                                 curTuple->columns.len,
                                 data, _pEDUCB, orUpdate ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
            goto error ;
         }

         ++tupleNum ;
         if ( 0 == tupleNum % SHD_INTERRUPT_CHECKPOINT &&
              _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      PD_LOG( PDDEBUG, "%d pieces of lob[%s] write done",
              tupleNum, lobContext->getOID().str().c_str() ) ;
   done:
      return rc ;
   error:
      if ( NULL != context &&
           SDB_CLS_COORD_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_DATA_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_NO_CATALOG_INFO != rc )
      {
         // Do not re-create
         _pCollectionName = NULL ;
         // do not delete main shard context
         if ( NULL == lobContext || !lobContext->isMainShard() )
         {
            rtnCB->contextDelete( context->contextID(), _pEDUCB ) ;
         }
      }
      goto done ;
   }

   INT32 _clsShdSession::_onLockLobReq( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      INT64 offset = 0 ;
      INT64 length = -1 ;

      rc = msgExtractLockLobRequest( ( const CHAR * )msg, &header, &offset, &length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract lock msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist",
                  header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid context type:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextShdOfLob * )context ;
      _pCollectionName = lobContext->getFullName() ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s",
                          header->contextID, _pCollectionName ) ;

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( lobContext->getFullName(),
                                    header->version,
                                    &_isMainCL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      /// do not check version coz we will not
      ///  change any thing except close the context.
      lobContext = ( rtnContextShdOfLob * )context ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s",
                          header->contextID,
                          lobContext->getFullName() ) ;

      rc = lobContext->lock( _pEDUCB, offset, length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to lock lob:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( NULL != context &&
           SDB_CLS_COORD_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_DATA_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_NO_CATALOG_INFO != rc )
      {
         // Do not re-create
         _pCollectionName = NULL ;
         // do not delete main shard context
         if ( NULL == lobContext || !lobContext->isMainShard() )
         {
            rtnCB->contextDelete( context->contextID(), _pEDUCB ) ;
         }
      }
      goto done ;
   }

   INT32 _clsShdSession::_onCloseLobReq( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = msgExtractCloseLobRequest( ( const CHAR * )msg, &header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract close msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist",
                  header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto done ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid context type:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// do not check version coz we will not
      ///  change any thing except close the context.
      lobContext = ( rtnContextShdOfLob * )context ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s",
                          header->contextID,
                          lobContext->getFullName() ) ;

      rc = lobContext->close( _pEDUCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != context )
      {
         rtnCB->contextDelete ( context->contextID(), _pEDUCB ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_onReadLobReq( MsgHeader *msg,
                                        rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      const MsgLobTuple *tuple = NULL ;
      UINT32 tuplesSize = 0 ;
      bson::BSONObj meta ;
      const CHAR *data = NULL ;
      UINT32 read = 0 ;

      rc = msgExtractLobRequest( ( const CHAR * )msg,
                                 &header, meta, &tuple, &tuplesSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract read msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist",
                  header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid context type:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextShdOfLob * )context ;
      _pCollectionName = lobContext->getFullName() ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s, TupleSize:%u",
                          header->contextID, _pCollectionName, tuplesSize ) ;

      rc = _checkPrimaryWhenRead(FLG_LOBREAD_PRIMARY,  header->flags ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check read status:%d", rc ) ;
         goto error ;
      }

      if ( !OSS_BIT_TEST( header->flags, FLG_LOBREAD_PRIMARY ) )
      {
         rc = _checkSecondaryWhenRead( FLG_LOBREAD_SECONDARY, header->flags ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to check read secondary status, "
                    "rc: %d", rc ) ;
            goto error ;
         }
      }

      /// When split, use writingCB to prevent reading lob conflicted
      /// with clean job
      eduCB()->writingDB( TRUE ) ;

      /// check catalog version
      rc = _checkCLStatusAndGetSth( lobContext->getFullName(),
                                    header->version,
                                    &_isMainCL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = lobContext->readv( tuple, tuplesSize / sizeof( MsgLobTuple ),
                              _pEDUCB, &data, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob:%d", rc ) ;
         goto error ;
      }

      buffObj = rtnContextBuf( data, read, 0 ) ;
   done:
      return rc ;
   error:
      if ( NULL != context &&
           SDB_CLS_COORD_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_DATA_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_NO_CATALOG_INFO != rc )
      {
         // Do not re-create
         _pCollectionName = NULL ;
         // do not delete main shard context
         if ( NULL == lobContext || !lobContext->isMainShard() )
         {
            rtnCB->contextDelete ( context->contextID(), _pEDUCB ) ;
         }
      }
      goto done ;
   }

   INT32 _clsShdSession::_onRemoveLobReq( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      const MsgLobTuple *begin = NULL ;
      UINT32 tuplesSize = 0 ;
      BSONObj obj ;
      INT16 w = 0 ;
      INT16 wWhenOpen = 0 ;
      UINT32 tupleNum = 0 ;

      rc = msgExtractLobRequest( ( const CHAR * )msg, &header,
                                 obj, &begin, &tuplesSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract close msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist",
                  header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid context type:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextShdOfLob * )context ;
      _pCollectionName = lobContext->getFullName() ;
      wWhenOpen = lobContext->getW() ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s, TupleSize:%u",
                          header->contextID, _pCollectionName,
                          tuplesSize ) ;

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( lobContext->getFullName(),
                                    header->version,
                                    &_isMainCL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _calculateW( &wWhenOpen, NULL, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         BOOLEAN got = FALSE ;
         const MsgLobTuple *curTuple = NULL ;
         rc = msgExtractTuples( &begin, &tuplesSize,
                                &curTuple, &got ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract next tuple:%d", rc ) ;
            goto error ;
         }

         if ( !got )
         {
            break ;
         }

         rc = lobContext->remove( curTuple->columns.sequence,
                                  _pEDUCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
            goto error ;
         }

         if ( 0 == ++tupleNum % SHD_INTERRUPT_CHECKPOINT &&
              _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      PD_LOG( PDDEBUG, "%d pieces of lob[%s] remove done",
              tupleNum, lobContext->getOID().str().c_str() ) ;
   done:
      return rc ;
   error:
      if ( NULL != context &&
           SDB_CLS_COORD_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_DATA_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_NO_CATALOG_INFO != rc )
      {
         // Do not re-create
         _pCollectionName = NULL ;
         // do not delete main shard context
         if ( NULL == lobContext || !lobContext->isMainShard() )
         {
            rtnCB->contextDelete ( context->contextID(), _pEDUCB ) ;
         }
      }
      goto done ;
   }

   INT32 _clsShdSession::_onUpdateLobReq( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      const MsgLobTuple *tuple = NULL ;
      UINT32 tSize = 0 ;
      const MsgLobTuple *curTuple = NULL ;
      UINT32 tupleNum = 0 ;
      const CHAR *data = NULL ;
      rtnContext *context = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      INT16 w = 0 ;
      INT16 wWhenOpen = 0 ;

      rc = msgExtractLobRequest( ( const CHAR * )msg,
                                 &header, obj,
                                 &tuple, &tSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract write msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist", header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid type of context:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextShdOfLob * )context ;
      _pCollectionName = lobContext->getFullName() ;
      wWhenOpen = lobContext->getW() ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s, TupleSize:%u",
                          header->contextID, _pCollectionName, tSize ) ;

      rc = _checkWriteStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check write status:%d", rc ) ;
         goto error ;
      }

      rc = _checkCLStatusAndGetSth( lobContext->getFullName(),
                                    header->version,
                                    &_isMainCL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _calculateW( &wWhenOpen, NULL, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to calculate w:%d", rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         BOOLEAN got = FALSE ;
         rc = msgExtractTuplesAndData( &tuple, &tSize,
                                       &curTuple, &data,
                                       &got ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract next tuple:%d", rc ) ;
            goto error ;
         }

         if ( !got )
         {
            break ;
         }

         rc = lobContext->update( curTuple->columns.sequence,
                                  curTuple->columns.offset,
                                  curTuple->columns.len,
                                  data, _pEDUCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
            goto error ;
         }

         if ( 0 == ++tupleNum % SHD_INTERRUPT_CHECKPOINT &&
              _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      PD_LOG( PDDEBUG, "%d pieces of lob[%s] update done",
              tupleNum, lobContext->getOID().str().c_str() ) ;
   done:
      return rc ;
   error:
      if ( NULL != context &&
           SDB_CLS_COORD_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_DATA_NODE_CAT_VER_OLD != rc &&
           SDB_CLS_NO_CATALOG_INFO != rc )
      {
         // Do not re-create
         _pCollectionName = NULL ;
         // do not delete main shard context
         if ( NULL == lobContext || !lobContext->isMainShard() )
         {
            rtnCB->contextDelete( context->contextID(), _pEDUCB ) ;
         }
      }
      goto done ;
   }

   /// get lob's runtime detail
   INT32 _clsShdSession::_onGetLobRTDetailReq( MsgHeader *msg,
                                               rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rtnContextShdOfLob *lobContext = NULL ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      BSONObj detail ;

      rc = msgExtractGetLobRTDetailRequest( ( const CHAR * )msg, &header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract read msg:%d", rc ) ;
         goto error ;
      }

      context = rtnCB->contextFind ( header->contextID, eduCB() ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "context %lld does not exist",
                  header->contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_SHARD_OF_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "invalid context type:%d", context->getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextShdOfLob * )context ;
      _pCollectionName = lobContext->getFullName() ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Collection:%s",
                          header->contextID, _pCollectionName ) ;

      rc = _checkPrimaryWhenRead(FLG_LOBREAD_PRIMARY,  header->flags ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "failed to check read status:%d", rc ) ;
         goto error ;
      }

      if ( !OSS_BIT_TEST( header->flags, FLG_LOBREAD_PRIMARY ) )
      {
         rc = _checkSecondaryWhenRead( FLG_LOBREAD_SECONDARY, header->flags ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to check read secondary status, "
                    "rc: %d", rc ) ;
            goto error ;
         }
      }

      /// check catalog version
      rc = _checkCLStatusAndGetSth( lobContext->getFullName(),
                                    header->version,
                                    &_isMainCL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = lobContext->getLobRTDetail( _pEDUCB, detail ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob:%d", rc ) ;
         goto error ;
      }

      buffObj = rtnContextBuf( detail ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_truncateMainCL( const CHAR *fullName )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCLName = NULL ;
      CLS_SUBCL_LIST subCLs ;
      CLS_SUBCL_LIST_IT itr ;
      BOOLEAN lockDms = FALSE ;

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = _pDmsCB->writable ( _pEDUCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      lockDms = TRUE ;

      rc = _getSubCLList( fullName, TRUE, subCLs ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Get sub collection list "
                   "failed, rc: %d", sessionName(), rc ) ;

      itr = subCLs.begin() ;
      while ( itr != subCLs.end() )
      {
         pSubCLName = itr->c_str() ;

         rc = rtnTruncCollectionCommand( pSubCLName, _pEDUCB,
                                         _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            rc = _processSubCLResult( rc, pSubCLName, fullName ) ;
            if ( SDB_OK == rc )
            {
               continue ;
            }
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to truncate sub-"
                    "collection[%s] fo main-collection[%s] failed, rc: %d",
                    sessionName(), pSubCLName, fullName, rc ) ;
            goto error ;
         }
         ++itr ;
      }

      // Clear cached main-collection plans
      // Note: cached sub-collection plans are cleared inside truncate
      // of sub-collections
      _pRtnCB->getAPM()->invalidateCLPlans( _pCollectionName ) ;

      // Tell secondary nodes to clear cached main-collection plans
      sdbGetClsCB()->invalidatePlan( _pCollectionName ) ;

   done:
      if ( lockDms )
      {
         _pDmsCB->writeDown( _pEDUCB ) ;
         lockDms = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_onPacketMsg( NET_HANDLE handle,
                                       MsgHeader *msg,
                                       INT64 &contextID,
                                       rtnContextBuf &buf,
                                       INT32 &startFrom,
                                       INT32 &opCode )
   {
      INT32 rc = SDB_OK ;
      INT32 pos = 0 ;
      MsgHeader *pTmpMsg = NULL ;

      ++_inPacketLevel ;

      pos += sizeof( MsgHeader ) ;
      while( pos < msg->messageLength )
      {
         pTmpMsg = ( MsgHeader* )( ( CHAR*)msg + pos ) ;
         opCode = pTmpMsg->opCode ;

         rc = _onOPMsg( handle, pTmpMsg ) ;
         if ( rc )
         {
            goto error ;
         }
         pos += pTmpMsg->messageLength ;
      }

   done:
      --_inPacketLevel ;
      if ( 0 == _inPacketLevel )
      {
         contextID = _pendingContextID ;
         _pendingContextID = -1 ;
         buf = _pendingBuff ;
         _pendingBuff = rtnContextBuf() ;
         startFrom = _pendingStartFrom ;
         _pendingStartFrom = 0 ;
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_testMainCollection( const CHAR *fullName )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCLName = NULL ;
      CLS_SUBCL_LIST subCLs ;
      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;
      rc = _getSubCLList( fullName, FALSE, subCLs ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Get sub collection list "
                   "failed, rc: %d", sessionName(), rc ) ;
      for ( CLS_SUBCL_LIST_IT itr =  subCLs.begin();
            itr != subCLs.end();
            ++itr )
      {
         pSubCLName = itr->c_str() ;
         rc = rtnTestCollectionCommand( pSubCLName, dmsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to test sub collection:%s, rc:%d",
                    pSubCLName, rc ) ;
            goto error ;
         }

      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_alterMainCL( _rtnCommand *command,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB,
                                       BSONObjBuilder *pBuilder )
   {
      INT32 rc = SDB_OK ;
      utilWriteResult wrResult ;
      CLS_SUBCL_LIST subCLs ;
      const _rtnAlterCollection *alterCommand = ( const _rtnAlterCollection * )command ;

      const rtnAlterJob * alterJob = alterCommand->getAlterJob() ;
      SDB_ASSERT( NULL != alterJob, "alterJob is invalid" ) ;

      const RTN_ALTER_TASK_LIST & alterTasks = alterJob->getAlterTasks() ;
      const rtnAlterOptions * options = alterJob->getOptions() ;
      const CHAR * collectionName = alterJob->getObjectName() ;

      BSONObj matcher = alterJob->getJobObject() ;
      BSONObj newMatcher ;
      CLS_SUBCL_LIST subCLList ;

      BOOLEAN lockDms = FALSE ;

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = _pDmsCB->writable ( _pEDUCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      lockDms = TRUE ;

      // Get sub-collection list
      rc = _getSubCLList( matcher, collectionName, TRUE,
                          newMatcher, subCLList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list of "
                   "collection [%s], rc: %d", collectionName, rc ) ;

      for ( RTN_ALTER_TASK_LIST::const_iterator iterTask = alterTasks.begin() ;
            iterTask != alterTasks.end() ;
            ++ iterTask )
      {
         const rtnAlterTask * task = ( *iterTask ) ;
         CLS_SUBCL_LIST_IT iterCL ;

         if ( !task->testFlags( RTN_ALTER_TASK_FLAG_MAINCLALLOW ) )
         {
            PD_LOG( PDINFO, "Failed to execute alter task [%s]: not supported "
                    "in main-collection", task->getActionName() ) ;
         }

         iterCL = subCLList.begin() ;
         while ( iterCL != subCLList.end() )
         {
            INT32 rcTmp = SDB_OK ;
            const CHAR * subCLName = iterCL->c_str() ;

            wrResult.resetInfo() ;
            rcTmp = rtnAlter( subCLName, task, options, cb, dpsCB, &wrResult ) ;
            if ( rcTmp )
            {
               rcTmp = _processSubCLResult( rcTmp, subCLName, collectionName ) ;
               if ( SDB_OK == rcTmp )
               {
                  continue ;
               }
            }

            if ( SDB_OK != rcTmp )
            {
               PD_LOG( PDERROR, "Session[%s]: Alter sub-collection [%s] of "
                       "main-collection[%s] failed, rc: %d", sessionName(),
                       subCLName, collectionName, rcTmp ) ;

               if ( SDB_OK == rc )
               {
                  if ( pBuilder )
                  {
                     wrResult.toBSON( *pBuilder ) ;
                  }
                  rc = rcTmp ;
               }
            }

            ++ iterCL ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to run alter task [%s], rc: %d",
                    task->getActionName(), rc ) ;
            if ( options->isIgnoreException() )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               goto error ;
            }
         }
      }

      // Clear cached main-collection plans
      // Note: cached sub-collection plans are cleared inside create-index
      // of sub-collections
      _pRtnCB->getAPM()->invalidateCLPlans( _pCollectionName ) ;

      // Tell secondary nodes to clear cached main-collection plans
      sdbGetClsCB()->invalidatePlan( _pCollectionName ) ;

   done:
      if ( lockDms )
      {
         _pDmsCB->writeDown( _pEDUCB ) ;
         lockDms = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_analyzeMainCL ( _rtnCommand *command )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN lockDms = FALSE ;

      SDB_ASSERT( command->type() == CMD_ANALYZE, "command is invalid" ) ;

      const CHAR *pMainCLName = command->collectionFullName() ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iterSubCL ;
      BOOLEAN foundIndex = FALSE ;

      _rtnAnalyze *pAnalyzeCmd = (_rtnAnalyze *)command ;

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = _pDmsCB->writable ( _pEDUCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      lockDms = TRUE ;

      rc = _getSubCLList( pMainCLName, TRUE, strSubCLList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list of "
                   "main-collection [%s], rc: %d", pMainCLName, rc ) ;

      iterSubCL = strSubCLList.begin() ;
      while( iterSubCL != strSubCLList.end() )
      {
         const CHAR *pSubCLName = iterSubCL->c_str() ;

         if ( _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = rtnAnalyze( NULL, pSubCLName, pAnalyzeCmd->getIndexName(),
                          pAnalyzeCmd->getAnalyzeParam(),
                          _pEDUCB, _pDmsCB, _pRtnCB, _pDpsCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_CS_NOTEXIST == rc ||
                 SDB_DMS_NOTEXIST == rc )
            {
               // The error should be found earlier in clsShardSesssion
               // If report here, means the collection or collection space had
               // been dropped, ignore the error to avoid clsShardSession to
               // retry
               rc = SDB_OK ;
            }
            else if ( NULL != pAnalyzeCmd->getIndexName() &&
                      SDB_IXM_NOTEXIST == rc )
            {
               // The index doesn't exist in this sub-collection, ignore it
               // and continue with the next sub-collection
               rc = SDB_OK ;
               ++ iterSubCL ;
               continue ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to analyze sub-collection [%s], rc: %d",
                       pSubCLName, rc ) ;
            }
            break ;
         }
         else if ( NULL != pAnalyzeCmd->getIndexName() )
         {
            foundIndex = TRUE ;
         }

         ++ iterSubCL ;
      }

      if ( !strSubCLList.empty() && NULL != pAnalyzeCmd->getIndexName() &&
           !foundIndex )
      {
         rc = SDB_IXM_NOTEXIST ;
         PD_LOG( PDERROR, "Failed to find index [%s] in main-collection [%s]",
                 pAnalyzeCmd->getIndexName(), pMainCLName ) ;
         goto error ;
      }

      _pRtnCB->getAPM()->invalidateCLPlans( pMainCLName ) ;

      // Tell secondary nodes to clear cached main-collection plans
      sdbGetClsCB()->invalidatePlan( pMainCLName ) ;

   done :
      if ( lockDms )
      {
         _pDmsCB->writeDown( _pEDUCB ) ;
         lockDms = FALSE ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsShdSession::_resetSnapshotMainCL ( _rtnCommand * command )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( command->type() == CMD_SNAPSHOT_RESET,
                  "command is invalid" ) ;

      const CHAR * mainCLName = command->collectionFullName() ;
      CLS_SUBCL_LIST strSubCLList ;
      CLS_SUBCL_LIST_IT iterSubCL ;

      rc = _getSubCLList( mainCLName, FALSE, strSubCLList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list of "
                   "main-collection [%s], rc: %d", mainCLName, rc ) ;

      iterSubCL = strSubCLList.begin() ;
      while( iterSubCL != strSubCLList.end() )
      {
         const CHAR * subCLName = iterSubCL->c_str() ;

         if ( _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = monResetMon( CMD_SNAPSHOT_COLLECTIONS, FALSE, _sessionID,
                           NULL, subCLName ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_CS_NOTEXIST == rc ||
                 SDB_DMS_NOTEXIST == rc )
            {
               // The error should be found earlier in clsShardSesssion
               // If report here, means the collection or collection space had
               // been dropped, ignore the error to avoid clsShardSession to
               // retry
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to reset snapshot for sub-collection "
                       "[%s], rc: %d", subCLName, rc ) ;
            }
            break ;
         }

         ++ iterSubCL ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CKPRIMARYSTATUS, "_clsShdSession::_checkPrimary" )
   INT32 _clsShdSession::_checkPrimaryStatus()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CKPRIMARYSTATUS ) ;
      UINT32 waitTime = 0 ;
      BOOLEAN hasBlock = FALSE ;

      while( TRUE )
      {
         rc = _pReplSet->primaryCheck( _pEDUCB ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }
         else if ( SDB_CLS_NOT_PRIMARY != rc )
         {
            goto error ;
         }
         else if ( MSG_INVALID_ROUTEID !=
                  ( _primaryID.value = _pReplSet->getPrimary().value ) &&
                  _pReplSet->isSendNormal( _primaryID.value ) )
         {
            rc = SDB_CLS_NOT_PRIMARY ;
            goto error ;
         }
         else if ( !CLS_IS_MAJORITY( _pReplSet->getAlivesByTimeout(),
                                     _pReplSet->groupSize() ) )
         {
            rc = SDB_CLS_NOT_PRIMARY ;
            goto error ;
         }
         else if ( waitTime < SHD_NOTPRIMARY_WAITTIME &&
                   !_pEDUCB->isInterrupted() )
         {
            INT32 result = SDB_OK ;

            if ( !hasBlock )
            {
               _pEDUCB->setBlock( EDU_BLOCK_PRIMARY, "Waiting for primary" ) ;
               hasBlock = TRUE ;
            }
            rc = _pReplSet->getFaultEvent()->wait( SHD_WAITTIME_INTERVAL,
                                                   &result ) ;
            if ( SDB_OK == rc && SDB_OK != result )
            {
               rc = result ;
               goto error;
            }

            rc = SDB_OK ;
            waitTime += SHD_WAITTIME_INTERVAL ;
            continue ;
         }
         else
         {
            goto error ;
         }
      }
   done:
      if ( hasBlock )
      {
         _pEDUCB->unsetBlock() ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CKPRIMARYSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CKRBSTATUS, "_clsShdSession::_checkRollbackStatus" )
   INT32 _clsShdSession::_checkRollbackStatus()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CKRBSTATUS ) ;
      UINT32 waitTime = 0 ;
      BOOLEAN hasBlock = FALSE ;

      while( TRUE )
      {
         if ( !pmdGetKRCB()->getTransCB()->isDoRollback() )
         {
            if ( waitTime > 0 && _pEDUCB->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
            break ;
         }
         else if ( waitTime < SHD_TRANSROLLBACK_WAITTIME &&
                   !_pEDUCB->isInterrupted() )
         {
            if ( !hasBlock )
            {
               _pEDUCB->setBlock( EDU_BLOCK_TRANSROLLBACK,
                                  "Waiting for transactions rollback" ) ;
               hasBlock = TRUE ;
            }
            ossSleep( SHD_WAITTIME_INTERVAL ) ;
            waitTime += SHD_WAITTIME_INTERVAL ;
            continue ;
         }

         rc = SDB_DPS_TRANS_DOING_ROLLBACK ;
         goto error ;
      }
   done:
      if ( hasBlock )
      {
         _pEDUCB->unsetBlock() ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CKRBSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CKWRITESTATUS, "_clsShdSession::_checkWriteStatus" )
   INT32 _clsShdSession::_checkWriteStatus()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CKWRITESTATUS ) ;

      BOOLEAN setWrite = FALSE ;
      clsDCBaseInfo *pInfo = _pShdMgr->getDCMgr()->getDCBaseInfo() ;

      /// dc data judge
      if ( pInfo->isReadonly() )
      {
         rc = SDB_CAT_CLUSTER_IS_READONLY ;
         goto error ;
      }
      else if ( DPS_TRANS_WAIT_COMMIT == _pEDUCB->getTransStatus() )
      {
         rc = SDB_RTN_EXIST_INDOUBT_TRANS ;
         goto error ;
      }

      /// First set writeDB, then check primary
      if ( !_pEDUCB->isWritingDB() )
      {
         _pEDUCB->writingDB( TRUE ) ;
         setWrite = TRUE ;
      }

      rc = _checkPrimaryStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDINFO, "Failed to check primary status, rc: %d", rc ) ;
         goto error ;
      }

      rc = _checkRollbackStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDINFO, "Failed to check rollback status, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CKWRITESTATUS, rc ) ;
      return rc ;
   error:
      if ( setWrite )
      {
         _pEDUCB->writingDB( FALSE ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CKPRIMARYWHENREAD, "_clsShdSession::_checkPrimaryWhenRead" )
   INT32 _clsShdSession::_checkPrimaryWhenRead( INT32 flag, INT32 reqFlag )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CKPRIMARYWHENREAD ) ;
      if ( flag & reqFlag )
      {
         rc = _checkPrimaryStatus() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDINFO, "failed to check primary status:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CKPRIMARYWHENREAD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CKSECONDARYWHENREAD, "_clsShdSession::_checkSecondaryWhenRead" )
   INT32 _clsShdSession::_checkSecondaryWhenRead( INT32 flag, INT32 reqFlag )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CKSECONDARYWHENREAD ) ;

      if ( flag & reqFlag )
      {
         rc = _checkPrimaryStatus() ;
         // check return code
         if ( SDB_OK == rc && _pReplSet->groupSize() > 1 )
         {
            rc = SDB_CLS_NOT_SECONDARY ;
         }
         else if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            rc = SDB_OK ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDINFO, "Failed to check secondary status, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CKSECONDARYWHENREAD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CKREPLSTATUS, "_clsShdSession::_checkReplStatus" )
   INT32 _clsShdSession::_checkReplStatus()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CKREPLSTATUS ) ;

      if ( SDB_DB_FULLSYNC == PMD_DB_STATUS() )
      {
         rc = SDB_CLS_FULL_SYNC ;
      }
      else if ( SDB_DB_REBUILDING == PMD_DB_STATUS() )
      {
         rc = SDB_RTN_IN_REBUILD ;
      }
      else if ( SDB_DB_SHUTDOWN == PMD_DB_STATUS() )
      {
         rc = SDB_DATABASE_DOWN ;
      }
      else if ( !pmdGetStartup().isOK() )
      {
         rc = SDB_RTN_IN_REBUILD ;
      }

      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CKREPLSTATUS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CHECKCLSANDGET, "_clsShdSession::_checkCLStatusAndGetSth" )
   INT32 _clsShdSession::_checkCLStatusAndGetSth( const CHAR *name,
                                                  INT32 version,
                                                  BOOLEAN *isMainCL,
                                                  INT16 *w,
                                                  CHAR *mainCLName,
                                                  utilCLUniqueID *clUniqueID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CHECKCLSANDGET ) ;

      rc = _checkReplStatus() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to check status of repl-set:%d", rc ) ;
         goto error ;
      }

      if ( DPS_TRANS_WAIT_COMMIT == eduCB()->getTransStatus() )
      {
         rc = SDB_RTN_EXIST_INDOUBT_TRANS ;
         goto error ;
      }
      else if ( eduCB()->isTransaction() )
      {
         rc = _checkRollbackStatus() ;
         if ( rc )
         {
            goto error ;
         }
      }

      rc = _pFreezingWindow->waitForOpr( name, eduCB(),
                                         eduCB()->isWritingDB() ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _checkCLVersion( name, version, isMainCL, w, mainCLName,
                            clUniqueID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CHECKCLSANDGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CALCW, "_clsShdSession::_calculateW" )
   INT32 _clsShdSession::_calculateW( const INT16 *replSize,
                                      const INT16 *clientW,
                                      INT16 &final )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CALCW ) ;
      INT16 w = 0 ;

      if ( NULL != replSize )
      {
         w = ( NULL == clientW || 0 == *clientW ) ?
                       *replSize : *clientW ;
      }
      else if ( 0 == *clientW )
      {
         w = 1 ;
      }
      else
      {
         w = *clientW ;
      }

      /// When first operation in transaction, should adjust the replsize
      /// by param 'transreplsize'
      if ( w >= 1 &&
           _pEDUCB->isTransaction() &&
           DPS_INVALID_LSN_OFFSET == _pEDUCB->getCurTransLsn() )
      {
         INT16 transReplSize = pmdGetOptionCB()->transReplSize() ;
         if ( transReplSize <= 0 )
         {
            w = transReplSize ;
         }
         else if ( w < transReplSize )
         {
            w = transReplSize ;
         }
      }

      rc = _pReplSet->replSizeCheck( w, final, _pEDUCB ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CALCW, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDSESS__CHKCLVER, "_clsShdSession::_checkCLVersion" )
   INT32 _clsShdSession::_checkCLVersion( const CHAR *name,
                                          INT32 version,
                                          BOOLEAN *isMainCL,
                                          INT16 *w,
                                          CHAR *mainCLName,
                                          utilCLUniqueID *clUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSSHDSESS__CHKCLVER ) ;

      INT32 curVer = -1 ;
      INT16 replSize = 0 ;
      UINT32 groupCount = 0 ;
      _clsCatalogSet *set = NULL ;
      BOOLEAN mainCL = FALSE ;
      BOOLEAN agentLocked = FALSE ;
      const CHAR *clShortName = NULL ;

      // For SYS collections, do not check the version. This limit is added when
      // developing text search. The search engine adapter will query and pop
      // data from capped collections through the shard flat, if the version
      // checking is enable, no operations can be done.
      clShortName = ossStrchr( name, '.' ) ;
      if ( !clShortName || ( clShortName == name + ossStrlen( name ) ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Collection name[%s] is invalid. Full name is "
                          "expected", name ) ;
         goto error ;
      }

      clShortName++ ;
      if ( dmsIsSysCLName( clShortName ) )
      {
         if ( NULL != w )
         {
            *w = 1 ;
         }
         goto done ;
      }

      _pCatAgent->lock_r () ;
      agentLocked = TRUE ;
      set = _pCatAgent->collectionSet( name ) ;
      if ( NULL == set )
      {
         rc = SDB_CLS_NO_CATALOG_INFO ;
         goto error ;
      }

      replSize = set->getW() ;
      curVer = set->getVersion() ;
      groupCount = set->groupCount() ;
      mainCL = set->isMainCL() ;
      if(  NULL != clUniqueID )
      {
         *clUniqueID = set->clUniqueID() ;
      }
      if ( NULL != mainCLName && !set->getMainCLName().empty() )
      {
         ossStrncpy( mainCLName, set->getMainCLName().c_str(),
                     DMS_COLLECTION_FULL_NAME_SZ ) ;
      }
      _pCatAgent->release_r () ;
      agentLocked = FALSE ;

      if ( curVer < 0 )
      {
         rc = SDB_CLS_NO_CATALOG_INFO ;
         goto error ;
      }
      else if ( curVer < version )
      {
         rc = SDB_CLS_DATA_NODE_CAT_VER_OLD ;
         goto error ;
      }
      else if ( curVer > version
                || ( 0 == groupCount && !mainCL ) )
      {
         if ( 0 == groupCount )
         {
            _pCatAgent->lock_w() ;
            _pCatAgent->clear( name ) ;
            _pCatAgent->release_w() ;
         }
         PD_LOG ( PDINFO, "Collecton[%s]: self verions:%d, coord version:%d, "
                  "groupCount:%d", name, curVer, version, groupCount ) ;
         rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
         goto error ;
      }
      else
      {
         if ( NULL != isMainCL )
         {
            *isMainCL = mainCL ;
         }

         if ( NULL != w )
         {
            *w = replSize ;
         }
      }

      // save version
      _clVersion = curVer ;

   done:
      if ( agentLocked )
      {
         _pCatAgent->release_r () ;
      }

      PD_TRACE_EXITRC( SDB__CLSSHDSESS__CHKCLVER, rc ) ;

      return rc ;

   error:
      goto done ;
   }

   INT32 _clsShdSession::_checkClusterActive( MsgHeader *msg )
   {
      INT32 rc = SDB_CAT_CLUSTER_IS_DEACTIVED ;

      if ( MSG_BS_INTERRUPTE == msg->opCode ||
           MSG_COM_SESSION_INIT_REQ == msg->opCode ||
           MSG_BS_MSG_REQ == msg->opCode ||
           MSG_BS_LOB_CLOSE_REQ == msg->opCode ||
           MSG_BS_KILL_CONTEXT_REQ == msg->opCode )
      {
         rc = SDB_OK ;
      }
      else if ( !pmdGetKRCB()->isDBDeactivated() )
      {
         rc = SDB_OK ;
      }
      else if ( MSG_BS_QUERY_REQ == msg->opCode )
      {
         MsgOpQuery *pQuery = ( MsgOpQuery* )msg ;
         if ( 0 == ossStrcmp( &pQuery->name[0],
                              CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC ) )
         {
            rc = SDB_OK ;
         }
      }

      return rc ;
   }

   INT32 _clsShdSession::_updateVCS( const CHAR *fullName,
                                     const BSONObj &updator )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( fullName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         schedTaskMgr *pSvcTaskMgr = pmdGetKRCB()->getSvcTaskMgr() ;

         _info._info.fromBSON( updator, FALSE ) ;

         /// update task info
         _info._ptr = pSvcTaskMgr->getTaskInfoPtr( _info._info.getTaskID(),
                                                   _info._info.getTaskName() ) ;
         /// update monApp's info
         eduCB()->getMonAppCB()->setSvcTaskInfo( _info._ptr.get() ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShdSession::_testCollectionBeforeCreate( const CHAR* clName,
                                                      utilCLUniqueID clUniqueID )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      utilCLUniqueID curClUniqueID = UTIL_UNIQUEID_NULL ;

      rc = rtnTestCollectionCommand( clName, _pDmsCB,
                                     &clUniqueID, &curClUniqueID ) ;

      if ( SDB_DMS_CS_REMAIN == rc )
      {
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

         rcTmp = rtnResolveCollectionSpaceName( clName, ossStrlen( clName ),
                                                csName,
                                                DMS_COLLECTION_SPACE_NAME_SZ ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Failed to resolve collection space name "
                    "from collection name[%s], rc: %d", clName, rcTmp ) ;
            goto error ;
         }

         rcTmp = rtnDropCollectionSpaceCommand( csName, _pEDUCB,
                                                _pDmsCB, _pDpsCB ) ;
         if ( SDB_OK == rcTmp )
         {
            _pCatAgent->lock_w () ;
            _pCatAgent->clearBySpaceName( csName ) ;
            _pCatAgent->release_w () ;

            PD_LOG( PDEVENT, "Drop remain collection space[%s]", csName ) ;
            rcTmp = SDB_DMS_CS_NOTEXIST ;
         }
         if ( SDB_DMS_CS_NOTEXIST == rcTmp )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
         }
         else
         {
            PD_LOG( PDERROR,
                    "Drop cs[%s] before create cl failed, rc: %d.",
                    csName, rcTmp ) ;
         }
      }
      else if ( SDB_DMS_REMAIN == rc )
      {
         rcTmp = rtnDropCollectionCommand( clName, _pEDUCB, _pDmsCB, _pDpsCB,
                                           curClUniqueID ) ;
         if ( SDB_OK == rcTmp )
         {
            _pCatAgent->lock_w () ;
            _pCatAgent->clear( clName ) ;
            _pCatAgent->release_w () ;

            PD_LOG( PDEVENT, "Drop remain collection[%s]", clName ) ;
            rcTmp = SDB_DMS_NOTEXIST ;
         }
         if ( SDB_DMS_NOTEXIST == rcTmp )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         else
         {
            PD_LOG( PDERROR,
                    "Drop cl[%s] before create cl failed, rc: %d.",
                    clName, rcTmp ) ;
         }
      }
      else if ( SDB_DMS_CS_UNIQUEID_CONFLICT == rc )
      {
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

         rcTmp = rtnResolveCollectionSpaceName( clName, ossStrlen( clName ),
                                                csName,
                                                DMS_COLLECTION_SPACE_NAME_SZ ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Failed to resolve collection space name "
                    "from collection name[%s], rc: %d", clName, rcTmp ) ;
            goto error ;
         }

         // try to drop cs if it is empty cs
         rcTmp = _pDmsCB->dropEmptyCollectionSpace( csName, _pEDUCB, _pDpsCB ) ;
         if ( SDB_OK == rcTmp )
         {
            _pCatAgent->lock_w () ;
            _pCatAgent->clearBySpaceName( csName ) ;
            _pCatAgent->release_w () ;

            PD_LOG( PDEVENT, "Drop emtpy collection space[%s]", csName ) ;
            rc = SDB_DMS_CS_NOTEXIST ;
         }
         else if ( SDB_DMS_CS_NOT_EMPTY != rcTmp )
         {
            PD_LOG( PDWARNING,
                    "Try to drop collection space[%s] failed, rc: %d",
                    csName, rcTmp ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

