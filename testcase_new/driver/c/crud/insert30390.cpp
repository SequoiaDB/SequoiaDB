/********************************************************************************************
 * @Description   : seqDB-30390:insert新增flagSDB_INSERT_CONTONDUP_ID/SDB_INSERT_REPLACEONDUP_ID
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.03.08
 * @LastEditTime  : 2023.03.08
 * @LastEditors   : Cheng Jingjing
 ********************************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class insert30390 : public testBase
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
      csName = "cs_30390" ;
      clName = "cl_30390" ;
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
};

TEST_F( insert30390, sdbInsert1 )
{
   INT32 rc = SDB_OK ;

   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "_id", 1 ) ;
   bson_append_string( &doc, "b", "bbb" ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   bson_destroy( &doc ) ;

   bson ignore ;
   bson_init( &ignore );
   bson_append_int( &ignore, "_id", 1 ) ;
   bson_append_string( &ignore, "b", "aaa" ) ;
   bson_finish( &ignore ) ;
   rc = sdbInsert2( cl, &ignore, FLG_INSERT_CONTONDUP_ID, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   bson_destroy( &ignore ) ;

   // get the record num
   SINT64 count  = 0 ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) ;

   // get query result
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "_id", 1 ) ;
   bson_finish( &cond ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "b" ) ;
   ASSERT_STREQ( "bbb", bson_iterator_string( &it ) ) ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
}

TEST_F( insert30390, sdbInsert2 )
{
   INT32 rc = SDB_OK ;

   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "_id", 1 ) ;
   bson_append_string( &doc, "b", "bbb" ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   bson_destroy( &doc ) ;

   bson replace ;
   bson_init( &replace );
   bson_append_int( &replace, "_id", 1 ) ;
   bson_append_string( &replace, "b", "aaa" ) ;
   bson_finish( &replace ) ;
   rc = sdbInsert2( cl, &replace, FLG_INSERT_REPLACEONDUP_ID, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   bson_destroy( &replace ) ;

   // get the record num
   SINT64 count  = 0 ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) ;

   // get query result
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "_id", 1 ) ;
   bson_finish( &cond ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "b" ) ;
   ASSERT_STREQ( "aaa", bson_iterator_string( &it ) ) ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
}