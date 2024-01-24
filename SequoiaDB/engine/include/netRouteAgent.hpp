/*******************************************************************************


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

   Source File Name = netRouteAgent.hpp

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

#ifndef NETROUTEAGENT_HPP_
#define NETROUTEAGENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netRoute.hpp"
#include "netFrame.hpp"

namespace engine
{
   /*
      _netRouteAgent define
   */
   class _netRouteAgent : public SDBObject
   {
      public:
         _netRouteAgent( _netMsgHandler *handler,
                         const NET_HANDLE &beginID = NET_MIN_HANDLE ) ;

         _netRoute* getRoute() { return &_route ; }
         _netFrame* getFrame() { return &_frame ; }

      public:
         OSS_INLINE INT32 run( NET_START_THREAD_FUNC pFunc = NULL )
         {
            _frame.setNetStartThreadFunc( pFunc ) ;
            return _frame.run() ;
         }

         OSS_INLINE void stop()
         {
            return _frame.stop() ;
         }

         OSS_INLINE void setLocalID( const _MsgRouteID &id )
         {
            _frame.setLocal( id ) ;
            _route.setLocal( id ) ;
            return ;
         }

         OSS_INLINE MsgRouteID localID()
         {
            return _route.local() ;
         }

         OSS_INLINE INT32 addTimer( UINT32 millsec,
                                    _netTimeoutHandler *handler,
                                    UINT32 &timerid )
         {
            return _frame.addTimer( millsec, handler, timerid ) ;
         }

         OSS_INLINE INT32 removeTimer( UINT32 timerid )
         {
            return _frame.removeTimer( timerid ) ;
         }

         OSS_INLINE void close( const _MsgRouteID &id )
         {
            _frame.close( id ) ;
         }

         OSS_INLINE void close( const NET_HANDLE &handle )
         {
            _frame.close( handle ) ;
         }

         OSS_INLINE void shutdownListen()
         {
            // WARNING: can not call close which is not thread-safe
            // during asyncAccept of acceptor
            _frame.shutdownListen( NET_FRAME_MASK_ALL ) ;
         }

         OSS_INLINE void disconnectAll()
         {
            _frame.close() ;
         }

         OSS_INLINE  IIOService* ioservice()
         {
            return &_frame ;
         }

         OSS_INLINE INT32 route( const _MsgRouteID &id,
                                 _netRouteNode &node )
         {
            return _route.route( id, node ) ;
         }

      public:
         INT32 listen( const _MsgRouteID &id,
                       UINT32 protocolMask = NET_FRAME_MASK_TCP,
                       UINT32 udpBufferSize = NET_UDP_DEFAULT_BUFFER_SIZE ) ;

         INT32 syncConnect( const _MsgRouteID &id,
                            NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSend( const _MsgRouteID &id,
                         MsgHeader *header,
                         NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSendUDP( const MsgRouteID &id,
                            void *header ) ;

         INT32 syncSend( const NET_HANDLE &handle,
                         MsgHeader *header ) ;

         INT32 syncSendRaw( const NET_HANDLE &handle,
                            const CHAR *pBuff,
                            UINT32 buffSize ) ;

         INT32 syncSend( const _MsgRouteID &id,
                         MsgHeader *header,
                         void *body,
                         UINT32 bodyLen,
                         NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSend( const NET_HANDLE &handle,
                         MsgHeader *header,
                         void *body,
                         UINT32 bodyLen ) ;

         INT32 syncSendv( const _MsgRouteID &id,
                          MsgHeader *header,
                          const netIOVec &iov,
                          NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSendv( const NET_HANDLE &handle,
                          MsgHeader *header,
                          const netIOVec &iov ) ;

         INT32 updateRoute( const _MsgRouteID &id,
                            const CHAR *host,
                            const CHAR *service ) ;

         INT32 updateRoute( const _MsgRouteID &id,
                            const _netRouteNode &node ) ;

         INT32 updateRoute( const _MsgRouteID &oldID,
                            const _MsgRouteID &newID ) ;

         void  delRoute( const _MsgRouteID &id ) ;

         INT64 netIn() ;

         INT64 netOut() ;

      private:
         _netFrame _frame ;
         _netRoute _route ;
   } ;

   typedef class _netRouteAgent netRouteAgent ;

}

#endif // NETROUTEAGENT_HPP_

