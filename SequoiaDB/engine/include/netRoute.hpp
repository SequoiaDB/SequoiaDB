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

      private:
         map<UINT64, _netRouteNode> _route ;
         _MsgRouteID _local ;
         _ossSpinSLatch _mtx ;
   };
}

#endif

