/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-13023:使用sdbGetQueryMeta获取查询元数据     
 * @Modify:        Liang xuewang Init
 *                 2017-10-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class queryMetaTest : public testBase
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
      csName = "queryMetaTestCs" ;
      clName = "queryMetaTestCl" ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = sdbCreateCollection( cs, clName, &cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
      bson idxDef ;
      bson_init( &idxDef ) ;
      bson_append_int( &idxDef, "a", 1 ) ;
      bson_finish( &idxDef ) ;
      rc = sdbCreateIndex( cl, &idxDef, "aIndex", false, false ) ;
      bson_destroy( &idxDef ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;
      bson doc ;
      bson_init( &doc ) ;
      bson_append_int( &doc, "a", 10 ) ;
      bson_finish( &doc ) ;
      rc = sdbInsert( cl, &doc ) ;
      bson_destroy( &doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ; 
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

TEST_F( queryMetaTest, nullHint )
{
   INT32 rc = SDB_OK ;

   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 10 ) ;
   bson_finish( &cond ) ;
   sdbCursorHandle cursor ;
   rc = sdbGetQueryMeta( cl, &cond, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "ScanType" ) ;
   ASSERT_STREQ( "ixscan", bson_iterator_string( &it ) ) ;
   ASSERT_NE( BSON_EOO, bson_find( &it, &obj, "Indexblocks" ) ) ;
   // bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( queryMetaTest, emptyHint )
{
   INT32 rc = SDB_OK ;
   
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 10 ) ;
   bson_finish( &cond ) ;
   bson hint ;
   bson_init( &hint ) ;
   bson_finish( &hint ) ;
   sdbCursorHandle cursor ;
   rc = sdbGetQueryMeta( cl, &cond, NULL, &hint, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &hint ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "ScanType" ) ;
   ASSERT_STREQ( "ixscan", bson_iterator_string( &it ) ) ;
   ASSERT_NE( BSON_EOO, bson_find( &it, &obj, "Indexblocks" ) ) ;
   // bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( queryMetaTest, withHint )
{
   INT32 rc = SDB_OK ;
   
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 10 ) ;
   bson_finish( &cond ) ;
   bson hint ;
   bson_init( &hint ) ;
   bson_append_null( &hint, "" ) ;
   bson_finish( &hint ) ;
   sdbCursorHandle cursor ;
   rc = sdbGetQueryMeta( cl, &cond, NULL, &hint, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &hint ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "ScanType" ) ;
   ASSERT_STREQ( "tbscan", bson_iterator_string( &it ) ) ;
   ASSERT_NE( BSON_EOO, bson_find( &it, &obj, "Datablocks" ) ) ;
   // bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
