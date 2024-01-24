/*********************************************************************************
 * @Description: test case for connectionpool
 *               seqDB-9499:申请连接超过最大闲置连接数后,一段时间内持续归还连接,
 *                          等待空闲时长大于checkInterval 
 *               seqDB-9500:申请连接超过最大闲置连接数后,一段时间内持续归还连接,
 *                          等待空闲时长小于checkInterval
 *               seqDB-9503:设置keepalive=0
 *               seqDB-9504:设置keepalive>checkInterval
 *               seqDB-9501:设置validateConnection=true
 *               seqDB-9502:设置validateConnection=false
 * @Modify:      Liangxw
 *               2019-09-04
 *********************************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include <iostream>
#include <vector>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class timeTest9499 : public testBase
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;
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

// 休眠时间>checkInterval, 检查连接池空闲连接数量<=maxIdleConnNum
TEST_F( timeTest9499, checkIntervalLong9499 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   vector<sdb*> vec ;

   conf.setCheckIntervalInfo( 3000, 0 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   while( vec.size() <= conf.getMaxIdleCount() )
   {
      rc = ds.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
      vec.push_back( conn ) ;
   }

   for( INT32 i = 0;i != vec.size();++i )
   {
      ds.releaseConnection( vec[i] ) ;
   }

   // check connection num before/after sleep more than checkInterval
   cout << "before sleep, idle connection num: " << ds.getIdleConnNum() << endl ;
   ossSleep( 6*1000 ) ;
   ASSERT_GE( 20, ds.getIdleConnNum() ) << "fail to check idle conn num when sleep "
                                        << "more than checkInterval" ;
}

// 休眠时间<checkInterval, 检查连接池空闲连接数量>=maxIdleConnNum
TEST_F( timeTest9499, checkIntervalShort9500 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   vector<sdb*> vec ;

   conf.setCheckIntervalInfo( 3000, 0 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   // get connection more than max idle connection num 20
   while( vec.size() <= conf.getMaxIdleCount()+10 )
   {
      rc = ds.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
      vec.push_back( conn ) ;
   }
   cout << "vector of connection size is " << vec.size() << endl ;

   for( INT32 i = 0;i != vec.size();++i )
   {
      ds.releaseConnection( vec[i] ) ;
   }

   // check idle connection num without sleep
   INT32 connNum = ds.getIdleConnNum() ;   
   ASSERT_LE( 20, connNum ) << "fail to check idle conn num when no sleep" ;
}

// 设置keepAliveTimeout=0,检查连接有效性
TEST_F( timeTest9499, keepAliveTimoutZero9503 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;

   conf.setCheckIntervalInfo( 3000, 0 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   ASSERT_EQ( 9, ds.getIdleConnNum() ) << "fail to check idle conn num" ;

   // release connection
   // 休眠时间>checkInterval,但是keepAliveTimeout=0,连接有效,空闲连接数为10
   ossSleep( 3*1000 ) ;
   ds.releaseConnection( conn ) ;
   ossSleep( 3*1000 ) ;
   ASSERT_EQ( 10, ds.getIdleConnNum() ) << "fail to check idle conn num" ;
}

// 设置keepAliveTimeout!=0,检查连接有效性
TEST_F( timeTest9499, keepAliveTimoutNotZero9504 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;

   // set checkInterval 3000ms keepAliveTimeout 6000ms
   conf.setCheckIntervalInfo( 3000, 6000 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   ASSERT_EQ( 9, ds.getIdleConnNum() ) << "fail to check idle conn num" ;

   // 获取连接后休眠时间>keepAliveTimeout,连接超时,此时连接池里的连接也都超时了。
   ossSleep( 7*1000 ) ;
   ds.releaseConnection( conn ) ;
   // 释放连接后休眠时间>checkInterval,连接应该被清除,连接池的空闲连接为0
   ossSleep( 4*1000 ) ;
   ASSERT_EQ( 0, ds.getIdleConnNum() ) << "fail to check idle conn num after sleep" ;
}

// 2419问题单回归后打开
// 设置keepAliveTimeout != 0,检查连接有效性
TEST_F( timeTest9499, keepAliveTimoutNotZeroAgain9504 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   sdbCollectionSpace cs ;

   // set checkInterval 3000ms keepAliveTimeout 9000ms
   conf.setCheckIntervalInfo( 3000, 9000 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   // get connection and check idle conn num
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   ASSERT_EQ( 9, ds.getIdleConnNum() ) ;   
   ossSleep( 3*1000 ) ; 
   ASSERT_EQ( 9, ds.getIdleConnNum() ) ;
   ossSleep( 1*1000 ) ; 
   ASSERT_EQ( 9, ds.getIdleConnNum() ) ;
   ossSleep( 1*1000 ) ;
   ASSERT_EQ( 9, ds.getIdleConnNum() ) ;   
   ossSleep( 2*1000 ) ; 
   ASSERT_EQ( 0, ds.getIdleConnNum() ) ;

   // craete/drop cs to check connection valid
   const CHAR* csName = "connectionpoolTestCs_9504" ;      
   rc = conn->createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;   
   rc = conn->dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" << csName ;

   // release connection and check idle conn num
   ds.releaseConnection( conn ) ;
   ASSERT_EQ( 1, ds.getIdleConnNum() ) ;
   ossSleep( 10*1000 ) ;
   ASSERT_EQ( 0, ds.getIdleConnNum() ) ;
}

// 出池检验连接有效性,停节点后获取不到连接
TEST_F( timeTest9499, trueTest9501 )
{
   INT32 rc = SDB_OK ;
   CHAR localHostName[100] ;
   rc = getLocalHost( localHostName, 100 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* hostName = strcmp( ARGS->hostName(), "localhost" ) ? ARGS->hostName() : localHostName ;
   const CHAR* svcName = ARGS->svcName() ;
   CHAR nodeName[100] ;
   sprintf( nodeName, "%s%s%s", hostName, ":", svcName ) ;
   sdb* conn = NULL ;

   // set validate connection
   conf.setValidateConnection( true ) ;
   conf.setSyncCoordInterval( false ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   
   // get connection
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;

   // check standalone
   if( isStandalone( *conn ) )
   {
      cout << "Run mode is standalone" << endl ;
      ds.releaseConnection( conn ) ;
      return ;
   }

   // get coord group nodes
   vector<string> nodes;
   rc = getGroupNodes( *conn, "SYSCoord", nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( nodes.size() < 2 )
   {
      cout << "coord group nodes num too few." << endl ;
      ds.releaseConnection( conn ) ;
      return ;
   }

   // connect another coord and get rg node
   sdbReplicaGroup rg ;
   sdbNode node ;
   sdb db ;
   for( INT32 i = 0;i < nodes.size();i++ )
   {
      if( nodes[i] == nodeName ) 
         continue ;
      size_t pos = nodes[i].find_first_of( ":" ) ;
      string host = nodes[i].substr( 0, pos ) ;
      string svc = nodes[i].substr( pos+1 ) ;
      rc = db.connect( host.c_str(), svc.c_str() ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect " << host << " " << svc ;
      break ;
   }
   rc = db.getReplicaGroup( "SYSCoord", rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get coord group" ;
   rc = rg.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node" ;

   // stop node and check conn invalid and get connection
   rc = node.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node" ;
   sdb tmp ;
   rc = tmp.connect( hostName, svcName ) ;
   ASSERT_EQ( SDB_NET_CANNOT_CONNECT, rc ) << "fail to check connect after stop node" ;
   ASSERT_EQ( 0, conn->isValid() ) << "fail to check conn invalid after stop node" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD, rc ) << "fail to test get connection after stop node" ;

   // start node and get connection
   rc = node.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start node" ;
   rc = tmp.connect( hostName, svcName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check connect after start node" ;
   rc = ds.getConnection( conn ) ;
   while( rc == SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD )
   {
      ossSleep( 1000 ) ;
      rc = ds.getConnection( conn ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "fail to check get connection after start node" ;
   ASSERT_EQ( 1, conn->isValid() ) << "fail to check connection valid after start node" ;

   ds.releaseConnection( conn ) ;
   tmp.disconnect() ;
   db.disconnect() ;
}

// 出池不检验连接有效性,停节点能获得连接但连接无效
TEST_F( timeTest9499, falseTest9502 )
{
   INT32 rc = SDB_OK ;
   CHAR localHostName[100] ;
   rc = getLocalHost( localHostName, 100 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* hostName = strcmp( ARGS->hostName(), "localhost" ) ? ARGS->hostName() : localHostName ;
   const CHAR* svcName = ARGS->svcName() ;
   CHAR nodeName[100] ;
   sprintf( nodeName, "%s%s%s", hostName, ":", svcName ) ;
   sdb* conn = NULL ;

   // set validate connection false
   conf.setValidateConnection( false ) ;
   conf.setSyncCoordInterval( false ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   // get connection
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   ASSERT_EQ( 1, conn->isValid() ) << "fail to check connection valid" ;

   // check standalone
   if( isStandalone( *conn ) )
   {
      cout << "Run mode is standalone." << endl ;
      ds.releaseConnection( conn ) ;
      return ;
   }

   // get coord group nodes
   vector<string> nodes ;
   rc = getGroupNodes( *conn, "SYSCoord", nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get coord nodes" ;
   if( nodes.size() < 2 )
   {
      cout << "coord group nodes num too few." << endl ;
      ds.releaseConnection( conn ) ;
      return ;
   }

   // connect another coord and get rg node
   sdbReplicaGroup rg ;
   sdbNode node ;
   sdb db ;
   for( INT32 i = 0;i < nodes.size();i++ )
   {
      if( nodes[i] == nodeName )
         continue ;
      size_t pos = nodes[i].find_first_of( ":" ) ;
      string host = nodes[i].substr( 0, pos ) ;
      string svc = nodes[i].substr( pos+1 ) ;
      rc = db.connect( host.c_str(), svc.c_str() ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect " << host << " " << svc ;
      break ;
   }
   rc = db.getReplicaGroup( "SYSCoord", rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get coord group" ;
   rc = rg.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node" ;

   // stop node and check get connection
   rc = node.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test get connection after stop node" ;  
   ASSERT_EQ( 0, conn->isValid() ) << "fail to check connection invalid after stop node" ;
   
   // start node
   rc = node.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start node" ;
      
   db.disconnect() ;
   ds.releaseConnection( conn ) ;
}
