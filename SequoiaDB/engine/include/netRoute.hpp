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

   Source File Name = netRoute.hpp

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

#ifndef NETROUTE_HPP_
#define NETROUTE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "netDef.hpp"
#include <map>

using namespace std ;

namespace engine
{

   class _netRoute : public SDBObject
   {
      public:
         ~_netRoute() ;
         INT32 route( const _MsgRouteID &id,
                      CHAR *host, UINT32 hostLen,
                      CHAR *service, UINT32 svcLen ) ;

         INT32 route( const _MsgRouteID &id,
                      _netRouteNode &node ) ;

         INT32 route( const CHAR* host,
                      const CHAR* service,
                      MSG_ROUTE_SERVICE_TYPE type,
                      _MsgRouteID &id ) ;

         INT32 route( const MsgRouteID &routeID,
                      netUDPEndPoint &endPoint,
                      BOOLEAN needCache = TRUE ) ;

         /// return err when update an existing node.
         INT32 update( const _MsgRouteID &id,
                       const CHAR *host,
                       const CHAR *service,
                       BOOLEAN *newAdd = NULL ) ;
         INT32 update( const _MsgRouteID &id,
                       const _netRouteNode &node,
                       BOOLEAN *newAdd = NULL ) ;
         INT32 update( const _MsgRouteID &oldID,
                       const _MsgRouteID &newID ) ;

         void  del( const _MsgRouteID &id, BOOLEAN &hasDel ) ;

         void  clear() ;

         OSS_INLINE void setLocal( const _MsgRouteID &id )
         {
            _local = id ;
         }

         OSS_INLINE const _MsgRouteID &local()
         {
            return _local ;
         }

         static INT32 getUDPEndPoint( const CHAR *hostName,
                                       const CHAR *serviceName,
                                       netUDPEndPoint &endPoint ) ;
         static INT32 getTCPEndPoint( const CHAR *hostName,
                                      const CHAR *serviceName,
                                      netTCPEndPoint &endPoint ) ;

         void clearUDPRoute ( const MsgRouteID &routeID,
                              BOOLEAN allServices ) ;

      protected :
         // in-lock
         void _clearUDPRoute( const MsgRouteID &routeID,
                              BOOLEAN allServices ) ;

      protected :
         NET_ROUTE_MAP     _route ;
         NET_UDP_EP_MAP    _udpRoute ;
         MsgRouteID        _local ;
         ossSpinSLatch     _mtx ;
   } ;

   typedef class _netRoute netRoute ;

}

#endif

