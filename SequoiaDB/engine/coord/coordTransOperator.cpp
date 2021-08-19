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

   Source File Name = coordTransOperator.cpp

   Descriptive Name = Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordTransOperator.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "coordUtil.hpp"
#include "rtnCommandDef.hpp"
#include "rtn.hpp"
#include "utilMemListPool.hpp"
#include "dpsUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordTransOperator implement
   */
   _coordTransOperator::_coordTransOperator()
   {
      _recvNum = 0 ;
   }

   _coordTransOperator::~_coordTransOperator()
   {
   }

   BOOLEAN _coordTransOperator::needRollback() const
   {
      return TRUE ;
   }

   INT32 _coordTransOperator::doOnGroups( coordSendMsgIn &inMsg,
                                          coordSendOptions &options,
                                          pmdEDUCB *cb,
                                          coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      if ( _canPrepareTrans( cb, inMsg.msg() ) )
      {
         if ( !cb->isTransaction() &&
              cb->getTransExecutor()->isTransAutoCommit() )
         {
            _prepareForTrans( cb, inMsg.msg() ) ;

            /// push down autocommit to data node,
            /// otherwise begin transaction
            if ( !_canPushDownAutoCommit( inMsg, options, cb ) )
            {
               _groupSession.getPropSite()->beginTrans( cb, TRUE ) ;
            }

            /// transaction should access with primary
            options._primary = TRUE ;
         }
         else if ( cb->isTransaction() )
         {
            if ( SDB_OK != cb->getTransRC() )
            {
               PD_LOG_MSG( PDERROR, "Transaction(%s) must rollback due to "
                           "error(%d)",
                           dpsTransIDToString( cb->getTransID() ).c_str(),
                           cb->getTransRC() ) ;
               rc = cb->getTransRC() ;
               goto error ;
            }
            // need add transaction info for the send msg
            _prepareForTrans( cb, inMsg.msg() ) ;

            /// transaction should access with primary
            options._primary = TRUE ;
         }
      }

      /// when data is old version
      if ( _remoteHandler.isVersion0() && _isTrans( cb, inMsg.msg() ) )
      {
         ROUTE_RC_MAP newNodeMap ;
         // build trans session on new data groups
         rc = buildTransSession( options._groupLst, cb, newNodeMap ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to build transaction session on "
                    "data nodes, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = coordOperator::doOnGroups( inMsg, options, cb, result ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Do command[%d] on data groups failed, rc: %d",
                 inMsg.opCode(), rc ) ;
         goto error ;
      }
      else if ( result.nokSize() > 0 )
      {
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordTransOperator::_isTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      if ( MSG_BS_TRANS_BEGIN_REQ == GET_REQUEST_TYPE( pMsg->opCode ) )
      {
         return FALSE ;
      }
      return cb->isTransaction() ;
   }

   BOOLEAN _coordTransOperator::_canPrepareTrans( pmdEDUCB *cb,
                                                  const MsgHeader *pMsg ) const
   {
      return TRUE ;
   }

   BOOLEAN _coordTransOperator::_canPushDownAutoCommit( coordSendMsgIn &inMsg,
                                                        coordSendOptions &options,
                                                        pmdEDUCB *cb ) const
   {
      if ( options._groupLst.size() <= 1 )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void _coordTransOperator::_onNodeReply( INT32 processType,
                                           MsgOpReply *pReply,
                                           pmdEDUCB *cb,
                                           coordSendMsgIn &inMsg )
   {
      if ( pReply->contextID > 0 )
      {
         _recvNum += pReply->contextID ;
      }
   }

   INT32 _coordTransOperator::releaseTransSession( SET_NODEID &nodes,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdRemoteSession *pSession = NULL ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itrSub ;
      MsgOpTransRollback msgReq ;

      msgReq.header.messageLength = sizeof( MsgOpTransRollback ) ;
      msgReq.header.opCode = MSG_BS_TRANS_ROLLBACK_REQ ;
      msgReq.header.routeID.value = 0 ;
      msgReq.header.TID = cb->getTID() ;

      pSession = _groupSession.getSession() ;
      pSession->resetAllSubSession() ;

      SET_NODEID::iterator itSet = nodes.begin() ;
      while( itSet != nodes.end() )
      {
         pSub = pSession->addSubSession( *itSet ) ;
         pSub->setReqMsg( ( MsgHeader* )&msgReq, PMD_EDU_MEM_NONE ) ;

         /// delete trans node
         _groupSession.getPropSite()->delTransNode( pSub->getNodeID() ) ;

         rc = pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Release node[%s] failed, rc: %d",
                    routeID2String( *itSet ).c_str(), rc ) ;
            /// remove the sub session
            pSession->resetSubSession( *itSet ) ;
         }
         ++itSet ;
      }

      /// get reply
      rc = pSession->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Wait reply for release transaction failed, "
                 "rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      itrSub = pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while ( itrSub.more() )
      {
         pSub = itrSub.next() ;
         MsgOpReply *pReply = ( MsgOpReply* )pSub->getRspMsg() ;

         if ( pReply->flags )
         {
            PD_LOG( PDWARNING, "Release node[%s]'s transaction failed, "
                    "rc: %d", routeID2String( pReply->header.routeID ).c_str(),
                    pReply->flags ) ;
         }
      }

      /// clear all sub session
      pSession->resetAllSubSession() ;

      return rc ;
   }

   INT32 _coordTransOperator::rollback( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SET_NODEID nodes ;
      DPS_TRANS_ID transID = cb->getTransID() ;

      PD_LOG ( PDEVENT, "Begin to rollback transaction(ID:%s, IDAttr:%s)...",
               dpsTransIDToString( transID ).c_str(),
               dpsTransIDAttrToString( transID ).c_str() ) ;

      _groupSession.getPropSite()->dumpTransNode( nodes ) ;

      if ( 0 == nodes.size() )
      {
         goto done ;
      }

      cb->startTransRollback() ;

      if ( DPS_TRANS_DOING == cb->getTransStatus() ||
           DPS_TRANS_DOING_INTERRUPT == cb->getTransStatus() )
      {
         rc = releaseTransSession( nodes, cb ) ;
      }

   done:
      _groupSession.getPropSite()->endTrans( cb ) ;
      cb->stopTransRollback() ;
      return rc ;
   }

   INT32 _coordTransOperator::buildTransSession( const CoordGroupList &groupLst,
                                                 pmdEDUCB *cb,
                                                 ROUTE_RC_MAP &newNodeMap )
   {
      INT32 rc = SDB_OK ;

      if ( !cb->isTransaction() )
      {
         goto done ;
      }
      else
      {
         coordSendOptions options( TRUE ) ;
         coordProcessResult result ;
         coordSessionPropSite *pPropSite = _groupSession.getPropSite() ;

         CoordGroupList::const_iterator iterGroup ;
         MsgOpTransBegin msgReq ;
         coordSendMsgIn inMsg( (MsgHeader*)&msgReq ) ;

         msgReq.header.messageLength = sizeof( MsgOpTransBegin ) ;
         msgReq.header.opCode = MSG_BS_TRANS_BEGIN_REQ ;
         msgReq.header.routeID.value = 0 ;
         msgReq.header.TID = cb->getTID() ;
         msgReq.transID = DPS_TRANS_GET_ID( cb->getTransID() ) ;
         ossMemset( msgReq.reserved, 0, sizeof( msgReq.reserved ) ) ;

         iterGroup = groupLst.begin() ;
         while(  iterGroup != groupLst.end() )
         {
            if ( !pPropSite->hasTransNode( iterGroup->first ) )
            {
               /// not found, need to being the trans
               options._groupLst[ iterGroup->first ] = iterGroup->second ;
            }
            ++iterGroup ;
         }

         newNodeMap.clear() ;
         result._pOkRC = &newNodeMap ;
         rc = coordOperator::doOnGroups( inMsg, options, cb, result ) ;
         // add ok route id to trans node id
         if ( newNodeMap.size() > 0 )
         {
            MsgRouteID nodeID ;
            ROUTE_RC_MAP::iterator itRC = newNodeMap.begin() ;
            while( itRC != newNodeMap.end() )
            {
               nodeID.value = itRC->first ;
               pPropSite->addTransNode( nodeID ) ;
               ++itRC ;
            }
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Build transaction failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      /// will rollback all suc node and the node before session
      goto done ;
   }

   /*
      _coordTransBegin implement
   */
   _coordTransBegin::_coordTransBegin()
   {
      const static string s_name( "TransBegin" ) ;
      setName( s_name ) ;
   }

   _coordTransBegin::~_coordTransBegin()
   {
   }

   INT32 _coordTransBegin::beginTrans( pmdEDUCB *cb, BOOLEAN isAutoCommit )
   {
      return _groupSession.getPropSite()->beginTrans( cb, isAutoCommit ) ;
   }

   INT32 _coordTransBegin::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      contextID   = -1 ;

      if ( cb->isAutoCommitTrans() )
      {
         rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
         PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                 rc ) ;
      }
      else
      {
         rc = beginTrans( cb ) ;
      }
      return rc ;
   }

   /*
      _coord2PhaseCommit implement
   */
   _coord2PhaseCommit::_coord2PhaseCommit()
   {
   }

   _coord2PhaseCommit::~_coord2PhaseCommit()
   {
   }

   INT32 _coord2PhaseCommit::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needCancel = FALSE ;

      contextID                        = -1 ;

      if ( !cb->isTransaction() )
      {
         goto done ;
      }

      needCancel = TRUE ;

      if ( canCompactCommit() )
      {
         rc = doCompactPhase( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute failed on compact phase in "
                    "operator[%s], rc: %d", getName(), rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = doPhase1( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute failed on phase1 in operator[%s], "
                    "rc: %d", getName(), rc ) ;
            goto error ;
         }

         needCancel = FALSE ;
         rc = doPhase2( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute failed on phase2 in operator[%s], "
                    "rc: %d", getName(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      if ( needCancel )
      {
         INT32 rcTmp = cancelOp( pMsg, cb, contextID, buf ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Execute cancel phase in operator[%s], rc: %d",
                    getName(), rcTmp ) ;
         }
      }
      goto done ;
   }

   INT32 _coord2PhaseCommit::doPhase1( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      CHAR *pMsgReq                    = NULL ;
      INT32 msgSize                    = 0 ;

      rc = buildPhase1Msg( (const CHAR*)pMsg, &pMsgReq, &msgSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed in operator[%s] phase1, "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      // execute on data nodes
      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on data group failed in operator[%s] "
                 "phase1, rc: %d", getName(), rc ) ;
         goto error ;
      }

      /// set trans status
      cb->setTransStatus( DPS_TRANS_WAIT_COMMIT ) ;

   done:
      if ( pMsgReq )
      {
         releasePhase1Msg( pMsgReq, msgSize, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coord2PhaseCommit::doPhase2( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsgReq                    = NULL ;
      INT32 msgSize                    = 0 ;

      rc = buildPhase2Msg( (const CHAR*)pMsg, &pMsgReq, &msgSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed in operator[%s] phase2, "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      // execute on data nodes
      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on data group failed in operator[%s] "
                 "phase2, rc: %d", getName(), rc ) ;
         goto error ;
      }

   done:
      if ( pMsgReq )
      {
         releasePhase2Msg( pMsgReq, msgSize, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coord2PhaseCommit::doCompactPhase( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsgReq = NULL ;
      INT32 msgSize = 0 ;

      rc = buildCompactMsg( ( const CHAR *)pMsg, &pMsgReq, &msgSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed in operator[%s] compact "
                 "phase, rc: %d", getName(), rc ) ;
         goto error ;
      }

      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on data group failed in operator[%s] "
                 "compact phase, rc: %d", getName(), rc ) ;
         goto error ;
      }

   done:
      if ( pMsgReq )
      {
         releaseCompactMsg( pMsgReq, msgSize, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coord2PhaseCommit::cancelOp( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      // do nothing, rollback will do in session
      return SDB_OK ;
   }

   /*
      _coordTransCommit implement
   */
   _coordTransCommit::_coordTransCommit()
   {
      const static string s_name( "TransCommit" ) ;
      setName( s_name ) ;
   }

   _coordTransCommit::~_coordTransCommit()
   {
   }

   INT32 _coordTransCommit::executeOnDataGroup( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                INT64 &contextID,
                                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itr ;

      ROUTE_RC_MAP nokRC ;
      _coordSessionPropSite::MAP_TRANS_NODES_CIT cit ;
      const _coordSessionPropSite::MAP_TRANS_NODES *pNodeMap = NULL ;

      pNodeMap = _groupSession.getPropSite()->getTransNodeMap() ;
      cit = pNodeMap->begin() ;

      /// clear
      _groupSession.resetSubSession() ;

      while( cit != pNodeMap->end() )
      {
         pSub = pSession->addSubSession( cit->second._nodeID.value ) ;
         pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

         rcTmp = pSession->sendMsg( pSub ) ;
         if ( rcTmp )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG ( PDWARNING, "Failed to send commit request to the "
                     "node[%s], rc: %d",
                     routeID2String( cit->second._nodeID ).c_str(),
                     rcTmp ) ;
            nokRC[ cit->second._nodeID.value ] = rcTmp ;
         }
         ++cit ;
      }

      rcTmp = pSession->waitReply1( TRUE ) ;
      if ( rcTmp )
      {
         rc = rc ? rc : rcTmp ;
         PD_LOG( PDERROR, "Failed to get the reply, rc: %d", rcTmp ) ;
      }

      itr = pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while ( itr.more() )
      {
         MsgOpReply *pReply = NULL ;
         pSub = itr.next() ;

         pReply = (MsgOpReply *)pSub->getRspMsg() ;
         rcTmp = pReply->flags ;

         if ( rcTmp )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG( PDERROR, "Data node[%s] commit transaction failed, rc: %d",
                    routeID2String( pReply->header.routeID ).c_str(),
                    rcTmp ) ;
            nokRC[ pReply->header.routeID.value ] = coordErrorInfo( pReply ) ;
         }
      }

      if ( rc )
      {
         goto error ;
      }
   done:
      _groupSession.resetSubSession() ;
      return rc ;
   error:
      if ( ( rc && nokRC.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 _coordTransCommit::buildPhase1Msg( const CHAR *pReceiveBuffer,
                                            CHAR **pMsg,
                                            INT32 *pMsgSize,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT32 msgLen = 0 ;
      MsgOpTransCommitPre *pCommitPreMsg = NULL ;
      UINT32 i = 0 ;
      UINT32 writeTransNodes = 0 ;

      _coordSessionPropSite::MAP_TRANS_NODES_CIT cit ;
      const _coordSessionPropSite::MAP_TRANS_NODES *pNodeMap = NULL ;

      /// dump trans nodes
      pNodeMap = _groupSession.getPropSite()->getTransNodeMap() ;
      writeTransNodes = _groupSession.getPropSite()->getWriteTransNodeSize() ;

      msgLen = sizeof( MsgOpTransCommitPre ) +
               writeTransNodes * sizeof( UINT64 ) ;

      pCommitPreMsg = ( MsgOpTransCommitPre* )SDB_THREAD_ALLOC( msgLen ) ;
      if ( !pCommitPreMsg )
      {
         PD_LOG( PDERROR, "Alloc memory failed(Size:%u)", msgLen ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      pCommitPreMsg->header.messageLength = msgLen ;
      pCommitPreMsg->header.opCode = MSG_BS_TRANS_COMMITPRE_REQ ;
      pCommitPreMsg->header.routeID.value = MSG_INVALID_ROUTEID ;
      pCommitPreMsg->header.requestID = 0 ;
      pCommitPreMsg->header.TID = cb->getTID() ;
      pCommitPreMsg->nodeNum = writeTransNodes ;

      /// set node id
      for ( cit = pNodeMap->begin() ; cit != pNodeMap->end() ; ++cit )
      {
         /// only use written nodes
         if ( cit->second._hasWritten )
         {
            pCommitPreMsg->nodes[ i++ ] = cit->second._nodeID.value ;
            if ( i >= writeTransNodes )
            {
               break ;
            }
         }
      }
      SDB_ASSERT( i == writeTransNodes, "Write transaction node is invalid" ) ;

      *pMsg = ( CHAR* )pCommitPreMsg ;
      *pMsgSize = msgLen ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordTransCommit::releasePhase1Msg( CHAR *pMsg,
                                             INT32 msgSize,
                                             pmdEDUCB *cb )
   {
      SDB_THREAD_FREE( pMsg ) ;
   }

   INT32 _coordTransCommit::buildPhase2Msg( const CHAR *pReceiveBuffer,
                                            CHAR **pMsg,
                                            INT32 *pMsgSize,
                                            pmdEDUCB *cb )
   {
      _phase2Msg.header.messageLength = sizeof( _phase2Msg ) ;
      _phase2Msg.header.opCode = MSG_BS_TRANS_COMMIT_REQ ;
      _phase2Msg.header.routeID.value = MSG_INVALID_ROUTEID ;
      _phase2Msg.header.requestID = 0 ;
      _phase2Msg.header.TID = cb->getTID() ;

      *pMsg = ( CHAR* )&_phase2Msg ;
      *pMsgSize = _phase2Msg.header.messageLength ;

      return SDB_OK ;
   }

   void _coordTransCommit::releasePhase2Msg( CHAR *pMsg,
                                             INT32 msgSize,
                                             pmdEDUCB *cb )
   {
   }

   INT32 _coordTransCommit::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      DPS_TRANS_ID curTransID = cb->getTransID() ;

      const CHAR *pHint = NULL ;
      rc = msgExtractTransCommit( (CHAR*)pMsg, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse commit request, rc: %d", rc ) ;
         goto error ;
      }

      if ( pHint )
      {
         try
         {
            BSONObj hint = BSONObj( pHint ) ;
            BSONObj clientInfo ;

            if ( cb->getMonQueryCB() && !hint.getField("$"FIELD_NAME_CLIENTINFO).eoo() )
            {
               rc = rtnGetObjElement( hint, "$"FIELD_NAME_CLIENTINFO, clientInfo ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                            "$"FIELD_NAME_CLIENTINFO, rc ) ;
               cb->getMonQueryCB()->clientInfo = clientInfo.getOwned() ;
            }
         }
         catch ( std::exception &e )
         {
            PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                         "Commit failed, received unexpected error: %s",
                         e.what() ) ;
         }
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          curTransID, curTransID ) ;

      rc = _coord2PhaseCommit::execute( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

      // complete, delete transaction
      _groupSession.getPropSite()->endTrans( cb ) ;

      PD_LOG( PDINFO, "Execute commit(ID:%s, IDAttr:%s)",
              dpsTransIDToString( curTransID ).c_str(),
              dpsTransIDAttrToString( curTransID ).c_str() ) ;

   done:
      return rc ;
   error:
      // rollback in session
      goto done ;
   }

   BOOLEAN _coordTransCommit::canCompactCommit()
   {
      if ( 1 == _groupSession.getPropSite()->getTransNodeSize() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _coordTransCommit::buildCompactMsg( const CHAR *pReceiveBuffer,
                                             CHAR **pMsg,
                                             INT32 *pMsgSize,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuf1 = NULL ;
      INT32 bufSize1 = 0 ;
      CHAR *pBuf2 = NULL ;
      INT32 bufSize2 = 0 ;

      pmdSubSession dummySession ;

      rc = buildPhase1Msg( pReceiveBuffer, &pBuf1, &bufSize1, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = buildPhase2Msg( pReceiveBuffer, &pBuf2, &bufSize2, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      dummySession.setReqMsg( ( MsgHeader* )pBuf2, PMD_EDU_MEM_NONE ) ;

      rc = coordBuildPacketMsg( &dummySession, ( MsgHeader* )pBuf1, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      *pMsg = ( CHAR* )dummySession.getReqMsg() ;
      SDB_ASSERT( PMD_EDU_MEM_SELF == dummySession.getReqMemType(),
                  "Invalid mem type" ) ;
      dummySession.setReqMsgMemType( PMD_EDU_MEM_NONE ) ;

   done:
      if ( pBuf1 )
      {
         releasePhase1Msg( pBuf1, bufSize1, cb ) ;
      }
      if ( pBuf2 )
      {
         releasePhase2Msg( pBuf2, bufSize2, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _coordTransCommit::releaseCompactMsg( CHAR *pMsg,
                                              INT32 msgSize,
                                              pmdEDUCB *cb )
   {
      if ( pMsg )
      {
         cb->releaseBuff( pMsg ) ;
      }
   }

   BOOLEAN _coordTransCommit::needRollback() const
   {
      return TRUE ;
   }

   /*
      _coordTransRollback implement
   */
   _coordTransRollback::_coordTransRollback()
   {
      const static string s_name( "TransRollback" ) ;
      setName( s_name ) ;
   }

   _coordTransRollback::~_coordTransRollback()
   {
   }

   INT32 _coordTransRollback::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;

      contextID                        = -1 ;

      if ( !cb->isTransaction() )
      {
         goto done ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_ROLLBACK_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          cb->getTransID(),
                          cb->getTransID() ) ;

      rc = _coordTransOperator::rollback( cb ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Rollback transaction failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done;
   }

   void _coordTransRollback::_prepareForTrans( pmdEDUCB *cb,
                                               MsgHeader *pMsg )
   {
   }

   BOOLEAN _coordTransRollback::needRollback() const
   {
      return FALSE ;
   }

}

