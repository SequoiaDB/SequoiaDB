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

   Source File Name = clsSrcSelector.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSSRCSELECTOR_HPP_
#define CLSSRCSELECTOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "msg.hpp"
#include "clsDef.hpp"
#include <set>

using namespace std ;

namespace engine
{
   class _clsSyncManager ;
   class _clsReplicateSet ;
   class _clsNodeMgrAgent ;

   class _clsSrcSelector : public SDBObject
   {
   public:
      _clsSrcSelector() ;
      ~_clsSrcSelector() ;

   public:
      const MsgRouteID &getFullSyncSrc() ;

      const MsgRouteID &getSyncSrc() ;

      const MsgRouteID &selected( BOOLEAN isFullSync = FALSE ) ;

      const MsgRouteID &selectPrimary ( UINT32 groupID,
                                        MSG_ROUTE_SERVICE_TYPE type =
                                        MSG_ROUTE_SHARD_SERVCIE ) ;

      OSS_INLINE BOOLEAN addToBlackList( const MsgRouteID &id )
      {
         try
         {
            return _blacklist.insert( id.value ).second ;
         }
         catch( ... )
         {
         }
         return FALSE ;
      }

      OSS_INLINE void clearBlacklist()
      {
         _blacklist.clear() ;
      }

      OSS_INLINE void clearSrc()
      {
         _src.value = MSG_INVALID_ROUTEID ;
         _noRes = 0 ;
      }

      OSS_INLINE void timeout( UINT32 timeout )
      {
         _noRes += timeout ;
      }

      OSS_INLINE void clearTime()
      {
         _noRes = 0 ;
      }

      OSS_INLINE const MsgRouteID &src()
      {
         return _src ;
      }
   private:
      set<UINT64>       _blacklist ;
      _clsSyncManager   *_syncmgr ;
      MsgRouteID        _src ;
      UINT32            _noRes ;
      _clsNodeMgrAgent  *_nodeMgrAgent ;

   } ;
   typedef class _clsSrcSelector clsSrcSelector ;
}

#endif

