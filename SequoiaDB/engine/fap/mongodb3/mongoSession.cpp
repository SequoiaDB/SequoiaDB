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

   Source File Name = mongoSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoSession.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEDU.hpp"
#include "pmdEnv.hpp"
#include "monCB.hpp"
#include "msg.hpp"
#include "../../bson/bson.hpp"
#include "rtnCommandDef.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "sdbInterface.hpp"
#include "fapMongoCursor.hpp"

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

static BOOLEAN checkBigEndian()
{
   BOOLEAN bigEndian = FALSE ;
   union
   {
      unsigned int i ;
      unsigned char s[4] ;
   } c ;

   c.i = 0x12345678 ;
   if ( 0x12 == c.s[0] )
   {
      bigEndian = TRUE ;
   }

   return bigEndian ;
}

static void buildGetMoreMsg( UINT64 requestID, INT64 contextID, msgBuffer &out )
{
   if ( !out.empty() )
   {
      out.zero() ;
   }
   out.reserve( sizeof( MsgOpGetMore ) ) ;
   out.advance( sizeof( MsgOpGetMore ) ) ;

   MsgOpGetMore *getmore = (MsgOpGetMore *)out.data() ;
   getmore->header.messageLength = sizeof( MsgOpGetMore ) ;
   getmore->header.opCode = MSG_BS_GETMORE_REQ ;
   getmore->header.requestID = requestID ;
   getmore->header.routeID.value = 0 ;
   getmore->header.TID = 0 ;
   getmore->contextID = contextID ;
   getmore->numToReturn = -1 ;
}

/////////////////////////////////////////////////////////////////
// implement for mongo processor

_mongoSession::_mongoSession( SOCKET fd, engine::IResource *resource )
   : engine::pmdSession( fd ), _masterRead( FALSE ),
     _resource( resource ), _needWaitResponse( FALSE ), _isAuthed( FALSE )
{
}

_mongoSession::~_mongoSession()
{
   _resource = NULL ;
}

void _mongoSession::_resetBuffers()
{
   // release buff context
   if ( 0 != _contextBuff.size() )
   {
      _contextBuff.release() ;
   }

   if ( !_inBuffer.empty() )
   {
      _inBuffer.zero() ;
   }
}

INT32 _mongoSession::getServiceType() const
{
   return CMD_SPACE_SERVICE_LOCAL ;
}

engine::SDB_SESSION_TYPE _mongoSession::sessionType() const
{
   return engine::SDB_SESSION_PROTOCOL ;
}

BOOLEAN _mongoSession::preProcess( pmdEDUEvent &event )
{
   BOOLEAN processed = FALSE ;

   if ( PMD_EDU_EVENT_MSG == event._eventType &&
        IS_MONGO_MSG( event._userData ) )
   {
      _tmpEventQue.push( event ) ;
      processed = TRUE ;
   }

   return processed ;
}

