/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordRemoteConnection.cpp

   Descriptive Name = Coordinator remote connection

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordRemoteConnection.hpp"
#include "pmdEDU.hpp"
#include "msgAuth.hpp"
#include "msgMessage.hpp"
#include "coordTrace.hpp"
#include "msgConvertorImpl.hpp"

namespace engine
{
   _coordRemoteConnection::_coordRemoteConnection()
   : _socket( NULL ),
     _newSocket( FALSE ),
     _msgConvertor( NULL )
   {
   }

   _coordRemoteConnection::~_coordRemoteConnection()
   {
      // Only destroy the socket when it's created by myself.
      if ( _socket && _newSocket )
      {
         if ( !_socket->isClosed() )
         {
            _socket->close() ;
         }
         SDB_OSS_DEL _socket ;
      }

      SAFE_OSS_DELETE( _msgConvertor ) ;
   }

   INT32 _coordRemoteConnection::init( ossSocket *socket )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( socket, "socket is null" ) ;
      if ( !socket->isConnected() )
      {
         rc = SDB_NOT_CONNECTED ;
         PD_LOG( PDERROR, "The spcified socket for the remote connection is "
                 "not connected[%d]", rc ) ;
         goto error ;
      }
      _socket = socket ;
      _newSocket = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION_INIT, "_coordRemoteConnection::init" )
   INT32 _coordRemoteConnection::init( const CHAR *host, const CHAR *service,
                                       INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION_INIT ) ;
      UINT16 port = 0 ;

      SDB_ASSERT( host, "host is null" ) ;
      SDB_ASSERT( service, "service is null" ) ;

      rc = ossGetPort( service, port ) ;
      PD_RC_CHECK( rc, PDERROR, "Service[%s] is invalid[%d]", service, rc ) ;

      _socket = SDB_OSS_NEW ossSocket( host, port, timeout ) ;
      if ( !_socket )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for socket failed[%d]", rc ) ;
         goto error ;
      }

      rc = _socket->initSocket() ;
      PD_RC_CHECK( rc, PDERROR, "Initialize socket failed[%d]", rc ) ;
      _socket->closeWhenDestruct( TRUE ) ;
      _socket->disableNagle() ;
      _socket->setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE, OSS_SOCKET_KEEP_INTERVAL,
                             OSS_SOCKET_KEEP_CONTER ) ;

      rc = _socket->connect( timeout ) ;
      PD_RC_CHECK( rc, PDERROR, "Connect to %s:%u failed[%d]",
                   host, port, rc ) ;
      _newSocket = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION_INIT, rc ) ;
      return rc ;
   error:
      SAFE_OSS_DELETE( _socket ) ;
      goto done ;
   }

   BOOLEAN _coordRemoteConnection::isConnected() const
   {
      return _socket && _socket->isConnected() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION_AUTHENTICATE, "_coordRemoteConnection::authenticate" )
   INT32 _coordRemoteConnection::authenticate( const CHAR *user,
                                               const CHAR *password,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION_AUTHENTICATE ) ;

      if ( !_socket )
      {
         rc = SDB_NOT_CONNECTED ;
         PD_LOG( PDERROR, "The connection has not been initialized yet[%d]",
                 rc ) ;
         goto error ;
      }

      // SYSINFO message is necessary. If we don't send this message, the data
      // source coordinator will treate the first message as the handle shaking
      // message for SSL connection. Refer to _pmdLocalSession::run().
      rc = _doSysInfo( cb ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _doAuth( user, password, cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION_AUTHENTICATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION_SYNCSEND, "_coordRemoteConnection::syncSend" )
   INT32 _coordRemoteConnection::syncSend( MsgHeader *header,
                                           pmdEDUEvent &recvEvent,
                                           pmdEDUCB *cb, INT32 timeout,
                                           INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION_SYNCSEND ) ;

      if ( !_socket->isConnected() )
      {
         rc = SDB_NOT_CONNECTED ;
         PD_LOG( PDERROR, "The socket is not connected" ) ;
         goto error ;
      }

      rc = pmdSyncSendMsg( header, recvEvent, _socket, cb,
                           timeout, forceTimeout ) ;
      PD_RC_CHECK( rc, PDERROR, "Sync send message[opCode: %d] failed[%d]",
                   header->opCode, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION_SYNCSEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION_DISCONNECT, "_coordRemoteConnection::disconnect" )
   INT32 _coordRemoteConnection::disconnect( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION_DISCONNECT  ) ;
      CHAR *msg = NULL ;
      CHAR *finalMsg = NULL ;
      UINT32 finalLen = 0 ;
      INT32 buffLen = 0 ;

      if ( !_socket || _socket->isClosed() )
      {
         goto done ;
      }

      rc = msgBuildDisconnectMsg( &msg, &buffLen, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build disconnect message failed[%d]", rc ) ;

      rc = _onSendMsg( (MsgHeader *)msg, finalMsg, finalLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare message for sending failed[%d]", rc ) ;

      rc = pmdSend( finalMsg, finalLen, _socket, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Send disconnect message to data source "
                                "failed[%d]", rc ) ;
   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION_DISCONNECT, rc ) ;
      return rc ;
   error:
      if ( _socket )
      {
         _socket->close() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION__DOSYSINFO, "_coordRemoteConnection::_doSysInfo" )
   INT32 _coordRemoteConnection::_doSysInfo( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION__DOSYSINFO ) ;

      MsgSysInfoRequest sysInfoReq ;
      sysInfoReq.header.specialSysInfoLen = MSG_SYSTEM_INFO_LEN ;
      sysInfoReq.header.eyeCatcher = MSG_SYSTEM_INFO_EYECATCHER ;
      sysInfoReq.header.realMessageLength = sizeof(MsgSysInfoRequest) ;

      MsgSysInfoReply sysInfoRes ;

      rc = pmdSend( (const CHAR *)&sysInfoReq, sizeof(MsgSysInfoRequest),
                    _socket, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Send SYSINFO message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = pmdRecv( (CHAR*)&sysInfoRes, sizeof(MsgSysInfoReply),
                    _socket, cb, OSS_SOCKET_DFT_TIMEOUT,
                    COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Recv SYSINFO reply message failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      // Check message protocol compatibility.
      rc = _checkProtocolCompatibility( sysInfoRes ) ;
      PD_RC_CHECK( rc, PDERROR, "Check protocol compatibility failed[%d]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION__DOSYSINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION__CHECKPROTOCOLCOMPATIBILITY, "_coordRemoteConnection::_checkProtocolCompatibility" )
   INT32 _coordRemoteConnection::_checkProtocolCompatibility(
                                          const MsgSysInfoReply &sysInfoReply )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION__CHECKPROTOCOLCOMPATIBILITY ) ;
      BOOLEAN endianConvert = FALSE ;
      SDB_PROTOCOL_VERSION peerVersion = SDB_PROTOCOL_VER_INVALID ;

      rc = msgExtractSysInfoReply( (const CHAR *)&sysInfoReply, endianConvert,
                                   NULL, &peerVersion ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract system info reply failed[%d]", rc ) ;

      if ( SDB_PROTOCOL_VER_1 == peerVersion )
      {
         _msgConvertor = SDB_OSS_NEW msgConvertorImpl ;
         if ( !_msgConvertor )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for message convertor[size: %u] "
                    "failed[%d]", sizeof(msgConvertorImpl), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION__CHECKPROTOCOLCOMPATIBILITY,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION__DOAUTH, "_coordRemoteConnection::_doAuth" )
   INT32 _coordRemoteConnection::_doAuth( const CHAR *pUser,
                                          const CHAR *pEncryptPass,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION__DOAUTH ) ;
      CHAR *pAuthMsgBuf = NULL ;
      INT32 authMsgSize = 0 ;
      pmdEDUEvent recvEvent ;
      MsgAuthReply *pReply = NULL ;

      rc = msgBuildAuthMsg( &pAuthMsgBuf, &authMsgSize,
                            pUser, pEncryptPass, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build auth message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = syncSend( (MsgHeader *)pAuthMsgBuf, recvEvent, cb,
                     OSS_SOCKET_DFT_TIMEOUT,
                     COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Do auth on remote node failed[%d]", rc ) ;

      pReply = ( MsgAuthReply* )recvEvent._Data ;
      if ( SDB_OK != pReply->flags )
      {
         rc = pReply->flags ;
         PD_LOG( PDERROR, "Authenticate failed, rc: %d", rc ) ;
         goto error ;
      }

  done:
      if ( pAuthMsgBuf )
      {
         msgReleaseBuffer( pAuthMsgBuf, cb ) ;
      }
      pmdEduEventRelease( recvEvent, cb ) ;
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION__DOAUTH, rc ) ;
      return rc ;
  error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDREMOTECONNECTION__ONSENDMSG, "_coordRemoteConnection::_onSendMsg" )
   INT32 _coordRemoteConnection::_onSendMsg( MsgHeader *origMsg,
                                             CHAR *&finalMsg, UINT32 &finalLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDREMOTECONNECTION__ONSENDMSG ) ;

      if ( _msgConvertor )
      {
         _msgConvertor->reset( FALSE ) ;
         rc = _msgConvertor->push( (const CHAR *)origMsg,
                                   origMsg->messageLength ) ;
         PD_RC_CHECK( rc, PDERROR, "Push message into message convertor "
                      "failed[%d]", rc ) ;
         rc = _msgConvertor->output( finalMsg, finalLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Get message from message convertor "
                      "failed[%d]", rc ) ;
         SDB_ASSERT( finalLen == *(UINT32 *)finalMsg,
                     "Converted message length is not as expected" ) ;
      }
      else
      {
         finalMsg = (CHAR *)origMsg ;
         finalLen = origMsg->messageLength ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDREMOTECONNECTION__ONSENDMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
