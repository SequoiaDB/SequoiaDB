/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-12638:执行update更新分区键
 * @Modify:        Liang xuewang Init
 *                 2017-10-18
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class updateShardingKeyTest : public testBase
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
      csName = "updateShardingKeyTestCs" ;
      clName = "updateShardingKeyTestCl" ;
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

TEST_F( updateShardingKeyTest, update )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   
   // get data groups
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( groups.size() < 2 )
   {
      printf( "data groups num: %uld, too few\n", groups.size() ) ;
      return ;
   } 
   const CHAR* srcGroup = groups[0].c_str() ;
   const CHAR* dstGroup = groups[1].c_str() ;

   // create split cl
   bson option ;
   bson_init( &option ) ;
   bson_append_start_object( &option, "ShardingKey" ) ;
   bson_append_int( &option, "a", 1 ) ;
   bson_append_finish_object( &option ) ;
   bson_append_string( &option, "ShardingType", "range" ) ;
   bson_append_string( &option, "Group", srcGroup ) ;
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
   
   // update ShardingKey before split
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 1 ) ;
   bson_finish( &cond ) ;
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", 20 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   rc = sdbUpdate1( cl, &rule, &cond, NULL, UPDATE_KEEP_SHARDINGKEY ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update" ;

   // check update
   sdbCursorHandle cursor ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 20 ) ;
   bson_finish( &cond ) ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // split cl
   bson begin ;
   bson_init( &begin ) ;
   bson_append_int( &begin, "a", 10 ) ;
   bson_finish( &begin ) ;
   bson end ;
   bson_init( &end ) ;
   bson_append_int( &end, "a", 30 ) ;
   bson_finish( &end ) ;
   rc = sdbSplitCollection( cl, srcGroup, dstGroup, &begin, &end ) ;
   bson_destroy( &begin ) ;
   bson_destroy( &end ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to split cl" ;

   // update ShardingKey after split
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 20 ) ;
   bson_finish( &cond ) ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", 15 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   rc = sdbUpdate1( cl, &rule, &cond, NULL, UPDATE_KEEP_SHARDINGKEY ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_UPDATE_SHARD_KEY, rc ) << "fail to test update after split" ;
}
