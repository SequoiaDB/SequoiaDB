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
   // connect to database
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop the exist cl first
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build option
   bson_init( &option );
   bson_append_int( &option, "ReplSize", 0 );
   bson_finish( &option ) ;
   // create cl
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop the exist cl first
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

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
   // get cl with sharding info
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create cl
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME,
                              &collection ) ;
   // drop cl
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl handle
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
   // memset ( pCSName, 0, sizeof(char)*128 ) ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs name
   rc = sdbGetCSName ( collectionspace, pCSName, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("CS name is :%s\n",pCSName ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( collectonspace, sdbCSListCollections )
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   INT32 rc                       = SDB_OK ;
   INT64 num                      = 0 ;
   CHAR pCSName[ NAME_LEN + 1 ]   = { 0 } ;
   
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: cs exists, cl exists
   rc = sdbCSListCollections( collectionspace, &cursor1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   num = getRecordNum( cursor1 ) ;
   ASSERT_EQ( 1, num ) ;

   // case 2: cs exists, cl does not exist
   rc = sdbDropCollection( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCSListCollections( collectionspace, &cursor2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   num = getRecordNum( cursor2 ) ;
   ASSERT_EQ( 0, num ) ;
   
   sdbDisconnect ( connection ) ; 
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseCursor( cursor1 ) ;
   sdbReleaseCursor( cursor2 ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( collectonspace, sdbCSGetDomainName )
{
   sdbConnectionHandle connection = 0 ;
   sdbDomainHandle dom            = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   const CHAR *pDomain            = "domain1" ;
   INT32 rc                       = SDB_OK ;
   CHAR pResult[ NAME_LEN + 1 ]   = { 0 } ;
   bson opt ;
   bson domObj ;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // case 1: domain does not exists
   rc = sdbCSGetDomainName ( collectionspace, pResult, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( '\0', pResult[0] ) ;

   // case 2: domain exists
   // create domain
   bson_init( &domObj ) ;
   bson_append_start_array( &domObj, "Groups" ) ;
   bson_append_string( &domObj, "0", GROUPNAME1 ) ;
   bson_append_string( &domObj, "1", GROUPNAME2 ) ;
   bson_append_finish_array( &domObj ) ;
   bson_finish( &domObj ) ;
   rc = sdbCreateDomain( connection,  pDomain, &domObj, &dom ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // set domain
   bson_init( &opt ) ;
   bson_append_string( &opt, "Domain", pDomain ) ;
   bson_finish( &opt ) ;
   rc = sdbCSSetDomain( collectionspace, &opt ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get domain name
   rc = sdbCSGetDomainName ( collectionspace, pResult, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strcmp( pResult, pDomain ) ) ;

   //clear the environment
   rc = sdbDropCollectionSpace ( connection, COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropDomain( connection, pDomain ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_destroy( &opt ) ;
   bson_destroy( &domObj ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   sdbReleaseDomain ( dom ) ;
}
