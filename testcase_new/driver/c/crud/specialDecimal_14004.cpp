/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-14004:插入特殊decimal值
 *                 seqDB-14005:删除特殊decimal值
 * @Modify:        Liang xuewang Init
 *                 2018-01-06
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class specialDecimalTest : public testBase
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
      csName = "specialDecimalTestCs_14004" ;
      clName = "specialDecimalTestCl_14004" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
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

TEST_F( specialDecimalTest, min )
{
   INT32 rc = SDB_OK ;

   bson_decimal dec ;
   decimal_init( &dec ) ;
   decimal_set_min( &dec ) ;
   rc = decimal_is_min( &dec ) ;
   ASSERT_TRUE( rc ) << "fail to check min" ;
      
   bson doc ;
   bson_init( &doc ) ;
   bson_append_decimal( &doc, "a", &dec ) ;
   bson_finish( &doc ) ;

   // insert min decimal 
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert min" ;

   // query
   sdbCursorHandle cursor ;        
   rc = sdbQuery( cl, &doc, NULL, NULL, NULL, 0, -1, &cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ); 
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "a" ) ;
   bson_decimal res ;
   decimal_init( &res ) ;
   bson_iterator_decimal( &it, &res ) ;
   rc = decimal_is_min( &res ) ;
   ASSERT_TRUE( rc ) << "fail to check query res" ;
   decimal_free( &res ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   // remove min decimal
   rc = sdbDelete( cl, &doc, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete" ;
   rc = sdbQuery( cl, &doc, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check query after delete" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   decimal_free( &dec ) ;
   bson_destroy( &doc ) ;
}

TEST_F( specialDecimalTest, max )
{
   INT32 rc = SDB_OK ;
   
   bson_decimal dec ;
   decimal_init( &dec ) ;
   decimal_set_max( &dec ) ;
   rc = decimal_is_max( &dec ) ;
   ASSERT_TRUE( rc ) << "fail to check max" ;
      
   bson doc ;
   bson_init( &doc ) ;
   bson_append_decimal( &doc, "a", &dec ) ;
   bson_finish( &doc ) ;

   // insert max decimal 
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert min" ;

   // query
   sdbCursorHandle cursor ;        
   rc = sdbQuery( cl, &doc, NULL, NULL, NULL, 0, -1, &cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ); 
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "a" ) ;
   bson_decimal res ;
   decimal_init( &res ) ;
   bson_iterator_decimal( &it, &res ) ;
   rc = decimal_is_max( &res ) ;
   ASSERT_TRUE( rc ) << "fail to check query res" ;
   decimal_free( &res ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   // remove max decimal
   rc = sdbDelete( cl, &doc, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete" ;
   rc = sdbQuery( cl, &doc, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check query after delete" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   decimal_free( &dec ) ;
   bson_destroy( &doc ) ;
}

TEST_F( specialDecimalTest, nan )
{
   INT32 rc = SDB_OK ;
   
   bson_decimal dec ;
   decimal_init( &dec ) ;
   decimal_set_nan( &dec ) ;
   rc = decimal_is_nan( &dec ) ;
   ASSERT_TRUE( rc ) << "fail to check nan" ;
      
   bson doc ;
   bson_init( &doc ) ;
   bson_append_decimal( &doc, "a", &dec ) ;
   bson_finish( &doc ) ;

   // insert nan decimal 
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert min" ;

   // query
   sdbCursorHandle cursor ;        
   rc = sdbQuery( cl, &doc, NULL, NULL, NULL, 0, -1, &cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ); 
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "a" ) ;
   bson_decimal res ;
   decimal_init( &res ) ;
   bson_iterator_decimal( &it, &res ) ;
   rc = decimal_is_nan( &res ) ;
   ASSERT_TRUE( rc ) << "fail to check query res" ;
   decimal_free( &res ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   // remove nan decimal
   rc = sdbDelete( cl, &doc, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete" ;
   rc = sdbQuery( cl, &doc, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check query after delete" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;

   decimal_free( &dec ) ;
   bson_destroy( &doc ) ;
}