INT32 _mongoSession::run()
{
   INT32 rc  = SDB_OK ;
   CHAR *pMsg = NULL ;
   _mongoCommand* pCommand = NULL ;
   pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
   mongoSessionCtx sessCtx ;
   _pmdRemoteSessionSite *pSite = NULL ;

   pSite = ( _pmdRemoteSessionSite* )(eduCB()->getRemoteSite()) ;
   SDB_ASSERT( pSite, "site is null" ) ;
   pSite->setMsgPreprocessor( this ) ;

   PD_CHECK( _pEDUCB, SDB_SYS, error, PDERROR,
             "_pEDUCB is null" ) ;
   PD_CHECK( FALSE == checkBigEndian(), SDB_SYS, error, PDERROR,
             "Big endian is not support" ) ;

   while ( !_pEDUCB->isDisconnected() && !_socket.isClosed() )
   {
      _pEDUCB->resetInterrupt() ;
      _pEDUCB->resetInfo( engine::EDU_INFO_ERROR ) ;
      _pEDUCB->resetLsn() ;

      // receive message
      BOOLEAN recvFromEvent = FALSE ;
      pmdEDUEvent event ;
      rc = _recvMsg( pMsg, recvFromEvent, event ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] recevie message failed, rc: %d",
                   sessionName(), rc ) ;

      // convert mongo message to command
      _resetBuffers() ;
      sessCtx.resetError( _errorInfo ) ;

      rc = mongoGetAndInitCommand( pMsg, &pCommand, sessCtx, _inBuffer ) ;
      if ( rc )
      {
         rc = _reply( pCommand, pMsg, rc, sessCtx.errorObj ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] failed to reply, rc: %d",
                      sessionName(), rc ) ;
         continue ;
      }

      if ( !recvFromEvent )
      {
         // receive message from socket, check this operation is owned by
         // current session or not
         UINT64 ownedEDUID = 0 ;
         BOOLEAN needAuth = FALSE ;
         BOOLEAN isOwned = _isOwnedCursor( pCommand, ownedEDUID, needAuth ) ;
         if ( isOwned )
         {
            // It is owned by current session, then process it
            if ( pCommand->needProcessByEngine() )
            {
               rc = _processMsg( _inBuffer.data(), pCommand ) ;

               _manageCursor( pCommand, _replyHeader ) ;

               if ( CMD_SASL_CONTINUE == pCommand->type() && SDB_OK == rc )
               {
                  _isAuthed = TRUE ;
               }
            }

            rc = _reply( pCommand ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Session[%s] failed to reply, rc: %d",
                         sessionName(), rc ) ;
         }
         else
         {
            // It is NOT owned by current session, then check authenticate and
            // post event to the thread which owned this cursor
            INT32 msgLen = ((mongoMsgHeader*)pMsg)->msgLen ;
            CHAR* pReq = NULL ;

            if ( needAuth && !_isAuthed )
            {
               rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
               PD_LOG( PDERROR, "Not authorized to execute %s command, rc: %d",
                       pCommand->name(), rc ) ;
               rc = _reply( pCommand, pMsg, rc, BSONObj() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Session[%s] failed to reply, rc: %d",
                            sessionName(), rc ) ;
               continue ;
            }

            pmdEDUCB *cb = eduMgr->getEDUByID( ownedEDUID ) ;
            PD_CHECK( cb, SDB_SYS, error, PDERROR,
                      "Session[%s] failed to get edu by [%llu], rc: %d",
                      sessionName(), ownedEDUID, rc ) ;

            pReq = (CHAR*)SDB_THREAD_ALLOC( msgLen ) ;
            PD_CHECK( pReq, SDB_OOM, error, PDERROR, "Out of memory" ) ;
            ossMemcpy( pReq, pMsg, msgLen ) ;

            cb->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                        PMD_EDU_MEM_THREAD,
                                        pReq,
                                        SET_MONGO_MSG_FLAG( eduID() ) ) ) ;
            PD_LOG( PDDEBUG, "edu[%llu] post mongo request to edu[%llu]",
                    eduID(), ownedEDUID ) ;

            // we need to wait for response event
            _needWaitResponse = TRUE ;
         }
      }
      else
      {
         // receive message from event, it must be GETMORE command
         UINT64 sourceEDUID = UNSET_MONGO_MSG_FLAG( event._userData ) ;

         rc = _processMsg( _inBuffer.data(), pCommand ) ;

         // build response
         CHAR* pRes = NULL ;
         rc = _buildResponse( pCommand, pRes ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] failed to build response, rc: %d",
                      sessionName(), rc ) ;

         pmdEduEventRelease( event, NULL ) ;

         // post respose to source edu
         pmdEDUCB *cb = eduMgr->getEDUByID( sourceEDUID ) ;
         PD_CHECK( cb, SDB_SYS, error, PDERROR,
                   "Session[%s] failed to get edu by [%llu], rc: %d",
                   sessionName(), sourceEDUID, rc ) ;

         cb->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                     PMD_EDU_MEM_THREAD,
                                     pRes,
                                     SET_MONGO_MSG_FLAG(eduID()) ) ) ;
         PD_LOG( PDDEBUG, "edu[%llu] post mongo response to edu[%llu]",
                 eduID(), sourceEDUID ) ;
      }

      mongoReleaseCommand( &pCommand ) ;
      _contextBuff.release() ;
   }

