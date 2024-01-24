/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2507
 * @Modify:      Liang xuewang Init
 *			 	     2017-06-19
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class dateTest : public testBase
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
      csName = "dateTestCs" ;
      clName = "dateTestCl" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;   
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( dateTest, json2Bson )
{
   INT32 rc = SDB_OK ;

   // normal date to insert, [ 0000-01-01, 9999-12-31 ]
   const CHAR* normalDate[] = {
      "{ \"myDate\": { \"$date\": \"0000-01-01\" } }",
      "{ \"myDate\": { \"$date\": \"1840-01-01\" } }",
      "{ \"myDate\": { \"$date\": \"9999-12-31\" } }"
   } ;
   const CHAR* abnormalDate[] = {
      "{ \"myDate\": { \"$date\": \"0000-01-00\" } }",
      "{ \"myDate\": { \"$date\": \"10000-01-01\" } }"
   } ;

   // insert normal date and query
   bson obj, sel, res ;
   bson_init( &sel ) ;
   rc = bson_append_string( &sel, "myDate", "" ) ;
   ASSERT_EQ( BSON_OK, rc ) << "fail to append myDate on sel" ;
   rc = bson_finish( &sel ) ;
   ASSERT_EQ( BSON_OK, rc ) << "fail to finish myDate sel" ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   for( INT32 i = 0;i < sizeof(normalDate)/sizeof(normalDate[0]);i++ )
   {
      rc = sdbDelete( cl, NULL, NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to truncate cl " << clName ;

      bson_init( &obj ) ;
      ASSERT_TRUE( jsonToBson( &obj, normalDate[i] ) ) ;
      rc = sdbInsert( cl, &obj ) ;
      bson_destroy( &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

      rc = sdbQuery( cl, NULL, &sel, NULL, NULL, 0, -1, &cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
      bson_init( &res ) ;
      rc = sdbNext( cursor, &res ) ;
      ASSERT_EQ( rc, SDB_OK ) << "fail to get next" ; 
      CHAR buffer[1024] = { 0 } ;
      ASSERT_TRUE( bsonToJson( buffer, sizeof(buffer), &res, false, false ) ) ;
      ASSERT_STREQ( buffer, normalDate[i] ) ;
      bson_destroy( &res ) ;
      sdbReleaseCursor( cursor ) ;	
   }

   // check abnormal date
   for( INT32 i = 0;i < sizeof(abnormalDate)/sizeof(abnormalDate[0]);i++ )
   {
      bson_init( &obj ) ;
      ASSERT_FALSE( jsonToBson( &obj, abnormalDate[i] ) ) ;
      bson_destroy( &obj ) ;
   }
}

TEST_F( dateTest, mills )
{
   INT32 rc = SDB_OK ;

   bson_date_t mills[] = { 
      -62167248352000,   // 0000-01-01 00:00:00
      253402271999000,   // 9999-12-31 23:59:59
      -62167248353000,   // < 0000-01-01 00:00:00
      253402272000000,   // > 9999-12-31 23:59:59
      -9223372036854775808,  // -2^63
      9223372036854775807    // 2^63-1
   } ;

   // insert mills and query
   bson obj, sel, res ;	
   bson_init( &sel ) ;
   rc = bson_append_string( &sel, "myDate", "" ) ;
   ASSERT_EQ( BSON_OK, rc ) << "fail to append myDate on sel" ;
   rc = bson_finish( &sel ) ;
   ASSERT_EQ( BSON_OK, rc ) << "fail to finish sel" ;

   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   for( INT32 i = 0;i < sizeof(mills)/sizeof(bson_date_t);i++ )
   {
      rc = sdbDelete( cl, NULL, NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to truncate cl " << clName ;

      bson_init( &obj ) ;
      rc = bson_append_date( &obj, "myDate", mills[i] ) ;
      ASSERT_EQ( BSON_OK, rc ) << "fail to append myDate on obj, mills: " << mills[i] ;
      rc = bson_finish( &obj ) ;
      ASSERT_EQ( BSON_OK, rc ) << "fail to finish obj, mills: " << mills[i] ;
      bson_print( &obj ) ;

      rc = sdbInsert( cl, &obj ) ;
      bson_destroy( &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert date " << mills[i] ;

      rc = sdbQuery( cl, NULL, &sel, NULL, NULL, 0, -1, &cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

      bson_init( &res ) ;
      rc = sdbNext( cursor, &res ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;

      bson_print( &res ) ;
      bson_iterator it ;
      bson_find( &it, &res, "myDate" ) ;
      bson_type type = bson_iterator_type( &it ) ;
      ASSERT_EQ( type, BSON_DATE ) ;
      bson_date_t date = bson_iterator_date( &it ) ;
      ASSERT_EQ( date, mills[i] ) ;
      bson_destroy( &res ) ;

      sdbReleaseCursor( cursor ) ;
   }
}	
