/******************************************************************************

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

   Source File Name = netUDPEventSuit.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "netUDPEventSuit.hpp"
#include "netFrame.hpp"
#include "netRoute.hpp"
#include "ossMem.hpp"
#include "pmdEnv.hpp"
#include "msgDef.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"
#include "msgMessageFormat.hpp"
#include <boost/bind.hpp>

using namespace boost::asio::ip ;

namespace engine
{

   /*
      _netUDPEventSuit implement
    */
   _netUDPEventSuit::_netUDPEventSuit( netFrame *frame,
                                       netRoute *route )
   : _frame( frame ),
     _handler( NULL ),
     _route( route ),
     _restartTimer( frame ),
     _sock( frame->getIOService() ),
     _bufferSize( NET_UDP_DEFAULT_BUFFER_SIZE ),
     _buffer( NULL )
   {
      SDB_ASSERT( NULL != frame, "frame is invalid" ) ;
   }

   _netUDPEventSuit::~_netUDPEventSuit()
   {
      boost::system::error_code ec ;
      _sock.close( ec ) ;
      SAFE_OSS_FREE( _buffer ) ;
   }

   NET_UDP_EV_SUIT _netUDPEventSuit::createShared( netFrame *frame,
                                                   netRoute *route )
   {
      NET_UDP_EV_SUIT suitPtr ;

      NET_UDP_EV_SUIT tmpPtr = NET_UDP_EV_SUIT::allocRaw( ALLOC_POOL ) ;
      if ( NULL != tmpPtr.get() &&
           NULL != new( tmpPtr.get() ) netUDPEventSuit( frame, route ) )
      {
         suitPtr = tmpPtr ;
      }

      return suitPtr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_ASYNCREAD, "_netUDPEventSuit::asyncRead" )
   void _netUDPEventSuit::asyncRead()
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_ASYNCREAD ) ;

      _sock.async_receive_from(
                     buffer( _buffer, _bufferSize ),
                     _remoteEndPoint,
                     boost::bind( ( &_netUDPEventSuit::_readCallback ),
                                  _getShared(),
                                  boost::asio::placeholders::error ) ) ;

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT_ASYNCREAD ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_SYNCBROADCAST, "_netUDPEventSuit::syncBroadcast" )
   INT32 _netUDPEventSuit::syncBroadcast( const void *buf, UINT32 len )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_SYNCBROADCAST ) ;

      UINT32 send = 0 ;

      PD_CHECK( _sock.is_open(), SDB_NETWORK, error, PDERROR,
                "Failed to broadcast message, UDP socket is closed" ) ;

      while ( TRUE )
      {
         try
         {
            netUDPEndPoint endPoint( address_v4::broadcast(),
                                     _localEndPoint.port() ) ;
            send = _sock.send_to( buffer( buf, len ), endPoint ) ;
         }
         catch ( boost::system::system_error &e )
         {
            if ( e.code().value() == boost::system::errc::interrupted )
            {
               PD_LOG( PDDEBUG, "UDP connection broadcast message interrupted: "
                       "%s,%d", e.what(), e.code().value() ) ;
               continue ;
            }
            if ( e.code().value() == boost::system::errc::timed_out ||
                 e.code().value() == boost::system::errc::resource_unavailable_try_again )
            {
               PD_LOG( PDWARNING, "UDP connection broadcast message timeout: "
                       "%s:%d", e.what(), e.code().value() ) ;
               continue ;
            }

            PD_LOG( PDERROR, "UDP connection broadcast message failed: "
                    "%s,%d", e.what(), e.code().value() ) ;
            rc = SDB_NET_SEND_ERR ;
            goto error ;
         }

         PD_CHECK( send == len, SDB_NET_SEND_ERR, error, PDERROR,
                   "UDP connection broadcast message failed, length is not "
                   "matched: given %d sent %d", len, send ) ;

         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVENTSUIT_SYNCBROADCAST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_HANDLEMSG, "_netUDPEventSuit::handleMsg" )
   void _netUDPEventSuit::handleMsg( NET_EH eh )
   {
      MsgHeader *message = (_MsgHeader *)eh->msg() ;

      if ( NULL != _handler )
      {
         _handler->onReceiveMsg( eh->handle(), eh->id(), message ) ;
      }

      // handle heart beats in net frame
      if ( NULL != _handler &&
           MSG_HEARTBEAT != message->opCode &&
           MSG_HEARTBEAT_RES != message->opCode )
      {
         _handler->handleMsg( eh->handle(), message, eh->msg() ) ;
      }
      else
      {
         _frame->handleMsg( eh ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_GETEH, "_netUDPEventSuit::getEH" )
   INT32 _netUDPEventSuit::getEH( const netUDPEndPoint &endPoint,
                                  const MsgRouteID &routeID,
                                  NET_EH &eh )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_GETEH ) ;

      BOOLEAN created = FALSE ;

      NET_EH tmpEH = _getEH( endPoint ) ;
      if ( NULL == tmpEH.get() )
      {
         ossScopedLock lock( ( &_mtx ), EXCLUSIVE ) ;
         // after got exclusive lock, check again
         NET_UDP_EP2EH_MAP::iterator iter = _ep2ehMap.find( endPoint ) ;
         if ( iter != _ep2ehMap.end() )
         {
            tmpEH = iter->second ;
         }
         else
         {
            rc = _createEH( endPoint, routeID, tmpEH ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create event handler, rc: %d",
                         rc ) ;

            // insert into end point map
            try
            {
               _ep2ehMap.insert( make_pair( endPoint, tmpEH ) ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add event handler to "
                       "end point map, occurred unexpected error: %s",
                       e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            created = TRUE ;
         }
      }

      if ( created && NULL != tmpEH.get() )
      {
         // insert new handle into net frame
         _frame->_addOpposite( tmpEH ) ;
      }

      eh = tmpEH ;

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVENTSUIT_GETEH, rc ) ;
      return rc  ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_REMOVEEH_EP, "_netUDPEventSuit::removeEH" )
   void _netUDPEventSuit::removeEH( const netUDPEndPoint &endPoint )
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_REMOVEEH_EP ) ;

      NET_EH eh ;

      {
         ossScopedLock lock( ( &_mtx ), EXCLUSIVE ) ;
         NET_UDP_EP2EH_MAP::iterator iter = _ep2ehMap.find( endPoint ) ;
         if ( iter != _ep2ehMap.end() )
         {
            eh = iter->second ;
            _ep2ehMap.erase( iter ) ;
         }
      }

      if ( NULL != eh.get() )
      {
         _frame->_erase( eh->handle() ) ;
         eh->close() ;
      }

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT_REMOVEEH_EP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_REMOVEEH_RID, "_netUDPEventSuit::removeEH" )
   void _netUDPEventSuit::removeEH( const MsgRouteID &routeID )
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_REMOVEEH_RID ) ;

      netUDPEndPoint endPoint ;
      INT32 rc = _route->route( routeID, endPoint, FALSE ) ;
      if ( SDB_OK == rc )
      {
         removeEH( endPoint ) ;
         _route->clearUDPRoute( routeID, TRUE ) ;
      }

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT_REMOVEEH_RID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_REMOVEALLEH, "_netUDPEventSuit::removeAllEH" )
   void _netUDPEventSuit::removeAllEH()
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_REMOVEALLEH ) ;

      while ( TRUE )
      {
         NET_EH eh ;
         {
            ossScopedLock lock( ( &_mtx ), EXCLUSIVE ) ;
            if ( _ep2ehMap.size() == 0 )
            {
               break ;
            }
            NET_UDP_EP2EH_MAP::iterator iter = _ep2ehMap.begin() ;
            if ( iter != _ep2ehMap.end() )
            {
               eh = iter->second ;
               _ep2ehMap.erase( iter ) ;
            }
         }
         if ( NULL != eh.get() )
         {
            _frame->_erase( eh->handle() ) ;
            eh->close() ;
         }
      }

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT_REMOVEALLEH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT__GETEH, "_netUDPEventSuit::_getEH" )
   NET_EH _netUDPEventSuit::_getEH( const netUDPEndPoint &endPoint )
   {
      NET_EH eh ;

      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT__GETEH ) ;

      ossScopedLock lock( ( &_mtx ), SHARED ) ;
      NET_UDP_EP2EH_MAP::iterator iter = _ep2ehMap.find( endPoint ) ;
      if ( iter != _ep2ehMap.end() )
      {
         eh = iter->second ;
      }

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT__GETEH ) ;

      return eh ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT__ALLOCATEBUFFER, "_netUDPEventSuit::_allocateBuffer" )
   INT32 _netUDPEventSuit::_allocateBuffer( UINT32 bufferSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT__ALLOCATEBUFFER ) ;

      // prepare buffer
      bufferSize = OSS_MAX( bufferSize, NET_UDP_DEFAULT_BUFFER_SIZE ) ;
      if ( NULL == _buffer || bufferSize > _bufferSize )
      {
         CHAR *buffer = (CHAR *)SDB_OSS_REALLOC( _buffer, bufferSize ) ;
         PD_CHECK( NULL != buffer, SDB_OOM, error, PDERROR,
                   "Failed to allocate UDP buffer with size [%u]",
                   bufferSize ) ;
         _buffer = buffer ;
         _bufferSize = bufferSize ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVENTSUIT__ALLOCATEBUFFER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT__READCALLBACK, "_netUDPEventSuit::_readCallback" )
   void _netUDPEventSuit::_readCallback( const boost::system::error_code &error )
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT__READCALLBACK ) ;

      NET_EH eh ;
      MsgHeader *message = (MsgHeader *)_buffer ;

      if ( error )
      {
         if ( error.value() == boost::system::errc::timed_out ||
              error.value() == boost::system::errc::resource_unavailable_try_again )
         {
            PD_LOG( PDWARNING, "UDP connection receive timeout: %s,%d",
                    error.message().c_str(), error.value() ) ;
            asyncRead() ;
            goto done ;
         }
         else if ( error.value() == boost::system::errc::operation_canceled ||
                   error.value() == boost::system::errc::no_such_file_or_directory )
         {
            PD_LOG ( PDINFO, "UDP connection has been closed: %s,%d",
                     error.message().c_str(), error.value() ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "UDP connection occur error: %s,%d",
                     error.message().c_str(), error.value() ) ;
         }

         goto error_close ;
      }

      if ( SDB_OK == getEH( _remoteEndPoint, message->routeID, eh ) )
      {
         netUDPEventHandler *handler = NULL ;

         SDB_ASSERT( NULL != eh.get(), "event handler is invalid" ) ;
         SDB_ASSERT( NET_EVENT_HANDLER_UDP == eh->getHandlerType(),
                     "event handler should be UDP handler" ) ;

         handler = dynamic_cast< netUDPEventHandler * >( eh.get() ) ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         handler->readCallback( message ) ;
      }

      asyncRead() ;

   done:
      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT__READCALLBACK ) ;
      return ;

   error_close :
      close() ;
      // start timer to listen again
      _restartTimer.startTimer() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_LISTEN, "_netUDPEventSuit::listen" )
   INT32 _netUDPEventSuit::listen( const CHAR *hostName,
                                   const CHAR *serviceName,
                                   INetUDPMsgHandler *handler,
                                   UINT32 bufferSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_LISTEN ) ;

      netUDPEndPoint localEndPoint ;

      PD_CHECK( !_sock.is_open(), SDB_NET_ALREADY_LISTENED, error, PDWARNING,
                "UDP socket is opened" ) ;

      _handler = handler ;

      // allocate buffer
      rc = _allocateBuffer( bufferSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to allocate buffer, rc: %d", rc ) ;

      // resolve host name
      rc = netRoute::getUDPEndPoint( NET_LISTEN_HOST, serviceName,
                                     localEndPoint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get local end point, rc: %d",
                   rc ) ;

      try
      {
         _sock.open( localEndPoint.protocol() ) ;
         _sock.bind( localEndPoint ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to listen UDP socket, error: %s",
                 e.what() ) ;
         rc = SDB_NET_CANNOT_LISTEN ;
         goto error ;
      }

      _localEndPoint = localEndPoint ;
      _restartTimer.setInfo( hostName, serviceName, handler, bufferSize ) ;

      setOptions() ;
      asyncRead() ;

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVENTSUIT_LISTEN, rc ) ;
      return rc ;

   error:
      close() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_CLOSE, "_netUDPEventSuit::close" )
   void _netUDPEventSuit::close()
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_CLOSE ) ;

      boost::system::error_code ec ;

      /// only shutdown, don't call _sock.close
      /// because when close, the net-thread will be in asyncRead code.
      /// then close will release socket's descriptor, but asyncRead
      /// will used after, so it cause null pointer exception
      /// To fix the bug, we call _sock.close in destructor
      _sock.shutdown( udp::socket::shutdown_both, ec ) ;

      removeAllEH() ;

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT_CLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT_SETOPT, "_netUDPEventSuit::setOptions" )
   void _netUDPEventSuit::setOptions()
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT_SETOPT ) ;

      // set priority
      try
      {
#ifdef _LINUX
         SOCKET nativeSock = _sock.native() ;
         INT32 priority = 6 ;
         INT32 res = setsockopt( nativeSock, SOL_SOCKET, SO_PRIORITY,
                                 &priority, sizeof( priority ) ) ;
         if ( res )
         {
            PD_LOG( PDWARNING, "UDP connection failed to set priority, "
                    "rc: %d", res ) ;
         }
#endif
         socket_base::broadcast option( true ) ;
         _sock.set_option( option ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to set priority for UDP connection: %s",
                 e.what() ) ;
      }

      // set receive buffer
      try
      {
         socket_base::receive_buffer_size option( NET_UDP_SOCKET_BUFFER_SIZE ) ;
         _sock.set_option( option ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to set receive buffer size for UDP "
                 "connection: %s", e.what() ) ;
      }

      // set broadcast
      try
      {
         socket_base::broadcast option( true ) ;
         _sock.set_option( option ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to set broadcast option for UDP "
                 "connection: %s", e.what() ) ;
      }

      PD_TRACE_EXIT( SDB__NETUDPEVENTSUIT_SETOPT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVENTSUIT__CREATEEH, "_netUDPEventSuit::_createEH" )
   INT32 _netUDPEventSuit::_createEH( const netUDPEndPoint &remoteEndPoint,
                                      const MsgRouteID &routeID,
                                      NET_EH &eh )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVENTSUIT__CREATEEH ) ;

      NET_HANDLE handle = _frame->allocateHandler() ;

      eh = netUDPEventHandler::createShared( _getShared(), handle, routeID,
                                             remoteEndPoint ) ;
      PD_CHECK( NULL != eh.get(), SDB_OOM, error, PDERROR,
                "Failed to allocate UDP event handler" ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVENTSUIT__CREATEEH, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
