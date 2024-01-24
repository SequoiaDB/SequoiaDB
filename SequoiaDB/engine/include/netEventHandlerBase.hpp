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

   Source File Name = netEventHandlerBase.hpp

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

#ifndef NET_EVENT_HANDLER_BASE_HPP_
#define NET_EVENT_HANDLER_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"
#include "msgConvertor.hpp"

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

namespace engine
{

   /*
      NET_EVENT_HANDLER_TYPE
    */
   enum NET_EVENT_HANDLER_TYPE
   {
      NET_EVENT_HANDLER_UNKNOWN  = 0,
      NET_EVENT_HANDLER_TCP      = 1,
      NET_EVENT_HANDLER_UDP      = 2
   } ;

   /*
      _netEventHandlerBase define
      Handler to deal with network events. The main task is to establish the
      connection(for TCP), and to send messages. It should be used in exclusive
      mode, that is, the user need to take X lock on it before using.
    */
   class _netEventHandlerBase : public utilPooledObject
   {
   public:
      _netEventHandlerBase( const NET_HANDLE &handle ) ;
      virtual ~_netEventHandlerBase() ;

   public:
      OSS_INLINE UINT64 getAndIncMsgID( BOOLEAN inc = TRUE )
      {
         if ( inc )
         {
            return _msgid++ ;
         }
         return _msgid ;
      }

      OSS_INLINE ossSpinXLatch &mtx()
      {
         return _mtx ;
      }

      OSS_INLINE NET_HANDLE handle() const
      {
         return _handle ;
      }

      OSS_INLINE BOOLEAN isConnected() const
      {
         return _isConnected ;
      }

      OSS_INLINE BOOLEAN isNew() const
      {
         return _isNew ;
      }

      OSS_INLINE void id( const MsgRouteID &id )
      {
         _id = id ;
      }

      OSS_INLINE const MsgRouteID &id()
      {
         return _id ;
      }

      OSS_INLINE UINT64 getLastSendTick() const
      {
         return _lastSendTick ;
      }

      OSS_INLINE UINT64 getLastRecvTick() const
      {
         return _lastRecvTick ;
      }

      OSS_INLINE UINT64 getLastBeatTick() const
      {
         return _lastBeatTick ;
      }

      OSS_INLINE UINT32 getIOPS() const
      {
         return _iops ;
      }

      void  syncLastBeatTick() ;
      void  makeStat( UINT64 curTick ) ;

      IMsgConvertor *getInMsgConvertor() ;
      IMsgConvertor *getOutMsgConvertor() ;

      virtual NET_EVENT_HANDLER_TYPE getHandlerType() const = 0 ;
      virtual INT32 syncConnect( const CHAR *hostName,
                                 const CHAR *serviceName ) = 0 ;
      virtual INT32 asyncRead() = 0 ;

      /**
       * @brief Send data in its raw format. It will NOT be treated as in any
       *        particular format.
       * @param buff Data to be sent.
       * @param len  Length of the data.
       */
      virtual INT32 syncSendRaw( const void *buff, UINT32 len ) = 0 ;

      virtual void close() = 0 ;
      virtual CHAR *msg() = 0 ;
      virtual void setOpt() = 0 ;

      virtual std::string localAddr() const = 0 ;
      virtual std::string remoteAddr() const = 0 ;
      virtual UINT16 localPort() const = 0 ;
      virtual UINT16 remotePort() const = 0 ;

      OSS_INLINE BOOLEAN isLocalConnection() const
      {
         return localAddr() == remoteAddr() ? TRUE : FALSE ;
      }

      // check if the net suit is stopped
      virtual BOOLEAN isSuitStopped() const = 0 ;

   protected:
      OSS_INLINE NET_EH _getSharedBase()
      {
         return NET_EH::makeRaw( this, ALLOC_POOL ) ;
      }

      INT32 _enableMsgConvertor() ;

   protected:
      ossSpinXLatch           _mtx ;
      NET_HANDLE              _handle ;
      MsgRouteID              _id ;
      volatile BOOLEAN        _isConnected ;
      volatile BOOLEAN        _isNew ;
      UINT64                  _msgid ;
      UINT64                  _lastSendTick ;
      UINT64                  _lastRecvTick ;
      UINT64                  _lastBeatTick ;
      UINT64                  _lastStatTick ;
      UINT64                  _totalIOTimes ;
      UINT32                  _iops ;     /// times/s
      SDB_PROTOCOL_VERSION    _peerVersion ;
      IMsgConvertor           *_inMsgConvertor ;
      IMsgConvertor           *_outMsgConvertor ;
   } ;

}

#endif // NET_EVENT_HANDLER_BASE_HPP_
