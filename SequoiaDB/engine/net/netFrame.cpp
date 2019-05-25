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
   #define NET_INNER_TIMER_INTERVAL       ( 2000 )
   #define NET_DUMMY_TIMER_INTERVAL       ( 2147483647 )

   #define NET_MBPS_MIN_VALUE             ( 102400 )     /// 100KB
   #define NET_MBPS_THRESHOLD             ( 5242880 )    /// 5MB

   /*
      _netInnerTimeHandle implement
   */
   _netInnerTimeHandle::_netInnerTimeHandle( _netFrame *pFrame )
   {
      _pFrame = pFrame ;
      _timeID = 0 ;
      _dummyTimerID = 0 ;
   }

   _netInnerTimeHandle::~_netInnerTimeHandle()
   {
   }

   void _netInnerTimeHandle::handleTimeout( const UINT32 &millisec,
                                            const UINT32 &id )
   {
      INT32 rc = SDB_OK ;

      if ( _timeID == id )
      {
         rc = _pFrame->listen( _hostName.c_str(), _svcName.c_str() ) ;
         if ( SDB_OK == rc || SDB_NET_ALREADY_LISTENED == rc )
         {
            _pFrame->removeTimer( _timeID ) ;
            PD_LOG( PDEVENT, "Restart listening on %s:%s succeed",
                    _hostName.c_str(), _svcName.c_str() ) ;
         }
      }
   }

   void _netInnerTimeHandle::setInfo( const CHAR *pHostName,
                                      const CHAR *pSvcName )
   {
      if ( _hostName.empty() )
      {
         _hostName = pHostName ;
      }
      if ( _svcName.empty() )
      {
         _svcName = pSvcName ;
      }
   }

   void _netInnerTimeHandle::startTimer()
   {
      INT32 rc = _pFrame->addTimer( NET_INNER_TIMER_INTERVAL,
                                    this, _timeID ) ;
      if ( rc )
      {
         PD_LOG( PDSEVERE, "Restore listen error when open files upto "
                 "limit, stop network, rc: %d", rc ) ;
         _pFrame->stop() ;
      }
   }

   INT32 _netInnerTimeHandle::startDummyTimer()
   {
      return _pFrame->addTimer( NET_DUMMY_TIMER_INTERVAL,
                                this, _dummyTimerID ) ;
   }

   #define NET_LISTEN_HOST          "0.0.0.0"

   /*
      _netFrame implement
   */
   _netFrame::_netFrame( _netMsgHandler *handler, _netRoute *pRoute )
   :_pRoute( pRoute ),
    _mainSuitPtr( SDB_OSS_NEW netEventSuit( this ) ),
    _handler( handler ),
    _acceptor( _mainSuitPtr->getIOService() ),
    _handle( 1 ),
    _timerID( NET_INVALID_TIMER_ID ),
    _netOut( 0 ),
    _netIn( 0 ),
    _innerTimeHandle( this )
   {
      _pThreadFunc = NULL ;
      _local.value = MSG_INVALID_ROUTEID ;
      _beatInterval = NET_HEARTBEAT_INTERVAL ;
      _beatTimeout = 0 ;
      _beatLastTick = pmdGetDBTick() ;
      _checkBeat = FALSE ;

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
      _maxThreadNum = maxThreadNum ;
   }

   void _netFrame::onRunSuitStart( netEvSuitPtr evSuitPtr )
   {
   }

   void _netFrame::onRunSuitStop( netEvSuitPtr evSuitPtr )
   {
      _mtx.get() ;
      _eraseSuit_i( evSuitPtr ) ;
      _mtx.release() ;

      _netEventSuit::SET_HANDLE setHandles = evSuitPtr->getHandles() ;
      _netEventSuit::SET_HANDLE_IT itr = setHandles.begin() ;
      while( itr != setHandles.end() )
      {
         close( *itr ) ;
         ++itr ;
      }
   }

   void _netFrame::onSuitTimer( netEvSuitPtr evSuitPtr )
   {
      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;

      if ( 0 == evSuitPtr->getHandleNum() )
      {
         evSuitPtr->stop() ;
         _eraseSuit_i( evSuitPtr ) ;
      }
   }

   UINT32 _netFrame::getEvSuitSize()
   {
      UINT32 size = 0 ;
      _mtx.get_shared() ;
      size = _vecEvSuit.size() ;
      _mtx.release_shared() ;

      return size ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_RUN, "_netFrame::run" )
   INT32 _netFrame::run( NET_START_THREAD_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_RUN ) ;

      _pThreadFunc = pFunc ;

      rc = _innerTimeHandle.startDummyTimer() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dummy timer failed, rc: %d", rc ) ;
         goto error ;
      }

      _mainSuitPtr->getIOService().run() ;

      _stopAllEvSuit() ;

      while( TRUE )
      {
         if ( getEvSuitSize() > 0 )
         {
            ossSleep( 200 ) ;
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
      closeListen() ;
      _mainSuitPtr->getIOService().stop() ;
      _stopAllEvSuit() ;
      close() ;
      PD_TRACE_EXIT ( SDB__NETFRAME_STOP );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_MAKESTAT, "_netFrame::makeStat" )
   void _netFrame::makeStat( UINT32 timeout )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_MAKESTAT ) ;
      NET_EH eh ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      MAP_EVENT_IT itr ;
      UINT64 curTick = pmdGetDBTick() ;

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

         eh->makeStat( curTick ) ;
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

         if ( pmdGetTickSpanTime( eh->getLastBeatTick() ) >= _beatInterval &&
              ( -1 == serviceType ||
                serviceType == eh->id().columns.serviceID ) )
         {
            eh->mtx().get() ;
            beat.requestID = eh->getAndIncMsgID() ;
            eh->syncSend( (const void*)&beat, beat.messageLength ) ;
            eh->syncLastBeatTick() ;
            eh->mtx().release() ;
         }
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

         spanTime = pmdGetTickSpanTime( eh->getLastRecvTick() ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_LISTEN, "_netFrame::listen" )
   INT32 _netFrame::listen( const CHAR *hostName,
                            const CHAR *serviceName )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_LISTEN );

      if ( _acceptor.is_open() )
      {
         rc = SDB_NET_ALREADY_LISTENED ;
         goto error ;
      }

      try
      {
         tcp::resolver::query query ( tcp::v4(), NET_LISTEN_HOST, serviceName ) ;
         tcp::resolver resolver ( _mainSuitPtr->getIOService() ) ;
         tcp::resolver::iterator itr = resolver.resolve ( query ) ;
         ip::tcp::endpoint endpoint = *itr ;
         _acceptor.open( endpoint.protocol() ) ;
         _acceptor.set_option(tcp::acceptor::reuse_address(TRUE)) ;
         _acceptor.bind( endpoint ) ;
         _acceptor.listen() ;
      }
      catch ( boost::system::system_error &e )
      {
         PD_LOG ( PDERROR, "Failed to listen on %s:%s, error:%s", hostName,
                  serviceName, e.what() ) ;
         rc = SDB_NET_CANNOT_LISTEN ;
         goto error ;
      }
      _innerTimeHandle.setInfo( hostName, serviceName ) ;

      rc = _asyncAccept() ;
      if ( rc )
      {
         goto error ;
      }

      PD_LOG( PDDEBUG, "listening on port %s", serviceName ) ;

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_LISTEN, rc );
      return rc ;
   error:
      closeListen() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNNCCONN, "_netFrame::syncConnect" )
   INT32 _netFrame::syncConnect( const CHAR *hostName,
                                 const CHAR *serviceName,
                                 const _MsgRouteID &id )
   {
      SDB_ASSERT( NULL != hostName, "hostName should not be NULL" ) ;
      SDB_ASSERT( NULL != serviceName, "serviceName should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNNCCONN );
      _netEventHandler *ev = SDB_OSS_NEW _netEventHandler( _getEvSuit( TRUE ),
                                                           _handle.inc() ) ;
      if ( NULL == ev )
      {
         PD_LOG ( PDERROR, "Failed to malloc mem" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      {
         NET_EH eh( ev ) ;
         rc = eh->syncConnect( hostName, serviceName ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         eh->id( id ) ;
         eh->asyncRead() ;

         _mtx.get() ;
         _opposite.insert( make_pair( eh->handle(), eh ) ) ;
         _route.insert( make_pair( eh->id().value, eh ) ) ;
         _mtx.release() ;

         _handler->handleConnect( eh->handle(), id, TRUE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNNCCONN, rc );
      return rc ;
   error:
      goto done ;
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
         }
         else
         {
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
         _handler->handleConnect( eh->handle(), eh->id(), TRUE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETFRAME_SYNNCCONN2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__GETHANDLE, "_netFrame::_getHandle" )
   INT32 _netFrame::_getHandle( const _MsgRouteID &id, NET_EH &eh )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__GETHANDLE ) ;

      MULMAP_ROUTE_IT_PAIR pitr ;
      UINT32 sockNum = 0 ;
      UINT32 minMBPS = 0 ;
      OSS_LATCH_MODE mode = SHARED ;
      UINT32 retryTimes = 0 ;

      while( ++retryTimes <= 2 )
      {
         sockNum = 0 ;
         ossScopedLock lock( &_mtx, mode ) ;

         pitr = _route.equal_range( id.value ) ;
         for ( MULMAP_ROUTE_IT mitr = pitr.first ; mitr != pitr.second ; ++mitr )
         {
            if ( 0 == sockNum )
            {
               eh = mitr->second ;
               minMBPS = eh->getMBPS() ;
            }
            else if ( mitr->second->getMBPS() < minMBPS )
            {
               eh = mitr->second ;
               minMBPS = eh->getMBPS() ;
            }

            ++sockNum ;

            if ( minMBPS <= NET_MBPS_MIN_VALUE ||
                 ( _maxSockPerNode > 0 && sockNum >= _maxSockPerNode ) )
            {
               break ;
            }
         }

         if ( sockNum > 0 && minMBPS <= NET_MBPS_THRESHOLD )
         {
            break ;
         }
         else if ( 1 == retryTimes )
         {
            mode = EXCLUSIVE ;
            continue ;
         }
         else
         {
            _netEventHandler *pEH = NULL ;
            pEH = SDB_OSS_NEW _netEventHandler( _getEvSuit( FALSE ),
                                                _handle.inc() ) ;
            if ( !pEH )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate netEventHandler failed" ) ;
               goto error ;
            }
            eh = NET_EH( pEH ) ;

            eh->id( id ) ;
            _opposite.insert( make_pair( eh->handle(), eh ) ) ;
            _route.insert( make_pair( eh->id().value, eh ) ) ;
         }
      }

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
      PD_TRACE_ENTRY ( SDB__NETFRAME_SYNCSEND2 );
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

      msgHeader = ( MsgHeader * )header ;
      if ( MSG_INVALID_ROUTEID == msgHeader->routeID.value )
      {
         msgHeader->routeID = _local ;
      }
      eh->mtx().get() ;
      rc = eh->syncSend( msgHeader, msgHeader->messageLength ) ;
      eh->mtx().release() ;
      if ( SDB_OK != rc )
      {
         eh->close() ;
         goto error ;
      }
      _netOut.add( msgHeader->messageLength ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_SYNCSEND3, "INT32 _netFrame::syncSend" )
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

      if ( MSG_INVALID_ROUTEID == header->routeID.value )
      {
         header->routeID = _local ;
      }
      eh->mtx().get() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_CLOSE, "_netFrame::close" )
   void _netFrame::close( const _MsgRouteID &id )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_CLOSE );
      MULMAP_ROUTE_IT_PAIR pitr ;

      _mtx.get_shared() ;
      pitr = _route.equal_range( id.value ) ;
      for ( MULMAP_ROUTE_IT mitr = pitr.first ; mitr != pitr.second ; ++mitr )
      {
         mitr->second->close() ;
      }
      _mtx.release_shared() ;
      PD_TRACE_EXIT ( SDB__NETFRAME_CLOSE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_CLOSE2, "_netFrame::close" )
   void _netFrame::close()
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME_CLOSE2 );
      MAP_EVENT_IT itr ;

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

   void _netFrame::closeListen ()
   {
      if ( _acceptor.is_open() )
      {
         _acceptor.close() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__NETFRAME_CLOSE3, "_netFrame::close" )
   void _netFrame::close( const NET_HANDLE &handle )
   {
      PD_TRACE_ENTRY( SDB__NETFRAME_CLOSE3 ) ;
      MAP_EVENT_IT itr ;

      _mtx.get_shared() ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() != itr )
      {
         itr->second->close() ;
         _mtx.release_shared() ;
      }
      else
      {
         _mtx.release_shared() ;
         PD_LOG( PDERROR, "invalid net handle:%d", handle ) ;
      }

      PD_TRACE_EXIT( SDB__NETFRAME_CLOSE3 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME_ADDTIMER, "_netFrame::addTimer" )
   INT32 _netFrame::addTimer( UINT32 millsec,
                              _netTimeoutHandler *handler,
                              UINT32 &timerid )
   {
      INT32 rc = SDB_OK ;
      _netTimer *t = NULL ;
      timerid = NET_INVALID_TIMER_ID ;
      NET_TH timer ;
      PD_TRACE_ENTRY ( SDB__NETFRAME_ADDTIMER );

      t = SDB_OSS_NEW _netTimer( millsec, ++_timerID,
                                 _mainSuitPtr->getIOService(),
                                 handler ) ;
      if ( !t )
      {
         PD_LOG( PDERROR, "Allocate netTimer failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      timer = NET_TH( t ) ;
      t = NULL ;

      _mtx.get() ;
      _timers.insert( std::make_pair( timer->id(), timer ) ) ;
      _mtx.release() ;

      timerid = timer->id() ;
      timer->asyncWait() ;

   done:
      if ( t )
      {
         SDB_OSS_DEL t ;
      }
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
         MsgOpReply reply ;
         reply.header.messageLength = sizeof( MsgOpReply ) ;
         reply.header.opCode = MSG_HEARTBEAT_RES ;
         reply.header.requestID = pMsg->requestID ;
         reply.header.routeID.value = 0 ;
         reply.header.TID = pMsg->TID ;
         reply.contextID = -1 ;
         reply.numReturned = 0 ;
         reply.startFrom = 0 ;
         reply.flags = pmdDBIsAbnormal() ? SDB_SYS : SDB_OK ;

         eh->mtx().get() ;
         reply.header.routeID = _local ;
         eh->syncSend( (const void*)&reply, reply.header.messageLength ) ;
         eh->mtx().release() ;
      }
      else if ( MSG_HEARTBEAT_RES == pMsg->opCode )
      {
         MsgOpReply *pReply = ( MsgOpReply* )pMsg ;
         if ( SDB_OK != pReply->flags )
         {
            PD_LOG( PDERROR, "Connection[Handle:%d, Node:%s] is broken "
                    "because of node is abnormal[%d]",
                    eh->handle(), routeID2String( eh->id() ).c_str(),
                    pReply->flags ) ;
            eh->close() ;
         }
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

   void _netFrame::handleClose( NET_EH eh , _MsgRouteID id)
   {
      _handler->handleClose( eh->handle(), id ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ADDRT, "_netFrame::_addRoute" )
   void _netFrame::_addRoute( NET_EH eh )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__ADDRT ) ;

      _mtx.get() ;
      _route.insert( make_pair( eh->id().value, eh ) ) ;
      _mtx.release() ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__GETEVSUIT, "_netFrame::_getEvSuit" )
   netEvSuitPtr _netFrame::_getEvSuit( BOOLEAN needLock )
   {
      netEventSuit *pSuit = NULL ;
      netEvSuitPtr ptr = _mainSuitPtr ;
      BOOLEAN hasLock = FALSE ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__GETEVSUIT ) ;

      if ( _pThreadFunc && _maxSockPerThread > 0 )
      {
         VEC_EVSUIT_IT itr ;
         UINT32 minSockNum = ptr->getHandleNum() ;
         UINT32 curSockNum = 0 ;

         if ( needLock )
         {
            _mtx.get() ;
            hasLock = TRUE ;
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
                  goto done ;
               }
            }
            ++itr ;
         }

         if ( 0 == _maxThreadNum || _vecEvSuit.size() < _maxThreadNum )
         {
            pSuit = SDB_OSS_NEW netEventSuit( this ) ;
            if ( pSuit )
            {
               INT32 rc = _pThreadFunc( pSuit ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Call _pThreadFunc failed, rc: %d", rc ) ;
                  goto done ;
               }

               ptr = netEvSuitPtr( pSuit ) ;
               pSuit = NULL ;
               _vecEvSuit.push_back( ptr ) ;
            }
         }
      }

   done:
      if ( hasLock )
      {
         _mtx.release() ;
      }
      if ( pSuit )
      {
         SDB_OSS_DEL pSuit ;
      }
      PD_TRACE_EXIT( SDB__NETFRAME__GETEVSUIT ) ;
      return ptr ;
   }

   void _netFrame::_stopAllEvSuit()
   {
      _mtx.get_shared() ;
      for ( UINT32 i = 0 ; i < _vecEvSuit.size() ; ++i )
      {
         _vecEvSuit[ i ]->stop() ;
      }
      _mtx.release_shared() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ASYNCAPT, "_netFrame::_asyncAccept" )
   INT32 _netFrame::_asyncAccept()
   {
      INT32 rc = SDB_OK ;
      _netEventHandler *pEH = NULL ;
      NET_EH eh ;
      PD_TRACE_ENTRY ( SDB__NETFRAME__ASYNCAPT ) ;

      pEH = SDB_OSS_NEW _netEventHandler( _getEvSuit( TRUE ), _handle.inc() ) ;
      if ( !pEH )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate netEventHandler failed" ) ;
         goto error ;
      }

      eh = NET_EH( pEH ) ;
      pEH = NULL ;

      _acceptor.async_accept( eh->socket(),
                              boost::bind( &_netFrame::_acceptCallback,
                                           this,
                                           eh,
                                           boost::asio::placeholders::error ) ) ;

   done:
      if ( pEH )
      {
         SDB_OSS_DEL pEH ;
      }
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
            closeListen() ;
            PD_LOG( PDERROR, "Can not accept more connections because of "
                    "open files upto limits, restart listening" ) ;
            _innerTimeHandle.startTimer() ;
            pmdIncErrNum( SDB_TOO_MANY_OPEN_FD ) ;
         }

         goto done ;
      }

      eh->setOpt() ;
      eh->asyncRead() ;

      _mtx.get() ;
      _opposite.insert( make_pair( eh->handle(), eh ) ) ;
      _mtx.release() ;

      _handler->handleConnect( eh->handle(), eh->id(), FALSE ) ;
      _asyncAccept() ;

   done:
      PD_TRACE_EXIT ( SDB__NETFRAME__APTCALLBCK ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETFRAME__ERASE, "_netFrame::_erase" )
   void _netFrame::_erase( const NET_HANDLE &handle )
   {
      PD_TRACE_ENTRY ( SDB__NETFRAME__ERASE );
      MAP_EVENT_IT itr ;
      MULMAP_ROUTE_IT_PAIR pitr ;

      _mtx.get() ;
      itr = _opposite.find( handle ) ;
      if ( _opposite.end() == itr )
      {
         goto done ;
      }

      pitr = _route.equal_range( itr->second->id().value ) ;
      for ( MULMAP_ROUTE_IT mitr = pitr.first ; mitr != pitr.second ; ++mitr )
      {
         if ( mitr->second->handle() == handle )
         {
            _route.erase( mitr ) ;
            break ;
         }
      }
      _opposite.erase( itr ) ;

   done:
      _mtx.release() ;
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
            status = NET_NODE_STAT_OFFLINE ;
            break ;
         default:
            break ;
      }

      return status ;
   }

}

