#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"
#include "base64c.h"

TEST(newapi, a)
{
   ASSERT_TRUE( 1==1 ) ;
}
/*
TEST(newapi,aggregate)
{
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
   BOOLEAN flag                      = false ;
   bson *ob[4] ;
   bson obj ;
   int iNUM = 2 ;
   int rNUM = 4 ;
   int i = 0 ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{$match:{status:\"A\"}}" ;
   command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;
   record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}" ;
   record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}" ;
   record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}" ;
   record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}" ;
   const char* m = "{$match:{status:\"A\"}}" ;
   const char* g = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // insert record
   for( i=0; i<rNUM; i++ )
   {
      bson_init( &obj ) ;
      flag = jsonToBson( &obj, record[i] ) ;
bson_print( &obj ) ;
      ASSERT_TRUE( flag == true ) ;
      rc = sdbInsert( collection, &obj ) ;
printf("rc= %d\n", rc ) ;
      ASSERT_TRUE( rc == SDB_OK ) ;
      bson_destroy( &obj ) ;
   }

   // build bson array
   for( i=0; i<iNUM; i++ )
   {
      ob[i] = bson_create() ;
      flag = jsonToBson ( ob[i], command[i] ) ;
bson_print( ob[i] ) ;
      ASSERT_TRUE( flag == true ) ;
   }
   // aggregate
   rc = sdbAggregate( collection, ob, iNUM, &cursor ) ;
   printf("rc = %d\n",rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // display
   displayRecord ( &cursor ) ;
   // free memory which is malloc by bson_create()
   for( i=0; i<iNUM; i++ )
   {
      bson_dispose( ob[i] ) ;
   }
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   //release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( newapi, insert_binary_data )
{
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
//   const char *str = "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":1}}" ;
//   const char *str = "{a:1}" ;
   char *rawstr = "hello world!" ;
   int len = getEnBase64Size ( rawstr ) ;
   char *out = ( char* )malloc ( len ) ;
   base64Encode ( rawstr, out, len ) ;
printf( "out is %s\n", out ) ;
   bson obj ;
   bson_init ( &obj ) ;
   bson_append_binary( &obj, "key", 49, rawstr, strlen ( rawstr ) );
   bson_finish ( &obj ) ;
   free ( out ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
//   rc = jsonToBson ( &obj, str ) ;
//printf ( "rc is %d\n", rc ) ;
   ASSERT_TRUE ( rc == SDB_OK ) ;
//bson_print ( &obj ) ;
   rc = sdbInsert ( collection, &obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   bson_destroy( &obj ) ;
   // query all the record in this collection
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get the current record
//   displayRecord ( &cursor ) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( newapi, insert_regex )
{
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
//   const char *str =
//   "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":1}}" ;
//   const char *str = "{a:1}" ;
   char *rawstr = "hello world!" ;
   int len = getEnBase64Size ( rawstr ) ;
   char *out = ( char* )malloc ( len ) ;
   base64Encode ( rawstr, out, len ) ;
printf( "out is %s\n", out ) ;
   bson obj ;
   bson_init ( &obj ) ;
   bson_append_binary( &obj, "key", 49, rawstr, strlen ( rawstr ) );
   bson_finish ( &obj ) ;
   free ( out ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
//   rc = jsonToBson ( &obj, str ) ;
//printf ( "rc is %d\n", rc ) ;
   ASSERT_TRUE ( rc == SDB_OK ) ;
//bson_print ( &obj ) ;
   rc = sdbInsert ( collection, &obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   bson_destroy( &obj ) ;
   // query all the record in this collection
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get the current record
//   displayRecord ( &cursor ) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/

/*
TEST(cursor,sdbCurrent)
{
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
   bson obj ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // query all the record in this collection
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   // get the current record
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( cbson, regex )
{
   bson obj ;
   bson_init ( &obj ) ;
   bson_append_regex ( &obj, "key", "^张", "i" ) ;
   bson_finish ( &obj ) ;
   bson_print ( &obj ) ;
   bson_destroy ( &obj ) ;

   bson_init ( &obj ) ;
   bson_append_regex ( &obj, "key", "\ba\w*\b", "m" ) ;
   bson_finish ( &obj ) ;
   bson_print ( &obj ) ;
   bson_destroy ( &obj ) ;

   bson_init ( &obj ) ;
   bson_append_regex ( &obj, "key", "0\d{2}-\d{8}|0\d{3}-\d{7}", "x" ) ;
   bson_finish ( &obj ) ;
   bson_print ( &obj ) ;
   bson_destroy ( &obj ) ;

   bson_init ( &obj ) ;
   bson_append_regex ( &obj, "key", "((2[0-4]\d|25[0-5]|[01]?\d\d?)\.){3}(2[0-4]\d|25[0-5]|[01]?\d\d?", "s" ) ;
   bson_finish ( &obj ) ;
   bson_print ( &obj ) ;
   bson_destroy ( &obj ) ;
}

*/
