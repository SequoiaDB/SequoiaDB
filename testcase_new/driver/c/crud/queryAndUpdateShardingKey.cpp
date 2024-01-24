/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-12639:执行QueryAndUpdate更新分区键
 * @Modify:        Liang xuewang Init
 *                 2017-10-18
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class queryAndUpdateShardingKeyTest : public testBase
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
      csName = "queryAndUpdateShardingKeyTestCs" ;
      clName = "queryAndUpdateShardingKeyTestCl" ;
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

TEST_F( queryAndUpdateShardingKeyTest, queryAndUpdate )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // create split cl
   bson option ;
   bson_init( &option ) ;
   bson_append_start_object( &option, "ShardingKey" ) ;
   bson_append_int( &option, "a", 1 ) ;
   bson_append_finish_object( &option ) ;
   bson_append_string( &option, "ShardingType", "range" ) ;
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
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   // queryAndUpdate ShardingKey
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
   sdbCursorHandle cursor ;
   rc = sdbQueryAndUpdate( cl, &cond, NULL, NULL, NULL, &rule,
                           0, -1, QUERY_KEEP_SHARDINGKEY_IN_UPDATE, TRUE, &cursor ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to queryAndUpdate" ;

   // query old doc before cursor next
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 1 ) ;
   bson_finish( &cond ) ;
   sdbCursorHandle cursor1 ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor1 ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor1, &obj ) ;
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;   // failed in cluster, SDB_DMS_EOC
   rc = sdbCloseCursor( cursor1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor1" ;

   // cursor next
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "a" ) ;
   ASSERT_EQ( 10, bson_iterator_int( &it ) ) << "fail to check a value" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;  

   // query old doc after cursor next
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 1 ) ;
   bson_finish( &cond ) ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to test query after queryAndRemove" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // query new doc after cursor next
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 10 ) ;
   bson_finish( &cond ) ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
