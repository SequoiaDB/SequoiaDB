/*****************************************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9483:同步coord的情况下，顺序策略申请连接
 *               seqDB-9484:同步coord的情况下，随机策略申请连接
 *               seqDB-9485:同步coord的情况下，本地策略申请连接
 *               seqDB-9486:同步coord的情况下，均衡策略申请连接
 *               seqDB-9487:初始化时指定多个coord，按顺序策略申请连接
 *               seqDB-9488:初始化时指定多个coord，按随机策略申请连接
 *               seqDB-9489:初始化时指定多个coord，按本地策略申请连接
 *               seqDB-9490:初始化时指定多个coord，按均衡策略申请连接
 *               手工测试用例，不加入scons脚本
 * @Modify:      Liangxw
 *               2019-09-05
 *****************************************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include <iostream>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

// 手工测试用例,通过控制台输出检查分配策略是否生效
// 需要开发修改代码显示新连接的连接地址测试
class strategyTest9483 : public testBase
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;
   string url ;
   string url1 ;
   string url2 ;
   INT32 coordNum ;  // 协调组中协调节点数

   void SetUp()
   {
      url = "192.168.30.45:50000" ;   // 临时协调节点放在第一个位置：50000 11810 11810
      url1 = "192.168.30.44:11810" ;
      url2 = "192.168.30.45:11810" ;
   }
   void TearDown()
   {
      ds.close() ;
   }

   int getCoordNodeNum(sdb* conn);
} ;

INT32 checkStartegy( sdbConnectionPool& ds )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   INT32 cnt = 0 ;
   vector<sdb*> vec ;
   map<string, int> addr2count ;
   map<string, int>::iterator it ;
   while( cnt++ < 100 )
   {
      rc = ds.getConnection( conn ) ;
      string addr = conn->getAddress() ;
      CHECK_RC( SDB_OK, rc, "fail to get connection" ) ;
      if ( addr2count.find( addr ) != addr2count.end() )
      {
         addr2count[addr] += 1;
      }
      else
      {
         addr2count[addr] = 1;
      }
      vec.push_back( conn ) ;
   }

   for ( it = addr2count.begin(); it != addr2count.end(); ++it )
   {
       cout << it->first << " :" << it->second << endl;
   }
done:
   for( INT32 i = 0;i < vec.size();i++ )
   {
      ds.releaseConnection( vec[i] ) ; 
   }
   return rc ;
error:
   goto done ;
}

int strategyTest9483::getCoordNodeNum(sdb* conn)
{
   if ( coordNum != 0 )
   {
      return coordNum ;
   }

   sdbReplicaGroup coord ;
   INT32 rc = conn->getReplicaGroup(2, coord);
   if ( rc != SDB_OK )
   {
      return 0 ;
   }

   bson::BSONObj obj;
   rc = coord.getDetail(obj) ;
   if ( rc != SDB_OK )
   {
      return 0 ;
   }

   std::vector<bson::BSONElement> group = obj.getField("Group").Array() ;
   coordNum = group.size() ;
   int i = 0 ;
   for (i = 0; i < group.size(); ++i)
   {
      bson::BSONObj ret = group[i].Obj() ;
      std::string hostName = ret.getField("HostName").String() ;
      
      std::vector<bson::BSONElement> svcs = ret.getField("Service").Array();
      std::string svcName = svcs[0].Obj().getField("Name").String() ;

      std::string tmpAddr = hostName + ":" + svcName ; 
      if ( tmpAddr == url )
      {
          break ;
      }
   }
   if ( i == group.size() ) { coordNum += 1; }
   return coordNum ;
}

// 同步情况下测试顺序分配策略
TEST_F( strategyTest9483, syncSerial9483 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   conf.setConnectStrategy( SDB_CONN_STY_SERIAL ) ;
   conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ossSleep( 2*1000 ) ;

   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc) << "getConnection failed" ;

   coordNum = getCoordNodeNum(conn) ;
   ASSERT_EQ( coordNum, ds.getNormalAddrNum () ) << "fail to check coord num" ;
   
   checkStartegy( ds ) ;
}

// 同步情况下测试随机分配策略
TEST_F( strategyTest9483, syncRandom9484 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   conf.setConnectStrategy( SDB_CONN_STY_RANDOM ) ;
   conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ossSleep( 2*1000 ) ;

   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc) << "getConnection failed" ;

   coordNum = getCoordNodeNum(conn) ;
   ASSERT_EQ( coordNum, ds.getNormalAddrNum () ) << "fail to check coord num" ;
	
   checkStartegy( ds ) ;
}

// 同步情况下测试本地分配策略
TEST_F( strategyTest9483, syncLocal9485 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   conf.setConnectStrategy( SDB_CONN_STY_LOCAL ) ;
   conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ossSleep( 2*1000 ) ;

   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc) << "getConnection failed" ;
   coordNum = getCoordNodeNum(conn) ;
   ASSERT_EQ( coordNum, ds.getNormalAddrNum () ) << "fail to check coord num" ;

   checkStartegy( ds ) ;
}
// 同步情况下测试均衡分配策略
TEST_F( strategyTest9483, syncBalance9486 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   conf.setConnectStrategy( SDB_CONN_STY_BALANCE ) ;
   conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ossSleep( 2*1000 ) ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc) << "getConnection failed" ;
   coordNum = getCoordNodeNum(conn) ;

   ASSERT_EQ( coordNum, ds.getNormalAddrNum () ) << "fail to check coord num" ;
   
   checkStartegy( ds ) ;
}
// 初始化多个节点情况下测试顺序分配策略
TEST_F( strategyTest9483, multiCoordSerial9487 )
{
   INT32 rc = SDB_OK ;
   conf.setConnectStrategy( SDB_CONN_STY_SERIAL ) ;
   conf.setSyncCoordInterval( 0 ) ;
   vector<string> urlList ;
   urlList.push_back( url ) ;
   urlList.push_back( url1 ) ;

   rc = ds.init( urlList, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ASSERT_EQ( urlList.size(), ds.getNormalAddrNum () ) << "fail to check coord num" ;

   checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试随机分配策略
TEST_F( strategyTest9483, multiCoordRandom9488 )
{
   INT32 rc = SDB_OK ;
   conf.setConnectStrategy( SDB_CONN_STY_RANDOM ) ;
   conf.setSyncCoordInterval( 0 ) ;
   vector<string> urlList ;
   urlList.push_back( url ) ;
   urlList.push_back( url1 ) ;

   rc = ds.init( urlList, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ASSERT_EQ( urlList.size(), ds.getNormalAddrNum () ) << "fail to check coord num" ;
	
   checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试本地分配策略
TEST_F( strategyTest9483, multiCoordLocal9489 )
{
   INT32 rc = SDB_OK ;
   conf.setConnectStrategy( SDB_CONN_STY_LOCAL ) ;
   conf.setSyncCoordInterval( 0 ) ;
   vector<string> urlList ;
   urlList.push_back( url ) ;
   urlList.push_back( url1 ) ;

   rc = ds.init( urlList, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ASSERT_EQ( urlList.size(), ds.getNormalAddrNum () ) << "fail to check coord num" ;

   checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试均衡分配策略
TEST_F( strategyTest9483, multiCoordBalance9490 )
{
   INT32 rc = SDB_OK ;
   conf.setConnectStrategy( SDB_CONN_STY_BALANCE ) ;
   conf.setSyncCoordInterval( 0 ) ;
   vector<string> urlList ;
   urlList.push_back( url ) ;
   urlList.push_back( url1 ) ;

   rc = ds.init( urlList, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   ASSERT_EQ( urlList.size(), ds.getNormalAddrNum () ) << "fail to check coord num" ;
	
   checkStartegy( ds ) ;
}
