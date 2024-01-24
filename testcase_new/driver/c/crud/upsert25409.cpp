/*
 * @Description   : SEQUOIADBMAINSTREAM-7980：sdbUpsert3接口测试 seqDB-25409
 * @Author        : xiao zhenfan
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.02.24
 * @LastEditors   : xiao zhenfan
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
 class upsert25409 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* indexName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   void SetUp() 
   {
      testBase::SetUp() ;  
      INT32 rc = SDB_OK ;
      csName = "cs_25409" ;
      clName = "cl_25409" ;
      cs = 0 ;
      cl = 0 ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( upsert25409, sdbUpsert3 )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "id", 1 ) ;
   bson_finish( &obj );
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc )  << "failed to insert, rc=" << rc ;
   bson_destroy( &obj ) ;
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int ( &cond, "id", 1000  ) ;
   bson_finish ( &cond ) ;
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "id", 22 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_finish( &rule ) ;
   bson_init ( &obj ) ;
   bson_append_int( &obj, "age", 22 ) ;
   bson_finish( &obj );
   bson result ;
   bson_init( &result ) ;
   rc = sdbUpsert3( cl, &rule, &cond, NULL, &obj, 0, &result ) ;
   ASSERT_EQ ( SDB_OK, rc ) << "failed to update, rc =" << rc ;
   bson_iterator it ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "UpdatedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;    
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "ModifiedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;    
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "InsertedNum" ) ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it) ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &result ) ;
   bson_destroy( &rule ) ;
}
