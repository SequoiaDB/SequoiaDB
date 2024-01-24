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
*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>

#include "clsSyncMinHeap.hpp"
#include "clsSyncManager.hpp"
#include "netRouteAgent.hpp"
#include "netMsgHandler.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEnv.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <boost/thread.hpp>

using namespace std;
using namespace engine;

TEST(clsTest, clsHeap_1)
{
   _clsSyncMinHeap heap ;
   _clsSyncSession session ;
   UINT32 num = 10;
   INT32 i = num ;
   /// push 9 - 0
   while ( i-- >  0 )
   {
      session.waitPlan.offset = i ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }

   ASSERT_TRUE( num == heap.dataSize() ) ;

   /// pop 0 - 9
   for ( UINT32 j = 0; j < num; j++ )
   {
      ASSERT_TRUE( SDB_OK == heap.root( session )) ;
      ASSERT_TRUE(session.waitPlan.offset == j) ;
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE(session.waitPlan.offset == j);
   }
   ASSERT_TRUE( 0 == heap.dataSize() ) ;

   /// push 0 - 9
   for ( UINT32 j = 0; j < num; j++ )
   {
      session.waitPlan.offset = j ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }
   ASSERT_TRUE( num == heap.dataSize() ) ;

   /// pop 0 - 9
   for ( UINT32 j = 0; j < num; j++ )
   {
      ASSERT_TRUE( SDB_OK == heap.root( session )) ;
      ASSERT_TRUE(session.waitPlan.offset == j) ;
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE(session.waitPlan.offset == j);
   }
   ASSERT_TRUE( 0 == heap.dataSize() ) ;
}

TEST(clsTest, clsHeap_2)
{
   _clsSyncMinHeap heap ;
   _clsSyncSession session ;
   INT32 i = 100 ;
   UINT32 size = i ;
   // push 99 - 0
   while ( i-- >  0 )
   {
      session.waitPlan.offset = i ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }

   ASSERT_TRUE(SDB_OK == heap.erase(size/2));

   UINT32 num = 0 ;
   UINT64 offset = 0 ;
   while ( SDB_OK == heap.root( session ) )
   {
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE( offset <= session.waitPlan.offset ) ;
      offset = session.waitPlan.offset ;
      num++ ;
   }
   ASSERT_TRUE( 99 == num ) ;
   ASSERT_TRUE( 0 == heap.dataSize() ) ;
}

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


void fun( _clsSyncManager *sync, _clsSyncSession session, UINT32 w,
           ossAtomic32 *complete )
{
   ASSERT_TRUE( SDB_OK == sync->sync( session, w) ) ;
   complete->inc() ;
}

TEST(clsTest, clsSyncManager_1)
{
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB eduCB( &mgr, EDU_TYPE_SHARDAGENT ) ;
   _clsSyncSession session ;
   session.eduCB = &eduCB ;
   session.waitPlan.offset = 10 ;
   UINT32 w = 3 ;
   boost::thread t( fun, &sync, session, w, &complete ) ;
   /// ensure that w is registered. sleep one sec.
   DPS_LSN lsn ;
   lsn.version = 1 ;
   lsn.offset = 11 ;
   id.columns.nodeID = 3 ;
   sync.complete( id, lsn, 1) ;
   ASSERT_TRUE( 0 == complete.peek() ) ;
   id.columns.nodeID = 4 ;
   sync.complete( id, lsn, 1) ;
   t.join() ;
   ASSERT_TRUE( 1 == complete.peek() ) ;
}

TEST(clsTest, clsSyncManager_2)
{
   /// local :2 group: 3, 4
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   /// w [0, 9]
   // cout << "construct write(lsn from 0 to 9, w:2)" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   /// require must > wait.
   /// require [1, 10]
   // cout << "construct complete" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ASSERT_TRUE( i - 1 == complete.peek() ) ;
      id.columns.nodeID++ ;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;
      delete ts[i-1] ;
      delete eduCBs[i-1] ;
   }
}

