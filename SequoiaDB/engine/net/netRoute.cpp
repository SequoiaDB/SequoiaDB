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

   Source File Name = netRoute.cpp

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

#include "netRoute.hpp"
#include "ossUtil.hpp"
#include "clsUtil.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"

using namespace boost::asio::ip ;

namespace engine
{

   _netRoute::~_netRoute()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_ROUTE, "_netRoute::route" )
   INT32 _netRoute::route( const _MsgRouteID &id,
                           CHAR *host, UINT32 hostLen,
                           CHAR *service, UINT32 svcLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRT_ROUTE );
      _MsgRouteID tmp = id ;
      tmp.columns.serviceID = 0 ;
      _mtx.get_shared() ;
      NET_ROUTE_MAP::const_iterator itr = _route.find( tmp.value ) ;
      if ( _route.end() == itr )
      {
         rc = SDB_NET_ROUTE_NOT_FOUND ;
         goto error ;
      }
      else if ( ((itr->second._service)[id.columns.serviceID]).empty() )
      {
         rc = SDB_NET_ROUTE_NOT_FOUND ;
         goto error ;
      }
      else
      {
         /// set des len with local value.
         ossMemset( host, 0, hostLen ) ;
         ossStrncpy( host, itr->second._host, hostLen - 1 ) ;
         ossMemset( service, 0, svcLen ) ;
         ossStrncpy( service,
                     ((itr->second._service)[id.columns.serviceID]).c_str(),
                     svcLen - 1 ) ;
      }
   done:
      _mtx.release_shared() ;
      PD_TRACE_EXITRC ( SDB__NETRT_ROUTE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_ROUTE2, "_netRoute::route" )
   INT32 _netRoute::route( const _MsgRouteID &id,
                           _netRouteNode &node )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRT_ROUTE2 );
      _MsgRouteID tmp = id ;
      tmp.columns.serviceID = 0 ;
      _mtx.get_shared() ;
      NET_ROUTE_MAP::const_iterator itr = _route.find( tmp.value ) ;
      if ( _route.end() == itr )
      {
         rc = SDB_NET_ROUTE_NOT_FOUND ;
         goto error ;
      }
      else
      {
         node = itr->second ;
      }

   done:
      _mtx.release_shared() ;
      PD_TRACE_EXITRC ( SDB__NETRT_ROUTE2, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netRoute::route( const CHAR *host, const CHAR *service,
                           MSG_ROUTE_SERVICE_TYPE type, _MsgRouteID &id )
   {
      INT32 rc = SDB_NET_ROUTE_NOT_FOUND ;
      _mtx.get_shared() ;
      NET_ROUTE_MAP::const_iterator itr = _route.begin() ;
      while( itr != _route.end() )
      {
         const _netRouteNode &nodeInfo = itr->second ;
         if ( 0 == ossStrcmp( nodeInfo._host, host ) &&
              0 == ossStrcmp( nodeInfo._service[ type ].c_str(), service ) )
         {
            rc = SDB_OK ;
            id.value = itr->first ;
            break ;
         }
         ++itr ;
      }

      _mtx.release_shared() ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_ROUTE_UDP, "_netRoute::route" )
   INT32 _netRoute::route( const MsgRouteID &routeID,
                           netUDPEndPoint &endPoint,
                           BOOLEAN needCache )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETRT_ROUTE_UDP ) ;

      ossScopedLock scopedLock( &_mtx, SHARED ) ;

      NET_UDP_EP_MAP::const_iterator udpIter = _udpRoute.find( routeID.value ) ;
      if ( udpIter != _udpRoute.end() )
      {
         endPoint = udpIter->second ;
      }
      else
      {
         UINT16 serviceID = routeID.columns.serviceID ;
         MsgRouteID tmpID = routeID ;
         tmpID.columns.serviceID = 0 ;
         NET_ROUTE_MAP::const_iterator routeIter = _route.find( tmpID.value ) ;
         PD_CHECK( routeIter != _route.end(), SDB_NET_ROUTE_NOT_FOUND, error,
                   PDWARNING, "Failed to find route for [ group: %u, node: %u, "
                   "service: %u ]", routeID.columns.groupID,
                   routeID.columns.nodeID, routeID.columns.serviceID ) ;
         PD_CHECK( '\0' != routeIter->second._host[ 0 ],
                   SDB_NET_ROUTE_NOT_FOUND, error, PDWARNING,
                   "Failed to find route for [ group: %u, node: %u, "
                   "service: %u ]: host is not found", routeID.columns.groupID,
                   routeID.columns.nodeID, routeID.columns.serviceID ) ;
         PD_CHECK( !( routeIter->second._service[ serviceID ].empty() ),
                   SDB_NET_ROUTE_NOT_FOUND, error, PDWARNING,
                   "Failed to find route for [ group: %u, node: %u, "
                   "service: %u ]: service is not found",
                   routeID.columns.groupID, routeID.columns.nodeID,
                   routeID.columns.serviceID ) ;
         rc = getUDPEndPoint( routeIter->second._host,
                              routeIter->second._service[ serviceID ].c_str(),
                              endPoint ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get UDP end point for "
                      "[ group: %u, node: %u, service: %u ]: address %s:%s "
                      "could not be resolved, rc: %d", routeID.columns.groupID,
                      routeID.columns.nodeID,routeID.columns.serviceID,
                      routeIter->second._host,
                      routeIter->second._service[ serviceID ].c_str(), rc ) ;

         if ( needCache )
         {
            _udpRoute.insert( make_pair( routeID.value, endPoint ) ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__NETRT_ROUTE_UDP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_UPDATE, "_netRoute::update" )
   INT32 _netRoute::update( const _MsgRouteID &id,
                            const CHAR *host,
                            const CHAR *service,
                            BOOLEAN *newAdd )
   {
      SDB_ASSERT( NULL != host, "host should not be NULL" ) ;
      SDB_ASSERT( NULL != service, "host should not be NULL" ) ;
      SDB_ASSERT( id.columns.serviceID < MSG_ROUTE_SERVICE_TYPE_MAX,
                  "service ID should < MSG_ROUTE_SERVICE_TYPE_MAX " ) ;
      INT32 rc = SDB_NET_UPDATE_EXISTING_NODE ;
      PD_TRACE_ENTRY ( SDB__NETRT_UPDATE );
      _MsgRouteID tmp = id ;
      tmp.columns.serviceID = 0 ;
      _mtx.get() ;
      _netRouteNode &node = _route[tmp.value] ;

      if ( newAdd )
      {
         if ( MSG_INVALID_ROUTEID == node._id.value &&
              0 == node._host[0] )
         {
            *newAdd = TRUE ;
         }
         else
         {
            *newAdd = FALSE ;
         }
      }

      if ( node._id.value != tmp.value )
      {
         node._id.value = tmp.value ;
      }
      if ( 0 != ossStrcmp( host, node._host ) )
      {
         ossMemset( node._host, 0, sizeof( node._host ) ) ;
         ossStrncpy( node._host, host, sizeof( node._host ) - 1 ) ;
         rc = SDB_OK ;
      }
      if ( 0 != ossStrcmp( service,
           ((node._service)[id.columns.serviceID]).c_str() ) )
      {
         (node._service)[id.columns.serviceID] = string( service ) ;
         rc = SDB_OK ;
      }
      _clearUDPRoute( id, FALSE ) ;
      _mtx.release() ;

      PD_TRACE_EXITRC ( SDB__NETRT_UPDATE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_UPDATE2, "_netRoute::update" )
   INT32 _netRoute::update ( const _MsgRouteID &oldID,
                             const _MsgRouteID &newID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRT_UPDATE2 );
      NET_ROUTE_MAP::iterator it ;
      _MsgRouteID oldTmp = oldID ;
      oldTmp.columns.serviceID = 0 ;
      _MsgRouteID newTmp = newID ;
      newTmp.columns.serviceID = 0 ;
      _mtx.get() ;
      it = _route.find ( oldTmp.value ) ;
      if ( _route.end() == it )
      {
         PD_LOG ( PDERROR, "Cannot find route id during update: %lld",
                  oldTmp.value ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( oldTmp.value != newTmp.value )
      {
         it->second._id.value = newTmp.value ;
         _route[newTmp.value] = it->second ;
         _route.erase ( oldTmp.value ) ;
      }

      _clearUDPRoute( oldID, TRUE ) ;

   done :
      _mtx.release () ;
      PD_TRACE_EXITRC ( SDB__NETRT_UPDATE2, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_UPDATE3, "_netRoute::update" )
   INT32 _netRoute::update( const _MsgRouteID &id,
                            const _netRouteNode &node,
                            BOOLEAN *newAdd )
   {
      INT32 rc = SDB_NET_UPDATE_EXISTING_NODE ;
      PD_TRACE_ENTRY ( SDB__NETRT_UPDATE3 );
      _MsgRouteID tmp = id ;
      tmp.columns.serviceID = 0 ;
      _mtx.get() ;
      _netRouteNode &update = _route[tmp.value] ;

      if ( newAdd )
      {
         if ( MSG_INVALID_ROUTEID == update._id.value &&
              0 == node._host[0] )
         {
            *newAdd = TRUE ;
         }
         else
         {
            *newAdd = FALSE ;
         }
      }

      if ( update._id.value != tmp.value )
      {
         update._id.value = tmp.value ;
      }
      if ( 0 != ossStrcmp( node._host, update._host ) )
      {
         ossMemset( update._host, 0, sizeof( update._host ) ) ;
         ossStrncpy( update._host, node._host, sizeof( update._host ) -1 ) ;
         rc = SDB_OK ;
      }
      for ( UINT32 i = 0; i < MSG_ROUTE_SERVICE_TYPE_MAX; i++ )
      {
         if ( (update._service)[i] != (node._service)[i] )
         {
            (update._service)[i] = (node._service)[i];
            rc = SDB_OK ;
         }
      }

      _clearUDPRoute( id, TRUE ) ;

      _mtx.release() ;
      PD_TRACE_EXITRC ( SDB__NETRT_UPDATE3, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_CLEAR, "_netRoute::clear" )
   void _netRoute::clear()
   {
      PD_TRACE_ENTRY ( SDB__NETRT_CLEAR );
      _mtx.get() ;
      _route.clear() ;
      _udpRoute.clear() ;
      _mtx.release() ;
      PD_TRACE_EXIT ( SDB__NETRT_CLEAR );
   }

   void _netRoute::del( const _MsgRouteID &id, BOOLEAN &hasDel )
   {
      hasDel = FALSE ;
      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;
      NET_ROUTE_MAP::iterator it = _route.find( id.value ) ;
      if ( it != _route.end() )
      {
         _route.erase( it ) ;
         hasDel = TRUE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_GETUDPEP, "_netRoute::getUDPEndPoint" )
   INT32 _netRoute::getUDPEndPoint( const CHAR *hostName,
                                    const CHAR *serviceName,
                                    netUDPEndPoint &endPoint )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETRT_GETUDPEP ) ;

      try
      {
         boost::asio::io_service ioService ;
         udp::resolver::query query( udp::v4(), hostName, serviceName ) ;
         udp::resolver resolver( ioService ) ;
         udp::resolver::iterator iter = resolver.resolve( query ) ;
         udp::resolver::iterator end ;
         PD_CHECK( iter != end, SDB_NET_ROUTE_NOT_FOUND, error, PDERROR,
                   "Failed to resolve UDP %s:%s", hostName, serviceName ) ;
         endPoint = ( *iter ) ;
      }
      catch ( boost::system::system_error &e )
      {
         PD_LOG ( PDERROR, "Failed to resolve UDP %s:%s, error:%s", hostName,
                  serviceName, e.what() ) ;
         rc = SDB_NET_ROUTE_NOT_FOUND ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETRT_GETUDPEP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRT_GETTCPEP, "_netRoute::getTCPEndPoint" )
   INT32 _netRoute::getTCPEndPoint( const CHAR *hostName,
                                    const CHAR *serviceName,
                                    netTCPEndPoint &endPoint )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__NETRT_GETUDPEP ) ;

      try
      {
         boost::asio::io_service ioService ;
         tcp::resolver::query query( tcp::v4(), hostName, serviceName ) ;
         tcp::resolver resolver( ioService ) ;
         tcp::resolver::iterator iter = resolver.resolve( query ) ;
         tcp::resolver::iterator end ;
         PD_CHECK( iter != end, SDB_NET_ROUTE_NOT_FOUND, error, PDERROR,
                   "Failed to resolve TCP %s:%s", hostName, serviceName ) ;
         endPoint = ( *iter ) ;
      }
      catch ( boost::system::system_error &e )
      {
         PD_LOG ( PDERROR, "Failed to resolve TCP %s:%s, error:%s", hostName,
                  serviceName, e.what() ) ;
         rc = SDB_NET_ROUTE_NOT_FOUND ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETRT_GETUDPEP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _netRoute::clearUDPRoute( const MsgRouteID &routeID,
                                  BOOLEAN allServices )
   {
      ossScopedLock( &_mtx, EXCLUSIVE ) ;
      _clearUDPRoute( routeID, allServices ) ;
   }

   void _netRoute::_clearUDPRoute( const MsgRouteID &routeID,
                                   BOOLEAN allServices )
   {
      if ( allServices )
      {
         MsgRouteID tmpID = routeID ;
         for ( UINT32 i = 0 ; i < MSG_ROUTE_SERVICE_TYPE_MAX ; ++ i )
         {
            tmpID.columns.serviceID = (UINT16)i ;
            _udpRoute.erase( tmpID.value ) ;
         }
      }
      else
      {
         _udpRoute.erase( routeID.value ) ;
      }
   }

}
