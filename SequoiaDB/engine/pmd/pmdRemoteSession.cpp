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

   Source File Name = pmdRemoteSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdRemoteSession.hpp"
#include "pmdEDU.hpp"
#include "msgMessage.hpp"

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      pmdSubSession implement
   */
   _pmdSubSession::_pmdSubSession()
   {
      _parent           = NULL ;
      _nodeID.value     = MSG_INVALID_ROUTEID ;
      _reqID            = 0 ;
      _pReqMsg          = NULL ;
      _isSend           = FALSE ;
      _isDisconnect     = TRUE ;
      _memType          = PMD_EDU_MEM_NONE ;

      _isProcessed      = FALSE ;
      _processResult    = SDB_OK ;
      _userData         = 0 ;
      _needToDel        = FALSE ;
      _hasStop          = FALSE ;
      _handle           = NET_INVALID_HANDLE ;
      _addPos           = -1 ;
   }

   _pmdSubSession::~_pmdSubSession()
   {
      clearReplyInfo() ;
      clearRequestInfo() ;
      _parent = NULL ;
   }

   void _pmdSubSession::clearReplyInfo()
   {
      if ( _event._Data && PMD_EDU_MEM_NONE != _event._dataMemType )
      {
         pmdEduEventRelase( _event, _parent->getEDUCB() ) ;
      }
      _event.reset() ;
   }

   void _pmdSubSession::clearRequestInfo()
   {
      _ioDatas.clear() ;

      if ( _pReqMsg )
      {
         if ( PMD_EDU_MEM_ALLOC == _memType )
         {
            SDB_OSS_FREE( (void*)_pReqMsg ) ;
         }
         else if ( PMD_EDU_MEM_SELF == _memType )
         {
            _parent->getEDUCB()->releaseBuff( (CHAR*)_pReqMsg ) ;
         }
         _pReqMsg = NULL ;
      }
      _memType = PMD_EDU_MEM_NONE ;
   }

   void _pmdSubSession::addIODatas( const netIOVec &ioVec )
   {
      for ( UINT32 i = 0 ; i < ioVec.size() ; ++i )
      {
         _ioDatas.push_back( ioVec[ i ] ) ;
      }
   }

   void _pmdSubSession::addIOData( const netIOV &io )
   {
      _ioDatas.push_back( io ) ;
   }

   UINT32 _pmdSubSession::getIODataLen()
   {
      UINT32 len = 0 ;
      for ( UINT32 i = 0 ; i < _ioDatas.size() ; ++i )
      {
         len += _ioDatas[ i ].iovLen ;
      }
      return len ;
   }

   void _pmdSubSession::setReqMsg( MsgHeader *pReqMsg,
                                   pmdEDUMemTypes memType )
   {
      clearRequestInfo() ;
      _pReqMsg = pReqMsg ;
      _memType = memType ;
   }

   MsgHeader* _pmdSubSession::getRspMsg( BOOLEAN owned )
   {
      if ( owned )
      {
         _event._dataMemType = PMD_EDU_MEM_NONE ;
      }
      return (MsgHeader*)_event._Data ;
   }

   void _pmdSubSession::setProcessInfo( INT32 processResult )
   {
      _processResult = processResult ;
      _isProcessed   = TRUE ;
   }

   void _pmdSubSession::clearProcessInfo()
   {
      _isProcessed   = FALSE ;
      _processResult = SDB_OK ;
   }

   void _pmdSubSession::setSendResult( BOOLEAN isSend )
   {
      _isSend = isSend ;
      if ( isSend )
      {
         _isDisconnect = FALSE ;
         _needToDel    = TRUE ;
         _hasStop      = FALSE ;
      }
   }

   void _pmdSubSession::resetForResend()
   {
      clearSend() ;
      clearReplyInfo() ;
      clearProcessInfo() ;
   }

   void _pmdSubSession::processEvent( pmdEDUEvent &event )
   {
      MsgHeader *pRsp = ( MsgHeader* )event._Data ;

      clearReplyInfo() ;

      if ( MSG_BS_DISCONNECT == pRsp->opCode )
      {
         _isDisconnect = TRUE ;
      }

      _event = event ;
      if ( PMD_EDU_MEM_NONE == event._dataMemType )
      {
         _event._Data = SDB_OSS_MALLOC( pRsp->messageLength ) ;
         if ( _event._Data )
         {
            ossMemcpy( _event._Data, pRsp, pRsp->messageLength ) ;
            _event._dataMemType = PMD_EDU_MEM_ALLOC ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to alloc memory[size: %d]",
                    pRsp->messageLength ) ;
         }
      }
      else
      {
         event._dataMemType = PMD_EDU_MEM_NONE ;
      }
   }

   /*
      _pmdSubSessionItr implement
   */
   _pmdSubSessionItr::_pmdSubSessionItr()
   {
      _pSessions = NULL ;
      _filter = PMD_SSITR_ALL ;
   }

   _pmdSubSessionItr::_pmdSubSessionItr( MAP_SUB_SESSION *pSessions,
                                         PMD_SSITR_FILTER filter )
   :_pSessions( pSessions ), _filter( filter )
   {
      if ( _pSessions )
      {
         _curPos = _pSessions->begin() ;
         _findPos() ;
      }
   }

   _pmdSubSessionItr::~_pmdSubSessionItr()
   {
      _pSessions = NULL ;
   }

   void _pmdSubSessionItr::_findPos()
   {
      if ( PMD_SSITR_ALL != _filter )
      {
         pmdSubSession *pSub = NULL ;
         while ( _curPos != _pSessions->end() )
         {
            pSub = &(_curPos->second) ;
            if ( PMD_SSITR_UNSENT == _filter && !pSub->isSend() )
            {
               break ;
            }
            else if ( PMD_SSITR_SENT == _filter && pSub->isSend() )
            {
               break ;
            }
            else if ( PMD_SSITR_UNREPLY == _filter && pSub->isSend() &&
                      !pSub->hasReply() )
            {
               break ;
            }
            else if ( PMD_SSITR_REPLY == _filter && pSub->hasReply() )
            {
               break ;
            }
            else if ( PMD_SSITR_UNPROCESSED == _filter && pSub->hasReply() &&
                      !pSub->isProcessed() )
            {
               break ;
            }
            else if ( PMD_SSITR_PROCESSED == _filter && pSub->isProcessed() )
            {
               break ;
            }
            else if ( PMD_SSITR_PROCESS_SUC == _filter &&
                      pSub->isProcessed() &&
                      SDB_OK == pSub->getProcessRet() )
            {
               break ;
            }
            else if ( PMD_SSITR_PROCESS_FAIL == _filter &&
                      pSub->isProcessed() &&
                      SDB_OK != pSub->getProcessRet() )
            {
               break ;
            }
            else if ( PMD_SSITR_DISCONNECT == _filter &&
                      pSub->isDisconnect() )
            {
               break ;
            }
            else if ( PMD_SSITR_CONNECT == _filter &&
                      !pSub->isDisconnect() )
            {
               break ;
            }
            ++_curPos ;
         }
      }
   }

   BOOLEAN _pmdSubSessionItr::more()
   {
      if ( !_pSessions || _curPos == _pSessions->end() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   pmdSubSession* _pmdSubSessionItr::next()
   {
      pmdSubSession *pSubSession = &(_curPos->second) ;
      ++_curPos ;
      _findPos() ;
      return pSubSession ;
   }

   /*
      _pmdRemoteSession implement
   */
   _pmdRemoteSession::_pmdRemoteSession( netRouteAgent *pAgent,
                                         UINT64 sessionID,
                                         _pmdRemoteSessionSite *pSite,
                                         INT64 timeout,
                                         IRemoteSessionHandler *pHandle )
   {
      _pAgent        = pAgent ;
      _pHandle       = pHandle ;
      _pEDUCB        = NULL ;
      _sessionID     = sessionID ;
      _pSite         = pSite ;
      _sessionChange = FALSE ;
      _userData      = 0 ;
      _totalWaitTime = 0 ;

      setTimeout( timeout ) ;
   }

   _pmdRemoteSession::_pmdRemoteSession()
   {
      _pAgent        = NULL ;
      _pHandle       = NULL ;
      _pEDUCB        = NULL ;
      _sessionID     = 0 ;
      _pSite         = NULL ;
      _sessionChange = FALSE ;
      _userData      = 0 ;

      setTimeout( -1 ) ;
   }

   _pmdRemoteSession::~_pmdRemoteSession()
   {
      if ( _pSite )
      {
         clear() ;
      }
      _pAgent        = NULL ;
      _pHandle       = NULL ;
      _pSite         = NULL ;
   }

   void _pmdRemoteSession::clear()
   {
      clearSubSession() ;
      _pHandle          = NULL ;
      _pSite            = NULL ;
   }

   void _pmdRemoteSession::reset( netRouteAgent *pAgent,
                                  UINT64 sessionID,
                                  _pmdRemoteSessionSite *pSite,
                                  INT64 timeout ,
                                  IRemoteSessionHandler *pHandle )
   {
      _pAgent           = pAgent ;
      _sessionID        = sessionID ;
      _pSite            = pSite ;
      _pHandle          = pHandle ;
      _totalWaitTime    = 0 ;

      setTimeout( timeout ) ;
   }

   pmdSubSession* _pmdRemoteSession::getSubSession( UINT64 nodeID )
   {
      MAP_SUB_SESSION_IT it = _mapSubSession.find( nodeID ) ;
      if ( it == _mapSubSession.end() )
      {
         return NULL ;
      }
      return &( it->second ) ;
   }

   void _pmdRemoteSession::delSubSession( UINT64 nodeID )
   {
      _mapPendingSubSession.erase( nodeID ) ;
      MAP_SUB_SESSION_IT it = _mapSubSession.find( nodeID ) ;
      if ( it != _mapSubSession.end() )
      {
         if ( it->second.isNeedToDel() )
         {
            stopSubSession( &(it->second) ) ;
            _pSite->delSubSession( it->second.getReqID() ) ;
            _pSite->removeAssitNode( it->second.getAddPos(),
                                     it->second.getNodeID().columns.nodeID ) ;
            it->second.setNeedToDel( FALSE ) ;
         }
         _mapSubSession.erase( it ) ;
      }
   }

   void _pmdRemoteSession::resetSubSession( UINT64 nodeID )
   {
      _mapPendingSubSession.erase( nodeID ) ;
      MAP_SUB_SESSION_IT it = _mapSubSession.find( nodeID ) ;
      if ( it != _mapSubSession.end() )
      {
         if ( it->second.isNeedToDel() )
         {
            stopSubSession( &(it->second) ) ;
            _pSite->delSubSession( it->second.getReqID() ) ;
            _pSite->removeAssitNode( it->second.getAddPos(),
                                     it->second.getNodeID().columns.nodeID ) ;
            it->second.setNeedToDel( FALSE ) ;
         }
         it->second.resetForResend() ;
         it->second.clearRequestInfo() ;
      }
   }

   void _pmdRemoteSession::clearSubSession()
   {
      _mapPendingSubSession.clear() ;

      stopSubSession() ;

      MAP_SUB_SESSION_IT it = _mapSubSession.begin() ;
      while ( it != _mapSubSession.end() )
      {
         if ( it->second.isNeedToDel() )
         {
            _pSite->delSubSession( it->second.getReqID() ) ;
            _pSite->removeAssitNode( it->second.getAddPos(),
                                     it->second.getNodeID().columns.nodeID ) ;
            it->second.setNeedToDel( FALSE ) ;
         }
         ++it ;
      }
      _mapSubSession.clear() ;
   }

   void _pmdRemoteSession::resetAllSubSession()
   {
      _mapPendingSubSession.clear() ;

      stopSubSession() ;

      MAP_SUB_SESSION_IT it = _mapSubSession.begin() ;
      while ( it != _mapSubSession.end() )
      {
         if ( it->second.isNeedToDel() )
         {
            _pSite->delSubSession( it->second.getReqID() ) ;
            _pSite->removeAssitNode( it->second.getAddPos(),
                                     it->second.getNodeID().columns.nodeID ) ;
            it->second.setNeedToDel( FALSE ) ;
         }
         it->second.resetForResend() ;
         it->second.clearRequestInfo() ;
         ++it ;
      }
   }

   void _pmdRemoteSession::stopSubSession()
   {
      INT32 rc = SDB_OK ;
      pmdSubSession *pSubSession = NULL ;
      MsgHeader interruptMsg ;
      interruptMsg.messageLength = sizeof( MsgHeader ) ;
      interruptMsg.opCode = MSG_BS_INTERRUPTE_SELF ;

      pmdSubSessionItr itr = getSubSessionItr( PMD_SSITR_UNREPLY ) ;
      while ( itr.more() )
      {
         pSubSession = itr.next() ;
         if ( pSubSession->hasStop() )
         {
            continue ;
         }
         rc = postMsg( &interruptMsg, pSubSession ) ;
         if ( SDB_OK == rc )
         {
            pSubSession->setStop( TRUE ) ;
         }
      }
   }

   void _pmdRemoteSession::stopSubSession( pmdSubSession *pSub )
   {
      if ( pSub->isSend() && !pSub->hasReply() && !pSub->hasStop() )
      {
         INT32 rc = SDB_OK ;
         MsgHeader interruptMsg ;
         interruptMsg.messageLength = sizeof( MsgHeader ) ;
         interruptMsg.opCode = MSG_BS_INTERRUPTE_SELF ;
         rc = postMsg( &interruptMsg, pSub ) ;
         if ( rc )
         {
            pSub->setStop( TRUE ) ;
         }
      }
   }

   UINT32 _pmdRemoteSession::getSubSessionCount( PMD_SSITR_FILTER filter )
   {
      UINT32 count = 0 ;
      if ( PMD_SSITR_ALL == filter )
      {
         count = _mapSubSession.size() ;
      }
      else
      {
         pmdSubSessionItr itr = getSubSessionItr( filter ) ;

         while ( itr.more() )
         {
            itr.next() ;
            ++count ;
         }
      }
      return count ;
   }

   BOOLEAN _pmdRemoteSession::isTimeout() const
   {
      return _milliTimeout <= 0 ? TRUE : FALSE ;
   }

   INT64 _pmdRemoteSession::getMilliTimeout () const
   {
      return _milliTimeout ;
   }

   INT64 _pmdRemoteSession::getTotalWaitTime() const
   {
      return _totalWaitTime ;
   }

   BOOLEAN _pmdRemoteSession::isAllReply()
   {
      BOOLEAN ret = TRUE ;
      pmdSubSession *pSub = NULL ;
      MAP_SUB_SESSION_IT it = _mapSubSession.begin() ;
      while ( it != _mapSubSession.end() )
      {
         pSub = &(it->second) ;
         ++it ;
         if ( pSub->isSend() && !pSub->hasReply() )
         {
            ret = FALSE ;
            break ;
         }
      }

      return ret ;
   }

   void _pmdRemoteSession::setTimeout( INT64 timeout )
   {
      if ( timeout <= 0 )
      {
         _milliTimeoutHard = 0x7FFFFFFFFFFFFFFF ;
      }
      else
      {
         _milliTimeoutHard = timeout ;
      }
      _milliTimeout = _milliTimeoutHard ;
   }

   pmdSubSessionItr _pmdRemoteSession::getSubSessionItr( PMD_SSITR_FILTER filter )
   {
      return pmdSubSessionItr( &_mapSubSession, filter ) ;
   }

   pmdSubSession* _pmdRemoteSession::addSubSession( UINT64 nodeID )
   {
      pmdSubSession &subSession = _mapSubSession[ nodeID ] ;
      if ( subSession.getNodeIDUInt() != nodeID )
      {
         subSession.setNodeID( nodeID ) ;
         subSession.setParent( this ) ;
         subSession._handle = _pSite->getNodeNet( nodeID ) ;
      }
      return &subSession ;
   }

   INT32 _pmdRemoteSession::sendMsg( MsgHeader * pSrcMsg,
                                     pmdEDUMemTypes memType,
                                     INT32 *pSucNum,
                                     INT32 *pTotalNum )
   {
      INT32 rc = SDB_OK ;
      VEC_SUB_SESSIONPTR vecFailedSession ;
      vector< INT32 > vecFailedFlag ;
      pmdSubSession *pSub = NULL ;

      if ( pTotalNum )
      {
         *pTotalNum = 0 ;
      }
      if ( pSucNum )
      {
         *pSucNum = 0 ;
      }

      pmdSubSessionItr itr = getSubSessionItr( PMD_SSITR_UNSENT ) ;
      while ( itr.more() )
      {
         pSub = itr.next() ;
         if ( pSrcMsg )
         {
            pSub->setReqMsg( pSrcMsg ) ;
         }
         rc = sendMsg( pSub ) ;

         if ( pTotalNum )
         {
            ++(*pTotalNum) ;
         }
         if ( SDB_OK == rc )
         {
            if ( pSucNum )
            {
               ++(*pSucNum) ;
            }
            if ( pSrcMsg && PMD_EDU_MEM_NONE != memType )
            {
               pSub->setReqMsgMemType( memType ) ;
               memType = PMD_EDU_MEM_NONE ;
            }
         }
         else
         {
            vecFailedSession.push_back( pSub ) ;
            vecFailedFlag.push_back( rc ) ;

            if ( !pSucNum && !_pHandle )
            {
               goto error ;
            }
         }
      }

      if ( !_pHandle )
      {
         if ( vecFailedFlag.size() > 0 )
         {
            rc = vecFailedFlag[ 0 ] ;
            goto error ;
         }
         goto done ;
      }
      else
      {
         INT32 rcTmp = SDB_OK ;
         UINT64 nodeID = 0 ;
         for ( UINT32 i = 0 ; i < vecFailedSession.size() ; ++i )
         {
            pSub = vecFailedSession[ i ] ;
            nodeID = pSub->getNodeIDUInt() ;
            rcTmp = vecFailedFlag[ i ] ;
            rc = _pHandle->onSendFailed( this, &pSub, rcTmp ) ;
            PD_RC_CHECK( rc, PDERROR, "Session[%s] send msg to node[%s] "
                         "failed[rc: %d] and processed failed[rc: %d]",
                         _pEDUCB->toString().c_str(),
                         routeID2String(nodeID).c_str(),
                         rcTmp, rc ) ;
            if ( pSucNum )
            {
               ++(*pSucNum) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRemoteSession::sendMsg( MsgHeader *pSrcMsg,
                                     SET_NODEID &subs,
                                     pmdEDUMemTypes memType,
                                     INT32 *pSucNum,
                                     INT32 *pTotalNum )
   {
      INT32 rc = SDB_OK ;
      pmdSubSession *pSub = NULL ;
      VEC_SUB_SESSIONPTR vecFailedSession ;
      vector< INT32 > vecFailedFlag ;
      SET_NODEID::iterator it = subs.begin() ;
      UINT64 nodeID = 0 ;

      if ( pTotalNum )
      {
         *pTotalNum = 0 ;
      }
      if ( pSucNum )
      {
         *pSucNum = 0 ;
      }

      while ( it != subs.end() )
      {
         nodeID = *it ;
         ++it ;
         pSub = addSubSession( nodeID ) ;
         if ( pSub )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Session[%s] failed to add sub session[%s]",
                    _pEDUCB->toString().c_str(),
                    routeID2String(nodeID).c_str() ) ;
            goto error ;
         }
         if ( pSub->isSend() )
         {
            continue ;
         }
         pSub->setReqMsg( pSrcMsg ) ;
         rc = sendMsg( pSub ) ;

         if ( pTotalNum )
         {
            ++(*pTotalNum) ;
         }
         if ( SDB_OK == rc )
         {
            if ( pSucNum )
            {
               ++(*pSucNum) ;
            }
            if ( pSrcMsg && memType != PMD_EDU_MEM_NONE )
            {
               pSub->setReqMsgMemType( memType ) ;
               memType = PMD_EDU_MEM_NONE ;
            }
         }
         else if ( SDB_OK != rc )
         {
            vecFailedSession.push_back( pSub ) ;
            vecFailedFlag.push_back( rc ) ;

            if ( !pSucNum && !_pHandle )
            {
               goto error ;
            }
         }
      }

      if ( !_pHandle )
      {
         if ( vecFailedFlag.size() > 0 )
         {
            rc = vecFailedFlag[ 0 ] ;
            goto error ;
         }
         goto done ;
      }
      else
      {
         INT32 rcTmp = SDB_OK ;
         for ( UINT32 i = 0 ; i < vecFailedSession.size() ; ++i )
         {
            pSub = vecFailedSession[ i ] ;
            nodeID = pSub->getNodeIDUInt() ;
            rcTmp = vecFailedFlag[ i ] ;
            rc = _pHandle->onSendFailed( this, &pSub, rcTmp ) ;
            PD_RC_CHECK( rc, PDERROR, "Session[%s] send msg to node[%s] "
                         "failed[rc: %d] and processed failed[rc: %d]",
                         _pEDUCB->toString().c_str(),
                         routeID2String(nodeID).c_str(),
                         rcTmp, rc ) ;
            if ( pSucNum )
            {
               ++(*pSucNum) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRemoteSession::sendMsg( INT32 *pSucNum, INT32 *pTotalNum )
   {
      return sendMsg( NULL, PMD_EDU_MEM_NONE, pSucNum, pTotalNum ) ;
   }

   INT32 _pmdRemoteSession::sendMsg( UINT64 nodeID )
   {
      INT32 rc = SDB_OK ;
      pmdSubSession *pSub = getSubSession( nodeID ) ;
      if ( !pSub )
      {
         PD_LOG( PDERROR, "Session[%s] can't find sub session[%s]",
                 _pEDUCB->toString().c_str(), routeID2String(nodeID).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      rc = sendMsg( pSub ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRemoteSession::sendMsg( pmdSubSession *pSub )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldReqID = 0 ;
      INT32 oldAddPos = 0 ;
      BOOLEAN hasSend = FALSE ;

      if ( !pSub )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !pSub->getReqMsg() )
      {
         PD_LOG( PDERROR, "Session[%s] request msg header is NULL",
                 _pEDUCB->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( pSub->isSend() )
      {
         PD_LOG( PDWARNING, "Session[%s] has already send msg to "
                 "node[%s]", _pEDUCB->toString().c_str(),
                 routeID2String(pSub->getNodeID()).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      oldReqID = pSub->getReqID() ;
      oldAddPos = *pSub->getAddPos() ;
      pSub->setReqID( _pEDUCB->incCurRequestID() ) ;
      pSub->getReqMsg()->requestID = pSub->getReqID() ;
      pSub->getReqMsg()->routeID.value = MSG_INVALID_ROUTEID ;
      pSub->getReqMsg()->TID = _pEDUCB->getTID() ;
      *pSub->getAddPos() = _pSite->addAssitNode(
         pSub->getNodeID().columns.nodeID ) ;

      if ( NET_INVALID_HANDLE == pSub->getHandle() && _pHandle )
      {
         rc = _pHandle->onSendConnect( pSub, pSub->getReqMsg(), TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s] onSendConnect failed, rc: %d",
                    _pEDUCB->toString().c_str(), rc ) ;
            _pSite->removeNodeNet( pSub->getNodeIDUInt() ) ;
            goto error ;
         }
      }

      if ( NET_INVALID_HANDLE != pSub->getHandle() )
      {
         if ( pSub->getIODatas()->size() > 0 )
         {
            pSub->getReqMsg()->messageLength = sizeof( MsgHeader ) +
                                               pSub->getIODataLen() ;
            rc = _pAgent->syncSendv( pSub->getHandle(), pSub->getReqMsg(),
                                     *(pSub->getIODatas()) ) ;
         }
         else
         {
            rc = _pAgent->syncSend( pSub->getHandle(),
                                    (void*)pSub->getReqMsg() ) ;
         }

         if ( SDB_OK == rc )
         {
            hasSend = TRUE ;
         }
         else if ( SDB_NET_INVALID_HANDLE != rc )
         {
            PD_LOG( PDERROR, "Session[%s] send msg to node[%s] failed, "
                    "rc: %d", _pEDUCB->toString().c_str(),
                    routeID2String(pSub->getNodeID()).c_str(), rc ) ;
            goto error ;
         }
         else
         {
            _pSite->removeNodeNet( pSub->getNodeIDUInt() ) ;
            if ( _pHandle )
            {
               rc = _pHandle->onSendConnect( pSub, pSub->getReqMsg(), FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Session[%s] onSendConnect failed, rc: %d",
                          _pEDUCB->toString().c_str(), rc ) ;
                  _pSite->removeNodeNet( pSub->getNodeIDUInt() ) ;
                  goto error ;
               }
            }
            rc = SDB_OK ;
         }
      }

      if ( !hasSend )
      {
         if ( pSub->getIODatas()->size() > 0 )
         {
            pSub->getReqMsg()->messageLength = sizeof( MsgHeader ) +
                                               pSub->getIODataLen() ;
            rc = _pAgent->syncSendv( pSub->getNodeID(), pSub->getReqMsg(),
                                     *(pSub->getIODatas()),
                                     &(pSub->_handle) ) ;
         }
         else
         {
            rc = _pAgent->syncSend( pSub->getNodeID(),
                                    (void*)pSub->getReqMsg(),
                                    &(pSub->_handle) ) ;
         }

         PD_RC_CHECK( rc, PDERROR, "Session[%s] send msg to node[%s] failed, "
                      "rc: %d", _pEDUCB->toString().c_str(),
                      routeID2String(pSub->getNodeID()).c_str(), rc ) ;
         hasSend = TRUE ;
         _pSite->addNodeNet( pSub->getNodeIDUInt(), pSub->getHandle() ) ;
      }

      if ( pSub->isNeedToDel() )
      {
         stopSubSession( pSub ) ;
         _pSite->delSubSession( oldReqID ) ;
         _pSite->removeAssitNode( &oldAddPos,
                                  pSub->getNodeID().columns.nodeID ) ;
      }
      _sessionChange = TRUE ;
      pSub->setSendResult( TRUE ) ;
      _pSite->addSubSession( pSub ) ;

   done:
      return rc ;
   error:
      if ( pSub )
      {
         _pSite->removeAssitNode( pSub->getAddPos(),
                                  pSub->getNodeID().columns.nodeID ) ;
      }
      goto done ;
   }

   INT32 _pmdRemoteSession::waitReply( BOOLEAN waitAll,
                                       VEC_SUB_SESSIONPTR *pSubs )
   {
      INT32 rc = SDB_OK ;

      if ( !pSubs )
      {
         rc = waitReply1( waitAll, NULL ) ;
      }
      else
      {
         MAP_SUB_SESSIONPTR mapSessionPtrs ;
         rc = waitReply1( waitAll, &mapSessionPtrs ) ;
         if ( SDB_OK == rc )
         {
            MAP_SUB_SESSIONPTR_IT it = mapSessionPtrs.begin() ;
            while ( it != mapSessionPtrs.end() )
            {
               pSubs->push_back( it->second ) ;
               ++it ;
            }
         }
      }

      return rc ;
   }

   INT32 _pmdRemoteSession::waitReply1( BOOLEAN waitAll,
                                        MAP_SUB_SESSIONPTR *pSubs )
   {
      INT32 rc                      = SDB_OK ;
      pmdEDUEvent event ;
      INT64 timeout                 = OSS_ONE_SEC ;
      UINT32 totalUnReplyNum        = 0 ;
      UINT32 replyNum               = 0 ;
      pmdSubSession *pSubSession    = NULL ;
      _sessionChange                = FALSE ;

      _milliTimeout = _milliTimeoutHard ;
      totalUnReplyNum = getSubSessionCount( PMD_SSITR_UNREPLY ) ;
      while ( totalUnReplyNum > 0 || _mapPendingSubSession.size() > 0 )
      {
         if ( _mapPendingSubSession.size() > 0 )
         {
            MAP_SUB_SESSIONPTR_IT itPending = _mapPendingSubSession.begin() ;
            while ( itPending != _mapPendingSubSession.end() )
            {
               if ( pSubs )
               {
                  (*pSubs)[ itPending->first ] = itPending->second ;
               }
               ++itPending ;
               ++replyNum ;
            }
            _mapPendingSubSession.clear() ;
            continue ;
         }

         if ( _pEDUCB->isInterrupted() )
         {
            if ( _pEDUCB->isOnlySelfWhenInterrupt() )
            {
               stopSubSession() ;
            }
            else
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
         }

         if ( !waitAll && replyNum > 0 )
         {
            timeout = 1 ;
         }
         else
         {
            timeout = _milliTimeout < OSS_ONE_SEC ?
                      _milliTimeout : OSS_ONE_SEC ;
         }

         if ( !_pEDUCB->waitEvent( event, timeout ) )
         {
            _milliTimeout -= timeout ;
            if ( 0 == replyNum || waitAll )
            {
               if ( _milliTimeout <= 0 )
               {
                  rc = SDB_TIMEOUT ;
                  goto error ;
               }
            }
            else
            {
               if ( _milliTimeout <= 0 )
               {
                  _milliTimeout = 1 ;
               }
               goto done ;
            }
            continue ;
         }

         if ( PMD_EDU_EVENT_MSG != event._eventType )
         {
            PD_LOG( PDWARNING, "Session[%s] recv unknonw event[type: %d]",
                    _pEDUCB->toString().c_str(), event._eventType ) ;
            pmdEduEventRelase( event, _pEDUCB ) ;
            event.reset() ;
            continue ;
         }

         pSubSession = NULL ;
         rc = _pSite->processEvent( event, _mapSubSession, &pSubSession,
                                    _pHandle ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process event, rc: %d", rc ) ;

         if ( pSubSession )
         {
            ++replyNum ;
            --totalUnReplyNum ;
            if ( pSubs )
            {
               (*pSubs)[ pSubSession->getNodeIDUInt() ] = pSubSession ;
            }
         }

         if ( _sessionChange )
         {
            totalUnReplyNum = getSubSessionCount( PMD_SSITR_UNREPLY ) ;
            _sessionChange = FALSE ;
         }
      }

   done:
      _totalWaitTime += ( _milliTimeoutHard - _milliTimeout ) ;
      return rc ;
   error:
      goto done ;
   }

   void _pmdRemoteSession::addPending( pmdSubSession *pSubSession )
   {
      _mapPendingSubSession[ pSubSession->getNodeIDUInt() ] = pSubSession ;
      _sessionChange = TRUE ;
   }

   INT32 _pmdRemoteSession::postMsg( MsgHeader * pMsg, UINT64 nodeID )
   {
      INT32 rc = SDB_OK ;
      pmdSubSession *pSub = getSubSession( nodeID ) ;
      if ( !pSub )
      {
         PD_LOG( PDERROR, "Session[%s] can't find sub session[%s]",
                 _pEDUCB->toString().c_str(), routeID2String(nodeID).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      rc = postMsg( pMsg, pSub ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRemoteSession::postMsg( MsgHeader *pMsg, pmdSubSession *pSub )
   {
      INT32 rc = SDB_OK ;

      if ( !pMsg || !pSub )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NET_INVALID_HANDLE == pSub->getHandle() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      pMsg->requestID = _pEDUCB->incCurRequestID() ;
      pMsg->routeID.value = MSG_INVALID_ROUTEID ;
      pMsg->TID = _pEDUCB->getTID() ;

      rc = _pAgent->syncSend( pSub->getHandle(), (void*)pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Session[%s] send msg to node[%s] failed, "
                 "rc: %d", _pEDUCB->toString().c_str(),
                 routeID2String(pSub->getNodeID()).c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _pmdRemoteSessionSite implement
   */
   _pmdRemoteSessionSite::_pmdRemoteSessionSite()
   {
      _pEDUCB = NULL ;
      _pAgent = NULL ;
      _pLatch = NULL ;

      ossMemset( _assitNodeBuff, 0, sizeof( _assitNodeBuff ) ) ;

      _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE ]._pos = 0 ;
      for ( UINT32 i = 0 ; i < PMD_SITE_NODEID_BUFF_SIZE ; ++i )
      {
         _assitNodeBuff[ i ]._pos = i + 1 ;
      }
      _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE - 1 ]._pos = -1 ;

      _sessionHWNum = 1 ;
      _curPos = -1 ;

      _userData = 0 ;
   }

   _pmdRemoteSessionSite::~_pmdRemoteSessionSite()
   {
      SDB_ASSERT( 0 == _mapSession.size(), "Has session not removed" ) ;
      SDB_ASSERT( NULL == _pEDUCB, "EDU is not NULL" ) ;
      _pEDUCB = NULL ;

      if ( _pLatch )
      {
         SDB_OSS_DEL _pLatch ;
      }
   }

   void _pmdRemoteSessionSite::addSubSession( pmdSubSession *pSub )
   {
      _mapReq2SubSession[ pSub->getReqID() ] = pSub ;
   }

   void _pmdRemoteSessionSite::delSubSession( UINT64 reqID )
   {
      _mapReq2SubSession.erase( reqID ) ;
   }

   void _pmdRemoteSessionSite::addNodeNet( UINT64 nodeID, NET_HANDLE handle )
   {
      _mapNode2Net[ nodeID ] = handle ;
   }

   void _pmdRemoteSessionSite::removeNodeNet( UINT64 nodeID )
   {
      _mapNode2Net.erase( nodeID ) ;
   }

   NET_HANDLE _pmdRemoteSessionSite::getNodeNet( UINT64 nodeID )
   {
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_NODE2NET::iterator it = _mapNode2Net.find( nodeID ) ;
      if ( it != _mapNode2Net.end() )
      {
         handle = it->second ;
      }
      return handle ;
   }

   INT32 _pmdRemoteSessionSite::addAssitNode( UINT16 nodeID )
   {
      INT32 pos = -1 ;
      if ( -1 != _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE ]._pos )
      {
         pos = _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE ]._pos ;
         _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE ]._pos =
            _assitNodeBuff[ pos ]._pos ;
         _assitNodeBuff[ pos ]._nodeID = nodeID ;
      }
      else
      {
         if ( !_pLatch )
         {
            _pLatch = SDB_OSS_NEW ossSpinSLatch ;
            SDB_ASSERT( _pLatch, "Alloc latch failed" ) ;
         }
         if ( _pLatch )
         {
            _pLatch->get() ;
         }
         _assitNodes.insert( nodeID ) ;
         if ( _pLatch )
         {
            _pLatch->release() ;
         }
      }
      return pos ;
   }

   void _pmdRemoteSessionSite::removeAssitNode( INT32 *pos, UINT16 nodeID )
   {
      if ( *pos >= 0 && *pos < PMD_SITE_NODEID_BUFF_SIZE )
      {
         _assitNodeBuff[ *pos ]._nodeID = 0 ;
         _assitNodeBuff[ *pos ]._pos =
            _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE ]._pos ;
         _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE ]._pos = *pos ;
         *pos = -1 ;
      }
      else
      {
         BOOLEAN bLock = FALSE ;
         if ( _pLatch )
         {
            _pLatch->get() ;
            bLock = TRUE ;
         }
         _assitNodes.erase( nodeID ) ;
         if ( bLock )
         {
            _pLatch->release() ;
         }
      }
   }

   BOOLEAN _pmdRemoteSessionSite::existNode( UINT16 nodeID )
   {
      BOOLEAN bFound = FALSE ;
      BOOLEAN bLock = FALSE ;

      for ( UINT32 i = 0 ; i < PMD_SITE_NODEID_BUFF_SIZE ; ++i )
      {
         if ( _assitNodeBuff[ i ]._nodeID == nodeID )
         {
            bFound = TRUE ;
            goto done ;
         }
      }

      if ( _pLatch )
      {
         _pLatch->get_shared() ;
         bLock = TRUE ;
      }
      if ( _assitNodes.count( nodeID ) > 0 )
      {
         bFound = TRUE ;
      }
      if ( bLock )
      {
         _pLatch->release_shared() ;
      }

   done:
      return bFound ;
   }

   void _pmdRemoteSessionSite::handleClose( const NET_HANDLE & handle,
                                            const _MsgRouteID & id )
   {
      if ( !eduCB()->isTransaction() &&
           FALSE == existNode( id.columns.nodeID ) )
      {
         goto done ;
      }
      else
      {
         MsgOpReply *pMsg = NULL ;
         pMsg = ( MsgOpReply* )SDB_OSS_MALLOC( sizeof( MsgOpReply ) ) ;
         if ( pMsg )
         {
            pMsg->header.messageLength = sizeof( MsgOpReply ) ;
            pMsg->header.opCode = MSG_BS_DISCONNECT ;
            pMsg->header.requestID = eduCB()->incCurRequestID() ;
            pMsg->header.TID = eduCB()->getTID() ;
            pMsg->header.routeID.value = id.value ;
            pMsg->contextID = -1 ;
            pMsg->flags = SDB_COORD_REMOTE_DISC ;
            pMsg->numReturned = 0 ;
            pMsg->startFrom = 0 ;

            eduCB()->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                             PMD_EDU_MEM_ALLOC,
                                             (CHAR*)pMsg,
                                             (UINT64)handle ) ) ;
         }
      }

   done:
      return ;
   }

   void _pmdRemoteSessionSite::interruptAllSubSession()
   {
      MsgHeader interruptMsg ;
      interruptMsg.messageLength = sizeof( MsgHeader ) ;
      interruptMsg.opCode = MSG_BS_INTERRUPTE ;
      interruptMsg.requestID = eduCB()->incCurRequestID() ;
      interruptMsg.routeID.value = MSG_INVALID_ROUTEID ;
      interruptMsg.TID = eduCB()->getTID() ;

      MAP_NODE2NET::iterator it = _mapNode2Net.begin() ;
      while ( it != _mapNode2Net.end() )
      {
         _pAgent->syncSend( it->second, (void*)&interruptMsg ) ;
         ++it ;
      }
   }

   void _pmdRemoteSessionSite::disconnectAllSubSession()
   {
      MsgHeader disconnectMsg ;
      disconnectMsg.messageLength = sizeof( MsgHeader ) ;
      disconnectMsg.opCode = MSG_BS_DISCONNECT ;
      disconnectMsg.requestID = eduCB()->incCurRequestID() ;
      disconnectMsg.routeID.value = MSG_INVALID_ROUTEID ;
      disconnectMsg.TID = eduCB()->getTID() ;

      MAP_NODE2NET::iterator it = _mapNode2Net.begin() ;
      while ( it != _mapNode2Net.end() )
      {
         _pAgent->syncSend( it->second, (void*)&disconnectMsg ) ;
         ++it ;
      }
   }

   UINT32 _pmdRemoteSessionSite::getAllNodeID( SET_NODEID &setNodes )
   {
      UINT32 count = 0 ;
      MAP_NODE2NET::iterator it = _mapNode2Net.begin() ;
      while( it != _mapNode2Net.end() )
      {
         setNodes.insert( it->first ) ;
         ++it ;
         ++count ;
      }
      return count ;
   }

   INT32 _pmdRemoteSessionSite::processEvent( pmdEDUEvent &event,
                                              MAP_SUB_SESSION &mapSessions,
                                              pmdSubSession **ppSub,
                                              IRemoteSessionHandler *pHandle )
   {
      INT32 rc = SDB_OK ;
      MAP_SUB_SESSION_IT it ;
      MAP_SUB_SESSIONPTR_IT itPtr ;
      MsgHeader *pReply = NULL ;
      UINT64 nodeID = 0 ;
      pmdSubSession *pSubSession = NULL ;
      NET_HANDLE handle = (NET_HANDLE)event._userData ;

      SDB_ASSERT( ppSub, "ppSub can't be NULL" ) ;

      if ( !event._Data )
      {
         PD_LOG( PDWARNING, "Session[%s] msg event data is NULL",
                 _pEDUCB->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pReply = ( MsgHeader* )event._Data ;
      nodeID = pReply->routeID.value ;

      if ( MSG_BS_DISCONNECT == pReply->opCode )
      {
         MAP_SUB_SESSIONPTR disSubs ;

         if ( eduCB()->isTransNode( pReply->routeID ) )
         {
            eduCB()->setTransRC( ((MsgOpReply*)pReply)->flags ) ;
            PD_LOG( PDERROR, "Session[%s] disconnect with node[%s] in "
                    "transaction", _pEDUCB->toString().c_str(),
                    routeID2String(pReply->routeID).c_str() ) ;
         }

         itPtr = _mapReq2SubSession.begin() ;
         while ( itPtr != _mapReq2SubSession.end() )
         {
            if ( pReply->requestID < itPtr->first )
            {
               break ;
            }
            else if ( itPtr->second->getNodeIDUInt() == nodeID &&
                      itPtr->second->getHandle() == handle )
            {
               pSubSession = itPtr->second ;
               pSubSession->processEvent( event ) ;
               pSubSession->setNeedToDel( FALSE ) ;
               if ( !*ppSub && ( it = mapSessions.find( nodeID ) ) !=
                    mapSessions.end() && &(it->second) == pSubSession )
               {
                  *ppSub = pSubSession ;
               }
               else
               {
                  pSubSession->parent()->addPending( pSubSession ) ;
               }
               _mapReq2SubSession.erase( itPtr++ ) ;
               removeAssitNode( pSubSession->getAddPos(),
                                pSubSession->getNodeID().columns.nodeID ) ;

               if ( pHandle )
               {
                  disSubs[ pSubSession->getNodeIDUInt() ] = pSubSession ;
               }
               continue ;
            }
            ++itPtr ;
         }

         if ( pHandle && !disSubs.empty() )
         {
            MAP_SUB_SESSIONPTR_IT disSubPtr = disSubs.begin() ;
            while ( disSubPtr != disSubs.end() )
            {
               pSubSession = disSubPtr->second ;
               ++disSubPtr ;
               if ( pSubSession == *ppSub )
               {
                  pHandle->onReply( pSubSession->parent(), ppSub,
                                    pSubSession->getRspMsg( FALSE ),
                                    FALSE ) ;
               }
               else
               {
                  pHandle->onReply( pSubSession->parent(), &pSubSession,
                                    pSubSession->getRspMsg( FALSE ),
                                    TRUE ) ;
               }
            }
         }
      }
      else
      {
         itPtr = _mapReq2SubSession.find( pReply->requestID ) ;
         if ( itPtr != _mapReq2SubSession.end() )
         {
            pSubSession = itPtr->second ;
            pSubSession->processEvent( event ) ;
            pSubSession->setNeedToDel( FALSE ) ;
            nodeID = pSubSession->getNodeIDUInt() ;
            if ( !*ppSub && ( it = mapSessions.find( nodeID ) ) !=
                 mapSessions.end() && &(it->second) == pSubSession )
            {
               *ppSub = pSubSession ;
            }
            else
            {
               pSubSession->parent()->addPending( pSubSession ) ;
            }
            _mapReq2SubSession.erase( itPtr ) ;
            removeAssitNode( pSubSession->getAddPos(),
                             pSubSession->getNodeID().columns.nodeID ) ;

            if ( pHandle )
            {
               if ( pSubSession == *ppSub )
               {
                  pHandle->onReply( pSubSession->parent(), ppSub,
                                    pSubSession->getRspMsg( FALSE ),
                                    FALSE ) ;
               }
               else
               {
                  pHandle->onReply( pSubSession->parent(), &pSubSession,
                                    pSubSession->getRspMsg( FALSE ),
                                    TRUE ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Session[%s] recv expired msg[opCode: (%d)%u, "
                    "ReqID: %lld, Len: %d, NodeID: %s]",
                    _pEDUCB->toString().c_str(), IS_REPLY_TYPE(pReply->opCode),
                    GET_REQUEST_TYPE(pReply->opCode), pReply->requestID,
                    pReply->messageLength,
                    routeID2String(pReply->routeID).c_str() ) ;

            if ( pHandle )
            {
               pHandle->onExpiredReply( this, pReply ) ;
            }
         }
      }

   done:
      pmdEduEventRelase( event, _pEDUCB ) ;
      return rc ;
   error:
      goto done ;
   }

   pmdRemoteSession* _pmdRemoteSessionSite::addSession( INT64 timeout,
                                                        IRemoteSessionHandler *pHandle )
   {
      pmdRemoteSession *pSession = NULL ;
      UINT64 sessionID = 0 ;

      sessionID = ( ( (UINT64)_pEDUCB->getTID() ) << 32 ) | _sessionHWNum ;
      ++_sessionHWNum ;

      if ( _curPos < 0 )
      {
         _curSession.clear() ;
         _curPos = 0 ;
         pSession = &_curSession ;
      }
      else
      {
         pmdRemoteSession &tmpSession = _mapSession[ sessionID ] ;
         pSession = &tmpSession ;
      }

      pSession->reset( _pAgent, sessionID, this, timeout, pHandle ) ;
      pSession->attachCB( _pEDUCB ) ;

      return pSession ;
   }

   pmdRemoteSession* _pmdRemoteSessionSite::getSession( UINT64 sessionID )
   {
      pmdRemoteSession *pSession = NULL ;

      if ( 0 == _curPos && sessionID == _curSession.sessionID() )
      {
         pSession = &_curSession ;
      }
      else
      {
         MAP_REMOTE_SESSION_IT it = _mapSession.find( sessionID ) ;
         if ( it != _mapSession.end() )
         {
            pSession = &(it->second) ;
         }
      }

      return pSession ;
   }

   void _pmdRemoteSessionSite::removeSession( UINT64 sessionID )
   {
      if ( 0 == _curPos && sessionID == _curSession.sessionID() )
      {
         _curPos = -1 ;
         _curSession.clear() ;
      }
      else
      {
         _mapSession.erase( sessionID ) ;
      }
   }

   void _pmdRemoteSessionSite::removeSession( pmdRemoteSession *pSession )
   {
      removeSession( pSession->sessionID() ) ;
   }

   UINT32 _pmdRemoteSessionSite::sessionCount()
   {
      UINT32 num = 0 ;
      if ( 0 == _curPos )
      {
         ++num ;
      }
      num += _mapSession.size() ;
      return num ;
   }

   /*
      _pmdRemoteSessionMgr implement
   */
   _pmdRemoteSessionMgr::_pmdRemoteSessionMgr()
   {
      _pHandle= NULL ;
      _pAgent = NULL ;
   }

   _pmdRemoteSessionMgr::~_pmdRemoteSessionMgr()
   {
      _mapTID2EDU.clear() ;
      _pAgent = NULL ;
   }

   INT32 _pmdRemoteSessionMgr::init( netRouteAgent * pAgent,
                                     IRemoteMgrHandle *pHandle )
   {
      if ( !pAgent )
      {
         return SDB_INVALIDARG ;
      }
      _pAgent = pAgent ;
      _pHandle = pHandle ;

      return SDB_OK ;
   }

   INT32 _pmdRemoteSessionMgr::fini()
   {
      SDB_ASSERT( _mapTID2EDU.size() == 0, "EDU must be Zero" ) ;

      return SDB_OK ;
   }

   pmdRemoteSessionSite* _pmdRemoteSessionMgr::registerEDU( _pmdEDUCB * cb )
   {
      ossScopedLock lock( &_edusLatch, EXCLUSIVE ) ;
      pmdRemoteSessionSite &site = _mapTID2EDU[ cb->getTID() ] ;
      site.setEduCB( cb ) ;
      site.setRouteAgent( _pAgent ) ;
      cb->attachRemoteSite( &site ) ;

      if ( _pHandle )
      {
         _pHandle->onRegister( &site, cb ) ;
      }
      return &site ;
   }

   void _pmdRemoteSessionMgr::unregEUD( _pmdEDUCB * cb )
   {
      pmdRemoteSessionSite *pSite = NULL ;
      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
#ifdef _DEBUG
      SDB_ASSERT( pSite == getSite( cb ), "Site is not the same" ) ;
#endif //_DEBUG
      if ( pSite )
      {
         pSite->disconnectAllSubSession() ;

         _edusLatch.get() ;
         if ( _pHandle )
         {
            _pHandle->onUnreg( pSite, cb ) ;
         }
         pSite->setEduCB( NULL ) ;
         _mapTID2EDU.erase( cb->getTID() ) ;
         _edusLatch.release() ;
      }

      cb->detachRemoteSite() ;
   }

   netRouteAgent* _pmdRemoteSessionMgr::getAgent()
   {
      return _pAgent ;
   }

   pmdRemoteSessionSite* _pmdRemoteSessionMgr::getSite( _pmdEDUCB *cb )
   {
      return getSite( cb->getTID() ) ;
   }

   pmdRemoteSessionSite* _pmdRemoteSessionMgr::getSite( UINT32 tid )
   {
      pmdRemoteSessionSite *pSite = NULL ;
      ossScopedLock lock( &_edusLatch, SHARED ) ;
      MAP_TID_2_EDU_IT it = _mapTID2EDU.find( tid ) ;
      if ( it != _mapTID2EDU.end() )
      {
         pSite = &(it->second) ;
      }
      return pSite ;
   }

   INT32 _pmdRemoteSessionMgr::pushMessage( const NET_HANDLE &handle,
                                            const MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *pEDUCB = NULL ;
      MAP_TID_2_EDU_IT it ;

      ossScopedLock lock( &_edusLatch, SHARED ) ;

      it = _mapTID2EDU.find( pMsg->TID ) ;
      if ( it != _mapTID2EDU.end() )
      {
         CHAR *pNewBuff = NULL ;
         pEDUCB = it->second.eduCB() ;

         pNewBuff = ( CHAR* )SDB_OSS_MALLOC( pMsg->messageLength + 1 ) ;
         if ( pNewBuff )
         {
            ossMemcpy( pNewBuff, pMsg, pMsg->messageLength ) ;
            pNewBuff[ pMsg->messageLength ] = 0 ;
            pEDUCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                            PMD_EDU_MEM_ALLOC,
                                            pNewBuff, (UINT64)handle ) ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to alloc memory[size: %d] for msg["
                    "opCode: (%d)%u, TID: %d, ReqID: %llu, NodeID: %s]",
                    pMsg->messageLength, IS_REPLY_TYPE(pMsg->opCode),
                    GET_REQUEST_TYPE(pMsg->opCode), pMsg->TID,
                    pMsg->requestID, routeID2String(pMsg->routeID).c_str() ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Can't find remote session[TID=%d] for msg["
                 "opCode: (%d)%u, ReqID: %llu, NodeID: %s, Len: %u]",
                 pMsg->TID, IS_REPLY_TYPE(pMsg->opCode),
                 GET_REQUEST_TYPE(pMsg->opCode), pMsg->requestID,
                 routeID2String(pMsg->routeID).c_str(), pMsg->messageLength ) ;
         rc = SDB_INVALIDARG ;
      }

      return rc ;
   }

   void _pmdRemoteSessionMgr::handleClose( const NET_HANDLE &handle,
                                           const _MsgRouteID &id )
   {
      PD_LOG( PDEVENT, "Connection[Handle:%u, RouteID:%s] disconnect",
              handle, routeID2String( id ).c_str() ) ;

      ossScopedLock lock( &_edusLatch, SHARED ) ;
      MAP_TID_2_EDU_IT it = _mapTID2EDU.begin() ;
      while ( it != _mapTID2EDU.end() )
      {
         it->second.handleClose( handle, id ) ;
         ++it ;
      }
   }

   void _pmdRemoteSessionMgr::handleConnect( const NET_HANDLE &handle,
                                             _MsgRouteID id,
                                             BOOLEAN isPositive )
   {
   }

   pmdRemoteSession* _pmdRemoteSessionMgr::addSession( _pmdEDUCB * cb,
                                                        INT64 timeout,
                                                        IRemoteSessionHandler *pHandle )
   {
      pmdRemoteSessionSite *pSite = NULL ;
      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
#ifdef _DEBUG
      SDB_ASSERT( pSite == getSite( cb ), "Site is not the same" ) ;
#endif //_DEBUG
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Session[%s] is not registered for remote session",
                 cb->toString().c_str() ) ;
         return NULL ;
      }
      return pSite->addSession( timeout, pHandle ) ;
   }

   pmdRemoteSession* _pmdRemoteSessionMgr::getSession( UINT64 sessionID )
   {
      UINT32 tid = (UINT32)( sessionID >> 32 ) ;
      pmdRemoteSessionSite *pSite = getSite( tid ) ;
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Session[%u] is not registered for remote session",
                 tid ) ;
         return NULL ;
      }
      return pSite->getSession( sessionID ) ;
   }

   void _pmdRemoteSessionMgr::removeSession( UINT64 sessionID )
   {
      UINT32 tid = (UINT32)( sessionID >> 32 ) ;
      pmdRemoteSessionSite *pSite = getSite( tid ) ;
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Session[%u] is not registered for remote session",
                 tid ) ;
      }
      else
      {
         pSite->removeSession( sessionID ) ;
      }
   }

   void _pmdRemoteSessionMgr::removeSession( pmdRemoteSession *pSession )
   {
      return removeSession( pSession->sessionID() ) ;
   }

   UINT32 _pmdRemoteSessionMgr::sessionCount()
   {
      UINT32 num = 0 ;
      ossScopedLock lock( &_edusLatch, SHARED ) ;
      MAP_TID_2_EDU_IT it = _mapTID2EDU.begin() ;
      while ( it != _mapTID2EDU.end() )
      {
         num += it->second.sessionCount() ;
         ++it ;
      }
      return num ;
   }

}

