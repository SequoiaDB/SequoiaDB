/*********************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9535:init与init之间并发
 *               seqDB-9538:init与close之间并发
 *               seqDB-9539:init与getConnection之间并发
 *               seqDB-9540:init与releaseConnection之间并发
 *               seqDB-9556:close与close之间并发
 *               seqDB-9557:close与getConnection之间并发
 *               seqDB-9558:close与releaseConnection之间并发
 *               seqDB-9561:getConnection与getConnection之间并发
 *               seqDB-9562:getConnection与releaseConnection之间并发
 *               seqDB-9565:releaseConnection与releaseConnection之间并发
 * @Modify:      Liangxw
 *               2019-09-05
 *********************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include <iostream>
#include "impWorker.hpp"
#include "connpool_thread.hpp"
#include "connpool_common.hpp"

using namespace sdbclient ;
using namespace import ;
using namespace std ;

// 定义线程数量
#define ThreadNum 5 

class threadTest9535 : public testBase
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;
   Worker* workers[ ThreadNum ] ;
   string url ;

   void SetUp()
   {
      url = ARGS->coordUrl() ;
   }
   void TearDown()
   {
      ds.close() ;
   }
} ;

/* 问题单1946,init与其他操作并发,core,init close与其他操作不并发
// init与init之间并发  正常获取释放连接
TEST_F( threadTest9535, initInit9535 )
{
   INT32 rc = SDB_OK ;
   DsArgs args( ds ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)init, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
   
   sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;		
   ds.releaseConnection( conn ) ;					
}

// init与close之间并发，不出现死锁
TEST_F( threadTest9535, initClose9538 )
{
   DsArgs args( ds ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)init_disable, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {   
      workers[i]->waitStop() ;
      delete workers[i] ;
   } 
}

// init与getConnection/releaseConnection之间并发，没有init时获取连接出错( 9539 9540 )
TEST_F( threadTest9535, initConn9539 )
{
   DsArgs args( ds ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)init_conn, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

*/


// close与close之间并发，无死锁
TEST_F( threadTest9535, closeClose9556 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   DsArgs args( ds ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)dsclose, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// close与getConnection/releaseConnection之间并发，close后获取连接出错( 9557 9558 )
TEST_F( threadTest9535, closeConn9557 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   DsArgs args( ds ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)dsclose_conn, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// getConnection与getConnection/releaseConnection之间并发，正常获取释放连接( 9561 9562 )
TEST_F( threadTest9535, getConnConn9561 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   DsArgs args( ds ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)connection, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// releaseConnection与releaseConnection之间并发，正常获取连接
TEST_F( threadTest9535, releaseConnConn9565 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   vector<sdb*> vec ;
   INT32 cnt = 0 ;
   while( cnt < 10 )
   {
      sdb* conn = NULL ;
      rc = ds.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
      vec.push_back( conn ) ;
      ++cnt ;
   }

   DsArgs args( ds, vec ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i] = new Worker( (WorkerRoutine)releaseConn, &args, false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