TEST(clsTest, clsSyncManager_3)
{
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;
   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;

   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   /// w = 2
   // cout << "construct write(lsn 0,2,4,6,8 w:2)" << endl ;
   for ( UINT32 i = 0; i < num; i++, i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.waitPlan.reset() ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      UINT32 w = 2 ;
      boost::thread *t = new boost::thread( fun, &sync, session,
                                            w, &complete ) ;
      ts[i] = t ;
   }

   ASSERT_TRUE( 0 == complete.peek() ) ;

   // cout << "construct write(lsn 1,3,5,7,9 w:3)" << endl ;
   /// w = 3
   for ( UINT32 i = 1; i < num; i++, i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   // cout << "construct complete" << endl ;
   for ( UINT32 i = 2; i < num + 1; i++, i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ts[i-2]->join() ;
      ASSERT_TRUE( i - 1 == complete.peek() ) ;

      id.columns.nodeID++ ;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;

      delete ts[i-2] ;
      delete ts[i-1] ;
      delete eduCBs[i-2] ;
      delete eduCBs[i-1] ;
   }
}

TEST(clsTest, clsSyncManager_4)
{
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   // cout << "construct write( lsn from 0 to 9, w:3 )" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   // cout << "construct complete( node 3 )" << endl ;
   /// only one node complete.
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
   }

   ASSERT_TRUE( 0 == complete.peek() ) ;

   // cout << "construct complete( node 4 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 4 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;
      delete ts[i-1] ;
      delete eduCBs[i-1] ;
   }
}

void fun2( _clsSyncManager *sync, _clsSyncSession session, UINT32 w,
           ossAtomic32 *complete,
           ossEvent *pEvent )
{
   pEvent->signal() ;
   ASSERT_TRUE( SDB_CLS_WAIT_SYNC_FAILED == sync->sync( session, w) ) ;
   complete->inc() ;
}

TEST(clsTest, clsSyncManager_5)
{
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   ossEvent event ;
   // cout << "construct write( lsn from 0 to 9, w:3 )" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      UINT32 w = 3 ;
      event.reset() ;
      boost::thread *t = new boost::thread( fun2, &sync, session, w,
                                            &complete, &event ) ;
      event.wait() ;
      ts[i] = t ;
   }

   // cout << "cut to 0" << endl ;
   sync.cut( 0 ) ;

   for ( UINT32 i = 0; i < num; i++ )
   {
      ts[i]->join() ;
      delete ts[i] ;
      delete eduCBs[i] ;
   }
   ASSERT_TRUE( 10 == complete.peek() ) ;
}

TEST(clsTest, clsSyncManager_6)
{
   CLS_WAKE_PLAN plan ;
   utilReplSizePlan expectPlan[6] ;
   expectPlan[0].offset = 100 ;
   expectPlan[0].affinitiveLocations = 1 ;
   expectPlan[0].primaryLocationNodes = 2 ;
   expectPlan[0].locations = 2 ;
   plan.insert( expectPlan[0] ) ;

   expectPlan[1].offset = 120 ;
   expectPlan[1].affinitiveLocations = 1 ;
   expectPlan[1].primaryLocationNodes = 2 ;
   expectPlan[1].locations = 2 ;
   plan.insert( expectPlan[1] ) ;

   expectPlan[2].offset = 140 ;
   expectPlan[2].affinitiveLocations = 1 ;
   expectPlan[2].primaryLocationNodes = 2 ;
   expectPlan[2].locations = 1 ;
   plan.insert( expectPlan[2] ) ;

   expectPlan[3].offset = 180 ;
   expectPlan[3].affinitiveLocations = 1 ;
   expectPlan[3].primaryLocationNodes = 2 ;
   expectPlan[3].locations = 1 ;
   plan.insert( expectPlan[3] ) ;

   expectPlan[4].offset = 180 ;
   expectPlan[4].affinitiveLocations = 0 ;
   expectPlan[4].primaryLocationNodes = 2 ;
   expectPlan[4].locations = 0 ;
   plan.insert( expectPlan[4] ) ;

   expectPlan[5].offset = 200 ;
   expectPlan[5].affinitiveLocations = 0 ;
   expectPlan[5].primaryLocationNodes = 1 ;
   expectPlan[5].locations = 0 ;
   plan.insert( expectPlan[5] ) ;

   UINT32 i = 0 ;
   CLS_WAKE_PLAN::iterator itr = plan.begin() ;
   while ( itr != plan.end() )
   {
      ASSERT_TRUE( *itr == expectPlan[i] ) ;
      i++ ;
      itr++ ;
   }
}

