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

   Source File Name = fapMongoSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/03/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoSession.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEnv.hpp"
#include "monCB.hpp"
#include "msg.hpp"
#include "../../bson/bson.hpp"
#include "rtnCommandDef.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "sdbInterface.hpp"
#include "fapMongoTrace.hpp"
#include "pdTrace.hpp"

using namespace engine ;

namespace fap
{

#define SET_MONGO_MSG_FLAG( userData )   ( userData | 0x8000000000000000 )
#define UNSET_MONGO_MSG_FLAG( userData ) ( userData & 0x7FFFFFFFFFFFFFFF )
#define IS_MONGO_MSG( userData )         ( userData & 0x8000000000000000 )
#define IS_MONGO_REQUEST( msgData )     \
            ( ((mongoMsgHeader*)msgData)->requestId != 0 )
#define IS_MONGO_RESPONSE( msgData )    \
            ( ((mongoMsgHeader*)msgData)->responseTo != 0 )

#define FAP_MONGO_ERROR_RESPONSE_MAX_LEN 128

static INT32 buildGetMoreSdbMsg( UINT64 requestID, INT64 contextID,
                                 mongoMsgBuffer &out )
{
   INT32 rc = SDB_OK ;
   MsgOpGetMore *pGetmore = NULL ;

   if ( !out.empty() )
   {
      out.zero() ;
   }

   rc = out.reserve( sizeof( MsgOpGetMore ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = out.advance( sizeof( MsgOpGetMore ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pGetmore = (MsgOpGetMore *)out.data() ;
   pGetmore->header.messageLength = sizeof( MsgOpGetMore ) ;
   pGetmore->header.opCode = MSG_BS_GETMORE_REQ ;
   pGetmore->header.requestID = requestID ;
   pGetmore->header.routeID.value = 0 ;
   pGetmore->header.TID = 0 ;
   pGetmore->contextID = contextID ;
   pGetmore->numToReturn = -1 ;

done:
   return rc ;
error:
   goto done ;
}

/////////////////////////////////////////////////////////////////
// implement for mongo processor

_mongoSession::_mongoSession( SOCKET fd, engine::IResource *pResource )
                : engine::pmdSession( fd ), _masterRead( FALSE ),
                  _pResource( pResource ),
                  _isAuthed( FALSE ), _requestIDOfPostEvent( 0 ),
                  _opCodeOfPostEvent( 0 ),
                  _cursorIdOfPostEvent( SDB_INVALID_CONTEXTID ),
                  _clFullName( NULL )
{
}

_mongoSession::~_mongoSession()
{
   _pResource = NULL ;
   _resetBuffers() ;
}

void _mongoSession::_resetBuffers()
{
   if ( 0 != _contextBuff.size() )
   {
      _contextBuff.release() ;
   }

   if ( !_inBuffer.empty() )
   {
      _inBuffer.zero() ;
   }
}

void _mongoSession::_eduEventRelease( pmdEDUEvent &event )
{
   pmdEduEventRelease( event, NULL ) ;
   event.reset() ;
}

INT32 _mongoSession::getServiceType() const
{
   return CMD_SPACE_SERVICE_LOCAL ;
}

engine::SDB_SESSION_TYPE _mongoSession::sessionType() const
{
   return engine::SDB_SESSION_PROTOCOL ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PREPROCESS, "_mongoSession::preProcess" )
BOOLEAN _mongoSession::preProcess( pmdEDUEvent &event )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PREPROCESS ) ;
   INT32 rc = SDB_OK ;
   BOOLEAN processed = FALSE ;

   // fap and coord are the same thread, with different roles
   if ( PMD_EDU_EVENT_MSG == event._eventType &&
        IS_MONGO_MSG( event._userData ) )
   {
      try
      {
         _fapEvents.push( event ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when pushing event: %s, "
                 "rc: %d", e.what(), rc ) ;
         _postInnerErrorEvent( rc, event ) ;
      }
      processed = TRUE ;
   }

   PD_TRACE_EXIT( SDB_FAPMONGO_PREPROCESS ) ;
   return processed ;
}

/*
eg:
            request event
   A session ---------------> B session
                                 |
                                 | process event
                                 |
            response event       V
   A session <--------------- B session

   Because of error, B session won't send response event to A session.
   In this situation, A session will be stuck until it receives a response event
   In order to deal with the stuck problem, B session should send a response
   header( _fapMongoInnerHeader ) to tell A session that the event you
   post has failed to process. After, A session need to build a error reponse
   by itself
*/
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_POSTINNERERROREVENT, "_mongoSession::_postInnerErrorEvent" )
void _mongoSession::_postInnerErrorEvent( INT32 errorCode,
                                          engine::pmdEDUEvent &event )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_POSTINNERERROREVENT ) ;
   INT32 rc = SDB_OK ;
   pmdEDUMgr *pEduMgr = pmdGetKRCB()->getEDUMgr() ;
   UINT64 sourceEDUID = UNSET_MONGO_MSG_FLAG( event._userData ) ; ;
   CHAR *pRes = NULL ;
   UINT32 pResLen = sizeof( fapMongoInnerHeader ) ;
   fapMongoInnerHeader errResHeader ;

   _eduEventRelease( event ) ;

   // if it failed to post response event to another edu,
   // it means that the edu we will post doesn't exist,
   // so we don't need to post response event
   if ( SDB_OOM == errorCode )
   {
      rc = pEduMgr->postEDUPost( sourceEDUID, PMD_EDU_EVENT_MSG,
                                 PMD_EDU_MEM_NONE,
                                 mongoGetOOMErrResHeader(),
                                 SET_MONGO_MSG_FLAG( eduID() ) ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "edu[%llu] failed to post inner response "
                  "to edu[%llu], rc: %d", eduID(), sourceEDUID, rc ) ;
         goto done ;
      }
   }
   else
   {
      pRes = (CHAR*)SDB_THREAD_ALLOC( pResLen ) ;
      if ( NULL == pRes )
      {
         PD_LOG ( PDERROR, "edu[%llu] failed to post inner response "
                  "to edu[%llu], rc: %d", eduID(), sourceEDUID, rc ) ;
         rc = pEduMgr->postEDUPost( sourceEDUID, PMD_EDU_EVENT_MSG,
                                    PMD_EDU_MEM_NONE,
                                    mongoGetOOMErrResHeader(),
                                    SET_MONGO_MSG_FLAG( eduID() ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "edu[%llu] failed to post inner response "
                     "to edu[%llu], rc: %d", eduID(), sourceEDUID, rc ) ;
            goto done ;
         }
      }
      else
      {
         ossMemset( pRes, 0, pResLen ) ;

         errResHeader.errorCode = errorCode ;
         ossMemcpy( pRes, (const CHAR*)&errResHeader, pResLen ) ;

         rc = pEduMgr->postEDUPost( sourceEDUID, PMD_EDU_EVENT_MSG,
                                    PMD_EDU_MEM_THREAD, pRes,
                                    SET_MONGO_MSG_FLAG( eduID() ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "edu[%llu] failed to post inner response "
                     "to edu[%llu], rc: %d", eduID(), sourceEDUID, rc ) ;
            if ( pRes )
            {
               SDB_THREAD_FREE( pRes ) ;
            }
            goto done ;
         }
      }
   }

   PD_LOG( PDDEBUG, "edu[%llu] post inner response to edu[%llu] successfully",
           eduID(), sourceEDUID ) ;

