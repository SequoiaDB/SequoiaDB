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

   Source File Name = coordContext.cpp

   Descriptive Name = Coord Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context helper
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordContext.hpp"
#include "pmd.hpp"
#include "coordCB.hpp"
#include "clsResourceContainer.hpp"
#include "coordUtil.hpp"
#include "coordRemoteSession.hpp"
#include "msgMessageFormat.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_SUBCTX_KILLCONTEXT_TIMEOUT ( 30000 )

   /*
      _rtnContextCoord implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextCoord, RTN_CONTEXT_COORD, "COORD")

   _rtnContextCoord::_rtnContextCoord( INT64 contextID, UINT64 eduID,
                                       BOOLEAN preRead )
   :_rtnContextMain( contextID, eduID )
   {
      _preRead          = preRead ;
      _needReOrder      = FALSE ;

      _pSession         = NULL ;

      _isModify         = FALSE ;
   }

   _rtnContextCoord::~_rtnContextCoord ()
   {
      if ( NULL != _pSession )
      {
         pmdEDUCB *cb = pmdGetThreadEDUCB() ;
         if ( NULL != cb->getRemoteSite() )
         {
            pmdRemoteSessionSite *pSite =
                  (pmdRemoteSessionSite *)( cb->getRemoteSite() ) ;
            _killSubContexts( cb ) ;
            pSite->removeSession( _pSession->sessionID() ) ;
         }
         else
         {
            _destroySubContexts() ;
         }
      }
   }

   INT64 _rtnContextCoord::getWaitTime() const
   {
      if ( _pSession )
      {
         return _pSession->getTotalWaitTime() ;
      }
      return 0 ;
   }

   void _rtnContextCoord::optimizeReturnOptions ( MsgOpQuery * pQueryMsg,
                                                  UINT32 targetGroupNum )
   {
      SDB_ASSERT( pQueryMsg, "query message is invalid" ) ;

      if ( targetGroupNum <= 1 )
      {
         INT64 origNumToReturn = _numToReturn ;
         INT64 origNumToSkip = _numToSkip ;

         if ( origNumToSkip > 0 )
         {
            pQueryMsg->numToSkip = origNumToSkip ;
            _numToSkip = 0 ;
            if ( origNumToReturn > 0 &&
                 pQueryMsg->numToReturn == origNumToReturn + origNumToSkip )
            {
               pQueryMsg->numToReturn -= origNumToSkip ;
            }
         }
      }
      else
      {
         if ( pQueryMsg->numToSkip > 0 )
         {
            if ( pQueryMsg->numToReturn > 0 )
            {
               _numToReturn = pQueryMsg->numToReturn ;
               pQueryMsg->numToReturn += pQueryMsg->numToSkip ;
            }
            _numToSkip = pQueryMsg->numToSkip ;
            pQueryMsg->numToSkip = 0 ;
         }
      }
   }

   void _rtnContextCoord::getErrorInfo( INT32 rc,
                                        pmdEDUCB *cb,
                                        rtnContextBuf &buffObj )
   {
      if ( rc && _nokRC.size() > 0 )
      {
         coordResource *pResource = sdbGetResourceContainer()->getResource() ;
         buffObj = coordBuildErrorObj( pResource, rc, cb, &_nokRC ) ;
      }
   }

   UINT32 _rtnContextCoord::getCachedRecordNum()
   {
      UINT32 recordNum = 0 ;
      SUB_ORDERED_CTX_SET_IT it = _orderedContexts.begin() ;
      while( it != _orderedContexts.end() )
      {
         rtnSubContext *pSub = *it ;
         recordNum += pSub->recordNum() ;
         ++it ;
      }
      if ( _numToSkip > recordNum )
      {
         recordNum = 0 ;
      }
      else if ( _numToSkip > 0 )
      {
         recordNum -= _numToSkip ;
      }

      return recordNum + _rtnContextBase::getCachedRecordNum() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__KILLSUBCTXS, "_rtnContextCoord::_killSubContexts" )
   void _rtnContextCoord::_killSubContexts( pmdEDUCB * cb )
   {
      UINT32 tid = 0 ;
      PD_TRACE_ENTRY ( SDB_CTXCOOR__KILLSUBCTXS ) ;

      coordSubContext *pSubContext  = NULL ;
      pmdSubSession *pSub = NULL ;

      if ( cb )
      {
         tid = cb->getTID() ;
         // get all pre-read reply
         _getPrepareNodesData( cb, TRUE ) ;
      }

      // push all ordered context to prepare map
      SUB_ORDERED_CTX_SET_IT itSub = _orderedContexts.begin() ;
      while ( _orderedContexts.end() != itSub )
      {
         rtnSubContext* rtnSubCtx = *itSub ;
         pSubContext = dynamic_cast<coordSubContext*>( rtnSubCtx ) ;

         try
         {
            _prepareContextMap.insert( EMPTY_CONTEXT_MAP::value_type(
                                       pSubContext->getRouteID().value,
                                       pSubContext ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to move context to prepare contexts, "
                    "occur exception %s", e.what() ) ;
            _releaseSubContext( rtnSubCtx ) ;
         }

         ++itSub ;
      }
      _orderedContexts.clear() ;

      EMPTY_CONTEXT_MAP::iterator it = _emptyContextMap.begin() ;
      while ( it != _emptyContextMap.end() )
      {
         try
         {
            _prepareContextMap.insert( EMPTY_CONTEXT_MAP::value_type(
                                       it->first,
                                       it->second ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to move context to prepare contexts, "
                    "occur exception %s", e.what() ) ;
            _releaseSubContext( it->second ) ;
         }

         ++it ;
      }
      _emptyContextMap.clear() ;

      // kill sub context
      if ( cb && ( !cb->isInterrupted() || cb->isOnlySelfWhenInterrupt() ) )
      {
         MsgOpKillContexts killMsg ;
         MsgRouteID routeID ;
         killMsg.header.messageLength = sizeof ( MsgOpKillContexts ) ;
         killMsg.header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
         killMsg.header.TID = tid ;
         killMsg.header.routeID.value = 0;
         killMsg.ZERO = 0;
         killMsg.numContexts = 1 ;

         it = _prepareContextMap.begin() ;
         while ( it != _prepareContextMap.end() )
         {
            SINT64 contextID = -1 ;
            pSubContext = it->second ;
            contextID = pSubContext->contextID() ;
            if ( -1 == contextID )
            {
               // Ignore invalid context ID
               ++it ;
               continue ;
            }
            routeID = pSubContext->getRouteID() ;
            killMsg.contextIDs[0] = contextID ;

            PD_LOG( PDDEBUG, "Send kill context[ContextID:%lld] to node[%s]",
                    contextID, routeID2String( routeID ).c_str() ) ;

            pSub = _pSession->addSubSession( routeID.value ) ;
            // If other msg is sent before, and no reply has been received.
            // At this time, the kill context msg will not be sent.
            // So we need to clearSend() before sending kill context msg.
            pSub->resetForResend() ;
            pSub->setReqMsg( (MsgHeader*)&killMsg, PMD_EDU_MEM_NONE ) ;
            _pSession->sendMsg( pSub ) ;

            ++it ;
         }

         if ( _prepareContextMap.size() > 0 )
         {
            /// recv reply
            _pSession->waitReply1( TRUE ) ;
         }
      }

      // release all context
      it = _prepareContextMap.begin() ;
      while ( it != _prepareContextMap.end() )
      {
         pSubContext = it->second ;
         // contexts on data nodes have already been killed
         pSubContext->setContextID( -1 ) ;
         _releaseSubContext( pSubContext ) ;
         ++it ;
      }
      _prepareContextMap.clear() ;

      PD_TRACE_EXIT( SDB_CTXCOOR__KILLSUBCTXS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__DESSUBCTXS, "_rtnContextCoord::_destroySubContexts" )
   void _rtnContextCoord::_destroySubContexts()
   {
      PD_TRACE_ENTRY ( SDB_CTXCOOR__DESSUBCTXS ) ;

      for ( SUB_ORDERED_CTX_SET_IT itSub = _orderedContexts.begin() ;
            _orderedContexts.end() != itSub ;
            ++ itSub )
      {
         rtnSubContext *pSubContext = *itSub ;
         // contexts on data nodes have already been killed
         pSubContext->setContextID( -1 ) ;
         _releaseSubContext( pSubContext ) ;
      }
      _orderedContexts.clear() ;

      for ( EMPTY_CONTEXT_MAP::iterator it = _emptyContextMap.begin() ;
            it != _emptyContextMap.end() ;
            ++ it )
      {
         rtnSubContext *pSubContext = it->second ;
         // contexts on data nodes have already been killed
         pSubContext->setContextID( -1 ) ;
         _releaseSubContext( it->second ) ;
      }
      _emptyContextMap.clear() ;

      // release all context
      for ( EMPTY_CONTEXT_MAP::iterator it = _prepareContextMap.begin() ;
            it != _prepareContextMap.end() ;
            ++ it )
      {
         rtnSubContext *pSubContext = it->second ;
         // contexts on data nodes have already been killed
         pSubContext->setContextID( -1 ) ;
         _releaseSubContext( pSubContext ) ;
      }
      _prepareContextMap.clear() ;

      PD_TRACE_EXIT( SDB_CTXCOOR__DESSUBCTXS ) ;
   }

   const CHAR* _rtnContextCoord::name() const
   {
      return "COORD" ;
   }

   RTN_CONTEXT_TYPE _rtnContextCoord::getType () const
   {
      return RTN_CONTEXT_COORD ;
   }

   INT32 _rtnContextCoord::open ( const rtnQueryOptions & options,
                                  BOOLEAN preRead )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      pmdRemoteSessionSite *pSite = NULL ;
      coordSessionPropSite *pPropSite = NULL ;
      INT64 timeout = -1 ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Session[%s] is invalid: remote site is NULL",
                 cb->getName() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;
      if ( pPropSite )
      {
         timeout = pPropSite->getOperationTimeout() ;
      }
      _pSession = pSite->addSession( timeout, &_handler ) ;
      if ( !_pSession )
      {
         PD_LOG( PDERROR, "Create remote session failed in session[%s]",
                 cb->getName() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _options = options ;
      rc = _options.getOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get query options owned, "
                   "rc: %d", rc ) ;

      _numToReturn = _options.getLimit() ;
      _numToSkip = _options.getSkip() ;
      _preRead = preRead ;

      _keyGen = SDB_OSS_NEW _ixmIndexKeyGen( _options.getOrderBy() ) ;
      PD_CHECK( _keyGen != NULL, SDB_OOM, error, PDERROR,
                "Failed to allocate index key generator" ) ;

      if ( !_options.isSelectorEmpty() )
      {
         rc = _selector.loadPattern ( _options.getSelector() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to load selector pattern rc: %d",
                    rc ) ;
            goto error ;
         }
      }

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextCoord::setModify( BOOLEAN modify )
   {
      _isModify = modify ;
   }

   INT32 _rtnContextCoord::reopen ()
   {
      if ( _isOpened )
      {
         return SDB_DMS_CONTEXT_IS_OPEN ;
      }
      if ( !eof() || !isEmpty() )
      {
         return SDB_SYS ;
      }

      _nokRC.clear() ;
      _resetTotalRecords( numRecords() ) ;
      _isOpened = TRUE ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__SEND2EMPTYNODES, "_rtnContextCoord::_send2EmptyNodes" )
   INT32 _rtnContextCoord::_send2EmptyNodes( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CTXCOOR__SEND2EMPTYNODES ) ;

      MsgOpGetMore msgReq ;
      MsgRouteID routeID ;
      EMPTY_CONTEXT_MAP::iterator emptyIter ;
      pmdSubSession *pSub = NULL ;
      pmdRemoteSessionSite *pSite = NULL ;
      coordSessionPropSite *pPropSite = NULL ;

      if ( _emptyContextMap.size() == 0 )
      {
         goto done ;
      }

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;

      msgFillGetMoreMsg( msgReq, cb->getTID(), -1, -1, 0 ) ;

      emptyIter = _emptyContextMap.begin() ;
      while( emptyIter != _emptyContextMap.end() )
      {
         if ( -1 == emptyIter->second->contextID() )
         {
            _releaseSubContext( emptyIter->second ) ;
            _emptyContextMap.erase( emptyIter++ ) ;
            continue ;
         }

         routeID.value = emptyIter->first ;
         msgReq.header.routeID.value = MSG_INVALID_ROUTEID ;
         msgReq.contextID = emptyIter->second->contextID() ;

         pSub = _pSession->addSubSession( routeID.value ) ;
         pSub->setReqMsg( (MsgHeader*)&msgReq, PMD_EDU_MEM_NONE ) ;

         /// In transaction and context is write, should check and update
         /// trans node's status
         if ( cb->isTransaction() && isWrite() && pPropSite )
         {
            pPropSite->checkAndUpdateNode( routeID, TRUE ) ;
         }
         rc = _pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send get more message[ContextID:%lld] to "
                    "node[%s] failed, rc: %d", msgReq.contextID,
                    routeID2String( routeID ).c_str(), rc ) ;
            goto error ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Send get more message[ContextID:%lld] to "
                    "node[%s] succeed", msgReq.contextID,
                    routeID2String( routeID ).c_str() ) ;
         }

         _prepareContextMap.insert( EMPTY_CONTEXT_MAP::value_type(
                                    emptyIter->first, emptyIter->second ) ) ;
         _emptyContextMap.erase( emptyIter++ ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CTXCOOR__SEND2EMPTYNODES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__GETPREPARENODESDATA, "_rtnContextCoord::_getPrepareNodesData" )
   INT32 _rtnContextCoord::_getPrepareNodesData( pmdEDUCB * cb,
                                                 BOOLEAN waitAll )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CTXCOOR__GETPREPARENODESDATA ) ;

      pmdEDUEvent event ;
      MsgOpReply *pReply = NULL ;

      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itr ;

      rc = _pSession->waitReply1( waitAll ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply failed, rc: %d", rc ) ;
         goto error ;
      }

      itr = _pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while ( itr.more() )
      {
         pSub = itr.next() ;
         event = pSub->getOwnedRspMsg() ;
         pReply = ( MsgOpReply* )event._Data ;
         pSub->resetForResend() ;

         if ( pReply->header.messageLength < (INT32)sizeof( MsgOpReply ) )
         {
            _delPrepareContext( pReply->header.routeID ) ;
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Get data failed, received invalid message "
                     "from node(groupID=%u, nodeID=%u, serviceID=%u, "
                     "messageLength=%d)",
                     pReply->header.routeID.columns.groupID,
                     pReply->header.routeID.columns.nodeID,
                     pReply->header.routeID.columns.serviceID,
                     pReply->header.messageLength ) ;
            break ;
         }
         else if ( pReply->flags )
         {
            _delPrepareContext( pReply->header.routeID, TRUE ) ;

            if ( SDB_DMS_EOC != pReply->flags )
            {
               PD_LOG ( PDERROR, "Get data failed, failed to get data "
                        "from node (groupID=%u, nodeID=%u, serviceID=%u, "
                        "flag=%d)", pReply->header.routeID.columns.groupID,
                        pReply->header.routeID.columns.nodeID,
                        pReply->header.routeID.columns.serviceID,
                        pReply->flags ) ;
               rc = pReply->flags ;
               _nokRC[ pReply->header.routeID.value ] =
                  coordErrorInfo( pReply ) ;
               break ;
            }
            else
            {
               // release data
               pmdEduEventRelease( event, NULL ) ;
               pReply = NULL ;
            }
         }
         else
         {
            BOOLEAN isTakeOver = FALSE ;
            rc = _appendSubData( event, isTakeOver ) ;
            if ( isTakeOver )
            {
               event.reset() ;
            }
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to append the data, rc: %d", rc ) ;
               break ;
            }
            pReply = NULL ;
         }
      } // end while

      if ( rc )
      {
         goto error ;
      }

   done:
      pmdEduEventRelease( event, NULL ) ;
      pReply = NULL ;
      PD_TRACE_EXITRC( SDB_CTXCOOR__GETPREPARENODESDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextCoord::_toString( stringstream &ss )
   {
      if ( !_options.isOrderByEmpty() )
      {
         ss << ",Orderby:" << _options.getOrderBy().toString().c_str() ;
      }
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
   }

   INT32 _rtnContextCoord::_reOrderSubContext()
   {
      INT32 rc = SDB_OK ;

      if ( _needReOrder && 1 == _orderedContexts.size() &&
           _requireExplicitSorting() )
      {
         rtnSubContext* subCtx = *( _orderedContexts.begin() ) ;

         _orderedContexts.clear() ;

         rc = _saveNonEmptyOrderedSubCtx( subCtx ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to get orderKey failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      _needReOrder = FALSE ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextCoord::_appendSubData( const pmdEDUEvent &event,
                                           BOOLEAN &isTakeOver )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *pReply = (MsgOpReply *)event._Data ;
      EMPTY_CONTEXT_MAP::iterator iter ;
      coordSubContext *pSubContext = NULL ;
      BOOLEAN skipData = FALSE ;

      isTakeOver = FALSE ;

      if ( pReply->header.opCode != MSG_BS_GETMORE_RES ||
           (UINT32)pReply->header.messageLength < sizeof( MsgOpReply ) )
      {
         rc = SDB_UNKNOWN_MESSAGE ;
         PD_LOG ( PDERROR, "Failed to append the data, invalid data"
                  "(opCode=%d, messageLength=%d)", pReply->header.opCode,
                  pReply->header.messageLength ) ;
         goto error ;
      }

      iter = _prepareContextMap.find( pReply->header.routeID.value ) ;
      if ( _prepareContextMap.end() == iter )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Failed to append the data, no match context"
                  "(groupID=%u, nodeID=%u, serviceID=%u)",
                  pReply->header.routeID.columns.groupID,
                  pReply->header.routeID.columns.nodeID,
                  pReply->header.routeID.columns.serviceID ) ;
         goto error ;
      }

      pSubContext = iter->second ;
      SDB_ASSERT( pSubContext != NULL, "subContext can't be NULL" ) ;

      if ( pSubContext->contextID() != pReply->contextID )
      {
         if ( -1 == pReply->contextID )
         {
            PD_LOG( PDDEBUG, "Context %lld closed by node "
                    "[ groupID=%u, nodeID=%u, serviceID=%u ]",
                    pSubContext->contextID(),
                    pReply->header.routeID.columns.groupID,
                    pReply->header.routeID.columns.nodeID,
                    pReply->header.routeID.columns.serviceID ) ;
            pSubContext->setContextID( pReply->contextID ) ;
         }
         else
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Failed to append the data, no match context"
                     "(expectContextID=%lld, contextID=%lld)",
                     pSubContext->contextID(), pReply->contextID ) ;
            goto error ;
         }
      }

      // after appendData success, the data-pointer is manage by subContext
      pSubContext->appendData( event ) ;
      isTakeOver = TRUE ;

      rc = _processSubContext( pSubContext, skipData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process sub-context"
                   "[ groupID=%u, nodeID=%u, serviceID=%u, contextID=%lld ], "
                   "rc: %d",
                   pReply->header.routeID.columns.groupID,
                   pReply->header.routeID.columns.nodeID,
                   pReply->header.routeID.columns.serviceID,
                   pSubContext->contextID(), rc ) ;

      if ( skipData )
      {
         _prepareContextMap.erase( iter ) ;
         rc = _saveEmptyOrderedSubCtx( pSubContext ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         goto done ;
      }

      if ( !_requireExplicitSorting() )
      {
         _prepareContextMap.erase( iter ) ;

         try
         {
            _orderedContexts.insert( pSubContext ) ;
         }
         catch( std::exception& e )
         {
            rc = SDB_SYS ;
            _releaseSubContext( pSubContext ) ;
            PD_LOG( PDERROR, "occur unexpected error:%s", e.what() );
            goto error ;
         }
      }
      else
      {
         if ( _needReOrder )
         {
            rc = _reOrderSubContext() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Re-order sub context last record "
                       "failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         _prepareContextMap.erase( iter ) ;

         rc = _saveNonEmptyOrderedSubCtx( pSubContext ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to save sub ctx by order, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextCoord::_createSubContext( MsgRouteID routeID,
                                              SINT64 contextID )
   {
      INT32 rc = SDB_OK ;
      EMPTY_CONTEXT_MAP::iterator iter ;
      coordSubContext *pSubContext = NULL ;

      if ( !_isOpened || NULL == _pSession )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }

      iter = _emptyContextMap.find( routeID.value ) ;
      if ( iter != _emptyContextMap.end() )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Repeat to add sub-context (groupID=%u, nodeID=%u, "
                 "serviceID=%u, oldContextID=%lld, newContextID=%lld)",
                 routeID.columns.groupID, routeID.columns.nodeID,
                 routeID.columns.serviceID, iter->second->contextID(),
                 contextID ) ;
         goto error ;
      }

      pSubContext = SDB_OSS_NEW coordSubContext( _options.getOrderBy(),
                                                 _keyGen,
                                                 contextID,
                                                 routeID ) ;
      if ( NULL == pSubContext )
      {
         rc = SDB_OOM;
         PD_LOG ( PDERROR, "Failed to alloc memory" ) ;
         goto error ;
      }

      _emptyContextMap.insert( EMPTY_CONTEXT_MAP::value_type( routeID.value,
                               pSubContext ) ) ;

      rc = _checkSubContext( pSubContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sub context, rc: %d", rc ) ;

      PD_LOG( PDDEBUG,
              "add sub context (groupID=%u, nodeID=%u, contextID=%lld)",
              routeID.columns.groupID,
              routeID.columns.nodeID,
              contextID) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextCoord::addSubContext( const pmdEDUEvent &event,
                                          BOOLEAN & takeOver )
   {
      INT32 rc = SDB_OK ;
      takeOver = FALSE ;
      BOOLEAN isEmpty = FALSE ;
      MsgOpReply *pReply = ( MsgOpReply* )event._Data ;

      SDB_ASSERT ( NULL != pReply, "pReply can't be NULL" ) ;

      if ( _orderedContexts.empty() && _emptyContextMap.empty() &&
           _prepareContextMap.empty() )
      {
         isEmpty = TRUE ;
      }

      rc = _createSubContext( pReply->header.routeID, pReply->contextID ) ;
      if ( rc )
      {
         goto error ;
      }

      // query with return data
      if ( pReply->numReturned > 0 )
      {
         EMPTY_CONTEXT_MAP::iterator it ;
         it = _emptyContextMap.find( pReply->header.routeID.value ) ;
         SDB_ASSERT( it != _emptyContextMap.end(), "System error" ) ;

         if ( !_needReOrder && isEmpty )
         {
            _needReOrder = TRUE ;
         }

         _prepareContextMap.insert( EMPTY_CONTEXT_MAP::value_type(
                                    it->first, it->second ) ) ;
         _emptyContextMap.erase( it ) ;

         pReply->header.opCode = MSG_BS_GETMORE_RES ;
         rc = _appendSubData( event, takeOver ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Append sub data failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextCoord::addSubDone( pmdEDUCB * cb )
   {
      if ( _preRead )
      {
         _send2EmptyNodes( cb ) ;
      }
   }

   void _rtnContextCoord::_delPrepareContext( const MsgRouteID & routeID,
                                              BOOLEAN setInvalidContext )
   {
      EMPTY_CONTEXT_MAP::iterator iter =
         _prepareContextMap.find( routeID.value ) ;

      if ( iter != _prepareContextMap.end() )
      {
         coordSubContext *pSubContext = iter->second ;

         if ( pSubContext != NULL )
         {
            if ( setInvalidContext )
            {
               pSubContext->setContextID( -1 ) ;
            }
            _releaseSubContext( pSubContext ) ;
         }
         _prepareContextMap.erase ( iter ) ;
      }
   }

   INT32 _rtnContextCoord::_prepareSubCtxData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN waitAll = FALSE ;

      if ( _emptyContextMap.size() == 0 &&
           _prepareContextMap.size() == 0 )
      {
         goto done ;
      }

      rc = _send2EmptyNodes( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Send request to empty nodes failed, rc: %d",
                   rc ) ;

      if ( _isModify || _requireExplicitSorting() )
      {
         waitAll = TRUE ;
      }

      rc = _getPrepareNodesData( cb, waitAll ) ;
      PD_RC_CHECK( rc, PDERROR, "Get data from prepare nodes failed, rc: %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextCoord::_prepareAllSubCtxDataByOrder( _pmdEDUCB *cb )
   {
      return _prepareSubCtxData( cb ) ;
   }

   INT32 _rtnContextCoord::_saveEmptyOrderedSubCtx( rtnSubContext* subCtx )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != subCtx, "subCtx should be not null" ) ;

      if ( -1 == subCtx->contextID() )
      {
         _releaseSubContext( subCtx ) ;
         goto done ;
      }

      try
      {
         coordSubContext* coordSubCtx = dynamic_cast<coordSubContext*>( subCtx ) ;
         _emptyContextMap.insert(
            EMPTY_CONTEXT_MAP::value_type(
               coordSubCtx->getRouteID().value, coordSubCtx ) ) ;
      }
      catch( std::exception& e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "occur unexpected error:%s", e.what() );
         goto error ;
      }

   done:
      return rc ;
   error:
      _releaseSubContext( subCtx ) ;
      goto done ;
   }

   INT32 _rtnContextCoord::_getNonEmptyNormalSubCtx( _pmdEDUCB *cb,
                                                     rtnSubContext*& subCtx )
   {
      INT32 rc = SDB_OK ;
      SUB_ORDERED_CTX_SET_IT iter ;

      subCtx = NULL ;

      while ( _orderedContexts.size() == 0 )
      {
         if ( _emptyContextMap.size() + _prepareContextMap.size() == 0 )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }

         rc = _prepareSubCtxData( cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      SDB_ASSERT( _orderedContexts.size() != 0,
                  "_orderedContextMap should not be empty" ) ;

      iter = _orderedContexts.begin() ;
      subCtx = *iter ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextCoord::_saveEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != subCtx, "subCtx can't be NULL" ) ;
      SDB_ASSERT( subCtx->recordNum() == 0, "sub ctx is not empty" ) ;

      // move empty sub ctx from _orderedContextMap to _emptyContextMap

      // generally, subctx is first element of _orderedContextMap
      // so don't worry about performance
      SUB_ORDERED_CTX_SET_IT iter = _orderedContexts.begin() ;
      if ( *iter == subCtx )
      {
         _orderedContexts.erase( iter ) ;
      }
      else
      {
         // if the context is not the first element, need iterate the
         // elements with the same order key in the set
         // NOTE: should not use erase(value) in multiset, which will
         //       remove all contexts with the same key
         SUB_ORDERED_CTX_SET_IT_PAIR itPair =
                                       _orderedContexts.equal_range( subCtx ) ;
         for ( SUB_ORDERED_CTX_SET_IT iter = itPair.first ;
               iter != itPair.second ;
               ++ iter )
         {
            if ( *iter == subCtx )
            {
               // found target
               _orderedContexts.erase( iter ) ;
               break ;
            }
         }
      }

      rc = _saveEmptyOrderedSubCtx( subCtx ) ;

      return rc ;
   }

   INT32 _rtnContextCoord::_saveNonEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      SDB_ASSERT( NULL != subCtx, "subCtx can't be NULL" ) ;
      SDB_ASSERT( subCtx->recordNum() > 0, "sub ctx is empty" ) ;

      // sub ctx is in _orderedContextMap,
      // no need to do anything
      return SDB_OK ;
   }

   INT32 _rtnContextCoord::_doAfterPrepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( _numToReturn != 0 && !_hitEnd && _preRead )
      {
         rc = _send2EmptyNodes( cb ) ;
      }

      if ( !_hitEnd && _emptyContextMap.empty() &&
           _orderedContexts.empty() && _prepareContextMap.empty() )
      {
         _hitEnd = TRUE ;
      }

      return rc ;
   }

   INT32 _rtnContextCoord::_processCacheData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( _prepareContextMap.size() > 0 )
      {
         rc = _getPrepareNodesData( cb, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get prepare node's data failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__PREPARESUBCTXSADVANCE, "_rtnContextCoord::_prepareSubCtxsAdvance" )
   INT32 _rtnContextCoord::_prepareSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CTXCOOR__PREPARESUBCTXSADVANCE ) ;

      EMPTY_CONTEXT_MAP::iterator itEmpty ;

      try
      {
         // get subcontext from empty
         itEmpty = _emptyContextMap.begin() ;
         while( itEmpty != _emptyContextMap.end() )
         {
            lstCtx.push_back( itEmpty->second ) ;
            ++itEmpty ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e );
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CTXCOOR__PREPARESUBCTXSADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__DOSUBCTXSADVANCE, "_rtnContextCoord::_doSubCtxsAdvance" )
   INT32 _rtnContextCoord::_doSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx,
                                              const BSONObj &arg,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CTXCOOR__DOSUBCTXSADVANCE ) ;

      CHAR *pBuffer = NULL ;
      INT32 buffSize = 0 ;
      MsgOpAdvance *pAdvance = NULL ;
      coordSubContext *pSubContext = NULL ;
      MsgRouteID routeID ;
      LST_SUB_CTX_PTR::iterator itrLst ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itr ;
      MsgOpReply *pReply = NULL ;

      if ( lstCtx.size() == 0 )
      {
         goto done ;
      }

      /// 1. build message
      rc = msgBuildAdvanceMsg( &pBuffer, &buffSize, -1, 0, &arg,
                               NULL, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build advance message failed, rc: %d", rc ) ;
         goto error ;
      }

      /// 2. send message
      pAdvance = ( MsgOpAdvance* )pBuffer ;
      itrLst = lstCtx.begin() ;
      while( itrLst != lstCtx.end() )
      {
         if ( -1 == (*itrLst)->contextID() )
         {
            ++itrLst ;
            continue ;
         }
         pSubContext = ( coordSubContext* )( *itrLst ) ;

         routeID.value = pSubContext->getRouteID().value ;
         pAdvance->header.routeID.value = MSG_INVALID_ROUTEID ;
         pAdvance->contextID = pSubContext->contextID() ;

         pSub = _pSession->addSubSession( routeID.value ) ;
         pSub->setReqMsg( (MsgHeader*)pAdvance, PMD_EDU_MEM_NONE ) ;

         rc = _pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send advance message[ContextID:%lld] to "
                    "node[%s] failed, rc: %d", pAdvance->contextID,
                    routeID2String( routeID ).c_str(), rc ) ;
            goto error ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Send advance message[ContextID:%lld] to "
                    "node[%s] succeed", pAdvance->contextID,
                    routeID2String( routeID ).c_str() ) ;
         }
         ++itrLst ;
      }

      /// 3. receive and process message
      rc = _pSession->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply failed, rc: %d", rc ) ;
         goto error ;
      }

      itr = _pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while ( itr.more() )
      {
         pSub = itr.next() ;
         pReply = ( MsgOpReply* )pSub->getRspMsg() ;

         if ( pReply->flags )
         {
            PD_LOG ( PDERROR, "Do sub context[%lld] advance failed on "
                     "node(groupID=%u, nodeID=%u, serviceID=%u), rc: %d",
                     pReply->contextID,
                     pReply->header.routeID.columns.groupID,
                     pReply->header.routeID.columns.nodeID,
                     pReply->header.routeID.columns.serviceID,
                     pReply->flags ) ;
            rc = pReply->flags ;
            goto error ;
         }
      } // end while

   done:
      if ( pBuffer )
      {
         msgReleaseBuffer( pBuffer, cb ) ;
      }
      _pSession->clearSubSession() ;
      PD_TRACE_EXITRC( SDB_CTXCOOR__DOSUBCTXSADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOOR__PRERELEASESUBCTX, "_rtnContextCoord::_preReleaseSubContext" )
   void _rtnContextCoord::_preReleaseSubContext( rtnSubContext *subCtx )
   {
      PD_TRACE_ENTRY( SDB_CTXCOOR__PRERELEASESUBCTX ) ;

      if ( NULL != subCtx )
      {
         coordSubContext *ctx = (coordSubContext *)subCtx ;
         ctx->clearData() ;

         pmdEDUCB *cb = pmdGetThreadEDUCB() ;
         if ( -1 != ctx->contextID() &&
              NULL != cb &&
              NULL != cb->getRemoteSite() )
         {
            pmdRemoteSession *pSession = NULL ;
            pmdSubSession *pSub = NULL ;
            MsgOpKillContexts msgKillContext ;

            pmdRemoteSessionSite *pSite =
                  (pmdRemoteSessionSite *)( cb->getRemoteSite() ) ;

            pSession = pSite->addSession( COORD_SUBCTX_KILLCONTEXT_TIMEOUT ) ;
            pSub = pSession->addSubSession( ctx->getRouteID().value ) ;

            /// send kill context
            msgKillContext.contextIDs[ 0 ] = ctx->contextID() ;
            msgKillContext.numContexts = 1 ;
            msgKillContext.ZERO = 0 ;
            msgKillContext.header.messageLength = sizeof( MsgOpKillContexts ) ;
            msgKillContext.header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
            msgKillContext.header.requestID = 0 ;
            msgKillContext.header.routeID.value = 0 ;
            msgKillContext.header.TID = 0 ;

            /// Ignore sendMsg failed
            pSession->sendMsg( (MsgHeader*)&msgKillContext, PMD_EDU_MEM_NONE ) ;

            // DON'T wait for reply, otherwise Timeout of sessionAttr may fail. Reset
            // sub session when ignoring the reply, otherwise interrupt message will
            // be sent to data node, the operation of data node may be interrupted.
            pSub->resetForResend() ;

            if ( pSession )
            {
               pSite->removeSession( pSession ) ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_CTXCOOR__PRERELEASESUBCTX ) ;
   }

   /*
      _coordSubContext implement
   */
   _coordSubContext::_coordSubContext ( const BSONObj& orderBy,
                                        _ixmIndexKeyGen* keyGen,
                                        INT64 contextID,
                                        MsgRouteID routeID )
   : _rtnSubContext( orderBy, keyGen, contextID ),
     _routeID( routeID )
   {
      _pData = NULL ;
      _curOffset = 0 ;
      _recordNum = 0 ;
   }

   _coordSubContext::~_coordSubContext ()
   {
      pmdEduEventRelease( _event, NULL ) ;
      _pData = NULL ;
   }

   INT32 _coordSubContext::remainLength()
   {
      if ( _pData->header.messageLength > _curOffset )
      {
         return _pData->header.messageLength - _curOffset ;
      }
      return 0 ;
   }

   INT32 _coordSubContext::truncate ( INT32 num )
   {
      INT32 rc = SDB_OK ;
      INT32 offset = _curOffset ;
      INT32 recordNum = 0 ;

      if ( num >= _recordNum )
      {
         goto done ;
      }

      while( ossAlign4( (UINT32)offset ) < (UINT32)_pData->header.messageLength &&
             recordNum < num )
      {
         offset = ossAlign4( (UINT32)offset ) ;
         try
         {
            BSONObj objTemp( (CHAR *)_pData + offset ) ;
            offset += objTemp.objsize() ;
            ++recordNum ;
         }
         catch( std::exception& e )
         {
            PD_LOG( PDERROR, "Failed to create bson object: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( offset < _pData->header.messageLength )
      {
         _pData->header.messageLength = offset ;
      }

      if ( recordNum < _recordNum )
      {
         _recordNum = recordNum ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COSUBCON_APPENDDATA, "coordSubContext::appendData" )
   void _coordSubContext::appendData( const pmdEDUEvent &event )
   {
      PD_TRACE_ENTRY ( SDB_COSUBCON_APPENDDATA ) ;
      SDB_ASSERT( event._Data != NULL, "Event's data can't be NULL" ) ;

      pmdEduEventRelease( _event, NULL ) ;

      _event = event ;
      _pData = ( MsgOpReply* )_event._Data ;

      _routeID = _pData->header.routeID ;
      _recordNum = _pData->numReturned ;
      _curOffset = ossAlign4( (UINT32)sizeof( MsgOpReply ) ) ;
      _isOrderKeyChange = TRUE ;
      _startFrom = _pData->startFrom ;
      PD_TRACE_EXIT ( SDB_COSUBCON_APPENDDATA ) ;
   }

   void _coordSubContext::clearData()
   {
      pmdEduEventRelease( _event, NULL ) ;
      _event.reset() ;
      _pData = NULL ;
      _curOffset = 0 ;
      _recordNum = 0 ;
      _isOrderKeyChange = TRUE ;
   }

   MsgRouteID _coordSubContext::getRouteID()
   {
      return _routeID;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COSUBCON_FRONT, "coordSubContext::front" )
   const CHAR* _coordSubContext::front ()
   {
      PD_TRACE_ENTRY ( SDB_COSUBCON_FRONT ) ;
      if ( _recordNum > 0 && _pData->header.messageLength > _curOffset )
      {
         PD_TRACE_EXIT ( SDB_COSUBCON_FRONT ) ;
         return ( (CHAR *)_pData + _curOffset ) ;
      }
      else
      {
         PD_TRACE_EXIT ( SDB_COSUBCON_FRONT ) ;
         return NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COSUBCON_POP, "coordSubContext::pop" )
   INT32 _coordSubContext::pop()
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_COSUBCON_POP ) ;
      do
      {
         if ( _curOffset >= _pData->header.messageLength )
         {
            SDB_ASSERT( FALSE, "data-buffer is empty!" );
            rc = SDB_RTN_COORD_CACHE_EMPTY ;
            PD_LOG ( PDWARNING, "Failed to pop the data, reach the end of the "
                     "buffer" ) ;
            break;
         }
         try
         {
            BSONObj boRecord( (CHAR *)_pData + _curOffset ) ;
            _curOffset += boRecord.objsize() ;
            _curOffset = ossAlign4( (UINT32)_curOffset ) ;
            _isOrderKeyChange = TRUE ;
            --_recordNum ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Failed to pop the data, occur unexpected "
                     "error(%s)", e.what() ) ;
         }
      }while ( FALSE ) ;

      PD_TRACE_EXITRC ( SDB_COSUBCON_POP, rc ) ;
      return rc;
   }

   INT32 _coordSubContext::popN( INT32 num )
   {
      INT32 rc = SDB_OK ;
      while ( num > 0 )
      {
         rc = pop() ;
         if ( rc != SDB_OK )
         {
            break ;
         }
         --num ;
      }
      return rc;
   }

   INT32 _coordSubContext::popAll()
   {
      _recordNum = 0 ;
      _curOffset = _pData->header.messageLength ;
      _isOrderKeyChange = TRUE ;
      return SDB_OK ;
   }

   INT32 _coordSubContext::pushFront( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      INT32 alignedSize = ossAlign4( (UINT32)obj.objsize() ) ;
      INT32 fixHeader = ( UINT32 )ossAlign4( (UINT32)sizeof( MsgOpReply ) ) ;

      if ( _curOffset < alignedSize + fixHeader )
      {
         rc = SDB_NOSPC ;
      }
      else
      {
         ++_recordNum ;
         _curOffset -= alignedSize ;
         ossMemcpy( ( (CHAR*)_pData + _curOffset ),
                    obj.objdata(), obj.objsize() ) ;
      }

      return rc ;
   }

   INT32 _coordSubContext::recordNum()
   {
      return _recordNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COSUBCON_GENORDERKEY, "coordSubContext::genOrderKey" )
   INT32 _coordSubContext::genOrderKey()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COSUBCON_GENORDERKEY ) ;

      if ( !_isOrderKeyChange )
      {
         goto done ;
      }
      if ( _recordNum <= 0 )
      {
         _orderKey.clear() ;
         goto done ;
      }
      try
      {
         BSONObj boRecord( (CHAR *)_pData + _curOffset ) ;
         rc = _orderKey.generateKey( boRecord, _keyGen ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to get order-key(rc=%d)", rc ) ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to get order-key, occur unexpected "
                 "error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_COSUBCON_GENORDERKEY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnContextCoordExplain implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextCoordExplain, RTN_CONTEXT_COORD_EXP, "COORD_EXPLAIN" )

   _rtnContextCoordExplain::_rtnContextCoordExplain ( INT64 contextID, UINT64 eduID,
                                                      BOOLEAN preRead )
   : _rtnContextCoord( contextID, eduID, preRead ),
     _explainCoordPath()
   {
   }

   _rtnContextCoordExplain::~_rtnContextCoordExplain ()
   {
   }

   const CHAR* _rtnContextCoordExplain::name () const
   {
      return "COORD_EXPLAIN" ;
   }

   RTN_CONTEXT_TYPE _rtnContextCoordExplain::getType () const
   {
      return RTN_CONTEXT_COORD_EXP ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP_OPEN, "_rtnContextCoordExplain::open" )
   INT32 _rtnContextCoordExplain::open ( const rtnQueryOptions & options,
                                         BOOLEAN preRead )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP_OPEN ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      _setPreRead( preRead ) ;

      rc = _openExplain( options, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open explain, rc: %d", rc ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP_OPEN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP_OPTRETOPTS, "_rtnContextCoordExplain::optimizeReturnOptions" )
   void _rtnContextCoordExplain::optimizeReturnOptions ( MsgOpQuery * pQueryMsg,
                                                         UINT32 targetGroupNum )
   {
      PD_TRACE_ENTRY( SDB_CTXCOORDEXP_OPTRETOPTS ) ;

      rtnContextCoord * coordContext =
                        dynamic_cast<rtnContextCoord *>( _getSubContext() ) ;

      if ( NULL != coordContext )
      {
         optMergeNodeBase * mergeNode = _explainCoordPath.getMergeNode() ;

         coordContext->optimizeReturnOptions( pQueryMsg, targetGroupNum ) ;

         // Reset local skip and limit values
         // NOTE: no need to reset _queryOptions which is the options passed to
         //       COORD rather than options executed on COORD
         _numToSkip = coordContext->getNumToSkip() ;
         _numToReturn = coordContext->getNumToReturn() ;

         if ( NULL != mergeNode )
         {
            mergeNode->getReturnOptions().setSkip( _numToSkip ) ;
            mergeNode->getReturnOptions().setLimit( _numToReturn ) ;
         }
      }

      PD_TRACE_EXIT( SDB_CTXCOORDEXP_OPTRETOPTS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP_ADDSUBCTX, "_rtnContextCoordExplain::addSubContext" )
   INT32 _rtnContextCoordExplain::addSubContext ( const pmdEDUEvent &event,
                                                  BOOLEAN &takeOver )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP_ADDSUBCTX ) ;

      rtnContextCoord * coordContext =
                     dynamic_cast<rtnContextCoord *>( _getSubContext() ) ;

      PD_CHECK( NULL != coordContext, SDB_RTN_CONTEXT_NOTEXIST, error,
                PDERROR, "Failed to get COORD context" ) ;

      rc = coordContext->addSubContext( event, takeOver ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add sub context, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP_ADDSUBCTX, rc ) ;
      rc = SDB_OK ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP_ADDSUBDONE, "_rtnContextCoordExplain::addSubDone" )
   void _rtnContextCoordExplain::addSubDone ( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB_CTXCOORDEXP_ADDSUBDONE ) ;

      if ( _enabledPreRead() )
      {
         rtnContextCoord * coordContext =
                        dynamic_cast<rtnContextCoord *>( _getSubContext() ) ;

         if ( NULL != coordContext )
         {
            coordContext->addSubDone( cb ) ;
         }
      }

      PD_TRACE_EXIT( SDB_CTXCOORDEXP_ADDSUBDONE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP__PREPAREDATA, "_rtnContextCoordExplain::_prepareData" )
   INT32 _rtnContextCoordExplain::_prepareData ( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP__PREPAREDATA ) ;

      rc = _prepareExplain( this, cb ) ;
      if ( SDB_DMS_EOC != rc &&
           SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare explain, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP__PREPAREDATA, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP__OPENSUBCTX, "_rtnContextCoordExplain::_openSubContext" )
   INT32 _rtnContextCoordExplain::_openSubContext ( rtnQueryOptions & options,
                                                     pmdEDUCB * cb,
                                                     rtnContextPtr *ppContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP__OPENSUBCTX ) ;

      SDB_ASSERT( ppContext, "context pointer is invalid" ) ;

      SDB_RTNCB * rtnCB = sdbGetRTNCB() ;

      INT64 queryContextID = -1 ;
      rtnContextCoord::sharePtr queryContext ;
      BOOLEAN needResetSubQuery = TRUE ;
      rtnQueryOptions subOptions( options ) ;

      rc = rtnCB->contextNew( RTN_CONTEXT_COORD, queryContext,
                              queryContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new main-collection "
                   "context, rc: %d", rc ) ;

      PD_CHECK( queryContext, SDB_SYS, error, PDERROR,
                "Failed to get the context of query" ) ;

      rc = _registerExplainProcessor( queryContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register explain processor, "
                   "rc: %d", rc ) ;

      // Reset selector
      if ( options.testFlag( FLG_QUERY_STRINGOUT ) )
      {
         needResetSubQuery = TRUE ;
      }
      else
      {
         rtnGetMergedSelector( options.getSelector(), options.getOrderBy(), needResetSubQuery ) ;
      }

      // Will not process selector in coord context
      if ( !needResetSubQuery )
      {
         subOptions.setSelector( BSONObj() ) ;
      }

      rc = queryContext->open( subOptions, _enabledPreRead() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open main-collection context, "
                   "rc: %d", rc ) ;

      rc = _explainCoordPath.createCoordPath( queryContext.get() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create MERGE node, rc: %d", rc ) ;

      _explainCoordPath.setCollectionName( options.getCLFullName() ) ;

      if ( _expOptions.isNeedRun() )
      {
         queryContext->setEnableMonContext( TRUE ) ;
      }

      if ( NULL != ppContext )
      {
         *ppContext = queryContext ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP__OPENSUBCTX, rc ) ;
      return rc ;

   error :
      if ( -1 != queryContextID )
      {
         rtnCB->contextDelete( queryContextID, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP__PREPAREEXPPATH, "_rtnContextCoordExplain::_prepareExplainPath" )
   INT32 _rtnContextCoordExplain::_prepareExplainPath ( rtnContext * context,
                                                        pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP__PREPAREEXPPATH ) ;

      SDB_ASSERT( NULL != context, "query context is invalid" ) ;

      if ( _expOptions.isNeedRun() )
      {
         // Calculate wait time
         rtnContextCoord * coordContext = NULL ;
         ossTickDelta waitTime ;

         coordContext = dynamic_cast<rtnContextCoord *>( context ) ;
         PD_CHECK( NULL != coordContext, SDB_SYS, error, PDERROR,
                   "Failed to get coord context" ) ;

         waitTime.fromUINT64( getWaitTime() * 1000 ) ;
         _explainCoordPath.getContextMonitor().setWaitTime( waitTime ) ;
      }

      if ( _expOptions.isNeedDetail() )
      {
         rc = _explainCoordPath.evaluate() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to evaluate MERGE path, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP__PREPAREEXPPATH, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP__BLDSIMPEXP, "_rtnContextCoordExplain::_buildSimpleExplain" )
   INT32 _rtnContextCoordExplain::_buildSimpleExplain ( rtnContext * explainContext,
                                                        BOOLEAN & hasMore )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP__BLDSIMPEXP ) ;

      optExplainResultList & childExplainList =
                                    _explainCoordPath.getChildExplains() ;
      if ( childExplainList.empty() )
      {
         BSONObjBuilder builder ;

         rc = _buildBSONNodeInfo( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for node info, "
                      "rc: %d", rc ) ;

         rc = _buildBSONQueryOptions( builder, _expOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for query options, "
                      "rc: %d", rc ) ;

         rc = explainContext->append( builder.obj() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed append explain result to "
                      "context [%lld], rc: %d", explainContext->contextID(),
                      rc ) ;

         hasMore = FALSE ;
      }
      else
      {
         rc = _buildSubExplains( explainContext, FALSE, hasMore ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build simple explain, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP__BLDSIMPEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXCOORDEXP__PARSELOCFILTER, "_rtnContextCoordExplain::_parseLocationOption" )
   INT32 _rtnContextCoordExplain::_parseLocationOption ( const BSONObj & explainOptions,
                                                         BOOLEAN & hasOption )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXCOORDEXP__PARSELOCFILTER ) ;

      BSONElement ele ;
      BSONObj locationOption ;
      coordCtrlParam ctrlParam ;
      CoordGroupList groupList ;

      coordResource *pResource = sdbGetResourceContainer()->getResource() ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      ctrlParam._isGlobal = TRUE ;

      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;

      try
      {
         hasOption = FALSE ;

         // Get location option
         ele = explainOptions.getField( FIELD_NAME_CMD_LOCATION ) ;
         if ( Object == ele.type() )
         {
            locationOption = ele.embeddedObject() ;
         }
         else
         {
            ele = explainOptions.getField( FIELD_NAME_LOCATION ) ;
            if ( Object == ele.type() )
            {
               locationOption = ele.embeddedObject() ;
            }
         }

         if ( locationOption.isEmpty() )
         {
            if ( explainOptions.hasField( FIELD_NAME_SUB_COLLECTIONS ) )
            {
               // The COORD doesn't need "SubCollections" option,
               // but it need to make sure "Detail" option is enabled
               hasOption = TRUE ;
            }
            goto done ;
         }

         hasOption = TRUE ;

         rc = coordParseControlParam( locationOption, ctrlParam,
                                      COORD_CTRL_MASK_NODE_SELECT,
                                      NULL, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse control param failed, rc: %d",
                      rc ) ;

         rc = coordParseGroupList( pResource, cb, locationOption, groupList,
                                   NULL, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse groups, rc: %d", rc  ) ;

         rc = coordGetGroupNodes( pResource, cb, locationOption,
                                  ctrlParam._emptyFilterSel, groupList,
                                  _locationFilter, NULL, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc  ) ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CTXCOORDEXP__PARSELOCFILTER, rc ) ;
      return rc ;

   error :
      _locationFilter.clear() ;
      goto done ;
   }

   BOOLEAN _rtnContextCoordExplain::_needChildExplain ( INT64 dataID,
                                                        const BSONObj & childExplain )
   {
      return _locationFilter.empty() ||
             _locationFilter.end() != _locationFilter.find( (UINT64)dataID ) ;
   }

}