TEST(clsTest, clsSyncManager_7)
{
   // local: 2, group: 3(primary location), 4(affinitive location)
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   info.localLocationID = 1 ;

   pmdSetLocationID( 1 ) ;

   ++id.columns.nodeID ;
   status.locationID = 1 ;
   status.locationIndex = 0 ;
   status.isAffinitiveLocation = TRUE ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   status.locationID = 2 ;
   status.locationIndex = 1 ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   // cout << "construct write( lsn from 0 to 9, w:2 )" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      UINT32 w = 2 ;
      session.waitPlan.setLocMajorReplSizePlan( w - 1, 1, 1, 1, 0 ) ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   // cout << "construct complete( node 3 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
   }

   ASSERT_TRUE( 0 == complete.peek() ) ;

   // cout << "construct complete( node 4 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 4 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;
      delete ts[i-1] ;
      delete eduCBs[i-1] ;
   }
}

TEST(clsTest, clsSyncManager_8)
{
   // local: 2, group: 3(primary location), 4(affinitive location)
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   info.localLocationID = 1 ;

   pmdSetLocationID( 1 ) ;

   ++id.columns.nodeID ;
   status.locationID = 1 ;
   status.locationIndex = 0 ;
   status.isAffinitiveLocation = TRUE ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   status.locationID = 2 ;
   status.locationIndex = 1 ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   // cout << "construct write( lsn from 0 to 9, w:2 )" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      UINT32 w = 2 ;
      session.waitPlan.setPryLocMajorReplSizePlan( w - 1, 1, 1, 1, 0 ) ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   // cout << "construct complete( node 4 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 4 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
   }

   ASSERT_TRUE( 0 == complete.peek() ) ;

   // cout << "construct complete( node 3 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;
      delete ts[i-1] ;
      delete eduCBs[i-1] ;
   }
}

TEST(clsTest, clsSyncManager_9)
{
   // local: 2, group: 3(primary location), 4(affinitive location)
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   info.localLocationID = 1 ;

   pmdSetLocationID( 1 ) ;

   ++id.columns.nodeID ;
   status.locationID = 1 ;
   status.locationIndex = 0 ;
   status.isAffinitiveLocation = TRUE ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   status.locationID = 2 ;
   status.locationIndex = 1 ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   // cout << "construct write( lsn from 0 to 9, w:3 )" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      UINT32 w = 3 ;
      session.waitPlan.setLocMajorReplSizePlan( w - 1, 1, 1, 1, 0 ) ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   // cout << "construct complete( node 3 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
   }

   // cout << "construct complete( node 4 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 4 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;
      delete ts[i-1] ;
      delete eduCBs[i-1] ;
   }
}

TEST(clsTest, clsSyncManager_10)
{
   // local: 2, group: 3(primary location), 4(affinitive location)
   const UINT32 num = 10 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   info.localLocationID = 1 ;

   pmdSetLocationID( 1 ) ;

   ++id.columns.nodeID ;
   status.locationID = 1 ;
   status.locationIndex = 0 ;
   status.isAffinitiveLocation = TRUE ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   status.locationID = 2 ;
   status.locationIndex = 1 ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB *eduCBs[num] ;
   boost::thread *ts[num] ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = NULL ;
      ts[i] = NULL ;
   }

   // cout << "construct write( lsn from 0 to 9, w:3 )" << endl ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      UINT32 w = 3 ;
      session.waitPlan.setPryLocMajorReplSizePlan( w - 1, 1, 1, 1, 0 ) ;
      session.eduCB = eduCBs[i] ;
      session.waitPlan.offset = i ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
   }

   // cout << "construct complete( node 4 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 4 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
   }

   // cout << "construct complete( node 3 )" << endl ;
   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ts[i-1]->join() ;
      ASSERT_TRUE( i == complete.peek() ) ;
      delete ts[i-1] ;
      delete eduCBs[i-1] ;
   }
}

