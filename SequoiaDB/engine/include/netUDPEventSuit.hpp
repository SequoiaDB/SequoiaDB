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

   Source File Name = netUDPEventSuit.hpp

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

#ifndef NET_UDP_EVENT_SUIT_HPP_
#define NET_UDP_EVENT_SUIT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"
#include "netMsgHandler.hpp"
#include "netUDPEventHandler.hpp"
#include "netInnerTimer.hpp"
#include "ossMemPool.hpp"

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

namespace engine
{

   /*
      _netUDPEventSuit define
    */
   class _netUDPEventSuit : public utilPooledObject
   {
   public:
      _netUDPEventSuit( netFrame *frame,
                        netRoute *route ) ;
      virtual ~_netUDPEventSuit() ;

      static NET_UDP_EV_SUIT createShared( netFrame *frame,
                                           netRoute *route ) ;

   public:
      INT32 listen( const CHAR *hostName,
                    const CHAR *serviceName,
                    INetUDPMsgHandler *handler,
                    UINT32 bufferSize ) ;
      void  setOptions() ;
      void  close() ;
      void  asyncRead() ;
      INT32 syncBroadcast( const void *buf, UINT32 len ) ;

      OSS_INLINE boost::asio::ip::udp::socket *getSocket()
      {
         return ( &_sock ) ;
      }

      OSS_INLINE BOOLEAN isOpened()
      {
         return _sock.is_open() ;
      }

      OSS_INLINE CHAR *getMessage()
      {
         return _buffer ;
      }

      OSS_INLINE netFrame *getFrame()
      {
         return _frame ;
      }

      OSS_INLINE netUDPEndPoint getLocalEndPoint() const
      {
         return _localEndPoint ;
      }

      OSS_INLINE INetUDPMsgHandler *getHandler()
      {
         return _handler ;
      }

      void  handleMsg( NET_EH eh ) ;
      INT32 getEH( const netUDPEndPoint &endPoint,
                   const MsgRouteID &routeID,
                   NET_EH &eh ) ;
      void  removeEH( const netUDPEndPoint &endPoint ) ;
      void  removeEH( const MsgRouteID &routeID ) ;
      void  removeAllEH() ;

   protected:
      typedef ossPoolMap< netUDPEndPoint, NET_EH > NET_UDP_EP2EH_MAP ;
      typedef ossPoolList< NET_EH >                NET_UDP_EH_LIST ;

   protected:
      OSS_INLINE NET_UDP_EV_SUIT _getShared()
      {
         return NET_UDP_EV_SUIT::makeRaw( this, ALLOC_POOL ) ;
      }

      void    _readCallback( const boost::system::error_code &error ) ;
      INT32   _createEH( const netUDPEndPoint &remoteEndPoint,
                         const MsgRouteID &routeID,
                         NET_EH &eh ) ;
      NET_EH  _getEH( const netUDPEndPoint &endPoint ) ;
      INT32   _allocateBuffer( UINT32 bufferSize ) ;

   protected:
      netFrame *                    _frame ;
      INetUDPMsgHandler *           _handler ;
      netRoute *                    _route ;
      netUDPRestartTimer            _restartTimer ;
      boost::asio::ip::udp::socket  _sock ;
      UINT32                        _bufferSize ;
      CHAR *                        _buffer ;
      netUDPEndPoint                _remoteEndPoint ;
      netUDPEndPoint                _localEndPoint ;
      ossSpinSLatch                 _mtx ;
      NET_UDP_EP2EH_MAP             _ep2ehMap ;
   } ;

}

#endif // NET_UDP_EVENT_SUIT_HPP_
