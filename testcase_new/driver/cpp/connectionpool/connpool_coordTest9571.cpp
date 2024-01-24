/***************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9571:单个coord停止
 *               seqDB-9572:单个coord异常
 *               seqDB-9573:所有coord停止
 *               seqDB-9574:所有coord异常
 *               seqDB-9575:coord所在主机异常
 *               手工测试用例，不加入scons脚本
 * @Modify:      Liangxw
 *               2019-09-05
 ****************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class coordTest9571 : public testBase
{
protected:
   sdbConnectionPoolConf conf ;
   sdbConnectionPool ds ;

   void SetUp()
   {
   }
   void TearDown()
   {
      ds.close() ;
   }
} ;

// 测试单个节点，所有节点正常停止，异常停止，主机异常( 9571-9575 )
TEST_F( coordTest9571, stop9571 )
{
   INT32 rc = SDB_OK ;
   string url1 = "localhost:50000" ;
   // string url2 = "192.168.20.166:11810" ;
   // string url3 = "192.168.20.166:50000" ;
   vector<string> urllist ;
   urllist.push_back( url1 ) ;
   // urllist.push_back( url2 ) ;
   // urllist.push_back( url3 ) ;

   // conf.setSyncCoordInterval( 10 * 1000 ) ;
   conf.setSyncCoordInterval( 0 ) ;
   
   rc = ds.init( urllist, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	
   CHAR flag ;
   for( INT32 i = 0;;i++ )
   {
      sdb* conn = NULL ;
      rc = ds.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
      if( i % 10 == 0 && i != 0 )
      {
         cout << "continue??[y/n]: " ;
         cin >> flag ;
         if( flag == 'n' )
            break ;
      }
   }
}
