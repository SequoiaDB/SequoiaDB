/*************************************************************
 * @Desciption: testcase for connectionpool
 *              seqDB-9578:valgrind校验是否有内存泄露
 *              手工测试用例，不加入scons脚本
 * @Modify:     Liangxw
 *              2019-09-05
 *************************************************************/
#include <client.hpp>
#include <sdbConnectionPool.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "connpool_common.hpp"

using namespace sdbclient ;
using namespace std ;

class memTest9578 : public testBase
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
   }   
} ;

// 手工测试用例，用valgrind测试内存泄露
TEST_F( memTest9578, memCheck9578 )
{
   INT32 rc = SDB_OK ;

   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   sdb* conn ;
   CHAR ch ;
   for( INT32 i = 0;;i++ )
   {
      rc = ds.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
      ds.releaseConnection( conn ) ;
      if( i % 10000000 == 0 )
      {
         cout << "continue??[y/n]" << endl ;
	 cin >> ch ;
         if( ch == 'n' )
            break ;
     }
   }
}
