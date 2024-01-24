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
      // do nothing
   }

   INT32 _rtnRSHandler::onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst )
   {
      // No process such as authority needs to be don. So, do nothing.
      return SDB_OK ;
   }

   _rtnMsgHandler::_rtnMsgHandler( pmdRemoteSessionMgr *pRSManager )
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
         PD_LOG( ( ( SDB_INVALIDARG == rc ) ? PDWARNING : PDERROR ),
                 "Push message[%s] failed, rc: %d",
                 msg2String( header, MSG_MASK_ALL, 0 ).c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMSGHANDLER_HANDLEMSG, rc ) ;
      return rc ;
   error:
      if ( ! ( rc == SDB_INVALIDARG && IS_REPLY_TYPE( header->opCode ) ) )
      {
         rc = SDB_NET_BROKEN_MSG ;
      }
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
   INT32 _rtnMsgHandler::handleConnect( const NET_HANDLE &handle,
                                        _MsgRouteID id,
                                        BOOLEAN isPositive )
   {
      PD_TRACE_ENTRY( SDB__RTNMSGHANDLER_HANDLECONNECT ) ;
      _pRSManager->handleConnect( handle, id, isPositive ) ;
      PD_TRACE_EXIT( SDB__RTNMSGHANDLER_HANDLECONNECT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTESITEHANDLE_WAITEVENT, "_rtnRemoteSiteHandle::waitEvent" )
   BOOLEAN _rtnRemoteSiteHandle::waitEvent( pmdEDUEvent &event, INT64 timeout )
   {
      PD_TRACE_ENTRY( SDB__RTNREMOTESITEHANDLE_WAITEVENT ) ;
      BOOLEAN gotEvent = FALSE ;

      if ( 0 > timeout )
      {
         _queue.wait_and_pop( event ) ;
         gotEvent = TRUE ;
      }
      else
      {
         gotEvent = _queue.timed_wait_and_pop( event, timeout ) ;
      }

      PD_TRACE_EXIT( SDB__RTNREMOTESITEHANDLE_WAITEVENT ) ;
      return gotEvent ;
   }

   void _rtnRemoteSiteHandle::postEvent( const pmdEDUEvent &event )
   {
      _queue.push ( event ) ;
   }

   void _rtnRemoteMgrHandle::onRegister( _pmdRemoteSessionSite *pSite,
                                         _pmdEDUCB *cb )
   {
      rtnRemoteSiteHandle *rsHandle = SDB_OSS_NEW rtnRemoteSiteHandle() ;
      if ( rsHandle )
      {
         pSite->setHandle( rsHandle ) ;
      }
   }

   void _rtnRemoteMgrHandle::onUnreg( _pmdRemoteSessionSite *pSite,
                                      _pmdEDUCB *cb )
   {
      rtnRemoteSiteHandle *rsHandle = (rtnRemoteSiteHandle *)pSite->getHandle() ;
      if ( rsHandle )
      {
         SDB_OSS_DEL rsHandle ;
         pSite->setHandle( NULL ) ;
      }
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

      rc = _rsMgr.init( &_routeAgent, &_rMgrHandle ) ;
      PD_RC_CHECK( rc, PDERROR, "Init remote session manager failed[ %d ]",
                   rc ) ;
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

   void _rtnRemoteMessenger::deactive()
   {
      _routeAgent.stop() ;
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
      {
         ossScopedRWLock lock( &_lock, EXCLUSIVE ) ;
         _ready = TRUE ;
      }

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

      _routeAgent.setLocalID( id ) ;

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

      // If we can find the site by educb, it has registered already. Return its
      // user data as the session ID.
      site = _rsMgr.getSite( cb ) ;
      if ( site )
      {
         sessionID = site->getUserData() ;
         goto done ;
      }
      else
      {
         site = _rsMgr.registerEDU( cb ) ;
      }

      session = site->addSession( -1, &_rsHandler ) ;
      if ( !session )
      {
         PD_LOG( PDERROR, "Add session to session site failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      (void)session->addSubSession( _targetNodeID ) ;
      sessionID = session->sessionID() ;
      // Use the sessionID as session data.
      site->setUserData( sessionID ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREMOTEMESSENGER_REMOVESESSION, "_rtnRemoteMessenger::removeSession" )
   INT32 _rtnRemoteMessenger::removeSession( pmdEDUCB *cb, UINT64 *sessionID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREMOTEMESSENGER_REMOVESESSION ) ;
      pmdRemoteSessionSite* site = _rsMgr.getSite( cb ) ;
      if ( !site )
      {
         goto done ;
      }

      if ( site->sessionCount() > 0 )
      {
         UINT64 sessionIDLocal = sessionID ? *sessionID : site->getUserData() ;
         pmdRemoteSession* session = site->getSession( sessionIDLocal ) ;
         if ( !session )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Session with id[ %llu ] dose not exist",
                    site->getUserData() ) ;
            goto error ;
         }
         site->removeSession( session ) ;
      }
      // If it's the last session in the session site, unregister.
      if ( 0 == site->sessionCount() )
      {
         _rsMgr.unregEUD( cb ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_REMOVESESSION, rc ) ;
      return rc ;
   error:
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

      if ( !site->getHandle() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Remote site handler is invalid" ) ;
         goto error ;
      }

      if ( !subSession )
      {
         PD_LOG( PDERROR, "Sub session does not exist" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      subSession->setReqMsg( (MsgHeader *)msg, PMD_EDU_MEM_NONE ) ;
      subSession->resetForResend() ;
      rc = session->sendMsg( subSession ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Send message to search engine adapter "
                   "failed[ %d ]", rc ) ;
         goto error ;
      }

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

      reply = (MsgOpReply *)subSession->getRspMsg() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNREMOTEMESSENGER_RECEIVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnRemoteMessenger::onDisconnect()
   {
      ossScopedRWLock lock( &_lock, EXCLUSIVE ) ;
      _ready = FALSE ;
   }
}

