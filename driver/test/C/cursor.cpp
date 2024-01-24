#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(cursor,sdbNext)
{
   INT32 rc = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson obj ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert record
   insertRecords( collection, NUM ) ;
   // query all the record in this collection
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;
   // get the next record
   bson_init(&obj);
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Next record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy(&obj);
   rc = sdbGetCount( collection, NULL, &count ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   CHECK_MSG("%s%d%s%lld\n", "NUM = ", NUM, " count = ", count) ;
   ASSERT_EQ ( NUM, count ) ;
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   //release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbCloseCursor)
{
   INT32 rc = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 10 ;
   bson obj ;
   bson obj1 ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert record
   insertRecords ( collection, NUM ) ;
   // query all the record in this collection
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   // get the current record
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;

   // closeCursor
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ;
   ASSERT_EQ( rc, SDB_DMS_CONTEXT_IS_CLOSE ) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbCursor_close_then_get)
{
   INT32 rc = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 1 ;
   bson obj ;
   bson obj1 ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert record
   insertRecords ( collection, NUM ) ;
   // query all the record in this collection
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   // get the current record
   bson_init(&obj);
   rc = sdbNext ( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "SdbNext record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;

   // closeCursor
   rc = sdbCloseCursor( cursor ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbCursor_get_over_then_close)
{
   INT32 rc = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 1 ;
   bson obj ;
   bson obj1 ;
   bson obj2 ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert record
   insertRecords ( collection, NUM ) ;
   // query all the record in this collection
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init(&obj);
   bson_init(&obj1);
   bson_init(&obj2);

   // get the next record, expect rc == SDB_OK
   rc = sdbNext ( cursor, &obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "SdbNext record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   // get the next record, expect rc == SDB_DMS_EOC
   rc = sdbNext ( cursor, &obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   // get the next record, expect rc == SDB_DMS_CONTEXT_IS_CLOSE
   rc = sdbNext ( cursor, &obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   bson_destroy( &obj ) ;
   bson_destroy( &obj1 ) ;
   bson_destroy( &obj2 ) ;

   // closeCursor, expect rc == SDB_OK
   rc = sdbCloseCursor( cursor ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

/*
TEST(cursor,sdbUpdateCurrent)
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
   bson rule ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   // get the current record
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;
   // set the update current record rule
   bson_init( &rule ) ;
   const char *r ="{$set:{age:99}}" ;
   rc = jsonToBson ( &rule, r ) ;
   ASSERT_EQ( TRUE, rc ) ;
   // update the current record
   rc = sdbUpdateCurrent( cursor, &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record again
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After update current record,the current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy(&obj) ;

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   //release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbDeleteCurrent)
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
   bson rule ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;
   // delete current record
   rc = sdbDeleteCurrent( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record again
   rc = sdbCurrent( cursor, &obj ) ;
   // make sure the record no longer exist
   ASSERT_EQ( SDB_CURRENT_RECORD_DELETED, rc ) ;
   printf( "The current record no longer exist.\n" ) ;
   bson_destroy(&obj);

   // disconnect the connection
   sdbDisconnect ( connection ) ;
   //release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/
