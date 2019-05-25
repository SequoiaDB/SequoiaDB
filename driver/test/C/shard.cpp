#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST( nothing, nothing )
{
   ASSERT_TRUE ( 1 == 1 ) ;
}
/*
TEST(cursor,sdbNext)
{
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   INT32 rc                          = SDB_OK ;
   bson obj ;
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

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

*/