done:
   PD_TRACE_EXIT( SDB_FAPMONGO_POSTINNERERROREVENT ) ;
   return ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSCLIENTMSG, "_mongoSession::_processClientMsg" )
INT32 _mongoSession::_processClientMsg( const CHAR* pMsg,
                                        _mongoCommand *&pCommand,
                                        mongoSessionCtx &sessCtx,
                                        BOOLEAN &msgForwarded )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSCLIENTMSG ) ;
   INT32 rc = SDB_OK ;
   mongoCursorInfo cursorInfo ;
   BOOLEAN needNext = FALSE ;
   BOOLEAN needReply = TRUE ;
   BOOLEAN isOwned = TRUE ;
   msgForwarded = FALSE ;

   if ( NULL == pMsg )
   {
      needReply = FALSE ;
      goto done ;
   }

   rc = mongoGetAndInitCommand( pMsg, &pCommand, sessCtx ) ;
   if ( rc )
   {
      goto error ;
   }

   // receive message from socket, check this operation is owned by
   // current session or not
   _getCursorInfo( pCommand, cursorInfo, isOwned ) ;

next:
   rc = mongoBuildSdbMsg( &pCommand, sessCtx, _inBuffer ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( isOwned )
   {
      rc = _processOwnedClientMsg( pMsg, pCommand, sessCtx, needNext ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to process owned client "
                   "msg, rc: %d", sessionName(), rc ) ;

      if ( needNext )
      {
         goto next ;
      }
   }
   else
   {
      rc = _processNonOwnedClientMsg( pMsg, pCommand, cursorInfo, sessCtx ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to process non-owned client "
                   "msg, rc: %d", sessionName(), rc ) ;
      needReply = FALSE ;
      msgForwarded = TRUE ;
   }

done:
   if ( needReply )
   {
      rc = _reply( pCommand, pMsg, rc, sessCtx.errorObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Session[%s] failed to reply, rc: %d",
                 sessionName(), rc ) ;
      }
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSCLIENTMSG, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSOWNEDCLIENTMSG, "_mongoSession::_processOwnedClientMsg" )
INT32 _mongoSession::_processOwnedClientMsg( const CHAR* pMsg,
                                             _mongoCommand *pCommand,
                                             mongoSessionCtx &sessCtx,
                                             BOOLEAN &needNext )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSOWNEDCLIENTMSG ) ;
   INT32 rc = SDB_OK ;
   needNext = FALSE ;

   if ( pCommand->needProcessByEngine() )
   {
      rc = _processMsg( _inBuffer.data(), pCommand, sessCtx.errorObj ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _manageCursor( pCommand, _replyHeader ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( CMD_SASL_CONTINUE == pCommand->type() && SDB_OK == rc )
      {
         _isAuthed = TRUE ;
      }
   }

   rc = pCommand->parseSdbReply( _replyHeader, _contextBuff ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( !pCommand->hasProcessAllMsg() )
   {
      needNext = TRUE ;
      goto done ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSOWNEDCLIENTMSG, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSNONOWNEDCLIENTMSG, "_mongoSession::_processNonOwnedClientMsg" )
INT32 _mongoSession::_processNonOwnedClientMsg( const CHAR* pMsg,
                                                _mongoCommand *pCommand,
                                                mongoCursorInfo cursorInfo,
                                                mongoSessionCtx &sessCtx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSNONOWNEDCLIENTMSG ) ;
   INT32 rc = SDB_OK ;
   INT32 msgLen = ((mongoMsgHeader*)pMsg)->msgLen ;
   CHAR* pReq = NULL ;
   pmdEDUMgr *pEduMgr = pmdGetKRCB()->getEDUMgr() ;

   // It is NOT owned by current session, then check authenticate
   // and post event to the thread which owned this cursor
   if ( cursorInfo.needAuth && !_isAuthed )
   {
      rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
      PD_LOG( PDERROR, "Not authorized to execute %s command, "
              "rc: %d", pCommand->name(), rc ) ;
      goto error ;
   }

   pReq = (CHAR*)SDB_THREAD_ALLOC( msgLen ) ;
   if ( NULL == pReq )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "edu[%llu] will post mongo request to "
              "edu[%llu] but edu[%llu] failed to alloc memory, "
              "rc: %d", eduID(), cursorInfo.EDUID, eduID(), rc ) ;
      goto error ;
   }
   ossMemcpy( pReq, pMsg, msgLen ) ;

   rc = pEduMgr->postEDUPost( cursorInfo.EDUID, PMD_EDU_EVENT_MSG,
                              PMD_EDU_MEM_THREAD,
                              pReq,
                              SET_MONGO_MSG_FLAG( eduID() ) ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "edu[%llu] failed to post mongo request "
               "to edu[%llu], rc: %d", eduID(), cursorInfo.EDUID, rc ) ;

      if ( pReq )
      {
         SDB_THREAD_FREE( pReq ) ;
      }

      goto error ;
   }

   pReq = NULL ;

   PD_LOG( PDDEBUG, "edu[%llu] post mongo request to edu[%llu] "
           "successfully", eduID(), cursorInfo.EDUID ) ;

   // we need to wait for response event
   _requestIDOfPostEvent = ((mongoMsgHeader*)pMsg)->requestId ;
   _opCodeOfPostEvent = ((mongoMsgHeader*)pMsg)->opCode ;

   if ( MONGO_OP_KILL_CURSORS == ((mongoMsgHeader*)pMsg)->opCode )
   {
      _cursorIdOfPostEvent = MONGO_INVALID_CURSORID ;
   }
   else
   {
      _cursorIdOfPostEvent = cursorInfo.cursorID ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSNONOWNEDCLIENTMSG, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSINTERIORMSG, "_mongoSession::_processInteriorMsg" )
// Receive message from event, it may be getMore command or killCursor command
INT32 _mongoSession::_processInteriorMsg( _mongoCommand *&pCommand,
                                          engine::pmdEDUEvent &event,
                                          mongoSessionCtx &sessCtx,
                                          BOOLEAN &hasRecvResponse )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSINTERIORMSG ) ;
   INT32 rc = SDB_OK ;
   hasRecvResponse = FALSE ;

   if ( NULL == event._Data )
   {
      goto done ;
   }

   if ( IS_MONGO_REQUEST( event._Data ) )
   {
      PD_LOG( PDDEBUG,
              "edu[%llu] wait mongo request from edu[%llu]",
              eduID(), UNSET_MONGO_MSG_FLAG( event._userData ) ) ;
      _processRequestMsg( pCommand, event, sessCtx ) ;
   }
   else if ( IS_MONGO_RESPONSE( event._Data ) )
   {
      PD_LOG( PDDEBUG,
              "edu[%llu] wait mongo response from edu[%llu]",
              eduID(), UNSET_MONGO_MSG_FLAG( event._userData ) ) ;
      rc = _reply( event ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to send response msg, "
                   "rc: %d", sessionName(), rc ) ;
      hasRecvResponse = TRUE ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Unknown msg, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSINTERIORMSG, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSREQUESTMSG, "_mongoSession::_processRequestMsg" )
