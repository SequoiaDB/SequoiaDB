/******************************************************************************

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

   Source File Name = netEventHandler.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "netEventHandler.hpp"
#include "netFrame.hpp"
#include "ossMem.hpp"
#include "pmdEnv.hpp"
#include "msgDef.h"
#include "msgMessage.hpp"

#include "pdTrace.hpp"
#include "netTrace.hpp"
#include "msgMessageFormat.hpp"
#include "netEventSuit.hpp"
#include <boost/bind.hpp>
#if defined (_WINDOWS)
#include <mstcpip.h>
#endif

using namespace boost::asio::ip ;
using namespace std ;

namespace engine
{
   #define NET_SOCKET_SNDTIMEO         ( 2 )

   /*
      _netEventHandler implement
    */
   _netEventHandler::_netEventHandler( netEvSuitPtr evSuitPtr,
                                       const NET_HANDLE &handle )
   : netEventHandlerBase( handle ),
     _evSuitPtr( evSuitPtr ),
     _sock( evSuitPtr->getIOService() ),
     _headerSz( sizeof(MsgHeader ) ),
     _buf( NULL ),
     _bufLen( 0 ),
     _state(NET_EVENT_HANDLER_STATE_HEADER)
   {
      _hasRecvMsg    = FALSE ;
   }

   _netEventHandler::~_netEventHandler()
   {
      boost::system::error_code ec ;
      close() ;
      _sock.close( ec ) ;
      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
      }

      /// detach
      _evSuitPtr->delHandle( _handle ) ;
   }

   NET_EH _netEventHandler::createShared( netEvSuitPtr evSuitPtr,
                                          const NET_HANDLE &handle )
   {
      NET_EH eh ;
      INT32 rc = SDB_OK ;

      NET_TCP_EH tmpEH = NET_TCP_EH::allocRaw( ALLOC_POOL ) ;
      if ( NULL != tmpEH.get() &&
           NULL != new( tmpEH.get() ) netEventHandler( evSuitPtr, handle ) )
      {
         // attach handle to evSuit if netEventHandler is created
         rc = evSuitPtr->addHandle( handle ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Add handle[%d] to evSuit failed, rc: %d",
                    handle, rc ) ;
         }
         else
         {
            eh = NET_EH::makeRaw( tmpEH.get(), ALLOC_POOL ) ;
         }
      }

      return eh ;
   }

   string _netEventHandler::localAddr() const
   {
      string addr ;
      try
      {
         addr = _sock.local_endpoint().address().to_string() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "get local address occurred exception: %s",
                 e.what() ) ;
      }
      return addr ;
   }

   string _netEventHandler::remoteAddr() const
   {
      string addr ;
      try
      {
         addr = _sock.remote_endpoint().address().to_string() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "get remote address occurred exception: %s",
                 e.what() ) ;
      }
      return addr ;
   }

   UINT16 _netEventHandler::localPort () const
   {
      UINT16 port = 0 ;
      try
      {
         port = _sock.local_endpoint().port() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "get local port occurred exception: %s", e.what() ) ;
      }
      return port ;
   }

   UINT16 _netEventHandler::remotePort () const
   {
      UINT16 port = 0 ;
      try
      {
         port = _sock.remote_endpoint().port() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "get remote port occurred exception: %s", e.what() ) ;
      }
      return port ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND_SETOPT, "_netEventHandler::setOpt" )
   void _netEventHandler::setOpt()
   {
      INT32 res = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETEVNHND_SETOPT );

      _isConnected = TRUE ;
      _isNew = FALSE ;

      INT32 keepAlive = 1 ;
      INT32 keepIdle = OSS_SOCKET_KEEP_IDLE ;
      INT32 keepInterval = OSS_SOCKET_KEEP_INTERVAL ;
      INT32 keepCount = OSS_SOCKET_KEEP_CONTER ;

      try
      {
         _sock.set_option( tcp::no_delay(TRUE) ) ;

#if defined (_LINUX)
         struct timeval sendtimeout ;
         sendtimeout.tv_sec = NET_SOCKET_SNDTIMEO ;
         sendtimeout.tv_usec = 0 ;
         SOCKET nativeSock = _sock.native() ;

         res = setsockopt( nativeSock, SOL_SOCKET, SO_KEEPALIVE,
                     ( void *)&keepAlive, sizeof(keepAlive) ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set keepalive,"
                    "err:%d", _handle, res ) ;
         }
         res = setsockopt( nativeSock, SOL_TCP, TCP_KEEPIDLE,
                     ( void *)&keepIdle, sizeof(keepIdle) ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set keepidle,"
                    "err:%d", _handle, res ) ;
         }
         res = setsockopt( nativeSock, SOL_TCP, TCP_KEEPINTVL,
                     ( void *)&keepInterval, sizeof(keepInterval) ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set keepintvl,"
                    "err:%d", _handle, res ) ;
         }
         res = setsockopt( nativeSock, SOL_TCP, TCP_KEEPCNT,
                     ( void *)&keepCount, sizeof(keepCount) ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set keepcnt,"
                    "err:%d", _handle, res ) ;
         }
         res = setsockopt( nativeSock, SOL_SOCKET, SO_SNDTIMEO,
                           ( CHAR * )&sendtimeout, sizeof(struct timeval) ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set sndtimeout,"
                    "err:%d", _handle, res ) ;
         }
