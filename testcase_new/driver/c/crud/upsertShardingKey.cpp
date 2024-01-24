/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-12640:执行upsert更新分区键
 * @Modify:        Liang xuewang Init
 *                 2017-10-18
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class upsertShardingKeyTest : public testBase
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
      csName = "upsertShardingKeyTestCs" ;
      clName = "upsertShardingKeyTestCl" ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( cl ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( upsertShardingKeyTest, upsertMatch )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // create hash split cl
   bson option ;
   bson_init( &option ) ;
   bson_append_start_object( &option, "ShardingKey" ) ;
   bson_append_int( &option, "a", 1 ) ;
   bson_append_finish_object( &option ) ;
   bson_finish( &option ) ;
   rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ; 

   // insert doc
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   bson_destroy( &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   // upsert with cond match exist
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 1 ) ;
   bson_finish( &cond ) ;
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", 10 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   rc = sdbUpsert2( cl, &rule, &cond, NULL, NULL, UPDATE_KEEP_SHARDINGKEY ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;

   // check upsert
   SINT64 count ;
   rc = sdbGetCount( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "a" ) ;
   ASSERT_EQ( 10, bson_iterator_int( &it ) ) << "fail to check query result" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}

TEST_F( upsertShardingKeyTest, upsertNotMatch )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // create hash split cl
   bson option ;
   bson_init( &option ) ;
   bson_append_start_object( &option, "ShardingKey" ) ;
   bson_append_int( &option, "a", 1 ) ;
   bson_append_finish_object( &option ) ;
   bson_finish( &option ) ;
   rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ; 

   // insert doc
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   bson_destroy( &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   // upsert with cond match not exist
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 2 ) ;
   bson_finish( &cond ) ;
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", 10 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   bson setOnInsert ;
   bson_init( &setOnInsert ) ;
   bson_append_int( &setOnInsert, "a", 5 ) ;
   bson_finish( &setOnInsert ) ;
   rc = sdbUpsert2( cl, &rule, &cond, NULL, &setOnInsert, UPDATE_KEEP_SHARDINGKEY ) ;
   bson_destroy( &setOnInsert ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;

   // check upsert
   SINT64 count ;
   rc = sdbGetCount( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 5 ) ;
   bson_finish( &cond ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}