done:
   pSite->setMsgPreprocessor( NULL ) ;
   if ( pCommand )
   {
      mongoReleaseCommand( &pCommand ) ;
   }
   disconnect() ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_recvFromSocket( CHAR *&pMsg, BOOLEAN &recvSomething )
{
   INT32 rc = SDB_OK ;
   UINT32 msgSize = 0 ;
   UINT32 headerLen = sizeof( mongoMsgHeader ) ;
   recvSomething = FALSE ;

   // receive message len
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

   recvSomething = TRUE ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_recvMsg( CHAR *&pMsg,
                               BOOLEAN &recvFromEvent,
                               pmdEDUEvent &event )
{
   INT32 rc = SDB_OK ;
   queue<pmdEDUEvent> tmpEventQue ;
   BOOLEAN recvSomething = FALSE ;

   while ( !_tmpEventQue.empty() )
   {
      _pEDUCB->postEvent( _tmpEventQue.front() ) ;
      _tmpEventQue.pop() ;
   }

  /* Wait message from socket for a short time, if receive nothing, then wait
   * for event. Loop util receive something.
   *
   * If current thread has posted a request event to other thread, we need to
   * wait for response event. No need to wait from socket, because mongo client
   * will not send any message util it receive a reply.
   */
   while( !recvSomething && !_pEDUCB->isInterrupted() )
   {
      // receive from socket first
      if ( !_needWaitResponse )
      {
         rc = _recvFromSocket( pMsg, recvSomething ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( recvSomething )
         {
            recvFromEvent = FALSE ;
            break ;
         }
      }

      // if receive nothing from socket, then wait event
      while( _pEDUCB->waitEvent( event, 0 ) )
      {
         if ( PMD_EDU_EVENT_MSG != event._eventType ||
              !IS_MONGO_MSG( event._userData ) )
         {
            // When Query preRead receive query response, it will send out
            // GETMORE request to data node immediately. At this time we may
            // get GETMORE response, and we just ignore it.
            tmpEventQue.push( event ) ;
            PD_LOG( PDDEBUG, "wait unexpected event" ) ;
            continue ;
         }
         if ( IS_MONGO_REQUEST( event._Data ) )
         {
            pMsg = (CHAR*)event._Data ;
            recvFromEvent = TRUE ;
            recvSomething = TRUE ;
            PD_LOG( PDDEBUG,
                    "edu[%llu] wait mongo request from edu[%llu]",
                    eduID(), UNSET_MONGO_MSG_FLAG( event._userData ) ) ;
            break ;

         }
         else if ( IS_MONGO_RESPONSE( event._Data ) )
         {
            // it is mongo response, just send it out
            _needWaitResponse = FALSE ;
            PD_LOG( PDDEBUG, "edu[%llu] wait mongo response from edu[%llu]",
                    eduID(), UNSET_MONGO_MSG_FLAG( event._userData ) ) ;

            rc = sendData( (CHAR*)event._Data, *((INT32*)event._Data) ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Session[%s] failed to send response header, "
                         "rc: %d", sessionName(), rc ) ;

            pmdEduEventRelease( event, NULL ) ;
         }
      }

   }

   _pEDUCB->incEventCount() ;
   pmdGetKRCB()->getMonDBCB()->addReceiveNum() ;

   while ( !tmpEventQue.empty() )
   {
      _pEDUCB->postEvent( tmpEventQue.front() ) ;
      tmpEventQue.pop() ;
   }

done:
   return rc ;
error:
   goto done ;
}

BOOLEAN _mongoSession::_isOwnedCursor( const _mongoCommand *pCommand,
                                       UINT64 &ownedEDUID,
                                       BOOLEAN &needAuth )
{
   BOOLEAN isOwned = TRUE ;
   ownedEDUID = eduID() ;
   INT64 cursorID = MONGO_INVALID_CURSORID ;
   _mongoCursorMgr* cursorMgr = getMongoCursorMgr() ;

   if ( CMD_GETMORE == pCommand->type() )
   {
      cursorID = ((_mongoGetmoreCommand*)pCommand)->cursorID() ;
   }
   else if ( CMD_KILL_CURSORS == pCommand->type() )
   {
      _mongoKillCursorCommand* killCmd = (_mongoKillCursorCommand*)pCommand ;
      // 'cusror.close()' send only one cursor.
      // 'db.runCommand( { killCursors: "bar", cursors: [1,2,...] } )' may send
      // multiple cursors, but we DON'T support it yet.
      if ( killCmd->cursorList().size() > 0 )
      {
         cursorID = killCmd->cursorList().front() ;
      }
   }

   if ( cursorID != MONGO_INVALID_CURSORID )
   {
      // First look up the cursor from local list. If nothing is found, then
      // look up from cursor mgr. The local list is to reduce access cursor mgr.
      if ( _cursorList.find( cursorID ) == _cursorList.end() )
      {
         mongoCursorInfo cursorInfo ;
         BOOLEAN foundOut = cursorMgr->find( cursorID, cursorInfo ) ;
         if ( foundOut )
         {
            ownedEDUID = cursorInfo.EDUID ;
            needAuth = cursorInfo.needAuth ;
            isOwned = FALSE ;
         }
      }
   }

   return isOwned ;
}

INT32 _mongoSession::_manageCursor( const _mongoCommand *pCommand,
                                    const MsgOpReply &sdbReply )
{
   INT32 rc = SDB_OK ;
   _mongoCursorMgr* cursorMgr = getMongoCursorMgr() ;

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
            rc = cursorMgr->insert( cursorInfo ) ;
         }
         break ;
      }
      case CMD_KILL_CURSORS :
      {
         _mongoKillCursorCommand* killCmd = (_mongoKillCursorCommand*)pCommand ;
         const vector<INT64>& cursorList = killCmd->cursorList() ;

         vector<INT64>::const_iterator it ;
         for( it = cursorList.begin() ; it != cursorList.end() ; it++ )
         {
            _cursorList.erase( *it ) ;
            cursorMgr->remove( *it ) ;
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
            _mongoGetmoreCommand* moreCmd = (_mongoGetmoreCommand*)pCommand ;
            _cursorList.erase( moreCmd->cursorID() ) ;
            cursorMgr->remove( moreCmd->cursorID() ) ;
         }
         break ;
      }
      default:
         break ;
   }

   return rc ;
}

