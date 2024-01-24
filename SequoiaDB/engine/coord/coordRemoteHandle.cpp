/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = coordRemoteHandle.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/17/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordRemoteHandle.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "msgMessageFormat.hpp"
#include "schedDef.hpp"
#include "coordCB.hpp"
#include "clsResourceContainer.hpp"
#include "coordResource.hpp"
#include "msgMessage.hpp"
#include "coordRemoteSession.hpp"
#include "../bson/bson.h"
#include "IDataSource.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_EXPIRED_KILLCONTEXT_TIMEOUT       ( 30000 )      /// 30s

   /*
      Tool functions
   */
   INT32 coordBuildPacketMsg( _pmdSubSession *pSub,
                              MsgHeader *pHeader,
                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT32 totalLen = 0 ;
      UINT32 pos = 0 ;
      MsgHeader *pOldHeader = pSub->getReqMsg() ;
      CHAR *pBuff = NULL ;

      if ( pSub->getIODatas()->size() > 0 )
      {
         pOldHeader->messageLength = sizeof( MsgHeader ) +
                                     pSub->getIODataLen() ;
      }

      totalLen = pHeader->messageLength + pOldHeader->messageLength ;

      if ( MSG_PACKET != pOldHeader->opCode )
      {
         totalLen += sizeof( MsgHeader ) ;
      }

      rc = cb->allocBuff( totalLen, &pBuff, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Allocate memory[%u] failed, rc: %d",
                 totalLen, rc ) ;
         goto error ;
      }
      else
      {
         MsgHeader *pMsgPacket = NULL ;
         /// packet
         pMsgPacket = ( MsgHeader* )( pBuff + pos ) ;
         pos += sizeof( MsgHeader ) ;

         /// new add
         ossMemcpy( pBuff + pos, (void*)pHeader, pHeader->messageLength ) ;
         MsgHeader *pNewAdd = ( MsgHeader* )( pBuff + pos ) ;
         pNewAdd->requestID = pOldHeader->requestID ;
         pNewAdd->routeID.value = pOldHeader->routeID.value ;
         pNewAdd->TID = pOldHeader->TID ;
         pNewAdd->globalID = pOldHeader->globalID ;
         pos += pHeader->messageLength ;

         if ( MSG_PACKET == pOldHeader->opCode )
         {
            ossMemcpy( (void*)pMsgPacket, (void*)pOldHeader,
                       sizeof( MsgHeader ) ) ;
         }
         else
         {
            pMsgPacket->opCode = MSG_PACKET ;
            pMsgPacket->requestID = pOldHeader->requestID ;
            pMsgPacket->routeID.value = pOldHeader->routeID.value ;
            pMsgPacket->TID = pOldHeader->TID ;
            pMsgPacket->globalID = pOldHeader->globalID ;
         }
         pMsgPacket->messageLength = totalLen ;

         /// old
         if ( pSub->getIODatas()->size() > 0 )
         {
            if ( MSG_PACKET != pOldHeader->opCode )
            {
               ossMemcpy( pBuff + pos, (void*)pOldHeader,
                          sizeof( MsgHeader ) ) ;
               pos += sizeof( MsgHeader ) ;
            }

            netIOVec *pIOVec = pSub->getIODatas() ;
            for ( UINT32 i = 0 ; i < pIOVec->size() ; ++i )
            {
               netIOV &ioItem = (*pIOVec)[i] ;
               ossMemcpy( pBuff + pos, ioItem.iovBase, ioItem.iovLen ) ;
               pos += ioItem.iovLen ;
            }
         }
         else
         {
            CHAR *pCopyData = ( CHAR* )pOldHeader ;
            UINT32 copyLen = pOldHeader->messageLength ;

            if ( MSG_PACKET == pOldHeader->opCode )
            {
               pCopyData += sizeof( MsgHeader ) ;
               copyLen -= sizeof( MsgHeader ) ;
            }
            ossMemcpy( pBuff + pos, pCopyData,copyLen ) ;
            pos += copyLen ;
         }

         /// set sub session
         pSub->clearIODatas() ;
         pSub->setReqMsg( (MsgHeader*)pBuff, PMD_EDU_MEM_SELF ) ;
         pBuff = NULL ;
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 coordBuildPacketMsg( _pmdRemoteSession *pSession,
                              _pmdSubSession *pSub,
                              MsgHeader *pHeader )
   {
      return coordBuildPacketMsg( pSub, pHeader, pSession->getEDUCB() ) ;
   }

   /*
      _coordRemoteHandlerBase implement
   */
   _coordRemoteHandlerBase::_coordRemoteHandlerBase()
   {
      _initType      = INIT_V1 ;

      coordRemoteHandleStatus tmp ;
      SDB_UNUSED( tmp ) ;
   }

   _coordRemoteHandlerBase::~_coordRemoteHandlerBase()
   {
   }

   BOOLEAN _coordRemoteHandlerBase::isVersion0() const
   {
      return ( INIT_V0 == _initType ) ? TRUE : FALSE ;
   }

   INT32 _coordRemoteHandlerBase::onSendFailed( _pmdRemoteSession *pSession,
                                                _pmdSubSession **ppSub,
                                                INT32 flag )
   {
      if ( pSession && ppSub && *ppSub )
      {
         pmdEDUCB *cb = pSession->getEDUCB() ;
         pmdRemoteSessionSite *pSite = NULL ;
         coordSessionPropSite *pPropSite = NULL ;
         coordRemoteHandleStatus *pStatus = NULL ;

         pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
         pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;

         pStatus = ( coordRemoteHandleStatus* )(*ppSub)->getUDFData() ;
         if ( pStatus->_initTrans )
         {
            pPropSite->delTransNode( (*ppSub)->getNodeID() ) ;
         }
      }

      return flag ;
   }

   void _coordRemoteHandlerBase::onReply( _pmdRemoteSession *pSession,
                                          _pmdSubSession **ppSub,
                                          const MsgHeader *pReply,
                                          BOOLEAN isPending )
   {
      pmdEDUCB *cb = pSession->getEDUCB() ;
      coordRemoteHandleStatus *pStatus = NULL ;
      pStatus = ( coordRemoteHandleStatus* )(*ppSub)->getUDFData() ;

      if ( !pStatus->_initFinished || pStatus->_initTrans ||
           0 != pStatus->_nodeID )
      {
         MsgOpReply *pOpReply = ( MsgOpReply* )pReply ;
         INT32 orgRspOpCode = (*ppSub)->getOrgRspOpCode() ;
         pmdRemoteSessionSite *pSite = NULL ;
         coordSessionPropSite *pPropSite = NULL ;
         pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
         pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;

         if ( SDB_INVALID_ROUTEID == pOpReply->flags )
         {
            pSession->reConnectSubSession( (*ppSub)->getNodeID().value ) ;
            if ( pStatus->_initTrans )
            {
               pPropSite->delTransNode( (*ppSub)->getNodeID() ) ;
            }
         }
         else if ( SDB_OK != pOpReply->flags &&
                   ( MSG_COM_SESSION_INIT_RSP == orgRspOpCode ||
                     MSG_PACKET_RES == orgRspOpCode ||
                     MSG_BS_TRANS_BEGIN_RSP == orgRspOpCode ) )
         {
            if ( MSG_COM_SESSION_INIT_RSP == orgRspOpCode ||
                 MSG_PACKET_RES == orgRspOpCode )
            {
               pSession->reConnectSubSession( pReply->routeID.value ) ;
            }
            if ( pStatus->_initTrans )
            {
               pPropSite->delTransNode( pReply->routeID ) ;
            }
         }
         else if ( 0 != pStatus->_nodeID )
         {
            /// update node version
            pSite->setNodeVer( pStatus->_nodeID, pStatus->_nodeVer ) ;
         }

         pStatus->init() ;
      }

      /// When in transaction, and data node has rollbacked,
      /// so need to set transaction result
      MsgOpReply *pOpReply = ( MsgOpReply* )pReply ;
      if ( cb->isTransaction() &&
           pReply->routeID.columns.groupID >= DATA_GROUP_ID_BEGIN &&
           pReply->routeID.columns.groupID <= DATA_GROUP_ID_END &&
           SDB_OK != pOpReply->flags &&
           SDB_DMS_EOC != pOpReply->flags &&
           SDB_CLS_COORD_NODE_CAT_VER_OLD != pOpReply->flags )
      {
         MsgRouteID routeID = pReply->routeID ;
         INT32 rcTmp = pOpReply->flags ;

         if ( SDB_DPS_TRANS_NO_TRANS == rcTmp )
         {
            cb->setTransRC( SDB_DPS_TRANS_NO_TRANS ) ;
         }
         else if ( pReply->messageLength > (INT32)sizeof( MsgOpReply ) )
         {
            try
            {
               BSONObj errObj( ( const CHAR* )pReply + sizeof( MsgOpReply )  ) ;
               BSONElement e ;

               // check transaction error
               e = errObj.getField( FIELD_NAME_TRANS_RC ) ;
               if ( NumberInt == e.type() )
               {
                  INT32 tempRC = e.numberInt() ;
                  if ( SDB_OK != tempRC )
                  {
                     PD_LOG( PDWARNING, "Remote node[%s] got transaction "
                             "error [%d] in session[%s]",
                             routeID2String( routeID ).c_str(), tempRC,
                             cb->toString().c_str() ) ;
                     if ( SDB_OK == cb->getTransRC() )
                     {
                        cb->setTransRC( tempRC ) ;
                     }
                  }
               }

               // check transaction rollback
               e = errObj.getField( FIELD_NAME_ROLLBACK ) ;
               if ( e.isBoolean() && e.boolean() )
               {
                  PD_LOG( PDWARNING, "Remote node[%s] has rollbacked "
                          "transaction due to error[%d] in session[%s]",
                          routeID2String( routeID ).c_str(), rcTmp,
                          cb->toString().c_str() ) ;
                  if ( SDB_OK == cb->getTransRC() )
                  {
                     cb->setTransRC( rcTmp ) ;
                  }
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
               /// ignored
            }
         }
      }
   }

   INT32 _coordRemoteHandlerBase::onExpiredReply ( pmdRemoteSessionSite *pSite,
                                                   const MsgHeader *pReply )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *pOpReply = NULL ;
      pmdRemoteSession *pSession = NULL ;
      pmdSubSession *pSub = NULL ;
      MsgOpKillContexts msgKillContext ;

      if ( NULL == pReply || !IS_REPLY_TYPE( pReply->opCode ) )
      {
         goto done ;
      }

      pOpReply = (MsgOpReply *)pReply ;

      PD_LOG( PDWARNING,
              "Received expired msg[context:%lld, opCode:(1)%d] from node[%s]",
              pOpReply->contextID, GET_REQUEST_TYPE(pOpReply->header.opCode),
              routeID2String( pReply->routeID ).c_str() ) ;

      if ( -1 == pOpReply->contextID )
      {
         goto done ;
      }
      /// The contextID of insert/delete/update message is NOT REAL context.
      if ( MSG_BS_INSERT_RES == pOpReply->header.opCode ||
           MSG_BS_DELETE_RES == pOpReply->header.opCode ||
           MSG_BS_UPDATE_RES == pOpReply->header.opCode ||
           MSG_BS_TRANS_INSERT_RSP == pOpReply->header.opCode ||
           MSG_BS_TRANS_DELETE_RSP == pOpReply->header.opCode ||
           MSG_BS_TRANS_UPDATE_RSP == pOpReply->header.opCode )
      {
         goto done ;
      }

      pSession = pSite->addSession( COORD_EXPIRED_KILLCONTEXT_TIMEOUT ) ;
      pSub = pSession->addSubSession( pReply->routeID.value ) ;

      /// send kill context
      msgKillContext.contextIDs[ 0 ] = pOpReply->contextID ;
      msgKillContext.numContexts = 1 ;
      msgKillContext.ZERO = 0 ;
      msgKillContext.header.messageLength = sizeof( MsgOpKillContexts ) ;
      msgKillContext.header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
      msgKillContext.header.requestID = 0 ;
      msgKillContext.header.routeID.value = 0 ;
      msgKillContext.header.TID = 0 ;

      /// Ignore sendMsg failed
      rc = pSession->sendMsg( (MsgHeader*)&msgKillContext,
                              PMD_EDU_MEM_NONE ) ;
      // DON'T wait for reply, otherwise Timeout of sessionAttr may fail. Reset
      // sub session when ignoring the reply, otherwise interrupt message will
      // be sent to data node, the operation of data node may be interrupted.
      pSub->resetForResend() ;

   done:
      if ( pSession )
      {
         pSite->removeSession( pSession ) ;
      }
      return rc ;
   }

   INT32 _coordRemoteHandlerBase::_onSendConnectOld( _pmdSubSession *pSub )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = NULL ;
      pmdRemoteSessionSite* pSite = NULL ;
      pmdRemoteSession *pInitSession = NULL ;

      cb = pSub->parent()->getEDUCB() ;
      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;

      pInitSession = pSite->addSession( -1, NULL ) ;
      if ( !pInitSession )
      {
         PD_LOG( PDERROR, "Create init remote session failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _sessionInit( pInitSession, pSub->getNodeID(), cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( pInitSession )
      {
         pSite->removeSession( pInitSession ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /**
    * Actions before create a connection with a remote node. For nodes in the
    * same cluster, sesion init is done. For a node of data source, it's not
    * needed.
    */
   INT32 _coordRemoteHandlerBase::onSendConnect( _pmdSubSession *pSub,
                                                 const MsgHeader *pReq,
                                                 BOOLEAN isFirst )
   {
      INT32 rc = SDB_OK ;
      coordRemoteHandleStatus *pStatus = NULL ;
      pStatus = ( coordRemoteHandleStatus* )pSub->getUDFData() ;

      if ( pSub->sessionInitRequired() )
      {
         if ( INIT_V0 == _initType || isNoReplyMsg( pReq->opCode ) )
         {
            rc = _onSendConnectOld( pSub ) ;
         }
         else
         {
            rc = _buildPacketWithSessionInit( pSub->parent(), pSub, FALSE ) ;
            if ( SDB_OK == rc )
            {
               pStatus->_initFinished = FALSE ;
            }
         }
      }
      return rc ;
   }

   INT32 _coordRemoteHandlerBase::_buildPacketWithSessionInit( _pmdRemoteSession *pSession,
                                                               _pmdSubSession *pSub,
                                                               BOOLEAN isUpdate )
   {
      INT32 rc = SDB_OK ;
      BSONObj objInfo ;
      CHAR *pBuff = NULL ;
      MsgComSessionInitReq *pInitReq = NULL ;
      UINT32 msgLength = sizeof( MsgComSessionInitReq ) ;
      pmdEDUCB *cb = pSession->getEDUCB() ;

      /// construct info
      try
      {
         objInfo = _buildSessionInitObj( cb ) ;
         msgLength += objInfo.objsize() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// allocate memory
      rc = cb->allocBuff( msgLength, &pBuff, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc memory failed, size: %u, rc: %d",
                 msgLength, rc ) ;
         goto error ;
      }
      pInitReq = (MsgComSessionInitReq*)pBuff ;

      /// init message
      pInitReq->header.messageLength = msgLength ;
      pInitReq->header.opCode = MSG_COM_SESSION_INIT_REQ ;
      pInitReq->header.requestID = 0 ;
      pInitReq->header.routeID.value = 0 ;
      pInitReq->header.TID = cb->getTID() ;
      pInitReq->dstRouteID.value = pSub->getNodeIDUInt() ;
      pInitReq->srcRouteID.value = pmdGetNodeID().value ;
      pInitReq->localIP = _netFrame::getLocalAddress() ;
      pInitReq->peerIP = 0 ;
      pInitReq->localPort = pmdGetLocalPort() ;
      pInitReq->peerPort = 0 ;
      pInitReq->localTID = cb->getTID() ;
      pInitReq->localSessionID = cb->getID() ;
      pInitReq->isUpdate = isUpdate ? 1 : 0 ;
      ossMemset( pInitReq->reserved, 0, sizeof( pInitReq->reserved ) ) ;
      ossMemcpy( pInitReq->data, objInfo.objdata(), objInfo.objsize() ) ;

      rc = coordBuildPacketMsg( pSession, pSub, ( MsgHeader*)pBuff ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
         pBuff = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   BSONObj _coordRemoteHandlerBase::_buildSessionInitObj( _pmdEDUCB *cb )
   {
      const CHAR *pRemoteIP = "" ;
      UINT16 remotePort = 0 ;
      UINT32 mask = 0 ;
      UINT32 configMask = 0 ;
      BSONObjBuilder builder( 256 ) ;

      if ( cb->getSession() )
      {
         IClient *pClient = cb->getSession()->getClient() ;
         if ( pClient )
         {
            pRemoteIP = pClient->getFromIPAddr() ;
            remotePort = pClient->getFromPort() ;
         }
      }

      builder.append( FIELD_NAME_SOURCE, cb->getSource() ) ;
      builder.append( SDB_AUTH_USER, cb->getUserName() ) ;
      builder.append( SDB_AUTH_PASSWD, cb->getPassword() ) ;
      builder.append( FIELD_NAME_HOST, pmdGetKRCB()->getHostName() ) ;
      builder.append( PMD_OPTION_SVCNAME, pmdGetOptionCB()->getServiceAddr() ) ;
      builder.append( FIELD_NAME_REMOTE_IP, pRemoteIP ) ;
      builder.append( FIELD_NAME_REMOTE_PORT, (INT32)remotePort ) ;

      /// audit mask
      pdGetCurAuditMask( AUDIT_LEVEL_USER, mask, configMask ) ;
      builder.append( FIELD_NAME_AUDIT_MASK, (INT32)mask ) ;
      builder.append( FIELD_NAME_AUDIT_CONFIG_MASK, (INT32)configMask ) ;

      /// trans info
      cb->getTransExecutor()->toBson( builder ) ;

      return builder.obj() ;
   }

   INT32 _coordRemoteHandlerBase::_sessionInit( _pmdRemoteSession *pSession,
                                                const MsgRouteID &nodeID,
                                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdSubSession *pSub = NULL ;
      MsgOpReply *pReply = NULL ;

      BSONObj objInfo ;
      CHAR *pBuff = NULL ;
      MsgComSessionInitReq *pInitReq = NULL ;
      UINT32 msgLength = sizeof( MsgComSessionInitReq ) ;

      /// construct info
      try
      {
         objInfo = _buildSessionInitObj( cb ) ;
         msgLength += objInfo.objsize() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// allocate memory
      rc = cb->allocBuff( msgLength, &pBuff, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc memory failed, size: %u, rc: %d",
                 msgLength, rc ) ;
         goto error ;
      }
      pInitReq = (MsgComSessionInitReq*)pBuff ;

      /// init message
      pInitReq->header.messageLength = msgLength ;
      pInitReq->header.opCode = MSG_COM_SESSION_INIT_REQ ;
      pInitReq->header.requestID = 0 ;
      pInitReq->header.routeID.value = 0 ;
      pInitReq->header.TID = cb->getTID() ;
      pInitReq->dstRouteID.value = nodeID.value ;
      pInitReq->srcRouteID.value = pmdGetNodeID().value ;
      pInitReq->localIP = _netFrame::getLocalAddress() ;
      pInitReq->peerIP = 0 ;
      pInitReq->localPort = pmdGetLocalPort() ;
      pInitReq->peerPort = 0 ;
      pInitReq->localTID = cb->getTID() ;
      pInitReq->localSessionID = cb->getID() ;
      ossMemset( pInitReq->reserved, 0, sizeof( pInitReq->reserved ) ) ;
      ossMemcpy( pInitReq->data, objInfo.objdata(), objInfo.objsize() ) ;

      /// send message to peer
      pSub = pSession->addSubSession( nodeID.value ) ;
      pSub->setReqMsg( ( MsgHeader* )pInitReq, PMD_EDU_MEM_NONE ) ;

      rc = pSession->sendMsg( pSub ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Send session init request to node[%s] failed, "
                 "rc: %d", routeID2String( nodeID ).c_str(), rc ) ;
         goto error ;
      }

      /// get reply
      rc = pSession->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait session init response from node[%s] failed, "
                 "rc: %d", routeID2String( nodeID ).c_str(), rc ) ;
         goto error ;
      }

      /// process result
      pReply = ( MsgOpReply* )pSub->getRspMsg() ;
      if ( !pReply )
      {
         PD_LOG( PDERROR, "Session init reply message is null in node[%s]",
                 routeID2String( nodeID ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      rc = pReply->flags ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Session init with node[%s] failed, rc: %d",
                 routeID2String( nodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
         pBuff = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordRemoteHandlerBase::_checkSessionTransaction( _pmdRemoteSession *pSession,
                                                            _pmdSubSession *pSub,
                                                            _pmdEDUCB *cb,
                                                            coordRemoteHandleStatus *pStatus )
   {
      INT32 rc = SDB_OK ;
      pmdRemoteSessionSite *pSite = NULL ;
      coordSessionPropSite *pPropSite = NULL ;

      // Transaction is not supported on data source.
      if ( SDB_IS_DSID( pSub->getNodeID().columns.groupID ) )
      {
         goto done ;
      }

      if ( cb->isTransaction() && isTransBSMsg( pSub->getOrgReqOpCode() ) )
      {
         BOOLEAN isWriteMsg = isTransWriteMsg( pSub->getOrgReqOpCode(),
                                               pSub->getReqMsg() ) ;
         pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
         pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;

         // Check if the target node is already in the transaction node list. If
         // it's not, it means the transaction has not operated on that node by
         // far. In this case, a transaction begin message shoud be packed
         // together with the current message and sent to that node together.
         if ( !pPropSite->checkAndUpdateNode( pSub->getNodeID(), isWriteMsg ) )
         {
            MsgOpTransBegin msgReq ;
            msgReq.header.messageLength = sizeof( MsgOpTransBegin ) ;
            msgReq.header.opCode = MSG_BS_TRANS_BEGIN_REQ ;
            msgReq.header.routeID.value = 0 ;
            msgReq.header.TID = cb->getTID() ;
            msgReq.transID = DPS_TRANS_GET_ID( cb->getTransID() ) ;
            ossMemset( msgReq.reserved, 0, sizeof( msgReq.reserved ) ) ;

            rc = coordBuildPacketMsg( pSession,pSub, &msgReq.header ) ;
            if ( rc )
            {
               goto error ;
            }
            pStatus->_initTrans = TRUE ;
            pPropSite->addTransNode( pSub->getNodeID(), isWriteMsg ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordRemoteHandlerBase::_checkSessionSchedInfo( _pmdRemoteSession *pSession,
                                                          _pmdSubSession *pSub,
                                                          _pmdEDUCB *cb,
                                                          UINT32 &nodeSiteVer )
   {
      INT32 rc = SDB_OK ;
      coordResource *pResource = sdbGetResourceContainer()->getResource() ;
      schedItem *pItem = NULL ;
      schedInfo *pInfo = NULL ;

      if ( SDB_IS_DSID( pSub->getNodeID().columns.groupID ) )
      {
         goto done ;
      }

      pItem = (schedItem*)cb->getSession()->getSchedItemPtr() ;
      if ( !pItem )
      {
         goto done ;
      }
      pInfo = &(pItem->_info) ;

      if ( pResource->getOmGroupInfo()->nodeCount() > 0 &&
           ( SCHED_UNKNWON_VERSION == pInfo->getVersion() ||
             pInfo->isNew() ) )
      {
         /// update task info
         coordOmStrategyAgent *pAgent = pResource->getOmStrategyAgent() ;
         omTaskStrategyInfoPtr ptr ;

         rc = pAgent->getTaskStrategy( pInfo->getTaskName(),
                                       pInfo->getUserName(),
                                       pInfo->getIP(),
                                       ptr,
                                       FALSE ) ;
         if ( SDB_OK == rc )
         {
            pInfo->fromBSON( ptr->toBSON( OM_STRATEGY_MASK_BASEINFO ), FALSE ) ;

            if ( pItem->_ptr.get() &&
                 ( pItem->_ptr->getTaskID() != (UINT64)pInfo->getTaskID() ||
                   0 != ossStrcmp( pItem->_ptr->getTaskName(),
                                   pInfo->getTaskName() ) ) )
            {
               schedTaskMgr *pSvcTaskMgr = pmdGetKRCB()->getSvcTaskMgr() ;
               /// update task info
               pItem->_ptr = pSvcTaskMgr->getTaskInfoPtr(
                                                pItem->_info.getTaskID(),
                                                pItem->_info.getTaskName() ) ;
               /// update monApp's info
               cb->getMonAppCB()->setSvcTaskInfo( pItem->_ptr.get() ) ;
            }
         }
      }

      if ( pInfo->getVersion() != (INT32)nodeSiteVer )
      {
         /// rebuild message
         rc = _buildPacketWithUpdateSched( pSession, pSub, pInfo->toBSON() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build packet message with update-sched failed, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         /// update version
         nodeSiteVer = pInfo->getVersion() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordRemoteHandlerBase::_checkSessionAttr( _pmdRemoteSession *pSession,
                                                     _pmdSubSession *pSub,
                                                     _pmdEDUCB *cb,
                                                     UINT32 &nodeSiteVer )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_IS_DSID( pSub->getNodeID().columns.groupID ) )
      {
         rc = _checkDSSessionAttr( pSub, cb, nodeSiteVer ) ;
         PD_RC_CHECK( rc, PDERROR, "Check data source session attribute "
                      "failed[%d]", rc ) ;
      }
      else
      {
         UINT32 curAuditVersion = pdGetCurAuditVersion() ;
         UINT32 curTransVer = cb->getTransExecutor()->getTransConfVer() ;
         UINT32 curVersion = curAuditVersion + curTransVer ;

         if ( 0 != curVersion && curVersion != nodeSiteVer )
         {
            /// when net handle is invalid, the info will
            /// stored in session-init message
            if ( NET_INVALID_HANDLE != pSub->getHandle() )
            {
               rc = _buildPacketWithSessionInit( pSession, pSub, TRUE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Build packet message with session-update "
                          "failed, rc: %d", rc ) ;
                  goto error ;
               }
            }

            /// update version
            nodeSiteVer = curVersion ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordRemoteHandlerBase::_checkDSSessionAttr( _pmdSubSession *pSub,
                                                       _pmdEDUCB *cb,
                                                       UINT32 &nodeSiteVer )
   {
      INT32 rc = SDB_OK ;
      const rtnSessionProperty *property = (rtnSessionProperty *)
         ((pmdRemoteSessionSite *)cb->getRemoteSite())->getUserData() ;
      UINT32 curVersion = property->getVersion() ;

      if ( nodeSiteVer != curVersion )
      {
         CoordDataSourcePtr dsPtr ;
         UINT32 groupID = pSub->getNodeID().columns.groupID ;
         CoordCB *coordCB = pmdGetKRCB()->getCoordCB() ;
         coordDataSourceMgr *dsMgr = coordCB->getDSManager() ;
         rc = dsMgr->getOrUpdateDataSource( SDB_GROUPID_2_DSID( groupID ),
                                            dsPtr, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get data source[%u] failed[%d]",
                      SDB_GROUPID_2_DSID( groupID ), rc ) ;

         if ( dsPtr->inheritSessionAttr() )
         {
#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Data source session attribute version[%u] is "
                    "not the same with local[%u]. Set it to %s", nodeSiteVer,
                    curVersion, property->toBSON().toString().c_str() ) ;
#endif /* _DEBUG */
            rc = _setSessionAttr( pSub, *property, cb ) ;
            if ( SDB_OK != rc && SDB_INVALIDARG != rc )
            {
               PD_LOG( PDERROR, "Set data source session attribute for [%s] "
                       "failed[%d]",
                       routeID2String( pSub->getNodeID() ).c_str(), rc ) ;
               goto error ;
            }
            else if ( SDB_INVALIDARG == rc )
            {
               // There are some attributes that the data source dose not
               // support.
               rc = SDB_OK ;
            }
            nodeSiteVer = curVersion ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordRemoteHandlerBase::_setSessionAttr( _pmdSubSession *pSub,
                                                   const rtnSessionProperty &property,
                                                   pmdEDUCB *cb,
                                                   BOOLEAN compatibleMode )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         BSONObj attributes ;
         BSONObj attrObj = property.toBSON() ;
         BSONObjIterator itr( attrObj ) ;
         // Try to set all supported session attributes together. Need to append
         // field 'PreferedInstanceV1'.
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE_LEGACY,
                                 ele.fieldName() ) ||
                 0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE,
                                 ele.fieldName() ) )
            {
               builder.append( ele ) ;
               builder.appendAs( ele, FIELD_NAME_PREFERED_INSTANCE_V1 ) ;
            }
            else if ( _isSupportedDSSessoinAttr( ele.fieldName() ) )
            {
               builder.append( ele ) ;
            }
         }

         attributes = builder.done() ;
         rc = _setSessionAttr( pSub, attributes, cb ) ;
         if ( SDB_INVALIDARG == rc && compatibleMode )
         {
            // For some old versions, if some attributes are not supported, try
            // to set one by one.
            BSONObjBuilder tmpBuilder ;
            BSONObj tmpAttr ;
            BSONObjIterator tmpItr( attrObj ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpEle = tmpItr.next() ;
               if ( 0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE_LEGACY,
                                    tmpEle.fieldName() ) ||
                    0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE,
                                    tmpEle.fieldName() ) )
               {
                  tmpBuilder.append( tmpEle ) ;
                  tmpBuilder.appendAs( tmpEle,
                                       FIELD_NAME_PREFERED_INSTANCE_V1 ) ;
               }
               else if ( _isSupportedDSSessoinAttr( tmpEle.fieldName() ) )
               {
                  tmpBuilder.append( tmpEle ) ;
               }
               else
               {
                  continue ;
               }

               tmpAttr = tmpBuilder.done() ;
               rc = _setSessionAttr( pSub, tmpAttr, cb ) ;
               if ( rc )
               {
                  if ( SDB_INVALIDARG != rc )
                  {
                     PD_LOG( PDERROR, "Set session attribute for node[%s] "
                             "failed[%d]. Attributes: %s",
                             routeID2String( pSub->getNodeID() ).c_str(),
                             rc, tmpAttr.toString().c_str() ) ;
                     goto error ;
                  }
#ifdef _DEBUG
                  PD_LOG( PDDEBUG, "Session attribute %s is not supported on "
                          "node[%s]", tmpAttr.toString().c_str(),
                          routeID2String( pSub->getNodeID() ).c_str() ) ;
#endif /* _DEBUG */
                  rc = SDB_OK ;
               }
               tmpBuilder.reset() ;
            }
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Set session attribute %s for node[%s] "
                    "failed[%d]", attributes.toString().c_str(),
                    routeID2String( pSub->getNodeID() ).c_str(), rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc= ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordRemoteHandlerBase::_setSessionAttr( _pmdSubSession *pSub,
                                                   const BSONObj &attrObj,
                                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *msgBuff = NULL ;
      INT32 buffSize = 0 ;
      BSONObj dummyObj ;
      pmdRemoteSession *tmpSession = NULL ;
      _pmdSubSession *subSession = NULL ;
      MsgOpReply *reply = NULL ;

      pmdRemoteSessionSite *site = (pmdRemoteSessionSite *)cb->getRemoteSite() ;
      tmpSession = site->addSession( -1, NULL ) ;
      if ( !tmpSession )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create temp remote session failed[%d]", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryCMDMsg( &msgBuff, &buffSize,
                                CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR,
                                attrObj, dummyObj, dummyObj, dummyObj,
                                0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build set session attribute message "
                  "failed[%d]", rc ) ;

      subSession = tmpSession->addSubSession( pSub->getNodeIDUInt() ) ;
      subSession->setReqMsg( (MsgHeader *)msgBuff, PMD_EDU_MEM_NONE ) ;

      rc = tmpSession->sendMsg( subSession ) ;
      PD_RC_CHECK( rc, PDERROR, "Send set session attribute message to "
                   "node[%s] failed[%d]",
                   routeID2String( pSub->getNodeID() ).c_str(), rc ) ;

      rc = tmpSession->waitReply1( TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait set session attribute response from "
                   "node[%s] failed[%d]",
                   routeID2String( pSub->getNodeID() ).c_str(), rc ) ;

      reply = (MsgOpReply *)subSession->getRspMsg() ;
      if ( !reply )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Set session attribute reply message is null" ) ;
         goto error ;
      }

      rc = reply->flags ;
      if ( rc && ( SDB_INVALIDARG != rc ) )
      {
         PD_LOG( PDERROR, "Set session attribute on node[%s] failed[%d]",
                 routeID2String( pSub->getNodeID() ).c_str(), rc ) ;
         goto error ;
      }

   done:
      // If the connection is just established by the steps above, set it to the
      // connection in the original sub session.
      if ( !pSub->getConnection()->isConnected() &&
           subSession->getConnection()->isConnected() )
      {
         pSub->getConnection()->init(
            subSession->getConnection()->getRouteAgent(), TRUE,
            subSession->getNodeID(), subSession->getHandle() ) ;
      }
      if ( tmpSession )
      {
         site->removeSession( tmpSession ) ;
      }
      if ( msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordRemoteHandlerBase::_isSupportedDSSessoinAttr( const CHAR *attrName )
   {
      SDB_ASSERT( attrName, "Attribute name is null" ) ;

      return ( 0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE_MODE, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_STRICT_LEGACY, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_STRICT, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_PERIOD_LEGACY, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_PERIOD, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_TIMEOUT, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, attrName ) ||
               0 == ossStrcmp( FIELD_NAME_PREFERRED_INSTANCE, attrName ) ) ;
   }

   INT32 _coordRemoteHandlerBase::onSend( _pmdRemoteSession *pSession,
                                          _pmdSubSession *pSub )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pSession->getEDUCB() ;
      pmdRemoteSessionSite* pSite = NULL ;
      coordRemoteHandleStatus *pStatus = NULL ;
      UINT32 nodeSiteSchedVer = (UINT32)SCHED_INVALID_VERSION ;
      UINT32 nodeSiteSessionVer = 0 ;
      UINT64 nodeSiteVer = 0 ;
      MsgRouteID nodeID = pSub->getNodeID() ;

      pStatus = ( coordRemoteHandleStatus* )pSub->getUDFData() ;
      pStatus->init() ;


      /// is not data node or data source, ignored
      if ( ( ( nodeID.columns.groupID < DATA_GROUP_ID_BEGIN ||
               nodeID.columns.groupID > DATA_GROUP_ID_END ) &&
               ( !SDB_IS_DSID( nodeID.columns.groupID ) ) ) ||
           isNoReplyMsg( pSub->getOrgReqOpCode() ) )
      {
         goto done ;
      }

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( pSite->getNodeVer( nodeID.value, nodeSiteVer ) )
      {
         ossUnpack32From64( nodeSiteVer, nodeSiteSchedVer,
                            nodeSiteSessionVer ) ;
      }

      rc = _checkSessionSchedInfo( pSession, pSub, cb, nodeSiteSchedVer ) ;
      if ( rc )
      {
         goto error ;
      }

      /// init trans
      rc = _checkSessionTransaction( pSession, pSub, cb, pStatus ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _checkSessionAttr( pSession, pSub, cb, nodeSiteSessionVer ) ;
      if ( rc )
      {
         goto error ;
      }

      pStatus->_nodeVer = ossPack32To64( nodeSiteSchedVer,
                                         nodeSiteSessionVer ) ;
      /// has changed
      if ( nodeSiteVer != pStatus->_nodeVer )
      {
         pStatus->_nodeID = nodeID.value ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordRemoteHandlerBase::onHandleClose( _pmdRemoteSessionSite *pSite,
                                                NET_HANDLE handle,
                                                const MsgHeader *pReply )
   {
      _coordSessionPropSite *pPropSite = NULL ;
      pPropSite = ( _coordSessionPropSite* )pSite->getUserData() ;

      if ( pPropSite && pPropSite->isTransNode( pReply->routeID ) )
      {
         if ( pSite->existHandle( handle ) )
         {
            pSite->eduCB()->setTransRC( SDB_COORD_REMOTE_DISC ) ;

            PD_LOG( PDERROR, "Session[%s] disconnect with node[%s] in "
                    "transaction, Handle:%d",
                    pPropSite->getEDUCB()->toString().c_str(),
                    routeID2String(pReply->routeID).c_str(), handle ) ;
         }
      }
   }

   void _coordRemoteHandlerBase::setUserData( UINT64 data )
   {
      if ( (INT32)data == INIT_V0 )
      {
         _initType = INIT_V0 ;
      }
      else
      {
         _initType = INIT_V1 ;
      }
   }

   BOOLEAN _coordRemoteHandlerBase::canReconnect ( _pmdRemoteSession * session,
                                                   _pmdSubSession * subSession )
   {
      SDB_ASSERT( NULL != session, "remote session is invalid" ) ;
      SDB_ASSERT( NULL != subSession, "sub session is invalid" ) ;

      pmdEDUCB * cb = session->getEDUCB() ;
      if ( cb->isTransaction() )
      {
         pmdRemoteSessionSite * site = (pmdRemoteSessionSite *)
                                                         cb->getRemoteSite() ;
         coordSessionPropSite * propSite = (coordSessionPropSite *)
                                                         site->getUserData() ;
         UINT32 groupID = subSession->getNodeID().columns.groupID ;
         coordRemoteHandleStatus * status =
                           (coordRemoteHandleStatus *)subSession->getUDFData() ;
         // could not reconnect transaction node during transaction
         if ( propSite->hasTransNode( groupID ) && !status->_initTrans )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   INT32 _coordRemoteHandlerBase::_buildPacketWithUpdateSched( _pmdRemoteSession *pSession,
                                                               _pmdSubSession *pSub,
                                                               const BSONObj &objSched )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pSession->getEDUCB() ;
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;

      rc = msgBuildUpdateMsg( &pBuff, &buffSize,
                              CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO,
                              0, 0, NULL,  &objSched, NULL, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build update message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = coordBuildPacketMsg( pSession, pSub, ( MsgHeader* )pBuff ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( pBuff )
      {
         msgReleaseBuffer( pBuff, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordNoSessionInitHandler implement
   */
   _coordNoSessionInitHandler::_coordNoSessionInitHandler()
   {
   }

   _coordNoSessionInitHandler::~_coordNoSessionInitHandler()
   {
   }

   INT32 _coordNoSessionInitHandler::onSendConnect( _pmdSubSession *pSub,
                                                    const MsgHeader *pReq,
                                                    BOOLEAN isFirst )
   {
      /// already disconnect
      return SDB_COORD_REMOTE_DISC ;
   }

   /*
      _coordRemoteHandler implement
   */
   _coordRemoteHandler::_coordRemoteHandler()
   {
      _interruptWhenFailed = FALSE ;
   }

   _coordRemoteHandler::~_coordRemoteHandler()
   {
   }

   void _coordRemoteHandler::enableInterruptWhenFailed( BOOLEAN enable,
                                                        const SET_RC *pIgnoreRC )
   {
      _interruptWhenFailed = enable ;
      if ( pIgnoreRC )
      {
         _ignoreRC = *pIgnoreRC ;
      }
      else
      {
         _ignoreRC.clear() ;
      }
   }

   void _coordRemoteHandler::onReply( _pmdRemoteSession *pSession,
                                      _pmdSubSession **ppSub,
                                      const MsgHeader *pReply,
                                      BOOLEAN isPending )
   {
      BOOLEAN transRollback = FALSE ;
      _pmdEDUCB *cb = pSession->getEDUCB() ;
      INT32 beforeRC = cb->getTransRC() ;

      _coordRemoteHandlerBase::onReply( pSession, ppSub, pReply, isPending ) ;

      if ( SDB_OK == beforeRC && SDB_OK != cb->getTransRC() )
      {
         transRollback = TRUE ;
      }

      if ( transRollback || _interruptWhenFailed )
      {
         MsgOpReply *pOpReply = ( MsgOpReply* )pReply ;
         /// When not ok and not in ignored rc set
         if ( SDB_OK != pOpReply->flags &&
              _ignoreRC.find( pOpReply->flags ) == _ignoreRC.end() )
         {
            PD_LOG( PDWARNING, "Session[%s]: Sub-session[%s] recieved "
                    "invalid reply with flag[%d], stop other sub-sessions",
                    pSession->getEDUCB()->toString().c_str(),
                    routeID2String( pReply->routeID ).c_str(),
                    pOpReply->flags ) ;
            pSession->stopSubSession() ;
         }
      }
   }

}

