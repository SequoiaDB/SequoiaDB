#include <stdio.h>
#include <gtest/gtest.h>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "client.h"

#define USERDEF       "sequoiadb"
#define PASSWDDEF     "sequoiadb"

class sdbTest: public testBase
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

TEST_F(sdbTest, sdbConnect_with_usr_22014)
{
   if( isStandalone( db ) ) 
   {   
      printf("Run mode is standalone\n\n") ;
      return ;
   }   

   INT32 rc                        = SDB_OK ;

   // create a new user
   rc = sdbCreateUsr( db, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // connect to database again with usrname and passwd
   sdbConnectionHandle connection = 0 ;
   rc = sdbConnect ( ARGS->hostName(), ARGS->svcName(), USERDEF, PASSWDDEF, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // Remove a user
   rc = sdbRemoveUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST_F(sdbTest, sdbConnect_with_several_address_10409)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor = 0 ;
   INT32 rc                       = SDB_OK ;
   INT32 len = strlen( ARGS->hostName() ) ;
   INT32 totalLen = len + 6 ;

   CHAR* connArr1 = ( CHAR* )malloc( ( totalLen + 1 ) * sizeof( CHAR ) ) ;
   strcpy( connArr1, ARGS->hostName() ) ;
   strcat( connArr1, ":50000" ) ;

   CHAR* connArr2 = ( CHAR* )malloc( ( totalLen + 1 ) * sizeof( CHAR ) ) ;
   strcpy( connArr2, ARGS->hostName() ) ;
   strcat( connArr2, ":12340" ) ;

   CHAR* connArr3 = ( CHAR* )malloc( ( totalLen + 1 ) * sizeof( CHAR ) ) ;
   strcpy( connArr3, ARGS->hostName() ) ;
   strcat( connArr3, ":" ) ;
   strcat( connArr3, ARGS->svcName() ) ;

   const CHAR* connArr[9] = {"192.168.20.35:12340",
                              "192.168.20.36:12340",
                              "123:123",
                              "",
                              ":12340",
                              "192.168.20.40",
                              connArr1,
                              connArr2,
                              connArr3} ;
   // connect to database
   rc = sdbConnect1 ( connArr, 9, ARGS->user(), ARGS->passwd(), &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( connection, 4, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   free( connArr1 ) ;
   free( connArr2 ) ;
   free( connArr3 ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST_F(sdbTest, sdbCreateGetDropCollectionSpace_10402)
{
   sdbCSHandle collectionspace    = 0 ;
   INT32 rc                       = SDB_OK ;

   // drop the exist cl first
   rc = sdbDropCollectionSpace ( db, COLLECTION_SPACE_NAME ) ; 
   if( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
   {   
      ASSERT_TRUE( false ) << "drop collectionspace fail, rc: " << rc ;
   }   

   rc = sdbCreateCollectionSpace ( db, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollectionSpace ( db, COLLECTION_SPACE_NAME,
                                   &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollectionSpace ( db, COLLECTION_SPACE_NAME ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbReleaseCS ( collectionspace ) ;
}

TEST_F(sdbTest, sdbGetCollection_10403)
{
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   
   rc = getCollection ( db, COLLECTION_FULL_NAME, &collection ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;

   collection = 0 ;
   rc = sdbGetCollection ( db, COLLECTION_FULL_NAME,
                           &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbReleaseCollection ( collection ) ;
}

TEST_F(sdbTest, sdbListCollectionSpaces_10402)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;

   rc = sdbListCollectionSpaces ( db, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // display all the record
   bson_init( &obj ) ;
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   printf("All the collection spaces have been list.\n") ;

   sdbReleaseCursor ( cursor ) ;
}

TEST_F(sdbTest, sdbListCollections_10403)
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;

   rc = sdbListCollections ( db, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // display all the record
   bson_init( &obj ) ;
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   printf("All the collections have been list.\n") ;

   sdbReleaseCursor ( cursor ) ;
}

TEST_F(sdbTest, sdbListReplicaGroups_22015)
{
   if( isStandalone( db ) )
   {
      printf("Run mode is standalone\n\n") ;
      return ;
   }

   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = sdbListReplicaGroups ( db, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // display all the record
   bson_init( &obj ) ;
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   printf("All the shards have been list.\n") ;

   sdbReleaseCursor ( cursor ) ;
}

TEST_F(sdbTest, sdbExec_ExecUpdate_22016)
{
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = getCollection ( db, COLLECTION_FULL_NAME, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR *sql1 = "insert into testfoo.testbar( name, age ) \
                       values( \"啊怪\", 28 )" ;
   const CHAR *sql2 = "select * from testfoo.testbar" ;
   bson obj ;
   rc = sdbExecUpdate ( db, sql1 ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbExec ( db, sql2, &cursor ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("The operation is :\n") ;
   printf("%s\n",sql1) ;
   printf("%s\n",sql2) ;
   printf("The result are as below:\n") ;
   // display all the record
   bson_init( &obj ) ;
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   printf("All the collection space have been list.\n") ;

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
}

TEST_F(sdbTest, sdbTransaction_22017)
{
   if( isStandalone( db ) )
   {  
      printf("Run mode is standalone\n\n") ;
      return ;
   }

   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;
   BOOLEAN isTranOnFlag           = FALSE ;
   bson conf ;
   bson obj ;
   rc = isTranOn( db, &isTranOnFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( FALSE == isTranOnFlag )
   {
      printf( "transaction is disable\n" ) ;
      return ;
   }
   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create cl with RepliSize = 0
   bson_init ( &conf ) ;
   bson_append_int ( &conf, "ReplSize", 0 ) ;
   bson_finish ( &conf ) ;
   rc = sdbCreateCollection1 ( cs, CLNAME, &conf, &cl ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   // delete all records before test
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   // TO DO:
   // transaction begin
   rc = sdbTransactionBegin ( db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert a English record
   bson_init( &obj ) ;
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // transaction commit
   rc = sdbTransactionCommit( db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   CHECK_MSG("%s%lld\n", "count = ", count);
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, count ) ;

   // TO DO:
   // transaction begin
   rc = sdbTransactionBegin ( db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert a English record
   bson_init( &obj ) ;
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // transaction roll back
   rc = sdbTransactionRollback( db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, count ) ;

   bson_destroy( &conf ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
}

TEST_F( sdbTest, sdbCloseCursor_run_out_close_22018 )
{
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   INT32 num                      = 1 ;

   bson obj ;
   bson obj1 ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;

   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db, COLLECTION_FULL_NAME, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // delete all records before test
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   // prepare some records
   insertRecords( cl, num ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( num, count ) ;
   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   // get next
   bson_init ( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; // getNext in cursor, expect SDB_OK
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get current
   bson_init ( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ; // getCurrent in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get next
   bson_init ( &obj2 ) ;
   rc = sdbNext( cursor, &obj2 ) ; // getNext in cursor, -29
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   // close cursor
   rc = sdbCloseCursor( cursor ) ; // close in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get next again
   bson_init ( &obj3 ) ;
   rc = sdbNext( cursor, &obj3 ) ; // getNext in cursor1, expect -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   // get currnet again
   bson_init ( &obj4 ) ;
   rc = sdbCurrent( cursor, &obj4 ) ; // getCurrent in cursor1, expect -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   bson_destroy( &obj );
   bson_destroy( &obj1 );
   bson_destroy( &obj2 );
   bson_destroy( &obj3 );
   bson_destroy( &obj4 );

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
}

TEST_F(sdbTest, sdbIsClose_10409)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   BOOLEAN result = FALSE ;

   rc = initEnv( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // TO DO:
   // test when we get nornal business packet back from server,
   // wether sdbIsValid() return false. if so, it means error
   result = sdbIsValid( connection );
   std::cout << "before close connection, result is " << result << std::endl ;
   ASSERT_EQ( TRUE, result ) ;

   result = sdbIsClosed( connection );
   ASSERT_EQ( FALSE, result ) ;

   // test close after disconnect
   sdbDisconnect ( connection ) ;
   result = sdbIsValid( connection );
   std::cout << "after close connection, result is " << result << std::endl ;
   ASSERT_EQ ( FALSE, result ) ;
   result = sdbIsClosed( connection );
   ASSERT_EQ( TRUE , result ) ;
   sdbDisconnect( connection ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST_F(sdbTest, truncate_22019)
{
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   INT32 i                        = 0 ;
   INT32 num                      = 100 ;
   SINT64 totalNum                = 0 ;

   // get cl
   rc = getCollection ( db, COLLECTION_FULL_NAME , &collection ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert some record
   for ( ; i < num; i++ )
   {
      bson obj ;
      bson_init( &obj ) ;
      bson_append_string ( &obj, "test_truncate_in_c", "test" ) ;
      bson_finish ( &obj ) ;
      rc = sdbInsert( collection, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }
   // test
   rc = sdbTruncateCollection( db, COLLECTION_FULL_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, totalNum ) ;

   rc = sdbDropCollectionSpace ( db, COLLECTION_SPACE_NAME ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   sdbReleaseCollection ( collection ) ;
}
