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

TEST(debug, lob_global_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   INT32 counter = 0 ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   sdbCursorHandle cur               = 0 ;
   sdbLobHandle lob                  = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson_oid_t oid ;
   bson obj ;
   #define BUFSIZE1 (1024 * 1024 * 3)
   //#define BUFSIZE1 ( 1024 * 2 )
   #define BUFSIZE2 (1024 * 1024 * 2)
   SINT64 lobSize = -1 ;
   UINT64 createTime = -1 ;
   CHAR buf[BUFSIZE1] = { 0 } ;
   CHAR readBuf[BUFSIZE2] = { 0 } ;
   UINT32 readCount = 0 ;
   CHAR c = 'a' ;
   for ( INT32 i = 0; i < BUFSIZE1; i++ )
   {
      buf[i] = c ;
   }
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // open lob
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, lobSize ) ;
   // get lob create time
//   rc = sdbGetLobCreateTime( lob, &createTime ) ;
//   ASSERT_EQ( 0, createTime ) ;
//   ASSERT_EQ( 0, createTime ) ;
   // write lob
   rc = sdbWriteLob( lob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BUFSIZE1, lobSize ) ;
   // write lob
   rc = sdbWriteLob( lob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 2 * BUFSIZE1, lobSize ) ;
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // open lob with the mode SDB_LOB_READ
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   // read lob
   rc = sdbReadLob( lob, 1000, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   // read lob
   rc = sdbReadLob( lob, BUFSIZE2, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) << "readCount is: " << readCount
         << ", c is: " << c << ", i is: "
         << i << ", readBuf[i] is: " << readBuf[i] ;
   }
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // reopen it, and read all the content
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = SDB_OK ;
   counter = 0 ;
   while( SDB_EOF != ( rc = sdbReadLob( lob, BUFSIZE2, readBuf, &readCount ) ) )
   {
       eof = sdbLobIsEof( lob ) ;
       ASSERT_EQ( SDB_OK, rc ) ;
       if ( TRUE == eof )
       {
          counter = 1 ;
       }
   }
   ASSERT_EQ( 1, counter ) ;
   //eof = sdbLobIsEof( lob ) ;
   //ASSERT_EQ( SDB_OK, rc ) ;
   //ASSERT_EQ( TRUE, eof ) ;
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // remove lob
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCursor ( cur ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}


