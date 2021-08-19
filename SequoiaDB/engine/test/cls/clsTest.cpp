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
      session.endLsn = i ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }

   ASSERT_TRUE( num == heap.dataSize() ) ;

   /// pop 0 - 9
   for ( UINT32 j = 0; j < num; j++ )
   {
      ASSERT_TRUE( SDB_OK == heap.root( session )) ;
      ASSERT_TRUE(session.endLsn == j) ;
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE(session.endLsn == j);
   }
   ASSERT_TRUE( 0 == heap.dataSize() ) ;

   /// push 0 - 9
   for ( UINT32 j = 0; j < num; j++ )
   {
      session.endLsn = j ;
      ASSERT_TRUE(SDB_OK == heap.push(session));
   }
   ASSERT_TRUE( num == heap.dataSize() ) ;

   /// pop 0 - 9
   for ( UINT32 j = 0; j < num; j++ )
   {
      ASSERT_TRUE( SDB_OK == heap.root( session )) ;
      ASSERT_TRUE(session.endLsn == j) ;
      ASSERT_TRUE(SDB_OK == heap.pop(session)) ;
      ASSERT_TRUE(session.endLsn == j);
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
   session.endLsn = 10 ;
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
      session.endLsn = i ;
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
      session.eduCB = eduCBs[i] ;
      session.endLsn = i ;
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
      session.endLsn = i ;
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
      session.endLsn = i ;
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
      session.endLsn = i ;
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
