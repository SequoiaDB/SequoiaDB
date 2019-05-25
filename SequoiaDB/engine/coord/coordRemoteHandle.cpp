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
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define COORD_EXPIRED_KILLCONTEXT_TIMEOUT       ( 30000 )      /// 30s

   /*
      _coordRemoteHandlerBase implement
   */
   _coordRemoteHandlerBase::_coordRemoteHandlerBase()
   {
   }

   _coordRemoteHandlerBase::~_coordRemoteHandlerBase()
   {
   }

   INT32 _coordRemoteHandlerBase::onSendFailed( _pmdRemoteSession *pSession,
                                                _pmdSubSession **ppSub,
                                                INT32 flag )
   {
      return flag ;
   }

   void _coordRemoteHandlerBase::onReply( _pmdRemoteSession *pSession,
                                          _pmdSubSession **ppSub,
                                          const MsgHeader *pReply,
                                          BOOLEAN isPending )
   {
   }

   INT32 _coordRemoteHandlerBase::onSendConnect( _pmdSubSession *pSub,
                                                 const MsgHeader *pReq,
                                                 BOOLEAN isFirst )
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

      const CHAR *pRemoteIP = "" ;
      UINT16 remotePort = 0 ;

      if ( cb->getSession() )
      {
         IClient *pClient = cb->getSession()->getClient() ;
         if ( pClient )
         {
            pRemoteIP = pClient->getFromIPAddr() ;
            remotePort = pClient->getFromPort() ;
         }
      }

      try
      {
         objInfo = BSON( SDB_AUTH_USER << cb->getUserName() <<
                         SDB_AUTH_PASSWD << cb->getPassword() <<
                         FIELD_NAME_HOST << pmdGetKRCB()->getHostName() <<
                         PMD_OPTION_SVCNAME << pmdGetOptionCB()->getServiceAddr() <<
                         FIELD_NAME_REMOTE_IP << pRemoteIP <<
                         FIELD_NAME_REMOTE_PORT << (INT32)remotePort ) ;
         msgLength += objInfo.objsize() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = cb->allocBuff( msgLength, &pBuff, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc memory failed, size: %u, rc: %d",
                 msgLength, rc ) ;
         goto error ;
      }
      pInitReq = (MsgComSessionInitReq*)pBuff ;

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

      pSub = pSession->addSubSession( nodeID.value ) ;
      pSub->setReqMsg( ( MsgHeader* )pInitReq, PMD_EDU_MEM_NONE ) ;

      rc = pSession->sendMsg( pSub ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Send session init request to node[%s] failed, "
                 "rc: %d", routeID2String( nodeID ).c_str(), rc ) ;
         goto error ;
      }

      rc = pSession->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait session init response from node[%s] failed, "
                 "rc: %d", routeID2String( nodeID ).c_str(), rc ) ;
         goto error ;
      }

      pReply = ( MsgOpReply* )pSub->getRspMsg( FALSE ) ;
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
      return SDB_NET_NOT_CONNECT ;
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
      if ( _interruptWhenFailed )
      {
         MsgOpReply *pOpReply = ( MsgOpReply* )pReply ;
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

   INT32 _coordRemoteHandler::onExpiredReply ( pmdRemoteSessionSite *pSite,
                                               const MsgHeader *pReply )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *pOpReply = NULL ;
      pmdRemoteSession *pSession = NULL ;
      MsgOpKillContexts msgKillContext ;

      if ( NULL == pReply || !IS_REPLY_TYPE( pReply->opCode ) )
      {
         goto done ;
      }

      pOpReply = (MsgOpReply *)pReply ;
      if ( -1 == pOpReply->contextID )
      {
         goto done ;
      }

      PD_LOG( PDWARNING, "Received expired context[%lld] from node[%s]",
              pOpReply->contextID,
              routeID2String( pReply->routeID ).c_str() ) ;

      pSession = pSite->addSession( COORD_EXPIRED_KILLCONTEXT_TIMEOUT ) ;
      pSession->addSubSession( pReply->routeID.value ) ;

      msgKillContext.contextIDs[ 0 ] = pOpReply->contextID ;
      msgKillContext.numContexts = 1 ;
      msgKillContext.ZERO = 0 ;
      msgKillContext.header.messageLength = sizeof( MsgOpKillContexts ) ;
      msgKillContext.header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
      msgKillContext.header.requestID = 0 ;
      msgKillContext.header.routeID.value = 0 ;
      msgKillContext.header.TID = 0 ;

      rc = pSession->sendMsg( (MsgHeader*)&msgKillContext,
                              PMD_EDU_MEM_NONE ) ;
      if ( SDB_OK == rc )
      {
         pSession->waitReply1() ;
      }

   done:
      if ( pSession )
      {
         pSite->removeSession( pSession ) ;
      }
      return rc ;
   }

}

