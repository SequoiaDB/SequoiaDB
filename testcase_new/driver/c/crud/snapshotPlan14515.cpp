/***************************************************
 * @Description : test case of snapshot AccessPlan 
 *                seqDB-14515:获取访问计划快照 
 * @Modify      : Liang xuewang
 *                2018-02-22
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class snapshotPlanTest14515 : public testBase
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
      csName = "snapshotPlanTestCs14515" ;
      clName = "snapshotPlanTestCl14515" ;
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

TEST_F( snapshotPlanTest14515, AccessPlan )
{
   INT32 rc = SDB_OK ;

   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 10 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   bson_destroy( &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ; 
   
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 10 ) ;
   bson_finish( &cond ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   CHAR clFullName[ 2*MAX_NAME_SIZE+2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;
   bson_init( &cond ) ;
   bson_append_string( &cond, "Collection", clFullName ) ;
   bson_finish( &cond ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_ACCESSPLANS, &cond, NULL, NULL, &cursor ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot AccessPlan" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_print( &obj ) ;
   bson_destroy( &obj ) ;

   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;
}