void _mongoSession::_processRequestMsg( _mongoCommand *&pCommand,
                                        engine::pmdEDUEvent &event,
                                        mongoSessionCtx &sessCtx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSREQUESTMSG ) ;
   INT32 rc = SDB_OK ;
   UINT64 sourceEDUID = UNSET_MONGO_MSG_FLAG( event._userData ) ;
   pmdEDUMgr *pEduMgr = pmdGetKRCB()->getEDUMgr() ;
   CHAR* pRes = NULL ;

   rc = mongoGetAndInitCommand( (CHAR*)event._Data, &pCommand, sessCtx ) ;
   if ( rc )
   {
      _postInnerErrorEvent( rc, event ) ;
      goto done ;
   }

   rc = mongoBuildSdbMsg( &pCommand, sessCtx, _inBuffer ) ;
   if ( rc )
   {
      _postInnerErrorEvent( rc, event ) ;
      goto done ;
   }

   _processMsg( _inBuffer.data(), pCommand, sessCtx.errorObj ) ;

   rc = _buildResponse( pCommand, pRes ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "edu[%llu] failed to build response of "
               "edu[%llu], rc: %d", eduID(), sourceEDUID, rc ) ;
      _postInnerErrorEvent( rc, event ) ;
      if ( pRes )
      {
         SDB_THREAD_FREE( pRes ) ;
      }
      goto done ;
   }

   _eduEventRelease( event ) ;

   rc = pEduMgr->postEDUPost( sourceEDUID, PMD_EDU_EVENT_MSG,
                              PMD_EDU_MEM_THREAD,
                              pRes,
                              SET_MONGO_MSG_FLAG( eduID() ) ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "edu[%llu] failed to post mongo response to "
               "edu[%llu], rc: %d", eduID(), sourceEDUID, rc ) ;
      if ( pRes )
      {
         SDB_THREAD_FREE( pRes ) ;
      }
      goto done ;
   }

   PD_LOG( PDDEBUG, "edu[%llu] post mongo response to edu[%llu] "
           "successfully", eduID(), sourceEDUID ) ;

done:
   PD_TRACE_EXIT( SDB_FAPMONGO_PROCESSREQUESTMSG ) ;
   return ;
}

/*

eg:

Query command is executed in two steps: query operation and getMore operation

In SequoiaDB, these two operations must be performed in the same session.
But in MongoDB, these two operations can be performed in different session.

Under a query command, when we receive query operation message in A session,
and receive getMore operation message in B session, we need to forward the
getMore message to Session B for execution, and B session will send response
to A session.

              getMore request msg
   A session ----------------------> B session
                                         |
                                         | process msg
                                         |
              getMore response msg       V
   A session <---------------------- B session

*/
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_RUN, "_mongoSession::run" )
INT32 _mongoSession::run()
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_RUN ) ;
   INT32 rc  = SDB_OK ;
   CHAR *pMsg = NULL ;
   _mongoCommand* pCommand = NULL ;
   mongoSessionCtx sessCtx ;
   _pmdRemoteSessionSite *pSite = NULL ;
   pmdEDUEvent event ;
   BOOLEAN hasMsg = TRUE ;
   BOOLEAN needWaitResponse = FALSE ;
   BOOLEAN hasRecvResponse = FALSE ;
   BOOLEAN msgForwarded = FALSE ;

   pSite = ( _pmdRemoteSessionSite* )(eduCB()->getRemoteSite()) ;
   // In standalone mode, pSite is NULL
   if ( pSite )
   {
      pSite->setMsgPreprocessor( this ) ;
   }

   PD_CHECK( _pEDUCB, SDB_SYS, error, PDERROR,
             "_pEDUCB is null" ) ;
   PD_CHECK( FALSE == mongoCheckBigEndian(), SDB_SYS, error, PDERROR,
             "Big endian is not support" ) ;

   while ( !_pEDUCB->isDisconnected() && !_socket.isClosed() )
   {
      if ( hasMsg )
      {
         _pEDUCB->resetInterrupt() ;
         _pEDUCB->resetInfo( engine::EDU_INFO_ERROR ) ;
         _pEDUCB->resetLsn() ;

         _resetBuffers() ;
         sessCtx.resetError() ;
         sessCtx.sessionName = sessionName() ;
         sessCtx.eduID = eduID() ;

         if ( pCommand )
         {
            mongoReleaseCommand( &pCommand ) ;
         }

         hasMsg = FALSE ;
      }

      if ( !needWaitResponse )
      {
         rc = _recvMsgFromClient( pMsg, hasMsg ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] recevie message from client failed, "
                      "rc: %d", sessionName(), rc ) ;

         if ( hasMsg )
         {
            rc = _processClientMsg( pMsg, pCommand, sessCtx,
                                    msgForwarded ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Session[%s] failed to process client msg, rc: %d",
                         sessionName(), rc ) ;

            if ( msgForwarded )
            {
               needWaitResponse = TRUE ;
            }

            continue;
         }
      }

      rc = _recvMsgFromInterior( event, hasMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] recevie message from interior failed, "
                   "rc: %d", sessionName(), rc ) ;

      if ( hasMsg )
      {
         rc = _processInteriorMsg( pCommand, event, sessCtx,
                                   hasRecvResponse ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] failed to process interior msg, rc: %d",
                      sessionName(), rc ) ;

         if ( hasRecvResponse )
         {
            needWaitResponse = FALSE ;
         }
      }
   }

done:
   // In standalone mode, pSite is NULL
   if ( pSite )
   {
      pSite->setMsgPreprocessor( NULL ) ;
   }
   if ( pCommand )
   {
      mongoReleaseCommand( &pCommand ) ;
   }
   _eduEventRelease( event ) ;
   _resetBuffers() ;
   disconnect() ;
   PD_TRACE_EXITRC( SDB_FAPMONGO_RUN, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_RECVMSGFROMINTERIOR, "_mongoSession::_recvMsgFromInterior" )
