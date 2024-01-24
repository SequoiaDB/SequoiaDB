/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-18581: insert 重复键时覆盖记录
 * @Modify:        liuxiaoxuan Init
 *                 2019-06-18
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class insertReplaceOnDup18581 : public testBase
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
      csName = "insertReplaceOnDup18581Cs" ;
      clName = "insertReplaceOnDup18581Cl" ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = sdbCreateCollection( cs, clName, &cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
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

TEST_F( insertReplaceOnDup18581, uniqueIndex )
{
   INT32 rc = SDB_OK ;

   bson doc ;
   bson ret ;
   bson_init( &doc ) ;
   bson_init( &ret );
   bson_append_int( &doc, "_id", 1 ) ;
   bson_append_string( &doc, "b", "b1" ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert2( cl, &doc, FLG_INSERT_REPLACEONDUP, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ; 
   bson_destroy( &doc ) ;
   bson_destroy( &ret );
   bson_init( &doc ) ;
   bson_init( &ret );
   bson_append_int( &doc, "_id", 1 ) ;
   bson_append_string( &doc, "b", "b2" ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert2( cl, &doc, FLG_INSERT_REPLACEONDUP, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ; 
   bson_destroy( &doc ) ;
   bson_destroy( &ret );
   
   // get the record num
   SINT64 count  = 0 ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count";
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
   ASSERT_STREQ( "b2", bson_iterator_string( &it ) ) ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
}
