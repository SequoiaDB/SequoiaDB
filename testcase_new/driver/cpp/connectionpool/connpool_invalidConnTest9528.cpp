/**************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9528:init时url不符合格式要求
 * @Modify:      Liangxw
 *               2019-09-05
 **************************************************************/
#include <sdbConnectionPool.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class invalidArgTest9528 : public testing::Test
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;

   void SetUp()
   {
   }
   void TearDown()
   {
      ds.close() ;
   }
} ;

// 地址格式合法但不存在时，init/enable正常返回,getConnection报错
TEST_F( invalidArgTest9528, url9528 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;

	// init and enable connectionpool with not exist url
   string urlWrong = "something:00000" ;
   rc = ds.init( urlWrong, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   
   // get connection
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD, rc ) << "fail to test get connection with not exist url" ;
}
