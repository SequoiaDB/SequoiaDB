#include <stdio.h>
#include <gtest/gtest.h>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "client.h"

class collectionspaceTest: public testBase
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
} ;

TEST_F(collectionspaceTest, sdbCreateCollection1_with_options_22027)
{
   if( isStandalone( db ) ) 
   {   
      printf( "Run mode is standalone\n" ) ; 
      return ;
   }   

   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson options ;
   bson obj ;

   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop the exist cl first
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   if( SDB_OK != rc && SDB_DMS_NOTEXIST != rc )
   {
      ASSERT_TRUE( false ) << "drop collection fail, rc: " << rc ;
   }

   // test sdbCreateCollection1
   // option is {ShardingKey:{age:1,name:-1}},{ReplSize:2},{Compressed:true}
   bson_init ( &options ) ;
   bson_append_start_object ( &options, "ShardingKey" ) ;
   bson_append_int ( &options, "age", 1 ) ;
   bson_append_int ( &options, "name", -1 ) ;
   bson_append_finish_object ( &options ) ;
   bson_append_int ( &options, "ReplSize", 2 ) ;
   bson_append_bool ( &options, "Compressed", true ) ;
   bson_finish ( &options ) ;
   bson_print ( &options ) ;
   // create cl with sharding info
   rc = sdbCreateCollection1 ( collectionspace,
                               COLLECTION_NAME,
                               &options,
                               &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // check cl's attribute
   bson matcher ;
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", COLLECTION_FULL_NAME ) ; 
   bson_finish( &matcher ) ; 
   rc = sdbGetSnapshot( db, SDB_SNAP_CATALOG, &matcher, NULL, NULL, &cursor ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_print( &obj ) ;  

   bson_iterator it, subit ;
   bson_find( &it, &obj, "ShardingKey" ) ; 
   bson subObj ;
   bson_init( &subObj ) ;
   bson_iterator_subobject( &it, &subObj ) ;
   bson_find( &subit, &subObj, "age" ) ;
   ASSERT_EQ( 1, bson_iterator_int( &subit ) ) ;
   bson_find( &subit, &subObj, "name" ) ;
   ASSERT_EQ( -1, bson_iterator_int( &subit ) ) ;

   bson_find( &it, &obj, "ReplSize" ) ;
   ASSERT_EQ( 2, bson_iterator_int( &it ) ) ;
   bson_find( &it, &obj, "CompressionTypeDesc" ) ;
   ASSERT_EQ( 0, strncmp( "lzw", bson_iterator_string( &it ), strlen( bson_iterator_string( &it ) ) ) ) ;

   // test sdbGetCollection1
   collection = 0 ;
   rc = sdbGetCollection1 ( collectionspace, COLLECTION_NAME, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check collection
   bson_destroy( &obj ) ;
   bson_init ( &obj ) ; 
   bson_append_int ( &obj, "age", 50 ) ; 
   bson_append_string ( &obj, "name", "Joe" ) ; 
   bson_finish ( &obj ) ; 
   rc = sdbInsert ( collection, &obj ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   bson_destroy ( &options ) ;
   bson_destroy( &matcher ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &subObj ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
}

TEST_F(collectionspaceTest, sdbGetCSName_10402)
{
   sdbCSHandle collectionspace    = 0 ;
   INT32 rc                       = SDB_OK ;
   CHAR pCSName[ NAME_LEN + 1 ]   = { 0 } ;
   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs name
   rc = sdbGetCSName ( collectionspace, pCSName, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check result
   ASSERT_EQ( 0, strncmp( pCSName, COLLECTION_SPACE_NAME, strlen(COLLECTION_SPACE_NAME) ) ) ;
   sdbReleaseCS ( collectionspace ) ;
}

TEST_F(collectionspaceTest, sdbDropCollectionSpace1_with_options25073)
{
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   const CHAR* csName             = "dropcs_collectionspace_25073" ;
   const CHAR* clName             = "dropcs_collection_25073" ;
   INT32 rc                       = SDB_OK ;
   bson options1 ;

   // dropCollectionSpace option bson
   bson_init( &options1 ) ;
   bson_append_bool( &options1, "EnsureEmpty", TRUE ) ;
   rc = bson_finish( &options1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   //case :the options parameter is {"EnsureEmpty" : true}
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create collectionspace, rc = " << rc;
   rc = sdbDropCollectionSpace1 ( db, csName, &options1) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbReleaseCS( collectionspace ) ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create collectionspace, rc = " << rc;
   rc = sdbCreateCollection( collectionspace, clName, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create collection, rc = " << rc;
   rc = sdbDropCollectionSpace1 ( db, csName, &options1) ;
   ASSERT_EQ( -275, rc ) << "Failed to drop collection, rc = " << rc;

   // Clear the environment
   rc = sdbDropCollectionSpace ( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &options1 ) ;
   sdbReleaseCS( collectionspace ) ;
   sdbReleaseCollection( collection ) ;
}

