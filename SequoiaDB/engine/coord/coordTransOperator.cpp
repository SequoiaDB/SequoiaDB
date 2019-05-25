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
#include "msgMessageFormat.hpp"
#include "coordUtil.hpp"
#include "rtnCommandDef.hpp"
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
      ROUTE_RC_MAP newNodeMap ;

      rc = buildTransSession( options._groupLst, cb, newNodeMap ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build transaction session on data node, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      if ( cb->isTransaction() )
      {
         _prepareForTrans( cb, inMsg.msg() ) ;
      }

      rc = coordOperator::doOnGroups( inMsg, options, cb, result ) ;
      if ( cb->isTransaction() && ( rc || result.nokSize() > 0 ) )
      {
         SET_NODEID nodes ;
         MsgRouteID nodeID ;
         ROUTE_RC_MAP::iterator itRCMap = newNodeMap.begin() ;
         while( itRCMap != newNodeMap.end() )
         {
            nodeID.value = itRCMap->first ;
            if ( result._sucGroupLst.find( nodeID.columns.groupID ) ==
                 result._sucGroupLst.end() )
            {
               nodes.insert( nodeID.value ) ;
            }
            ++itRCMap ;
         }
         releaseTransSession( nodes, cb ) ;
      }

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

         cb->delTransNode( pSub->getNodeID() ) ;

         rc = pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Release node[%s] failed, rc: %d",
                    routeID2String( *itSet ).c_str(), rc ) ;
            pSession->resetSubSession( *itSet ) ;
         }
         ++itSet ;
      }

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
         MsgOpReply *pReply = ( MsgOpReply* )pSub->getRspMsg( FALSE ) ;

         if ( pReply->flags )
         {
            PD_LOG( PDWARNING, "Release node[%s]'s transaction failed, "
                    "rc: %d", routeID2String( pReply->header.routeID ).c_str(),
                    pReply->flags ) ;
         }
      }

      pSession->resetAllSubSession() ;

      return rc ;
   }

   INT32 _coordTransOperator::rollback( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SET_NODEID nodes ;
      DpsTransNodeMap *pTransMap = cb->getTransNodeLst() ;
      DpsTransNodeMap::iterator it ;

      if ( NULL == pTransMap )
      {
         goto done ;
      }

      it = pTransMap->begin() ;
      while( it != pTransMap->end() )
      {
         nodes.insert( it->second.value ) ;
         ++it ;
      }

      cb->startRollback() ;

      rc = releaseTransSession( nodes, cb ) ;

   done:
      cb->delTransaction() ;
      cb->stopRollback() ;
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

         DpsTransNodeMap *pTransNodeLst = cb->getTransNodeLst() ;
         DpsTransNodeMap::iterator iterTrans ;
         CoordGroupList::const_iterator iterGroup ;
         MsgOpTransBegin msgReq ;
         coordSendMsgIn inMsg( (MsgHeader*)&msgReq ) ;

         msgReq.header.messageLength = sizeof( MsgOpTransBegin ) ;
         msgReq.header.opCode = MSG_BS_TRANS_BEGIN_REQ ;
         msgReq.header.routeID.value = 0 ;
         msgReq.header.TID = cb->getTID() ;

         iterGroup = groupLst.begin() ;
         while(  iterGroup != groupLst.end() )
         {
            iterTrans = pTransNodeLst->find( iterGroup->first );
            if ( pTransNodeLst->end() == iterTrans )
            {
               options._groupLst[ iterGroup->first ] = iterGroup->second ;
            }
            ++iterGroup ;
         }

         newNodeMap.clear() ;
         result._pOkRC = &newNodeMap ;
         rc = coordOperator::doOnGroups( inMsg, options, cb, result ) ;
         if ( newNodeMap.size() > 0 )
         {
            MsgRouteID nodeID ;
            ROUTE_RC_MAP::iterator itRC = newNodeMap.begin() ;
            while( itRC != newNodeMap.end() )
            {
               nodeID.value = itRC->first ;
               cb->addTransNode( nodeID ) ;
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

   INT32 _coordTransBegin::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc    = SDB_OK ;
      contextID   = -1 ;

      rc = cb->createTransaction() ;
      PD_RC_CHECK( rc, PDERROR, "Create transaction failed, rc: %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
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
      rc = doPhase1( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute failed on phase1 in operator[%s], rc: %d",
                 getName(), rc ) ;
         goto error ;
      }

      needCancel = FALSE ;
      rc = doPhase2( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute failed on phase2 in operator[%s], rc: %d",
                 getName(), rc ) ;
         goto error ;
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

      rc = executeOnDataGroup( (MsgHeader*)pMsgReq, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on data group failed in operator[%s] "
                 "phase1, rc: %d", getName(), rc ) ;
         goto error ;
      }

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

   INT32 _coord2PhaseCommit::cancelOp( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
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

      DpsTransNodeMap *pNodeMap = cb->getTransNodeLst() ;
      DpsTransNodeMap::iterator iterMap = pNodeMap->begin() ;
      ROUTE_RC_MAP nokRC ;

      _groupSession.resetSubSession() ;

      while( iterMap != pNodeMap->end() )
      {
         pSub = pSession->addSubSession( iterMap->second.value ) ;
         pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

         rcTmp = pSession->sendMsg( pSub ) ;
         if ( rcTmp )
         {
            rc = rc ? rc : rcTmp ;
            PD_LOG ( PDWARNING, "Failed to send commit request to the "
                     "node[%s], rc: %d",
                     routeID2String( iterMap->second ).c_str(),
                     rcTmp ) ;
            nokRC[ iterMap->second.value ] = rcTmp ;
         }
         ++iterMap ;
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

         pReply = (MsgOpReply *)pSub->getRspMsg( FALSE ) ;
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
      _phase1Msg.header.messageLength = sizeof( _phase1Msg ) ;
      _phase1Msg.header.opCode = MSG_BS_TRANS_COMMITPRE_REQ ;
      _phase1Msg.header.routeID.value = MSG_INVALID_ROUTEID ;
      _phase1Msg.header.requestID = 0 ;
      _phase1Msg.header.TID = cb->getTID() ;

      *pMsg = ( CHAR* )&_phase1Msg ;
      *pMsgSize = _phase1Msg.header.messageLength ;

      return SDB_OK ;
   }

   void _coordTransCommit::releasePhase1Msg( CHAR *pMsg,
                                             INT32 msgSize,
                                             pmdEDUCB *cb )
   {
   }

   INT32 _coordTransCommit::buildPhase2Msg( const CHAR *pReceiveBuffer,
                                            CHAR **pMsg,
                                            INT32 *pMsgSize,
                                            pmdEDUCB *cb )
   {
      _phase2Msg.header.messageLength = sizeof( _phase1Msg ) ;
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

      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), MSG_BS_TRANS_COMMIT_REQ,
                          "TransactionID: 0x%016x(%llu)",
                          cb->getTransID(),
                          cb->getTransID() ) ;

      rc = _coord2PhaseCommit::execute( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

      cb->delTransaction() ;

   done:
      return rc ;
   error:
      goto done ;
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

