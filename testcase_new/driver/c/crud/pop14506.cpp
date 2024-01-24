/***************************************************
 * @Description : test case of pop 
 *                seqDB-14506:pop固定集合的纪录
 * @Modify      : Liang xuewang
 *                2018-02-22
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class popTest14506 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   INT32 logicalID1 ;
   INT32 logicalID2 ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csName = "popTestCs14506" ;
      clName = "popTestCl14506" ;

      // create Capped cs
      bson csOption ;
      bson_init( &csOption ) ;
      bson_append_bool( &csOption, "Capped", true ) ;
      bson_finish( &csOption ) ;
      rc = sdbCreateCollectionSpaceV2( db, csName, &csOption, &cs ) ;
      bson_destroy( &csOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

      // create Capped cl
      bson clOption ;
      bson_init( &clOption ) ;
      bson_append_bool( &clOption, "Capped", true ) ;
      bson_append_int( &clOption, "Size", 1024 ) ;
      bson_append_bool( &clOption, "AutoIndexId", false ) ;
      bson_finish( &clOption ) ;
      rc = sdbCreateCollection1( cs, clName, &clOption, &cl ) ;
      bson_destroy( &clOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

      // insert 2 doc and get logicalID1 logicalID2
      bson doc ;
      bson_init( &doc ) ;
      bson_append_int( &doc, "a", 0 ) ;
      bson_finish( &doc ) ;
      rc = sdbInsert( cl, &doc ) ;
      bson_destroy( &doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
      bson_init( &doc ) ;
      bson_append_int( &doc, "a", 1 ) ;
      bson_finish( &doc ) ;
      rc = sdbInsert( cl, &doc ) ;
      bson_destroy( &doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

      sdbCursorHandle cursor ;   
      bson sort ;
      bson_init( &sort ) ;
      bson_append_int( &sort, "_id", 1 ) ;
      bson_finish( &sort ) ;
      rc = sdbQuery( cl, NULL, NULL, &sort, NULL, 0, -1, &cursor ) ;
      bson_destroy( &sort ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
      bson obj ;
      bson_init( &obj ) ;
      rc = sdbNext( cursor, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
      bson_print( &obj ) ;
      bson_iterator it ;
      bson_find( &it, &obj, "_id" ) ;
      logicalID1 = bson_iterator_long( &it ) ;
      bson_destroy( &obj ) ;

      bson_init( &obj ) ;
      rc = sdbNext( cursor, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
      bson_print( &obj ) ;
      bson_find( &it, &obj, "_id" ) ;
      logicalID2 = bson_iterator_long( &it ) ;
      bson_destroy( &obj ) ;

      printf( "logicalID1: %d, logicalID2: %d\n", logicalID1, logicalID2 ) ;

      rc = sdbCloseCursor( cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
      sdbReleaseCursor( cursor ) ;
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

TEST_F( popTest14506, directionNegative )
{
   INT32 rc = SDB_OK ;

   CHAR clFullName[ 2*MAX_NAME_SIZE+2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "LogicalID", logicalID1 ) ;
   bson_append_int( &option, "Direction", -1 ) ;
   bson_finish( &option ) ;
   rc = sdbPop( cl, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to pop" ;

   // |--doc1--|--doc2--| 
   // LogicalID points to doc1 
   // Direction -1 means pop doc1 and docs after doc1, pop doc1 and doc2
   // so cl will have no doc
   SINT64 count ;
   rc = sdbGetCount( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 0, count ) << "fail to check count" ;
}

TEST_F( popTest14506, directionPositive )
{
   INT32 rc = SDB_OK ;

   CHAR clFullName[ 2*MAX_NAME_SIZE+2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "LogicalID", logicalID2 ) ;
   bson_append_int( &option, "Direction", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbPop( cl, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to pop" ;

   // |--doc1--|--doc2--| 
   // LogicalID points to doc2 
   // Direction 1 means pop doc2 and docs before doc2, pop doc2 and doc1
   // so cl will have no doc
   SINT64 count ;
   rc = sdbGetCount( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 0, count ) << "fail to check count" ;
}
