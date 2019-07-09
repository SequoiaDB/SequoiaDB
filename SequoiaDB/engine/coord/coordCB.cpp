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

   Source File Name = coordCB.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCB.hpp"
#include "pmd.hpp"
#include "pmdController.hpp"
#include "pmdStartup.hpp"
#include "coordOmStrategyJob.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   #define COORD_WAIT_EDU_ATTACH_TIMEOUT ( 60 * OSS_ONE_SEC )
   #define COORD_INVALID_TIMERID         (0)
   /*
   note: _CoordCB implement
   */
   BEGIN_OBJ_MSG_MAP( _CoordCB, _pmdObjBase )
      ON_MSG ( MSG_CAT_REG_RES, _onCatRegisterRes )
   END_OBJ_MSG_MAP()

   _CoordCB::_CoordCB()
   :_pMsgHandler( NULL ),
    _pTimerHandler( NULL ),
    _pAgent( NULL ),
    _shardServiceID ( MSG_ROUTE_SHARD_SERVCIE ),
    _regTimerID ( COORD_INVALID_TIMERID ),
    _pDmsCB( NULL ),
    _pDpsCB( NULL ),
    _pRtnCB( NULL ),
    _pEDUCB( NULL ),
    _needReply( TRUE )
   {
      _shdServiceName[0]  = 0 ;
      _selfNodeID.value   = MSG_INVALID_ROUTEID ;
      ossMemset( (void*)&_replyHeader, 0, sizeof(_replyHeader) ) ;
   }

   _CoordCB::~_CoordCB()
   {
      _pDmsCB    = NULL ;
      _pRtnCB    = NULL ;
      _pDpsCB    = NULL ;
      _pCollectionName = NULL ;
      _cmdCollectionName.clear() ;
   }

   coordResource* _CoordCB::getResource()
   {
      return &_resource ;
   }

   netRouteAgent* _CoordCB::getRouteAgent()
   {
      return _pAgent ;
   }

   pmdRemoteSessionMgr* _CoordCB::getRSManager()
   {
      return &_remoteSessionMgr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB_INIT, "_CoordCB::init" )
   INT32 _CoordCB::init ()
   {
      PD_TRACE_ENTRY ( SDB__COORDCB_INIT ) ;

      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb       = pmdGetKRCB() ;
      _pDmsCB              = pKrcb->getDMSCB() ;
      _pRtnCB              = pKrcb->getRTNCB() ;
      _pDpsCB              = pKrcb->getDPSCB() ;
      MsgRouteID nodeID = _selfNodeID ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      const CHAR* hostName = pmdGetKRCB()->getHostName() ;

      _pTimerHandler = SDB_OSS_NEW pmdRemoteTimerHandler() ;
      if ( !_pTimerHandler )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for timer handler" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pMsgHandler = SDB_OSS_NEW pmdRemoteMsgHandler( &_remoteSessionMgr ) ;
      if ( !_pMsgHandler )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for message handler" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pAgent = SDB_OSS_NEW _netRouteAgent( _pMsgHandler ) ;
      if ( !_pAgent )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for net agent" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pAgent->getFrame()->setBeatInfo( optCB->getOprTimeout() ) ;

      rc = _resource.init( _pAgent, optCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Init resource failed, rc: %d", rc ) ;

      ossStrncpy( _shdServiceName, optCB->shardService(),
                  OSS_MAX_SERVICENAME ) ;

      _sitePropMgr.setInstanceOption( optCB->getPrefInstStr(),
                                      optCB->getPrefInstModeStr(),
                                      PREFER_INSTANCE_TYPE_MASTER ) ;

      rc = _remoteSessionMgr.init( _pAgent, &_sitePropMgr ) ;
      PD_RC_CHECK ( rc, PDERROR, "Init session manager failed, rc: %d", rc ) ;

      sdbGetPMDController()->setRSManager( &_remoteSessionMgr ) ;

      nodeID.columns.serviceID = _shardServiceID ;
      _pAgent->updateRoute( nodeID, hostName, _shdServiceName ) ;
      rc = _pAgent->listen( nodeID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Create listen[Hostname:%s, ServiceName:%s] "
                    "failed", hostName, _shdServiceName ) ;
      PD_LOG ( PDEVENT, "Create sharding listen[ServiceName:%s] succeed",
               _shdServiceName ) ;

      pmdGetKRCB()->setBusinessOK( TRUE ) ;

      pmdGetStartup().ok( TRUE ) ;

      pmdGetKRCB()->setGroupName( COORD_GROUPNAME ) ;

   done:
      PD_TRACE_EXITRC ( SDB__COORDCB_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB_ACTIVE, "_CoordCB::active" )
   INT32 _CoordCB::active ()
   {
      PD_TRACE_ENTRY ( SDB__COORDCB_ACTIVE ) ;

      INT32 rc = SDB_OK ;
      pmdEDUMgr* pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;
      CoordVecNodeInfo catalogAddrList ;

      pmdSetPrimary( TRUE ) ;
      sdbGetPMDController()->registerNet( _pAgent->getFrame() ) ;

      rc = pEDUMgr->startEDU ( EDU_TYPE_COORDNETWORK, (void*)_pAgent,
                               &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start coord network edu, rc: %d",
                   rc ) ;
      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait CoordNet active failed, rc: %d", rc ) ;

      _attachEvent.reset() ;
      rc = pEDUMgr->startEDU ( EDU_TYPE_COORDMGR, (_pmdObjBase*)this,
                               &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start coord main controller edu, "
                   "rc: %d", rc ) ;
      rc = _attachEvent.wait( COORD_WAIT_EDU_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait coord manager edu "
                   "attach, rc: %d", rc ) ;

      _resource.getCataNodeAddrList( catalogAddrList ) ;
      if ( !catalogAddrList.empty() )
      {
         _regTimerID = setTimer( OSS_ONE_SEC ) ;

         if ( NET_INVALID_TIMER_ID == _regTimerID )
         {
            PD_LOG ( PDERROR, "Register timer failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         _sendRegisterMsg () ;
      }

      rc = coordStartOmStrategyJob( NULL ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__COORDCB_ACTIVE, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _CoordCB::deactive ()
   {
      if ( _pAgent )
      {
         sdbGetPMDController()->unregNet( _pAgent->getFrame() ) ;
         _pAgent->closeListen() ;
         _pAgent->stop() ;
      }

      return SDB_OK ;
   }

   INT32 _CoordCB::fini ()
   {
      _remoteSessionMgr.fini() ;
      _resource.fini() ;

      if ( _pAgent )
      {
         SDB_OSS_DEL _pAgent ;
         _pAgent = NULL ;
      }
      if ( _pMsgHandler )
      {
         SDB_OSS_DEL _pMsgHandler ;
         _pMsgHandler = NULL ;
      }
      if ( _pTimerHandler )
      {
         SDB_OSS_DEL _pTimerHandler ;
         _pTimerHandler = NULL ;
      }

      return SDB_OK ;
   }

   void _CoordCB::onConfigChange ()
   {
      pmdOptionsCB * optCB = pmdGetOptionCB() ;

      if ( _pAgent )
      {
         UINT32 oprtimeout = optCB->getOprTimeout() ;
         _pAgent->getFrame()->setBeatInfo( oprtimeout ) ;
      }

      _sitePropMgr.setInstanceOption( optCB->getPrefInstStr(),
                                      optCB->getPrefInstModeStr(),
                                      PREFER_INSTANCE_TYPE_MASTER ) ;
   }

   void _CoordCB::attachCB( _pmdEDUCB *cb )
   {
      _pEDUCB = cb ;
      _remoteSessionMgr.registerEDU( cb ) ;
      _pMsgHandler->attach( cb ) ;
      _pTimerHandler->attach( cb ) ;

      _attachEvent.signalAll() ;
   }

   void _CoordCB::detachCB( _pmdEDUCB *cb )
   {
      _pMsgHandler->detach() ;
      _pTimerHandler->detach() ;
      _remoteSessionMgr.unregEUD( cb ) ;

      _pEDUCB = NULL ;
   }

   UINT32 _CoordCB::setTimer( UINT32 milliSec )
   {
      UINT32 timeID = NET_INVALID_TIMER_ID ;
      if ( _pAgent )
      {
         _pAgent->addTimer( milliSec, _pTimerHandler, timeID ) ;
      }
      return timeID ;
   }

   void _CoordCB::killTimer( UINT32 timerID )
   {
      if ( _pAgent )
      {
         _pAgent->removeTimer( timerID ) ;
      }
   }

   void _CoordCB::onTimer( UINT64 timerID, UINT32 interval )
   {
      if ( timerID == _regTimerID )
      {
         _sendRegisterMsg () ;
      }
      else
      {
         _pmdObjBase::onTimer( timerID, interval ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__REPLY, "_CoordCB::_reply" )
   INT32 _CoordCB::_reply ( const NET_HANDLE &handle,
                            MsgOpReply *pReply,
                            const CHAR *pReplyData,
                            UINT32 replyDataLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__COORDCB__REPLY ) ;

      if ( (UINT32)( pReply->header.messageLength ) !=
           sizeof( MsgOpReply ) + replyDataLen )
      {
         PD_LOG ( PDERROR, "Reply message length error[%u != %u]",
                  pReply->header.messageLength,
                  sizeof( MsgOpReply ) + replyDataLen ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( replyDataLen > 0 )
      {
         rc = _pAgent->syncSend ( handle, (MsgHeader *)pReply,
                                  (void*)pReplyData, replyDataLen ) ;
      }
      else
      {
         rc = _pAgent->syncSend ( handle, (void *)pReply ) ;
      }

      PD_RC_CHECK ( rc, PDDEBUG, "Fail to send reply message[opCode:(%d)%d, "
              "tid: %d, reqID: %lld, nodeID: %u.%u.%u], rc: %d",
              IS_REPLY_TYPE( pReply->header.opCode ),
              GET_REQUEST_TYPE( pReply->header.opCode ),
              pReply->header.TID, pReply->header.requestID,
              pReply->header.routeID.columns.groupID,
              pReply->header.routeID.columns.nodeID,
              pReply->header.routeID.columns.serviceID, rc ) ;
      PD_LOG( PDDEBUG, "Succeed in sending reply message[opCode:(%d)%d, "
              "tid: %d, reqID: %lld, nodeID: %u.%u.%u]",
              IS_REPLY_TYPE( pReply->header.opCode ),
              GET_REQUEST_TYPE( pReply->header.opCode ),
              pReply->header.TID, pReply->header.requestID,
              pReply->header.routeID.columns.groupID,
              pReply->header.routeID.columns.nodeID,
              pReply->header.routeID.columns.serviceID ) ;

   done :
      PD_TRACE_EXITRC ( SDB__COORDCB__REPLY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__SENDTOCATLOG, "_CoordCB::_sendToCatlog" )
   INT32 _CoordCB::_sendToCatlog ( MsgHeader *pMsg, NET_HANDLE *pHandle )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__SENDTOCATLOG ) ;

      INT32 rc = SDB_OK ;
      MsgRouteID nodeID ;
      BOOLEAN hasUpdate = FALSE ;
      CoordGroupInfoPtr cataGroupPtr ;

retry :
      if ( !_pAgent )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Network runtime agent does not exist" ) ;
         goto error ;
      }

      cataGroupPtr = _resource.getCataGroupInfo() ;
      if ( 0 == cataGroupPtr->nodeCount() )
      {
         if ( !hasUpdate )
         {
            rc = _resource.updateCataGroupInfo( cataGroupPtr, _pEDUCB ) ;
            PD_RC_CHECK ( rc, PDWARNING, "Fail to update catalog group "
                          "info[rc:%d]", rc ) ;
            hasUpdate = TRUE ;
            goto retry ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "catalog list is empty, rc = %d", rc ) ;
            goto error ;
         }
      }

      nodeID = cataGroupPtr->primary( MSG_ROUTE_CAT_SERVICE ) ;
      if ( MSG_INVALID_ROUTEID == nodeID.value )
      {
         if ( !hasUpdate )
         {
            rc = _resource.updateCataGroupInfo( cataGroupPtr, _pEDUCB ) ;
            if ( rc != SDB_OK )
            {
               PD_LOG ( PDWARNING,
                        "Fail to update catalog group info[rc:%d]", rc ) ;
               goto error ;
            }
            hasUpdate = TRUE ;
            goto retry ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG ( PDWARNING, "Fail to get primary node of catalog group" ) ;
            goto error ;
         }
      }

      rc = _pAgent->syncSend ( nodeID, (void*)pMsg, pHandle ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDWARNING, "Send message to primary catalog[%u, "
                  "%u, %u] failed[rc: %d]", nodeID.columns.groupID,
                  nodeID.columns.nodeID, nodeID.columns.serviceID, rc ) ;
         cataGroupPtr->updateNodeStat( nodeID.columns.nodeID,
                                       netResult2Status( rc ) ) ;
      }
      else
      {
         goto done ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__COORDCB__SENDTOCATLOG, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__SNDREGMSG, "_CoordCB::_sendRegisterMsg" )
   INT32 _CoordCB::_sendRegisterMsg ()
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__SNDREGMSG );
      INT32 rc = SDB_OK ;
      UINT32 length = 0 ;
      CHAR *buff = NULL ;
      MsgCatRegisterReq *pReq = NULL ;
      clsRegAssit regAssit ;

      BSONObj regObj = regAssit.buildRequestObj () ;
      length = regObj.objsize () + sizeof ( MsgCatRegisterReq ) ;

      buff = (CHAR *)SDB_OSS_MALLOC ( length ) ;
      if ( buff == NULL )
      {
         PD_LOG ( PDERROR, "Failed to allocate memroy for register req" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      pReq = (MsgCatRegisterReq*)buff ;
      pReq->header.messageLength = length ;
      pReq->header.opCode = MSG_CAT_REG_REQ ;
      pReq->header.requestID = 0 ;
      pReq->header.TID = 0 ;
      pReq->header.routeID.value = 0 ;
      ossMemcpy( pReq->data, regObj.objdata(), regObj.objsize() ) ;

      rc = _sendToCatlog( (MsgHeader *) pReq ) ;

   done :
      if ( buff )
      {
         SDB_OSS_FREE ( buff ) ;
         buff = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__COORDCB__SNDREGMSG, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__ONCATREGRES, "_clsMgr::_onCatRegisterRes" )
   INT32 _CoordCB::_onCatRegisterRes ( const NET_HANDLE &handle,
                                       MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__COORDCB__ONCATREGRES );
      UINT32 groupID = INVALID_GROUPID ;
      UINT16 nodeID = INVALID_NODEID ;
      const CHAR *hostname = NULL ;
      MsgRouteID routeID ;
      clsRegAssit regAssit ;

      if ( _regTimerID == CLS_INVALID_TIMERID )
      {
         goto done ;
      }

      rc = MSG_GET_INNER_REPLY_RC( pMsg ) ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         CoordGroupInfoPtr cataGroupPtr ;
         rc = _resource.updateCataGroupInfo( cataGroupPtr, _pEDUCB ) ;
         PD_RC_CHECK ( rc, PDWARNING, "Fail to update catalog group "
                       "info[rc:%d]", rc ) ;
         goto done ;
      }
      if ( SDB_CAT_AUTH_FAILED == rc )
      {
         killTimer ( _regTimerID ) ;
         _regTimerID = CLS_INVALID_TIMERID ;
         goto done ;
      }
      PD_RC_CHECK ( rc, PDSEVERE, "Node register failed, rc: %d", rc ) ;

      rc = regAssit.extractResponseMsg ( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Node register response error, rc: %d", rc ) ;
      groupID = regAssit.getGroupID () ;
      nodeID = regAssit.getNodeID () ;
      hostname = regAssit.getHostname () ;

      killTimer ( _regTimerID ) ;
      _regTimerID = CLS_INVALID_TIMERID ;

      _selfNodeID.columns.groupID = groupID ;
      _selfNodeID.columns.nodeID = nodeID ;
      PD_LOG ( PDEVENT, "Register succeed, groupID:%u, nodeID:%u",
               _selfNodeID.columns.groupID, _selfNodeID.columns.nodeID ) ;

      /*
       * The node can be created by hostname or ip,
       * so the actual 'HostName' maybe current host's name or ip address.
       * Here we ensure the KRCB's HostName is consistent with catalog.
       */
      pmdGetKRCB()->setHostName( hostname ) ;

      routeID.value = _selfNodeID.value ;
      routeID.columns.serviceID = _shardServiceID ;
      _pAgent->setLocalID ( routeID ) ;

      pmdSetNodeID( _selfNodeID ) ;
      pmdGetKRCB()->callRegisterEventHandler( _selfNodeID ) ;

   done:
      PD_TRACE_EXITRC ( SDB__COORDCB__ONCATREGRES, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _CoordCB::_defaultMsgFunc( NET_HANDLE handle, MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;

      rc = _processMsg( handle, pMsg ) ;
      return rc ;
   }

   void _CoordCB::_onMsgBegin( MsgHeader *pMsg )
   {
      _replyHeader.numReturned          = 0 ;
      _replyHeader.startFrom            = 0 ;
      _replyHeader.header.opCode        = MAKE_REPLY_TYPE( pMsg->opCode ) ;
      _replyHeader.header.requestID     = pMsg->requestID ;
      _replyHeader.header.TID           = pMsg->TID ;
      _replyHeader.header.routeID.value = 0 ;

      if ( MSG_BS_INTERRUPTE      == pMsg->opCode ||
           MSG_BS_INTERRUPTE_SELF == pMsg->opCode ||
           MSG_BS_DISCONNECT      == pMsg->opCode ||
           MSG_COM_REMOTE_DISC    == pMsg->opCode ||
           MSG_CLS_GINFO_UPDATED  == pMsg->opCode ||
           MSG_CAT_GRP_CHANGE_NTY == pMsg->opCode )
      {
         _needReply = FALSE ;
      }
      else
      {
         _needReply = TRUE ;
      }

      MON_START_OP( _pEDUCB->getMonAppCB() ) ;
      _pEDUCB->getMonAppCB()->setLastOpType( pMsg->opCode ) ;
   }

   void _CoordCB::_onMsgEnd( )
   {
      MON_END_OP( _pEDUCB->getMonAppCB() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__PROCESSMSG, "_CoordCB::_processMsg" )
   INT32 _CoordCB::_processMsg( const NET_HANDLE &handle,
                                MsgHeader *pMsg )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__PROCESSMSG ) ;
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;
      INT64 contextID = -1 ;

      _onMsgBegin( pMsg ) ;
      switch ( pMsg->opCode )
      {
         case MSG_BS_QUERY_REQ:
            rc = _processQueryMsg( pMsg, buffObj, contextID );
            break;
         case MSG_BS_GETMORE_REQ :
            rc = _processGetMoreMsg( pMsg, buffObj, contextID ) ;
            break ;
         case MSG_BS_KILL_CONTEXT_REQ:
            rc = _processKillContext( pMsg ) ;
            break;
         case MSG_COM_SESSION_INIT_REQ :
            rc = _processSessionInit( pMsg ) ;
            break;
         case MSG_BS_INTERRUPTE :
            rc = _processInterruptMsg( handle, pMsg ) ;
            break ;
         case MSG_BS_DISCONNECT :
            rc = _processDisconnectMsg( handle, pMsg ) ;
            break ;
         case MSG_COM_REMOTE_DISC :
            rc = _processRemoteDisc( handle, pMsg ) ;
            break ;
         case MSG_CLS_GINFO_UPDATED :
            rc = _processUpdateGrpInfo() ;
            break ;
         case MSG_CAT_GRP_CHANGE_NTY :
            rc = _processCatGrpChgNty() ;
            break ;
         case MSG_BS_MSG_REQ :
            rc = _processMsgReq( pMsg ) ;
            break ;
         case MSG_AUTH_VERIFY_REQ :
         case MSG_AUTH_CRTUSR_REQ :
         case MSG_AUTH_DELUSR_REQ :
            rc = SDB_OK ;
            break ;
         case MSG_BS_INTERRUPTE_SELF :
            rc = SDB_OK ;
            break ;
         default :
            rc = SDB_CLS_UNKNOW_MSG ;
            break ;
      }

      if ( rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDERROR, "prcess msg[opCode:(%d)%d, len: %d, "
                 "tid: %d, reqID: %lld, nodeID: %u.%u.%u] failed, rc: %d",
                 IS_REPLY_TYPE(pMsg->opCode), GET_REQUEST_TYPE(pMsg->opCode),
                 pMsg->messageLength, pMsg->TID, pMsg->requestID,
                 pMsg->routeID.columns.groupID, pMsg->routeID.columns.nodeID,
                 pMsg->routeID.columns.serviceID, rc ) ;
      }

      if(  MSG_BS_QUERY_REQ == pMsg->opCode && contextID != -1 )
      {
         _addContext( handle, pMsg->TID, contextID );
      }

      if ( _needReply )
      {
         if ( rc && 0 == buffObj.size() )
         {
            _errorInfo = utilGetErrorBson( rc, _pEDUCB->getInfo(
                                           EDU_INFO_ERROR ) ) ;
            buffObj = rtnContextBuf( _errorInfo ) ;
         }

         _replyHeader.header.messageLength = sizeof( MsgOpReply ) +
                                             buffObj.size();
         _replyHeader.flags                = rc ;
         _replyHeader.contextID            = contextID ;
         _replyHeader.startFrom            = (INT32)buffObj.getStartFrom() ;
         _replyHeader.numReturned          = buffObj.recordNum() ;

         rc = _reply( handle, &_replyHeader, buffObj.data(),
                      buffObj.size() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to send reply, rc: %d", rc ) ;
         }
      }

      _onMsgEnd() ;
      PD_TRACE_EXITRC ( SDB__COORDCB__PROCESSMSG, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__GETMOREMSG, "_CoordCB::_processGetMoreMsg" )
   INT32 _CoordCB::_processGetMoreMsg ( MsgHeader *pMsg,
                                        rtnContextBuf &buffObj,
                                        INT64 &contextID )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__GETMOREMSG ) ;
      INT32 rc         = SDB_OK ;
      INT32 numToRead  = 0 ;
      BOOLEAN rtnDel   = TRUE ;

      rc = msgExtractGetMore( (CHAR*)pMsg, &numToRead, &contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Extract GETMORE msg failed[rc:%d]", rc ) ;

      MON_SAVE_OP_DETAIL( _pEDUCB->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld, NumToRead:%d",
                          contextID, numToRead ) ;

      rc = rtnGetMore ( contextID, numToRead, buffObj, _pEDUCB, _pRtnCB ) ;
      if ( rc )
      {
         rtnDel = FALSE ;
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG ( PDERROR, "Failed to get more, rc: %d", rc ) ;
         }
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__COORDCB__GETMOREMSG, rc ) ;
      return rc ;
   error :
      _delContextByID( contextID, rtnDel ) ;
      contextID = -1 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__KILLCONTEXT, "_CoordCB::_processKillContext" )
   INT32 _CoordCB::_processKillContext( MsgHeader *pMsg )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__KILLCONTEXT ) ;

      INT32 rc = SDB_OK ;
      INT32 contextNum = 0 ;
      INT64 *pContextIDs = NULL ;

      rc = msgExtractKillContexts ( (CHAR *)pMsg, &contextNum, &pContextIDs ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to parse the killcontexts request, "
                    "rc: %d", rc ) ;

      if ( contextNum > 0 )
      {
         PD_LOG ( PDDEBUG, "Kill context: contextNum: %d", contextNum ) ;
      }

      for ( INT32 i = 0 ; i < contextNum ; ++i )
      {
         PD_LOG( PDDEBUG, "Kill context %lld", pContextIDs[i] ) ;
         _delContextByID( pContextIDs[ i ], TRUE ) ;
      }

   done:
      _replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      _replyHeader.flags = rc ;
      PD_TRACE_EXITRC ( SDB__COORDCB__KILLCONTEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _CoordCB::_filterQueryCmd( _rtnCommand *pCommand,
                                    MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;

      switch ( pCommand->type() )
      {
         case CMD_SNAPSHOT_COLLECTIONS :
         case CMD_SNAPSHOT_COLLECTIONSPACES :
         case CMD_SNAPSHOT_CONTEXTS_CURRENT :
         case CMD_SNAPSHOT_SESSIONS_CURRENT :

         case CMD_LIST_CONTEXTS_CURRENT :
         case CMD_LIST_SESSIONS_CURRENT :

         case CMD_LIST_BACKUPS :

         case CMD_BACKUP_OFFLINE :
         case CMD_REMOVE_BACKUP :
         case CMD_SYNC_DB :
         case CMD_LOAD_COLLECTIONSPACE :
         case CMD_UNLOAD_COLLECTIONSPACE :
         case CMD_ANALYZE :

            rc = SDB_RTN_CMD_NO_NODE_AUTH ;
            PD_LOG ( PDWARNING, "Filter query command[%s], rc: %d",
                     pCommand->name(), rc ) ;
            break ;

         case CMD_LIST_TRANS_CURRENT :
         case CMD_SNAPSHOT_TRANSACTIONS_CUR :
            rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
            PD_LOG ( PDWARNING, "Filter query command[%s], rc: %d",
                     pCommand->name(), rc ) ;
            break ;

         default:
            break ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCB__QUERYMSG, "_CoordCB::_processQueryMsg" )
   INT32 _CoordCB::_processQueryMsg( MsgHeader *pMsg,
                                     rtnContextBuf &buffObj,
                                     INT64 &contextID )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__QUERYMSG ) ;

      INT32 rc                = SDB_OK ;
      CHAR *pCollectionName   = NULL ;
      CHAR *pQueryBuff        = NULL ;
      CHAR *pFieldSelector    = NULL ;
      CHAR *pOrderByBuffer    = NULL ;
      CHAR *pHintBuffer       = NULL ;
      INT32 flags             = 0 ;
      INT64 numToSkip         = -1 ;
      INT64 numToReturn       = -1 ;
      INT16 w                 = 1 ;
      _rtnCommand *pCommand   = NULL ;

      rc = msgExtractQuery ( (CHAR *)pMsg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQueryBuff,
                             &pFieldSelector, &pOrderByBuffer, &pHintBuffer ) ;
      PD_RC_CHECK ( rc, PDERROR, "Extract query msg failed[rc:%d]", rc ) ;


      if ( !rtnIsCommand ( pCollectionName ) )
      {
         PD_LOG( PDERROR, "Receive unknown msg[opCode:(%d)%d, len: %d, "
                 "tid: %d, reqID: %lld, nodeID: %u.%u.%u]",
                 IS_REPLY_TYPE( pMsg->opCode ),
                 GET_REQUEST_TYPE( pMsg->opCode ),
                 pMsg->messageLength, pMsg->TID, pMsg->requestID,
                 pMsg->routeID.columns.groupID, pMsg->routeID.columns.nodeID,
                 pMsg->routeID.columns.serviceID ) ;
         rc = SDB_RTN_CMD_NO_NODE_AUTH ;
         goto error ;
      }

      rc = rtnParserCommand( pCollectionName, &pCommand ) ;
      PD_RC_CHECK ( rc, PDERROR, "Parse command[%s] failed[rc:%d]",
                    pCollectionName, rc ) ;

      if ( NULL != pCommand->collectionFullName() ||
           TRUE == pCommand->writable() )
      {
         rc = SDB_RTN_CMD_NO_NODE_AUTH ;
         PD_LOG ( PDERROR, "Coord not allowed command[%s], rc: %d",
                  pCommand->name(), rc ) ;
         goto error ;
      }

      rc = _filterQueryCmd( pCommand, pMsg ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_RTN_CMD_NO_NODE_AUTH == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG ( PDERROR, "Filter query command[%s], rc: %d",
                  pCommand->name(), rc ) ;
         goto error ;
      }

      rc = rtnInitCommand( pCommand , flags, numToSkip, numToReturn,
                           pQueryBuff, pFieldSelector, pOrderByBuffer,
                           pHintBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _pCollectionName = NULL ;
      _cmdCollectionName.clear() ;
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

      if ( CMD_INVALIDATE_CACHE == pCommand->type() )
      {
         getResource()->invalidateCataInfo() ;
         getResource()->invalidateGroupInfo() ;
      }
      else
      {
         rc = rtnRunCommand( pCommand, CMD_SPACE_SERVICE_SHARD, _pEDUCB,
                             _pDmsCB, _pRtnCB, _pDpsCB, w, &contextID ) ;
         PD_RC_CHECK ( rc, PDERROR, "Run command[%s] failed, rc: %d",
                       pCommand->name(), rc ) ;
      }

   done :
      if ( pCommand )
      {
         rtnReleaseCommand( &pCommand ) ;
      }
      PD_TRACE_EXITRC ( SDB__COORDCB__QUERYMSG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__SESSIONINIT, "_CoordCB::_processSessionInit" )
   INT32 _CoordCB::_processSessionInit( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__COORDCB__SESSIONINIT ) ;
      MsgComSessionInitReq *pMsgReq = (MsgComSessionInitReq*)pMsg ;

      MsgRouteID localRouteID = _pAgent->localID() ;
      if ( pMsgReq->dstRouteID.value != localRouteID.value )
      {
         rc = SDB_INVALID_ROUTEID ;
         PD_LOG ( PDERROR, "Session init failed: route id does not match."
                  "Message info: [%s], Local route id: %s",
                  msg2String( pMsg ).c_str(),
                  routeID2String( localRouteID ).c_str() ) ;
      }

      PD_TRACE_EXITRC ( SDB__COORDCB__SESSIONINIT, rc ) ;
      return rc ;
   }

   INT32 _CoordCB::_processInterruptMsg( const NET_HANDLE & handle,
                                         MsgHeader * header )
   {
      PD_LOG( PDEVENT, "Recieve interrupt msg[handle: %u, tid: %u]",
              handle, header->TID ) ;
      _delContext( handle, header->TID ) ;
      return SDB_OK ;
   }

   INT32 _CoordCB::_processDisconnectMsg( const NET_HANDLE & handle,
                                          MsgHeader * header )
   {
      PD_LOG( PDEVENT, "Recieve disconnect msg[handle: %u, tid: %u]",
              handle, header->TID ) ;
      _delContext( handle, header->TID ) ;
      return SDB_OK ;
   }

   INT32 _CoordCB::_processRemoteDisc( const NET_HANDLE &handle,
                                       MsgHeader *pMsg )
   {
      PD_LOG ( PDDEBUG, "Killing handle contexts %u", handle ) ;
      _delContextByHandle( handle ) ;
      return SDB_OK ;
   }

   INT32 _CoordCB::_processMsgReq( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      rc = rtnMsg( (MsgOpMsg*)pMsg ) ;
      return rc ;
   }

   INT32 _CoordCB::_processUpdateGrpInfo ()
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr groupPtr ;

      rc = _resource.updateGroupInfo ( COORD_GROUPID, groupPtr, _pEDUCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDWARNING, "Fail to update coord group info, rc: %d", rc ) ;
      }

      return rc ;
   }

   INT32 _CoordCB::_processCatGrpChgNty ()
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr groupPtr ;

      rc = _resource.updateCataGroupInfo ( groupPtr, _pEDUCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDWARNING, "Fail to update cata group info, rc: %d", rc ) ;
      }

      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__DELCTXHDL, "_CoordCB::_delContextByHandle" )
   void _CoordCB::_delContextByHandle( const UINT32 &handle )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__DELCTXHDL ) ;

      PD_LOG ( PDDEBUG, "delete context( handle=%u )", handle ) ;
      UINT32 saveTid = 0 ;
      UINT32 saveHandle = 0 ;

      ossScopedLock lock( &_contextLatch ) ;
      CONTEXT_LIST::iterator iterMap = _contextLst.begin() ;
      while ( iterMap != _contextLst.end() )
      {
         ossUnpack32From64( iterMap->second, saveHandle, saveTid ) ;
         if ( handle != saveHandle )
         {
            ++iterMap ;
            continue ;
         }
         _pRtnCB->contextDelete( iterMap->first, _pEDUCB );
         iterMap =  _contextLst.erase( iterMap ) ;
      }

      PD_TRACE_EXIT ( SDB__COORDCB__DELCTXHDL );
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__DELCTX, "_CoordCB::_delContext" )
   void _CoordCB::_delContext( const UINT32 &handle, UINT32 tid )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__DELCTX ) ;

      PD_LOG ( PDDEBUG, "delete context( handle=%u, tid=%u )", handle, tid ) ;
      UINT32 saveTid = 0 ;
      UINT32 saveHandle = 0 ;

      ossScopedLock lock( &_contextLatch ) ;
      CONTEXT_LIST::iterator iterMap = _contextLst.begin() ;
      while ( iterMap != _contextLst.end() )
      {
         ossUnpack32From64( iterMap->second, saveHandle, saveTid ) ;
         if ( handle != saveHandle || tid != saveTid )
         {
            ++iterMap ;
            continue ;
         }
         _pRtnCB->contextDelete( iterMap->first, _pEDUCB ) ;
         iterMap = _contextLst.erase( iterMap ) ;
      }

      PD_TRACE_EXIT ( SDB__COORDCB__DELCTX );
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__COORDCB__DELCTXID, "_CoordCB::_delContextByID" )
   void _CoordCB::_delContextByID( INT64 contextID, BOOLEAN rtnDel )
   {
      PD_TRACE_ENTRY ( SDB__COORDCB__DELCTXID ) ;

      PD_LOG ( PDDEBUG,
               "delete context( contextID=%lld )", contextID ) ;

      ossScopedLock lock( &_contextLatch ) ;
      CONTEXT_LIST::iterator iterMap = _contextLst.find( contextID ) ;
      if ( iterMap != _contextLst.end() )
      {
         if ( rtnDel )
         {
            _pRtnCB->contextDelete( iterMap->first, _pEDUCB ) ;
         }
         _contextLst.erase( iterMap ) ;
      }

      PD_TRACE_EXIT ( SDB__COORDCB__DELCTXID );
   }

   void _CoordCB::_addContext( const UINT32 &handle, UINT32 tid,
                               INT64 contextID )
   {
      if ( -1 != contextID )
      {
         PD_LOG( PDDEBUG, "add context( handle=%u, contextID=%lld )",
                 handle, contextID );
         ossScopedLock lock( &_contextLatch ) ;
         _contextLst[ contextID ] = ossPack32To64( handle, tid ) ;
      }
   }

   /*
      get global coord cb
   */
   CoordCB* sdbGetCoordCB ()
   {
      static CoordCB s_coordCB ;
      return &s_coordCB ;
   }
}