TEST(clsTest, clsSyncManager_11)
{
   // local: 2, group: 3(primary location), 4(affinitive location), 5(other location)
   const UINT32 num = 4 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _clsGroupInfo info ;
   _clsSharingStatus status ;
   MsgRouteID id ;
   id.columns.groupID = 1 ;
   id.columns.nodeID = 2 ;
   info.primary = id ;
   info.local = id ;
   info.localLocationID = 1 ;

   pmdSetLocationID( 1 ) ;

   ++id.columns.nodeID ;
   status.locationID = 1 ;
   status.isAffinitiveLocation = TRUE ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   status.locationID = 2 ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   ++id.columns.nodeID ;
   status.locationID = 3 ;
   status.isAffinitiveLocation = FALSE ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   utilReplSizePlan _locationMajority[2] ;
   utilReplSizePlan _primaryLocationMajority[2] ;

   _locationMajority[0].primaryLocationNodes = 0 ;
   _locationMajority[0].affinitiveLocations = 1 ;
   _locationMajority[0].locations = 1 ;

   _locationMajority[1].primaryLocationNodes = 1 ;
   _locationMajority[1].affinitiveLocations = 1 ;
   _locationMajority[1].locations = 1 ;


   _primaryLocationMajority[0].primaryLocationNodes = 1 ;
   _primaryLocationMajority[0].affinitiveLocations = 0 ;
   _primaryLocationMajority[0].locations = 0 ;

   _primaryLocationMajority[1].primaryLocationNodes = 1 ;
   _primaryLocationMajority[1].affinitiveLocations = 1 ;
   _primaryLocationMajority[1].locations = 1 ;


   // cout << "get consistency strategy( w from 2 to 4 )" << endl ;
   for ( UINT32 w = 2; w <= num; w++ )
   {
      _clsSyncSession session ;
      session.waitPlan.setLocMajorReplSizePlan( w - 1, 1, 1, 2, 0 ) ;
      if ( 2 == w )
      {
         ASSERT_TRUE( session.waitPlan == _locationMajority[0] ) ;
      }
      else
      {
         ASSERT_TRUE( session.waitPlan == _locationMajority[1] ) ;
      }

      session.waitPlan.setPryLocMajorReplSizePlan( w - 1, 1, 1, 2, 0 ) ;
      if ( 2 == w )
      {
         ASSERT_TRUE( session.waitPlan == _primaryLocationMajority[0] ) ;
      }
      else
      {
         ASSERT_TRUE( session.waitPlan == _primaryLocationMajority[1] ) ;
      }
   }
}

