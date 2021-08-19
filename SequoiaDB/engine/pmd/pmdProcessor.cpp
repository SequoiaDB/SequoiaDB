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

   Source File Name = pmdPorcessor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/12/2014  Lin Youbin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdProcessor.hpp"
#include "rtn.hpp"
#include "../bson/bson.h"
#include "pmdSession.hpp"
#include "pmdRestSession.hpp"
#include "msgMessage.hpp"
#include "sqlCB.hpp"
#include "rtnLob.hpp"
#include "rtnLocalLobStream.hpp"
#include "coordCB.hpp"
#include "coordFactory.hpp"
#include "coordMsgOperator.hpp"
#include "coordInsertOperator.hpp"
#include "coordUpdateOperator.hpp"
#include "coordSqlOperator.hpp"
#include "coordDeleteOperator.hpp"
#include "coordAggrOperator.hpp"
#include "coordLobOperator.hpp"
#include "coordAuthCrtOperator.hpp"
#include "coordAuthDelOperator.hpp"
#include "coordQueryOperator.hpp"
#include "coordInterruptOperator.hpp"
#include "pmdController.hpp"
#include "schedDef.hpp"
#include "dpsUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"


using namespace bson ;

namespace engine
{
   _pmdDataProcessor::_pmdDataProcessor()
   {
      _pKrcb    = pmdGetKRCB() ;
      _pDMSCB   = _pKrcb->getDMSCB() ;
      _pRTNCB   = _pKrcb->getRTNCB() ;
   }