/*
TEST(debug, sdbGetLastErrorObjTest)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *pIndexName         = "aIndex" ;
   bson errorResult ;
   bson indexDef ;
   bson_init( &indexDef ) ;
   bson_append_int( &indexDef, "a", 1 ) ;
   bson_finish( &indexDef ) ;
   bson_destroy( &indexDef ) ;

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
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   sleep( 1 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &errorResult ) ;
   rc = sdbGetLastErrorObj( connection, &errorResult ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   ASSERT_EQ( 0, bson_size( &errorResult ) ) ;
   bson_destroy( &errorResult ) ;

   // case 1:
   rc = sdbGetCollection1( collectionspace, "aaaa", &cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;
   bson_init( &errorResult ) ;
   rc = sdbGetLastErrorObj( connection, &errorResult ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "get cl fail, error obj is: \n" ) ;
   bson_print( &errorResult ) ;
   ASSERT_TRUE( bson_size( &errorResult ) > 5 ) ;
   bson_destroy( &errorResult ) ;

   bson_init( &errorResult ) ;
   rc = sdbGetLastErrorObj( collectionspace, &errorResult ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "get cl fail, error obj is: \n" ) ;
   bson_print( &errorResult ) ;
   ASSERT_TRUE( bson_size( &errorResult ) > 5 ) ;
   bson_destroy( &errorResult ) ;

   sdbCleanLastErrorObj( connection ) ;

   bson_init( &errorResult ) ;
   rc = sdbGetLastErrorObj( connection, &errorResult ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   ASSERT_EQ( 0, bson_size( &errorResult ) ) ;
   bson_destroy( &errorResult ) ;

   rc = sdbGetCollection1( collectionspace, "aaaa", &cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;

   sdbDisconnect ( connection ) ;

   sdbCleanLastErrorObj( connection ) ;
   sdbCleanLastErrorObj( collectionspace ) ;
   bson_init( &errorResult ) ;
   rc = sdbGetLastErrorObj( connection, &errorResult ) ;
   bson_destroy( &errorResult ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   bson_init( &errorResult ) ;
   rc = sdbGetLastErrorObj( collectionspace, &errorResult ) ;
   bson_destroy( &errorResult ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;


   sdbReleaseCollection ( cl ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;

   // case 2:
//   rc = sdbGetLastErrorObj( connection, &errorResult ) ;
//   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
//   rc = sdbGetLastErrorObj( collectionspace, &errorResult ) ;
//   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
//   sdbCleanLastErrorObj( connection ) ;
//   sdbCleanLastErrorObj( collectionspace ) ;
}

TEST(debug, sdbCloseAllCursors)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;
   INT32 num                      = 10 ;

   bson obj ;
   bson obj1 ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;
   bson conf ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME, &cl ) ;
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
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor1 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get record
   bson_init ( &obj3 ) ;
   rc = sdbCurrent( cursor, &obj3 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // close all the cursors
   rc = sdbCloseAllCursors( connection );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
//   // check
//   bson_init ( &obj ) ;
//   rc = sdbCurrent( cursor, &obj ) ; // getCurrent in cursor, expect SDB_OK
//   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
//   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
//   bson_init ( &obj1 ) ;
//   rc = sdbNext( cursor, &obj1 ) ; // getNext in cursor, -31
//   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
//   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
//   rc = sdbCloseCursor( cursor ) ; // close in cursor, 0
//   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_init ( &obj2 ) ;
//   rc = sdbNext( cursor1, &obj2 ) ; // getNext in cursor1, expect -31
//   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
//   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
//   bson_init ( &obj4 ) ;
//   rc = sdbCurrent( cursor1, &obj4 ) ; // getCurrent in cursor1, expect -31
//   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
//   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
//   rc = sdbCloseCursor( cursor1 ) ; // close in cursor1, expect 0
//   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_destroy( &obj );
//   bson_destroy( &obj1 );
//   bson_destroy( &obj2 );
   bson_destroy( &obj3 );
//   bson_destroy( &obj4 );

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbDisconnect( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(debug, sdbConnect_with_usr)
{
   sdbConnectionHandle connection  = 0 ;
   sdbCursorHandle cursor          = 0 ;
   INT32 rc                        = SDB_OK ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check whether it is in the cluster environment
   rc = sdbGetList( connection, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      printf("sdbConnect_with_usr is use in cluster environment only\n") ;
      return ;
   }
//   sdbReleaseCursor ( cursor ) ;
   // create a new user
   rc = sdbCreateUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect to db
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection( connection ) ;
   connection = 0 ;
   // connect to database again with usrname and passwd
   rc = sdbConnect ( HOST, SERVER, USERDEF, PASSWDDEF, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // Remove a user
   rc = sdbRemoveUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(debug, sdbQuery_with_some_flags)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   bson index ;
   bson condition ;
   bson select ;
   bson orderBy ;
   bson hint ;
   int num = 10;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //create indexes
   bson_init( &hint );
   bson_append_string( &hint, "", "indexForNotExist" );
   bson_finish( &hint );
   bson_destroy( &hint ) ;
   // insert some records

   insertRecords( cl, num ) ;
   printf( "query the specifeed record :" OSS_NEWLINE ) ;
   // execute query
   rc = sdbQuery1 ( cl, NULL, NULL,
                    NULL, NULL, 0, -1, 0, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // execute query
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   rc = sdbQuery1 ( cl, NULL, NULL,
                    NULL, NULL, 0, -1, QUERY_FORCE_HINT, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;
   // execute query
   rc = sdbQuery1 ( cl, NULL, NULL,
                    NULL, NULL, 0, -1, QUERY_WITH_RETURNDATA, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;
   // execute query
   rc = sdbQuery1 ( cl, NULL, NULL,
                    NULL, NULL, 0, -1, QUERY_FORCE_HINT | QUERY_WITH_RETURNDATA, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;
   // execute query
   rc = sdbQuery1 ( cl, NULL, NULL,
                    NULL, NULL, 0, -1, QUERY_FORCE_HINT | QUERY_PARALLED | QUERY_WITH_RETURNDATA, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   sdbReleaseCollection ( cl ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;

}
*/

