/*
 * @Description   : SEQUOIADBMAINSTREAM-7492 C驱动返回CONTEXT的COMMAND支持RETURNWITHDATA优化 seqDB-26398
 * @Author        : XiaoZhenFan
 * @CreateTime    : 2022.04.19
 * @LastEditTime  : 2022.04.20
 * @LastEditors   : XiaoZhenFan
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
class list26398 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   void SetUp() 
   {
      testBase::SetUp() ;  
      INT32 rc = SDB_OK ;
      csName = "ccasecommoncs" ;
      clName = "cl_26398" ;
      cs = 0 ;
      cl = 0 ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollection( cs, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop collection" << clName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

long getCurrentProcessEventCount( sdbConnectionHandle db )
{
      INT32 rc = SDB_OK ;
      bson obj ;
      sdbCursorHandle cursor ;
      rc = sdbGetSnapshot( db, SDB_SNAP_SESSIONS_CURRENT, NULL, NULL, NULL, &cursor ) ;
      if( SDB_OK != rc )
       {
          std::cout << "fail to dbGetSnapshot SDB_SNAP_SESSIONS_CURRENT,rc = " << rc ;
          return  -1 ;
       }
      bson_init( &obj ) ;
      rc = sdbNext( cursor, &obj ) ;
      if( SDB_OK != rc )
       {
          std::cout << "fail to next" << rc ;
          return  -1 ;
       }

      bson_iterator it ;
      bson_type type = bson_find( &it, &obj, "ProcessEventCount" ) ;
      if( BSON_LONG != type )
       {
          std::cout << "type of 'ProcessEventCount' should be 'BSON_LONG' " << std::endl ;
          return  -1 ;
       }
      long ProcessEventCount = bson_iterator_long( &it ) ;

      bson_destroy( &obj ) ;
      sdbCloseCursor( cursor ) ;
      return ProcessEventCount ;
}

TEST_F( list26398, RETURNWITHDATA )
{
      INT32 rc = SDB_OK ;
      INT32 expDiff = 2 ;
      bson obj ;
      bson_init( &obj ) ;
      bson_append_int( &obj, "id", 1 ) ;  
      bson_finish ( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ ( SDB_OK, rc ) << "failed to insert ,rc =" << rc ;
      bson_destroy( &obj ) ;

      long ProcessEventCount1 = getCurrentProcessEventCount( db )  ;
      SINT64 totalNum = 0 ;
      rc = sdbGetCount ( cl, NULL, &totalNum ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      long ProcessEventCount2 = getCurrentProcessEventCount( db )  ;
      //ProcessEventCount增加了2，其中1次是count产生，1次是snapshot产生
      ASSERT_EQ( expDiff, ProcessEventCount2-ProcessEventCount1 ) ;

      sdbCursorHandle cursor ;
      rc = sdbGetList( db, SDB_SNAP_SESSIONS_CURRENT, NULL, NULL, NULL, &cursor ) ;
      ASSERT_EQ ( SDB_OK, rc ) << "failed to get list ,rc =" << rc ;
      long ProcessEventCount3 = getCurrentProcessEventCount( db )  ;
      //ProcessEventCount增加了2，其中1次是list产生，1次是snapshot产生
      ASSERT_EQ( expDiff, ProcessEventCount3-ProcessEventCount2 ) ;
      sdbCloseCursor( cursor ) ;
}
