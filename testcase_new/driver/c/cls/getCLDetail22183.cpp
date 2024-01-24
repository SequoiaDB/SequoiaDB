/***************************************************
 * @Description : test cl.getDetail()
 * @Modify      : liuxiaoxuan 
 *                seqDB-22183
 *                2020-07-03
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class getCLDetail22183 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   string groupName ; 
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()  
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "getCLDetail22183" ;
      clName = "getCLDetail22183" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;

      if ( isStandalone( db ) ) 
      {   
         printf( "Run mode is standalone\n" ) ; 
         return ;
      }   

      // get group 
      vector<string> groups ;
      rc = getGroups( db, groups ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
      groupName = groups[0] ;

      // create cs cl
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      bson option ;
      bson_init( &option ) ;
      bson_append_string( &option, "Group", groupName.c_str() ) ;
      bson_finish( &option ) ;
      rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
      bson_destroy( &option ) ;

      // insert data
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
      if ( !isStandalone( db ) )
      {
         INT32 rc = SDB_OK ;
         rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }

      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( getCLDetail22183, getCLDetail )
{
   INT32 rc = SDB_OK ;

   if ( isStandalone( db ) ) 
   { 
      printf( "Run mode is standalone\n" ) ;
      return ;    
   }   

   BOOLEAN isExpect = FALSE ;
   // get detail   
   sdbCursorHandle cursor = 0 ; 
   rc = sdbCLGetDetail( cl, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson obj ;
   bson_init ( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;

   bson_iterator it, sub ;
   bson_find( &it, &obj, "Details" ) ; 
   bson_iterator_subiterator( &it, &sub ) ; 
   while( bson_iterator_more( &sub ) ) 
   {   
      bson subObj ;
      bson_init( &subObj ) ; 
      bson_iterator_subobject( &sub, &subObj ) ; 
      bson_iterator i1 ;
      bson_find( &i1, &subObj, "GroupName" ) ; 
      string group = bson_iterator_string( &i1 ) ;
      bson_find( &i1, &subObj, "TotalRecords") ;
      INT32 totalRecords = bson_iterator_int( &i1 ) ;
      if ( group == groupName && 10 == totalRecords )
      {   
         isExpect = TRUE ;
      }   
      else
      {   
         printf( "expect groupName: %s, actual groupName: %s, actual TotalRecords: %d\n", groupName.c_str(), group.c_str(), totalRecords ) ; 
      }   
      bson_destroy( &subObj ) ; 
      bson_iterator_next( &sub ) ; 
   }
   ASSERT_TRUE( isExpect ) ;
                                                                                   
   bson_destroy( &obj );
   sdbReleaseCursor ( cursor ) ;
}