INT32 _mongoSession::_recvMsgFromInterior( engine::pmdEDUEvent &event,
                                           BOOLEAN &hasMsg )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_RECVMSGFROMINTERIOR ) ;
   INT32 rc = SDB_OK ;
   pmdEDUEvent tmpEvent ;
   hasMsg = FALSE ;

   while ( !_fapEvents.empty() )
   {
      _fapEvents.try_pop( tmpEvent ) ;
      _pEDUCB->postEvent( tmpEvent ) ;
   }

   while( !_pEDUCB->isInterrupted() && !_pEDUCB->isDisconnected() &&
          _pEDUCB->waitEvent( event, 0 )  )
   {
      // fap and coord are the same thread, with different roles
      if ( PMD_EDU_EVENT_MSG != event._eventType ||
           !IS_MONGO_MSG( event._userData ) )
      {
         // When Query preRead receive query response, it will send out
         // GETMORE request to data node immediately. At this time we may
         // get GETMORE response, and we just ignore it.
         try
         {
            _coordEvents.push( event ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "An exception occurred when push coord event: "
                    "%s, rc: %d", e.what(), rc ) ;
            goto error ;
         }
         // event may be released by other threads using event in
         // _coordEvents, so we must reset event after pushing
         event.reset() ;
         PD_LOG( PDDEBUG, "wait unexpected event" ) ;
         continue ;
      }

      while ( !_coordEvents.empty() )
      {
         _coordEvents.try_pop( tmpEvent ) ;
         _pEDUCB->postEvent( tmpEvent ) ;
      }
      hasMsg = TRUE ;
      break ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_RECVMSGFROMINTERIOR, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_RECVMSGFROMCLIENT, "_mongoSession::_recvMsgFromClient" )
INT32 _mongoSession::_recvMsgFromClient( CHAR *&pMsg, BOOLEAN &hasMsg )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_RECVMSGFROMCLIENT ) ;
   INT32 rc = SDB_OK ;
   UINT32 msgSize = 0 ;
   UINT32 headerLen = sizeof( mongoMsgHeader ) ;
   pmdEDUEvent tmpEvent ;
   hasMsg = FALSE ;

   while ( !_fapEvents.empty() )
   {
      _fapEvents.try_pop( tmpEvent ) ;
      _pEDUCB->postEvent( tmpEvent ) ;
   }

   if( !_pEDUCB->isInterrupted() && !_pEDUCB->isDisconnected() )
   {
      rc = recvData( (CHAR*)&msgSize, sizeof(UINT32), 10 ) ;
      if ( SDB_TIMEOUT == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( SDB_APP_FORCED == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to receive message, rc: %d",
                   sessionName(), rc ) ;

      PD_CHECK( msgSize >= headerLen && msgSize <= SDB_MAX_MSG_LENGTH,
                SDB_INVALIDARG, error, PDERROR,
                "Session[%s] receive message size[%d] is invalid",
                msgSize ) ;

      // alloc memory
      pMsg = getBuff( msgSize ) ;
      PD_CHECK( pMsg, SDB_OOM, error, PDERROR, "Out of memory" ) ;
      *(UINT32*)pMsg = msgSize ;

      // receive rest of message
      rc = recvData( pMsg + sizeof(UINT32), msgSize - sizeof(UINT32) ) ;
      if ( SDB_APP_FORCED == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to receive rest of message, rc: %d",
                   sessionName(), rc ) ;

      while ( !_coordEvents.empty() )
      {
         _coordEvents.try_pop( tmpEvent ) ;
         _pEDUCB->postEvent( tmpEvent ) ;
      }
      hasMsg = TRUE ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_RECVMSGFROMCLIENT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETCURSORINFO, "_mongoSession::_getCursorInfo" )
void _mongoSession::_getCursorInfo( const _mongoCommand *pCommand,
                                    mongoCursorInfo &cursorInfo,
                                    BOOLEAN &isOwned )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETCURSORINFO ) ;
   _mongoCursorMgr* pCursorMgr = getMongoCursorMgr() ;
   isOwned = TRUE ;
   cursorInfo.cursorID = MONGO_INVALID_CURSORID ;

   if ( CMD_GETMORE == pCommand->type() )
   {
      cursorInfo.cursorID = ((_mongoGetmoreCommand*)pCommand)->cursorID() ;
   }
   else if ( CMD_KILL_CURSORS == pCommand->type() )
   {
      _mongoKillCursorCommand* pKillCmd = (_mongoKillCursorCommand*)pCommand ;
      // 'cusror.close()' send only one cursor.
      // 'db.runCommand( { killCursors: "bar", cursors: [1,2,...] } )' may send
      // multiple cursors, but we DON'T support it yet.
      if ( pKillCmd->cursorList().size() > 0 )
      {
         cursorInfo.cursorID = pKillCmd->cursorList().front() ;
      }
   }

   if ( cursorInfo.cursorID != MONGO_INVALID_CURSORID )
   {
      // First look up the cursor from local list. If nothing is found, then
      // look up from cursor mgr. The local list is to reduce access cursor mgr.
      if ( _cursorList.find( cursorInfo.cursorID ) == _cursorList.end() )
      {
         mongoCursorInfo cursorInfoTmp ;
         BOOLEAN foundOut = pCursorMgr->find( cursorInfo.cursorID,
                                              cursorInfoTmp ) ;
         if ( foundOut )
         {
            cursorInfo.EDUID = cursorInfoTmp.EDUID ;
            cursorInfo.needAuth = cursorInfoTmp.needAuth ;
            isOwned = FALSE ;
         }
      }
   }

   PD_TRACE_EXIT( SDB_FAPMONGO_GETCURSORINFO ) ;
   return ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_MANAGECURSOR, "_mongoSession::_manageCursor" )
