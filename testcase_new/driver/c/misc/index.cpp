/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1341
 * @Modify:      Liang xuewang Init
 *			 	     2016-11-10
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class indexTest : public testBase
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
      csName = "indexTestCs" ;
      clName = "indexTestCl" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;   
   
      for( INT32 i = 0;i < 1000;++i )
      {
         bson record ;
         bson_init( &record ) ;
         bson_append_int( &record, "id", i ) ;
         bson_append_int( &record, "f1", i+1 ) ;
         bson_append_int( &record, "f2", i+2 ) ;
         bson_finish( &record ) ;
         rc = sdbInsert( cl, &record ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
         bson_destroy( &record ) ;
      }
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollection( cs, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( indexTest, createIndex )
{
   INT32 rc = SDB_OK ;

   // create index
   const CHAR* indexName = "myIndex" ;
   bson indexDef ;
   bson_init( &indexDef ) ;
   bson_append_int( &indexDef, "id", -1 ) ;
   bson_finish( &indexDef ) ;
   rc = sdbCreateIndex1( cl, &indexDef, indexName, false, false, 128 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;
   bson_destroy( &indexDef ) ;

   // get index
   sdbCursorHandle cursor ;
   rc = sdbGetIndexes( cl, indexName, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get index" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_iterator_init( &it, &obj ) ;
   bson_iterator sub ;
   bson_iterator_subiterator( &it, &sub ) ;
   const CHAR* name = bson_iterator_string( &sub ) ;
   ASSERT_STREQ( indexName, name ) << "fail to check index name" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // query record 
   const CHAR* c = "{ id: 555 }" ;
   bson cond ;
   jsonToBson( &cond, c ) ;
   const CHAR* s = "{ id: \"\" }" ;
   bson sel ;
   jsonToBson( &sel, s ) ;
   const CHAR* h = "{ id: 0 }" ;
   bson hint ;
   jsonToBson( &hint, h ) ;
   rc = sdbQuery( cl, &cond, &sel, NULL, &hint, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   CHAR result[100] = { 0 } ;
   bson_sprint( result, sizeof(result), &obj ) ;
   CHAR* expect = "{ \"id\": 555 }" ;
   ASSERT_STREQ( expect, result ) << "fail to check query result" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;   

   bson_destroy( &obj ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &sel ) ;
   bson_destroy( &hint ) ;
   sdbReleaseCursor( cursor ) ;
}