#else
         struct tcp_keepalive alive_in ;
         DWORD ulBytesReturn       = 0 ;
         SOCKET nativeSock          = _sock.native() ;
         alive_in.onoff             = keepAlive ;
         alive_in.keepalivetime     = keepIdle * 1000 ; // ms
         alive_in.keepaliveinterval = keepInterval * 1000 ; // ms
         res = setsockopt( nativeSock, SOL_SOCKET, SO_KEEPALIVE,
                           ( CHAR *)&keepAlive, sizeof(keepAlive) ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set keepalive,"
                    "err:%d", _handle, res ) ;
         }
         res = WSAIoctl( nativeSock, SIO_KEEPALIVE_VALS,
                         &alive_in, sizeof(alive_in),
                         NULL, 0, &ulBytesReturn, NULL, NULL ) ;
         if ( SDB_OK != res )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] failed to set keepalive "
                    "vals, err:%d", _handle, res ) ;
         }
#endif
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Connection[Handle:%d] failed to set no delay:%s",
                 _handle, e.what() ) ;
      }
      PD_TRACE_EXIT ( SDB__NETEVNHND_SETOPT );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND_SYNCCONN, "_netEventHandler::syncConnect" )
   INT32 _netEventHandler::syncConnect( const CHAR *hostName,
                                        const CHAR *serviceName )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETEVNHND_SYNCCONN );

      if ( _isConnected )
      {
         close() ;
         boost::system::error_code ec ;
         _sock.close( ec ) ;
      }

      UINT16 port = 0 ;
      rc = ossGetPort( serviceName, port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Connection[Handle:%d] convert svc[%s] to port "
                 "failed, rc: %d", _handle, serviceName, rc ) ;
         goto error ;
      }

      {
         _ossSocket sock( hostName, port ) ;
         rc = sock.initSocket() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] init socket failed, "
                    "rc: %d", _handle, rc ) ;
            goto error ;
         }
         sock.closeWhenDestruct( FALSE ) ;
         rc = sock.connect() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] connect to %s:%d(%s) "
                    "failed, rc: %d", _handle, hostName, port,
                    serviceName, rc ) ;
            goto error ;
         }

         try
         {
            _sock.assign( tcp::v4(), sock.native() ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] occur unexpected: %s",
                    _handle, e.what() ) ;
            rc = SDB_SYS ;
            sock.close() ;
            _sock.close() ;
            goto error ;
         }

         setOpt() ;

         rc = _syncCheckSysInfo( sock ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Connection[Handle%d] checking system info of "
                    "node[%s:%s] failed[%d]",
                    _handle, hostName, serviceName, rc ) ;
            boost::system::error_code ec ;
            close() ;
            _sock.close( ec ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETEVNHND_SYNCCONN, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND__SYNCCHECKSYSINFO, "_netEventHandler::_syncCheckSysInfo" )
   INT32 _netEventHandler::_syncCheckSysInfo( ossSocket &socket )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__NETEVNHND__SYNCCHECKSYSINFO ) ;
      INT32 sentLen = 0 ;
      INT32 recvLen = 0 ;
      BOOLEAN endianConvert = FALSE ;
      MsgSysInfoRequest sysInfoReq ;
      MsgSysInfoReply sysInfoReply ;

      sysInfoReq.header.specialSysInfoLen = MSG_SYSTEM_INFO_LEN ;
      sysInfoReq.header.eyeCatcher = MSG_SYSTEM_INFO_EYECATCHER ;
      sysInfoReq.header.realMessageLength = sizeof(MsgSysInfoRequest) ;

      rc = socket.send( (const CHAR *)&sysInfoReq,
                        sizeof(MsgSysInfoRequest), sentLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Send system info request failed[%d]", rc ) ;

      rc = socket.recv( (CHAR *)&sysInfoReply, sizeof(MsgSysInfoReply),
                        recvLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Receive system info reply failed[%d]", rc ) ;
      SDB_ASSERT( sizeof(MsgSysInfoReply) == recvLen,
                  "Received length is not as expected" ) ;

      rc = msgExtractSysInfoReply( (const CHAR *)&sysInfoReply, endianConvert,
                                   NULL, &_peerVersion ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract system info reply failed[%d]", rc ) ;

      if ( SDB_PROTOCOL_VER_1 == _peerVersion )
      {
         _headerSz = sizeof(MsgHeaderV1) ;
         rc = _enableMsgConvertor() ;
         PD_RC_CHECK( rc, PDERROR, "Enabled message convertor for remote node"
                      "[%s:%d] failed[%d]", remoteAddr().c_str(), remotePort(),
                      rc ) ;
         PD_LOG( PDDEBUG, "Message convertor enabled for remote node[%s:%d]",
                 remoteAddr().c_str(), remotePort() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETEVNHND__SYNCCHECKSYSINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND_ASYNCRD, "_netEventHandler::asyncRead" )
   INT32 _netEventHandler::asyncRead()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETEVNHND_ASYNCRD ) ;

      if ( NET_EVENT_HANDLER_STATE_HEADER == _state ||
           NET_EVENT_HANDLER_STATE_HEADER_LAST == _state )
      {
         if ( !_isConnected )
         {
            PD_LOG( PDWARNING, "Connection[Handle:%d, Node:%s] is "
                    "already closed", _handle, routeID2String( _id ).c_str() ) ;
            goto error ;
         }

         try
         {
            if ( NET_EVENT_HANDLER_STATE_HEADER_LAST == _state )
            {
               async_read( _sock, buffer(
                           (CHAR*)&_header + sizeof(MsgSysInfoRequest),
                           _headerSz - sizeof(MsgSysInfoRequest) ),
                           boost::bind(&_netEventHandler::_readCallback,
                                       _getShared(),
                                       boost::asio::placeholders::error ) ) ;
            }
            // for MsgSysInfoRequest msg(12bytes)
            else if ( FALSE == _hasRecvMsg )
            {
               async_read( _sock, buffer(&_header, sizeof(MsgSysInfoRequest)),
                           boost::bind(&_netEventHandler::_readCallback,
                                       _getShared(),
                                       boost::asio::placeholders::error )) ;
            }
            else
            {
               async_read( _sock, buffer(&_header, _headerSz),
                           boost::bind(&_netEventHandler::_readCallback,
                                       _getShared(),
                                       boost::asio::placeholders::error )) ;
            }
         }
         catch( std::exception &e)
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Failed to async read, occur exception: %s,"
                    " rc: %d", e.what(), rc ) ;
            goto error ;
         }
      }
      else
      {
         UINT32 len = _header.messageLength ;
         if ( SDB_OK != _allocateBuf( len ) )
         {
            goto error ;
         }

         ossMemcpy( _buf, &_header, _headerSz ) ;

         if ( !_isConnected )
         {
            PD_LOG( PDWARNING, "Connection[Handle:%d, Node:%s] is already "
                    "closed", _handle, routeID2String( _id ).c_str() ) ;
            goto error ;
         }
         try
         {
            async_read( _sock, buffer(
                        (CHAR *)((ossValuePtr)_buf + _headerSz ),
                        len - _headerSz ),
                        boost::bind( &_netEventHandler::_readCallback,
                                     _getShared(),
                                     boost::asio::placeholders::error ) ) ;
         }
         catch( std::exception &e)
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Failed to async read, occur exception: %s,"
                    " rc: %d", e.what(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__NETEVNHND_ASYNCRD ) ;
      return rc ;
   error:
      if ( _isConnected )
      {
         close() ;
      }
      _evSuitPtr->getFrame()->handleClose( _getSharedBase(), _id ) ;
      _evSuitPtr->getFrame()->_erase( handle() ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND_SYNCSNDRAW, "_netEventHandler::syncSendRaw" )
   INT32 _netEventHandler::syncSendRaw( const void *buf, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETEVNHND_SYNCSNDRAW );
      UINT32 send = 0 ;

      /// not care send suc or failed
      _lastSendTick = pmdGetDBTick() ;
      ++_totalIOTimes ;

      while ( send < len )
      {
         try
         {
            send +=  _sock.send( buffer( (const void*)((ossValuePtr)buf + send),
                                         len - send) ) ;
         }
         catch ( boost::system::system_error &e )
         {
            CHAR routeIDBuffer[ MSG_ROUTEID_STRING_MAX_SIZE + 1 ] = { 0 } ;
            if ( e.code().value() == boost::system::errc::interrupted )
            {
               PD_LOG( PDDEBUG, "Connection[Handle:%d, Node:%s] send message "
                       "interrupted: %s,%d", _handle,
                       routeID2String( _id,
                                       routeIDBuffer,
                                       MSG_ROUTEID_STRING_MAX_SIZE ),
                       e.what(), e.code().value() ) ;
               continue ;
            }
            if ( e.code().value() == boost::system::errc::timed_out ||
                 e.code().value() == boost::system::errc::resource_unavailable_try_again )
            {
               PD_LOG( PDWARNING, "Connection[Handle:%d, Node:%s] send "
                       "message timeout: %s:%d", _handle,
                       routeID2String( _id,
                                       routeIDBuffer,
                                       MSG_ROUTEID_STRING_MAX_SIZE ),
                       e.what(), e.code().value() ) ;
               continue ;
            }

            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] send message "
                    "failed: %s,%d", _handle,
                    routeID2String( _id,
                                    routeIDBuffer,
                                    MSG_ROUTEID_STRING_MAX_SIZE ),
                    e.what(), e.code().value() ) ;
            rc = SDB_NET_SEND_ERR ;
            goto error ;
         }
         catch ( exception &e )
         {
            CHAR routeIDBuffer[ MSG_ROUTEID_STRING_MAX_SIZE + 1 ] = { 0 } ;
            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] send message "
                    "failed: %s", _handle,
                    routeID2String( _id, routeIDBuffer,
                                    MSG_ROUTEID_STRING_MAX_SIZE ),
                    e.what() ) ;
            rc = SDB_NET_SEND_ERR ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETEVNHND_SYNCSNDRAW, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND__ALLOBUF, "_netEventHandler::_allocateBuf" )
   INT32 _netEventHandler::_allocateBuf( UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETEVNHND__ALLOBUF );
      if ( _bufLen < len )
      {
         if ( NULL != _buf )
         {
            SDB_OSS_FREE( _buf ) ;
            _bufLen = 0 ;
         }
         _buf = (CHAR *)SDB_OSS_MALLOC( len ) ;
         if ( NULL == _buf )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d] allocate memeory[Len:%u] "
                    "failed", _handle, len ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _bufLen = len ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETEVNHND__ALLOBUF, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETEVNHND__RDCALLBK, "_netEventHandler::_readCallback" )
   void _netEventHandler::_readCallback( const boost::system::error_code &error )
   {
      PD_TRACE_ENTRY ( SDB__NETEVNHND__RDCALLBK ) ;

      if ( error )
      {
         if ( error.value() == boost::system::errc::timed_out ||
              error.value() == boost::system::errc::resource_unavailable_try_again )
         {
            PD_LOG( PDWARNING, "Connection[Handle:%d, Node:%s] receive "
                    "timeout: %s,%d", _handle, routeID2String( _id ).c_str(),
                    error.message().c_str(), error.value() ) ;
            asyncRead() ;
            goto done ;
         }
         else if ( error.value() == boost::system::errc::operation_canceled ||
                   error.value() == boost::system::errc::no_such_file_or_directory ||
                   error.value() == boost::asio::error::operation_aborted )
         {
            PD_LOG ( PDINFO, "Connection[Handle:%d, Node:%s] has been "
                     "closed: %s,%d", _handle, routeID2String( _id ).c_str(),
                     error.message().c_str(), error.value() ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Connection[Handle:%d, Node:%s] occur "
                     "error: %s,%d", _handle, routeID2String( _id ).c_str(),
                     error.message().c_str(), error.value() ) ;
         }

         goto error_close ;
      }

      _lastRecvTick = pmdGetDBTick() ;
      _lastBeatTick = _lastRecvTick ;

      if ( NET_EVENT_HANDLER_STATE_HEADER == _state )
      {
         UINT32 minMsgLen = (SDB_PROTOCOL_VER_2 == _peerVersion) ?
                            sizeof(MsgHeader) : sizeof(MsgHeaderV1) ;

         /// error header
         if ( ( UINT32 )MSG_SYSTEM_INFO_LEN == (UINT32)_header.messageLength )
         {
            // Only node of new version will send the system info request.
            // But some places in the new version will not send the system info
            // request too. So we can NOT be sure the version of peer node is
            // 1 if it hasn't sent the system info request.
            _peerVersion = SDB_PROTOCOL_VER_2 ;
            if ( SDB_OK != _allocateBuf( sizeof(MsgSysInfoRequest) ))
            {
               goto error_close ;
            }
            _hasRecvMsg = TRUE ;
            ossMemcpy( _buf, &_header, sizeof( MsgSysInfoRequest ) ) ;
            _evSuitPtr->getFrame()->handleMsg( _getSharedBase() ) ;
            _state = NET_EVENT_HANDLER_STATE_HEADER ;
            asyncRead() ;
            goto done ;
         }
         else if ( minMsgLen > (UINT32)_header.messageLength ||
                   SDB_MAX_MSG_LENGTH < (UINT32)_header.messageLength )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] received invalid "
                    "message[%s] from %s:%d", _handle,
                    routeID2String( _id ).c_str(),
                    msg2String( &_header, MSG_MASK_ALL, 0 ).c_str(),
                    remoteAddr().c_str(), remotePort() ) ;
            goto error_close ;
         }
         else
         {
            if ( FALSE == _hasRecvMsg )
            {
               _hasRecvMsg = TRUE ;

               if ( SDB_PROTOCOL_VER_INVALID == _peerVersion )
               {
                  // If we hit this code, it means no sys info request has been
                  // received. As the first received length is
                  // sizeof(MsgSysInfoRequest), the MsgHeader::eye is received
                  // already. We can check the version based on that.
                  _peerVersion =
                        ( MSG_COMM_EYE_DEFAULT == _header.eye ) ?
                        SDB_PROTOCOL_VER_2 : SDB_PROTOCOL_VER_1 ;
                  if ( SDB_PROTOCOL_VER_1 == _peerVersion )
                  {
                     _headerSz = sizeof( MsgHeaderV1 ) ;
                     INT32 rcTmp = _enableMsgConvertor() ;
                     if ( rcTmp )
                     {
                        PD_LOG( PDERROR, "Enabled message convertor for "
                                "node[%s] failed[%d]",
                                routeID2String( _id ).c_str(), rcTmp ) ;
                        goto error_close ;
                     }
                     PD_LOG( PDDEBUG, "Message convertor is enabled for "
                             "node[%s] successfully",
                             routeID2String( _id ).c_str() ) ;
                  }
               }

               // need to recv the last header msg
               _state = NET_EVENT_HANDLER_STATE_HEADER_LAST ;
               asyncRead() ;
               _state = NET_EVENT_HANDLER_STATE_HEADER ;
               goto done ;
            }

            SDB_ASSERT( SDB_PROTOCOL_VER_INVALID != _peerVersion,
                        "Peer node version should not be invalid" ) ;

            /// add to route table
            if ( MSG_INVALID_ROUTEID == _id.value )
            {
               if ( MSG_INVALID_ROUTEID != _header.routeID.value )
               {
                  // check service ID
                  if ( _header.routeID.columns.serviceID >=
                                       (UINT16)MSG_ROUTE_SERVICE_TYPE_MAX )
                  {
                     PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] "
                             "received message[%s] with unknown "
                             "service ID [%u]", _handle,
                             routeID2String( _id ).c_str(),
                             msg2String( &_header, MSG_MASK_ALL, 0 ).c_str(),
                             _header.routeID.columns.serviceID ) ;
                     goto error_close ;
                  }
                  _id = _header.routeID ;
                  if ( SDB_OK !=
                        _evSuitPtr->getFrame()->_addRoute( _getSharedBase() ) )
                  {
                     goto error_close ;
                  }
               }
            }

            // If peer node protocol version is 1, we can not print the message
            // like this, as the structure of the message is different. Once all
            // nodes in the cluster finish upgrading, the message can be logged.
            if ( SDB_PROTOCOL_VER_2 == _peerVersion )
            {
               PD_LOG( PDDEBUG, "Connection[Handle:%d, Node:%s] received "
                                "message[%s] from %s:%d", _handle,
                       routeID2String( _id ).c_str(),
                       msg2String( &_header, MSG_MASK_ALL, 0 ).c_str(),
                       remoteAddr().c_str(), remotePort() ) ;
            }
         }
         /// msg has only header
         if ( _headerSz == (UINT32)_header.messageLength )
         {
            if ( SDB_OK != _allocateBuf( _headerSz ) )
            {
               goto error_close ;
            }
            ossMemcpy( _buf, &_header, _headerSz ) ;
            _evSuitPtr->getFrame()->handleMsg( _getSharedBase() ) ;
            _state = NET_EVENT_HANDLER_STATE_HEADER ;
            asyncRead() ;
            goto done ;
         }

