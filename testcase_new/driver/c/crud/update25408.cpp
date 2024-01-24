/*
 * @Description   : SEQUOIADBMAINSTREAM-7980：sdbUpdate2接口测试 seqDB-25408
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
 class update25408 : public testBase
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
      csName = "cs_25408" ;
      clName = "cl_25408" ;
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


TEST_F( update25408, sdbUpdate2 )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "id", 1 ) ;
   bson_finish( &obj );
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 22 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_finish( &rule ) ;
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc )  << "failed to insert, rc=" << rc ;
   
   bson result ;
   bson_init( &result ) ;
   rc = sdbUpdate2( cl, &rule, NULL, NULL, 0, &result ) ;
   ASSERT_EQ ( SDB_OK, rc ) << "failed to update, rc =" << rc ;
   bson_iterator it ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "UpdatedNum" ) ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it) ) ;    
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "ModifiedNum" ) ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it) ) ;    
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "InsertedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &result ) ;
   bson_destroy( &rule ) ;
}