/*

TEST(debug, load_unload_rename_cs_cl)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCSHandle cs2                = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // unload/load cs
   rc = sdbUnloadCollectionSpace( connection, COLLECTION_SPACE_NAME, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbLoadCollectionSpace( connection, COLLECTION_SPACE_NAME, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // rename cs/cl
   rc = sdbRenameCollectionSpace( connection, COLLECTION_SPACE_NAME, "tmp", NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollectionSpace( connection, "tmp", &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollectionSpace( connection, "tmp", &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbRenameCollection( cs, COLLECTION_NAME, "tmp", NULL ) ;   
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbRenameCollection( cs, "tmp", COLLECTION_NAME, NULL ) ;   
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbRenameCollectionSpace( connection, "tmp", COLLECTION_SPACE_NAME, NULL ) ;   
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollectionSpace( connection, COLLECTION_SPACE_NAME, &cs2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollectionSpace( connection, COLLECTION_SPACE_NAME, &cs2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseCS ( cs2 ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}


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

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_GROUPS,
                       NULL, NULL, NULL, &cursor ) ;
      ASSERT_TRUE( rc == SDB_RTN_COORD_ONLY ) ;
      //displayRecord( &cursor ) ;
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cl
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // insert some records--{firstName:"John",lastName:"Smith",age:50,"Id":999}
   insertRecords( collection, num ) ;
   // select
   bson_init( &select ) ;
   bson_append_string( &select, "firstName", "" ) ;
   bson_append_string( &select, "age", ""  ) ;
   bson_append_string( &select, "Id", "" ) ;
   bson_finish( &select ) ;
   // execute query
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME ,
                        &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // build insert record
   bson_init ( &obj ) ;
   bson_append_string( &obj, "_id", "abc" );
   bson_append_int ( &obj, "a", 1 ) ;
   bson_finish ( &obj ) ;
   // insert record into the current collection
   printf( "Insert a record, the record is:" OSS_NEWLINE ) ;
   bson_print( &obj ) ;
   rc = sdbInsert1 ( collection, &obj, &it ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc );
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get id
   ret = bson_iterator_string( &it );
   ASSERT_TRUE( memcmp(ret, "", sizeof("")) ) ;
   printf( "Ihe record is %s\n", ret );
   rc = bson_iterator_next( &it ) ;
   ASSERT_TRUE( rc == BSON_INT ) ;
   // display the inserted record by iterator
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // create cl with RepliSize = 0
   bson_init ( &conf ) ;
   bson_append_int ( &conf, "ReplSize", 0 ) ;
   bson_finish ( &conf ) ;
   rc = sdbCreateCollection1 ( cs, CLNAME, &conf, &cl ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE ( rc == SDB_OK ) ;
   // TO DO:
   // transaction begin
   rc = sdbTransactionBegin ( connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // insert a English record
   bson_init( &obj ) ;
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   bson_destroy ( &obj ) ;
   // transaction roll back
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
   // connect to database
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME, &cl ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // prepare some records
   insertRecords( cl, num ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   ASSERT_TRUE( num == count ) ;
   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;

   // check
   // get next
   bson_init ( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; // getNext in cursor, expect SDB_OK
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get current
   bson_init ( &obj1 ) ;
   rc = sdbCurrent( cursor, &obj1 ) ; // getCurrent in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get next
   bson_init ( &obj2 ) ;
   rc = sdbNext( cursor, &obj2 ) ; // getNext in cursor, -29
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_DMS_EOC ) ;
   // close cursor
   rc = sdbCloseCursor( cursor ) ; // close in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // get next again
   bson_init ( &obj3 ) ;
   rc = sdbNext( cursor, &obj3 ) ; // getNext in cursor1, expect -36
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc == SDB_RTN_CONTEXT_NOTEXIST ) ;
   // get currnet again
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
   // connect to database
   rc = sdbConnect ( host, port, username, password, &connection ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   // check whether it is in the cluster environment
   rc = sdbGetList( connection, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      printf("sdbConnect_with_usr is use in cluster environment only\n") ;
      return ;
   }
//
//   // create a new user
//   rc = sdbCreateUsr( connection, user, passwd ) ;
//   ASSERT_TRUE( rc == SDB_OK ) ;
//   // disconnect to db
//   sdbDisconnect ( connection ) ;
//   sdbReleaseConnection( connection ) ;
//   connection = 0 ;
//   // connect to database again with usrname and passwd
//   rc = sdbConnect ( host, port, user, passwd, &connection ) ;
//   ASSERT_TRUE( rc == SDB_OK ) ;
//   // Remove a user
//   rc = sdbRemoveUsr( connection, user, passwd ) ;
//   ASSERT_TRUE( rc == SDB_OK ) ;
//
   sdbDisconnect ( connection ) ;
}
*/
/*
TEST(debug, binary)
{
   INT32 rc = SDB_OK ;
   bson obj ;
//   const CHAR *str = "{ \"key\": { \"$binary\" : \"aGVsbG8gd29ybGQ=\",\"$type\": \"1\" } }" ;
   const CHAR *str = "hello world" ;
   int len = getEnBase64Size( str ) ;
   CHAR *out = (CHAR *) malloc( len ) ;
   rc = base64Encode ( str, strlen(str) + 1, out, len ) ;
   ASSERT_TRUE ( rc == SDB_OK ) ;
   printf( "hello world encode is: %s\n", out ) ;
   free ( out ) ;
}
*/
//
//TEST(debug, regex)
//{
//   sdbConnectionHandle db = 0 ;
//   sdbCSHandle cs    = 0 ;
//   sdbCollectionHandle cl = 0 ;
//   sdbCursorHandle cursor         = 0 ;
//   INT32 rc                       = SDB_OK ;
//   bson obj ;
//   bson rule ;
//   bson cond ;
//   CHAR buf[64] = { 0 } ;
//   const CHAR* regex = "^31" ;
//   const CHAR* options = "i" ;
//   INT32 num = 10 ;
//   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // connect to database
//   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
//   ASSERT_TRUE( rc == SDB_OK ) ;
//   // get cs
//   rc = getCollectionSpace ( db,
//                             COLLECTION_SPACE_NAME,
//                             &cs ) ;
//   ASSERT_TRUE( rc == SDB_OK ) ;
//   // get cl
//   rc = getCollection ( db,
//                        COLLECTION_FULL_NAME,
//                        &cl ) ;
//   CHECK_MSG("%s%d\n","rc = ", rc) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//
//   // insert some recored
//   for ( INT32 i = 0 ; i < num; i++ )
//   {
//      CHAR buff[32] = { 0 } ;
//      CHAR bu[2] = { 0 } ;
//      sprintf( bu,"%d",i ) ;
//      strcat( buff, "31" ) ;
//      strncat( buff, bu, 1 ) ;
//      bson_init ( &obj ) ;
//      bson_append_string( &obj, "name", buff  ) ;
//      bson_append_int ( &obj, "age", 30 + i ) ;
//      bson_finish( &obj ) ;
//      rc = sdbInsert( cl, &obj ) ;
//      ASSERT_TRUE( rc==SDB_OK ) ;
//      bson_destroy( &obj ) ;
//   }
//   // insert some recored
//   for ( INT32 i = 0 ; i < num; i++ )
//   {
//      CHAR buff[32] = { 0 } ;
//      CHAR bu[2] = { 0 } ;
//      sprintf( bu, "%d", i ) ;
//      strcat( buff, "41" ) ;
//      strncat( buff, bu, 1 ) ;
//      bson_init ( &obj ) ;
//      bson_append_string( &obj, "name", buff  ) ;
//      bson_append_int ( &obj, "age", 40 + i ) ;
//      bson_finish( &obj ) ;
//      rc = sdbInsert( cl, &obj ) ;
//      ASSERT_TRUE( rc==SDB_OK ) ;
//      bson_destroy( &obj ) ;
//   }
//
//   // cond
//   bson_init ( &cond ) ;
//// *************************************************************
//// 情况1
////   bson_append_regex( &cond, "name", regex, options ) ;
//// *************************************************************
//// 情况2
//     bson_append_start_object( &cond, "name" ) ;
//     bson_append_string ( &cond, "$regex", "^31" ) ;
//     bson_append_string ( &cond, "$options", "i" ) ;
//     bson_append_finish_object( &cond ) ;
//// *************************************************************
//   rc = bson_finish( &cond ) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//printf("cond is: \n" ) ;
//bson_print(&cond) ;
//   // rule
//   bson_init ( &rule ) ;
//   bson_append_start_object( &rule, "$set" ) ;
//   bson_append_int ( &rule, "age", 999 ) ;
//   bson_append_finish_object( &rule ) ;
//   rc = bson_finish ( &rule ) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//   // update with regex expression
//   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
//   CHECK_MSG("%s%d\n","rc = ", rc) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//
//   // query
//   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//   // print the records
//   displayRecord( &cursor ) ;
//
//   sdbDisconnect ( db ) ;
//   sdbReleaseCursor ( cursor ) ;
//   sdbReleaseCollection ( cl ) ;
//   sdbReleaseCS ( cs ) ;
//   sdbReleaseConnection ( db ) ;
//}
//

