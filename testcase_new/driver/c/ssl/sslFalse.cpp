/********************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1958
 *				     test ssl with ssl is forbidden in configure file
 * @Modify:		  Liang xuewang Init
 *				     2016-11-10
 ********************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "arguments.hpp"

const CHAR* csName = "testSSLCs" ;
const CHAR* clName = "testSSLCl" ;
sdbConnectionHandle db ;
sdbCSHandle cs ;
sdbCollectionHandle cl ;

// 关闭ssl时，测试sdbConnect连接
TEST( sslFalse, sdbConnect )
{
   INT32 rc = SDB_OK ;

   // 使用sdbSecureConnect连接时出错
   rc = sdbSecureConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_NETWORK, rc ) << "fail to test secure connect when ssl is closed" ;

   CHAR connAddr[200] ;
   sprintf( connAddr, "%s:%s", ARGS->hostName(), ARGS->svcName() ) ;
   const CHAR* connAddrs[1] ;
   connAddrs[0] = connAddr ;
   INT32 arrSize = sizeof(connAddrs) / sizeof(connAddrs[0]) ;
   rc = sdbSecureConnect1( connAddrs, arrSize, ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_NET_CANNOT_CONNECT, rc ) << "fail to test secure connect1 when ssl is closed" ;

   // 使用sdbConnect连接时正常创建集合空间集合
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test connect when ssl is closed" ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // 删除集合空间集合，断开连接
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;

   // 释放句柄
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}