INT32 _mongoSession::_autoCreateCS( const CHAR *csName )
{
   INT32 rc            = SDB_OK ;
   MsgOpQuery *query   = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;
   bson::BSONObj obj   = BSON( FIELD_NAME_NAME << csName <<
                               FIELD_NAME_PAGE_SIZE << 65536 ) ;
   bson::BSONObj empty ;

   _tmpBuffer.zero() ;
   _tmpBuffer.reserve( sizeof( MsgOpQuery ) ) ;
   _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )_tmpBuffer.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = 0 ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   query->nameLength = ossStrlen( cmdName ) ;

   _tmpBuffer.write( cmdName, query->nameLength + 1, TRUE ) ;
   _tmpBuffer.write( obj, TRUE ) ;
   _tmpBuffer.write( empty, TRUE ) ;
   _tmpBuffer.write( empty, TRUE ) ;
   _tmpBuffer.write( empty, TRUE ) ;
   _tmpBuffer.doneLen() ;

   rc = _processMsg( (CHAR*)query ) ;

   if ( SDB_OK == rc )
   {
      PD_LOG( PDEVENT,
              "Session[%s]: Create collection space[%s] automatically",
              sessionName(), csName ) ;
   }
   else
   {
      PD_LOG( PDWARNING,
              "Session[%s]: failed to create collection space[%s] automatically"
              ", rc: %d", sessionName(), csName, rc ) ;
      if ( SDB_DMS_CS_EXIST == rc )
      {
         rc = SDB_OK ;
      }
   }

   return rc ;
}

