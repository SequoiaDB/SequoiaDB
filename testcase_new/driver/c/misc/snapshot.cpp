#include <stdio.h>
#include <gtest/gtest.h>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "client.h"

class snapshotTest: public testBase
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
    
    void checkSnapshotResult ( INT32 TYPE, bson* matcher, bson* expectObj ) ;
} ;

TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_CONTEXTS_22057)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_CONTEXTS, NULL, NULL, NULL, &cursor ) ;
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
      checkSnapshotResult( SDB_SNAP_CONTEXTS, NULL, &expectObj ) ;

      bson_destroy( &empty ) ;
      bson_destroy( &expectObj ) ;
   }
   
   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
}


TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_CONTEXTS_CURRENT_22057)
{
   sdbCursorHandle cursor         = 0 ; 
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ; 
   rc = sdbGetSnapshot( db, SDB_SNAP_CONTEXTS_CURRENT, NULL, NULL, NULL, &cursor ) ; 
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
      checkSnapshotResult( SDB_SNAP_CONTEXTS_CURRENT, NULL, &expectObj ) ; 

      bson_destroy( &empty ) ; 
      bson_destroy( &expectObj ) ; 
   }   

   bson_destroy( &obj ) ; 
   sdbReleaseCursor ( cursor ) ; 
}


TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_SESSIONS_22058)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ; 
   rc = sdbGetSnapshot( db, SDB_SNAP_SESSIONS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ; 
   if( SDB_DMS_EOC != rc )
   {
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_long( &expectObj, "SessionID", 1 ) ;
      bson_append_int( &expectObj, "TID", 1 ) ;
      bson_finish( &expectObj ) ;
      checkSnapshotResult( SDB_SNAP_SESSIONS, NULL, &expectObj ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ; 
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_SESSIONS_CURRENT_22058)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ; 
   rc = sdbGetSnapshot( db, SDB_SNAP_SESSIONS_CURRENT, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &obj ) ; 
   if( SDB_DMS_EOC != rc )
   {  
      bson expectObj ;
      bson_init( &expectObj ) ;
      bson_append_long( &expectObj, "SessionID", 1 ) ;
      bson_append_int( &expectObj, "TID", 1 ) ;
      bson_finish( &expectObj ) ;
      checkSnapshotResult( SDB_SNAP_SESSIONS_CURRENT, NULL, &expectObj ) ;
      bson_destroy( &expectObj ) ;
   }

   bson_destroy( &obj ) ; 
   sdbReleaseCursor ( cursor ) ;
}

TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_COLLECTIONS_AND_COLLECTIONSPACES_22059)
{
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   INT32 rc                       = SDB_OK ;

   // create cl
   const CHAR* csName             = "snapshot_collectionspace_22059" ;
   const CHAR* clName             = "snapshot_collection_22059" ;
   const CHAR* fullCLName         = "snapshot_collectionspace_22059.snapshot_collection_22059" ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // SDB_SNAP_COLLECTIONS
   bson matcher ;
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", fullCLName ) ; 
   bson_finish( &matcher ) ; 
   bson empty;
   bson_init( &empty ) ; 
   bson_finish( &empty ) ;
   bson expectObj ;
   bson_init( &expectObj ) ;
   bson_append_long( &expectObj, "UniqueID", 1 ) ;
   bson_append_array( &expectObj, "Details", &empty ) ;
   bson_finish( &expectObj ) ;
   checkSnapshotResult( SDB_SNAP_COLLECTIONS, &matcher, &expectObj ) ;
   bson_destroy( &expectObj ) ;
   bson_destroy( &matcher ) ;
            
   // SDB_SNAP_COLLECTIONSPACES
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", csName ) ;
   bson_finish( &matcher ) ;
   bson_init( &expectObj ) ;
   bson_append_int( &expectObj, "UniqueID", 1 ) ;
   bson_append_array( &expectObj, "Collection", &empty ) ;
   bson_finish( &expectObj ) ;
   checkSnapshotResult( SDB_SNAP_COLLECTIONSPACES, &matcher, &expectObj ) ;
   bson_destroy( &empty ) ;
   bson_destroy( &expectObj ) ;
   bson_destroy( &matcher ) ;

   // drop cs in the end
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   sdbReleaseCS( cs ) ; 
   sdbReleaseCollection( cl ) ; 
}

TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_DATABASE_22060)
{
   bson expectObj ;
   bson_init( &expectObj ) ;
   if ( isStandalone( db ) )
   { 
      bson_append_long( &expectObj, "TotalReadTime", 1 ) ;
   } 
   else
   { 
      bson_append_double( &expectObj, "TotalReadTime", 1 ) ;
   }
   bson_finish( &expectObj ) ;
   checkSnapshotResult( SDB_SNAP_DATABASE, NULL, &expectObj ) ;
   bson_destroy( &expectObj ) ;
}

TEST_F(snapshotTest, sdbGetSnapshot_SDB_SNAP_SYSTEM_22060)
{
   bson expectObj ;
   bson_init( &expectObj ) ; 
   bson_append_start_object( &expectObj, "CPU" ) ; 
   bson_append_string( &expectObj, "", "" ) ; 
   bson_append_finish_object( &expectObj ) ; 
   bson_finish( &expectObj ) ; 
   checkSnapshotResult( SDB_SNAP_SYSTEM, NULL, &expectObj ) ;
   bson_destroy( &expectObj ) ; 
}

void snapshotTest::checkSnapshotResult ( INT32 TYPE, bson* matcher, bson* expectObj )
{
   INT32 rc                       = SDB_OK ;
   sdbCursorHandle cursor         = 0 ;

   // get snapshot 
   rc = sdbGetSnapshot( db, TYPE, matcher, NULL, NULL, &cursor ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   bson actualObj ;
   bson_init( &actualObj ) ;
   rc = sdbNext( cursor, &actualObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   // check snapshot result
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
