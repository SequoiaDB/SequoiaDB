/********************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9505:申请到池满，再次申请连接
 *               seqDB-9517:close后，获取连接
 *               seqDB-9510:没有调用init，获取连接
 *               seqDB-9514:没有调用init，close连接池
 *               seqDB-9534:releaseConnection不属于连接池的连接
 * @Modify:      Liangxw
 *               2019-09-05
 ********************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include "connpool_common.hpp"

using namespace sdbclient ;

class connTest9505 : public testBase
{
protected:
   sdbConnectionPoolConf conf ;
   sdbConnectionPool ds ;
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

// 获取连接到连接池满后继续申请连接
TEST_F( connTest9505, fullConn9505 )
{
   INT32 rc = SDB_OK ;
   // init connectionpool
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   // get connection until max count
   sdb* conn = NULL ;
   vector<sdb*> vec ;
   while( vec.size() < conf.getMaxCount() )
   {
      ASSERT_EQ( SDB_OK, ds.getConnection( conn ) ) ;
      vec.push_back( conn ) ;
   }
   
   // continue to get connection
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_DRIVER_DS_RUNOUT, rc ) << "fail to test get connection after get max count" ;
   
   // release connection
   for( INT32 i = 0;i < vec.size();i++ )
   {
      ds.releaseConnection( vec[i] ) ;
   }
}

// 调用close后继续执行相关操作( 9517-9520 )
TEST_F( connTest9505, close9517 )
{
   INT32 rc = SDB_OK ;
   // init connectionpool   
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   // close connectionpool
   ds.close() ;
	
   // operation after close
   sdb* conn ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_CLOSE, rc ) << "fail to test get connection after close" ;
}


// 没有调用init就执行相关操作( 9510-9514 )
TEST_F( connTest9505, withoutInit9510 )
{
   INT32 rc = SDB_OK ;
   conf.setSyncCoordInterval( 0 ) ;
   sdb* conn = NULL ;
   
   // operation before init
   rc = ds.getConnection( conn ) ;	
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_NOT_INIT, rc ) << "fail to test get connection before init" ;
   ds.close() ;		
}

// 释放不属于连接池的连接
TEST_F( connTest9505, releaseNormalConnection9534 )
{
   INT32 rc = SDB_OK ;
   // init connectionpool
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   // get a normal connection
   sdb *tmp ;
   sdb conn ;
   rc = conn.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   
   // release normal connection
   tmp = &conn ;
   ds.releaseConnection( tmp ) ;
   
   ASSERT_EQ( 1, conn.isValid() ) << "fail to check connection valid" ;
   conn.disconnect() ;
}
