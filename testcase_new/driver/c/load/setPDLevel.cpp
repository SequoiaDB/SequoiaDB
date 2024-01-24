/**************************************************************
 * @Description: test case for Jira questionaire Task
 *				     SEQUOIADBMAINSTREAM-2165
 *				     seqDB-11002:setPDLevel
 *				     手工测试：设置PDLevel后执行操作检查节点日志
 * @Modify     : Liang xuewang Init
 *			 	     2017-01-22
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "arguments.hpp"

sdbConnectionHandle db = SDB_INVALID_HANDLE ;
INT32 pdLevel = 5 ;   // 0 SERVER 1 ERROR 2 EVENT 3 WARNING 4 INFO 5 DEBUG 默认为3

TEST( setPDLevel, debugLevel )
{
   INT32 rc = SDB_OK ;

   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;

   // 设置节点sdbserver1:50000的日志级别
   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "Global", false ) ;
   bson_finish( &option ) ;
   rc = sdbSetPDLevel( db, pdLevel, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to set pd level" ;

   // 执行断开连接操作时检查节点日志是否有DEBUG信息
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}
