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

   Source File Name = clsSrcSelectTest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/22/2022  HYQ  Initial Draft

   Last Changed =

******************************************************************************/

#include"ossTypes.hpp"
#include<gtest/gtest.h>

#include "clsSyncManager.hpp"
#include "netRouteAgent.hpp"
#include "netMsgHandler.hpp"
#include "pmdEnv.hpp"

#include <iostream>

namespace engine
{
   class myHandler : public _netMsgHandler
   {
   public:
      myHandler(){}
      virtual ~myHandler(){}
      virtual INT32 handleMsg( const NET_HANDLE &handle,
                              const _MsgHeader *header,
                              const CHAR *msg )
      {
         return SDB_OK ;
      }
   } ;

   /*
   Name: src_select_test1
   Description:
    sync source selected
   Expected Result:
      sync source selected order :
      1.location primary peer -> location secondary peer -> location secondary rc
      2.affinity location primary peer ->affinity location secondary peer
        -> affinity location secondary peer
      3.group primary peer -> group secondary peer -> group secondary rc
   */
   TEST(clsSrcSelectTest, src_select_test1)
   {
      _clsGroupInfo info ;
      myHandler handler ;
      _netRouteAgent agent( &handler ) ;
      _clsSharingStatus status ;
      status.beat.beatVersion = CLS_BEAT_VERSION_2 ;
      MsgRouteID id ;
      id.columns.groupID = 1001 ;
      id.columns.nodeID = 1 ;
      const UINT32 nodeSize = 9;
      UINT64 res[ nodeSize ] ;

      // 0 - 2 location node
      // 3 - 6 affinity node
      // 6 - 9 group node

      // prepare node status environment
      for ( UINT64 i = 0; i < nodeSize; i++ )
      {
         if ( i < 3 )
         {
            status.beat.locationID = 1 ;
         }
         else if ( i < 6 )
         {
            status.isAffinitiveLocation = TRUE ;
         }

         if ( ( i + 1 ) % 3 == 0 )
         {
            status.beat.syncStatus = CLS_SYNC_STATUS_RC ;
         }
         else
         {
            status.beat.syncStatus = CLS_SYNC_STATUS_PEER ;
         }

         if ( ( i + 1 ) % 3 == 1 )
         {
            status.beat.locationRole = CLS_GROUP_ROLE_PRIMARY ;
         }
         else
         {
            status.beat.locationRole = CLS_GROUP_ROLE_SECONDARY ;
         }
         if ( i == 6 )
         {
            status.beat.role = CLS_GROUP_ROLE_PRIMARY ;
         }
         else
         {
            status.beat.role = CLS_GROUP_ROLE_SECONDARY ;
         }
         info.info.insert( std::make_pair( id.value, status ) ) ;
         info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
         res[i] = id.value ;
         //std ::cout << res[i]<< endl;
         ++id.columns.nodeID ;
      }

      // my location
      info.primary.value = res[6] ;
      _clsSyncManager sync( &agent, &info ) ;
      set<UINT64> blacklist ;
      CLS_GROUP_VERSION version = 1 ;

      for ( UINT64 i = 3; i < nodeSize ; i++ )
      {
         blacklist.insert( res[i] ) ;
      }

      // location range
      MsgRouteID ret ;
      BOOLEAN isLoc = TRUE ;
      // 1. location primary peer
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[0] ) ;
      //std ::cout << ret.value<< endl;
      info.alives.erase( ret.value ) ;

