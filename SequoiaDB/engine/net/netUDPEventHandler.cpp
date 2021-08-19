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

   Source File Name = netUDPEventHandler.cpp

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
#include "netUDPEventHandler.hpp"
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
using namespace std ;

namespace engine
{

   /*
      _netUDPEventHandler implement
    */
   _netUDPEventHandler::_netUDPEventHandler( NET_UDP_EV_SUIT evSuit,
                                             const NET_HANDLE &handle,
                                             const MsgRouteID &routeID,
                                             const netUDPEndPoint &endPoint )
   : _netEventHandlerBase( handle ),
     _evSuitPtr( evSuit ),
     _remoteEndPoint( endPoint )
   {
      setRouteID( routeID ) ;

      _isConnected = TRUE ;
      _isNew = FALSE ;
   }

   _netUDPEventHandler::~_netUDPEventHandler()
   {
   }

   NET_EH _netUDPEventHandler::createShared( NET_UDP_EV_SUIT evSuit,
                                             const NET_HANDLE &handle,
                                             const MsgRouteID &routeID,
                                             const netUDPEndPoint &endPoint )
   {
      NET_EH eh ;

      NET_UDP_EH tmpEH = NET_UDP_EH::allocRaw( ALLOC_POOL ) ;
      if ( NULL != tmpEH.get() &&
           NULL != new( tmpEH.get() ) netUDPEventHandler( evSuit,
                                                          handle,
                                                          routeID,
                                                          endPoint ) )
      {
         eh = NET_EH::makeRaw( tmpEH.get(), ALLOC_POOL ) ;
      }

      return eh ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVNHND_SYNCCONNECT, "_netUDPEventHandler::syncConnect" )
   INT32 _netUDPEventHandler::syncConnect( const CHAR *hostName,
                                           const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVNHND_SYNCCONNECT ) ;

      PD_CHECK( NULL != _evSuitPtr.get() && _evSuitPtr->isOpened(),
                SDB_NETWORK, error, PDERROR, "Failed to send UDP message to "
                "%s:%s, UDP suit is not opened" ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVNHND_SYNCCONNECT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _netUDPEventHandler::asyncRead()
   {
      // do nothing
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVNHND_SYNCSEND, "_netUDPEventHandler::syncSend" )
   INT32 _netUDPEventHandler::syncSend( const void *buf, UINT32 len )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETUDPEVNHND_SYNCSEND ) ;

      udp::socket * sock = NULL ;
      UINT32 send = 0 ;

      PD_CHECK( _isConnected, SDB_NETWORK, error, PDWARNING,
                "Failed to send message via UDP handle [%u], "
                "it has been closed", _handle ) ;

      PD_CHECK( NULL != _evSuitPtr.get(), SDB_NETWORK, error, PDWARNING,
                "Failed to send message via UDP handle [%u], "
                "suit is invalid", _handle ) ;

      sock = _evSuitPtr->getSocket() ;
      PD_CHECK( NULL != sock, SDB_NETWORK, error, PDWARNING,
                "Failed to send message via UDP handle [%u], "
                "socket is invalid", _handle ) ;


      /// not care send suc or failed
      _lastSendTick = pmdGetDBTick() ;
      ++_totalIOTimes ;

      PD_CHECK( sock->is_open(), SDB_NETWORK, error, PDERROR,
                "Failed to send message, UDP socket is closed" ) ;

      if ( NULL != _evSuitPtr->getHandler() )
      {
         _evSuitPtr->getHandler()->onSendMsg( handle(),
                                              id(),
                                              (MsgHeader *)( buf ) ) ;
      }

      while ( TRUE )
      {
         try
         {
            send = sock->send_to( buffer( buf, len ), _remoteEndPoint ) ;
         }
         catch ( boost::system::system_error &e )
         {
            if ( e.code().value() == boost::system::errc::interrupted )
            {
               PD_LOG( PDDEBUG, "UDP connection send message interrupted: "
                       "%s,%d", e.what(), e.code().value() ) ;
               continue ;
            }
            if ( e.code().value() == boost::system::errc::timed_out ||
                 e.code().value() == boost::system::errc::resource_unavailable_try_again )
            {
               PD_LOG( PDWARNING, "UDP connection send message timeout: %s:%d",
                       e.what(), e.code().value() ) ;
               continue ;
            }

            PD_LOG( PDERROR, "UDP connection send message failed: %s,%d",
                    e.what(), e.code().value() ) ;
            rc = SDB_NET_SEND_ERR ;
            goto error ;
         }

         PD_CHECK( send == len, SDB_NET_SEND_ERR, error, PDERROR,
                   "UDP connection send message failed, length is not "
                   "matched: given %d sent %d", len, send ) ;

         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETUDPEVNHND_SYNCSEND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETUDPEVNHND_CLOSE, "_netUDPEventHandler::close" )
   void _netUDPEventHandler::close()
   {
      PD_TRACE_ENTRY( SDB__NETUDPEVNHND_CLOSE ) ;

      _isConnected = FALSE ;

      PD_TRACE_EXIT( SDB__NETUDPEVNHND_CLOSE ) ;
   }

   void _netUDPEventHandler::setOpt()
   {
      // do nothing
   }

   CHAR *_netUDPEventHandler::msg()
   {
      return _evSuitPtr->getMessage() ;
   }

   string _netUDPEventHandler::localAddr() const
   {
      boost::system::error_code error ;
      string addr ;
      netUDPEndPoint localEndPoint = _evSuitPtr->getLocalEndPoint() ;
      addr = localEndPoint.address().to_string( error ) ;
      if ( error )
      {
         PD_LOG( PDERROR, "Failed to get local address, occurred error: %s",
                 error.message().c_str() ) ;
      }
      return addr ;
   }

   string _netUDPEventHandler::remoteAddr() const
   {
      boost::system::error_code error ;
      string addr ;
      addr = _remoteEndPoint.address().to_string( error ) ;
      if ( error )
      {
         PD_LOG( PDERROR, "Failed to get local address, occurred error: %s",
                 error.message().c_str() ) ;
      }
      return addr ;
   }

   UINT16 _netUDPEventHandler::localPort() const
   {
      return _evSuitPtr->getLocalEndPoint().port() ;
   }

   UINT16 _netUDPEventHandler::remotePort() const
   {
      return _remoteEndPoint.port() ;
   }

   void _netUDPEventHandler::readCallback( MsgHeader *message )
   {
      _lastRecvTick = pmdGetDBTick() ;
      _lastBeatTick = _lastRecvTick ;

      PD_LOG( PDDEBUG, "UDP connection[Handle:%d] received "
              "message[%s] from %s:%d", _handle,
              msg2String( message, MSG_MASK_ALL, 0 ).c_str(),
              _remoteEndPoint.address().to_string().c_str(),
              _remoteEndPoint.port() ) ;

      if ( !_isConnected )
      {
         _isConnected = TRUE ;
      }

      _evSuitPtr->handleMsg( _getSharedBase() ) ;
   }

   void _netUDPEventHandler::setRouteID( const MsgRouteID &routeID )
   {
      if ( MSG_INVALID_ROUTEID == _id.value &&
           MSG_INVALID_ROUTEID != routeID.value )
      {
         id( routeID ) ;
      }
   }

}
