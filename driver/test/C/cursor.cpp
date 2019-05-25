#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(cursor,sdbNext)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson obj ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   insertRecords( collection, NUM ) ;
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;
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
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbCloseCursor)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 10 ;
   bson obj ;
   bson obj1 ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   insertRecords ( collection, NUM ) ;
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;

   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ;
   ASSERT_EQ( rc, SDB_DMS_CONTEXT_IS_CLOSE ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbCursor_close_then_get)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 1 ;
   bson obj ;
   bson obj1 ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   insertRecords ( collection, NUM ) ;
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_init(&obj);
   rc = sdbNext ( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "SdbNext record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;

   rc = sdbCloseCursor( cursor ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   bson_init( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbCursor_get_over_then_close)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 NUM                         = 1 ;
   bson obj ;
   bson obj1 ;
   bson obj2 ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   insertRecords ( collection, NUM ) ;
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init(&obj);
   bson_init(&obj1);
   bson_init(&obj2);

   rc = sdbNext ( cursor, &obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "SdbNext record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   rc = sdbNext ( cursor, &obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   rc = sdbNext ( cursor, &obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   bson_destroy( &obj ) ;
   bson_destroy( &obj1 ) ;
   bson_destroy( &obj2 ) ;

   rc = sdbCloseCursor( cursor ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

/*
TEST(cursor,sdbUpdateCurrent)
{
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
   bson obj ;
   bson rule ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;
   bson_init( &rule ) ;
   const char *r ="{$set:{age:99}}" ;
   rc = jsonToBson ( &rule, r ) ;
   ASSERT_EQ( TRUE, rc ) ;
   rc = sdbUpdateCurrent( cursor, &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After update current record,the current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy(&obj) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(cursor,sdbDeleteCurrent)
{
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
   bson obj ;
   bson rule ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init(&obj);
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Current record is:\n" ) ;
   bson_print( &obj ) ;
   printf("\n") ;
   bson_destroy( &obj ) ;
   rc = sdbDeleteCurrent( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCurrent( cursor, &obj ) ;
   ASSERT_EQ( SDB_CURRENT_RECORD_DELETED, rc ) ;
   printf( "The current record no longer exist.\n" ) ;
   bson_destroy(&obj);

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/
