/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1110
 *               SEQUOIADBMAINSTREAM-849
 * @Modify     : Liang xuewang Init
 *			 	     2016-11-23
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class idIndexTest : public testBase
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
      csName = "idIndexTestCs" ;
      clName = "idIndexTestCl" ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   
      bson options ;
      bson_init( &options ) ;
      bson_append_int( &options, "ReplSize", 0 ) ;
      bson_append_bool( &options, "Compressed", true ) ; 
      bson_append_bool( &options, "AutoIndexId", false ) ;
      bson_finish( &options ) ;
      rc = sdbCreateCollection1( cs, clName, &options, &cl ) ; 
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
      bson_destroy( &options ) ;

      INT32 num = 2000, i ;
      bson* obj[num] ;
      for( i = 0;i < num;++i )
      {
         obj[i] = bson_create() ;
         bson_append_int( obj[i], "_id", i ) ;
         bson_append_int( obj[i], "f1", i+1 ) ;
         bson_append_int( obj[i], "f2", i+2 ) ;
         bson_finish( obj[i] ) ;
      }
      rc = sdbBulkInsert( cl, 0, obj, num ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to bulk insert" ;
      for( i = 0;i < num;++i )
      {
         bson_dispose( obj[i] ) ;
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

TEST_F( idIndexTest, createIdIndex )
{
   INT32 rc = SDB_OK ;

   // create id index
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "SortBufferSize", 128 ) ;
   rc = sdbCreateIdIndex( cl, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create id index" ;
   bson_destroy( &option ) ;

   // get id index
   sdbCursorHandle cursor ;
   rc = sdbGetIndexes( cl, "$id", &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get id index" ;

   // query record 
   const CHAR* c = "{ _id: 555 }" ;
   bson cond ;
   jsonToBson( &cond, c ) ;
   const CHAR* s = "{ _id: \"\" }" ;
   bson sel ;
   jsonToBson( &sel, s ) ;
   const CHAR* h = "{ _id: 0 }" ;
   bson hint ;
   jsonToBson( &hint, h ) ;
   rc = sdbQuery( cl, &cond, &sel, NULL, &hint, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   CHAR result[100] = { 0 } ;
   bson_sprint( result, sizeof(result), &obj ) ;
   CHAR* expect = "{ \"_id\": 555 }" ;
   ASSERT_STREQ( expect, result  ) << "fail to check query" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // explain
   rc = sdbExplain( cl, &cond, &sel, NULL, NULL, 0, 0, -1, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to explain" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "ScanType" ) ;
   const CHAR* scanType = bson_iterator_string( &it ) ;
   ASSERT_STREQ( "ixscan", scanType ) << "fail to check scan type" ;
   bson_find( &it, &obj, "IndexName" ) ;
   const CHAR* indexName =  bson_iterator_string( &it ) ;
   ASSERT_STREQ( "$id", indexName ) << "fail to check index name" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop id index
   rc = sdbDropIdIndex( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop id index" ;

   // update after drop id index
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$inc" ) ;
   bson_append_int( &rule, "f1", 1 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   rc = sdbUpdate( cl, &rule, NULL, NULL ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) << "fail to test update after drop id index" ;

   // destroy bson and release cursor
   bson_destroy( &rule ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &sel ) ;
   bson_destroy( &hint ) ;
   sdbReleaseCursor( cursor ) ;
}