      // 2. location secondary peer
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[1] ) ;
      info.alives.erase( ret.value ) ;

      // 3. location secondary rc
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[2] ) ;
      info.alives.erase( ret.value ) ;

      blacklist.clear() ;
      for ( UINT64 i = 6; i < nodeSize ; i++ )
      {
         blacklist.insert( res[i] ) ;
      }

      // affinity range
      // 1. affinity location primary peer
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[3] ) ;
      info.alives.erase( ret.value ) ;
      // 2. affinity location secondary peer
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[4] ) ;
      info.alives.erase( ret.value ) ;
      // 3. affinity location secondary rc
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[5] ) ;
      info.alives.erase( ret.value ) ;

      blacklist.clear() ;
      for ( UINT64 i = 0; i < 6 ; i++ )
      {
         blacklist.insert( res[i] ) ;
      }

      isLoc = FALSE ;
      // group range
      // 1. group primary peer
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[6] ) ;
      info.alives.erase( ret.value ) ;
      // 2. secondary peer
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[7] ) ;
      info.alives.erase( ret.value ) ;
      // 3. secondary rc
      ret.value = sync.getSyncSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[8] ) ;
      info.alives.erase( ret.value ) ;

   }

   /*
   Name: src_select_test2
   Description:
      full sync source select
   Expected Result:
      location secondary -> location primary
      affinity location secondary -> affinity location primary
      group secondary -> group primary
   */
   TEST(clsSrcSelectTest, src_select_test2)
   {
      _clsGroupInfo info ;
      myHandler handler ;
      _netRouteAgent agent( &handler ) ;
      _clsSharingStatus status ;
      MsgRouteID id ;
      id.columns.groupID = 1001 ;
      id.columns.nodeID = 1 ;
      const UINT32 nodeSize = 6 ;
      UINT64 res[ nodeSize ] ;
      CLS_GROUP_VERSION version = 1 ;

      // make sure the node normal
      status.beat.endLsn.offset = 100 ;
      status.beat.nodeRunStat = CLS_NODE_RUNNING ;
      status.beat.serviceStatus = SERVICE_NORMAL ;
      status.beat.ftConfirmStat = 0 ;

      // 0 - 2 location node
      // 3 - 6 affinity node
      // 6 - 9 group node

      // prepare node status environment
      for ( UINT64 i = 0; i < nodeSize; i++ )
      {
         if ( i < 2 )
         {
            status.beat.locationID = 1 ;
         }
         else if ( i < 4 )
         {
            status.isAffinitiveLocation = TRUE ;
         }

         if ( ( i + 1 ) % 2 == 0 )
         {
            status.beat.locationRole = CLS_GROUP_ROLE_PRIMARY ;
         }
         else
         {
            status.beat.locationRole = CLS_GROUP_ROLE_SECONDARY ;
         }

         if ( i == 5 )
         {
            status.beat.role = CLS_GROUP_ROLE_PRIMARY ;
         }
         info.info.insert( std::make_pair( id.value, status ) ) ;
         info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
         res[i] = id.value ;
         //std ::cout << res[i]<< endl;
         ++id.columns.nodeID ;
      }

      // my location
      info.localLocationID = 1 ;
      BOOLEAN isLoc = TRUE ;
      _clsSyncManager sync( &agent, &info ) ;
      set<UINT64> blacklist ;
      MsgRouteID ret ;

      for ( UINT64 i = 2; i < nodeSize ; i++ )
      {
         blacklist.insert( res[i] ) ;
      }

      // location range
      // 1. location secondary
      ret.value = sync.getFullSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[0] ) ;
      info.alives.erase( res[0] ) ;

      // 2. location primary
      ret.value = sync.getFullSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[1] )  ;
      info.alives.erase( res[1] ) ;

      blacklist.clear() ;
      for ( UINT64 i = 4; i < nodeSize ; i++ )
      {
         blacklist.insert( res[i] ) ;
      }

      // affinity location range
      // 1. affinity location secondary
      ret.value = sync.getFullSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[2] ) ;
      info.alives.erase( res[2] ) ;

      // 2. affinity location primary
      ret.value = sync.getFullSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[3] ) ;
      info.alives.erase( res[3] ) ;

      blacklist.clear() ;
      for ( UINT64 i = 0; i < 4 ; i++ )
      {
         blacklist.insert( res[i] ) ;
      }

      isLoc = FALSE ;
      // group range
      // 1. group secondary
      ret.value = sync.getFullSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[4] ) ;
      info.alives.erase( res[4] ) ;

      // 2. group primary
      ret.value = sync.getFullSrc( blacklist, isLoc, version ).value ;
      EXPECT_TRUE( ret.value == res[5] ) ;
      info.alives.erase( res[5] ) ;

   }

}