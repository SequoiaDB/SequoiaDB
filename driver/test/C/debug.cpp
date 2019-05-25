#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"
#include "base64c.h"

#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

using namespace std ;

#define USERDEF       "sequoiadb"
#define PASSWDDEF     "sequoiadb"

TEST(debug, debug)
{
   ASSERT_TRUE( 1 == 1 ) ;
}

/*
TEST(debug,sdbGetList_SDB_LIST_GROUPS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_GROUPS,
                       NULL, NULL, NULL, &cursor ) ;
      ASSERT_TRUE( rc == SDB_RTN_COORD_ONLY ) ;
   } // standalone mode
   else
   {
      displayRecord( &cursor ) ;
      ASSERT_TRUE( rc == SDB_OK ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
}


TEST(debug, sdbGetIndexes)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}
*/

/*
TEST(debug, sdbQuery_with_flag1)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *pStr               = "{ \"\": \"999|50|John\" }" ;
   CHAR buff[ 1024 ]              = { 0 } ;
   bson obj ;
   bson condition ;
   bson select ;
   bson orderBy ;
   bson hint ;
   INT32 i = 0 ;
   INT32 num = 10;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   insertRecords( collection, num ) ;
   bson_init( &select ) ;
   bson_append_string( &select, "firstName", "" ) ;
   bson_append_string( &select, "age", ""  ) ;
   bson_append_string( &select, "Id", "" ) ;
   bson_finish( &select ) ;
   rc = sdbQuery1 ( collection, NULL, &select,
                   NULL, NULL, 0, -1, 1, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   bson_init ( &obj ) ;
   i = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      i++ ;
      bson_sprint( buff, 1024, &obj ) ;
      ASSERT_TRUE ( 0 == strncmp( pStr, buff, sizeof( buff ) ) ) ;
      printf("obj is: %s\n", buff ) ;
      bson_destroy( &obj ) ;
   }
   ASSERT_TRUE ( 10 == i ) ;   

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/
/*
TEST(collection,sdbInsert1_check_id)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   const char *key                = "" ;
   int value                      = 0 ;
   bson obj ;
   bson_iterator it ;
   const char *ret = NULL;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME ,
                        &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_init ( &obj ) ;
   bson_append_string( &obj, "_id", "abc" );
   bson_append_int ( &obj, "a", 1 ) ;
   bson_finish ( &obj ) ;
   printf( "Insert a record, the record is:" OSS_NEWLINE ) ;
   bson_print( &obj ) ;
   rc = sdbInsert1 ( collection, &obj, &it ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc );
   ASSERT_TRUE( rc == SDB_OK ) ;
   ret = bson_iterator_string( &it );
   ASSERT_TRUE( memcmp(ret, "", sizeof("")) ) ;
   printf( "Ihe record is %s\n", ret );
   rc = bson_iterator_next( &it ) ;
   ASSERT_TRUE( rc == BSON_INT ) ;
   key = bson_iterator_key( &it ) ;
   value = bson_iterator_int( &it ) ;
   printf("The insert record is {%s:%d}\n", key, value ) ;
   bson_destroy ( &obj ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/
/*
TEST ( debug, connect_wrong_addr )
{
   INT32 rc = SDB_OK ;
   const CHAR *host = "localhost" ;
   const CHAR *port = "11810" ;
   sdbConnectionHandle db = 0 ;
   rc = sdbConnect( host, port, "", "", &db ) ;
   if ( rc )
   {
      std::cout << "Failed to connect.\n" << std::endl ;
      ASSERT_TRUE ( 0 == 1 ) ;
   }
   sdbDisconnect( db ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}
*/

/*
TEST(debug,sdbTransactionRollback)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;

   bson obj ;
   bson conf ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_init ( &conf ) ;
   bson_append_int ( &conf, "ReplSize", 0 ) ;
   bson_finish ( &conf ) ;
   rc = sdbCreateCollection1 ( cs, CLNAME, &conf, &cl ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE ( rc == SDB_OK ) ;
   rc = sdbTransactionBegin ( connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_init( &obj ) ;
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   bson_destroy ( &obj ) ;
   rc = sdbTransactionRollback( connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   ASSERT_TRUE( count == 0 ) ;

   bson_destroy( &conf ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( connection ) ;
}
*/
/*
TEST(debug, sdbConnect_with_several_address)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR* connArr[10] = {
                              "192.168.20.35:12340",
                              "192.168.20.36:12340",
                              "123:123",
                              "",
                              ":12340",
                              "192.168.20.40",
                              "localhost:50000",
                              "192.168.20.40:12340",
                              "localhost:11810",
                              "localhost:118101"
                              } ;
   rc = sdbConnect1 ( connArr, 10, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = sdbGetList( connection, 4, NULL, NULL, NULL, &cursor ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( sdb, sdbCloseCursor_run_out_close )
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;
   INT32 num                      = 1 ;

   bson obj ;
   bson obj1 ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;
   bson conf ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = getCollection ( connection, COLLECTION_FULL_NAME, &cl ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   insertRecords( cl, num ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   ASSERT_TRUE( num == count ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;

   bson_init ( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; // getNext in cursor, expect SDB_OK
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_init ( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ; // getCurrent in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_init ( &obj2 ) ;
   rc = sdbNext( cursor, &obj2 ) ; // getNext in cursor, -29
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_DMS_EOC ) ;
   rc = sdbCloseCursor( cursor ) ; // close in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_init ( &obj3 ) ;
   rc = sdbNext( cursor, &obj3 ) ; // getNext in cursor1, expect -36
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_RTN_CONTEXT_NOTEXIST ) ;
   bson_init ( &obj4 ) ;
   rc = sdbCurrent( cursor, &obj4 ) ; // getCurrent in cursor1, expect -36
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_RTN_CONTEXT_NOTEXIST ) ;

   bson_destroy( &obj );
   bson_destroy( &obj1 );
   bson_destroy( &obj2 );
   bson_destroy( &obj3 );
   bson_destroy( &obj4 );

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbDisconnect( connection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/
/*
TEST(debug, sdbConnect_with_usr)
{
   sdbConnectionHandle connection  = 0 ;
   sdbCursorHandle cursor          = 0 ;
   INT32 rc                        = SDB_OK ;
   const CHAR* host                = "192.168.20.186" ;
   const CHAR* port                = "11810" ;
   const CHAR* username            = "" ;
   const CHAR* password            = "" ;
   const CHAR* user                = "" ;
   const CHAR* passwd              = "" ;
   rc = sdbConnect ( host, port, username, password, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = sdbGetList( connection, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      printf("sdbConnect_with_usr is use in cluster environment only\n") ;
      return ;
   }
   sdbDisconnect ( connection ) ;
}
*/
/*
TEST(debug, binary)
{
   INT32 rc = SDB_OK ;
   bson obj ;
   const CHAR *str = "hello world" ;
   int len = getEnBase64Size( str ) ;
   CHAR *out = (CHAR *) malloc( len ) ;
   rc = base64Encode ( str, strlen(str) + 1, out, len ) ;
   ASSERT_TRUE ( rc == SDB_OK ) ;
   printf( "hello world encode is: %s\n", out ) ;
   free ( out ) ;
}
*/




