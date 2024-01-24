/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1958
 *				     test ssl with ssl is allowed in configure file
 * @Modify:      Liang xuewang Init
 *               2016-11-10
 **************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const CHAR* csName = "sslTestCs" ;
const CHAR* clName = "sslTestCl" ;
const CHAR* flag = "FALSE" ;
sdbConnectionHandle db1 ;
sdbCSHandle cs ;
sdbCollectionHandle cl ;

class sslTrue : public testBase
{
protected:
   
   void updateConf( bool value)
   {
         bson config ;
         bson_init( &config ) ;
         bson_append_bool( &config, "usessl", value ) ;
         bson_finish( &config ) ;
         INT32 rc = sdbUpdateConfig( db, &config, NULL ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to update conf" ;
   }

   void SetUp()
   {
      testBase::SetUp() ;

      //检查ssl是否开启，未开启则开启ssl
      INT32 rc ; 
      sdbCursorHandle cursor ;
      rc = sdbGetSnapshot(db, SDB_SNAP_CONFIGS, NULL, NULL, NULL, &cursor) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot trans" ;
      bson obj ;
      bson_init( &obj ) ;
      rc = sdbNext( cursor, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
      bson_iterator it ;
      ASSERT_NE( BSON_EOO, bson_find( &it, &obj, "usessl" ) ) ;
      if ( strcmp(bson_iterator_string( &it ), flag) == 0 )
      {
         flag = "TRUE" ;
         updateConf( true ) ;
      }

      bson_destroy( &obj ) ;      
      rc = sdbCloseCursor( cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   }

   void TearDown()
   {
      //恢复环境
      if ( strcmp(flag, "TRUE") == 0 )
      {
         flag = "FALSE" ;
         updateConf( false ) ;
      }

      testBase::TearDown() ;
   }
};

// 测试开启ssl，使用sdbSecureConnect正常连接创建集合空间、集合
TEST_F( sslTrue, sdbSecureConnect )
{
   INT32 rc = SDB_OK ;
   // 使用ssl连接sdb并创建集合空间、集合
   rc = sdbSecureConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to secure connect when ssl is open" ;
   rc = sdbCreateCollectionSpace( db1, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // 删除集合、集合空间、断开连接
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;

   rc = sdbDropCollectionSpace( db1, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   sdbDisconnect( db1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db1 ) ;
}

// 测试开启ssl，使用sdbSecureConnect1正常连接创建集合空间、集合
TEST_F( sslTrue, sdbSecureConnect1 )
{
   INT32 rc = SDB_OK ;

   // 使用ssl连接sdb并创建集合空间、集合
   CHAR connAddr[200] ;
   sprintf( connAddr, "%s:%s", ARGS->hostName(), ARGS->svcName() ) ;
   const CHAR* connAddrs[1] ;
   connAddrs[0] = connAddr ;
   INT32 arrSize = sizeof(connAddrs) / sizeof(connAddrs[0]) ;
   rc = sdbSecureConnect1( connAddrs, arrSize, ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to secure connect when ssl is open" ;

   rc = sdbCreateCollectionSpace( db1, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // 删除集合、集合空间、断开连接
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
   rc = sdbDropCollectionSpace( db1, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   sdbDisconnect( db1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db1 ) ;
}
