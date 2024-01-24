/**************************************************************
 * @Description: test case of $numberLong JSCompatible 
 *				     TestLink 10968  
 * @Modify     : Liang xuewang Init
 *			 	     2017-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class numberLongTest : public testBase
{
protected:
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* csName ;
   const CHAR* clName ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "numberLongTestCs" ;
      clName = "numberLongTestCl" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
   }
} ;

TEST_F( numberLongTest, JSfalse )
{
   INT32 rc = SDB_OK ;

   // insert int/long/double max min 
   INT32 a[]  = { -2147483648, 0, 2147483647 } ;  // -2^31 0 2^31-1
   INT64 b[] = { -9223372036854775808, -9007199254740992, -9007199254740991, 1, 
                  9007199254740991, 9007199254740992, 9223372036854775807 } ; // -2^63 -2^53 -2^53+1 1 2^53-1 2^53 2^63-1
   bson obj ;
   bson_init( &obj ) ;
   for( INT32 i = 0;i < sizeof(a)/sizeof(a[0]);i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "int", i ) ;
      bson_append_int( &obj, key, a[i] ) ;
   }
   for( INT32 i = 0;i < sizeof(b)/sizeof(b[0]);i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "long", i ) ;
      bson_append_long( &obj, key, b[i] ) ;
   }
   bson_finish( &obj ) ;
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   bson_destroy( &obj ) ;

   // query data
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson selector ;
   bson_init( &selector ) ;
   bson_append_start_object( &selector, "_id" ) ;
   bson_append_int( &selector, "$include", 0 ) ;
   bson_append_finish_object( &selector ) ;
   bson_finish( &selector ) ;
   rc = sdbQuery( cl, NULL, &selector, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next in cursor" ;

   const CHAR* expect = "{ \"int0\": -2147483648, \"int1\": 0, \"int2\": 2147483647,"
                        " \"long0\": -9223372036854775808, \"long1\": -9007199254740992,"
                        " \"long2\": -9007199254740991, \"long3\": 1, \"long4\": 9007199254740991,"
                        " \"long5\": 9007199254740992, \"long6\": 9223372036854775807 }" ; 
   CHAR real[1024] ;
   bson_sprint( real, 1024, &obj ) ;
   ASSERT_STREQ( expect, real ) << "fail to check query" ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
}

TEST_F( numberLongTest, JStrue )
{
   INT32 rc = SDB_OK ;

   bson_set_js_compatibility( true ) ;

   // insert int/long/double max min 
   INT32  a[] = { -2147483648, 0, 2147483647 } ;  // -2^31 0 2^31-1
   SINT64 b[] = { -9223372036854775808, -9007199254740992, -9007199254740991, 1,
                  9007199254740991, 9007199254740992, 9223372036854775807 } ; // -2^63 -2^53 -2^53+1 1 2^53-1 2^53 2^63-1
   bson obj ;
   bson_init( &obj ) ;
   for( INT32 i = 0;i < sizeof(a)/sizeof(a[0]);i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "int", i ) ;
      bson_append_int( &obj, key, a[i] ) ;
   }
   for( INT32 i = 0;i < sizeof(b)/sizeof(b[0]);i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "long", i ) ;
      bson_append_long( &obj, key, b[i] ) ;
   }
   bson_finish( &obj ) ;
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   bson_destroy( &obj ) ;

   // query data
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson selector ;
   bson_init( &selector ) ;
   bson_append_start_object( &selector, "_id" ) ;
   bson_append_int( &selector, "$include", 0 ) ;
   bson_append_finish_object( &selector ) ;
   bson_finish( &selector ) ;
   rc = sdbQuery( cl, NULL, &selector, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next in cursor" ;
   const CHAR* expect = "{ \"int0\": -2147483648, \"int1\": 0, \"int2\": 2147483647,"
                        " \"long0\": { \"$numberLong\": \"-9223372036854775808\" },"
                        " \"long1\": { \"$numberLong\": \"-9007199254740992\" },"
                        " \"long2\": -9007199254740991, \"long3\": 1, \"long4\": 9007199254740991,"
                        " \"long5\": { \"$numberLong\": \"9007199254740992\" },"
                        " \"long6\": { \"$numberLong\": \"9223372036854775807\" } }" ;
   CHAR real[1024] ;
   bson_sprint( real, 1024, &obj ) ;
   ASSERT_STREQ( expect, real ) << "fail to check query data" ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
}