INT32 _mongoSession::_autoCreateCL( const CHAR *clFullName )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
   bson::BSONObj obj       = BSON( FIELD_NAME_NAME << clFullName );
   bson::BSONObj empty ;

   while( TRUE )
   {
      _tmpBuffer.zero() ;
      _tmpBuffer.reserve( sizeof( MsgOpQuery ) ) ;
      _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;

      query = ( MsgOpQuery * )_tmpBuffer.data() ;
      query->header.opCode = MSG_BS_QUERY_REQ ;
      query->header.TID = 0 ;
      query->header.routeID.value = 0 ;
      query->header.requestID = 0 ;
      query->version = 0 ;
      query->w = 0 ;
      query->padding = 0 ;
      query->flags = 0 ;
      query->nameLength = ossStrlen( cmdName ) ;
      query->numToSkip = 0 ;
      query->numToReturn = -1 ;

      _tmpBuffer.write( cmdName, query->nameLength + 1, TRUE ) ;
      _tmpBuffer.write( obj, TRUE ) ;
      _tmpBuffer.write( empty, TRUE ) ;
      _tmpBuffer.write( empty, TRUE ) ;
      _tmpBuffer.write( empty, TRUE ) ;
      _tmpBuffer.doneLen() ;

      rc = _processMsg( (CHAR*)query ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         string csName ;
         csName.assign( clFullName, ossStrstr( clFullName, "." ) - clFullName ) ;
         rc = _autoCreateCS( csName.c_str() ) ;
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
              sessionName(), clFullName ) ;
   }
   else
   {
      PD_LOG( PDWARNING,
              "Session[%s]: failed to create collection[%s] automatically"
              ", rc: %d", sessionName(), clFullName, rc ) ;
      if ( SDB_DMS_EXIST == rc )
      {
         rc = SDB_OK ;
      }
   }

   return rc ;
}

INT32 _mongoSession::_processMsg( const CHAR *pMsg,
                                  const _mongoCommand *pCommand )
{
   INT32 rc = SDB_OK ;
   INT32 orgOpCode = ((MsgHeader*)pMsg)->opCode ;
   BOOLEAN hasBuildGetMore = FALSE ;
   MONGO_CMD_TYPE cmdType = pCommand->type() ;

   while ( TRUE )
   {
      rc = _processMsg( pMsg ) ;

      // auto create cs/cl
      if ( SDB_DMS_CS_NOTEXIST == _replyHeader.flags )
      {
         if ( CMD_COLLECTION_CREATE == cmdType )
         {
            if ( SDB_OK == _autoCreateCS( pCommand->csName() ) )
            {
               ((MsgHeader*)pMsg)->opCode = orgOpCode ;
               continue ;
            }
         }
      }
      else if ( SDB_DMS_NOTEXIST == _replyHeader.flags )
      {
         if ( CMD_INSERT == cmdType ||
              CMD_INDEX_CREATE == cmdType ||
              ( CMD_UPDATE == cmdType &&
              ((_mongoUpdateCommand*)pCommand)->isUpsert()) )
         {
            if ( SDB_OK == _autoCreateCL( pCommand->clFullName() ) )
            {
               ((MsgHeader*)pMsg)->opCode = orgOpCode ;
               continue ;
            }
         }
      }
      else if ( SDB_OK == _replyHeader.flags )
      {
         if ( CMD_COUNT     == cmdType || CMD_LIST_INDEX      == cmdType ||
              CMD_AGGREGATE == cmdType || CMD_LIST_COLLECTION == cmdType ||
              CMD_DISTINCT  == cmdType || CMD_LIST_DATABASE   == cmdType ||
              CMD_LIST_USER == cmdType )
         {
            if ( SDB_INVALID_CONTEXTID != _replyHeader.contextID &&
                 !hasBuildGetMore )
            {
               buildGetMoreMsg( _replyHeader.header.requestID,
                                _replyHeader.contextID, _inBuffer ) ;
               hasBuildGetMore = TRUE ;
               continue ;
            }
         }
      }

      break ;
   }

   return rc ;
}

