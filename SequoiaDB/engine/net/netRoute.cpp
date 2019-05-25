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
      map<UINT64, _netRouteNode>::const_iterator itr =
                                  _route.find( tmp.value ) ;
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
      map<UINT64, _netRouteNode>::const_iterator itr =
                                  _route.find( tmp.value ) ;
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
      map<UINT64, _netRouteNode>::const_iterator itr = _route.begin() ;
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
      map<UINT64, _netRouteNode>::iterator it ;
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
      _mtx.release() ;
      PD_TRACE_EXIT ( SDB__NETRT_CLEAR );
   }

   void _netRoute::del( const _MsgRouteID &id, BOOLEAN &hasDel )
   {
      hasDel = FALSE ;
      ossScopedLock lock( &_mtx, EXCLUSIVE ) ;
      map<UINT64, _netRouteNode>::iterator it = _route.find( id.value ) ;
      if ( it != _route.end() )
      {
         _route.erase( it ) ;
         hasDel = TRUE ;
      }
   }

}
