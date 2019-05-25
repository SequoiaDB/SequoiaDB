/******************************************************************************


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

   Source File Name = netEventHandler.hpp

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

#ifndef NETEVENTHANDLER_HPP_
#define NETEVENTHANDLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
using namespace boost::asio ;
using namespace std ;

namespace engine
{
   /*
      NET_EVENT_HANDLER_STATE define
   */
   enum NET_EVENT_HANDLER_STATE
   {
      NET_EVENT_HANDLER_STATE_HEADER         = 0,
      NET_EVENT_HANDLER_STATE_HEADER_LAST,
      NET_EVENT_HANDLER_STATE_BODY
   } ;

   class _netEventSuit ;
   typedef boost::shared_ptr<_netEventSuit>     netEvSuitPtr ;

   /*
      _netEventHandler define
   */
   class _netEventHandler :
         public boost::enable_shared_from_this<_netEventHandler>,
         public SDBObject
   {
      public:
         _netEventHandler( netEvSuitPtr evSuitPtr,
                           const NET_HANDLE &handle  ) ;
         ~_netEventHandler() ;

         netEvSuitPtr getEVSuitPtr() const { return _evSuitPtr ; }

         BOOLEAN  isConnected() const { return _isConnected ; }
         BOOLEAN  isNew() const { return _isNew ; }

      public:
         OSS_INLINE void id( const _MsgRouteID &id )
         {
            _id = id ;
         }
         OSS_INLINE const _MsgRouteID &id()
         {
            return _id ;
         }
         OSS_INLINE UINT64 getAndIncMsgID( BOOLEAN inc = TRUE )
         {
            if ( inc )
            {
               return _msgid++ ;
            }
            return _msgid ;
         }
         OSS_INLINE boost::asio::ip::tcp::socket &socket()
         {
            return _sock ;
         }
         OSS_INLINE _ossSpinXLatch &mtx()
         {
            return _mtx ;
         }
         OSS_INLINE NET_HANDLE handle() const
         {
            return _handle ;
         }
         CHAR *msg()
         {
            return _buf ;
         }
         OSS_INLINE NET_EVENT_HANDLER_STATE state() const
         {
            return _state ;
         }

         void  close() ;

         UINT64 getLastSendTick() const { return _lastSendTick ; }
         UINT64 getLastRecvTick() const { return _lastRecvTick ; }
         UINT64 getLastBeatTick() const { return _lastBeatTick ; }

         UINT32 getMBPS() const { return _mbps ; }

         void   syncLastBeatTick() ;
         void   makeStat( UINT64 curTick ) ;

      public:
         void  asyncRead() ;

         INT32 syncConnect( const CHAR *hostName,
                            const CHAR *serviceName ) ;

         INT32 syncSend( const void *buf,
                         UINT32 len ) ;

         void  setOpt() ;

         string localAddr() const ;
         string remoteAddr() const ;
         UINT16 localPort() const ;
         UINT16 remotePort() const ;

         BOOLEAN isLocalConnection() const ;

      private:
         void  _readCallback( const boost::system::error_code &error ) ;
         INT32 _allocateBuf( UINT32 len ) ;

      private:
         boost::asio::ip::tcp::socket     _sock ;
         _ossSpinXLatch                   _mtx ;
         _MsgHeader                       _header ;
         CHAR                             *_buf ;
         UINT32                           _bufLen ;
         NET_EVENT_HANDLER_STATE          _state ;
         _MsgRouteID                      _id ;
         netEvSuitPtr                     _evSuitPtr ;
         NET_HANDLE                       _handle ;
         volatile BOOLEAN                 _isConnected ;
         volatile BOOLEAN                 _isNew ;
         BOOLEAN                          _hasRecvMsg ;
         UINT64                           _lastSendTick ;
         UINT64                           _lastRecvTick ;
         UINT64                           _lastBeatTick ;
         UINT64                           _msgid ;

         UINT64                           _lastStatTick ;
         UINT64                           _srDataLen ;
         UINT32                           _mbps ;     /// Byte/s

   } ;
   typedef _netEventHandler netEventHandler ;

   typedef boost::shared_ptr<_netEventHandler> NET_EH ;

}

#endif // NETEVENTHANDLER_HPP_