INT32 _mongoSession::_processMsg( const CHAR *pMsg )
{
   INT32 rc  = SDB_OK ;
   BOOLEAN needReply = FALSE ;
   BOOLEAN needRollback = FALSE ;
   bson::BSONObjBuilder retBuilder ;

   rc = _onMsgBegin( (MsgHeader *) pMsg ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   _contextBuff.release() ;
   rc = getProcessor()->processMsg( (MsgHeader *) pMsg, _contextBuff,
                                    _replyHeader.contextID,
                                    needReply, needRollback, retBuilder ) ;
   _errorInfo = engine::utilGetErrorBson( rc,
                              _pEDUCB->getInfo( engine::EDU_INFO_ERROR ) ) ;
   if ( rc && needRollback )
   {
      PD_LOG( PDDEBUG,
              "Session rolling back operation[opCode: %d], rc: %d",
              ((MsgHeader*)pMsg)->opCode, rc ) ;

      INT32 rcTmp = getProcessor()->doRollback() ;
      PD_RC_CHECK( rcTmp, PDERROR,
                   "Session failed to rollback trans info, rc: %d",
                   rcTmp ) ;
   }

   _replyHeader.numReturned = _contextBuff.recordNum() ;
   _replyHeader.startFrom = (INT32)_contextBuff.getStartFrom() ;
   _replyHeader.flags = rc ;

   if ( rc )
   {
      bson::BSONObjBuilder bob ;
      bob.append( "ok", 0 ) ;
      bob.append( "code",  _errorInfo.getIntField( OP_ERRNOFIELD ) ) ;
      bob.append( "errmsg", _errorInfo.getStringField( OP_ERRDESP_FIELD ) ) ;
      _contextBuff = engine::rtnContextBuf( bob.obj() ) ;
      _replyHeader.numReturned = 1 ;
   }

   _onMsgEnd( rc, (MsgHeader *) pMsg ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_onMsgBegin( MsgHeader *msg )
{
   // set reply header ( except flags, length )
   _replyHeader.contextID          = -1 ;
   _replyHeader.numReturned        = 0 ;
   _replyHeader.startFrom          = 0 ;
   _replyHeader.header.opCode      = MAKE_REPLY_TYPE(msg->opCode) ;
   _replyHeader.header.requestID   = msg->requestID ;
   _replyHeader.header.TID         = msg->TID ;
   _replyHeader.header.routeID     = engine::pmdGetNodeID() ;

   // start operator
   MON_START_OP( _pEDUCB->getMonAppCB() ) ;

   return SDB_OK ;
}

INT32 _mongoSession::_onMsgEnd( INT32 result, MsgHeader *msg )
{
   // release buff context
   //_contextBuff.release() ;

   if ( result && SDB_DMS_EOC != result )
   {
      PD_LOG( PDWARNING, "Session[%s] process msg[opCode=%d, len: %d, "
              "TID: %u, requestID: %llu] failed, rc: %d",
              sessionName(), msg->opCode, msg->messageLength, msg->TID,
              msg->requestID, result ) ;
   }

   // end operator
   MON_END_OP( _pEDUCB->getMonAppCB() ) ;

   return SDB_OK ;
}

INT32 _mongoSession::_buildResponse( _mongoCommand *pCommand,
                                     CHAR *&pRes )
{
   INT32 rc = SDB_OK ;
   _mongoResponseBuffer headerBuf ;
   INT32 resSize = 0 ;

   rc = mongoPostRunCommand( pCommand, _replyHeader, _contextBuff, headerBuf ) ;
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
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_reply( _mongoCommand *pCommand )
{
   INT32 rc = SDB_OK ;
   _mongoResponseBuffer headerBuf ;

   rc = mongoPostRunCommand( pCommand, _replyHeader, _contextBuff, headerBuf ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Session[%s] failed to build response, rc: %d",
                sessionName(), rc ) ;

   // send response
   if ( headerBuf.usedSize > 0 )
   {
      INT32 rcTmp = SDB_OK ;
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
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_reply( _mongoCommand *pCommand, const CHAR* pMsg,
                             INT32 errCode, const BSONObj &errObj )
{
   INT32 rc = SDB_OK ;
   _mongoResponseBuffer headerBuf ;

   SDB_ASSERT( errCode != SDB_OK, "Invalid error code" ) ;

   if ( errObj.isEmpty() )
   {
      bson::BSONObjBuilder bob ;
      bob.append( "ok", 0 ) ;
      bob.append( "code",  errCode ) ;
      bob.append( "errmsg", getErrDesp( errCode ) ) ;
      _contextBuff = engine::rtnContextBuf( bob.obj() ) ;
      _replyHeader.numReturned = 1 ;
      _replyHeader.flags = errCode ;
   }
   else
   {
      _contextBuff = engine::rtnContextBuf( errObj ) ;
      _replyHeader.numReturned = 1 ;
      _replyHeader.flags = errCode ;
   }

   if ( pCommand && pCommand->isInitialized() )
   {
      rc = mongoPostRunCommand( pCommand, _replyHeader,
                                _contextBuff, headerBuf ) ;
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
         msgBuffer tmpBuffer ;
         tmpBuffer.write( org.objdata(), org.objsize() ) ;
         tmpBuffer.write( empty.objdata(), empty.objsize() ) ;
         _contextBuff = rtnContextBuf( tmpBuffer.data(), tmpBuffer.size(), 2 ) ;

         mongoCommandResponse res ;
         res.header.msgLen = sizeof( mongoCommandResponse ) + _contextBuff.size() ;
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

   // send response
   if ( headerBuf.usedSize > 0 )
   {
      INT32 rcTmp = SDB_OK ;
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
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_setSeesionAttr()
{
   INT32 rc = SDB_OK ;
   engine::pmdEDUMgr *pmdEDUMgr = _pEDUCB->getEDUMgr() ;
   const CHAR *cmd = CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR ;
   MsgOpQuery *set = NULL ;

   bson::BSONObj obj ;
   bson::BSONObj emptyObj ;

   msgBuffer msgSetAttr ;
   if ( _masterRead )
   {
      goto done ;
   }

   msgSetAttr.reserve( sizeof( MsgOpQuery ) ) ;
   msgSetAttr.advance( sizeof( MsgOpQuery ) - 4 ) ;
   obj = BSON( FIELD_NAME_PREFERED_INSTANCE << PREFER_REPL_MASTER ) ;
   set = (MsgOpQuery *)msgSetAttr.data() ;

   set->header.opCode = MSG_BS_QUERY_REQ ;
   set->header.TID = 0 ;
   set->header.routeID.value = 0 ;
   set->header.requestID = 0 ;
   set->version = 0 ;
   set->w = 0 ;
   set->padding = 0 ;
   set->flags = 0 ;
   set->nameLength = ossStrlen(cmd) ;
   set->numToSkip = 0 ;
   set->numToReturn = -1 ;

   msgSetAttr.write( cmd, set->nameLength + 1, TRUE ) ;
   msgSetAttr.write( obj, TRUE ) ;
   msgSetAttr.write( emptyObj, TRUE ) ;
   msgSetAttr.write( emptyObj, TRUE ) ;
   msgSetAttr.write( emptyObj, TRUE ) ;
   msgSetAttr.doneLen() ;

   // activate edu
   if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
   {
      PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
              sessionName(), rc ) ;
      goto error ;
   }

   rc = _processMsg( msgSetAttr.data() ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   _masterRead = TRUE ;

   // wait edu
   if ( SDB_OK != ( rc = pmdEDUMgr->waitEDU( _pEDUCB ) ) )
   {
      PD_LOG( PDERROR, "Session[%s] wait edu failed, rc: %d",
              sessionName(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

}