INT32 _mongoSession::_manageCursor( const _mongoCommand *pCommand,
                                    const MsgOpReply &sdbReply )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_MANAGECURSOR ) ;
   INT32 rc = SDB_OK ;
   _mongoCursorMgr* pCursorMgr = getMongoCursorMgr() ;

   switch ( pCommand->type() )
   {
      case CMD_FIND :
      case CMD_QUERY :
      case CMD_AGGREGATE :
      case CMD_LIST_COLLECTION :
      case CMD_LIST_INDEX :
      {
         INT64 cursorID = SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ;
         if ( cursorID != MONGO_INVALID_CURSORID )
         {
            _cursorList.insert( cursorID ) ;

            mongoCursorInfo cursorInfo ;
            cursorInfo.cursorID = cursorID ;
            cursorInfo.EDUID = eduID() ;
            cursorInfo.needAuth = _isAuthed ;
            rc = pCursorMgr->insert( cursorInfo ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         break ;
      }
      case CMD_KILL_CURSORS :
      {
         _mongoKillCursorCommand* pKillCmd = (_mongoKillCursorCommand*)pCommand ;
         const vector<INT64>& cursorList = pKillCmd->cursorList() ;

         vector<INT64>::const_iterator it ;
         for( it = cursorList.begin() ; it != cursorList.end() ; it++ )
         {
            _cursorList.erase( *it ) ;
            pCursorMgr->remove( *it ) ;
         }
         break ;
      }
      case CMD_GETMORE :
      {
         // if getmore command return zero for the cursor id,
         // it means the cursor is closed by engine.
         INT64 cursorID = SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ;
         if ( MONGO_INVALID_CURSORID == cursorID )
         {
            _mongoGetmoreCommand* pMoreCmd = (_mongoGetmoreCommand*)pCommand ;
            _cursorList.erase( pMoreCmd->cursorID() ) ;
            pCursorMgr->remove( pMoreCmd->cursorID() ) ;
         }
         break ;
      }
      default:
         break ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_MANAGECURSOR, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOCREATECS, "_mongoSession::_autoCreateCS" )
INT32 _mongoSession::_autoCreateCS( const CHAR *pCsName, BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOCREATECS ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;
   BSONObj obj, empty ;

   _tmpBuffer.zero() ;

   rc = _tmpBuffer.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )_tmpBuffer.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = 0 ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   try
   {
      obj = BSON( FIELD_NAME_NAME << pCsName << FIELD_NAME_PAGE_SIZE << 65536 ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb createCS "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _tmpBuffer.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.write( obj, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   _tmpBuffer.doneLen() ;

   rc = _processMsg( (CHAR*)pQuery, errorObj ) ;
   if ( SDB_OK == rc )
   {
      PD_LOG( PDEVENT,
              "Session[%s]: Create collection space[%s] automatically",
              sessionName(), pCsName ) ;
   }
   else
   {
      PD_LOG( PDWARNING,
              "Session[%s]: failed to create collection space[%s] automatically"
              ", rc: %d", sessionName(), pCsName, rc ) ;
      if ( SDB_DMS_CS_EXIST == rc )
      {
         rc = SDB_OK ;
      }
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOCREATECS, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOINSERT, "_mongoSession::_autoInsert" )
INT32 _mongoSession::_autoInsert( const CHAR *pClFullName,
                                  const BSONObj &matcher,
                                  const BSONObj &updatorObj,
                                  BSONObj &target,
                                  BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOINSERT ) ;
   INT32 rc = SDB_OK ;
   MsgOpInsert *pInsert = NULL ;

   rc = mongoGenerateNewRecord( matcher, updatorObj, target ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to generate new record, rc: %d", rc ) ;
      goto error ;
   }

   _tmpBuffer.zero() ;

   rc = _tmpBuffer.reserve( sizeof( MsgOpInsert ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.advance( sizeof( MsgOpInsert ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pInsert = ( MsgOpInsert *)_tmpBuffer.data() ;
   pInsert->header.opCode = MSG_BS_INSERT_REQ ;
   pInsert->header.TID = 0 ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.requestID = 0 ;
   pInsert->version = 0 ;
   pInsert->w = 0 ;
   pInsert->padding = 0 ;
   pInsert->flags = FLG_INSERT_RETURNNUM ;

   pInsert->nameLength = ossStrlen( pClFullName ) ;

   rc = _tmpBuffer.write( pClFullName, pInsert->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.write( target, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   _tmpBuffer.doneLen() ;

   rc = _processMsg( (CHAR*)pInsert, errorObj ) ;
   if ( rc )
   {
      PD_LOG( PDERROR,
              "Session[%s]: failed to insert automatically"
              ", rc: %d", sessionName(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOINSERT, rc ) ;
   return rc ;
error:
   _replyHeader.flags = rc ;
   errorObj = mongoGetErrorBson( rc ) ;
   _contextBuff = engine::rtnContextBuf( errorObj ) ;
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOCEATECL, "_mongoSession::_autoCreateCL" )
INT32 _mongoSession::_autoCreateCL( const CHAR *pCSName,
                                    const CHAR *pClFullName,
                                    BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOCEATECL ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
   BSONObj obj, empty ;

   while( TRUE )
   {
      _tmpBuffer.zero() ;

      rc = _tmpBuffer.reserve( sizeof( MsgOpQuery ) ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;
      if ( rc )
      {
         goto error ;
      }

      pQuery = ( MsgOpQuery * )_tmpBuffer.data() ;
      pQuery->header.opCode = MSG_BS_QUERY_REQ ;
      pQuery->header.TID = 0 ;
      pQuery->header.routeID.value = 0 ;
      pQuery->header.requestID = 0 ;
      pQuery->version = 0 ;
      pQuery->w = 0 ;
      pQuery->padding = 0 ;
      pQuery->flags = 0 ;
      pQuery->nameLength = ossStrlen( pCmdName ) ;
      pQuery->numToSkip = 0 ;
      pQuery->numToReturn = -1 ;

      try
      {
         obj = BSON( FIELD_NAME_NAME << pClFullName ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when building sdb createCL "
                 "request: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

      rc = _tmpBuffer.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( obj, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      _tmpBuffer.doneLen() ;

      rc = _processMsg( (CHAR*)pQuery, errorObj ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = _autoCreateCS( pCSName, errorObj ) ;
         if ( rc )
         {
            break ;
         }
      }
      else
      {
         break ;
      }
   }

   if ( SDB_OK == rc )
   {
      PD_LOG( PDEVENT,
              "Session[%s]: Create collection[%s] automatically",
              sessionName(), pClFullName ) ;
   }
   else
   {
      PD_LOG( PDWARNING,
              "Session[%s]: failed to create collection[%s] automatically"
              ", rc: %d", sessionName(), pClFullName, rc ) ;
      if ( SDB_DMS_EXIST == rc )
      {
         rc = SDB_OK ;
      }
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOCEATECL, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOKILLCURRSOR, "_mongoSession::_autoKillCursor" )
INT32 _mongoSession::_autoKillCursor( UINT64 requestID, INT64 contextID )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOKILLCURRSOR ) ;
   INT32 rc = SDB_OK ;
   MsgOpKillContexts *pKill = NULL ;
   BSONObj errorObj ;
   BSONObj returnObjCpy ;
   UINT64 requestIDCpy = _replyHeader.header.requestID ;
   INT32 flagsCpy = _replyHeader.flags ;

   // _contextBuff will be released in _processMsg,
   // so we should call getOwned()
   if ( NULL != _contextBuff.data() )
   {
      returnObjCpy = BSONObj( _contextBuff.data() ).getOwned() ;
   }

   _tmpBuffer.zero() ;

   rc = _tmpBuffer.reserve( sizeof( MsgOpKillContexts ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.advance( sizeof( MsgOpKillContexts ) - sizeof( SINT64 ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pKill = ( MsgOpKillContexts * )_tmpBuffer.data() ;
   pKill->header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
   pKill->header.TID = 0 ;
   pKill->header.routeID.value = 0 ;
   pKill->header.requestID = requestID ;
   pKill->ZERO = 0 ;
   pKill->numContexts = 1 ;

   rc = _tmpBuffer.write( (CHAR*)&contextID, sizeof( SINT64 ) ) ;
   if ( rc )
   {
      goto error ;
   }

   _tmpBuffer.doneLen() ;

   rc = _processMsg( (CHAR*)pKill, errorObj ) ;
   if ( rc )
   {
      PD_LOG( PDWARNING,
              "Session[%s]: failed to kill cursor[cursorID: %lld] "
              "automatically, rc: %d",
              sessionName(), contextID, rc ) ;
      goto error ;
   }
   else
   {
      PD_LOG( PDEVENT, "Session[%s] kill cursor[cursorID: %lld] "
              "automatically", sessionName(), contextID ) ;
   }

done:
   _replyHeader.flags = flagsCpy ;
   _replyHeader.contextID = SDB_INVALID_CONTEXTID ;
   _replyHeader.header.requestID = requestIDCpy ;
   _contextBuff = engine::rtnContextBuf( returnObjCpy ) ;
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOKILLCURRSOR, rc ) ;
   return rc ;
error:
   goto done ;
}

BOOLEAN _mongoSession::_shouldAutoCrtCS( const _mongoCommand *pCommand )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;

   MONGO_CMD_TYPE cmdType = pCommand->type() ;
   BOOLEAN isUpsert = ( CMD_UPDATE == cmdType &&
                        ((_mongoUpdateCommand*)pCommand)->isUpsert() ) ;
   BOOLEAN isFindAndUpsert = ( CMD_FINDANDMODIFY == cmdType &&
                        ((_mongoFindandmodifyCommand*)pCommand)->isUpsert() ) ;

   if ( CMD_COLLECTION_CREATE == cmdType ||
        CMD_INSERT == cmdType ||
        CMD_INDEX_CREATE == cmdType ||
        isUpsert ||
        isFindAndUpsert )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN _mongoSession::_shouldAutoCrtCL( const _mongoCommand *pCommand )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;

   MONGO_CMD_TYPE cmdType = pCommand->type() ;
   BOOLEAN isUpsert = ( CMD_UPDATE == cmdType &&
                        ((_mongoUpdateCommand*)pCommand)->isUpsert() ) ;
   BOOLEAN isFindAndUpsert = ( CMD_FINDANDMODIFY == cmdType &&
                        ((_mongoFindandmodifyCommand*)pCommand)->isUpsert() ) ;

   if ( CMD_INSERT == cmdType ||
        CMD_INDEX_CREATE == cmdType ||
        isUpsert ||
        isFindAndUpsert )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN _mongoSession::_shouldBuildGetMoreMsg( const _mongoCommand *pCommand )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;

   MONGO_CMD_TYPE cmdType = pCommand->type() ;

   if ( SDB_OK != _replyHeader.flags ||
        SDB_INVALID_CONTEXTID == _replyHeader.contextID )
   {
      return FALSE ;
   }

   if ( CMD_COUNT     == cmdType || CMD_LIST_INDEX      == cmdType ||
        CMD_AGGREGATE == cmdType || CMD_LIST_COLLECTION == cmdType ||
        CMD_DISTINCT  == cmdType || CMD_LIST_DATABASE   == cmdType ||
        CMD_LIST_USER == cmdType )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }

}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSMSG1, "_mongoSession::_processMsg" )
INT32 _mongoSession::_processMsg( const CHAR *pMsg,
                                  const _mongoCommand *pCommand,
                                  BSONObj &errorObj )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSMSG1 ) ;
   INT32 rc = SDB_OK ;
   INT32 orgOpCode = ((MsgHeader*)pMsg)->opCode ;
   BOOLEAN hasBuildGetMore = FALSE ;
   MONGO_CMD_TYPE cmdType = pCommand->type() ;
   BOOLEAN isNewCS = FALSE ;
   BOOLEAN needKillCursor = FALSE ;
   _clFullName = pCommand->clFullName() ;

   while ( TRUE )
   {
      rc = _processMsg( pMsg, errorObj ) ;

      // auto create cs/cl
      if ( SDB_DMS_CS_NOTEXIST == _replyHeader.flags )
      {
         if ( _shouldAutoCrtCS( pCommand ) )
         {
            rc = _autoCreateCS( pCommand->csName(), errorObj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to auto create cs, rc: %d", rc ) ;
               goto error ;
            }

            ((MsgHeader*)pMsg)->opCode = orgOpCode ;
            isNewCS = TRUE ;

            if ( CMD_COLLECTION_CREATE == cmdType )
            {
               continue ;
            }
         }
      }

      if ( isNewCS || SDB_DMS_NOTEXIST == _replyHeader.flags )
      {
         if ( _shouldAutoCrtCL( pCommand ) )
         {
            rc = _autoCreateCL( pCommand->csName(), pCommand->clFullName(),
                                errorObj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to auto create cl, rc: %d", rc ) ;
               goto error ;
            }

            ((MsgHeader*)pMsg)->opCode = orgOpCode ;
            isNewCS = FALSE ;
            continue ;
         }
      }
      // findAndUpsert is executed in two steps

      // first: findAndModify( matcher, update )
      // if the first step returns empty data, it means that the record we are
      // looking for doesn't exist. Then we need to perform the second step.
      // if the first step doesn't return empty data, we don't need to
      // perform the second step.

      // second: insert( new record )
      // we should generate a new record we will insert based on
      // matcher condition and update condition firstly. Then we insert this
      // new record.
      if ( ( CMD_FINDANDMODIFY == cmdType &&
           ((_mongoFindandmodifyCommand*)pCommand)->isUpsert()) &&
           ( _contextBuff.size() == 0 ) &&
           SDB_OK == _replyHeader.flags )
      {
         _mongoFindandmodifyCommand* pCmd = (_mongoFindandmodifyCommand*)pCommand ;

         rc = _autoInsert( pCmd->clFullName(), pCmd->getCond(),
                           pCmd->getUpdater(), pCmd->getUpsertReturnRecord(),
                           errorObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to auto insert, rc: %d", rc ) ;
            goto error ;
         }

         _contextBuff = engine::rtnContextBuf( pCmd->getUpsertReturnRecord() ) ;
         pCmd->setHasInsertRecord( TRUE ) ;
         break ;
      }
      else if ( !hasBuildGetMore && _shouldBuildGetMoreMsg( pCommand ) )
      {
         rc = buildGetMoreSdbMsg( _replyHeader.header.requestID,
                                  _replyHeader.contextID, _inBuffer ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to build sdb getMore msg, rc: %d", rc ) ;
            goto error;
         }

         hasBuildGetMore = TRUE ;
         // In SequoiaDB, count command is executed in three steps:
         // first: count, and return a cursor
         // second: getMore, and return count number
         // third: close cursor
         if ( CMD_COUNT == cmdType )
         {
            needKillCursor = TRUE ;
         }

         continue ;
      }
      else if ( needKillCursor )
      {
         _autoKillCursor( _replyHeader.header.requestID,
                          _replyHeader.contextID ) ;
         break ;
      }

      break ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSMSG1, rc ) ;
   return rc ;
error:
   _replyHeader.flags = rc ;
   errorObj = mongoGetErrorBson( rc ) ;
   _contextBuff = engine::rtnContextBuf( errorObj ) ;
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSMSG2, "_mongoSession::_processMsg" )
INT32 _mongoSession::_processMsg( const CHAR *pMsg, BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSMSG2 ) ;
   INT32   rc           = SDB_OK ;
   BOOLEAN needReply    = FALSE ;
   BOOLEAN needRollback = FALSE ;
   bson::BSONObjBuilder retBuilder ;

   rc = _onMsgBegin( (MsgHeader *) pMsg ) ;

   _contextBuff.release() ;

   if ( SDB_OK == rc )
   {
      rc = getProcessor()->processMsg( (MsgHeader *) pMsg, _contextBuff,
                                       _replyHeader.contextID,
                                       needReply, needRollback, retBuilder ) ;

      _replyHeader.numReturned = _contextBuff.recordNum() ;
      _replyHeader.startFrom   = (INT32)_contextBuff.getStartFrom() ;
   }
   _replyHeader.flags       = rc ;

   if ( rc )
   {
      _buildErrorObj( _contextBuff, _replyHeader.flags, errorObj ) ;
      _contextBuff = engine::rtnContextBuf( errorObj ) ;
      _replyHeader.numReturned = 1 ;

      if ( needRollback )
      {
         PD_LOG( PDDEBUG,
                 "Session rolling back operation[opCode: %d], rc: %d",
                 ((MsgHeader*)pMsg)->opCode, rc ) ;

         INT32 rcTmp = getProcessor()->doRollback() ;
         PD_RC_CHECK( rcTmp, PDERROR,
                      "Session failed to rollback trans info, rc: %d",
                      rcTmp ) ;
      }
   }
   else
   {
      errorObj = BSONObj() ;

      // In standalone mode, we can get InsertedNum, DuplicatedNum, UpdatedNum,
      // ModifiedNum and DeletedNum from retBuilder.obj().
      if ( !retBuilder.isEmpty() && 0 == _contextBuff.size() )
      {
         _contextBuff = engine::rtnContextBuf( retBuilder.obj() ) ;
      }
   }

   _onMsgEnd( rc, (MsgHeader *) pMsg ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSMSG2, rc ) ;
   return rc ;
error:
   goto done ;
}

void _mongoSession::_buildErrorObj( const engine::rtnContextBuf &contextBuff,
                                    INT32 errCode, BSONObj &errObj )
{
   INT32 rc = SDB_OK ;
   BSONObj obj ;
   errObj = BSONObj() ;

   if ( NULL == contextBuff.data() )
   {
      goto done ;
   }

   obj = BSONObj( contextBuff.data() ) ;

   if ( SDB_IXM_DUP_KEY == errCode )
   {
      rc = mongoBuildDupkeyErrObj( obj, _clFullName, errObj ) ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   if ( errObj.isEmpty() )
   {
      errObj = mongoGetErrorBson( errCode ) ;
   }
   return ;
error:
   goto done ;
}

INT32 _mongoSession::_onMsgBegin( MsgHeader *pMsg )
{
   INT32 rc = SDB_OK ;
   // set reply header ( except flags, length )
   _replyHeader.contextID          = -1 ;
   _replyHeader.numReturned        = 0 ;
   _replyHeader.startFrom          = 0 ;
   _replyHeader.header.opCode      = MAKE_REPLY_TYPE(pMsg->opCode) ;
   _replyHeader.header.requestID   = pMsg->requestID ;
   _replyHeader.header.TID         = pMsg->TID ;
   _replyHeader.header.routeID     = engine::pmdGetNodeID() ;

   // start operator
   MON_START_OP( _pEDUCB->getMonAppCB() ) ;

done:
   return rc ;
error:
   goto done ;
}

void _mongoSession::_onMsgEnd( INT32 result, MsgHeader *pMsg )
{
   // release buff context
   //_contextBuff.release() ;

   if ( result && SDB_DMS_EOC != result )
   {
      PD_LOG( PDWARNING, "Session[%s] process msg[opCode=%d, len: %d, "
              "TID: %u, requestID: %llu] failed, rc: %d",
              sessionName(), pMsg->opCode, pMsg->messageLength, pMsg->TID,
              pMsg->requestID, result ) ;
   }

   // end operator
   MON_END_OP( _pEDUCB->getMonAppCB() ) ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_BUILDRESPONSE, "_mongoSession::_buildResponse" )
INT32 _mongoSession::_buildResponse( _mongoCommand *pCommand,
                                     CHAR *&pRes )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_BUILDRESPONSE ) ;
   INT32 rc = SDB_OK ;
   _mongoResponseBuffer headerBuf ;
   INT32 resSize = 0 ;

   rc = pCommand->buildMongoReply( _replyHeader, _contextBuff, headerBuf ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Session[%s] failed to build response, rc: %d",
                sessionName(), rc ) ;

   resSize = headerBuf.usedSize + _contextBuff.size() ;
   pRes = (CHAR*)SDB_THREAD_ALLOC( resSize ) ;
   PD_CHECK( pRes, SDB_OOM, error, PDERROR, "Out of memory" ) ;

   ossMemcpy( pRes, headerBuf.data, headerBuf.usedSize ) ;
   ossMemcpy( pRes + headerBuf.usedSize, _contextBuff.data(),
              _contextBuff.size() ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_BUILDRESPONSE, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_REPLY1, "_mongoSession::_reply" )
INT32 _mongoSession::_reply( _mongoCommand *pCommand, const CHAR* pMsg,
                             INT32 errCode, BSONObj &errObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_REPLY1 ) ;
   INT32 rc = SDB_OK ;
   _mongoResponseBuffer headerBuf ;

   if ( errCode )
   {
      if ( errObj.isEmpty() )
      {
         errObj = mongoGetErrorBson( errCode ) ;
      }

      _contextBuff = engine::rtnContextBuf( errObj ) ;
      _replyHeader.numReturned = 1 ;
      _replyHeader.flags = errCode ;
   }

   if ( pCommand && pCommand->isInitialized() )
   {
      rc = pCommand->buildMongoReply( _replyHeader, _contextBuff, headerBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to build response, rc: %d",
                   sessionName(), rc ) ;
   }
   else
   {
      _mongoMessage mongoMsg ;
      rc = mongoMsg.init( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to init message, rc: %d",
                   sessionName(), rc ) ;
      if ( MONGO_OP_COMMAND == mongoMsg.opCode() )
      {
         BSONObj empty ;
         BSONObj org( _contextBuff.data() ) ;

         _tmpBuffer.zero() ;

         rc = _tmpBuffer.write( org.objdata(), org.objsize() ) ;
         if ( rc )
         {
            goto error ;
         }


         rc = _tmpBuffer.write( empty.objdata(), empty.objsize() ) ;
         if ( rc )
         {
            goto error ;
         }

         _contextBuff = rtnContextBuf( _tmpBuffer.data(), _tmpBuffer.size(), 2 ) ;

         mongoCommandResponse res ;
         res.header.msgLen = sizeof( mongoCommandResponse ) +
                             _contextBuff.size() ;
         res.header.responseTo = mongoMsg.requestID() ;
         headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
      }
      else
      {
         mongoResponse res ;
         res.header.msgLen = sizeof( mongoResponse ) + _contextBuff.size() ;
         res.header.responseTo = mongoMsg.requestID() ;
         res.nReturned = _contextBuff.recordNum() ;
         headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
      }
   }

   PD_LOG( PDDEBUG, "Build mongodb reply msg[ tid: %d, session: %s, "
           "command: %s, clFullName: %s, eduID: %llu ] done",
           ossGetCurrentThreadID(),
           sessionName(),
           pCommand ? ( (pCommand)->name() ? (pCommand)->name() : "" ) : "",
           pCommand ? ( (pCommand)->clFullName() ?
           (pCommand)->clFullName() : "" ) : "",
           eduID() ) ;

   // send response
   if ( headerBuf.usedSize > 0 )
   {
      INT32 rcTmp = SDB_OK ;
      const mongoResponse *pRes = (mongoResponse*)headerBuf.data ;

      rcTmp = sendData( (CHAR *)&headerBuf, headerBuf.usedSize ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to send response header, rc: %d",
                   sessionName(), rcTmp ) ;

      if ( _contextBuff.data() )
      {
         rcTmp = sendData( _contextBuff.data(), _contextBuff.size() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] failed to send response body, rc: %d",
                      sessionName(), rcTmp ) ;
      }

      PD_LOG( PDDEBUG, "Send mongodb reply msg[ tid: %d, session: %s, "
              "command: %s, clFullName: %s, msgHead: { msgLen: %d,"
              " requestId: %d, responseTo: %d, opCode: %d, reservedFlags: %d,"
              " cursorId: %llu, startingFrom: %d, nReturned: %d }, "
              "eduID: %llu ] done",
              ossGetCurrentThreadID(),
              sessionName(),
              pCommand ? ( (pCommand)->name() ? (pCommand)->name() : "" ) : "",
              pCommand ? ( (pCommand)->clFullName() ?
              (pCommand)->clFullName() : "" ) : "",
              pRes->header.msgLen, pRes->header.requestId,
              pRes->header.responseTo, pRes->header.opCode, pRes->reservedFlags,
              pRes->cursorId, pRes->startingFrom, pRes->nReturned, eduID() ) ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_REPLY1, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_BUILDERRRESPONSEMSG, "_mongoSession::_buildErrResponseMsg" )
void _mongoSession::_buildErrResponseMsg( CHAR* pMsg, INT32 errorCode,
                                          INT32 &msgLen )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_BUILDERRRESPONSEMSG ) ;
   BSONObj errorBson = mongoGetErrorBson( errorCode ) ;

   if ( MONGO_OP_QUERY == _opCodeOfPostEvent ||
        MONGO_OP_GET_MORE == _opCodeOfPostEvent )
   {
      mongoResponse res ;
      res.header.msgLen = sizeof( res ) + errorBson.objsize() ;
      res.header.responseTo = _requestIDOfPostEvent ;
      res.nReturned = 1 ;
      // In MongoDB-Java-2.x driver, if we send cursorID that isn't 0,
      // the driver will send request message based on this cursorID to
      // fap again. But the context corresponding to this cursorID may have
      // been closed in SequoiaDB.
      res.cursorId = MONGO_INVALID_CURSORID ;
      msgLen = res.header.msgLen ;

      ossMemcpy( pMsg, (const CHAR*)&res, sizeof( res ) ) ;
      ossMemcpy( pMsg + sizeof( res ),
                 errorBson.objdata(), errorBson.objsize() ) ;
   }
   else if ( MONGO_OP_COMMAND == _opCodeOfPostEvent )
   {
      BSONObj empty ;
      mongoCommandResponse res ;
      res.header.msgLen = sizeof( res ) + errorBson.objsize() +
                          empty.objsize() ;
      res.header.responseTo = _requestIDOfPostEvent ;
      msgLen = res.header.msgLen ;

      ossMemcpy( pMsg, (const CHAR*)&res, sizeof( res ) ) ;
      ossMemcpy( pMsg + sizeof( res ),
                 errorBson.objdata(), errorBson.objsize() ) ;
      ossMemcpy( pMsg + sizeof( res ) + errorBson.objsize(),
                 empty.objdata(), empty.objsize() ) ;
   }
   else if ( MONGO_KILL_CURSORS_MSG == _opCodeOfPostEvent )
   {
      // Don't need to reply
      msgLen = 0 ;
   }

   PD_TRACE_EXIT( SDB_FAPMONGO_BUILDERRRESPONSEMSG ) ;
   return ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_REPLY2, "_mongoSession::_reply" )
INT32 _mongoSession::_reply( engine::pmdEDUEvent &event )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_REPLY2 ) ;
   INT32 rc = SDB_OK ;
   _fapMongoInnerHeader* pResHeader = (_fapMongoInnerHeader*)event._Data ;

   if ( pResHeader->header.opCode == MONGO_OP_REPLY ||
        pResHeader->header.opCode == MONGO_OP_COMMAND_REPLY )
   {
      rc = sendData( (CHAR*)event._Data, *((INT32*)event._Data) ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to send response header, "
                   "rc: %d", sessionName(), rc ) ;
   }
   // It means that the event isn't a normal MongoDB event.
   // It's a _fapMongoInnerHeader. For detail, see the comment of the
   // function fap::_mongoSession::_postInnerErrorEvent
   else if ( pResHeader->header.opCode == MONGO_OP_INNER_REPLY )
   {
      // The command must be getMore or killCursor
      CHAR resMsg[ FAP_MONGO_ERROR_RESPONSE_MAX_LEN ] ;
      INT32 sendDataLen = 0 ;
      _mongoCursorMgr* pCursorMgr = getMongoCursorMgr() ;

      ossMemset( resMsg, 0, FAP_MONGO_ERROR_RESPONSE_MAX_LEN ) ;

      _buildErrResponseMsg( resMsg, pResHeader->errorCode, sendDataLen ) ;

      _cursorList.erase( _cursorIdOfPostEvent ) ;
      pCursorMgr->remove( _cursorIdOfPostEvent ) ;
      _autoKillCursor( _requestIDOfPostEvent,
                       MGCURSOID_TO_SDBCTXID ( _cursorIdOfPostEvent ) ) ;

      _requestIDOfPostEvent = 0 ;
      _opCodeOfPostEvent = 0 ;
      _cursorIdOfPostEvent = MONGO_INVALID_CURSORID ;

      if ( sendDataLen > 0 )
      {
         rc = sendData( resMsg, sendDataLen ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] failed to send response, rc: %d",
                      sessionName(), rc ) ;
      }
   }
   else
   {
      rc = SDB_SYS ;
      PD_LOG ( PDERROR, "Unknown opCode: %d, rc: %d",
               pResHeader->header.opCode, rc ) ;
      goto error ;
   }

done:
   _eduEventRelease( event ) ;
   PD_TRACE_EXITRC( SDB_FAPMONGO_REPLY2, rc ) ;
   return rc ;
error:
   goto done ;
}

}
