/*******************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9580:keepalive设置后，时间跳变
 *               手工测试用例，不加入scons脚本
 * @Modify:      Liangxw
 *               2019-09-05
 *******************************************************/
#include <sdbConnectionPool.hpp>
#include <gtest/gtest.h>
#include <client.hpp>
#include "connpool_common.hpp"

using namespace sdbclient ;

// 手动测试用例，时间跳变测试连接超时
class timeJumpTest9580 : public testing::Test
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

TEST_F( timeJumpTest9580, jump9580 )
{
   INT32 rc = SDB_OK ;
   conf.setCheckIntervalInfo( 3, 6 ) ;

   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;

   // 使用date命令使时间跳变
   CHAR flag ;
   while( true )
   {
      cout << "wait signal, please input( y/n ):" ;
      cin >> flag ;
      if( flag == 'n' )
         break ;
   }
   
   ds.releaseConnection( conn ) ;
   ASSERT_EQ( 0, ds.getIdleConnNum() ) << "fail to check idle conn num after time jump" ;
}
