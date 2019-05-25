/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnRemoteMessenger.cpp

   Descriptive Name = RunTime Remote Messenger

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/12/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdRemoteSession.hpp"
#include "pmdController.hpp"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "rtnRemoteMessenger.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   _rtnRSHandler::_rtnRSHandler()
   {
   }

   _rtnRSHandler::~_rtnRSHandler()
   {
   }

   INT32 _rtnRSHandler::onSendFailed( _pmdRemoteSession *pSession,
                                      _pmdSubSession **ppSub,
                                      INT32 flag )
   {
      return flag ;
   }

   void _rtnRSHandler::onReply( _pmdRemoteSession *pSession,
                                _pmdSubSession **ppSub,
                                const MsgHeader *pReply,
                                BOOLEAN isPending )
   {
   }

   INT32 _rtnRSHandler::onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst )
   {
      return SDB_OK ;
   }

   _rtnMsgHandler::_rtnMsgHandler( _pmdRemoteSessionMgr *pRSManager )
   {
      _pRSManager = pRSManager ;
   }

   _rtnMsgHandler::~_rtnMsgHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMSGHANDLER_HANDLEMSG, "_rtnMsgHandler::handleMsg" )
   INT32 _rtnMsgHandler::handleMsg( const NET_HANDLE &handle,
                                    const _MsgHeader *header,
                                    const CHAR *msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNMSGHANDLER_HANDLEMSG ) ;

      rc = _pRSManager->pushMessage( handle, header ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Push message[%s] failed, rc: %d",
                 msg2String( header, MSG_MASK_ALL, 0 ).c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMSGHANDLER_HANDLEMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMSGHANDLER_HANDLECLOSE, "_rtnMsgHandler::handleClose" )
   void _rtnMsgHandler::handleClose( const NET_HANDLE &handle, _MsgRouteID id )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;
      PD_TRACE_ENTRY( SDB__RTNMSGHANDLER_HANDLECLOSE ) ;
      _pRSManager->handleClose( handle, id ) ;
      PD_TRACE_EXIT( SDB__RTNMSGHANDLER_HANDLECLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMSGHANDLER_HANDLECONNECT, "_rtnMsgHandler::handleConnect" )
   void _rtnMsgHandler::handleConnect( const NET_HANDLE &handle,
                                       _MsgRouteID id,
                                       BOOLEAN isPositive )
   {
      PD_TRACE_ENTRY( SDB__RTNMSGHANDLER_HANDLECONNECT ) ;
      _pRSManager->handleConnect( handle, id, isPositive ) ;
      PD_TRACE_EXIT( SDB__RTNMSGHANDLER_HANDLECONNECT ) ;
   }

   _rtnRemoteMessenger::_rtnRemoteMessenger()
   : _msgHandler( &_rsMgr ),
     _routeAgent( &_msgHandler ),
     _ready( FALSE ),
     _targetNodeID( 0 )
   {
   }

   _rtnRemoteMessenger::~_rtnRemoteMessenger()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_INIT, "_rtnRemoteMessenger::init" )
   INT32 _rtnRemoteMessenger::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_INIT ) ;

      rc = _rsMgr.init( &_routeAgent, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Init remote session manager failed[ %d ]",
                   rc ) ;
      sdbGetPMDController()->setRSManager( &_rsMgr ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_ACTIVE, "_rtnRemoteMessenger::active" )
   INT32 _rtnRemoteMessenger::active()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_ACTIVE ) ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      rc = eduMgr->startEDU( EDU_TYPE_RTNNETWORK, (void *)&_routeAgent,
                             &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Start external search network failed[ %d ]",
                   rc ) ;
      rc = eduMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait external search net active failed[ %d ]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_ACTIVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_SETTARGET, "_rtnRemoteMessenger::setTarget" )
   INT32 _rtnRemoteMessenger::setTarget( const _MsgRouteID &id,
                                         const CHAR *host, const CHAR *service )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_SETTARGET ) ;

      if ( MSG_INVALID_ROUTEID == id.value || !host || !service )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid route info argument" ) ;
         goto error ;
      }

      rc = _routeAgent.updateRoute( id, host, service ) ;
      if ( rc )
      {
         if ( SDB_NET_UPDATE_EXISTING_NODE != rc )
         {
            PD_LOG( PDERROR, "Update route failed[ %d ], host[ %s ], "
                    "service[ %s ]", rc, host, service ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }
      _targetNodeID = id.value ;
      _ready = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_SETTARGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_SETLOCALID, "_rtnRemoteMessenger::setLocalID" )
   INT32 _rtnRemoteMessenger::setLocalID( const MsgRouteID &id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_SETLOCALID ) ;
      if ( MSG_INVALID_ROUTEID == id.value )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid route info argument" ) ;
         goto error ;
      }

      if ( MSG_INVALID_ROUTEID == _routeAgent.localID().value )
      {
         _routeAgent.setLocalID( id ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_SETLOCALID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_PREPARESESSION, "_rtnRemoteMessenger::prepareSession" )
   INT32 _rtnRemoteMessenger::prepareSession( pmdEDUCB *cb, UINT64 &sessionID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_PREPARESESSION ) ;
      pmdRemoteSessionSite* site = NULL ;
      pmdRemoteSession* session = NULL ;

      site = _rsMgr.registerEDU( cb ) ;
      session = site->addSession( -1, &_rsHandler ) ;
      if ( !session )
      {
         PD_LOG( PDERROR, "Add session to session site failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      (void)session->addSubSession( _targetNodeID ) ;
      sessionID = session->sessionID() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_PREPARESESSION, rc ) ;
      return rc ;
   error:
      if ( site )
      {
         _rsMgr.unregEUD( cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_SEND, "_rtnRemoteMessenger::send" )
   INT32 _rtnRemoteMessenger::send( UINT64 sessionID, const MsgHeader *msg,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_SEND ) ;

      pmdRemoteSessionSite* site = _rsMgr.getSite( cb ) ;
      pmdRemoteSession* session = site->getSession( sessionID ) ;
      pmdSubSession* subSession = session->getSubSession( _targetNodeID ) ;
      if ( !subSession )
      {
         PD_LOG( PDERROR, "Sub session does not exist" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      subSession->setReqMsg( (MsgHeader *)msg, PMD_EDU_MEM_NONE ) ;
      subSession->resetForResend() ;
      rc = session->sendMsg( subSession ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to search engine adapter "
                   "failed[ %d ]", rc ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_SEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_RECEIVE, "_rtnRemoteMessenger::receive" )
   INT32 _rtnRemoteMessenger::receive( UINT64 sessionID, pmdEDUCB *cb,
                                       MsgOpReply *&reply )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_RECEIVE ) ;
      pmdRemoteSessionSite* site = _rsMgr.getSite( cb ) ;
      pmdRemoteSession* session = site->getSession( sessionID ) ;
      pmdSubSession* subSession = session->getSubSession( _targetNodeID ) ;
      if ( !subSession )
      {
         PD_LOG( PDERROR, "Sub session does not exist" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = session->waitReply( TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait reply failed[ %d ]", rc ) ;

      subSession = session->getSubSession( _targetNodeID ) ;
      if ( !subSession )
      {
         PD_LOG( PDERROR, "Get subsession of search engine adapter "
                 "failed[ %d ]", rc ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      reply = (MsgOpReply *)subSession->getRspMsg( FALSE ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_RECEIVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_REMOVESESSION, "_rtnRemoteMessenger::removeSession" )
   INT32 _rtnRemoteMessenger::removeSession( UINT64 sessionID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_REMOVESESSION ) ;
      pmdRemoteSessionSite* site = _rsMgr.getSite( cb ) ;

      if ( !site )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Remote session site is NULL" ) ;
         goto error ;
      }

      if ( 1 == site->sessionCount() )
      {
         _rsMgr.unregEUD( cb ) ;
      }
      else
      {
         pmdRemoteSession* session = site->getSession( sessionID ) ;
         if ( !session )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Session with id[ %llu ] dose not exist",
                    sessionID ) ;
            goto error ;
         }
         site->removeSession( session ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_REMOVESESSION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnRemoteMessenger::removeSession( pmdEDUCB *cb )
   {
      _rsMgr.unregEUD( cb ) ;
   }
}

