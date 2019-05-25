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

   Source File Name = omSdbConnector.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/28/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omSdbConnector.hpp"
#include "omDef.hpp"
#include "msgMessage.hpp"

using namespace bson ;
#define SDB_OM_CONNECTOR_SOCKET_TIMEOUT OM_MSG_TIMEOUT_TWO_HOUR


namespace engine
{
   _omSdbConnector::_omSdbConnector()
                   :_hostName(""), _port(0), _pSocket(NULL), _init(FALSE)
   {
   }

   _omSdbConnector::~_omSdbConnector()
   {
      if ( NULL != _pSocket )
      {
         close() ;
      }
   }

   INT32 _omSdbConnector::_connect( const CHAR * host, UINT32 port )
   {
      INT32 rc = SDB_OK ;
      _pSocket = SDB_OSS_NEW _ossSocket ( host, port ) ;
      if ( NULL == _pSocket )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "out of memory:rc=%d", rc ) ;
         goto error ;
      }

      rc = _pSocket->initSocket() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "initial socket failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _pSocket->connect() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "connect to remote failed:hostName=%s,port=%u,rc=%d",
                 host, port, rc ) ;
         goto error ;
      }

      rc = _pSocket->disableNagle() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "disable nagle failed:rc=%d", rc ) ;
         rc = SDB_OK ;
      }

      rc = _pSocket->setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                                   OSS_SOCKET_KEEP_INTERVAL,
                                   OSS_SOCKET_KEEP_CONTER ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Set socket keep alive failed, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbConnector::_sendRequest( const CHAR *request, INT32 reqSize )
   {
      INT32 rc     = SDB_OK ;
      INT32 offset = 0 ;
      while ( reqSize > offset )
      {
         INT32 sendSize = 0 ;
         rc = _pSocket->send( &request[offset], reqSize - offset, sendSize,
                              SDB_OM_CONNECTOR_SOCKET_TIMEOUT ) ;
         offset += sendSize ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "send request failed:rc=%d", rc ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbConnector::_recvResponse( CHAR *response, INT32 resSize )
   {
      INT32 rc     = SDB_OK ;
      INT32 offset = 0 ;
      while ( resSize > offset )
      {
         INT32 receivedSize = 0 ;
         rc = _pSocket->recv( &response[offset], resSize - offset,
                              receivedSize, SDB_OM_CONNECTOR_SOCKET_TIMEOUT ) ;
         offset += receivedSize ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "receive response failed:rc=%d", rc ) ;
            goto error ;
         }
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbConnector::_requestSysInfo()
   {
      INT32 rc = SDB_OK ;
      MsgSysInfoRequest request ;
      MsgSysInfoReply reply ;

      request.header.specialSysInfoLen = MSG_SYSTEM_INFO_LEN ;
      request.header.eyeCatcher        = MSG_SYSTEM_INFO_EYECATCHER ;
      request.header.realMessageLength = sizeof(MsgSysInfoRequest) ;

      rc = _sendRequest( (CHAR *)&request, (INT32)sizeof(request) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "send request failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _recvResponse( (CHAR *)&reply, (INT32)sizeof( MsgSysInfoReply ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "recv response failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( reply.header.eyeCatcher != MSG_SYSTEM_INFO_EYECATCHER &&
           reply.header.eyeCatcher != MSG_SYSTEM_INFO_EYECATCHER_REVERT )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "response is unreconigzed:eyeCatcher=%u",
                 reply.header.eyeCatcher ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbConnector::_authority( const string &user, const string &passwd )
   {
      BSONObj auth ;
      INT32 authSize ;
      INT32 msgLen ;
      CHAR *buff = NULL ;
      INT32 rc   = SDB_OK ;
      MsgAuthentication *reqMsg = NULL ;
      MsgHeader *recvMsg = NULL ;

      auth     = BSON( SDB_AUTH_USER << user << SDB_AUTH_PASSWD << passwd ) ;
      authSize = auth.objsize() ;
      
      msgLen = sizeof( MsgAuthentication ) +
               ossRoundUpToMultipleX( authSize, 4 ) ;
      buff = (CHAR *)SDB_OSS_MALLOC( msgLen ) ;
      if ( NULL == buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "out of memory!" ) ;
         goto error ;
      }

      reqMsg                       = ( MsgAuthentication * )(buff) ;
      reqMsg->header.requestID     = 0 ;
      reqMsg->header.opCode        = MSG_AUTH_VERIFY_REQ ;
      reqMsg->header.messageLength = sizeof( MsgAuthentication ) + authSize ;
      reqMsg->header.routeID.value = 0 ;
      reqMsg->header.TID           = ossGetCurrentThreadID() ;
      ossMemcpy( buff + sizeof( MsgAuthentication ), auth.objdata(), 
                 authSize ) ;

      rc = sendMessage( (MsgHeader *)reqMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "send authority request failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = recvMessage( &recvMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "recv authority response failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = ( ( MsgOpReply * )recvMsg )->flags ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "remote's reply failed:flags=%d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != buff )
      {
         SDB_OSS_FREE( buff ) ;
      }
      if ( NULL != recvMsg )
      {
         SDB_OSS_FREE( recvMsg ) ;
      }
      return rc ;
   error:
      
      goto done ;
   }

   INT32 _omSdbConnector::_negotiation( const string &user, 
                                        const string &passwd )
   {
      INT32 rc = SDB_OK ;
      rc = _requestSysInfo() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "send request sysinfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _authority( user, passwd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "authority failed:user=%s,rc=%d", 
                 user.c_str(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _omSdbConnector::_setAttr( INT32 preferedInstance )
   {
      INT32 rc = SDB_OK ;
      MsgHeader *reqMsg     = NULL ;
      MsgHeader *recvMsg    = NULL ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR ;

      if ( 0 > preferedInstance )
      {
         goto done ;
      }

      match = BSON( FIELD_NAME_PREFERED_INSTANCE << preferedInstance ) ;
      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match, 
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "build command failed:command=%s, rc=%d", 
                 pCommand, rc ) ;
         goto error ;
      }

      reqMsg = ( MsgHeader * )pBuff ;
      rc = sendMessage( reqMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "send attribute request failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = recvMessage( &recvMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "recv authority response failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = ( ( MsgOpReply * )recvMsg )->flags ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDWARNING, "remote's reply failed:flags=%d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      if ( NULL != recvMsg )
      {
         SDB_OSS_FREE( recvMsg ) ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _omSdbConnector::init( const string &hostName, UINT32 port, 
                                const string &user, const string &passwd,
                                INT32 preferedInstance )
   {
      INT32 rc = SDB_OK ;
      if ( _init )
      {
         goto done ;
      }

      rc = _connect( hostName.c_str(), port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "connect failed:host=%s,port=%u,rc=%d", 
                 hostName.c_str(), port, rc ) ;
         goto error ;
      }

      rc = _negotiation( user, passwd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "negotiation failed:host=%s,port=%u,user=%s,rc=%d",
                 hostName.c_str(), port, user.c_str(), rc ) ;
         goto error ;
      }

      rc = _setAttr( preferedInstance ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "set attribute failed:host=%s,port=%u,user=%s,"
                 "preferedInstance=%d,rc=%d", hostName.c_str(), port, 
                 user.c_str(), preferedInstance, rc ) ;
         rc = SDB_OK ;
      }

      _init = TRUE ;

   done:
      return rc ;
   error:
      close() ;
      goto done ;
   }

   INT32 _omSdbConnector::sendMessage( const MsgHeader *msg )
   {
      return _sendRequest( ( CHAR * )msg, msg->messageLength ) ;
   }

   INT32 _omSdbConnector::recvMessage( MsgHeader **msg )
   {
      INT32 rc     = SDB_OK ;
      CHAR *rspMsg = NULL ;
      MsgHeader rspHeader ;
      INT32 msgHeaderSize = 0 ;
      SDB_ASSERT( msg != NULL, "msg must not be null!" ) ;

      msgHeaderSize = sizeof( rspHeader ) ;
      rc = _recvResponse( (CHAR *)&rspHeader, msgHeaderSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "recveive response header failed:rc=%d", rc ) ;
         goto error ;
      }

      rspMsg = (CHAR *)SDB_OSS_MALLOC( rspHeader.messageLength ) ;
      if ( NULL == rspMsg )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "out of memory" ) ;
         goto error ;
      }

      ossMemcpy( rspMsg, (CHAR *)&rspHeader, msgHeaderSize ) ;

      if ( rspHeader.messageLength > msgHeaderSize )
      {
         rc = _recvResponse( rspMsg + msgHeaderSize, 
                             rspHeader.messageLength - msgHeaderSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "receive response body failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      *msg = ( MsgHeader * )rspMsg ;

   done:
      return rc ;
   error:
      if ( NULL != rspMsg )
      {
         SDB_OSS_FREE( rspMsg ) ;
         rspMsg = NULL ;
      }
      goto done ;
   }

   INT32 _omSdbConnector::close()
   {
      if ( NULL != _pSocket )
      {
         _pSocket->close() ;
         SDB_OSS_DEL( _pSocket ) ;
         _pSocket = NULL ;
      }

      _init = FALSE ;
      return SDB_OK ;
   }
}




