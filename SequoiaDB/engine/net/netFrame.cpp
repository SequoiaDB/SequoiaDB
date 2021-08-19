/*******************************************************************************


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

   Source File Name = netFrame.cpp

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
#include "netFrame.hpp"
#include "netMsgHandler.hpp"
#include "msgDef.h"
#include "pmdEnv.hpp"
#include "pd.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"
#include "netRoute.hpp"
#include <boost/bind.hpp>

using namespace boost::asio::ip ;

namespace engine
{

   #define NET_IOPS_MIN_VALUE             ( 500 )
   #define NET_IOPS_THRESHOLD             ( 5000 )

   /*
     _netEHSegment implement
   */
   _netEHSegment::_netEHSegment( _netFrame *pFrame, UINT32 capacity, const _MsgRouteID &id )
    :_pFrame( pFrame ),
     _id ( id ),
     _index( 0 ),
     _mtx( MON_LATCH_NETEHSEGMENT_MTX )
   {
      if ( MSG_ROUTE_SHARD_SERVCIE != id.columns.serviceID ||
           id.columns.groupID < DATA_GROUP_ID_BEGIN ||
           id.columns.groupID > DATA_GROUP_ID_END )
      {
         _capacity = 1 ;
      }
      else
      {
         _capacity = ( capacity > 0) ? capacity : 1 ;
      }
   }

   _netEHSegment::~_netEHSegment()
   {
      _vecEH.clear() ;
   }

   netEHSegPtr _netEHSegment::createShared( netFrame *pFrame,
                                            UINT32 capacity,
                                            const MsgRouteID &id )
   {
      netEHSegPtr segPtr ;

      netEHSegPtr tmpPtr = netEHSegPtr::allocRaw( ALLOC_POOL ) ;
      if ( NULL != tmpPtr.get() &&
           NULL != new( tmpPtr.get() ) netEHSegment( pFrame,
                                                     capacity,
                                                     id ) )
      {
         segPtr = tmpPtr ;
      }

      return segPtr ;
   }

   // return an event handler to the caller
   // Algorithm: During start time when # of socket is smaller than capacity,
   // keeping creating it once a time, when this function is called until we hit
   // the capacity. After we hit the capacity, do a round robin.
   INT32 _netEHSegment::getEH( NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      UINT32 retries = 0 ;
   retry:
      _mtx.get_shared() ;
      if ( _vecEH.size() >= _capacity || retries == 2 )
      {
         if ( _vecEH.size() == 0 )
         {
            PD_LOG( PDSEVERE, "cannot create any net event handler" ) ;
            rc = SDB_OOM ;
         }
         else
         {
            eh = _vecEH[_index.inc() % _vecEH.size()] ;
         }
         _mtx.release_shared() ;
      }
      else
      {
         _mtx.release_shared() ;

         BOOLEAN created = _createEH( eh ) ;

         if ( !eh.get() )
         {
            retries++ ;
            goto retry ;
         }
         else if ( created )
         {
            // add to opposite map
            _pFrame->_addOpposite( eh ) ;
         }
      }

      return rc ;
   }

   // creating the netEventHandler. We have to get the
   // size of created event handler to make sure after
   // we get x latch, there is still room to create a
   // new one
   BOOLEAN _netEHSegment::_createEH( NET_EH &eh )
   {
      BOOLEAN ret = FALSE ;

      _mtx.get() ;
      if ( _vecEH.size() < _capacity )
      {
         /// create a new socket
         NET_EH tmpEH = netEventHandler::createShared(
                                  _pFrame->_getEvSuit( TRUE ),
                                  (NET_HANDLE)( _pFrame->_handle.inc() ) ) ;
         if ( NULL == tmpEH.get() )
         {
            PD_LOG( PDERROR, "Allocate netEventHandler failed" ) ;
            goto done ;
         }

         eh = tmpEH ;
         eh->id( _id ) ;
         _vecEH.push_back(eh) ;
         ret = TRUE ;
      }
      else
      {
         eh = _vecEH[_index.inc() % _vecEH.size() ] ;
      }

   done:
      _mtx.release() ;
      return ret ;
   }

   void _netEHSegment::close()
   {
      _mtx.get_shared() ;
      for ( VEC_EH_IT itr=_vecEH.begin(); itr!=_vecEH.end(); ++itr )
      {
         (*itr)->close() ;
      }
      _mtx.release_shared() ;
   }

   // This function is called when a connection is passively
   // created and it needs to be added into netEHSegment,
   // otherwise the connection will be lost. Therefore, it is possible
   // that the new connection has to be added into a container that has
   // already hit the capacity, in that case we will resize the container
   // by increasing the capacity
   void _netEHSegment::addEH( NET_EH eh )
   {
      _mtx.get() ;
      _vecEH.push_back(eh) ;
      _mtx.release() ;
   }

   void _netEHSegment::delEH( const NET_HANDLE& handle )
   {
      _mtx.get() ;
      for ( VEC_EH_IT itr=_vecEH.begin(); itr!=_vecEH.end(); ++itr )
      {
         if ( handle == (*itr)->handle() )
         {
            _vecEH.erase(itr) ;
            break ;
         }
      }
      _mtx.release() ;
   }

   /// define listen host
   #define NET_LISTEN_HOST          "0.0.0.0"

   /*
      _netFrame implement
   */
   _netFrame::_netFrame( _netMsgHandler *handler, _netRoute *pRoute )
   : _protocolMask( NET_FRAME_MASK_EMPTY ),
     _pRoute( pRoute ),
     // this might have bad-alloc issue in initialize phase
     // but when this happens, the whole system might not be able
     // to start at the beginning event the bad-alloc is handled
     _mainSuitPtr( netEventSuit::createShared( this ) ),
     _suiteMtx( MON_LATCH_NETFRAME_SUITEMTX ),
     _handler( handler ),
     _mtx( MON_LATCH_NETFRAME_MTX ),
     _acceptor( _mainSuitPtr->getIOService() ),
     _handle( NET_HANDLE_BEGIN ),
     _timerID( NET_INVALID_TIMER_ID ),
     _netOut( 0 ),
     _netIn( 0 ),
     _restartTimer( this ),
     _suiteExitMutex( MON_LATCH_NETFRAME_SUITEEXITMUTEX ),
     _suiteStopFlag( FALSE )
   {
      _pThreadFunc = NULL ;
      _local.value = MSG_INVALID_ROUTEID ;
      _beatInterval = NET_HEARTBEAT_INTERVAL ;
      _beatTimeout = 0 ;
      _beatLastTick = pmdGetDBTick() ;
      _checkBeat = FALSE ;

      _statInterval = NET_MAKE_STAT_INTERVAL ;
      _statLastTick = 0 ;

      _maxSockPerNode = 1 ;
      _maxSockPerThread = 0 ;
      _maxThreadNum = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_DECONS, "_netFrame::~_netFrame" )
   _netFrame::~_netFrame()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_DECONS );
      stop() ;
      _route.clear() ;
      _timers.clear() ;
      _opposite.clear() ;
      PD_TRACE_EXIT ( SDB__NETFRAME_DECONS );
   }

   void _netFrame::setMaxSockPerNode( UINT32 maxSockPerNode )
   {
      _maxSockPerNode = maxSockPerNode ;
   }

   void _netFrame::setMaxSockPerThread( UINT32 maxSockPerThread )
   {
      _maxSockPerThread = maxSockPerThread ;
   }

   void _netFrame::setMaxThreadNum( UINT32 maxThreadNum )
   {
      if ( maxThreadNum >= 1 )
      {
         _maxThreadNum = maxThreadNum - 1 ;
      }
      else
      {
         _maxThreadNum = 0 ;
      }
   }

   void _netFrame::onRunSuitStart( netEvSuitPtr evSuitPtr )
   {
      /// do nothing
   }

   void _netFrame::onRunSuitStop( netEvSuitPtr evSuitPtr )
   {
      /// make sure all the netEventHandles have closed
      _suiteMtx.get() ;
      _eraseSuit_i( evSuitPtr ) ;
      _suiteMtx.release() ;

      MsgRouteID nodeID ;
      _netEventSuit::SET_HANDLE setHandles = evSuitPtr->getHandles() ;
      _netEventSuit::SET_HANDLE_IT itr = setHandles.begin() ;
      while( itr != setHandles.end() )
      {
         close( *itr, &nodeID ) ;
         if ( MSG_INVALID_ROUTEID != nodeID.value )
         {
            _handler->handleClose( *itr, nodeID ) ;
         }
         ++itr ;
      }
      // make sure event handlers are released
      // NOTE: handler has shared pointer of event suit, if we
      //       don't release handlers, the event suit will not be
      //       released
      evSuitPtr->removeAllEH() ;
   }

   void _netFrame::onSuitTimer( netEvSuitPtr evSuitPtr )
   {
      ossScopedLock lock( &_suiteMtx, EXCLUSIVE ) ;

      if ( 0 == evSuitPtr->getHandleNum() )
      {
         /// stop the suit and remove it
         evSuitPtr->stop() ;
         _eraseSuit_i( evSuitPtr ) ;
      }
   }

   UINT32 _netFrame::getEvSuitSize()
   {
      UINT32 size = 0 ;
      _suiteMtx.get_shared() ;
      size = _vecEvSuit.size() ;
      _suiteMtx.release_shared() ;

      return size ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_RUN, "_netFrame::run" )
   INT32 _netFrame::run()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_RUN ) ;

      INT32 retryCount = 0 ;

      /// start dummy timer
      rc = _restartTimer.startDummyTimer() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dummy timer failed, rc: %d", rc ) ;
         goto error ;
      }

      /// run main suit ioservice
      _mainSuitPtr->getIOService().run() ;
      onRunSuitStop( _mainSuitPtr ) ;

      /// stop all evSuit
      _stopAllEvSuit() ;
      close() ;

      /// wait all evSuit stop
      while( TRUE )
      {
         if ( getEvSuitSize() > 0 )
         {
            ossSleep( 200 ) ;
            // sub-network may be added after quiesced
            // let's retry stop after each second
            ++ retryCount ;
            if ( 0 == retryCount % 5 )
            {
               _stopAllEvSuit() ;
            }
            continue ;
         }
         break ;
      }

      if ( _handler )
      {
         _handler->onStop() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME_RUN, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_STOP, "_netFrame::stop" )
   void _netFrame::stop()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_STOP );

      _suiteMtx.get() ;
      _suiteStopFlag = TRUE ;
      _suiteMtx.release() ;

      closeListen( NET_FRAME_MASK_ALL ) ;
      _mainSuitPtr->getIOService().stop() ;
      PD_TRACE_EXIT ( SDB__NETFRAME_STOP );
   }

   void _netFrame::setNetStartThreadFunc( NET_START_THREAD_FUNC pFunc )
   {
      _pThreadFunc = pFunc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_MAKESTAT, "_netFrame::makeStat" )
   void _netFrame::makeStat( UINT32 timeout )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_MAKESTAT ) ;
      UINT64 span = pmdGetTickSpanTime( _statLastTick ) ;
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;

      if ( span >= _statInterval )
      {
         _statLastTick = pmdGetDBTick() ;

         while( TRUE )
         {
            _mtx.get_shared() ;
            itr = _opposite.upper_bound( handle ) ;
            if ( itr == _opposite.end() )
            {
               _mtx.release_shared() ;
               break ;
            }
            eh = itr->second ;
            handle = itr->first ;
            _mtx.release_shared() ;

            /// make stat
            eh->makeStat( _statLastTick ) ;
         }
      }

      PD_TRACE_EXIT ( SDB__NETFRAME_MAKESTAT ) ;
   }

   void _netFrame::setBeatInfo( UINT32 beatTimeout, UINT32 beatInteval )
   {
      if ( beatTimeout > 0 && beatTimeout < 2000 )
      {
         beatTimeout = 2000 ;
         beatInteval = 1000 ;
      }
      if ( 0 == beatInteval )
      {
         beatInteval = beatTimeout / 5 ;
      }
      if ( beatInteval < 1000 )
      {
         beatInteval = 1000 ;
      }
      _beatInterval = beatInteval ;
      _beatTimeout = beatTimeout ;
   }

   void _netFrame::_heartbeat( INT32 serviceType )
   {
      MsgHeader beat ;
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;

      beat.messageLength = sizeof( MsgHeader ) ;
      beat.opCode = MSG_HEARTBEAT ;
      beat.requestID = 0 ;
      beat.routeID.value = _local.value ;
      beat.TID = 0 ;

      while( TRUE )
      {
         _mtx.get_shared() ;
         itr = _opposite.upper_bound( handle ) ;
         if ( itr == _opposite.end() )
         {
            _mtx.release_shared() ;
            break ;
         }
         eh = itr->second ;
         handle = itr->first ;
         _mtx.release_shared() ;

         if ( eh->isNew() || !eh->isConnected() )
         {
            // it is new (not connected), no need to check beat
            continue ;
         }

         /// send msg if had not received message for a while
         if ( pmdGetTickSpanTime( eh->getLastBeatTick() ) >= _beatInterval &&
              ( -1 == serviceType ||
                serviceType == eh->id().columns.serviceID ) )
         {
            eh->mtx().get() ;
            beat.requestID = eh->getAndIncMsgID() ;
            eh->syncSend( &beat, beat.messageLength ) ;
            eh->mtx().release() ;
         }
      }
   }

   void _netFrame::_handleHeartBeat( NET_EH eh, MsgHeader *message )
   {
      SDB_ASSERT( NULL != message, "message is invalid" ) ;
      MsgOpReply reply ;
      reply.header.messageLength = sizeof( MsgOpReply ) ;
      reply.header.opCode = MSG_HEARTBEAT_RES ;
      reply.header.requestID = message->requestID ;
      reply.header.routeID.value = 0 ;
      reply.header.TID = message->TID ;
      reply.contextID = -1 ;
      reply.numReturned = 0 ;
      reply.startFrom = 0 ;
      reply.flags = pmdDBIsAbnormal() ? SDB_SYS : SDB_OK ;

      eh->mtx().get() ;
      reply.header.routeID = _local ;
      eh->syncSend( (const void*)&reply, reply.header.messageLength ) ;
      eh->mtx().release() ;
   }

   void _netFrame::_handleHeartBeatRes( NET_EH eh, MsgHeader *message )
   {
      SDB_ASSERT( NULL != message, "message is invalid" ) ;
      MsgOpReply *reply = (MsgOpReply *)message ;
      if ( SDB_OK != reply->flags )
      {
         PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] is broken "
                 "because of node is abnormal[%d]",
                 eh->handle(), routeID2String( eh->id() ).c_str(),
                 reply->flags ) ;
         eh->close() ;
      }
   }

   void _netFrame::_checkBreak( UINT32 timeout, INT32 serviceType )
   {
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;
      UINT64 spanTime = 0 ;
      MsgRouteID routeid ;

      while( timeout > 0 )
      {
         _mtx.get_shared() ;
         itr = _opposite.upper_bound( handle ) ;
         if ( itr == _opposite.end() )
         {
            _mtx.release_shared() ;
            break ;
         }
         eh = itr->second ;
         handle = itr->first ;
         _mtx.release_shared() ;

         if ( eh->isNew() ||
              NET_EVENT_HANDLER_UDP == eh->getHandlerType() )
         {
            continue ;
         }

         spanTime = pmdGetTickSpanTime( eh->getLastRecvTick() ) ;
         /// check break
         if ( ( -1 == serviceType ||
                serviceType == eh->id().columns.serviceID ) &&
              spanTime >= timeout )
         {
            routeid = eh->id() ;
            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] is "
                    "broken[BrokenTime: %lld(ms)]",
                    handle, routeID2String( routeid ).c_str(),
                    spanTime ) ;
            eh->close() ;
         }
      }
   }

   void _netFrame::heartbeat( UINT32 interval, INT32 serviceType )
   {
      UINT32 beatTimeout = _beatTimeout ;
      UINT64 spanTime = pmdGetTickSpanTime( _beatLastTick ) ;

      if ( 0 == beatTimeout )
      {
         return ;
      }

      if ( _checkBeat )
      {
         _checkBeat = FALSE ;
         _checkBreak( beatTimeout, serviceType ) ;
      }
      else if ( spanTime >= _beatInterval )
      {
         _beatLastTick = pmdGetDBTick() ;
         _checkBeat = TRUE ;
         _heartbeat( serviceType ) ;

         if ( spanTime > 3 * _beatInterval )
         {
            PD_LOG( PDWARNING, "Heartbeat span time[%u] is more than "
                    "interval time[%u], the thread maybe blocked by "
                    "some operations", spanTime, _beatInterval ) ;
         }
      }
   }

   UINT32 _netFrame::getCurrentLocalAddress()
   {
      UINT32 ip = 0 ;

      try
      {
         boost::asio::io_service io_srv ;
         tcp::resolver resolver( io_srv ) ;
         tcp::resolver::query query( boost::asio::ip::host_name(), "") ;
         tcp::resolver::iterator itr = resolver.resolve( query ) ;
         tcp::resolver::iterator end ;
         for ( ; itr != end; itr++ )
         {
            tcp::endpoint ep = *itr ;
            if ( ep.address().is_v4() )
            {
               ip = ep.address().to_v4().to_ulong() ;
               break ;
            }
         }
      }
      catch ( std::exception& )
      {
         // ignore error
      }

      return ip ;
   }

   UINT32 _netFrame::getLocalAddress()
   {
      static UINT32 ip = _netFrame::getCurrentLocalAddress() ;
      return ip ;
   }

   NET_EH _netFrame::getEventHandle( const NET_HANDLE &handle )
   {
      NET_EH eh ;
      MAP_EVENT_IT itr ;

      _mtx.get_shared() ;

      itr = _opposite.find( handle ) ;
      if ( _opposite.end() != itr )
      {
         eh = itr->second ;
      }
      _mtx.release_shared() ;

      return eh ;
   }

   NET_EVENT_HANDLER_TYPE _netFrame::getEventHandleType(
                                                   const NET_HANDLE &handle )
   {
      NET_EVENT_HANDLER_TYPE type = NET_EVENT_HANDLER_UNKNOWN ;

      NET_EH eh = getEventHandle( handle ) ;
      if ( NULL != eh.get() )
      {
         type = eh->getHandlerType() ;
      }

      return type ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_LISTEN, "_netFrame::listen" )
   INT32 _netFrame::listen( const CHAR *hostName,
                            const CHAR *serviceName,
                            UINT32 protocolMask,
                            INetUDPMsgHandler *udpHandler,
                            UINT32 udpBufferSize )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETFRAME_LISTEN ) ;

      PD_CHECK( OSS_BIT_TEST( protocolMask, NET_FRAME_MASK_TCP ) ||
                OSS_BIT_TEST( protocolMask, NET_FRAME_MASK_UDP ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to listen, protocol mask [%d] is unknown",
                protocolMask ) ;

      if ( OSS_BIT_TEST( protocolMask, NET_FRAME_MASK_TCP ) )
      {
         rc = _listenTCP( hostName, serviceName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to listen TCP for host %s "
                      "service %s, rc: %d", hostName, serviceName, rc ) ;
      }
      if ( OSS_BIT_TEST( protocolMask, NET_FRAME_MASK_UDP ) )
      {
         rc = _listenUDP( hostName, serviceName, udpHandler, udpBufferSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to listen UDP for host %s "
                      "service %s, rc: %d", hostName, serviceName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME_LISTEN, rc ) ;
      return rc ;

   error:
      closeListen( protocolMask ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__LISTENTCP, "_netFrame::_listenTCP" )
   INT32 _netFrame::_listenTCP( const CHAR *hostName,
                                const CHAR *serviceName )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETFRAME__LISTENTCP ) ;

      if ( _acceptor.is_open() )
      {
         rc = SDB_NET_ALREADY_LISTENED ;
         goto error ;
      }

      try
      {
         /// here we bind 0.0.0.0.
         tcp::resolver::query query( tcp::v4(), NET_LISTEN_HOST, serviceName ) ;
         tcp::resolver resolver( _mainSuitPtr->getIOService() ) ;
         tcp::resolver::iterator itr = resolver.resolve( query ) ;
         tcp::endpoint endpoint = *itr ;
         _acceptor.open( endpoint.protocol() ) ;
         _acceptor.set_option(tcp::acceptor::reuse_address(TRUE)) ;
         _acceptor.bind( endpoint ) ;
         _acceptor.listen() ;
      }
      catch ( boost::system::system_error & e )
      {
         PD_LOG ( PDERROR, "Failed to listen on %s:%s with TCP, error:%s",
                  hostName, serviceName, e.what() ) ;
         rc = SDB_NET_CANNOT_LISTEN ;
         goto error ;
      }

      /// set restart info before we launch asynchronous accept
      _restartTimer.setInfo( hostName, serviceName ) ;

      rc = _asyncAccept() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call async accept, rc: %d", rc ) ;

      OSS_BIT_SET( _protocolMask, NET_FRAME_MASK_TCP ) ;
      PD_LOG( PDDEBUG, "TCP listening on port %s", serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME__LISTENTCP, rc );
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__LISTENUDP, "_netFrame::_listenUDP" )
   INT32 _netFrame::_listenUDP( const CHAR *hostName,
                                const CHAR *serviceName,
                                INetUDPMsgHandler *handler,
                                UINT32 bufferSize )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETFRAME__LISTENUDP ) ;

      netUDPEndPoint localEndPoint ;

      if ( NULL != _udpMainSuit.get() && _udpMainSuit->isOpened() )
      {
         rc = SDB_NET_ALREADY_LISTENED ;
         goto error ;
      }

      if ( NULL == _udpMainSuit.get() )
      {
         // only one UDP socket
         NET_UDP_EV_SUIT suitPtr =
                              netUDPEventSuit::createShared( this,
                                                             _pRoute ) ;
         PD_CHECK( NULL != suitPtr.get(), SDB_OOM, error, PDERROR,
                   "Failed to allocate UDP event suit" ) ;

         // swap to smart pointer
         _udpMainSuit = suitPtr ;
      }

      rc = _udpMainSuit->listen( hostName, serviceName, handler, bufferSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to listen UDP on port %s, rc: %d",
                   serviceName, rc ) ;

      OSS_BIT_SET( _protocolMask, NET_FRAME_MASK_UDP ) ;
      PD_LOG( PDDEBUG, "UDP listening on port [%s], buffer [%u]",
              serviceName, bufferSize ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME__LISTENUDP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNNCCONN, "_netFrame::syncConnect" )
   INT32 _netFrame::syncConnect( const CHAR *hostName,
                                 const CHAR *serviceName,
                                 const _MsgRouteID &id,
                                 NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNNCCONN );

      NET_EH eh = netEventHandler::createShared(
                     _getEvSuit( TRUE ), (NET_HANDLE)( _handle.inc() ) ) ;
      if ( NULL == eh.get() )
      {
         PD_LOG ( PDERROR, "Failed to malloc mem" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = eh->syncConnect( hostName, serviceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      eh->id( id ) ;
      eh->asyncRead() ;

      /// add to map
      // addRoute will take latch inside the function
      _addRoute( eh ) ;
      _mtx.get() ;
      _opposite.insert( make_pair( eh->handle(), eh ) ) ;
      _mtx.release() ;

      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }

      // callback: handleConnect
      _handler->handleConnect( eh->handle(), id, TRUE ) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNNCCONN, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netFrame::syncConnect( const _MsgRouteID &id,
                                 NET_HANDLE *pHandle )
   {
      INT32 rc = SDB_OK ;
      CHAR host[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
      CHAR service[ OSS_MAX_SERVICENAME + 1] = { 0 } ;

      rc = _pRoute->route( id, host, OSS_MAX_HOSTNAME,
                           service, OSS_MAX_SERVICENAME ) ;
      if ( SDB_OK == rc )
      {
         rc = syncConnect( host, service, id, pHandle ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNNCCONN2, "_netFrame::syncConnect" )
   INT32 _netFrame::syncConnect( NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasConnect = FALSE ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNNCCONN2 ) ;

      eh->mtx().get() ;
      if ( !eh->isConnected() )
      {
         if ( eh->isNew() )
         {
            MsgRouteID id = eh->id() ;
            CHAR host[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
            CHAR service[ OSS_MAX_SERVICENAME + 1] = { 0 } ;

            rc = _pRoute->route( id, host, OSS_MAX_HOSTNAME,
                                 service, OSS_MAX_SERVICENAME ) ;
            if ( SDB_OK == rc )
            {
               rc = eh->syncConnect( host, service ) ;
               if ( SDB_OK == rc )
               {
                  hasConnect = TRUE ;
                  eh->asyncRead() ;
               }
            }

            if ( rc )
            {
               eh->close() ;
               _erase( eh->handle() ) ;
            }
         }
         else
         {
            // make sure the handle is erased
            _erase( eh->handle() ) ;

            rc = SDB_NETWORK ;
         }
      }
      eh->mtx().release() ;

      if ( rc )
      {
         goto error ;
      }
      if ( hasConnect )
      {
         // callback: handleConnect
         _handler->handleConnect( eh->handle(), eh->id(), TRUE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNNCCONN2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // This function is called when a sender needs an Event Handler(socket/connection)
   // to do the sending. The logic here is that it will first seach the appropriate
   // netEHSegment, which contains all the socket that assign to the same route
   // id(i.e. ip address and service). Then it will call getEH to get an
   // event handler. If netEHSegment can not be found, will create one.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__GETHANDLE, "_netFrame::_getHandle" )
   INT32 _netFrame::_getHandle( const _MsgRouteID &id, NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      MAP_ROUTE_IT itr ;
      netEHSegPtr ptr ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__GETHANDLE ) ;

      // protect exit of sub-network
      ossScopedRWLock scopeLock( &_suiteExitMutex, SHARED ) ;

      PD_CHECK( !_suiteStopFlag, SDB_QUIESCED, error, PDWARNING,
                "Suite service of net frame is stopped" ) ;

      _mtx.get_shared() ;
      itr = _route.find(id.value) ;

      if ( itr == _route.end() )
      {
         _mtx.release_shared() ;
         _mtx.get() ;
         // after we get the x latch, re-check if someone has already create
         // the netEHSegment
         itr = _route.find(id.value) ;
         if ( itr != _route.end())
         {
            ptr = itr->second ;
         }
         else
         {
            // create new netEHSegment
            ptr = netEHSegment::createShared( this, _maxSockPerNode, id ) ;
            if ( NULL == ptr.get() )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate netEHSegment failed" ) ;
               _mtx.release() ;
               goto error ;
            }

            // insert the shared ptr into route table
            _route.insert( make_pair(id.value, ptr) ) ;
         }
         _mtx.release() ;
      }
      else
      {
         // if we found the netEHSegment in the route table, just use it
         ptr = itr->second ;
         _mtx.release_shared() ;
      }
      // get event handler
      rc = ptr->getEH(eh) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME__GETHANDLE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const _MsgRouteID &id,
                              void *header,
                              NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL") ;
      SDB_ASSERT( MSG_INVALID_ROUTEID != id.value,
                  "id.value should not be zero" ) ;
      INT32 rc = SDB_OK ;
      MsgHeader *msgHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND );
      NET_EH eh ;

      rc = _getHandle( id, eh ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !eh->isConnected() )
      {
         rc = syncConnect( eh ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      msgHeader = ( MsgHeader* )header ;
      if ( MSG_INVALID_ROUTEID == msgHeader->routeID.value )
      {
         msgHeader->routeID = _local ;
      }
      eh->mtx().get() ;
      rc = eh->syncSend( msgHeader, msgHeader->messageLength ) ;
      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }
      eh->mtx().release() ;
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( msgHeader->messageLength ) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND2, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const NET_HANDLE &handle,
                              void *header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL") ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle,
                  "handle should not be invalid" ) ;
      INT32 rc = SDB_OK ;
      MsgHeader *msgHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND2 ) ;

      msgHeader = ( MsgHeader * )header ;
      if ( MSG_INVALID_ROUTEID == msgHeader->routeID.value )
      {
         msgHeader->routeID = _local ;
      }

      rc = syncSendRaw( handle, (const CHAR * )msgHeader,
                        msgHeader->messageLength ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND2, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netFrame::syncSendRaw( const NET_HANDLE &handle,
                                 const CHAR *pBuff,
                                 UINT32 buffSize )
   {
      SDB_ASSERT( NULL != pBuff, "pBuff should not be NULL") ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle,
                  "handle should not be invalid" ) ;
      INT32 rc = SDB_OK ;

      NET_EH eh ;
      MAP_EVENT_IT itr ;

      _mtx.get_shared() ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         _mtx.release_shared() ;
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itr->second ;
      _mtx.release_shared() ;

      eh->mtx().get() ;
      rc = eh->syncSend( pBuff, buffSize ) ;
      eh->mtx().release() ;
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }

      _netOut.add( buffSize ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND3, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const NET_HANDLE &handle,
                              MsgHeader *header,
                              const void *body,
                              UINT32 bodyLen )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL") ;
      SDB_ASSERT( NULL != body, "body should not be NULL") ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle,
                  "handle should not be invalid" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND3 );
      UINT32 headLen = header->messageLength - bodyLen ;
      NET_EH eh ;
      MAP_EVENT_IT itr ;

      _mtx.get_shared() ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         _mtx.release_shared() ;
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itr->second ;
      _mtx.release_shared() ;

      SDB_ASSERT( NET_EVENT_HANDLER_TCP == eh->getHandlerType(),
                  "Should not use UDP socket to send multiple packets" ) ;

      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }
      eh->mtx().get() ;
      /// header len should be computed. can not get sizeof(MsgHeader)
      rc = eh->syncSend( header, headLen ) ;
      if ( SDB_OK != rc )
      {
         eh->mtx().release() ;
         eh->close() ;
         goto error ;
      }
      _netOut.add( headLen ) ;
      rc = eh->syncSend( body, bodyLen ) ;
      eh->mtx().release() ;
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( bodyLen ) ;
   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND3, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netFrame::syncSendv( const NET_HANDLE & handle,
                               MsgHeader *header,
                               const netIOVec & iov )
   {
      SDB_ASSERT( NULL != header, "should not be NULL" ) ;
      SDB_ASSERT( NET_INVALID_HANDLE != handle, "invalid handle" ) ;

      INT32 rc = SDB_OK ;
      NET_EH eh ;
      MAP_EVENT_IT itHandle ;

      header->messageLength = sizeof( MsgHeader ) + netCalcIOVecSize( iov ) ;
      if ( header->messageLength > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Invalid msg size: %d", header->messageLength ) ;
         rc = SDB_INVALIDSIZE ;
         goto error ;
      }
      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }

      _mtx.get_shared() ;
      itHandle = _opposite.find( handle ) ;
      if ( _opposite.end() == itHandle )
      {
         _mtx.release_shared() ;
         rc = SDB_NET_INVALID_HANDLE ;
         goto error ;
      }
      eh = itHandle->second ;
      _mtx.release_shared() ;

      SDB_ASSERT( NET_EVENT_HANDLER_TCP == eh->getHandlerType(),
                  "Should not use UDP socket to send multiple packets" ) ;

      eh->mtx().get() ;
      rc = eh->syncSend( header, sizeof(MsgHeader) ) ;
      if ( SDB_OK != rc )
      {
         eh->mtx().release() ;
         eh->close() ;
         goto error ;
      }
      _netOut.add( sizeof(MsgHeader) ) ;

      for ( netIOVec::const_iterator itr = iov.begin() ; itr != iov.end();
            ++itr )
      {
         SDB_ASSERT( NULL != itr->iovBase, "should not be NULL" ) ;

         if ( itr->iovBase )
         {
            rc = eh->syncSend( itr->iovBase, itr->iovLen ) ;
            if ( SDB_OK != rc )
            {
               eh->mtx().release() ;
               eh->close() ;
               goto error ;
            }
            _netOut.add( itr->iovLen ) ;
         }
      }
      eh->mtx().release() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND4, "_netFrame::syncSend" )
   INT32 _netFrame::syncSend( const  _MsgRouteID &id,
                              MsgHeader *header,
                              const void *body,
                              UINT32 bodyLen,
                              NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header && NULL != body, "should not be NULL") ;
      SDB_ASSERT( MSG_INVALID_ROUTEID != id.value,
                  "id.value should not be zero" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND4 );
      UINT32 headLen = header->messageLength - bodyLen ;
      NET_EH eh ;

      rc = _getHandle( id, eh ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !eh->isConnected() )
      {
         rc = syncConnect( eh ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }
      eh->mtx().get() ;
      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }
      rc = eh->syncSend( header, headLen ) ;
      if ( SDB_OK != rc )
      {
         eh->mtx().release() ;
         eh->close() ;
         goto error ;
      }
      _netOut.add( headLen ) ;
      rc = eh->syncSend( body, bodyLen ) ;
      eh->mtx().release() ;
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( bodyLen ) ;
   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNCSEND4, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSENDV, "_netFrame::syncSendv" )
   INT32 _netFrame::syncSendv( const _MsgRouteID &id,
                               MsgHeader *header,
                               const netIOVec &iov,
                               NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header, "should not be NULL" ) ;
      SDB_ASSERT( MSG_INVALID_ROUTEID != id.value,
                  "id.value should not be zero" ) ;
      PD_TRACE_ENTRY( SDB__NETFRAME_SYNCSENDV ) ;
      INT32 rc = SDB_OK ;
      NET_EH eh ;

      header->messageLength = sizeof( MsgHeader ) + netCalcIOVecSize( iov ) ;
      if ( header->messageLength > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Invalid msg size: %d", header->messageLength ) ;
         rc = SDB_INVALIDSIZE ;
         goto error ;
      }
      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }

      rc = _getHandle( id, eh ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !eh->isConnected() )
      {
         rc = syncConnect( eh ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      eh->mtx().get() ;
      if ( pHandle )
      {
         *pHandle = eh->handle() ;
      }
      rc = eh->syncSend( header, sizeof(MsgHeader) ) ;
      if ( SDB_OK != rc )
      {
         eh->mtx().release() ;
         eh->close() ;
         goto error ;
      }
      _netOut.add( sizeof(MsgHeader) ) ;

      for ( netIOVec::const_iterator itr = iov.begin() ; itr != iov.end() ;
            ++itr )
      {
         SDB_ASSERT( NULL != itr->iovBase, "should not be NULL" ) ;

         if ( itr->iovBase && itr->iovLen > 0 )
         {
            rc = eh->syncSend( itr->iovBase, itr->iovLen ) ;
            if ( SDB_OK != rc )
            {
               eh->mtx().release() ;
               eh->close() ;
               goto error ;
            }
            _netOut.add( itr->iovLen ) ;
         }
      }
      eh->mtx().release() ;

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME_SYNCSENDV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSENDUDP, "_netFrame::syncSendUDP" )
   INT32 _netFrame::syncSendUDP( const MsgRouteID &id,
                                 void *header )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETFRAME_SYNCSENDUDP ) ;

      MsgHeader *message = (MsgHeader *)header ;
      netUDPEndPoint endPoint ;
      NET_EH eh ;

      rc = _pRoute->route( id, endPoint, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to route [ group: %d, node: %d, "
                   "service: %d ], rc: %d", id.columns.groupID,
                   id.columns.nodeID, id.columns.serviceID, rc ) ;

      PD_CHECK( NULL != _udpMainSuit.get() && _udpMainSuit->isOpened(),
                SDB_NET_INVALID_HANDLE, error, PDERROR,
                "Failed to send UDP message, UDP handle is invalid" ) ;

      rc = _udpMainSuit->getEH( endPoint, id, eh ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get event handler, rc: %d", rc ) ;

      PD_CHECK( NULL != eh.get() &&
                NET_EVENT_HANDLER_UDP == eh->getHandlerType(),
                SDB_NET_INVALID_HANDLE, error, PDERROR,
                "Failed to send UDP message, UDP handler is invalid" ) ;

      // set route ID if needed
      if ( MSG_INVALID_ROUTEID == message->routeID.value )
      {
         message->routeID.value = _local.value ;
      }

      rc = eh->syncSend( message, message->messageLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send message by UDP, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME_SYNCSENDUDP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // close all connections to one id(i.e. node)
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_CLOSE, "_netFrame::close" )
   void _netFrame::close( const _MsgRouteID &id )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_CLOSE );
      MAP_ROUTE_IT routeItr ;
      netEHSegPtr ptr ;

      _mtx.get_shared() ;
      routeItr = _route.find( id.value ) ;
      // check if the entry with corresponding id exists
      if ( routeItr != _route.end() )
      {
         ptr = routeItr->second ;
         _mtx.release_shared() ;
         // retrieve the netEHSegment shared ptr and release
         // s latch for the route table
         // call the netEHSEgment::close interface to close all
         // sockets in the netEHSEgment
         ptr->close() ;
      }
      else
      {
         _mtx.release_shared() ;
      }

      PD_TRACE_EXIT ( SDB__NETFRAME_CLOSE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_CLOSE2, "_netFrame::close" )
   // close all connections
   void _netFrame::close()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_CLOSE2 );
      MAP_EVENT_IT itr ;

      // protect exit of sub-network
      ossScopedRWLock scopeLock( &_suiteExitMutex, EXCLUSIVE ) ;

      _mtx.get_shared() ;
      itr = _opposite.begin() ;
      for ( ; itr != _opposite.end(); itr++ )
      {
         itr->second->close() ;
      }
      _mtx.release_shared() ;

      PD_TRACE_EXIT ( SDB__NETFRAME_CLOSE2 );
      return ;
   }

   INT32 _netFrame::closeListen( UINT32 protocolMask )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( OSS_BIT_TEST( protocolMask, NET_FRAME_MASK_TCP ) &&
              _acceptor.is_open() )
         {
            _acceptor.close() ;
            OSS_BIT_CLEAR( _protocolMask, NET_FRAME_MASK_TCP ) ;
         }
         if ( OSS_BIT_TEST( protocolMask, NET_FRAME_MASK_UDP ) &&
              NULL != _udpMainSuit.get() )
         {
            _udpMainSuit->close() ;
            OSS_BIT_CLEAR( _protocolMask, NET_FRAME_MASK_UDP ) ;
         }
      }
      catch( boost::system::system_error &e )
      {
         PD_LOG ( PDERROR, "Close listen occur error: %s,%d",
                  e.what(), e.code().value() ) ;
         rc = SDB_NETWORK ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__NETFRAME_CLOSE3, "_netFrame::close" )
   void _netFrame::close( const NET_HANDLE &handle,
                          MsgRouteID *pID )
   {
      PD_TRACE_ENTRY( SDB__NETFRAME_CLOSE3 ) ;
      MAP_EVENT_IT itr ;
      UINT64 routeID = MSG_INVALID_ROUTEID ;

      _mtx.get_shared() ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() != itr )
      {
         itr->second->close() ;
         routeID = itr->second->id().value ;
         _mtx.release_shared() ;
      }
      else
      {
         _mtx.release_shared() ;
         PD_LOG( PDINFO, "invalid net handle:%d", handle ) ;
      }

      if ( pID )
      {
         pID->value = routeID ;
      }

      PD_TRACE_EXIT( SDB__NETFRAME_CLOSE3 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_ADDTIMER, "_netFrame::addTimer" )
   INT32 _netFrame::addTimer( UINT32 millsec,
                              _netTimeoutHandler *handler,
                              UINT32 &timerid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__NETFRAME_ADDTIMER );

      timerid = NET_INVALID_TIMER_ID ;

      NET_TH timer = netTimer::createShared( millsec,
                                              ++ _timerID,
                                              _mainSuitPtr->getIOService(),
                                              handler ) ;
      PD_CHECK( NULL != timer.get(), SDB_OOM, error, PDERROR,
                "Allocate netTimer failed" ) ;

      /// lock
      _mtx.get() ;
      _timers.insert( std::make_pair( timer->id(), timer ) ) ;
      /// release
      _mtx.release() ;

      timerid = timer->id() ;
      timer->asyncWait() ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_ADDTIMER, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_REMTIMER, "_netFrame::removeTimer" )
   INT32 _netFrame::removeTimer( UINT32 id )
   {
      INT32 rc = SDB_OK ;
      MAP_TIMMER_IT it ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_REMTIMER ) ;

      _mtx.get() ;
      it = _timers.find( id ) ;
      if ( _timers.end() == it )
      {
         rc = SDB_NET_TIMER_ID_NOT_FOUND ;
      }
      else
      {
         it->second->cancel() ;
         _timers.erase( it ) ;
      }
      _mtx.release() ;

      PD_TRACE_EXITRC ( SDB__NETFRAME_REMTIMER, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_HNDMSG, "_netFrame::handleMsg" )
   void _netFrame::handleMsg( NET_EH eh )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_HNDMSG );
      INT32 rc = SDB_OK ;
      MsgHeader *pMsg = (_MsgHeader *)eh->msg() ;

      if ( MSG_HEARTBEAT == pMsg->opCode )
      {
         _handleHeartBeat( eh, pMsg ) ;
      }
      else if ( MSG_HEARTBEAT_RES == pMsg->opCode )
      {
         _handleHeartBeatRes( eh, pMsg ) ;
      }
      else
      {
         rc = _handler->handleMsg( eh->handle(), pMsg, eh->msg() ) ;
         _netIn.add( pMsg->messageLength ) ;
         if ( SDB_NET_BROKEN_MSG == rc )
         {
            eh->close() ;
         }
      }
      PD_TRACE1 ( SDB__NETFRAME_HNDMSG, PD_PACK_INT(rc) );
      PD_TRACE_EXIT ( SDB__NETFRAME_HNDMSG );
      return ;
   }

   void _netFrame::handleClose( NET_EH eh, _MsgRouteID id )
   {
      _handler->handleClose( eh->handle(), id ) ;
   }

   //TODO rewrite it later
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ADDRT, "_netFrame::_addRoute" )
   void _netFrame::_addRoute( NET_EH eh )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__ADDRT ) ;
      MAP_ROUTE_IT itr ;
      netEHSegPtr ptr ;

      _mtx.get_shared() ;
      itr = _route.find(eh->id().value) ;

      if ( itr == _route.end() )
      {
         _mtx.release_shared() ;
         _mtx.get() ;
         // after we get the x latch, re-check if someone has already create
         // the netEHSegment
         itr = _route.find(eh->id().value) ;
         if ( itr != _route.end())
         {
            ptr = itr->second ;
         }
         else
         {
            // create new netEHSegment
            ptr = netEHSegment::createShared( this,
                                              _maxSockPerNode,
                                              eh->id() ) ;
            if ( NULL == ptr.get() )
            {
               PD_LOG( PDERROR, "Allocate netEHSegment failed" ) ;
               _mtx.release() ;
               goto done ;
            }

            // insert the shared ptr into route table
            _route.insert( make_pair(eh->id().value, ptr) ) ;
         }
         _mtx.release() ;
      }
      else
      {
         // if we found the netEHSegment in the route table, just use it
         ptr = itr->second ;
         _mtx.release_shared() ;
      }
      // get event handler
      ptr->addEH(eh) ;

   done:
      PD_TRACE_EXIT ( SDB__NETFRAME__ADDRT );
   }

   void _netFrame::_eraseSuit_i( netEvSuitPtr &ptr )
   {
      VEC_EVSUIT_IT itr = _vecEvSuit.begin() ;
      while( itr != _vecEvSuit.end() )
      {
         if ( (*itr).get() == ptr.get() )
         {
            _vecEvSuit.erase( itr ) ;
            break ;
         }
         ++itr ;
      }
   }

   void _netFrame::_addOpposite( NET_EH eh )
   {
     _mtx.get() ;
     _opposite.insert( make_pair( eh->handle(), eh ) ) ;
     _mtx.release() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__GETEVSUIT, "_netFrame::_getEvSuit" )
   netEvSuitPtr _netFrame::_getEvSuit( BOOLEAN needLock )
   {
      netEvSuitPtr ptr = _mainSuitPtr ;
      UINT32 minSockNum = ptr->getHandleNum() ;
      BOOLEAN hasLock = FALSE ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__GETEVSUIT ) ;

      if ( _pThreadFunc && _maxSockPerThread > 0 &&
           minSockNum >= _maxSockPerThread )
      {
         VEC_EVSUIT_IT itr ;
         UINT32 curSockNum = 0 ;

         if ( needLock )
         {
            _suiteMtx.get() ;
            hasLock = TRUE ;
         }

         if ( _suiteStopFlag )
         {
            PD_LOG( PDWARNING, "Suite service of net frame is stopped" ) ;
            goto done ;
         }

         itr = _vecEvSuit.begin() ;
         while( itr != _vecEvSuit.end() )
         {
            curSockNum = (*itr)->getHandleNum() ;
            if ( curSockNum < minSockNum )
            {
               minSockNum = curSockNum ;
               ptr = (*itr) ;

               if ( minSockNum < _maxSockPerThread )
               {
                  /// find
                  goto done ;
               }
            }
            ++itr ;
         }

         /// when all suit's socket is >= _maxSockPerThread
         if ( _maxThreadNum > 0 && _vecEvSuit.size() < _maxThreadNum )
         {
            /// create new
            netEvSuitPtr suitPtr = netEventSuit::createShared( this ) ;
            if ( NULL != suitPtr.get() )
            {
               /// start thread
               INT32 rc = _pThreadFunc( suitPtr.get() ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Call _pThreadFunc failed, rc: %d", rc ) ;
                  goto done ;
               }

               ptr = suitPtr ;
               _vecEvSuit.push_back( ptr ) ;
            }
         }
      }

   done:
      if ( hasLock )
      {
         _suiteMtx.release() ;
      }
      PD_TRACE_EXIT( SDB__NETFRAME__GETEVSUIT ) ;
      return ptr ;
   }

   void _netFrame::_stopAllEvSuit()
   {
      _suiteMtx.get_shared() ;
      for ( UINT32 i = 0 ; i < _vecEvSuit.size() ; ++i )
      {
         _vecEvSuit[ i ]->stop() ;
      }
      _suiteMtx.release_shared() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ASYNCAPT, "_netFrame::_asyncAccept" )
   INT32 _netFrame::_asyncAccept()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__NETFRAME__ASYNCAPT ) ;

      netEventHandler *pEH = NULL ;
      NET_EH eh = netEventHandler::createShared(
                        _getEvSuit( TRUE ), (NET_HANDLE)( _handle.inc() ) ) ;
      if ( NULL == eh.get() )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate netEventHandler failed" ) ;
         goto error ;
      }

      pEH = (netEventHandler *)( eh.get() ) ;
      _acceptor.async_accept( pEH->socket(),
                              boost::bind( &_netFrame::_acceptCallback,
                                           this,
                                           eh,
                                           boost::asio::placeholders::error ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__NETFRAME__ASYNCAPT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__APTCALLBCK, "_netFrame::_acceptCallback" )
   void _netFrame::_acceptCallback( NET_EH eh,
                                    const boost::system::error_code &error )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__APTCALLBCK );
      if ( error )
      {
         PD_LOG ( PDERROR, "Accept connection occur exception: %s, %d",
                  error.message().c_str(), error.value() ) ;

         if ( boost::system::errc::too_many_files_open == error.value() ||
              boost::system::errc::too_many_files_open_in_system ==
              error.value() )
         {
            closeListen( NET_FRAME_MASK_TCP ) ;
            PD_LOG( PDERROR, "Can not accept more connections because of "
                    "open files upto limits, restart listening" ) ;
            _restartTimer.startTimer() ;
            pmdIncErrNum( SDB_TOO_MANY_OPEN_FD ) ;
         }

         goto done ;
      }

      eh->setOpt() ;

      /// add to map
      _mtx.get() ;
      _opposite.insert( make_pair( eh->handle(), eh ) ) ;
      _mtx.release() ;

      // callback: handleConnect
      _handler->handleConnect( eh->handle(), eh->id(), FALSE ) ;

      eh->asyncRead() ;
      _asyncAccept() ;

   done:
      PD_TRACE_EXIT ( SDB__NETFRAME__APTCALLBCK ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ERASE, "_netFrame::_erase" )
   void _netFrame::_erase( const NET_HANDLE &handle )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__ERASE );
      MsgRouteID removeUDPRouteID ;
      MAP_EVENT_IT itr ;
      MAP_ROUTE_IT routeItr ;

      removeUDPRouteID.value = MSG_INVALID_ROUTEID ;

      _mtx.get() ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         goto done ;
      }

      if ( NET_EVENT_HANDLER_TCP == itr->second->getHandlerType() )
      {
         // TCP handler
         routeItr = _route.find( itr->second->id().value ) ;
         if ( routeItr != _route.end() )
         {
            routeItr->second->delEH( handle ) ;
            /// when nobody used and is empty
            /// remove the route and also try to remove UDP event
            /// handler
            if ( routeItr->second->isEmpty() &&
                 1 == routeItr->second.refCount() )
            {
               _route.erase(routeItr) ;
               removeUDPRouteID.value = itr->second->id().value ;
            }
         }
      }

      _opposite.erase( itr ) ;

   done:
      _mtx.release() ;

      // if no TCP event handlers left for the given route
      // also remove UDP event handler
      if ( MSG_INVALID_ROUTEID != removeUDPRouteID.value &&
           NULL != _udpMainSuit.get() )
      {
         _udpMainSuit->removeEH( removeUDPRouteID ) ;
      }

      PD_TRACE_EXIT ( SDB__NETFRAME__ERASE );
   }

   INT64 _netFrame::netIn()
   {
      return _netIn.peek() ;
   }

   INT64 _netFrame::netOut()
   {
      return _netOut.peek() ;
   }

   void _netFrame::resetMon()
   {
      _netIn.poke( 0 ) ;
      _netOut.poke( 0 ) ;
   }

   /*
      Common function
   */
   UINT32 netCalcIOVecSize( const netIOVec &ioVec )
   {
      UINT32 size = 0 ;
      for ( UINT32 i = 0 ; i < ioVec.size() ; ++i )
      {
         if ( ioVec[ i ].iovBase )
         {
            size += ioVec[ i ].iovLen ;
         }
      }
      return size ;
   }

   NET_NODE_STATUS netResult2Status( INT32 result )
   {
      NET_NODE_STATUS status = NET_NODE_STAT_NORMAL ;

      switch ( result )
      {
         case SDB_CLS_FULL_SYNC:
            status = NET_NODE_STAT_FULLSYNC ;
            break ;
         case SDB_RTN_IN_REBUILD:
            status = NET_NODE_STAT_REBUILD ;
            break ;
         case SDB_RTN_IN_BACKUP:
            status = NET_NODE_STAT_BACKUP ;
            break ;
         case SDB_NETWORK:
         case SDB_NETWORK_CLOSE:
         case SDB_NET_CANNOT_CONNECT:
         case SDB_COORD_REMOTE_DISC:
         case SDB_INVALID_ROUTEID:
         case SDB_TIMEOUT:
         case SDB_DATABASE_DOWN:
            status = NET_NODE_STAT_OFFLINE ;
            break ;
         default:
            break ;
      }

      return status ;
   }

}
