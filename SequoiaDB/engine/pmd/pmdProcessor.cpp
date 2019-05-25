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

   INT32 _pmdDataProcessor::processMsg( MsgHeader *msg,
                                        rtnContextBuf &contextBuff,
                                        INT64 &contextID,
                                        BOOLEAN &needReply )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( getSession(), "Must attach session at first" ) ;

      if ( MSG_AUTH_VERIFY_REQ == msg->opCode )
      {
         rc = getClient()->authenticate( msg ) ;
      }
      else if ( MSG_BS_INTERRUPTE == msg->opCode )
      {
         rc = _onInterruptMsg( msg, getDPSCB() ) ;
      }
      else if ( MSG_BS_INTERRUPTE_SELF == msg->opCode )
      {
         rc = _onInterruptSelfMsg() ;
      }
      else if ( MSG_BS_DISCONNECT == msg->opCode )
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

         switch( msg->opCode )
         {
            case MSG_BS_MSG_REQ :
               rc = _onMsgReqMsg( msg ) ;
               break ;
            case MSG_BS_UPDATE_REQ :
               rc = _onUpdateReqMsg( msg, getDPSCB() ) ;
               break ;
            case MSG_BS_INSERT_REQ :
               rc = _onInsertReqMsg( msg ) ;
               break ;
            case MSG_BS_QUERY_REQ :
               rc = _onQueryReqMsg( msg, getDPSCB(), contextBuff, contextID ) ;
               break ;
            case MSG_BS_DELETE_REQ :
               rc = _onDelReqMsg( msg, getDPSCB() ) ;
               break ;
            case MSG_BS_GETMORE_REQ :
               rc = _onGetMoreReqMsg( msg, contextBuff, contextID ) ;
               break ;
            case MSG_BS_KILL_CONTEXT_REQ :
               rc = _onKillContextsReqMsg( msg ) ;
               break ;
            case MSG_BS_SQL_REQ :
               rc = _onSQLMsg( msg, contextID, getDPSCB() ) ;
               break ;
            case MSG_BS_TRANS_BEGIN_REQ :
               rc = _onTransBeginMsg() ;
               break ;
            case MSG_BS_TRANS_COMMIT_REQ :
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
            case MSG_AUTH_CRTUSR_REQ:
            case MSG_AUTH_DELUSR_REQ:
               rc = SDB_RTN_COORD_ONLY ;
               break ;
            default :
               PD_LOG( PDWARNING, "Session[%s] recv unknow msg[type:[%d]%d, "
                       "len: %d, tid: %d, routeID: %d.%d.%d, reqID: %lld]",
                       getSession()->sessionName(), IS_REPLY_TYPE(msg->opCode),
                       GET_REQUEST_TYPE(msg->opCode), msg->messageLength,
                       msg->TID, msg->routeID.columns.groupID,
                       msg->routeID.columns.nodeID,
                       msg->routeID.columns.serviceID, msg->requestID ) ;
               rc = SDB_INVALIDARG ;
               break ;
         }
      }

   done:
      return rc ;
   }

   INT32 _pmdDataProcessor::_onMsgReqMsg( MsgHeader * msg )
   {
      return rtnMsg( (MsgOpMsg*)msg ) ;
   }

   INT32 _pmdDataProcessor::_onUpdateReqMsg( MsgHeader * msg, SDB_DPSCB *dpsCB )
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

      try
      {
         INT64   updatedNum = 0 ;
         INT32   insertNum = 0 ;
         BSONObj selector( pSelectorBuffer );
         BSONObj updator( pUpdatorBuffer );
         BSONObj hint( pHintBuffer );
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, Matcher:%s, Updator:%s, Hint:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             selector.toString().c_str(),
                             updator.toString().c_str(),
                             hint.toString().c_str(),
                             flags, flags ) ;

         PD_LOG ( PDDEBUG, "Session[%s] Update:\nMatcher: %s\nUpdator: %s\n"
                  "hint: %s\nFlag: 0x%08x(%u)", getSession()->sessionName(), 
                  selector.toString().c_str(),
                  updator.toString().c_str(), hint.toString().c_str(),
                  flags, flags ) ;

         rc = rtnUpdate( pCollectionName, selector, updator, hint,
                         flags, eduCB(), _pDMSCB, dpsCB, 1, &updatedNum,
                         &insertNum ) ;
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_UPDATE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "UpdatedNum:%llu, InsertedNum:%u, Matcher:%s, "
                      "Updator:%s, Hint:%s, Flag:0x%08x(%u)",
                      updatedNum, insertNum,
                      selector.toString().c_str(),
                      updator.toString().c_str(),
                      hint.toString().c_str(), flags, flags ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s] Failed to create selector and updator "
                  "for update: %s", getSession()->sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onInsertReqMsg( MsgHeader * msg )
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

      try
      {
         INT32   insertedNum = 0 ;
         INT32   ignoredNum = 0 ;
         BSONObj insertor( pInsertor ) ;
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, Insertors:%s, ObjNum:%d, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             insertor.toString().c_str(),
                             count, flag, flag ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] insert objs: %s\nObjCount: %d\n"
                  "Collection: %s\nFlag:0x%08x(%u)",
                  getSession()->sessionName(), insertor.toString().c_str(),
                  count, pCollectionName, flag, flag ) ; */

         rc = rtnInsert( pCollectionName, insertor, count, flag, eduCB(),
                         &insertedNum, &ignoredNum ) ;
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_INSERT_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc, "InsertedNum:%u, IgnoredNum:%u, "
                      "ObjNum:%u, Insertor:%s, Flag:0x%08x(%u)", insertedNum,
                      ignoredNum, count, insertor.toString().c_str(), flag,
                      flag ) ;

         PD_RC_CHECK( rc, PDERROR, "Session[%s] insert objs[%s, count:%d, "
                      "collection: %s] failed, rc: %d", 
                      getSession()->sessionName(), insertor.toString().c_str(), 
                      count, pCollectionName, rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s] insert objs occur exception: %s",
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
                                            INT64 &contextID )
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

      rc = msgExtractQuery ( (CHAR *)msg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQueryBuff,
                             &pFieldSelector, &pOrderByBuffer, &pHintBuffer ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract query msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      if ( !rtnIsCommand ( pCollectionName ) )
      {
         rtnContextBase *pContext = NULL ;
         try
         {
            BSONObj matcher ( pQueryBuff ) ;
            BSONObj selector ( pFieldSelector ) ;
            BSONObj orderBy ( pOrderByBuffer ) ;
            BSONObj hint ( pHintBuffer ) ;
            MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                                "Collection:%s, Matcher:%s, Selector:%s, "
                                "OrderBy:%s, Hint:%s, Skip:%llu, Limit:%lld, "
                                "Flag:0x%08x(%u)",
                                pCollectionName,
                                matcher.toString().c_str(),
                                selector.toString().c_str(),
                                orderBy.toString().c_str(),
                                hint.toString().c_str(),
                                numToSkip, numToReturn,
                                flags, flags ) ;

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
            if ( rc )
            {
               goto error ;
            }

            if ( pContext && pContext->isWrite() )
            {
               pContext->setWriteInfo( dpsCB, 1 ) ;
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

         rc = rtnRunCommand( pCommand, getSession()->getServiceType(),
                             eduCB(), _pDMSCB, _pRTNCB,
                             dpsCB, 1, &contextID ) ;
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

   INT32 _pmdDataProcessor::_onDelReqMsg( MsgHeader * msg, SDB_DPSCB *dpsCB )
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

      try
      {
         INT64 deletedNum = 0 ;
         BSONObj deletor ( pDeletorBuffer ) ;
         BSONObj hint ( pHintBuffer ) ;
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, Deletor:%s, Hint:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             deletor.toString().c_str(),
                             hint.toString().c_str(),
                             flags, flags ) ;

         /*
         PD_LOG ( PDDEBUG, "Session[%s] Delete: Deletor: %s\nhint: %s\n"
                  "Flag: 0x%08x(%u)",
                  getSession()->sessionName(), deletor.toString().c_str(), 
                  hint.toString().c_str(), flags, flags ) ; */
         rc = rtnDelete( pCollectionName, deletor, hint, flags, eduCB(), 
                         _pDMSCB, dpsCB, 1, &deletedNum ) ;
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_DELETE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "DeletedNum:%u, Deletor:%s, Hint:%s, Flag:0x%08x(%u)",
                      deletedNum, deletor.toString().c_str(),
                      hint.toString().c_str(), flags, flags ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s] Failed to create deletor for "
                  "DELETE: %s", getSession()->sessionName(), e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onGetMoreReqMsg( MsgHeader * msg,
                                              rtnContextBuf &buffObj,
                                              INT64 &contextID )
   {
      INT32 rc         = SDB_OK ;
      INT32 numToRead  = 0 ;

      rc = msgExtractGetMore ( (CHAR*)msg, &numToRead, &contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract get more msg failed, "
                   "rc: %d", getSession()->sessionName(), rc ) ;

      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "ContextID:%lld, NumToRead:%d",
                          contextID, numToRead ) ;

      /*
      PD_LOG ( PDDEBUG, "Session[%s] GetMore: contextID:%lld\nnumToRead: %d",
               getSession()->sessionName(), contextID, numToRead ) ; */

      rc = rtnGetMore ( contextID, numToRead, buffObj, eduCB(), _pRTNCB ) ;

   done:
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
                                       SDB_DPSCB *dpsCB )
   {
      CHAR *sql = NULL ;
      INT32 rc = SDB_OK ;
      SQL_CB *sqlcb = pmdGetKRCB()->getSqlCB() ;
      BOOLEAN needRollback = FALSE ;

      rc = msgExtractSql( (CHAR*)msg, &sql ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s] extract sql msg failed, rc: %d",
                   getSession()->sessionName(), rc ) ;

      MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                          "%s", sql ) ;

      rc = sqlcb->exec( sql, eduCB(), contextID, needRollback ) ;
      if ( rc )
      {
         if ( needRollback )
         {
            INT32 rcTmp = rtnTransRollback( eduCB(), dpsCB );
            if ( rcTmp )
            {
               PD_LOG ( PDERROR, "Failed to rollback, rc: %d", rcTmp );
            }
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onTransBeginMsg ()
   {
      INT32 rc = SDB_OK ;
      if ( pmdGetDBRole() != SDB_ROLE_STANDALONE )
      {
         rc = SDB_PERM ;
         PD_LOG( PDERROR, "In sharding mode, couldn't execute "
                 "transaction operation from local service" ) ;
         goto error ;
      }
      else
      {
         rc = rtnTransBegin( eduCB() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onTransCommitMsg ( SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      if ( pmdGetDBRole() != SDB_ROLE_STANDALONE )
      {
         rc = SDB_PERM ;
         PD_LOG( PDERROR, "In sharding mode, couldn't execute "
                 "transaction operation from local service" ) ;
         goto error ;
      }
      else
      {
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                             "TransactionID: 0x%016x(%llu)",
                             eduCB()->getTransID(),
                             eduCB()->getTransID() ) ;

         rc = rtnTransCommit( eduCB(), dpsCB ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdDataProcessor::_onTransRollbackMsg ( SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      if ( pmdGetDBRole() != SDB_ROLE_STANDALONE )
      {
         rc = SDB_PERM ;
         PD_LOG( PDERROR, "In sharding mode, couldn't execute "
                 "transaction operation from local service" ) ;
         goto error ;
      }
      else
      {
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), MSG_BS_TRANS_ROLLBACK_REQ,
                             "TransactionID: 0x%016x(%llu)",
                             eduCB()->getTransID(),
                             eduCB()->getTransID() ) ;

         rc = rtnTransRollback( eduCB(), dpsCB ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
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

      try
      {
         BSONObj objs( pObjs ) ;

         CHAR szTmp[ MON_APP_LASTOP_DESC_LEN + 1 ] = { 0 } ;
         UINT32 len = 0 ;
         const CHAR *pObjData = pObjs ;
         for ( INT32 i = 0 ; i < count ; ++i )
         {
            BSONObj tmpObj( pObjData ) ;
            len += ossSnprintf( szTmp, MON_APP_LASTOP_DESC_LEN - len,
                                "%s", tmpObj.toString().c_str() ) ;
            pObjData += ossAlignX( (UINT32)tmpObj.objsize(), 4 ) ;
            if ( len >= MON_APP_LASTOP_DESC_LEN )
            {
               break ;
            }
         }

         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Collection:%s, ObjNum:%u, Objs:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName, count, szTmp,
                             flags, flags ) ;

         rc = rtnAggregate( pCollectionName, objs, count, flags, eduCB(),
                            _pDMSCB, contextID ) ;

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
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Option:%s", lob.toString().c_str() ) ;

         pStream = SDB_OSS_NEW _rtnLocalLobStream() ;
         if ( !pStream )
         {
            PD_LOG( PDERROR, "Create lob stream failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         rc = rtnOpenLob( lob, header->flags, eduCB(), dpsCB, pStream,
                          header->w, contextID, buffObj ) ;
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
         MON_SAVE_OP_DETAIL( eduCB()->getMonAppCB(), msg->opCode,
                             "Option:%s", meta.toString().c_str() ) ;

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

   INT32 _pmdDataProcessor::_onInterruptMsg( MsgHeader *msg, SDB_DPSCB *dpsCB )
   {
      PD_LOG ( PDEVENT, "Session[%s, %lld] recieved interrupt msg",
               getSession()->sessionName(), eduCB()->getID() ) ;

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
      PD_LOG( PDEVENT, "Session[%s, %lld] recv interrupt self msg",
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
      if ( DPS_INVALID_TRANS_ID != eduCB()->getTransID() )
      {
         INT32 rc = rtnTransRollback( eduCB(), getDPSCB() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s] rollback trans info failed, rc: %d",
                    getSession()->sessionName(), rc ) ;
         }
      }

      INT64 contextID = -1 ;
      while ( -1 != ( contextID = eduCB()->contextPeek() ) )
      {
         _pRTNCB->contextDelete( contextID, NULL ) ;
      }
   }

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
      pmdDataProcessor::_onAttach() ;
      if ( sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->registerEDU( eduCB() ) ;
      }
   }

   void _pmdCoordProcessor::_onDetach()
   {
      pmdDataProcessor::_onDetach() ;
      if ( sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->unregEUD( eduCB() ) ;
      }
   }

   INT32 _pmdCoordProcessor::_processCoordMsg( MsgHeader *msg, 
                                               INT64 &contextID,
                                               rtnContextBuf &contextBuff )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needRollback = FALSE ;
      CoordCB *pCoordCB = _pKrcb->getCoordCB() ;
      coordResource *pResource = pCoordCB->getResource() ;

      if ( MSG_AUTH_VERIFY_REQ == msg->opCode )
      {
         rc = SDB_COORD_UNKNOWN_OP_REQ ;
         goto done ;
      }
      else if ( MSG_BS_INTERRUPTE == msg->opCode ||
                MSG_BS_INTERRUPTE_SELF == msg->opCode ||
                MSG_BS_DISCONNECT == msg->opCode )
      {
      }
      else if ( !getClient()->isAuthed() )
      {
         rc = getClient()->authenticate( "", "" ) ;
         if ( rc )
         {
            goto done ;
         }
      }

      switch ( msg->opCode )
      {
         case MSG_BS_INTERRUPTE :
         {
            coordInterrupt opr ;
            rc = opr.init( pResource, eduCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                         opr.getName(), rc ) ;
            needRollback = opr.needRollback() ;
            rc = opr.execute( msg, eduCB(), contextID, &contextBuff ) ;
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

      if ( rc && contextBuff.size() == 0 )
      {
         BSONObj obj = utilGetErrorBson( rc, eduCB()->getInfo(
                                         EDU_INFO_ERROR ) ) ;
         contextBuff = rtnContextBuf( obj ) ;
      }

      if ( needRollback && rc )
      {
         coordTransRollback rollbackOpr ;
         INT32 rcTmp = rollbackOpr.init( pResource, eduCB() ) ;
         PD_RC_CHECK( rcTmp, PDERROR, "Init operator[%s] failed, rc: %d",
                      rollbackOpr.getName(), rcTmp ) ;

         rollbackOpr.rollback( eduCB() ) ;
      }

   done:
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
         needRollback = opr.needRollback() ;
         rc = opr.execute( msg, eduCB(), contextID, &buffObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }

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

   INT32 _pmdCoordProcessor::processMsg( MsgHeader *msg,
                                         rtnContextBuf &contextBuff,
                                         INT64 &contextID,
                                         BOOLEAN &needReply )
   {
      INT32 rc = SDB_OK ;

      rc = _processCoordMsg( msg, contextID, contextBuff ) ;
      if ( SDB_COORD_UNKNOWN_OP_REQ == rc )
      {
         contextBuff.release() ;
         rc = _pmdDataProcessor::processMsg( msg, contextBuff,
                                             contextID, needReply ) ;
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

      return rc ;
   }

}