#if defined (_LINUX)
         try
         {
            boost::asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_QUICKACK>
                                                    quickack( TRUE ) ;
            _sock.set_option( quickack ) ;
         }
         catch ( exception &e )
         {
            PD_LOG ( PDERROR, "Connection[Handle:%d] quick ack failed: %s",
                     _handle, e.what() ) ;
         }
#endif // _LINUX

         _state = NET_EVENT_HANDLER_STATE_BODY ;
         asyncRead() ;
      }
      else
      {
         _evSuitPtr->getFrame()->handleMsg( _getSharedBase() ) ;
         _state = NET_EVENT_HANDLER_STATE_HEADER ;
         asyncRead() ;
      }

   done:
      PD_TRACE_EXIT ( SDB__NETEVNHND__RDCALLBK ) ;
      return ;
   error_close:
      if ( _isConnected )
      {
         close() ;
      }
      _evSuitPtr->getFrame()->handleClose( _getSharedBase(), _id ) ;
      _evSuitPtr->getFrame()->_erase( handle() ) ;
      goto done ;
   }

   void _netEventHandler::close()
   {
      boost::system::error_code ec ;

      /// only shutdown, don't call _sock.close
      /// because when close, the net-thread will be in asyncRead code.
      /// then close will release socket's descriptor, but asyncRead
      /// will used after, so it cuase null poiniter exception
      /// To fix the bug, we call _sock.close in destructor
      _sock.shutdown( boost::asio::ip::tcp::socket::shutdown_both,
                      ec ) ;
      _isConnected = FALSE ;
      _isNew = FALSE ;
   }

   BOOLEAN _netEventHandler::isSuitStopped() const
   {
      return NULL != _evSuitPtr.get() ? _evSuitPtr->isStoppped() : FALSE ;
   }

}