// node priority strategy
TEST(clsTest, clsSyncManager_12)
{
   const UINT32 num = 3 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _pmdEDUMgr mgr ;

   for ( UINT32 i = 0; i < 3; i++ )
   {
      _clsGroupInfo info ;
      _clsSharingStatus status ;
      MsgRouteID id ;
      id.columns.groupID = 1 ;
      id.columns.nodeID = 1 ;
      info.primary = id ;
      info.local = id ;
      info.localLocationID = 1 ;
      pmdSetLocationID( 1 ) ;
      // primary location node
      ++id.columns.nodeID ;
      status.locationID = 1 ;
      status.locationIndex = 1 ;
      status.isAffinitiveLocation = TRUE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      // affinitive location node
      ++id.columns.nodeID ;
      status.locationID = 2 ;
      status.locationIndex = 2 ;
      status.isAffinitiveLocation = TRUE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      // other location node
      ++id.columns.nodeID ;
      status.locationID = 3 ;
      status.locationIndex = 3 ;
      status.isAffinitiveLocation = FALSE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      _clsSyncManager sync( &agent, &info ) ;
      sync.updateNotifyList( TRUE ) ;
      ossAtomic32 complete( 0 ) ;

      // case 1: the remote node lsn less than all local node.
      if ( i == 0 )
      {
         UINT32 w = 3 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setNodeReplSizePlan( w, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num - 1; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            lsn.offset = 2 ;
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 2: the remote node lsn more than all local node.
      else if ( i == 1 )
      {
         UINT32 w = 3 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setNodeReplSizePlan( w, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == 0 )
            {
               lsn.offset = 2 ;
            }
            else if ( i == 1 )
            {
               lsn.offset = 4 ;
            }
            else
            {
               lsn.offset = 3 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 3: w < affinitiveNodes
      else if ( i == 2 )
      {
         UINT32 w = 2 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setNodeReplSizePlan( w, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == 0 )
            {
               lsn.offset = 3 ;
            }
            else
            {
               lsn.offset = 0 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
   }
}

// location majority strategy
TEST(clsTest, clsSyncManager_13)
{
   const UINT32 num = 6 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _pmdEDUMgr mgr ;
   for ( UINT32 i = 0; i < 4; i++ )
   {
      _clsGroupInfo info ;
      _clsSharingStatus status ;
      MsgRouteID id ;
      id.columns.groupID = 1 ;
      id.columns.nodeID = 1 ;
      info.primary = id ;
      info.local = id ;
      info.localLocationID = 1 ;
      pmdSetLocationID( 1 ) ;
      // primary location node
      ++id.columns.nodeID ;
      status.locationID = 1 ;
      status.locationIndex = 1 ;
      status.isAffinitiveLocation = TRUE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      // affinitive location node
      ++id.columns.nodeID ;
      status.locationID = 2 ;
      status.locationIndex = 2 ;
      status.isAffinitiveLocation = TRUE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      // other location node
      ++id.columns.nodeID ;
      status.locationID = 3 ;
      status.locationIndex = 3 ;
      status.isAffinitiveLocation = FALSE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      ++id.columns.nodeID ;
      status.locationID = 3 ;
      status.locationIndex = 3 ;
      status.isAffinitiveLocation = FALSE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      ++id.columns.nodeID ;
      status.locationID = 4 ;
      status.locationIndex = 4 ;
      status.isAffinitiveLocation = FALSE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      ++id.columns.nodeID ;
      status.locationID = 5 ;
      status.locationIndex = 5 ;
      status.isAffinitiveLocation = FALSE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      _clsSyncManager sync( &agent, &info ) ;
      sync.updateNotifyList( TRUE ) ;
      ossAtomic32 complete( 0 ) ;

      // case 1: the remote node lsn less than all local node.
      if ( i == 0 )
      {
         UINT32 w = 3 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i > 1 )
            {
               lsn.offset = 3 ;
            }
            else if ( i == 0 )
            {
               lsn.offset = 2 ;
            }
            else
            {
               lsn.offset = 5 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 2: the remote node lsn more than local node.
      else if ( i == 1 )
      {
         UINT32 w = 3 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == num - 1 )
            {
               lsn.offset = 3 ;
            }
            else
            {
               lsn.offset = 2 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 3: need write affinitive location node
      else if ( i == 2 )
      {
         UINT32 w = 2 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == 1 )
            {
               lsn.offset = 3 ;
            }
            else
            {
               lsn.offset = 0 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         boost::chrono::milliseconds timeOut( 1000 ) ;
         boost::this_thread::sleep_for( timeOut ) ;
         if ( t->joinable() )
         {
            t->interrupt() ;
            t->join() ;
         }
         ASSERT_TRUE( 0 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 4: need wait
      else if ( i == 3 )
      {
         UINT32 w = 2 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == 1 )
            {
               lsn.offset = 0 ;
            }
            else
            {
               lsn.offset = 4 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         boost::chrono::milliseconds timeOut( 1000 ) ;
         boost::this_thread::sleep_for( timeOut ) ;
         if ( t->joinable() )
         {
            t->interrupt() ;
            t->join() ;
         }
         // cout << complete.peek() << endl ;
         ASSERT_TRUE( 0 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
   }
}

// primary Location strategy
TEST(clsTest, clsSyncManager_14)
{
   const UINT32 num = 5 ;
   myHandler handler ;
   _netRouteAgent agent( &handler ) ;
   _pmdEDUMgr mgr ;


   for ( UINT32 i = 0; i < 4; i++ )
   {
      _clsGroupInfo info ;
      _clsSharingStatus status ;
      MsgRouteID id ;
      id.columns.groupID = 1 ;
      id.columns.nodeID = 1 ;
      info.primary = id ;
      info.local = id ;
      info.localLocationID = 1 ;
      pmdSetLocationID( 1 ) ;
      // primary location node
      ++id.columns.nodeID ;
      status.locationID = 1 ;
      status.locationIndex = 1 ;
      status.isAffinitiveLocation = TRUE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      // affinitive location node
      ++id.columns.nodeID ;
      status.locationID = 2 ;
      status.locationIndex = 2 ;
      status.isAffinitiveLocation = TRUE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      // other location node
      ++id.columns.nodeID ;
      status.locationID = 3 ;
      status.locationIndex = 3 ;
      status.isAffinitiveLocation = FALSE ;
      info.info.insert( std::make_pair( id.value, status ) ) ;
      info.alives.insert( std::make_pair( id.value, &(info.info[id.value]) ) ) ;

      _clsSyncManager sync( &agent, &info ) ;
      sync.updateNotifyList( TRUE ) ;
      ossAtomic32 complete( 0 ) ;

      // case 1: the remote node lsn less than all local node.
      if ( i == 0 )
      {
         UINT32 w = 3 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setPryLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i > 1 )
            {
               lsn.offset = 3 ;
            }
            else if ( i == 0 )
            {
               lsn.offset = 2 ;
            }
            else
            {
               lsn.offset = 5 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 2: the remote node lsn more than local node.
      else if ( i == 1 )
      {
         UINT32 w = 3 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setPryLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == num - 1 )
            {
               lsn.offset = 3 ;
            }
            else
            {
               lsn.offset = 2 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         t->join() ;
         ASSERT_TRUE( 1 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 3: need write primary location node
      else if ( i == 2 )
      {
         UINT32 w = 2 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setPryLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == 0 )
            {
               lsn.offset = 3 ;
            }
            else
            {
               lsn.offset = 0 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         //cout << complete.peek() << endl ;
         boost::chrono::milliseconds timeOut( 1000 ) ;
         boost::this_thread::sleep_for( timeOut ) ;
         if ( t->joinable() )
         {
            t->interrupt() ;
            t->join() ;
         }
         ASSERT_TRUE( 0 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
      // case 4: need wait
      else if ( i == 3 )
      {
         UINT32 w = 2 ;
         pmdEDUCB *eduCBs = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
         _clsSyncSession session ;
         session.eduCB = eduCBs ;
         session.waitPlan.setPryLocMajorReplSizePlan( w, 1, 1, 1, 2 ) ;
         session.waitPlan.offset = 1 ;
         id.columns.nodeID = 2 ;
         for ( UINT32 i = 0 ; i < num ; i++ )
         {
            DPS_LSN lsn ;
            lsn.version = 1 ;
            if ( i == 0 )
            {
               lsn.offset = 0 ;
            }
            else
            {
               lsn.offset = 2 ;
            }
            sync.complete( id, lsn, 1 ) ;
            id.columns.nodeID++ ;
         }
         boost::thread *t = new boost::thread( fun, &sync, session, w, &complete ) ;
         boost::chrono::milliseconds timeOut( 1000 ) ;
         boost::this_thread::sleep_for( timeOut ) ;
         if ( t->joinable() )
         {
            t->interrupt() ;
            t->join() ;
         }
         //cout << complete.peek() << endl ;
         ASSERT_TRUE( 0 == complete.peek() ) ;
         delete t ;
         delete eduCBs ;
      }
   }
}