//TEST( debug, sdbQueryAndRemove )
//{
//   sdbConnectionHandle connection = 0 ;
//   sdbCSHandle cs                 = 0 ;
//   sdbCollectionHandle cl         = 0 ;
//   sdbCursorHandle cursor         = 0 ;
//   INT32 rc                       = SDB_OK ;
//   SINT64 NUM                     = 100 ;
//   SINT64 count                   = 0 ;
//   INT32 i                        = 0 ;
//   INT32 set_value                = 100 ;
//   const CHAR *pIndexName1        = "test_index1" ;
//   const CHAR *pIndexName2        = "test_index2" ;
//   const CHAR *pField1            = "testQueryAndUpdate1" ;
//   const CHAR *pField2            = "testQueryAndUpdate2" ;
//
//
//   bson index ;
//   bson tmp ;
//   bson condition ;
//   bson selector ;
//   bson orderBy ;
//   bson hint ;
//
//   // initialize
//   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // connect to database
//   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // get cs
//   rc = getCollectionSpace ( connection,
//                             COLLECTION_SPACE_NAME,
//                             &cs ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // get cl
//   rc = getCollection ( connection,
//                        COLLECTION_FULL_NAME,
//                        &cl ) ;
//   CHECK_MSG("%s%d\n","rc = ", rc) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // create index
//   bson_init( &index ) ;
//   bson_append_int( &index, pField1, -1 ) ;
//   bson_finish( &index ) ;
//   rc = sdbCreateIndex( cl, &index, pIndexName1, FALSE, FALSE ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   bson_destroy( &index ) ;
//
//   bson_init( &index ) ;
//   bson_append_int( &index, pField2, 1 ) ;
//   bson_finish( &index ) ;
//   rc = sdbCreateIndex( cl, &index, pIndexName2, FALSE, FALSE ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   bson_destroy( &index ) ;
//
//   // gen some record
//   for ( i = 0; i < NUM; i++ )
//   {
//      bson obj ;
//      bson_init( &obj ) ;
//      bson_append_int( &obj, pField1, i ) ;
//      bson_append_int( &obj, pField2, i ) ;
//      bson_finish( &obj ) ;
//      rc = sdbInsert( cl, &obj ) ;
//      ASSERT_EQ( SDB_OK, rc ) ;
//      bson_destroy( &obj ) ;
//   }
//
//   /// in case: use extend sort 
//   bson_init( &selector ) ;
//   bson_append_string( &selector, pField2, "" ) ;
//   bson_finish( &selector ) ;
//   
//   bson_init( &orderBy ) ;
//   bson_append_int( &orderBy, pField2, 1 ) ;
//   bson_finish( &orderBy ) ;
//
//   bson_init( &tmp ) ;
//   bson_append_int( &tmp, "$gte", 0 ) ;
//   bson_finish( &tmp ) ;
//   bson_init( &condition ) ;
//   bson_append_bson( &condition, pField1, &tmp ) ;
//   bson_finish( &condition ) ;
//
//   bson_init( &hint ) ;
//   bson_append_string( &hint, "", pIndexName1 ) ;
//   bson_finish( &hint ) ;
//
//   rc = sdbQueryAndRemove( cl, &condition, &selector, &orderBy, &hint,
//                           0, -1, 0, &cursor ) ;
//   ASSERT_EQ( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, rc ) ;
//   bson_destroy( &hint ) ;
//
//   /// in case: does not use extend sort 
//   bson_init( &hint ) ;
//   bson_append_string( &hint, "", pIndexName2 ) ;
//   bson_finish( &hint ) ;
//   
//   // test
//   rc = sdbQueryAndRemove( cl, &condition, &selector, &orderBy, &hint,
//                           50, 10, 0x00000080, &cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   bson_destroy( &tmp ) ;
//   bson_destroy( &condition ) ;
//   bson_destroy( &selector ) ;
//   bson_destroy( &orderBy ) ;
//   bson_destroy( &hint ) ;
//   // check
//   i = 0 ;
//   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
//   {
//      bson_iterator it ;
//      bson_iterator_init( &it, &tmp ) ;
//      const CHAR *pKey = bson_iterator_key( &it ) ;
//      ASSERT_EQ( 0, strncmp( pKey, pField2, strlen(pField2) ) ) ;
//      INT32 value =  bson_iterator_int( &it ) ;
//      ASSERT_EQ( 50 + i, value ) ;
//      bson_destroy( &tmp ) ;
//      i++ ;
//   }
//   ASSERT_EQ( 10, i ) ;
//   i = 100 ;
//   while ( i-- )
//   {
//      rc = sdbGetCount( cl, NULL, &count ) ;
//      ASSERT_EQ( rc, SDB_OK ) ;
//      if ( 0 == count )
//         break ;
//   }
//   if ( 0 == i )
//      ASSERT_EQ( 0, count ) ;
//
//   // realse
//   sdbCloseCursor( cursor ) ;
//   sdbReleaseCursor ( cursor ) ;
//   sdbReleaseCollection ( cl ) ;
//   sdbReleaseCS ( cs ) ;
//   sdbDisconnect ( connection ) ;
//   sdbReleaseConnection ( connection ) ;
//}
//


