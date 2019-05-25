#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(collectonspace,sdbCreateCollection)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollection ( collectionspace,
                            COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME,
                              &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbCreateCollection1_without_options)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson option ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &option );
   bson_append_int( &option, "ReplSize", 0 );
   bson_finish( &option ) ;
   rc = sdbCreateCollection1 ( collectionspace,
                               COLLECTION_NAME,
                               &option,
                               &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &option ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbCreateCollection1_with_options)
{
   printf("The test work in cluster environment\
 with at least 2 data notes.\n") ;
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson options ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init ( &options ) ;
   bson_append_start_object ( &options, "ShardingKey" ) ;
   bson_append_int ( &options, "age", 1 ) ;
   bson_append_int ( &options, "name", -1 ) ;
   bson_append_finish_object ( &options ) ;
   bson_append_int ( &options, "ReplSize", 2 ) ;
   bson_append_bool ( &options, "Compressed", true ) ;
   bson_finish ( &options ) ;
   bson_print ( &options ) ;
   rc = sdbCreateCollection1 ( collectionspace,
                               COLLECTION_NAME,
                               &options,
                               &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &options ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbDropCollection)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME,
                              &collection ) ;
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbGetCollection1)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollection1 ( collectionspace, COLLECTION_NAME, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbGetCSName)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   INT32 rc                       = SDB_OK ;
   CHAR pCSName[ NAME_LEN + 1 ]   = { 0 } ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCSName ( collectionspace, pCSName, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("CS name is :%s\n",pCSName ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

