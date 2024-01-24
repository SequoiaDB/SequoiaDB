/*
 * @Description   : SEQUOIADBMAINSTREAM-6222 C驱动 insert 成功时返回记录数
 * @Author        : XiaoZhenFan
 * @CreateTime    : 2022.04.07
 * @LastEditTime  : 2022.04.08
 * @LastEditors   : XiaoZhenFan
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
class insert26324 : public testBase
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
      clName = "cl_26324" ;
      cs = 0 ;
      cl = 0 ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollection( cs, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop collection" << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( insert26324, sdbInsert2 )
{
   INT32 rc = SDB_OK ;
   bson returnObj ;
   bson_init( &returnObj ) ;
   const INT32 num = 1 ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "id", 1 ) ;  
   bson_finish ( &obj ) ;
   rc = sdbInsert2( cl, &obj, 0, &returnObj ) ;
   ASSERT_EQ ( SDB_OK, rc ) << "failed to insert ,rc =" << rc ;
   bson_iterator it ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "InsertedNum" ) ) ;
   ASSERT_EQ( num, bson_iterator_long( &it ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "DuplicatedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   bson_destroy( &returnObj ) ;
   bson_destroy( &obj ) ;
}

TEST_F( insert26324, sdbBulkInsert2 )
{
   INT32 rc = SDB_OK ;
   bson returnObj ;
   bson_init( &returnObj ) ;

   bson obj[1] ;
   bson_init(&obj[0]) ;
   bson_append_int( &obj[0], "id", 1 ) ;
   bson_finish(&obj[0]) ;
   
   bson *tmp = &obj[0];
   int num = sizeof(obj)/sizeof(obj[0]);
   rc = sdbBulkInsert2( cl, 0, &tmp, num, &returnObj ) ;
   ASSERT_EQ ( SDB_OK, rc ) << "failed to insert ,rc =" << rc ;
   bson_iterator it ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "InsertedNum" ) ) ;
   ASSERT_EQ( num, bson_iterator_long( &it ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "DuplicatedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   bson_destroy( &returnObj ) ;
   bson_destroy( &obj[0] ) ;
}