//struct MyThreadFunc
//{
//   void operator()()
//   {
//      INT32 rc = SDB_OK ;
//      sdbConnectionHandle db = 0 ;
//      
//      rc = sdbConnect( "192.168.20.165", "11810", "", "", &db ) ;
//      if ( SDB_OK != rc )
//      {
//         cout << "Error: failed to connect to database, rc = " << rc << endl ;
//         return ;
//      }
//      sdbDisconnect( db ) ;
//      sdbReleaseConnection( db ) ;
//      cout << "ok" <<endl ;
//   }
//} threadFun ;
//
//TEST( debug, multi_thread_build_connect )
//{
//   boost::thread myThread( threadFun ) ;
//
//   boost::thread::yield() ;
//
//   myThread.join() ;
//   
//}

//TEST( debug, alter_collection )
//{
//   sdbConnectionHandle db = 0 ;
//   sdbCSHandle cs         = 0 ;
//   sdbCollectionHandle cl = 0 ;
//   sdbCursorHandle cursor = 0 ;
//
//   INT32 rc                = SDB_OK ;
//   const CHAR *pCSName     = "test_alter_cs_in_c" ;
//   const CHAR *pCLName     = "test_alter_cl_in_c" ;
//   const CHAR *pCLFullName = "test_alter_cs_in_c.test_alter_cl_in_c" ;
//   const CHAR *pValue      = NULL ;
//   INT32 n_value = 0 ;
//   bson_iterator it ;
//   bson_iterator it2 ;
//   bson option ;
//   bson matcher ;
//   bson record ;
//   bson obj ;
//
//   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // connect to database
//   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   if ( FALSE == isCluster( db ) )
//   {
//      return ;
//   }
//
//   // drop cs
//   rc = sdbDropCollectionSpace( db, pCSName ) ;
//   if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
//   {
//      ASSERT_EQ( 0, 1 ) << "failed to drop cs " << pCSName ;
//   }
//
//   // create cs 
//   rc = sdbCreateCollectionSpace( db, pCSName, 4096, &cs ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // create cl 
//   rc = sdbCreateCollection( cs, pCLName, &cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_init( &option ) ;
//   bson_append_int( &option, "ReplSize", 0 ) ;
//   bson_append_start_object( &option, "ShardingKey" ) ;
//   bson_append_int( &option, "a", 1 ) ;
//   bson_append_finish_object( &option ) ;
//   bson_append_string( &option, "ShardingType", "hash" ) ;
//   bson_append_int( &option, "Partition", 1024 ) ;
//   bson_finish( &option) ;
//
//   rc = sdbAlterCollection( cl, &option ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // check
//   bson_init( &matcher ) ;
//   bson_append_string( &matcher, "Name", pCLFullName ) ;
//   bson_finish( &matcher ) ;
//   rc = sdbGetSnapshot( db, SDB_SNAP_CATALOG, &matcher, NULL, NULL, &cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_init( &record ) ;
//   rc = sdbNext( cursor, &record ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // check Name
//   if ( BSON_STRING != bson_find( &it, &record, "Name" ) )
//   {
//      ASSERT_EQ( 0, 1 ) << "after alter cl, the snapshot record is not the one we want" ;
//   }
//   pValue = bson_iterator_string( &it ) ;
//   ASSERT_EQ( 0, strcmp( pValue, pCLFullName ) ) << "after alter cl, the cl's name is not what we want" ;
//
//   // check ReplSize
//   if ( BSON_INT != bson_find( &it, &record, "ReplSize" ) )
//   {
//      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding type is not the one we want" ;
//   }
//   n_value = bson_iterator_int( &it ) ;
//   ASSERT_EQ( 7, n_value ) ;
//
//   // check ShardingType
//   if ( BSON_STRING != bson_find( &it, &record, "ShardingType" ) )
//   {
//      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding type is not the noe we want" ;
//   }
//   pValue = bson_iterator_string( &it ) ;
//
//   // check partition
//   if ( BSON_INT != bson_find( &it, &record, "Partition" ) )
//   {
//      ASSERT_EQ( 0, 1 ) << "after alter cl, the partition is not the one we want" ;
//   }
//   n_value = bson_iterator_int( &it ) ;
//   ASSERT_EQ( 1024, n_value ) ;
//
//   // check ShardingKey
//   if ( BSON_OBJECT != bson_find( &it, &record, "ShardingKey" ) )
//   {
//      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding key is not the one we want" ;
//   }
//   bson_iterator_subobject( &it, &obj ) ;
//   if ( BSON_INT != bson_find( &it2, &obj, "a" ) )
//   {
//      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding key is not the one we want" ; 
//   }
//   n_value = bson_iterator_int( &it2 ) ;
//   ASSERT_EQ( 1, n_value ) ; 
//   
//   rc = sdbDropCollectionSpace( db, pCSName ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_destroy( &option ) ;
//   bson_destroy( &matcher ) ;
//   bson_destroy( &record ) ;
//   bson_destroy( &obj ) ;
//
//
//   sdbDisconnect ( db ) ;
//   sdbReleaseCursor ( cursor ) ;
//   sdbReleaseCollection ( cl ) ;
//   sdbReleaseCS ( cs ) ;
//   sdbReleaseConnection ( db ) ;
//}
//
//TEST( debug, create_and_remove_id_index )
//{
//   sdbConnectionHandle db = 0 ;
//   sdbCSHandle cs         = 0 ;
//   sdbCollectionHandle cl = 0 ;
//   sdbCursorHandle cursor = 0 ;
//
//   INT32 rc               = SDB_OK ;  
//   const CHAR *pIndexName = "$id" ;
//   INT32 count            = 0 ;
//   bson obj ;
//   bson record ;
//   bson updater ;
//
//   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // connect to database
//   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // get cs
//   rc = getCollectionSpace ( db, COLLECTION_SPACE_NAME, &cs ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // get cl
//   rc = getCollection ( db, COLLECTION_FULL_NAME, &cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_init( &obj ) ;
//   bson_append_int( &obj, "a", 1 ) ;
//   bson_finish( &obj ) ;
//
//   bson_init( &record ) ;
//   bson_append_start_object( &record, "$set" ) ;
//   bson_append_int( &record, "a", 2 ) ;
//   bson_append_finish_object( &record ) ;
//   bson_finish( &record ) ;
//
//   bson_init( &updater ) ; 
//   bson_append_start_object( &updater, "$set" ) ;
//   bson_append_int( &updater, "a", 10 ) ;
//   bson_append_finish_object( &updater ) ;
//   rc = bson_finish( &updater ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   rc = sdbInsert( cl, &obj ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // test
//   rc = sdbDropIdIndex( cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // check
//   rc = sdbGetIndexes( cl, pIndexName, &cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 0, count ) << "after drop id index, &id index still exist" ;
//   
//   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   count = 0 ;
//   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 1, count ) ;
//
//   rc = sdbUpdate( cl, &updater, NULL, NULL ) ;
//   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;
//
//   rc = sdbUpsert( cl, &updater, NULL, NULL ) ;
//   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;
//
//   rc = sdbDelete( cl, NULL, NULL ) ;
//   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;
//
//   // test
//   rc = sdbCreateIdIndex( cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   rc = sdbDelete( cl, NULL, NULL ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//  
//   count = 0 ;
//   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 0, count ) ;
//
//   rc = sdbUpsert( cl, &record, NULL, NULL ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   count = 0 ;
//   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 1, count ) ;
//
//   rc = sdbUpdate( cl, &updater, NULL, NULL ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   bson_init( &obj ) ;
//   bson_append_int( &obj, "a", 10 ) ;
//   bson_finish( &obj ) ;
//   rc = sdbQuery( cl, &obj, NULL, NULL, NULL, 0, -1, &cursor ) ;
//   count = 0 ;
//   while( SDB_OK == ( rc = sdbNext( cursor, &record ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 1, count ) ;
//   
//   bson_destroy( &record ) ;
//   bson_destroy( &obj ) ;
//   bson_destroy( &updater ) ;
//
//   sdbDisconnect ( db ) ;
//   sdbReleaseCursor ( cursor ) ;
//   sdbReleaseCollection ( cl ) ;
//   sdbReleaseCS ( cs ) ;
//   sdbReleaseConnection ( db ) ;
//}
