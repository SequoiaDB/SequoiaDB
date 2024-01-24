#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

#define USERDEF       "sequoiadb"
#define PASSWDDEF     "sequoiadb"

using namespace std ;

TEST(sdb,sdbConnect_without_usr)
{
   sdbConnectionHandle connection = 0 ;
   INT32 rc                       = SDB_OK ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbConnect_with_usr)
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
   sdbReleaseCursor ( cursor ) ;
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
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbConnect_with_several_address)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR* connArr[9] = {"192.168.20.35:12340",
                              "192.168.20.36:12340",
                              "123:123",
                              "",
                              ":12340",
                              "192.168.20.40",
                              "localhost:50000",
                              "192.168.31.17:21810",
                              "localhost:11810"} ;
   // connect to database
   rc = sdbConnect1 ( connArr, 9, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( connection, 4, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbCreateUsr)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check whether it is in the cluster environment
   rc = sdbGetList( connection, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      printf("sdbCreateUsr() is use in cluster environment only\n") ;
      return ;
   }
   sdbReleaseCursor ( cursor ) ;
   cursor = 0 ;
   // create a new user
   rc = sdbCreateUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // Remove a user
   rc = sdbRemoveUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbRemoveUsr)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check whether it is in the cluster environment
   rc = sdbGetList( connection, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      printf("sdbRemoveUsr() is use in cluster environment only\n") ;
      return ;
   }
   sdbReleaseCursor ( cursor ) ;
   cursor = 0 ;
   // create a new user
   rc = sdbCreateUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // Remove a user
   rc = sdbRemoveUsr( connection, USERDEF, PASSWDDEF ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbCreateCollectionSpace)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollectionSpace ( connection,
                                 COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbDropCollectionSpace)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;   
   INT32 rc                       = SDB_OK ;
   bson options1 ;
   bson options2 ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //case 1:no options parameter
   rc = sdbDropCollectionSpace ( connection,
                                 COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //case 2:the options parameter is {"EnsureEmpty" : true}
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollection( collectionspace, COLLECTION_NAME, &collection) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &options1 ) ;
   bson_append_bool( &options1, "EnsureEmpty", TRUE ) ;
   rc = sdbDropCollectionSpace1 ( connection,
                                 COLLECTION_SPACE_NAME, &options1) ;
   ASSERT_EQ(  SDB_DMS_CS_NOT_EMPTY, rc ) ;  
   bson_destroy( &options1 ) ;
   //case 3:the options parameter is {"EnsureEmpty" : false}
   bson_init( &options2 ) ;
   bson_append_bool( &options2, "EnsureEmpty", FALSE ) ;
   rc = sdbDropCollectionSpace1 ( connection,
                                 COLLECTION_SPACE_NAME, &options2) ;
   ASSERT_EQ(  SDB_OK, rc ) ;  
   bson_destroy( &options2 ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbGetCollectionSpace)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbGetCollection)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCollection ( connection, COLLECTION_FULL_NAME,
                           &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbListCollectionSpaces)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbListCollectionSpaces ( connection, &cursor ) ;
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

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbListCollections)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbListCollections ( connection, &cursor ) ;
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

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbListReplicaGroups)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check whether it is in the cluster environment
   rc = sdbGetList( connection, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      printf("sdbListReplicaGroups is use in cluster environment only\n") ;
      return ;
   }
   sdbReleaseCursor ( cursor ) ;
   cursor = 0 ;
   rc = sdbListReplicaGroups ( connection, &cursor ) ;
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

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbExec)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *sql1 = "insert into testfoo.testbar( name, age ) \
                       values( \"啊怪\", 28 )" ;
   const CHAR *sql2 = "select * from testfoo.testbar" ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbExecUpdate ( connection, sql1 ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbExec ( connection, sql2, &cursor ) ;
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

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbExecUpdate)
{
   sdbConnectionHandle connection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *sql2 = "INSERT into testfoo.testbar ( name, age )\
 values( \"Aimee\",23 )" ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbExecUpdate ( connection, sql2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("The operation is :\n") ;
   printf("%s\n",sql2) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbTransactionBegin)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;
   BOOLEAN isTranOnFlag           = FALSE ;
   bson conf ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( false == isCluster(connection) )
   {
      printf( "it's not in cluster environment\n" ) ;
      return ;
   }
   rc = isTranOn( connection, &isTranOnFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( FALSE == isTranOnFlag )
   {
      sdbDisconnect( connection ) ;
      sdbReleaseConnection( connection ) ;
      printf( "transaction is disable\n" ) ;
      return ;
   }
   // get cs
   rc = getCollectionSpace ( connection,
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
   // TO DO:
   // transaction begin
   rc = sdbTransactionBegin ( connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert a English record
   bson_init( &obj ) ;
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // transaction commit
   rc = sdbTransactionCommit( connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   CHECK_MSG("%s%lld\n", "count = ", count);
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, count ) ;

   bson_destroy( &conf ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbTransactionCommit)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;
   BOOLEAN isTranOnFlag           = FALSE ;

   bson conf ;
   bson obj ;
   bson tmp ;
   bson c ;
   bson record ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( false == isCluster(connection) )
   {
      printf( "it's not in cluster environment\n" ) ;
      return ;
   }
   rc = isTranOn( connection, &isTranOnFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( FALSE == isTranOnFlag )
   {
      sdbDisconnect( connection ) ;
      sdbReleaseConnection( connection ) ;
      printf( "transaction is disable\n" ) ;
      return ;
   }
   // get cs
   rc = getCollectionSpace ( connection,
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
   // TO DO:
   // get count
   // transaction begin
   rc = sdbTransactionBegin ( connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert a English record
   bson_init( &obj ) ;
   bson_init( &tmp ) ;
   createEnglishRecord( &obj  ) ;
   bson_copy( &tmp, &obj ) ;
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &tmp ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &tmp ) ;
   // transaction commit
   rc = sdbTransactionCommit( connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check, read from master
   rc = sdbGetCount ( cl, NULL, &count ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc );
   CHECK_MSG( "%s%lld\n", "count = ", count );
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 2, count ) ;

   bson_destroy( &conf ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb,sdbTransactionRollback)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   const CHAR *CLNAME             = "transaction" ;
   BOOLEAN isTranOnFlag           = FALSE ;

   bson obj ;
   bson conf ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( false == isCluster(connection) )
   {
      printf( "it's not in cluster environment\n" ) ;
      return ;
   }
   rc = isTranOn( connection, &isTranOnFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if ( FALSE == isTranOnFlag )
   {
      sdbDisconnect( connection ) ;
      sdbReleaseConnection( connection ) ;
      printf( "transaction is disable\n" ) ;
      return ;
   }
   // get cs
   rc = getCollectionSpace ( connection,
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
   // TO DO:
   // transaction begin
   rc = sdbTransactionBegin ( connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert a English record
   bson_init( &obj ) ;
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // transaction roll back
   rc = sdbTransactionRollback( connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetCount ( cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, count ) ;

   bson_destroy( &conf ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCollection ( cl ) ;
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

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbDisconnect( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb, sdbCloseAllCursors)
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
   // check
   bson_init ( &obj ) ;
   rc = sdbCurrent( cursor, &obj ) ; // getCurrent in cursor, expect SDB_OK
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   bson_init ( &obj1 ) ;
   rc = sdbNext( cursor, &obj1 ) ; // getNext in cursor, -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = sdbCloseCursor( cursor ) ; // close in cursor, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init ( &obj2 ) ;
   rc = sdbNext( cursor1, &obj2 ) ; // getNext in cursor1, expect -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   bson_init ( &obj4 ) ;
   rc = sdbCurrent( cursor1, &obj4 ) ; // getCurrent in cursor1, expect -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = sdbCloseCursor( cursor1 ) ; // close in cursor1, expect 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

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

TEST(sdb, sdbCloseAllCursors_cursor_close_first)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
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
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor2 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor3 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get record
   bson_init ( &obj3 ) ;
   rc = sdbNext( cursor, &obj3 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // close cursor
   rc = sdbCloseCursor( cursor1 ) ; // close cursor1
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseCursor( cursor ) ; // close cursor
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( SDB_OK, rc ) ;
   // TO DO:
   // close all the cursors
   rc = sdbCloseAllCursors( connection );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseAllCursors( connection );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   bson_init ( &obj ) ;
   rc = sdbCurrent( cursor, &obj ) ; // getCurrent in cursor, expect -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   bson_init ( &obj1 ) ;
   rc = sdbNext( cursor, &obj1 ) ; // getNext in cursor, expect -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = sdbCloseCursor( cursor ) ; // close in cursor, expect 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init ( &obj2 ) ;
   rc = sdbNext( cursor1, &obj2 ) ; // getNext in cursor1, -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   bson_init ( &obj4 ) ;
   rc = sdbCurrent( cursor1, &obj4 ) ; // getCurrent in cursor1, -31
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = sdbCloseCursor( cursor1 ) ; // close in cursor1, 0
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &obj );
   bson_destroy( &obj1 );
   bson_destroy( &obj2 );
   bson_destroy( &obj3 );
   bson_destroy( &obj4 );

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb, sdbIsClosed)
{
   sdbConnectionHandle connection  = 0 ;
   sdbConnectionHandle connection1 = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   BOOLEAN result = FALSE ;
   bson conf ;
   bson obj ;
   bson tmp ;
   bson c ;
   bson record ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, sdbIsClosed( connection ) ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, sdbIsClosed( connection ) ) ;
   ASSERT_EQ( TRUE, sdbIsValid( connection ) ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // TO DO:
   // scene 1
   // test when we get nornal business packet back from server,
   // wether sdbIsValid() return false. if so, it means error
   result = sdbIsValid( connection );
   std::cout << "before close connection, result is " << result << std::endl ;
   ASSERT_EQ( TRUE, result ) ;

   // scene 2
   // test close connection manually
   result = FALSE ;
   result = sdbIsValid( connection );
   std::cout << "after close connection manually, result is " << result << std::endl ;
   ASSERT_EQ( TRUE, result ) ;

   // scene 3
   // test close after disconnect
   sdbDisconnect ( connection ) ;
   result = TRUE ;
   result = sdbIsValid( connection );
   std::cout << "after close connection, result is " << result << std::endl ;
   ASSERT_EQ ( FALSE, result ) ;
   result = FALSE ;
   result = sdbIsClosed( connection ) ;
   ASSERT_EQ ( TRUE, result ) ;
   
   sdbDisconnect( connection ) ;
   sdbDisconnect( connection1 ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( connection ) ;
   sdbReleaseConnection ( connection1 ) ;
}

TEST(sdb, truncate)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   INT32 i                        = 0 ;
   INT32 num                      = 100 ;
   SINT64 totalNum                = 0 ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
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
   rc = sdbTruncateCollection( connection, COLLECTION_FULL_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, totalNum ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(sdb, sdbGetLastErrorObjTest)
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

   // case 2:
   sdbCleanLastErrorObj( connection ) ;
   sdbCleanLastErrorObj( collectionspace ) ;
   rc = sdbGetLastErrorObj( connection, &errorResult ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   rc = sdbGetLastErrorObj( collectionspace, &errorResult ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;

}

TEST(sdb, sdbGetListAndSnapshot)
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
   // get list
   rc = sdbGetList1( db, SDB_LIST_COLLECTIONS, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   ASSERT_TRUE( rc == SDB_OK ) ;
   cout << "get list shows: " << endl ;
   displayRecord( &cursor ) ;
   // get snapshot
   rc = sdbGetSnapshot1( db, SDB_SNAP_COLLECTIONS, NULL, NULL, NULL, NULL, 0, -1, &cursor1 ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   cout << "get snapshot shows: " << endl ;
   displayRecord( &cursor1 ) ;

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(sdb, sdbCreateUserOutOfRange)
{
   INT32 rc                       = SDB_OK ;
   sdbConnectionHandle connection = 0 ;
   CHAR *outOfRange               = NULL ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // max size 256, need 257 + 1 for \0
   outOfRange = allocMemory( 258 ) ;
   memset( outOfRange, 'a', 257 ) ;
   rc = sdbCreateUsr( connection, USERDEF, outOfRange ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   rc = sdbCreateUsr( connection, outOfRange, PASSWDDEF ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   freeMemory( outOfRange ) ;
   sdbDisconnect( connection ) ;
   sdbReleaseConnection( connection ) ;
}

TEST(sdb, sdbCreateUserWithMaxSize)
{
   INT32 rc                       = SDB_OK ;
   sdbConnectionHandle connection = 0 ;
   CHAR *maxCharSize              = NULL ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   maxCharSize = allocMemory( 257 ) ;
   memset( maxCharSize, 'a', 256 ) ;
   rc = sdbCreateUsr( connection, maxCharSize, maxCharSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbRemoveUsr( connection, maxCharSize, maxCharSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   freeMemory( maxCharSize) ;
   sdbDisconnect( connection ) ;
   sdbReleaseConnection( connection ) ;
}

/*
the follow APIs it waiting to add at this file :
sdbBackupOffline
sdbListBackup
sdbRemoveBackup
sdbListTasks
sdbWaitTasks
sdbCancelTask
snapshot(8)
list(8)
sdbResetSnapshot()
sdbFlushConfigure()
sdbCrtJSProcedure()
sdbRmProcedures()
sdbListProcedures()
sdbEvalJS()
sdbSetSessionAttr()
_sdbMsg()
*/
