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
#include "netEventHandlerBase.hpp"

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

   /*
      _netEventHandler define
    */
   class _netEventHandler : public netEventHandlerBase
   {
      public:
         _netEventHandler( netEvSuitPtr evSuitPtr,
                           const NET_HANDLE &handle ) ;
         virtual ~_netEventHandler() ;

         static NET_EH createShared( netEvSuitPtr evSuitPtr,
                                     const NET_HANDLE &handle ) ;

      public:
         OSS_INLINE boost::asio::ip::tcp::socket &socket()
         {
            return _sock ;
         }
         virtual CHAR *msg()
         {
            return _buf ;
         }
         OSS_INLINE NET_EVENT_HANDLER_STATE state() const
         {
            return _state ;
         }

         virtual void  close() ;

      public:
         virtual INT32 asyncRead() ;

         virtual INT32 syncConnect( const CHAR *hostName,
                                    const CHAR *serviceName ) ;

         virtual INT32 syncSendRaw( const void *buf,
                                    UINT32 len ) ;

         virtual void  setOpt() ;

         virtual std::string localAddr() const ;
         virtual std::string remoteAddr() const ;
         virtual UINT16 localPort() const ;
         virtual UINT16 remotePort() const ;

         OSS_INLINE virtual NET_EVENT_HANDLER_TYPE getHandlerType() const
         {
            return NET_EVENT_HANDLER_TCP ;
         }

         // check if the net suit is stopped
         virtual BOOLEAN isSuitStopped() const ;

      protected:
         void  _readCallback( const boost::system::error_code &error ) ;
         INT32 _allocateBuf( UINT32 len ) ;
         INT32 _syncCheckSysInfo( ossSocket &socket ) ;

         OSS_INLINE NET_TCP_EH _getShared()
         {
            return NET_TCP_EH::makeRaw( this, ALLOC_POOL ) ;
         }

      protected:
         netEvSuitPtr                     _evSuitPtr ;
         boost::asio::ip::tcp::socket     _sock ;
         _MsgHeader                       _header ;
         UINT16                           _headerSz ;
         CHAR                             *_buf ;
         UINT32                           _bufLen ;
         NET_EVENT_HANDLER_STATE          _state ;
         BOOLEAN                          _hasRecvMsg ;
   } ;

}

#endif // NETEVENTHANDLER_HPP_

