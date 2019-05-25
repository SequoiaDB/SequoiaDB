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

*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>

#include "clsSyncMinHeap.hpp"
#include "clsSyncManager.hpp"
#include "netRouteAgent.hpp"
#include "netMsgHandler.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "ossAtomic.hpp"

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
   while ( i-- >  0 )
   {
      session.endLsn = i ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }

   ASSERT_TRUE( num == heap.dataSize() ) ;

   for ( UINT32 j = 0; j < num; j++ )
   {
      ASSERT_TRUE( SDB_OK == heap.root( session )) ;
      ASSERT_TRUE(session.endLsn == j) ;
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE(session.endLsn == j);
      cout << session.endLsn << endl ;
   }
   ASSERT_TRUE( 0 == heap.dataSize() ) ;
   for ( UINT32 j = 0; j < num; j++ )
   {
      session.endLsn = j ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }
   ASSERT_TRUE( num == heap.dataSize() ) ;
   cout << endl ;
   for ( UINT32 j = 0; j < num; j++ )
   {
      ASSERT_TRUE( SDB_OK == heap.root( session )) ;
      ASSERT_TRUE(session.endLsn == j) ;
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE(session.endLsn == j);
      cout << session.endLsn << endl ;
   }
   ASSERT_TRUE( 0 == heap.dataSize() ) ;
}

TEST(clsTest, clsHeap_2)
{
   _clsSyncMinHeap heap ;
   _clsSyncSession session ;
   INT32 i = 100 ;
   UINT32 size = i ;
   while ( i-- >  0 )
   {
      session.endLsn = i ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }

   ASSERT_TRUE(SDB_OK == heap.erase(size/2));

   UINT32 num = 0 ;
   UINT64 offset = 0 ;
   while ( SDB_OK == heap.root( session ) )
   {
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE( offset <= session.endLsn ) ;
      offset = session.endLsn ;
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
   cout << "multi w complete" << endl ;
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
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
   _clsSyncManager sync( &agent, &info ) ;
   sync.updateNotifyList( TRUE ) ;
   ossAtomic32 complete( 0 ) ;

   _pmdEDUMgr mgr ;
   pmdEDUCB eduCB( &mgr, EDU_TYPE_SHARDAGENT ) ;
   _clsSyncSession session ;
   session.eduCB = &eduCB ;
   session.endLsn = 10 ;
   UINT32 w = 3 ;
   boost::thread t( fun, &sync, session, w, &complete ) ;
   ossSleepsecs(2) ;
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
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
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

   for ( UINT32 i = 0; i < num; i++ )
   {
      cout << "construct w" << endl ;
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.endLsn = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
      ossSleepsecs(1) ;
   }

   cout << "waiting..." << endl ;
   ossSleepsecs(2) ;
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
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
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

   for ( UINT32 i = 0; i < num; i++, i++ )
   {
      cout << "construct w" << endl ;
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.endLsn = i ;
      UINT32 w = 2 ;
      boost::thread *t = new boost::thread( fun, &sync, session,
                                            w, &complete ) ;
      ts[i] = t ;
      ossSleepsecs(1) ;
   }

   ASSERT_TRUE( 0 == complete.peek() ) ;

   for ( UINT32 i = 1; i < num; i++, i++ )
   {
      cout << "construct w" << endl ;
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.endLsn = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
      ossSleepsecs(1) ;
   }

   cout << "waiting..." << endl ;
   ossSleepsecs(2) ;

   for ( UINT32 i = 2; i < num + 1; i++, i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
      ASSERT_TRUE( i - 2 == complete.peek() ) ;
      id.columns.nodeID++ ;
      sync.complete( id, lsn, 1 ) ;
      ts[i-2]->join() ;
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
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
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

   for ( UINT32 i = 0; i < num; i++ )
   {
      cout << "construct w" << endl ;
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.endLsn = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
      ossSleepsecs(1) ;
   }

   cout << "waiting..." << endl ;
   ossSleepsecs(2) ;

   for ( UINT32 i = 1; i < num + 1; i++ )
   {
      id.columns.nodeID = 3 ;
      DPS_LSN lsn ;
      lsn.version = 1 ;
      lsn.offset = i;
      sync.complete( id, lsn, 1 ) ;
   }

   ASSERT_TRUE( 0 == complete.peek() ) ;

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
           ossAtomic32 *complete )
{
   ASSERT_TRUE( SDB_CLS_NODE_NOT_ENOUGH == sync->sync( session, w) ) ;
   complete->inc() ;
   cout << "multi w complete" << endl ;
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
   ++id.columns.nodeID ;
   info.info.insert( std::make_pair( id.value, status ) ) ;
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

   for ( UINT32 i = 0; i < num; i++ )
   {
      cout << "construct w" << endl ;
      eduCBs[i] = new pmdEDUCB( &mgr, EDU_TYPE_AGENT ) ;
      _clsSyncSession session ;
      session.eduCB = eduCBs[i] ;
      session.endLsn = i ;
      UINT32 w = 3 ;
      boost::thread *t = new boost::thread( fun2, &sync, session, w,
                                            &complete ) ;
      ts[i] = t ;
      ossSleepsecs(1) ;
   }

   cout << "waiting..." << endl ;
   ossSleepsecs(2) ;

   sync.cut( 0 ) ;
   for ( UINT32 i = 0; i < num; i++ )
   {
      ts[i]->join() ;
      delete ts[i] ;
      delete eduCBs[i] ;
   }
   ASSERT_TRUE( 10 == complete.peek() ) ;
}