   _pmdDataProcessor::~_pmdDataProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDDATAPROC_PROMSG, "_pmdDataProcessor::processMsg" )
   INT32 _pmdDataProcessor::processMsg( MsgHeader *msg,
                                        rtnContextBuf &contextBuff,
                                        INT64 &contextID,
                                        BOOLEAN &needReply,
                                        BOOLEAN &needRollback,
                                        BSONObjBuilder &builder )
   {
      INT32 rc     = SDB_OK ;
      INT32 opCode = msg->opCode ;
      monClassQuery *monQuery = NULL ;
      ossTick startTime ;
      monClassQueryTmpData tmpData ;
      tmpData = *(eduCB()->getMonAppCB()) ;

      PD_TRACE_ENTRY ( SDB_PMDDATAPROC_PROMSG );

      SDB_ASSERT( getSession(), "Must attach session at first" ) ;

      needRollback = FALSE ;

      if ( eduCB()->getMonQueryCB() == NULL )
      {
         monQuery = pmdGetKRCB()->getMonMgr()->
                    registerMonitorObject<monClassQuery>() ;

         if ( monQuery )
         {
            monQuery->sessionID = eduCB()->getID() ;
            monQuery->opCode = opCode ;
            monQuery->tid = eduCB()->getTID() ;
            eduCB()->setMonQueryCB( monQuery ) ;
         }
      }

      startTime.sample() ;

      if ( MSG_AUTH_VERIFY_REQ == opCode )
      {
         rc = getClient()->authenticate( msg ) ;
      }
      else if ( MSG_AUTH_VERIFY1_REQ == opCode )
      {
         const CHAR *authBuf = NULL ;
         rc = getClient()->authenticate( msg, &authBuf ) ;
         if ( SDB_OK == rc && NULL != authBuf )
         {
            try
            {
               contextBuff = rtnContextBuf( BSONObj( authBuf ) ) ;
            }
            catch ( std::exception &e )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Unexpected error: %s", e.what() ) ;
            }
         }
      }
      else if ( MSG_BS_INTERRUPTE == opCode )
      {
         rc = _onInterruptMsg( msg, getDPSCB() ) ;
      }
      else if ( MSG_BS_INTERRUPTE_SELF == opCode )
      {
         rc = _onInterruptSelfMsg() ;
      }
      else if ( MSG_BS_DISCONNECT == opCode )
      {
         rc = _onDisconnectMsg() ;
      }
      else
      {
         if ( !getClient()->isAuthed() )
         {
            rc = getClient()->authenticate( "", "" ) ;
            if ( rc )
            {
               goto done ;
            }
         }

         switch( opCode )
         {
            case MSG_BS_MSG_REQ :
               rc = _onMsgReqMsg( msg ) ;
               break ;
            case MSG_BS_UPDATE_REQ :
            {
               MsgOpUpdate *pUpdateMsg = ( MsgOpUpdate* )msg ;
               utilUpdateResult upResult ;
               needRollback = TRUE ;
               rc = _onUpdateReqMsg( msg, getDPSCB(), upResult ) ;
               if ( FLG_UPDATE_RETURNNUM & pUpdateMsg->flags )
               {
                  upResult.toBSON( builder ) ;
               }
               break ;
            }
            case MSG_BS_INSERT_REQ :
            {
               MsgOpInsert *pInsertMsg = ( MsgOpInsert* )msg ;
               utilInsertResult inResult ;
               needRollback = TRUE ;
               rc = _onInsertReqMsg( msg, contextBuff, inResult ) ;
               if ( FLG_INSERT_RETURNNUM & pInsertMsg->flags )
               {
                  inResult.toBSON( builder ) ;
               }
               break ;
            }
            case MSG_BS_QUERY_REQ :
               rc = _onQueryReqMsg( msg, getDPSCB(), contextBuff, contextID,
                                    needRollback, builder ) ;
               break ;
            case MSG_BS_DELETE_REQ :
            {
               MsgOpDelete *pDeleteMsg = ( MsgOpDelete* )msg ;
               utilDeleteResult delResult ;
               needRollback = TRUE ;
               rc = _onDelReqMsg( msg, getDPSCB(), delResult ) ;
               if ( FLG_DELETE_RETURNNUM & pDeleteMsg->flags )
               {
                  delResult.toBSON( builder ) ;
               }
               break ;
            }
            case MSG_BS_GETMORE_REQ :
               rc = _onGetMoreReqMsg( msg, contextBuff, contextID,
                                      needRollback ) ;
               break ;
            case MSG_BS_KILL_CONTEXT_REQ :
               rc = _onKillContextsReqMsg( msg ) ;
               break ;
            case MSG_BS_SQL_REQ :
               rc = _onSQLMsg( msg, contextID, getDPSCB(),
                               needRollback, builder ) ;
               break ;
            case MSG_BS_TRANS_BEGIN_REQ :
               rc = _onTransBeginMsg() ;
               break ;
            case MSG_BS_TRANS_COMMIT_REQ :
               needRollback = TRUE ;
               rc = _onTransCommitMsg( getDPSCB() ) ;
               break ;
            case MSG_BS_TRANS_ROLLBACK_REQ :
               rc = _onTransRollbackMsg( getDPSCB() ) ;
               break ;
            case MSG_BS_AGGREGATE_REQ :
               rc = _onAggrReqMsg( msg, contextID ) ;
               break ;
            case MSG_BS_LOB_OPEN_REQ :
               rc = _onOpenLobMsg( msg, getDPSCB(), contextID, contextBuff ) ;
               break ;
            case MSG_BS_LOB_WRITE_REQ:
               rc = _onWriteLobMsg( msg ) ;
               break ;
            case MSG_BS_LOB_READ_REQ:
               rc = _onReadLobMsg( msg, contextBuff ) ;
               break ;
            case MSG_BS_LOB_LOCK_REQ:
               rc = _onLockLobMsg( msg ) ;
               break ;
            case MSG_BS_LOB_CLOSE_REQ:
               rc = _onCloseLobMsg( msg, contextBuff ) ;
               break ;
            case MSG_BS_LOB_REMOVE_REQ:
               rc = _onRemoveLobMsg( msg, getDPSCB() ) ;
               break ;
            case MSG_BS_LOB_TRUNCATE_REQ:
               rc = _onTruncateLobMsg( msg, getDPSCB() ) ;
               break ;
            case MSG_BS_LOB_GETRTDETAIL_REQ:
               rc = _onGetLobRTDetailMsg( msg, contextBuff ) ;
               break ;
            case MSG_BS_LOB_CREATELOBID_REQ:
               rc = _onCreateLobIDMsg( msg, contextBuff ) ;
               break ;
            case MSG_AUTH_CRTUSR_REQ:
            case MSG_AUTH_DELUSR_REQ:
               rc = SDB_RTN_COORD_ONLY ;
               break ;
            default :
               PD_LOG( PDWARNING, "Session[%s] recv unknow msg[type:[%d]%d, "
                       "len: %d, tid: %d, routeID: %d.%d.%d, reqID: %lld]",
                       getSession()->sessionName(), IS_REPLY_TYPE(opCode),
                       GET_REQUEST_TYPE(opCode), msg->messageLength,
                       msg->TID, msg->routeID.columns.groupID,
                       msg->routeID.columns.nodeID,
                       msg->routeID.columns.serviceID, msg->requestID ) ;
               rc = SDB_INVALIDARG ;
               break ;
         }
      }

      if ( eduCB()->getMonQueryCB() )
      {
         monQuery = eduCB()->getMonQueryCB() ;
         ossTick endTime ;
         endTime.sample() ;
         monQuery->responseTime += endTime - startTime ;
         monQuery->rowsReturned += contextBuff.recordNum() ;

         tmpData.diff(*(eduCB()->getMonAppCB())) ;
         monQuery->incMetrics(tmpData) ;

         if ( !monQuery->anchorToContext )
         {
            pmdGetKRCB()->getMonMgr()->removeMonitorObject( monQuery ) ;

            eduCB()->setMonQueryCB( NULL ) ;
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB_PMDDATAPROC_PROMSG, rc ) ;
      return rc ;
   }

   INT32 _pmdDataProcessor::doRollback()
   {
      INT32 rc = SDB_OK ;

      if ( eduCB() )
      {
         DPS_TRANS_ID transID = eduCB()->getTransID() ;

         rc = rtnTransRollback( eduCB(), getDPSCB() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rollback transaction(ID:%s, IDAttr:%s) failed, "
                    "rc: %d", dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str(),
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::doCommit()
   {
      INT32 rc = SDB_OK ;
      if ( eduCB() )
      {
         DPS_TRANS_ID transID = eduCB()->getTransID() ;

         rc = rtnTransCommit( eduCB(), getDPSCB() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Commit transaction(ID:%s, IDAttr:%s) failed, "
                    "rc: %d", dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str(),
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onMsgReqMsg( MsgHeader * msg )
   {
      return rtnMsg( (MsgOpMsg*)msg ) ;
   }

   INT32 _pmdDataProcessor::_updateVCS( const CHAR *fullName,
                                        const BSONObj &updator )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( fullName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         schedTaskMgr *pSvcTaskMgr = pmdGetKRCB()->getSvcTaskMgr() ;
         schedItem *pItem = ( schedItem* )getSession()->getSchedItemPtr() ;
         BSONObj objSrc = pItem->_info.toBSON() ;
         BSONObj objDest ;

         objDest = rtnUpdator2Obj( objSrc, updator ) ;

         pItem->_info.fromBSON( objDest, TRUE ) ;

         /// update task info
         pItem->_ptr = pSvcTaskMgr->getTaskInfoPtr( pItem->_info.getTaskID(),
                                                    pItem->_info.getTaskName() ) ;
         /// update monApp's info
         eduCB()->getMonAppCB()->setSvcTaskInfo( pItem->_ptr.get() ) ;
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

   INT32 _pmdDataProcessor::_insertVCS( const CHAR *fullName,
                                        const BSONObj &insertor )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( fullName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         schedTaskMgr *pSvcTaskMgr = pmdGetKRCB()->getSvcTaskMgr() ;
         schedItem *pItem = ( schedItem* )getSession()->getSchedItemPtr() ;

         pItem->_info.fromBSON( insertor, TRUE ) ;

         /// update task info
         pItem->_ptr = pSvcTaskMgr->getTaskInfoPtr( pItem->_info.getTaskID(),
                                                    pItem->_info.getTaskName() ) ;
         /// update monApp's info
         eduCB()->getMonAppCB()->setSvcTaskInfo( pItem->_ptr.get() ) ;
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

   INT32 _pmdDataProcessor::_deleteVCS( const CHAR *fullName,
                                        const BSONObj &deletor )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( fullName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         schedTaskMgr *pSvcTaskMgr = pmdGetKRCB()->getSvcTaskMgr() ;
         schedItem *pItem = ( schedItem* )getSession()->getSchedItemPtr() ;

         pItem->_info.reset() ;

         /// update task info
         pItem->_ptr = pSvcTaskMgr->getTaskInfoPtr( pItem->_info.getTaskID(),
                                                    pItem->_info.getTaskName() ) ;
         /// update monApp's info
         eduCB()->getMonAppCB()->setSvcTaskInfo( pItem->_ptr.get() ) ;
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

   INT32 _pmdDataProcessor::_checkTransOperator( BOOLEAN checkDps,
                                                 BOOLEAN autoCommit )
   {
      INT32 rc = SDB_OK ;

      if ( pmdGetDBRole() == SDB_ROLE_DATA ||
           pmdGetDBRole() == SDB_ROLE_CATALOG )
      {
         rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
         if ( !autoCommit )
         {
            PD_LOG_MSG( PDERROR, "In sharding mode, couldn't execute "
                        "transaction operation from local service" ) ;
         }
      }
      else if ( checkDps && !getDPSCB() )
      {
         rc = SDB_PERM ;
         if ( !autoCommit )
         {
            PD_LOG_MSG( PDERROR, "Couldn't execute transaction operation when "
                        "dps log is off" ) ;
         }
      }

      return rc ;
   }

   INT32 _pmdDataProcessor::_checkTransAutoCommit( const MsgHeader *msg,
                                                   BOOLEAN checkDps )
   {
      INT32 rc = SDB_OK ;

      if ( !eduCB()->isTransaction() )
      {
         if ( eduCB()->getTransExecutor()->isTransAutoCommit() )
         {
            rc = _checkTransOperator( checkDps, TRUE ) ;
            if ( SDB_OK == rc )
            {
               rc = rtnTransBegin( eduCB(), TRUE ) ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
      }
      else if ( eduCB()->isAutoCommitTrans() )
      {
         rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
         PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                 rc ) ;
      }

      return rc ;
   }

   INT32 _pmdDataProcessor::_onUpdateReqMsg( MsgHeader *msg,
                                             SDB_DPSCB *dpsCB,
                                             utilUpdateResult &upResult )
   {
      INT32 rc    = SDB_OK ;
      INT32 flags = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pSelectorBuffer = NULL ;
      CHAR *pUpdatorBuffer  = NULL ;
      CHAR *pHintBuffer     = NULL ;

      rc = msgExtractUpdate( (CHAR*)msg, &flags, &pCollectionName,
                             &pSelectorBuffer, &pUpdatorBuffer,
                             &pHintBuffer );
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract update message failed, "
                   "rc: %d", getSession()->sessionName(), rc ) ;

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

      /// check auto-commit
      rc = _checkTransAutoCommit( msg ) ;
      if ( rc )
      {
         goto error ;
      }

      try
      {
         BSONObj selector( pSelectorBuffer );
         BSONObj updator( pUpdatorBuffer );
         BSONObj hint( pHintBuffer );
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, Matcher:%s, Updator:%s, Hint:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             selector.toPoolString().c_str(),
                             updator.toPoolString().c_str(),
                             hint.toPoolString().c_str(),
                             flags, flags ) ;

         PD_LOG ( PDDEBUG, "Session[%s] Update:\nMatcher: %s\nUpdator: %s\n"
                  "hint: %s\nFlag: 0x%08x(%u)", getSession()->sessionName(),
                  selector.toPoolString().c_str(),
                  updator.toPoolString().c_str(), hint.toPoolString().c_str(),
                  flags, flags ) ;

         rc = rtnUpdate( pCollectionName, selector, updator, hint,
                         flags, eduCB(), _pDMSCB, dpsCB, 1, &upResult ) ;

         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_UPDATE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "UpdatedNum:%llu, ModifiedNum:%llu, InsertedNum:%llu, "
                      "Matcher:%s, Updator:%s, Hint:%s, Flag:0x%08x(%u)",
                      upResult.updateNum(), upResult.modifiedNum(),
                      upResult.insertedNum(), selector.toPoolString().c_str(),
                      updator.toPoolString().c_str(),
                      hint.toPoolString().c_str(), flags, flags ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG ( PDERROR, "Session[%s] Failed to create selector and "
                      "updator for update: %s",
                      getSession()->sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onInsertReqMsg( MsgHeader *msg,
                                             rtnContextBuf &buff,
                                             utilInsertResult &inResult )
   {
      INT32 rc    = SDB_OK ;
      INT32 flag  = 0 ;
      INT32 count = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pInsertor       = NULL ;

      rc = msgExtractInsert( (CHAR *)msg, &flag, &pCollectionName,
                             &pInsertor, count ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extrace insert msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      if ( (flag & FLG_INSERT_CONTONDUP) && (flag & FLG_INSERT_REPLACEONDUP) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR,"Conflict insert flag(CONTONDUP and REPLACEONDUP):"
                     "flag=%d,rc=%d", flag, rc ) ;
         goto error ;
      }

      /// When insert virtual cs
      if ( 0 == ossStrncmp( pCollectionName, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         try
         {
            BSONObj objInsertor( pInsertor ) ;
            rc = _insertVCS( pCollectionName, objInsertor ) ;
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

      /// check auto-commit
      rc = _checkTransAutoCommit( msg ) ;
      if ( rc )
      {
         goto error ;
      }

      try
      {
         BSONObj insertor( pInsertor ) ;
         // add list op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, Insertors:%s, ObjNum:%d, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             insertor.toPoolString().c_str(),
                             count, flag, flag ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] insert objs: %s\nObjCount: %d\n"
                  "Collection: %s\nFlag:0x%08x(%u)",
                  getSession()->sessionName(), insertor.toPoolString().c_str(),
                  count, pCollectionName, flag, flag ) ; */

         rc = rtnInsert( pCollectionName, insertor, count, flag, eduCB(),
                         &inResult ) ;
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_INSERT_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc, "InsertedNum:%llu, "
                      "DuplicatedNum:%llu, ObjNum:%u, Insertor:%s, "
                      "Flag:0x%08x(%u)",
                      inResult.insertedNum(), inResult.duplicatedNum(),
                      count, insertor.toPoolString().c_str(), flag,
                      flag ) ;

         PD_RC_CHECK( rc, PDERROR, "Session[%s] insert objs[%s, count:%d, "
                      "collection: %s] failed, rc: %d",
                      getSession()->sessionName(),
                      insertor.toPoolString().c_str(),
                      count, pCollectionName, rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Session[%s] insert objs occur exception: %s",
                     getSession()->sessionName(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onQueryReqMsg( MsgHeader * msg,
                                            SDB_DPSCB *dpsCB,
                                            _rtnContextBuf &buffObj,
                                            INT64 &contextID,
                                            BOOLEAN &needRollback,
                                            BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flags = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pQueryBuff = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderByBuffer = NULL ;
      CHAR *pHintBuffer = NULL ;
      INT64 numToSkip = -1 ;
      INT64 numToReturn = -1 ;
      _rtnCommand *pCommand = NULL ;
      monClassQuery *monQuery = NULL ;

      rc = msgExtractQuery ( (CHAR *)msg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQueryBuff,
                             &pFieldSelector, &pOrderByBuffer, &pHintBuffer ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract query msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      if ( !rtnIsCommand ( pCollectionName ) )
      {
         rtnContextBase *pContext = NULL ;

         /// check auto-commit
         rc = _checkTransAutoCommit( msg, ( flags & FLG_QUERY_MODIFY ) ?
                                          TRUE : FALSE ) ;
         if ( rc )
         {
            goto error ;
         }

         try
         {
            BSONObj matcher ( pQueryBuff ) ;
            BSONObj selector ( pFieldSelector ) ;
            BSONObj orderBy ( pOrderByBuffer ) ;
            BSONObj hint ( pHintBuffer ) ;
            // add last op info
            MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                                "Collection:%s, Matcher:%s, Selector:%s, "
                                "OrderBy:%s, Hint:%s, Skip:%llu, Limit:%lld, "
                                "Flag:0x%08x(%u)",
                                pCollectionName,
                                matcher.toPoolString().c_str(),
                                selector.toPoolString().c_str(),
                                orderBy.toPoolString().c_str(),
                                hint.toPoolString().c_str(),
                                numToSkip, numToReturn,
                                flags, flags ) ;

            if ( flags & FLG_QUERY_MODIFY )
            {
               needRollback = TRUE ;
            }

            /*
            PD_LOG ( PDDEBUG, "Session[%s] Query: Matcher: %s\nSelector: "
                     "%s\nOrderBy: %s\nHint:%s\nSkip: %llu\nLimit: %lld\n"
                     "Flag: 0x%08x(%u)", getSession()->sessionName(),
                     matcher.toString().c_str(), selector.toString().c_str(),
                     orderBy.toString().c_str(), hint.toString().c_str(),
                     numToSkip, numToReturn, flags ,flags ) ; */

            rc = rtnQuery( pCollectionName, selector, matcher, orderBy,
                           hint, flags, eduCB(), numToSkip, numToReturn,
                           _pDMSCB, _pRTNCB, contextID, &pContext, TRUE ) ;
            /// AUDIT
            PD_AUDIT_OP( ( flags & FLG_QUERY_MODIFY ? AUDIT_DML : AUDIT_DQL ),
                         MSG_BS_QUERY_REQ, AUDIT_OBJ_CL,
                         pCollectionName, rc,
                         "ContextID:%lld, Matcher:%s, Selector:%s, OrderBy:%s, "
                         "Hint:%s, Skip:%llu, Limit:%lld, Flag:0x%08x(%u)",
                         contextID,
                         matcher.toString().c_str(),
                         selector.toString().c_str(),
                         orderBy.toString().c_str(),
                         hint.toString().c_str(),
                         numToSkip, numToReturn,
                         flags, flags ) ;
            /// Jduge error
            if ( rc )
            {
               goto error ;
            }

            if ( eduCB()->isAutoCommitTrans() )
            {
               eduCB()->setCurAutoTransCtxID( contextID ) ;
            }
            /// if write operator, need set dps info( local session: w=1)
            if ( pContext && pContext->isWrite() )
            {
               pContext->setWriteInfo( dpsCB, 1 ) ;
            }

            monQuery = pContext->getMonQueryCB() ;

            if ( monQuery && pContext->getPlanRuntime() )
            {
               monQuery->accessPlanID = pContext->
                                         getPlanRuntime()->
                                         getAccessPlanID() ;
            }

            if ( ( flags & FLG_QUERY_WITH_RETURNDATA ) && NULL != pContext )
            {
               rc = pContext->getMore( -1, buffObj, eduCB() ) ;
               if ( rc || pContext->eof() )
               {
                  _pRTNCB->contextDelete( contextID, eduCB() ) ;
                  contextID = -1 ;
               }

               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
               }
               else if ( rc )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to query with return "
                          "data, rc: %d", getSession()->sessionName(), rc ) ;
                  goto error ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Session[%s] Failed to create matcher and "
                     "selector for QUERY: %s", getSession()->sessionName(),
                     e.what () ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
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

         MON_SAVE_CMD_DETAIL( eduCB()->getMonAppCB(), pCommand->type(),
                              "Command:%s, Collection:%s, Match:%s, "
                              "Selector:%s, OrderBy:%s, Hint:%s, Skip:%llu, "
                              "Limit:%lld, Flag:0x%08x(%u)",
                              pCollectionName,
                              pCommand->collectionFullName() ?
                              pCommand->collectionFullName() : "",
                              BSONObj(pQueryBuff).toString().c_str(),
                              BSONObj(pFieldSelector).toString().c_str(),
                              BSONObj(pOrderByBuffer).toString().c_str(),
                              BSONObj(pHintBuffer).toString().c_str(),
                              numToSkip, numToReturn, flags, flags ) ;

         PD_LOG ( PDDEBUG, "Command: %s", pCommand->name () ) ;

         //run command
         rc = rtnRunCommand( pCommand, getSession()->getServiceType(),
                             eduCB(), _pDMSCB, _pRTNCB,
                             dpsCB, 1, &contextID ) ;
         if ( pCommand->hasBuff() )
         {
            buffObj = pCommand->getBuff() ;
         }
         if ( rc && pCommand->getResult() )
         {
            pCommand->getResult()->toBSON( builder ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( pCommand )
      {
         rtnReleaseCommand( &pCommand ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onDelReqMsg( MsgHeader * msg,
                                          SDB_DPSCB *dpsCB,
                                          utilDeleteResult &delResult )
   {
      INT32 rc    = SDB_OK ;
      INT32 flags = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pDeletorBuffer  = NULL ;
      CHAR *pHintBuffer     = NULL ;

      rc = msgExtractDelete ( (CHAR *)msg , &flags, &pCollectionName,
                              &pDeletorBuffer, &pHintBuffer ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract delete msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      /// When delete virtual cs
      if ( 0 == ossStrncmp( pCollectionName, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         try
         {
            BSONObj objDeletor( pDeletorBuffer ) ;
            rc = _deleteVCS( pCollectionName, objDeletor ) ;
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

      /// check auto-commit
      rc = _checkTransAutoCommit( msg ) ;
      if ( rc )
      {
         goto error ;
      }

      try
      {
         BSONObj deletor ( pDeletorBuffer ) ;
         BSONObj hint ( pHintBuffer ) ;
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, Deletor:%s, Hint:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             deletor.toPoolString().c_str(),
                             hint.toPoolString().c_str(),
                             flags, flags ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] Delete: Deletor: %s\nhint: %s\n"
                  "Flag: 0x%08x(%u)",
                  getSession()->sessionName(), deletor.toString().c_str(),
                  hint.toString().c_str(), flags, flags ) ; */
         rc = rtnDelete( pCollectionName, deletor, hint, flags, eduCB(),
                         _pDMSCB, dpsCB, 1, &delResult ) ;
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_DELETE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "DeletedNum:%u, Deletor:%s, Hint:%s, Flag:0x%08x(%u)",
                      delResult.deletedNum(), deletor.toPoolString().c_str(),
                      hint.toPoolString().c_str(), flags, flags ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG ( PDERROR, "Session[%s] Failed to create deletor for "
                      "DELETE: %s", getSession()->sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_beginTrans( BOOLEAN isAutoCommit )
   {
      return rtnTransBegin( eduCB(), isAutoCommit ) ;
   }

   INT32 _pmdDataProcessor::_onGetMoreReqMsg( MsgHeader * msg,
                                              rtnContextBuf &buffObj,
                                              INT64 &contextID,
                                              BOOLEAN &needRollback )
   {
      INT32 rc         = SDB_OK ;
      INT32 numToRead  = 0 ;
      rtnContext *pContext = NULL ;

      rc = msgExtractGetMore ( (CHAR*)msg, &numToRead, &contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract get more msg failed, "
                   "rc: %d", getSession()->sessionName(), rc ) ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, NumToRead:%d",
                          contextID, numToRead ) ;

      /*
      PD_LOG ( PDDEBUG, "Session[%s] GetMore: contextID:%lld\nnumToRead: %d",
               getSession()->sessionName(), contextID, numToRead ) ; */

      pContext = _pRTNCB->contextFind ( contextID, eduCB() ) ;
      if ( !pContext )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      needRollback = pContext->needRollback() ;

      rc = rtnGetMore ( pContext, numToRead, buffObj, eduCB(), _pRTNCB ) ;
      if ( rc )
      {
         contextID = -1 ;
         goto error ;
      }
      else if ( pContext->eof() &&
                contextID == eduCB()->getCurAutoTransCtxID() )
      {
         eduCB()->setCurAutoTransCtxID( -1 ) ;
      }

   done:
      if ( SDB_DMS_EOC == rc )
      {
         needRollback = FALSE ;
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onKillContextsReqMsg( MsgHeader *msg )
   {
      PD_LOG ( PDDEBUG, "session[%s] _onKillContextsReqMsg",
               getSession()->sessionName() ) ;

      INT32 rc = SDB_OK ;
      INT32 contextNum = 0 ;
      INT64 *pContextIDs = NULL ;

      rc = msgExtractKillContexts ( (CHAR*)msg, &contextNum, &pContextIDs ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract kill contexts msg failed, "
                   "rc: %d", getSession()->sessionName(), rc ) ;

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

      rc = rtnKillContexts ( contextNum, pContextIDs, eduCB(), _pRTNCB ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onSQLMsg( MsgHeader *msg,
                                       INT64 &contextID,
                                       SDB_DPSCB *dpsCB,
                                       BOOLEAN &needRollback,
                                       BSONObjBuilder &builder )
   {
      CHAR *sql = NULL ;
      INT32 rc = SDB_OK ;
      SQL_CB *sqlcb = pmdGetKRCB()->getSqlCB() ;

      rc = msgExtractSql( (CHAR*)msg, &sql ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract sql msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      MONQUERY_SET_NAME( eduCB(), sql ) ;

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "%s", sql ) ;

      rc = sqlcb->exec( sql, eduCB(), contextID, needRollback, &builder ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onTransBeginMsg ()
   {
      INT32 rc = _checkTransOperator() ;
      if ( SDB_OK == rc )
      {
         if ( eduCB()->isAutoCommitTrans() )
         {
            rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
            PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                    rc ) ;
         }
         else
         {
            rc = rtnTransBegin( eduCB() ) ;
         }
      }
      return rc ;
   }

   INT32 _pmdDataProcessor::_onTransCommitMsg ( SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      if ( eduCB()->isTransaction() )
      {
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                             "TransactionID: 0x%016x(%llu)",
                             eduCB()->getTransID(),
                             eduCB()->getTransID() ) ;

         rc = rtnTransCommit( eduCB(), dpsCB ) ;
      }

      return rc ;
   }

   INT32 _pmdDataProcessor::_onTransRollbackMsg ( SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      if ( eduCB()->isTransaction() )
      {
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_ROLLBACK_REQ,
                             "TransactionID: 0x%016x(%llu)",
                             eduCB()->getTransID(),
                             eduCB()->getTransID() ) ;

         rc = rtnTransRollback( eduCB(), dpsCB ) ;
      }

      return rc ;
   }

   INT32 _pmdDataProcessor::_onAggrReqMsg( MsgHeader *msg, INT64 &contextID )
   {
      INT32 rc    = SDB_OK ;
      CHAR *pObjs = NULL ;
      INT32 count = 0 ;
      INT32 flags = 0 ;
      CHAR *pCollectionName = NULL ;

      rc = msgExtractAggrRequest( (CHAR*)msg, &pCollectionName,
                                  &pObjs, count, &flags ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extrace aggr msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      MONQUERY_SET_NAME( eduCB(), pCollectionName ) ;

      try
      {
         BSONObj objs( pObjs ) ;

         /// Prepare last info
         CHAR szTmp[ MON_APP_LASTOP_DESC_LEN + 1 ] = { 0 } ;
         UINT32 len = 0 ;
         const CHAR *pObjData = pObjs ;
         for ( INT32 i = 0 ; i < count ; ++i )
         {
            BSONObj tmpObj( pObjData ) ;
            len += ossSnprintf( szTmp, MON_APP_LASTOP_DESC_LEN - len,
                                "%s", tmpObj.toPoolString().c_str() ) ;
            pObjData += ossAlignX( (UINT32)tmpObj.objsize(), 4 ) ;
            if ( len >= MON_APP_LASTOP_DESC_LEN )
            {
               break ;
            }
         }

         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, ObjNum:%u, Objs:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName, count, szTmp,
                             flags, flags ) ;

         rc = rtnAggregate( pCollectionName, objs, count, flags, eduCB(),
                            _pDMSCB, contextID ) ;

         /// AUDIT
         PD_AUDIT_OP( AUDIT_DQL, msg->opCode, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "ContextID:%lld, ObjNum:%u, Objs:%s, Flag:0x%08x(%u)",
                      contextID, count, szTmp, flags, flags ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s] occurred exception in aggr: %s",
                 getSession()->sessionName(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onOpenLobMsg( MsgHeader *msg, SDB_DPSCB *dpsCB,
                                           SINT64 &contextID,
                                           rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rtnLobStream *pStream = NULL ;
      BSONObj lob ;
      rc = msgExtractOpenLobRequest( ( const CHAR * )msg, &header, lob ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract open msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Option:%s", lob.toPoolString().c_str() ) ;

         /// pStream will delete in context
         pStream = SDB_OSS_NEW _rtnLocalLobStream() ;
         if ( !pStream )
         {
            PD_LOG( PDERROR, "Create lob stream failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         rc = rtnOpenLob( lob, header->flags, eduCB(), dpsCB, pStream,
                          header->w, contextID, buffObj ) ;
         /// Jduge
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to open lob:%d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onWriteLobMsg( MsgHeader *msg )
   {
      INT32 rc         = SDB_OK ;
      UINT32 len       = 0 ;
      INT64 offset     = -1 ;
      const CHAR *data = NULL ;
      const MsgOpLob *header = NULL ;

      rc = msgExtractWriteLobRequest( ( const CHAR * )msg, &header,
                                        &len, &offset, &data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract write msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Len:%u, Offset:%llu",
                          header->contextID, len, offset ) ;

      rc = rtnWriteLob( header->contextID, eduCB(), len, data, offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onReadLobMsg( MsgHeader *msg,
                                           rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      SINT64 offset = -1 ;
      UINT32 readLen = 0 ;
      UINT32 length = 0 ;
      const CHAR *data = NULL ;

      rc = msgExtractReadLobRequest( ( const CHAR * )msg, &header,
                                      &readLen, &offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract read msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, Len:%u, Offset:%llu",
                          header->contextID, readLen, offset ) ;

      rc = rtnReadLob( header->contextID, eduCB(),
                       readLen, offset, &data, length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob:%d", rc ) ;
         goto error ;
      }

      buffObj = rtnContextBuf( data, length, 0 ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onLockLobMsg( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      INT64 offset = 0 ;
      INT64 length = -1 ;

      rc = msgExtractLockLobRequest( ( const CHAR * )msg, &header, &offset, &length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract close msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnLockLob( header->contextID, eduCB(), offset, length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to lock lob:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onCloseLobMsg( MsgHeader *msg,
                                            rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rc = msgExtractCloseLobRequest( ( const CHAR * )msg, &header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract close msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnCloseLob( header->contextID, eduCB(), &buffObj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onRemoveLobMsg( MsgHeader *msg, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj meta ;
      const MsgOpLob *header = NULL ;
      rc = msgExtractRemoveLobRequest( ( const CHAR * )msg, &header,
                                        meta ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract remove msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Option:%s", meta.toPoolString().c_str() ) ;

         rc = rtnRemoveLob( meta, header->flags, header->w, eduCB(), dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onTruncateLobMsg( MsgHeader *msg, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj meta ;
      const MsgOpLob *header = NULL ;

      rc = msgExtractTruncateLobRequest( ( const CHAR * )msg, &header,
                                         meta ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract truncate msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         // add last op info
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Option:%s", meta.toString().c_str() ) ;

         rc = rtnTruncateLob( meta, header->flags, header->w, eduCB(), dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to truncate lob:%d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onGetLobRTDetailMsg( MsgHeader *msg,
                                                  rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      rc = msgExtractGetLobRTDetailRequest( (const CHAR*)msg, &header ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract get Lob runtime detail msg"
                   ":rc=%d", rc) ;

      rc = rtnGetLobRTDetail( header->contextID, eduCB(), &buffObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onCreateLobIDMsg( MsgHeader *msg,
                                               rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      bson::OID oid ;
      rc = msgExtractCreateLobIDRequest( (const CHAR*)msg, &header,
                                         obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to extract create LobID msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnCreateLobID( obj, oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to create lobID:rc=%d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         builder.appendOID( FIELD_NAME_LOB_OID, &oid ) ;
         buffObj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to init Object id:exception=%s,rc=%d",
               e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onInterruptMsg( MsgHeader *msg, SDB_DPSCB *dpsCB )
   {
      PD_LOG ( PDINFO, "Session[%s, %lld] received interrupt msg",
               getSession()->sessionName(), eduCB()->getID() ) ;

      // delete all contextID, rollback transaction
      INT64 contextID = -1 ;
      while ( -1 != ( contextID = eduCB()->contextPeek() ) )
      {
         _pRTNCB->contextDelete ( contextID, NULL ) ;
      }

      INT32 rcTmp = rtnTransRollback( eduCB(), dpsCB );
      if ( rcTmp )
      {
         PD_LOG ( PDERROR, "Failed to rollback(rc=%d)", rcTmp );
      }

      return SDB_OK ;
   }

   INT32 _pmdDataProcessor::_onInterruptSelfMsg()
   {
      PD_LOG( PDINFO, "Session[%s, %lld] recv interrupt self msg",
              getSession()->sessionName(), eduCB()->getID() ) ;
      return SDB_OK ;
   }

   INT32 _pmdDataProcessor::_onDisconnectMsg()
   {
      PD_LOG( PDEVENT, "Session[%s, %lld] recv disconnect msg",
              getSession()->sessionName(), eduCB()->getID() ) ;
      getClient()->disconnect() ;
      return SDB_OK ;
   }

   const CHAR* _pmdDataProcessor::processorName() const
   {
      return "DataProcessor" ;
   }

   SDB_PROCESSOR_TYPE _pmdDataProcessor::processorType() const
   {
      return SDB_PROCESSOR_DATA ;
   }

   void _pmdDataProcessor::_onAttach()
   {
   }

   void _pmdDataProcessor::_onDetach()
   {
      // rollback transaction
      if ( DPS_INVALID_TRANS_ID != eduCB()->getTransID() )
      {
         INT32 rc = doRollback() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s] rollback trans info failed, rc: %d",
                    getSession()->sessionName(), rc ) ;
         }
      }

      // Remove remote session, if any.
      rtnRemoteMessenger *messenger =
         pmdGetKRCB()->getRTNCB()->getRemoteMessenger() ;
      if ( messenger )
      {
         messenger->removeSession( eduCB() ) ;
      }

      // delete all context
      INT64 contextID = -1 ;
      while ( -1 != ( contextID = eduCB()->contextPeek() ) )
      {
         _pRTNCB->contextDelete( contextID, NULL ) ;
      }
   }

   //***************_pmdCoordProcessor*********************
   _pmdCoordProcessor::_pmdCoordProcessor()
   {
   }

   _pmdCoordProcessor::~_pmdCoordProcessor()
   {
   }

   const CHAR* _pmdCoordProcessor::processorName() const
   {
      return "CoordProcessor" ;
   }

   SDB_PROCESSOR_TYPE _pmdCoordProcessor::processorType() const
   {
      return SDB_PROCESSOR_COORD;
   }

   void _pmdCoordProcessor::_onAttach()
   {
      // must call base _onAttach first
      pmdDataProcessor::_onAttach() ;
      // do self
      if ( sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->registerEDU( eduCB() ) ;
      }
   }

   void _pmdCoordProcessor::_onDetach()
   {
      // must call base _onDetach first( will kill context, but kill context
      // need the coordSession
      pmdDataProcessor::_onDetach() ;
      // do self
      if ( sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->unregEUD( eduCB() ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDCOORDPROC_PROCOORDMSG, "_pmdCoordProcessor::_processCoordMsg" )
   INT32 _pmdCoordProcessor::_processCoordMsg( MsgHeader *msg,
                                               INT64 &contextID,
                                               rtnContextBuf &contextBuff,
                                               BOOLEAN &needReply,
                                               BOOLEAN &needRollback )
   {
      INT32 rc = SDB_OK ;
      INT32 opCode = msg->opCode ;
      CoordCB *pCoordCB = _pKrcb->getCoordCB() ;
      coordResource *pResource = pCoordCB->getResource() ;

      PD_TRACE_ENTRY ( SDB_PMDCOORDPROC_PROCOORDMSG ) ;

      if ( MSG_AUTH_VERIFY_REQ == opCode || MSG_AUTH_VERIFY1_REQ == opCode )
      {
         rc = SDB_COORD_UNKNOWN_OP_REQ ;
         goto done ;
      }
      else if ( MSG_BS_INTERRUPTE == opCode ||
                MSG_BS_INTERRUPTE_SELF == opCode ||
                MSG_BS_DISCONNECT == opCode )
      {
         // don't need auth
      }
      else if ( !getClient()->isAuthed() )
      {
         rc = getClient()->authenticate( "", "" ) ;
         if ( rc )
         {
            goto done ;
         }
      }

      switch ( opCode )
      {
         case MSG_BS_INTERRUPTE :
         {
            coordInterrupt opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;

            /// rollback transaction
            if ( eduCB()->isTransaction() )
            {
               INT32 rcTmp = doRollback() ;
               if ( rcTmp )
               {
                  PD_LOG( PDERROR, "Rollback trans info failed, rc: %d",
                          rcTmp ) ;
                  rc = rcTmp ;
               }
            }
            break ;
         }
         case MSG_BS_MSG_REQ :
         {
            coordMsgOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_INSERT_REQ :
         {
            coordInsertOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_UPDATE_REQ :
         {
            coordUpdateOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_SQL_REQ :
         {
            coordSqlOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            /// needRollback call must after opr.execute, because the sql
            /// command is parsed in execute
            needRollback = opr.needRollback() ;
            break ;
         }
         case MSG_BS_DELETE_REQ :
         {
            coordDeleteOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_TRANS_BEGIN_REQ :
         {
            coordTransBegin opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_TRANS_COMMIT_REQ :
         {
            coordTransCommit opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_TRANS_ROLLBACK_REQ :
         {
            coordTransRollback opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_AGGREGATE_REQ :
         {
            coordAggrOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_OPEN_REQ :
         {
            coordOpenLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_WRITE_REQ :
         {
            coordWriteLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_READ_REQ :
         {
            coordReadLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_LOCK_REQ :
         {
            coordLockLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_CLOSE_REQ :
         {
            coordCloseLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_REMOVE_REQ :
         {
            coordRemoveLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_TRUNCATE_REQ:
         {
            coordTruncateLob opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_GETRTDETAIL_REQ:
         {
            coordGetLobRTDetail opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_LOB_CREATELOBID_REQ:
         {
            coordCreateLobID opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_AUTH_CRTUSR_REQ :
         {
            coordAuthCrtOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_AUTH_DELUSR_REQ :
         {
            coordAuthDelOperator opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
            break ;
         }
         case MSG_BS_QUERY_REQ :
         {
            rc = _onQueryReqMsg( msg, contextBuff, contextID, needRollback ) ;
            break ;
         }
         default :
         {
            rc = SDB_COORD_UNKNOWN_OP_REQ ;
            break ;
         }
      }

      PD_TRACE1 ( SDB_PMDCOORDPROC_PROCOORDMSG, PD_PACK_INT ( needRollback ) );

      if ( rc && contextBuff.size() == 0 )
      {
         BSONObj obj = utilGetErrorBson( rc, eduCB()->getInfo(
                                         EDU_INFO_ERROR ) ) ;
         contextBuff = rtnContextBuf( obj ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_PMDCOORDPROC_PROCOORDMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdCoordProcessor::_onQueryReqMsg( MsgHeader *msg,
                                             _rtnContextBuf &buffObj,
                                             INT64 &contextID,
                                             BOOLEAN &needRollback )
   {
      INT32 rc = SDB_OK ;
      coordResource *pResource = NULL ;
      coordCommandFactory *pFactory = NULL ;
      coordOperator *pOpr = NULL ;
      pResource = pmdGetKRCB()->getCoordCB()->getResource() ;

      CHAR *pCollectionName            = NULL ;
      INT32 flag                       = 0 ;
      INT64 numToSkip                  = 0 ;
      INT64 numToReturn                = 0 ;
      CHAR *pQuery                     = NULL ;
      CHAR *pSelector                  = NULL ;
      CHAR *pOrderby                   = NULL ;
      CHAR *pHint                      = NULL ;

      rc = msgExtractQuery( (CHAR*)msg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery, &pSelector,
                            &pOrderby, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse query request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      MONQUERY_SET_NAME(eduCB(), pCollectionName) ;

      /// command
      if ( pCollectionName && CMD_ADMIN_PREFIX[0] == pCollectionName[0] )
      {
         pFactory = coordGetFactory() ;
         rc = pFactory->create( &pCollectionName[1], pOpr ) ;
         if ( rc )
         {
            if ( SDB_COORD_UNKNOWN_OP_REQ != rc )
            {
               PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                       pCollectionName, rc ) ;
            }
            goto error ;
         }

         // add last op info
         MON_SAVE_CMD_DETAIL( eduCB()->getMonAppCB(), CMD_UNKNOW - 1,
                              "Command:%s, Match:%s, "
                              "Selector:%s, OrderBy:%s, Hint:%s, Skip:%llu, "
                              "Limit:%lld, Flag:0x%08x(%u)",
                              pCollectionName,
                              BSONObj(pQuery).toString().c_str(),
                              BSONObj(pSelector).toString().c_str(),
                              BSONObj(pOrderby).toString().c_str(),
                              BSONObj(pHint).toString().c_str(),
                              numToSkip, numToReturn, flag, flag ) ;

         MONQUERY_SET_QUERY_TEXT( eduCB(), eduCB()->getMonAppCB()->_lastOpDetail ) ;

         rc = pOpr->init( pResource, eduCB() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    pOpr->getName(), rc ) ;
            goto error ;
         }

         needRollback = pOpr->needRollback() ;

         rc = pOpr->execute( msg, eduCB(), contextID, &buffObj ) ;
         if ( rc )
         {
            PD_LOG( ( SDB_COORD_UNKNOWN_OP_REQ == rc ? PDINFO : PDERROR ),
                    "Execute operator[%s] failed, rc: %d",
                    pOpr->getName(), rc ) ;
            goto error ;
         }
      }
      else
      {
         rtnContextBase *pContext = NULL ;
         coordQueryOperator opr ;
         rc = opr.init( pResource, eduCB() ) ;
         PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                      opr.getName(), rc ) ;

         /// transaction check
         if ( eduCB()->isAutoCommitTrans() &&
              opr.canPrepareTrans( eduCB(), msg ) )
         {
            rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
            PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                    rc ) ;
            goto error ;
         }

         rc = opr.execute( msg, eduCB(), contextID, &buffObj ) ;
         /// should call opr.needRollback() after execute. because
         /// the func 'needRollback()' is take effect in execute()
         needRollback = opr.needRollback() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }

         // query with return data
         if ( ( flag & FLG_QUERY_WITH_RETURNDATA ) &&
              -1 != contextID &&
              NULL != ( pContext = _pRTNCB->contextFind( contextID ) ) )
         {
            rc = pContext->getMore( -1, buffObj, eduCB() ) ;
            if ( rc || pContext->eof() )
            {
               _pRTNCB->contextDelete( contextID, eduCB() ) ;
               contextID = -1 ;
            }

            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Failed to query with return data, "
                       "rc: %d", rc ) ;
            }
         }
      }
   done:
      if ( pOpr )
      {
         pFactory->release( pOpr ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDCOORDPROC_PROMSG, "_pmdCoordProcessor::processMsg" )
   INT32 _pmdCoordProcessor::processMsg( MsgHeader *msg,
                                         rtnContextBuf &contextBuff,
                                         INT64 &contextID,
                                         BOOLEAN &needReply,
                                         BOOLEAN &needRollback,
                                         BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      monClassQuery *monQueryCB = NULL ;
      ossTick startTime ;

      monClassQueryTmpData tmpData ;
      tmpData = *(eduCB()->getMonAppCB()) ;

      BSONObjBuilder clientInfoBuilder ;
      PD_TRACE_ENTRY ( SDB_PMDCOORDPROC_PROMSG );

      if ( eduCB()->getMonQueryCB() == NULL )
      {
         monQueryCB = pmdGetKRCB()->getMonMgr()->
                      registerMonitorObject<monClassQuery>() ;

         if ( monQueryCB )
         {
            monQueryCB->sessionID = eduCB()->getID() ;
            monQueryCB->opCode = msg->opCode ;
            monQueryCB->tid = eduCB()->getTID() ;
            monQueryCB->clientTID = msg->TID ;
            monQueryCB->clientHost.assign(getClient()->getFromIPAddr()) ;

            eduCB()->setMonQueryCB( monQueryCB ) ;
         }
      }

      startTime.sample() ;

      rc = _processCoordMsg( msg, contextID, contextBuff,
                             needReply, needRollback ) ;
      if ( SDB_COORD_UNKNOWN_OP_REQ == rc )
      {
         contextBuff.release() ;
         rc = _pmdDataProcessor::processMsg( msg, contextBuff, contextID,
                                             needReply, needRollback,
                                             builder ) ;
      }
      else
      {
         if ( eduCB()->getMonQueryCB() )
         {
            monQueryCB = eduCB()->getMonQueryCB() ;
            ossTick endTime ;
            endTime.sample() ;
            monQueryCB->responseTime += endTime - startTime ;
            monQueryCB->rowsReturned += contextBuff.recordNum() ;
            tmpData.diff(*(eduCB()->getMonAppCB())) ;
            monQueryCB->incMetrics(tmpData) ;
            if ( !monQueryCB->anchorToContext )
            {
               pmdGetKRCB()->getMonMgr()->removeMonitorObject( monQueryCB ) ;
               eduCB()->setMonQueryCB( NULL ) ;
            }
         }
      }

      if ( rc )
      {
         if ( SDB_APP_INTERRUPT == rc )
         {
            PD_LOG ( PDINFO, "Agent is interrupt" ) ;
         }
         else if ( SDB_DMS_EOC != rc )
         {
            PD_LOG ( PDERROR, "Error processing Agent request, rc=%d", rc ) ;
         }
      }

      PD_TRACE_EXITRC ( SDB_PMDCOORDPROC_PROMSG, rc );
      return rc ;
   }

   INT32 _pmdCoordProcessor::doRollback()
   {
      INT32 rc = SDB_OK ;
      CoordCB *pCoordCB = _pKrcb->getCoordCB() ;
      coordResource *pResource = pCoordCB->getResource() ;
      DPS_TRANS_ID transID = eduCB()->getTransID() ;

      coordTransRollback rollbackOpr ;
      rc = rollbackOpr.init( pResource, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback init operator[%s] failed, rc: %d",
                 rollbackOpr.getName(), rc ) ;
         goto error ;
      }

      rc = rollbackOpr.rollback( eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback transaction(ID:%s, IDAttr:%s) failed, "
                 "rc: %d", dpsTransIDToString( transID ).c_str(),
                 dpsTransIDAttrToString( transID ).c_str(),
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdCoordProcessor::doCommit()
   {
      INT32 rc = SDB_OK ;
      CoordCB *pCoordCB = _pKrcb->getCoordCB() ;
      coordResource *pResource = pCoordCB->getResource() ;

      INT64 contextID = -1 ;
      DPS_TRANS_ID transID = eduCB()->getTransID() ;
      MsgOpTransCommit commitMsg ;
      coordTransCommit commitOpr ;
      rc = commitOpr.init( pResource, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                 commitOpr.getName(), rc ) ;
         goto error ;
      }

      commitMsg.header.messageLength = sizeof( MsgOpTransCommit ) ;
      commitMsg.header.opCode = MSG_BS_TRANS_COMMIT_REQ ;
      commitMsg.header.requestID = 0 ;
      commitMsg.header.routeID.value = 0 ;
      commitMsg.header.TID = 0 ;

      rc = commitOpr.execute( &commitMsg.header, eduCB(),
                              contextID, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Commit transaction(ID:%s, IDAttr:%s) failed, "
                 "rc: %d", dpsTransIDToString( transID ).c_str(),
                 dpsTransIDAttrToString( transID ).c_str(),
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdCoordProcessor::_beginTrans( BOOLEAN isAutoCommit )
   {
      INT32 rc = SDB_OK ;
      CoordCB *pCoordCB = _pKrcb->getCoordCB() ;
      coordResource *pResource = pCoordCB->getResource() ;

      coordTransBegin oprBegin ;
      rc = oprBegin.init( pResource, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                 oprBegin.getName(), rc ) ;
         goto error ;
      }

      rc = oprBegin.beginTrans( eduCB(), isAutoCommit ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

