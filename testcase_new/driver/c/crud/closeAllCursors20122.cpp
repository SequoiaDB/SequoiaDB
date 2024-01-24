/***************************************************
 * @Description : test sdbCloseAllCursors 
 * @Modify      : liuxiaoxuan 
 *                seqDB-20122
 *                2019-10-30
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class closeAllCursors20122 : public testBase
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
      csName = "closeAllCursors20122" ;
      clName = "closeAllCursors20122" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ; 
      ASSERT_EQ( SDB_OK, rc ) ; 

      // insert datas
      bson* docs[10] ; 
      for( INT32 i = 0; i < 10; i++ )
      {   
         docs[i] = bson_create() ;
         bson_append_int( docs[i], "a", i ) ; 
         bson_finish( docs[i] ) ; 
      }   
      rc = sdbBulkInsert( cl, 0, docs, 10 ) ; 
      ASSERT_EQ( SDB_OK, rc ) ; 
      for( INT32 i = 0; i < 10; i++ )
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

TEST_F( closeAllCursors20122, closeAllCursors )
{
   INT32 rc = SDB_OK ;

   if ( isStandalone( db ) ) 
   { 
      printf( "Run mode is standalone\n" ) ;
      return ;    
   }   

   sdbCursorHandle cursor = 0 ;
   sdbCursorHandle cursor1 = 0 ;
   sdbCursorHandle cursor2 = 0 ;
            
   bson obj ;
   bson obj1 ;
   bson obj2 ;
                             
   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get next
   bson_init ( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
                                                                                   
   // close all the cursors
   rc = sdbCloseAllCursors( db );
   ASSERT_EQ( SDB_OK, rc ) ;
   // check get next failed
   rc = sdbCurrent( cursor, &obj ) ; 
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   bson_init ( &obj1 ) ;
   rc = sdbNext( cursor1, &obj1 ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   bson_init ( &obj2 ) ;
   rc = sdbNext( cursor2, &obj2 ) ; 
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   bson_destroy( &obj );
   bson_destroy( &obj1 );
   bson_destroy( &obj2 );
   
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
}
