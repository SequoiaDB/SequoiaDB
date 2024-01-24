#include <stdio.h>
#include <gtest/gtest.h>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "client.h"

class listTest: public testBase
{
protected:
    void SetUp()  
    {
       testBase::SetUp() ;
    }
    void TearDown()
    {
       testBase::TearDown() ;
    }
    
    void checkListResult ( INT32 TYPE, bson* matcher, bson* expectObj ) ;
} ;

TEST_F(listTest, sdbGetList_SDB_LIST_CONTEXTS_22061)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbGetList( db, SDB_LIST_CONTEXTS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ;
   if( SDB_DMS_EOC != rc )
   {
      bson empty;
      bson_init( &empty ) ;
      bson_finish( &empty ) ;
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_long( &expectObj, "SessionID", 1 ) ;
      bson_append_array( &expectObj, "Contexts", &empty ) ;
      bson_finish( &expectObj ) ;
      checkListResult( SDB_LIST_CONTEXTS, NULL, &expectObj ) ;

      bson_destroy( &empty ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(listTest, sdbGetList_SDB_LIST_CONTEXTS_CURRENT_22061)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbGetList( db, SDB_LIST_CONTEXTS_CURRENT, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ;
   if( SDB_DMS_EOC != rc )
   {
      bson empty;
      bson_init( &empty ) ;
      bson_finish( &empty ) ;
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_long( &expectObj, "SessionID", 1 ) ;
      bson_append_array( &expectObj, "Contexts", &empty ) ;
      bson_finish( &expectObj ) ;
      checkListResult( SDB_LIST_CONTEXTS_CURRENT, NULL, &expectObj ) ;

      bson_destroy( &empty ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(listTest, sdbGetList_SDB_LIST_SESSIONS_22062)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbGetList( db, SDB_LIST_SESSIONS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ;
   if( SDB_DMS_EOC != rc )
   {
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_long( &expectObj, "SessionID", 1 ) ;
      bson_append_int( &expectObj, "TID", 1 ) ;
      bson_finish( &expectObj ) ;
      checkListResult( SDB_LIST_SESSIONS, NULL, &expectObj ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(listTest, sdbGetList_SDB_LIST_SESSIONS_CURRENT_22062)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbGetList( db, SDB_LIST_SESSIONS_CURRENT, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ;
   if( SDB_DMS_EOC != rc )
   {   
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_long( &expectObj, "SessionID", 1 ) ;
      bson_append_int( &expectObj, "TID", 1 ) ;
      bson_finish( &expectObj ) ;
      checkListResult( SDB_LIST_SESSIONS_CURRENT, NULL, &expectObj ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(listTest, sdbGetList_SDB_LIST_COLLECTIONS_AND_COLLECTIONSPACES_22063)
{
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   INT32 rc                       = SDB_OK ;

   // create cl
   const CHAR* csName             = "list_collectionspace_22063" ;
   const CHAR* clName             = "list_collection_22063" ;
   const CHAR* fullCLName         = "list_collectionspace_22063.list_collection_22063" ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // SDB_LIST_COLLECTIONS
   bson matcher ;
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", fullCLName ) ;
   bson_finish( &matcher ) ;
   bson expectObj ;
   bson_init( &expectObj ) ;
   rc = bson_copy( &expectObj, &matcher ) ;
   ASSERT_EQ( BSON_OK, rc ) ;
   checkListResult( SDB_LIST_COLLECTIONS, &matcher, &expectObj ) ;
   bson_destroy( &expectObj ) ;
   bson_destroy( &matcher ) ;

   // SDB_LIST_COLLECTIONSPACES
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", csName ) ;
   bson_finish( &matcher ) ;
   rc = bson_copy( &expectObj, &matcher ) ;
   ASSERT_EQ( BSON_OK, rc ) ;
   checkListResult( SDB_LIST_COLLECTIONSPACES, &matcher, &expectObj ) ;
   bson_destroy( &expectObj ) ;
   bson_destroy( &matcher ) ;

   // drop cs in the end
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   sdbReleaseCS( cs ) ;
   sdbReleaseCollection( cl ) ;
}

TEST_F(listTest, sdbGetList_SDB_LIST_STORAGEUNITS_22064)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbGetList( db, SDB_LIST_STORAGEUNITS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ;
   if( SDB_DMS_EOC != rc )
   {
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_string( &expectObj, "NodeName", "" ) ;
      bson_append_string( &expectObj, "Name", "" ) ;
      bson_finish( &expectObj ) ;
      checkListResult( SDB_LIST_STORAGEUNITS, NULL, &expectObj ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ; 
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(listTest, sdbGetList_SDB_LIST_GROUPS_22065)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   // connect to database
   if ( isStandalone( db ) )
   {
      rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
      ASSERT_EQ( SDB_RTN_COORD_ONLY, rc ) ;
      sdbReleaseCursor ( cursor ) ;
   }
   else
   {
      bson empty;
      bson_init( &empty ) ;
      bson_finish( &empty ) ;
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_array( &expectObj, "Group", &empty ) ;
      bson_finish( &expectObj ) ;
      checkListResult( SDB_LIST_GROUPS, NULL, &expectObj ) ;

      bson_destroy( &empty ) ; 
      bson_destroy( &expectObj ) ;
   }
}

TEST_F(listTest, sdbCSListCollections_25083)
{
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   const CHAR* csName             = "list_collecitonspace_25083" ;
   const CHAR* clName             = "list_collection_25073" ;
   INT32 rc                       = SDB_OK ;
   INT64 num                      = 0 ;

   // case 1: cs exists, cl exists
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create collectionspace, rc = " << rc;
   rc = sdbCreateCollection( collectionspace, clName, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create collection, rc = " << rc;
   rc = sdbCSListCollections( collectionspace, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   num = getRecordNum( cursor ) ;
   ASSERT_EQ( 1, num ) ;

   // case 2: cs exists, cl does not exist
   rc = sdbDropCollection( collectionspace, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to Drop collection, rc = " << rc;
   rc = sdbCSListCollections( collectionspace, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   num = getRecordNum( cursor ) ;
   ASSERT_EQ( 0, num ) ;

   // Clear the environment
   rc= sdbDropCollectionSpace( db, csName );
   ASSERT_EQ( SDB_OK, rc ) << "Failed to drop collection, rc = " << rc;
   sdbReleaseCollection( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseCursor( cursor ) ;
}

void listTest::checkListResult ( INT32 TYPE, bson* matcher, bson* expectObj )
{
   INT32 rc                       = SDB_OK ;
   sdbCursorHandle cursor         = 0 ;

   // get list
   rc = sdbGetList( db, TYPE, matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson actualObj ;
   bson_init( &actualObj ) ;
   rc = sdbNext( cursor, &actualObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   // check result
   BOOLEAN isExpect = FALSE ;
   bson_iterator ite, ita ;
   bson_iterator_init( &ite, expectObj ) ;
   bson_iterator_init( &ita, &actualObj ) ;
   while( BSON_EOO != bson_iterator_next( &ite ) &&
            BSON_EOO != bson_iterator_next( &ita ) )
   {
      const char *key = bson_iterator_key( &ite ) ;
      if ( bson_find( &ite, expectObj, key ) ==
             bson_find( &ita, &actualObj, key ) )
      {
         isExpect = TRUE ;
      }
      else
      {
         printf( "expect result: \n" ) ;
         bson_print( expectObj ) ;
         printf( "actual result: \n" ) ;
         bson_print( &actualObj ) ;
         isExpect = FALSE ;
         break ;
      }
   }
   bson_destroy( &actualObj ) ;
   sdbReleaseCursor ( cursor ) ;

   ASSERT_TRUE( isExpect ) ;
}
