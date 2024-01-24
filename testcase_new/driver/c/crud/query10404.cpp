/***************************************************
 * @Description : test insert/update/delete/query 
 * @Modify      : liu xiaoxuan 
 *                seqDB-10404
 *                2019-10-09
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class query10404 : public testBase
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
      csName = "query10404" ;
      clName = "query10404" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ; 
      ASSERT_EQ( SDB_OK, rc ) ; 

      // insert datas
      bson* docs[5] ; 
      for( INT32 i = 0; i < 5; i++ )
      {   
         docs[i] = bson_create() ;
         bson_append_int( docs[i], "a", i ) ; 
         bson_finish( docs[i] ) ; 
      }   
      rc = sdbBulkInsert( cl, 0, docs, 5 ) ; 
      ASSERT_EQ( SDB_OK, rc ) ; 
      for( INT32 i = 0; i < 5; i++ )
      {   
         bson_dispose( docs[i] ) ; 
      }   

   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( query10404, QueryWithFlag )
{
   INT32 rc = SDB_OK ;
  
   sdbCursorHandle cursor ;
   // QUERY_EXPLAIN: 1024
   bson orderby ;
   bson_init ( &orderby ) ;
   bson_append_int( &orderby, "a", 1 ) ;
   bson_finish( &orderby ) ;
   rc = sdbQuery1( cl, NULL, NULL, &orderby, NULL, 0, 2, 1024, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &orderby) ;

   // check result
   INT32 count = 0 ;
   vector<INT32> actVals;
   bson ret ;
   bson_init( &ret ) ;
   while( !sdbNext( cursor, &ret ) )
   {
      count++ ;
      bson_iterator it ; 
      bson_find( &it, &ret, "a" ) ;
      INT32 val = bson_iterator_int( &it ) ;
      actVals.push_back( val ) ;
      bson_destroy( &ret ) ;
      bson_init( &ret ) ;
   }
   ASSERT_EQ( 2, count ) ;
   ASSERT_EQ( 0, actVals[0] ) ;
   ASSERT_EQ( 1, actVals[1] ) ;

   bson_destroy( &ret ) ;
   sdbCloseCursor( cursor ) ; 
   sdbReleaseCursor( cursor ) ;
}

TEST_F( query10404, Update )
{
   INT32 rc = SDB_OK ;
   // update
   bson rule ;
   bson cond ;
   bson_init( &rule ) ;
   bson_init( &cond ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", -1 ) ;
   bson_append_finish_object( &rule ) ;
   bson_append_int ( &cond, "a", 0 ) ;
   bson_finish( &rule ) ;
   bson_finish( &cond ) ;
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &cond ) ;

   // check result
   INT32 count = 0 ;
   sdbCursorHandle cursor ;
   bson_init( &cond ) ;
   bson_append_int ( &cond, "a", -1 ) ;
   bson_finish( &cond ) ;
   rc = sdbQuery1( cl, &cond, NULL, NULL, NULL, 0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &cond ) ;

   bson ret ;
   bson_init( &ret ) ;
   while( !sdbNext( cursor, &ret ) )
   {  
      count++ ;
      bson_iterator it ; 
      bson_find( &it, &ret, "a" ) ;
      INT32 val = bson_iterator_int( &it ) ;
      ASSERT_EQ( -1, val ) ;
      bson_destroy( &ret ) ;
      bson_init( &ret ) ;
   }           
   bson_destroy( &ret ) ;
   ASSERT_EQ( 1, count ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;
}

TEST_F( query10404, Delete)
{              
   INT32 rc = SDB_OK ;
   // remove 
   bson cond ;
   bson_init( &cond ) ;
   bson_append_start_object( &cond, "a" ) ;
   bson_append_int ( &cond, "$gt", 0 ) ;
   bson_append_finish_object( &cond );
   bson_finish( &cond ) ;
   rc = sdbDelete( cl, &cond, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &cond ) ;
                  
   // check result
   INT32 count = 0 ;
   sdbCursorHandle cursor ;
   rc = sdbQuery1( cl, NULL, NULL, NULL, NULL, 0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson ret ;
   bson_init( &ret ) ;
   while( !sdbNext( cursor, &ret ) )
   {
      count++ ;
      bson_iterator it ;
      bson_find( &it, &ret, "a" ) ;
      INT32 val = bson_iterator_int( &it ) ;
      ASSERT_EQ( 0, val ) ; 
      bson_destroy( &ret ) ; 
      bson_init( &ret ) ; 
   }                                    
   bson_destroy( &ret ) ;
   ASSERT_EQ( 1, count ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;                                                           
} 
