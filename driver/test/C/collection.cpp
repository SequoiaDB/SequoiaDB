#include <stdio.h>
#include <gtest/gtest.h>
#include <string>
#include "testcommon.h"
#include "client.h"

TEST(collection,sdbGetName)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   CHAR pBuffer[ NAME_LEN + 1 ]   = { 0 } ;
   CHAR pBuffer2[ 1 ]             = { 0 } ;
   CHAR *pBuffer3                 = NULL ;
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
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = sdbGetCSName( collectionspace, pBuffer, NAME_LEN + 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, COLLECTION_SPACE_NAME, strlen(COLLECTION_SPACE_NAME) ) ) ;

   rc = sdbGetCLName( collection, pBuffer, NAME_LEN + 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, COLLECTION_NAME, strlen(COLLECTION_NAME) ) ) ;
     
   rc = sdbGetCLFullName( collection, pBuffer, NAME_LEN + 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, COLLECTION_FULL_NAME, strlen(COLLECTION_FULL_NAME) ) ) ;

   rc = sdbGetCLFullName( collection, pBuffer2, 1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;

   rc = sdbGetCLFullName( collection, pBuffer3, 1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbGetIndexes)
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
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get index
   rc = sdbGetIndexes( collection, "$id", &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   displayRecord( &cursor ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbCreateIndex)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   const CHAR *pIndexName1        = INDEX_NAME ;
   const CHAR *pIndexName2        = "index_c_offline" ;
   const CHAR *pStr               = NULL ;
   INT32 rc                       = SDB_OK ;
   bson_iterator it1 ;
   bson_iterator it2 ;
   bson obj ;
   bson tmp_obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   /// create index in online mode
   // build a bson for index definition
   bson_init( &obj ) ;
   bson_append_int( &obj, "name", 1 ) ;
   bson_append_int( &obj, "age", -1 ) ;
   bson_finish( &obj ) ;
   // create index
   rc = sdbCreateIndex ( collection, &obj, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // get the newly build index
   rc = sdbGetIndexes( collection, pIndexName1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   bson_init( &obj ) ;
   rc = sdbCurrent( cursor, &obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After creating index in online mode ,the current index is:\n" ) ;
   bson_print( &obj ) ;
   bson_iterator_init( &it1, &obj ) ;
   bson_init( &tmp_obj ) ;
   while( BSON_EOO != ( bson_iterator_next ( &it1 ) ) )
   {
      const CHAR *pKey = bson_iterator_key( &it1 ) ;
      if ( 0 == strncmp( pKey, "IndexDef", sizeof("IndexDef") ) )
      {
         bson_iterator it ;
         bson tmp_obj ;
         bson_init( &tmp_obj ) ;
         bson_iterator_subobject( &it1, &tmp_obj ) ;
         bson_iterator_init( &it, &tmp_obj ) ;
         while( BSON_EOO != ( bson_iterator_next ( &it ) ) )
         {
            const CHAR *pKey2 = bson_iterator_key( &it ) ;
            if ( 0 == strncmp( pKey2, "name", sizeof("name") ) )
            {
               pStr = bson_iterator_string( &it ) ;
               ASSERT_EQ( 0, strncmp( pStr, pIndexName1, sizeof(pIndexName1) ) ) ;
            }
         }
         bson_destroy( &tmp_obj ) ;
      }
   }
   bson_destroy ( &obj ) ;

   /// create index in offline mode
   bson_init( &obj ) ;
   bson_append_int( &obj, "name2", 1 ) ;
   bson_append_int( &obj, "age2", -1 ) ;
   bson_finish( &obj ) ;
   // create index
   rc = sdbCreateIndex1 ( collection, &obj, pIndexName2, TRUE, TRUE, 100 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // get the newly build index
   sdbReleaseCursor ( cursor ) ;
   rc = sdbGetIndexes( collection, pIndexName2, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   bson_init( &obj ) ;
   rc = sdbCurrent( cursor, &obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After creating index in offline mode ,the current index is:\n" ) ;
   bson_print( &obj ) ;
   bson_iterator_init( &it1, &obj ) ;
   bson_init( &tmp_obj ) ;
   while( BSON_EOO != ( bson_iterator_next ( &it1 ) ) )
   {
      const CHAR *pKey = bson_iterator_key( &it1 ) ;
      if ( 0 == strncmp( pKey, "IndexDef", sizeof("IndexDef") ) )
      {
         bson_iterator it ;
         bson tmp_obj ;
         bson_init( &tmp_obj ) ;
         bson_iterator_subobject( &it1, &tmp_obj ) ;
         bson_iterator_init( &it, &tmp_obj ) ;
         while( BSON_EOO != ( bson_iterator_next ( &it ) ) )
         {
            const CHAR *pKey2 = bson_iterator_key( &it ) ;
            if ( 0 == strncmp( pKey2, "name", sizeof("name") ) )
            {
               pStr = bson_iterator_string( &it ) ;
               ASSERT_EQ( 0, strncmp( pStr, pIndexName2, sizeof(pIndexName2) ) ) ;
            }
         }
         bson_destroy( &tmp_obj ) ;
      }
   }
   bson_destroy ( &obj ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbCreateIndex2)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   const CHAR *pIndexName         = INDEX_NAME ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   bson options ;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection ( connection, COLLECTION_FULL_NAME, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "name", 1 ) ;
   bson_finish( &obj ) ;

   bson_init( &options ) ;
   bson_append_int( &options, "SortBufferSize", 1024 ) ;
   bson_finish( &options ) ;

   rc = sdbCreateIndex2 ( collection, &obj, pIndexName, &options ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetIndexes( collection, pIndexName, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy ( &obj ) ;
   bson_destroy ( &options ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbDropIndex)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME,
                        &collection ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj ) ;
   // build a bson for index definition
   bson_append_int( &obj, "name", 1 ) ;
   bson_append_int( &obj, "age", -1 ) ;
   bson_finish( &obj ) ;
   // create index
   rc = sdbCreateIndex ( collection, &obj, INDEX_NAME, FALSE, FALSE ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // drop the newly build index
   rc = sdbDropIndex( collection, INDEX_NAME ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the index
   rc = sdbGetIndexes( collection, INDEX_NAME, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   bson_init( &obj ) ;
   rc = sdbCurrent( cursor, &obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   printf("obj is :\n") ;
   bson_print ( &obj ) ;
   // make sure the index is not exist now
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   bson_destroy ( &obj ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbInsert_with_EN_and_CH)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME,
                        &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert records into the current collection
   printf( "Insert an English record and a Chinese record." OSS_NEWLINE ) ;
   bson_init( &obj ) ;
   // insert a English record
   createEnglishRecord ( &obj  ) ;
   printf( "The English record is:\n" ) ;
   bson_print( &obj ) ;
   rc = sdbInsert ( collection, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // insert a Chinese record
   bson_init( &obj ) ;
   createChineseRecord( &obj ) ;
   printf( "The Chinese record is:" OSS_NEWLINE ) ;
   bson_print( &obj ) ;
   rc = sdbInsert ( collection, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   rc = sdbQuery( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // display the record we insert
   printf( "The records inserted are as below:" OSS_NEWLINE ) ;
   displayRecord( &cursor ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbInsert1)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   const char *key                = NULL ;
   int value                      = 0 ;
   bson obj ;
   bson_iterator it ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME ,
                        &collection ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build insert record
   bson_init ( &obj ) ;
   bson_append_int ( &obj, "a", 1 ) ;
   bson_finish ( &obj ) ;
   // insert record into the current collection
   printf( "Insert a record, the record is:" OSS_NEWLINE ) ;
   bson_print( &obj ) ;
   rc = sdbInsert1 ( collection, &obj, &it ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = bson_iterator_next( &it ) ;
   ASSERT_EQ( BSON_INT, rc ) ;
   // display the inserted record by iterator
   key = bson_iterator_key( &it ) ;
   value = bson_iterator_int( &it ) ;
   printf("The insert record is {%s:%d}\n", key, value ) ;
   bson_destroy ( &obj ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbInsert1_check_id)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   const char *key                = "" ;
   int value                      = 0 ;
   bson obj ;
   bson result ;
   bson_iterator it ;
   bson_init( &result ) ;
   const char *ret = NULL;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME ,
                        &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
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
   ASSERT_EQ( SDB_OK, rc ) ;
   // get id
   ret = bson_iterator_string( &it );
   ASSERT_TRUE( memcmp(ret, "", sizeof("")) ) ;
   rc = bson_iterator_next( &it ) ;
   ASSERT_EQ( BSON_INT, rc ) ;
   // display the inserted record by iterator
   key = bson_iterator_key( &it ) ;
   value = bson_iterator_int( &it ) ;
   printf("The insert record is {%s:%d}\n", key, value ) ;
   rc = sdbInsert2 ( collection, &obj, FLG_INSERT_CONTONDUP, &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc );
   printf("Result is: \n") ;
   bson_print( &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   bson_destroy ( &result ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbBulkInsert)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 10 ;
   int count                      = 0 ;
   SINT64 totalNum                = 0 ;
   bson attr ;
   bson obj ;
   bson *objList [ NUM ] ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // set read from master
   bson_init( &attr ) ;
   bson_append_string( &attr, "PreferedInstance", "m" ) ;
   bson_finish( &attr ) ;
   rc = sdbSetSessionAttr( connection, &attr ) ;
   bson_destroy( &attr ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // allocate memory and add data
   for ( count = 0; count < NUM; count++ )
   {
      objList[count] = bson_create() ;
      if ( (count % 5) == 0 )
      {
        // get a duplicate key record
        rc = bson_append_int ( objList[count], "_id", count-1 ) ;
        ASSERT_EQ( SDB_OK, rc ) ;
        rc = bson_append_int ( objList[count], "NUM", count ) ;
        ASSERT_EQ( SDB_OK, rc ) ;
        bson_finish ( objList[count] ) ;
        continue ;
      }
      rc = bson_append_int ( objList[count], "_id", count ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = bson_append_int ( objList[count], "NUM", count ) ;
      bson_finish ( objList[count] ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   // TODO:
   // bulk insert,if the argument "flags" is set FLG_INSERT_CONTONDUP,
   // datebase will not stop bulk insert while one failed with dup key
   rc = sdbBulkInsert ( collection, 0, objList, NUM ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_IXM_DUP_KEY, rc ) ;
   // check
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   CHECK_MSG("%lld%s\n", totalNum, " record(s) have been insert with flag 0.") ;
   ASSERT_EQ( 5, totalNum ) ;
   // delete all the records in collection
   rc = sdbDelete( collection, NULL, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // test another situation
   rc = sdbBulkInsert ( collection, FLG_INSERT_CONTONDUP, objList, NUM ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   CHECK_MSG("%lld%s\n", totalNum,
             " record(s) have been insert with flag FLG_INSERT_CONTONDUP.") ;
   ASSERT_EQ( 9, totalNum ) ;
   // free memory that allocated by bson_create()
   for ( count = 0; count < NUM; count++ )
   {
      bson_dispose ( objList[count] ) ;
   }

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}


TEST(collection,sdbBulkInsert_empty)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 10000 ;
   int count                      = 0 ;
   SINT64 totalNum                = 0 ;
   bson obj ;
   bson *objList [ NUM ] ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   sleep(3) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Bulk insert records." OSS_NEWLINE ) ;
   rc = sdbBulkInsert ( collection, 0, objList, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbDelete)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 totalNum                = 0 ;
   SINT64 NUM                     = 10 ;
   bson obj ;
   bson result ;
   bson_iterator it ;

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
   // insert record
   insertRecords( collection, NUM ) ;
   // get the record num
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("totalNum is: %lld\n", totalNum ) ;
   ASSERT_EQ( NUM, totalNum ) ;
   // delete all the record
   rc = sdbDelete ( collection, NULL, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the record num
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, totalNum ) ;
   // test sdbDelete1
   bson_init( &result ) ;
   rc = sdbDelete1 ( collection, NULL, NULL, 0, &result ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "DeletedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   rc = sdbDelete1 ( collection, NULL, NULL, -1, &result ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &result ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbUpdate)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson rule ;
   bson result ;
   bson obj ;
   bson_iterator it ;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Update the records with the rule :" OSS_NEWLINE ) ;
   // build insert record
   bson_init ( &obj ) ;
   bson_append_int ( &obj, "age", 1 ) ;
   bson_finish ( &obj ) ;
   // insert
   rc = sdbInsert ( collection, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // build up the rule
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 19 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_finish ( &rule ) ;
   bson_print( &rule ) ;
   // update
   bson_init( &result ) ;
   rc = sdbUpdate2( collection, &rule, NULL, NULL, 0, &result ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "UpdatedNum" ) ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it) ) ;    
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "ModifiedNum" ) ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it) ) ;    
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "InsertedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   bson_destroy( &result ) ;
   rc = sdbUpdate( collection, &rule, NULL, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbUpdate1( collection, &rule, NULL, NULL, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbUpsert_without_condition)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson rule ;
   bson obj ;
   bson setOnInsert ;
   bson condition ;
   bson result ;
   bson_iterator it ;

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

   printf( "Update the records with the rule :" OSS_NEWLINE ) ;
   printf( "Update the records with the rule :" OSS_NEWLINE ) ;
   // build insert record
   bson_init ( &obj ) ;
   bson_append_int ( &obj, "ID", 1 ) ;
   bson_finish ( &obj ) ;
   // insert
   rc = sdbInsert ( collection, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // build up the setOnInsert
   bson_init( &setOnInsert ) ;
   bson_append_string ( &setOnInsert, "name", "liang" ) ;
   bson_finish ( &setOnInsert ) ;
   // build up the condition
   bson_init( &condition ) ;
   bson_append_int ( &condition, "ID", 1000  ) ;
   bson_finish ( &condition ) ;
   bson_print(&condition);
   // build up the rule
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "ID", 999 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_finish ( &rule ) ;
   bson_print( &rule ) ;
   // update
   bson_init( &result ) ;
   rc = sdbUpsert3( collection, &rule, &condition, NULL, &setOnInsert, 0, &result ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "UpdatedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;   
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "ModifiedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;   
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "InsertedNum" ) ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it) ) ;
   bson_destroy( &setOnInsert ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &result ) ;
   rc = sdbUpsert( collection, &rule, NULL, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbUpsert1( collection, &rule, NULL, NULL, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbUpsert2( collection, &rule, NULL, NULL, NULL, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule );
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbUpsert_with_simle_condition)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson rule ;
   bson condition ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   printf( "Update the records with the rule :" OSS_NEWLINE ) ;
   // build up the rule
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "ID", 999 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_finish ( &rule ) ;
   bson_print( &rule ) ;
   printf( "Update the records with the condition is :" OSS_NEWLINE ) ;
   // build up the condition
   bson_init( &condition ) ;
   bson_append_int(&condition, "ID", 12345 ) ;
   bson_finish( &condition ) ;
   bson_print( &condition ) ;
   // Upsert
   rc = sdbUpsert( collection, &rule, &condition, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule );
   bson_destroy( &condition );

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbUpsert_with_complex_condition)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson rule ;
   bson condition ;
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

   printf( "Update the records with the rule :" OSS_NEWLINE ) ;
   // build up the rule
   bson_init( &rule ) ;
   bson_append_int( &rule, "age", 50 ) ;
   bson_finish( &rule ) ;

   printf( "Update the records with the condition is :" OSS_NEWLINE ) ;
   // build up the condition
   bson_init( &condition ) ;
   bson_append_start_object ( &condition, "$set" ) ;
   bson_append_start_array( &condition, "phone" ) ;
   bson_append_int(&condition, "0", 12345 ) ;
   bson_append_int( &condition, "1", 678910 ) ;
   bson_append_finish_array( &condition ) ;
   bson_append_start_object( &condition, "address" ) ;
   bson_append_string( &condition, "hometown", "guangzhou" ) ;
   bson_append_string( &condition, "workplace","beijing" ) ;
   bson_append_start_object( &condition, "other" ) ;
   bson_append_string( &condition, "now","shanghai" ) ;
   bson_append_string( &condition, "tomorrow","unkown" ) ;
   bson_append_finish_object( &condition ) ;
   bson_append_string( &condition, "play","guangzhou" ) ;
   bson_append_finish_object( &condition ) ;
   bson_append_finish_object ( &condition ) ;
   bson_finish ( &condition ) ;
   bson_print( &condition ) ;
   // Upsert
   // exchange the place of condition and rule
   rc = sdbUpsert( collection, &condition, &rule, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule );
   bson_destroy( &condition );

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbUpdate_with_regex1)
{
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs    = 0 ;
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   INT32 i = 0 ;
   bson obj ;
   bson rule ;
   bson cond ;
   CHAR buf[64] = { 0 } ;
   const CHAR* regex = "^31" ;
   const CHAR* options = "i" ;
   INT32 num = 10 ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu,"%d",i ) ;
      strcat( buff, "31" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 30 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }
   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu, "%d", i ) ;
      strcat( buff, "41" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 40 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // cond
   bson_init ( &cond ) ;
// *************************************************************
// 情况1
     bson_append_start_object( &cond, "name" ) ;
     bson_append_string ( &cond, "$regex", "^31" ) ;
     bson_append_string ( &cond, "$options", "i" ) ;
     bson_append_finish_object( &cond ) ;
// *************************************************************
   rc = bson_finish( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // rule
   bson_init ( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 999 ) ;
   bson_append_finish_object( &rule ) ;
   rc = bson_finish ( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // update with regex expression
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   bson_destroy( &cond ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build query condition
   bson_destroy( &rule ) ;
   bson_init( &rule ) ;
   bson_append_int ( &rule, "age", 999 ) ;
   rc = bson_finish( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query
   rc = sdbQuery( cl, &rule, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule ) ;
   bson_init( &rule ) ;
   while ( !( rc = sdbNext( cursor, &rule ) ) )
   {
      bson_destroy( &rule ) ;
      bson_init( &rule ) ;
      i++ ;
   }
   bson_destroy( &rule ) ;
   ASSERT_EQ( num, i ) ;
   // print the records
//   displayRecord( &cursor ) ;

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(collection, sdbUpdate_with_regex2)
{
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs    = 0 ;
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   INT32 i = 0 ;
   bson obj ;
   bson rule ;
   bson cond ;
   CHAR buf[64] = { 0 } ;
   const CHAR* regex = "^31" ;
   const CHAR* options = "i" ;
   INT32 num = 10 ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu,"%d",i ) ;
      strcat( buff, "31" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 30 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }
   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu, "%d", i ) ;
      strcat( buff, "41" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 40 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // cond
   bson_init ( &cond ) ;
// *************************************************************
// 情况2
   bson_append_regex( &cond, "name", regex, options ) ;
// *************************************************************
   rc = bson_finish( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // rule
   bson_init ( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 999 ) ;
   bson_append_finish_object( &rule ) ;
   rc = bson_finish ( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // update with regex expression
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build query condition
   bson_destroy( &rule ) ;
   bson_init( &rule ) ;
   bson_append_int ( &rule, "age", 999 ) ;
   rc = bson_finish( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query
   rc = sdbQuery( cl, &rule, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule ) ;
   bson_init( &rule ) ;
   while ( !( rc = sdbNext( cursor, &rule ) ) )
   {
      bson_destroy( &rule ) ;
      bson_init( &rule ) ;
      i++ ;
   }
   bson_destroy( &rule ) ;
   bson_destroy( &cond ) ;
   ASSERT_EQ( num, i ) ;
   // print the records
//   displayRecord( &cursor ) ;

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(collection,sdbQuery_all)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
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
   printf( "query all the record :" OSS_NEWLINE ) ;
   // query all the data
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   displayRecord( &cursor ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbQuery_with_fields)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   bson condition ;
   bson select ;
   bson orderBy ;
   bson hint ;

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
   // condition
   bson_init( &condition ) ;
   bson_append_int( &condition, "age", 50 ) ;
   bson_finish( &condition ) ;
   // select
   bson_init( &select ) ;
   bson_append_string( &select, "firstName", "" ) ;
   bson_append_int( &select,"age",0 ) ;
   bson_finish( &select ) ;
   // orderBy
   bson_init( &orderBy ) ;
   bson_append_int( &orderBy, "firstName", 1 ) ;
   bson_append_int( &orderBy, "age", -1 ) ;
   bson_finish( &orderBy ) ;
   // hint
   bson_init( &hint ) ;
   bson_append_int( &hint, "age", 0 ) ;
   bson_finish( &hint ) ;
   printf( "query the specifeed record :" OSS_NEWLINE ) ;
   // execute query
   rc = sdbQuery ( collection, &condition, &select, &orderBy, &hint, 0, -1, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   displayRecord( &cursor ) ;

   bson_destroy( &hint ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &select ) ;
   bson_destroy( &condition ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbQuery_with_flag1)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *pStr               = "{ \"\": \"John|50|999\" }" ;
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
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
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
   bson_destroy( &select ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   bson_init ( &obj ) ;
   i = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      i++ ;
      bson_sprint( buff, 1024, &obj ) ;
      ASSERT_EQ ( 0, strncmp( pStr, buff, sizeof( buff ) ) ) ;
      printf("obj is: %s\n", buff ) ;
      bson_destroy( &obj ) ;
   }
   ASSERT_EQ ( 10, i ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}


TEST(collection, sdbQuery_with_flag128)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs       = 0 ;
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
   const CHAR *indexName = "indexFlag128";

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
   bson_init( &index );
   bson_append_int( &index, "age", 1 );
   bson_finish( &index );
   rc = sdbCreateIndex( cl, &index, indexName, FALSE, FALSE ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert some records
   insertRecords( cl, num ) ;

   // condition
   bson_init( &condition ) ;
   bson_append_int( &condition, "age", 50 ) ;
   bson_finish( &condition ) ;

   // select
   bson_init( &select ) ;
   bson_append_string( &select, "firstName", "" ) ;
   bson_append_string( &select,"age", ""  ) ;
   bson_append_string( &select, "lastName", "" ) ;
   bson_finish( &select ) ;

   // orderBy
   bson_init( &orderBy ) ;
   bson_append_string( &select, "firstName", "" ) ;
   bson_append_int( &select,"age",0 ) ;
   bson_finish( &orderBy ) ;

   // hint
   bson_init( &hint ) ;
   bson_append_int( &hint, "age", 0 ) ;
   bson_finish( &hint ) ;

   printf( "query the specifeed record :" OSS_NEWLINE ) ;
   // execute query
   rc = sdbQuery1 ( cl, NULL, &select,
                   NULL, NULL, 0, -1, 128, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   displayRecord( &cursor ) ;

   bson_destroy ( &hint );
   bson_destroy ( &orderBy );
   bson_destroy ( &select ) ;
   bson_destroy ( &condition ) ;
   bson_destroy ( &index ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbQuery_with_flag128_nohint)
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
   // insert some records
   insertRecords( cl, num ) ;

   printf( "query the specifeed record :" OSS_NEWLINE ) ;
   // execute query
   rc = sdbQuery1 ( cl, NULL, NULL,
                    NULL, &hint, 0, -1, 128, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_RTN_INVALID_HINT, rc ) ;

   bson_destroy ( &hint );
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbQuery_with_flag256)
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
                    NULL, NULL, 0, -1, 256, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbQuery_with_some_flags)
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

TEST(collection, sdbGetCount_with_condition)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 totalNum                = 0 ;
   bson condition ;
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
   printf( "Count the record with the condition \n" ) ;
   // build up the condition
   bson_init( &condition ) ;
   bson_append_int ( &condition, "age", 50 ) ;
   bson_finish ( &condition ) ;
   bson_print( &condition ) ;
   // count records
   rc = sdbGetCount ( collection, &condition, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The number of matched records is %lld\n", totalNum ) ;
   bson_destroy( &condition ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection,sdbGetCount_without_condition)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 totalNum                = 0 ;
   SINT64 NUM                     = 10 ;
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
   // insert records
   insertRecords( collection, NUM ) ;
   printf( "Count the record without condition :\n" ) ;
   // count records
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "totalNum is: %lld\n", totalNum ) ;
   ASSERT_EQ( NUM, totalNum ) ;
   printf( "The number of matched records is %lld\n", totalNum ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( collection, aggregate )
{
   INT32 rc                          = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
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
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert record
   for( i=0; i<rNUM; i++ )
   {
      bson_init( &obj ) ;
      flag = jsonToBson( &obj, record[i] ) ;
      bson_print( &obj ) ;
      ASSERT_EQ( true, flag ) ;
      rc = sdbInsert( collection, &obj ) ;
      printf("rc= %d\n", rc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // build bson array
   for( i=0; i<iNUM; i++ )
   {
      ob[i] = bson_create() ;
      flag = jsonToBson ( ob[i], command[i] ) ;
      bson_print( ob[i] ) ;
      ASSERT_EQ( true, flag ) ;
   }
   // aggregate
   rc = sdbAggregate( collection, ob, iNUM, &cursor ) ;
   CHECK_MSG("%s%d","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
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

TEST( collection, sdbGetQueryMeta )
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle dataCursor     = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 100 ;

   bson obj ;
   bson condition ;
   bson hint ;
   bson dataBlock ;
   bson indexBlock ;
   bson_iterator it ;
   // initialize
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
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   sleep( 3 ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // gen some record
   rc = genRecord ( &connection, COLLECTION_FULL_NAME, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build condition
   bson_init ( &condition ) ;
   bson_append_start_object ( &condition, "age" ) ;
   bson_append_int ( &condition, "$gt", 0 ) ;
   bson_append_long ( &condition, "$lt", NUM ) ;
   bson_append_finish_object ( &condition ) ;
   bson_finish ( &condition ) ;
   bson_print( &condition ) ;

   // build hint
   bson_init ( &hint ) ;
   bson_append_string ( &hint, "Collection", COLLECTION_FULL_NAME) ;
   bson_finish ( &hint ) ;
   bson_print( &hint ) ;

   // test
   rc = sdbGetQueryMeta( cl, &condition, NULL, NULL,
                         0, -1, &cursor ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &hint ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;

   // get indexBlock from return cursor
   bson_init( &dataBlock ) ;
   rc = sdbCurrent ( cursor, &dataBlock ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   bson_init ( &indexBlock ) ;
   bson_iterator_init ( &it, &dataBlock ) ;
   while ( BSON_EOO != bson_iterator_next( &it ) )
   {
      if ( strcmp( "Datablocks", bson_iterator_key( &it ) ) )
      {
         bson_append_element ( &indexBlock, NULL, &it ) ;
      }
   }
   bson_finish ( &indexBlock ) ;
   bson_destroy( &dataBlock ) ;
   // use dataBlock to query
   rc = sdbQuery ( cl, NULL, NULL, NULL, &indexBlock, 0, -1, &dataCursor ) ;
   long count = getRecordNum( dataCursor ) ;
   bson_destroy( &indexBlock ) ;
   printf ( "count is : %ld\n", count ) ;
   ASSERT_EQ ( NUM, count ) ;

   // drop the cs
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   // realse
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( dataCursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collection, sdbIsClose)
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
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // TO DO:
   result = sdbIsValid( connection );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   std::cout << "before close connection, result is " << result << std::endl ;
   ASSERT_EQ ( TRUE, result ) ;
   sdbDisconnect ( connection ) ;
   sdbDisconnect ( connection1 ) ;
   result = sdbIsValid( connection );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   std::cout << "after close connection, result is " << result << std::endl ;
   ASSERT_EQ ( FALSE, result ) ;
   
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseConnection ( connection ) ;
   sdbReleaseConnection ( connection1 ) ;
}



// the follow test need to dasign more robust
/*
TEST(collection,sdbSplitCollection)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build shardingkey and replsize info:{ShardingKey:{age:1}, ReplSize:1}
   bson_init ( &obj ) ;
   bson_append_start_object ( &obj, "ShardingKey" ) ;
   bson_append_int ( &obj, "age", 1 ) ;
   bson_append_finish_object ( &obj ) ;
   bson_append_int ( &obj, "ReplSize", 1 ) ;
   bson_finish ( &obj ) ;
   //bson_print ( &obj ) ;
   // create cl
   rc = sdbCreateCollection1 ( cs, COLLECTION_NAME1, &obj, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // build split condition
   bson_init ( &obj ) ;
   bson_append_int ( &obj, "age", 50 ) ;
   bson_finish ( &obj ) ;
   //bson_print ( &obj ) ;
   // collection split
   rc = sdbSplitCollection ( collection, SOURCEGROUP, TARGETGROUP,
                             &obj, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( connection ) ;
}
*/

/*
the follow APIs it waiting to add at this file :
sdbGetDataBlocks()
sdbAlterCollection()
sdbCreateCollection1()
sdbSplitCLAsync()
sdbSplitCollectionByPercent()
sdbSplitCLByPercentAsync()
sdbAttachCollection()
sdbDetachCollection()

*/
/*******************************************************************************
*@Description : query one testcase.[db.foo.bar.findOne()]
*@Modify List :
*               2014-10-24   xiaojun Hu   Init
*******************************************************************************/
TEST( collection, sdbCQueryOne )
{
   sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   sdbCSHandle cs = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   const CHAR *csName = "sdb_client_collection_test" ;
   const CHAR *clName = "sdb_query_limit_one" ;
   INT32 rc = SDB_OK ;
   INT32 recordNum = 200 ;

   rc = sdbConnect( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollectionSpace( db, csName ) ;
   rc = sdbCreateCollectionSpace( db, csName, 0, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "ReplSize", 0 ) ;
   bson_finish( &obj ) ;
   rc = sdbCreateCollection1( cs, clName, &obj, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
   for( INT32 i = 0 ; i < recordNum ; ++i )
   {
      bson_init( &obj ) ;
      bson_append_int( &obj, "reacordNumber", i ) ;
      bson_append_string( &obj, "description", "testcase for query limit one" ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }
   /****************************
   * specify all [TestPoint_1]
   ****************************/
   bson_init( &obj ) ;
   //bson_append_string( &obj, "description", "testcase for query limit one" ) ;
   bson_append_int( &obj, "reacordNumber", 11 ) ;
   bson_finish( &obj ) ;
   rc = sdbQuery( cl, &obj, NULL, NULL, NULL, NULL, 1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
   bson_init( &obj ) ;
   INT32 cnt = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      cnt++ ;
   }
   // inspect query one
   bson_destroy( &obj ) ;
   ASSERT_EQ( 1, cnt ) ;
   printf( "success specify one query\n" ) ;
   /****************************
   * specify all [TestPoint_2]
   ****************************/
   bson_init( &obj ) ;
   bson_append_string( &obj, "description", "testcase for query limit one" ) ;
   bson_finish( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQuery( cl, &obj, NULL, NULL, NULL, NULL, 1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
   bson_init( &obj ) ;
   cnt = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      cnt++ ;
   }
   // inspect query one
   bson_destroy( &obj ) ;
   ASSERT_EQ( 1, cnt ) ;
   printf( "success specify all query \n" ) ;
   /****************************
   * specify none [TestPoint_3]
   ****************************/
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, NULL, 1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj ) ;
   cnt = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      cnt++ ;
   }
   // inspect query one
   bson_destroy( &obj ) ;
   ASSERT_EQ( 1, cnt ) ;
   printf( "success specify none query \n" ) ;
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}


TEST( collection, sdbQueryAndUpdate )
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 100 ;
   INT32 i                        = 0 ;
   INT32 set_value                = 100 ;
   const CHAR *pIndexName1        = "test_index1" ;
   const CHAR *pIndexName2        = "test_index2" ;
   const CHAR *pField1            = "testQueryAndUpdate1" ;
   const CHAR *pField2            = "testQueryAndUpdate2" ;


   bson index ;
   bson tmp ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson update ;

   // initialize
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
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // create index
   bson_init( &index ) ;
   bson_append_int( &index, pField1, -1 ) ;
   bson_finish( &index ) ;
   rc = sdbCreateIndex( cl, &index, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &index ) ;

   bson_init( &index ) ;
   bson_append_int( &index, pField2, 1 ) ;
   bson_finish( &index ) ;
   rc = sdbCreateIndex( cl, &index, pIndexName2, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &index ) ;

   // gen some record
   for ( i = 0; i < NUM; i++ )
   {
      bson obj ;
      bson_init( &obj ) ;
      bson_append_int( &obj, pField1, i ) ;
      bson_append_int( &obj, pField2, i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   /// in case: update and return the new record
   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 0 ) ;
   bson_append_int( &tmp, "$lt", 10 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;

   bson_init( &update ) ;
   bson_append_start_object( &update, "$set" ) ;
   bson_append_int( &update, pField2, set_value ) ;
   bson_append_finish_object( &update ) ;
   bson_finish( &update ) ;
   // test
   rc = sdbQueryAndUpdate( cl, &condition, NULL, NULL, NULL, &update,
                           0, -1, 0, TRUE, &cursor ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   bson_destroy( &tmp ) ;
   bson_destroy( &condition ) ;
   // check
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      while ( bson_iterator_more( &it ) )
      {
         const CHAR *pKey = bson_iterator_key( &it ) ;
         if ( 0 == strncmp( pKey, pField2, strlen(pField2) ) )
         {
            INT32 value = bson_iterator_int( &it ) ;
            ASSERT_EQ( set_value, value ) ;
         }
         bson_iterator_next( &it ) ;
      }
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;

   /// in case: update and return the original one
   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 10 ) ;
   bson_append_int( &tmp, "$lt", 20 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;

   // test
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, NULL, NULL, NULL, &update,
                           0, -1, 0, FALSE, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &tmp ) ;
   bson_destroy( &condition ) ;
   // check
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      while ( bson_iterator_more( &it ) )
      {
         const CHAR *pKey = bson_iterator_key( &it ) ;
         if ( 0 == strncmp( pKey, pField2, strlen(pField2) ) )
         {
            INT32 value = bson_iterator_int( &it ) ;
            ASSERT_TRUE( 10 <= value && value < 20 ) ;
         }
         bson_iterator_next( &it ) ;
      }
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;
   /// in case: use selector orderBy without hint
   bson_init( &selector ) ;
   bson_append_string( &selector, pField2, "" ) ;
   bson_finish( &selector ) ;

   bson_init( &orderBy ) ;
   bson_append_int( &orderBy, pField2, 1 ) ;
   bson_finish( &orderBy ) ;

   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 20 ) ;
   bson_append_int( &tmp, "$lt", 30 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;

   bson_init( &hint ) ;
   bson_append_null( &hint, "" ) ;
   bson_finish( &hint ) ;

   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, &selector, &orderBy, &hint,
                           &update, 0, -1, 0, TRUE, &cursor ) ;
   ASSERT_EQ( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, rc ) ;

   /// in case: use selector orderBy with hint
   bson_destroy( &hint ) ;
   bson_init( &hint ) ;
   bson_append_string( &hint, "", pIndexName2 ) ;
   bson_finish( &hint ) ;

   // test
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, &selector, &orderBy, &hint,
                           &update, 0, -1, 0, TRUE, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &tmp ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   // check
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      const CHAR *pKey = bson_iterator_key( &it ) ;
      ASSERT_EQ( 0, strncmp( pKey, pField2, strlen(pField2) ) ) ;
      INT32 value =  bson_iterator_int( &it ) ;
      ASSERT_EQ( set_value, value ) ;
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;

   /// in case: use limit and skip in single group
   // TODO: limit and skip
   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 30 ) ;
   bson_append_int( &tmp, "$lt", 40 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;
   bson_destroy( &tmp ) ;
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, NULL, NULL, NULL,
                           &update, 10, -1, 0, TRUE, &cursor ) ;
   bson_destroy( &condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   /// in case: use limit and skip in different groups, need to split
   //ASSERT_EQ( SDB_RTN_QUERYMODIFY_MULTI_NODES, rc ) ;

   /// in case: use flag
   // FLG_QUERY_FORCE_HINT
   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 40 ) ;
   bson_append_int( &tmp, "$lt", 50 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;

   bson_init( &hint ) ;
   bson_append_string( &hint, "", pIndexName2 ) ;
   bson_finish( &hint ) ;
   bson_destroy( &tmp ) ;

   // watch what index sdbQueryAndUpdate going to use
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "Run", 1 ) ;
   bson_finish( &option ) ;
   sdbReleaseCursor ( cursor ) ;
   rc = sdbExplain( cl, &condition, NULL, NULL, &hint,
                    0x00000080, 0, -1, &option, &cursor ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   printf("The explain of query with FLG_QUERY_FORCE_HINT is: \n") ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext(cursor, &tmp ) ) )
   {
      bson_print( &tmp ) ;
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;

   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, NULL, NULL, &hint,
                           &update, 0, -1, 0x00000080, TRUE, &cursor ) ;
   bson_destroy( &condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &hint ) ;
   // check
   printf("Test using flag: FLG_QUERY_FORCE_HINT\n") ;
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      bson_print( &tmp ) ;
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;

   // FLG_QUERY_PARALLED
   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 50 ) ;
   bson_append_int( &tmp, "$lt", 60 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;
   bson_destroy( &tmp ) ;
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, NULL, NULL, NULL,
                           &update, 0, -1, 0x00000100, TRUE, &cursor ) ;
   bson_destroy( &condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   printf("Test using flag: FLG_QUERY_PARALLED\n") ;
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      bson_print( &tmp ) ;
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;


   // FLG_QUERY_WITH_RETURNDATA
   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 60 ) ;
   bson_append_int( &tmp, "$lt", 70 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;
   bson_destroy( &tmp ) ;
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndUpdate( cl, &condition, NULL, NULL, NULL,
                           &update, 0, -1, 0x00000200, TRUE, &cursor ) ;
   bson_destroy( &condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   printf("Test using flag: FLG_QUERY_WITH_RETURNDATA\n") ;
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      bson_print( &tmp ) ;
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;
   bson_destroy( &update ) ;
   // realse
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( collection, sdbQueryAndRemove )
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 100 ;
   SINT64 count                   = 0 ;
   INT32 i                        = 0 ;
   INT32 set_value                = 100 ;
   const CHAR *pIndexName1        = "test_index1" ;
   const CHAR *pIndexName2        = "test_index2" ;
   const CHAR *pField1            = "testQueryAndUpdate1" ;
   const CHAR *pField2            = "testQueryAndUpdate2" ;


   bson index ;
   bson tmp ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;

   // initialize
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
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // create index
   bson_init( &index ) ;
   bson_append_int( &index, pField1, -1 ) ;
   bson_finish( &index ) ;
   rc = sdbCreateIndex( cl, &index, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &index ) ;

   bson_init( &index ) ;
   bson_append_int( &index, pField2, 1 ) ;
   bson_finish( &index ) ;
   rc = sdbCreateIndex( cl, &index, pIndexName2, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &index ) ;

   // gen some record
   for ( i = 0; i < NUM; i++ )
   {
      bson obj ;
      bson_init( &obj ) ;
      bson_append_int( &obj, pField1, i ) ;
      bson_append_int( &obj, pField2, i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   /// in case: use extend sort
   bson_init( &selector ) ;
   bson_append_string( &selector, pField2, "" ) ;
   bson_finish( &selector ) ;

   bson_init( &orderBy ) ;
   bson_append_int( &orderBy, pField2, 1 ) ;
   bson_finish( &orderBy ) ;

   bson_init( &tmp ) ;
   bson_append_int( &tmp, "$gte", 0 ) ;
   bson_finish( &tmp ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &tmp ) ;
   bson_finish( &condition ) ;

   bson_init( &hint ) ;
   bson_append_string( &hint, "", pIndexName1 ) ;
   bson_finish( &hint ) ;

   rc = sdbQueryAndRemove( cl, &condition, &selector, &orderBy, &hint,
                           0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, rc ) ;
   bson_destroy( &hint ) ;

   /// in case: does not use extend sort
   bson_init( &hint ) ;
   bson_append_string( &hint, "", pIndexName2 ) ;
   bson_finish( &hint ) ;

   // test
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQueryAndRemove( cl, &condition, &selector, &orderBy, &hint,
                           50, 10, 0x00000080, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &tmp ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   // check
   i = 0 ;
   bson_init( &tmp ) ;
   while ( SDB_OK == ( rc = sdbNext( cursor, &tmp ) ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &tmp ) ;
      const CHAR *pKey = bson_iterator_key( &it ) ;
      ASSERT_EQ( 0, strncmp( pKey, pField2, strlen(pField2) ) ) ;
      INT32 value =  bson_iterator_int( &it ) ;
      ASSERT_EQ( 50 + i, value ) ;
      bson_destroy( &tmp ) ;
      bson_init( &tmp ) ;
      i++ ;
   }
   bson_destroy( &tmp ) ;
   ASSERT_EQ( 10, i ) ;
   i = 100 ;
   while ( i-- )
   {
      rc = sdbGetCount( cl, NULL, &count ) ;
      ASSERT_EQ( rc, SDB_OK ) ;
      if ( 0 == count )
         break ;
   }
   if ( 0 == i )
      ASSERT_EQ( 0, count ) ;

   // realse
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( collection, alter_collection )
{
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs         = 0 ;
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor = 0 ;

   INT32 rc                = SDB_OK ;
   const CHAR *pCSName     = "test_alter_cs_in_c" ;
   const CHAR *pCLName     = "test_alter_cl_in_c" ;
   const CHAR *pCLFullName = "test_alter_cs_in_c.test_alter_cl_in_c" ;
   const CHAR *pValue      = NULL ;
   INT32 n_value = 0 ;
   bson_iterator it ;
   bson_iterator it2 ;
   bson option ;
   bson matcher ;
   bson record ;
   bson subObj ;

   bson_init( &subObj ) ;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   if ( FALSE == isCluster( db ) )
   {
      return ;
   }

   // drop cs
   rc = sdbDropCollectionSpace( db, pCSName ) ;
   if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
   {
      ASSERT_EQ( 0, 1 ) << "failed to drop cs " << pCSName ;
   }

   // create cs
   rc = sdbCreateCollectionSpace( db, pCSName, 4096, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // create cl
   rc = sdbCreateCollection( cs, pCLName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &option ) ;
   bson_append_int( &option, "ReplSize", 0 ) ;
   bson_append_start_object( &option, "ShardingKey" ) ;
   bson_append_int( &option, "a", 1 ) ;
   bson_append_finish_object( &option ) ;
   bson_append_string( &option, "ShardingType", "hash" ) ;
   bson_append_int( &option, "Partition", 1024 ) ;
   bson_finish( &option) ;

   rc = sdbAlterCollection( cl, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", pCLFullName ) ;
   bson_finish( &matcher ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_CATALOG, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &record ) ;
   rc = sdbNext( cursor, &record ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check Name
   if ( BSON_STRING != bson_find( &it, &record, "Name" ) )
   {
      ASSERT_EQ( 0, 1 ) << "after alter cl, the snapshot record is not the one we want" ;
   }
   pValue = bson_iterator_string( &it ) ;
   ASSERT_EQ( 0, strcmp( pValue, pCLFullName ) ) << "after alter cl, the cl's name is not what we want" ;

   // check ReplSize
   if ( BSON_INT != bson_find( &it, &record, "ReplSize" ) )
   {
      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding type is not the one we want" ;
   }
   n_value = bson_iterator_int( &it ) ;
   ASSERT_EQ( 7, n_value ) ;

   // check ShardingType
   if ( BSON_STRING != bson_find( &it, &record, "ShardingType" ) )
   {
      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding type is not the noe we want" ;
   }
   pValue = bson_iterator_string( &it ) ;

   // check partition
   if ( BSON_INT != bson_find( &it, &record, "Partition" ) )
   {
      ASSERT_EQ( 0, 1 ) << "after alter cl, the partition is not the one we want" ;
   }
   n_value = bson_iterator_int( &it ) ;
   ASSERT_EQ( 1024, n_value ) ;

   // check ShardingKey
   if ( BSON_OBJECT != bson_find( &it, &record, "ShardingKey" ) )
   {
      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding key is not the one we want" ;
   }
   bson_iterator_subobject( &it, &subObj ) ;
   if ( BSON_INT != bson_find( &it2, &subObj, "a" ) )
   {
      ASSERT_EQ( 0, 1 ) << "after alter cl, the sharding key is not the one we want" ;
   }
   n_value = bson_iterator_int( &it2 ) ;
   ASSERT_EQ( 1, n_value ) ;

   rc = sdbDropCollectionSpace( db, pCSName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &option ) ;
   bson_destroy( &matcher ) ;
   bson_destroy( &record ) ;
   bson_destroy( &subObj ) ;


   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
}

TEST( collection, bson_timestamp_over_millis )
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const char *key                = NULL ;
   int value                      = 0 ;
   int sec = 1534942305 ;
   const CHAR *expect = "{ \"ts1\": { \"$timestamp\": \"2018-08-22-20.51.45.000000\" }, \"ts2\": { \"$timestamp\": \"2018-08-22-20.51.44.999999\" }, \"ts3\": { \"$timestamp\": \"2018-08-22-20.51.45.000001\" }, \"ts4\": { \"$timestamp\": \"2018-08-22-20.53.48.456789\" }, \"ts5\": { \"$timestamp\": \"2018-08-22-20.49.41.543211\" }, \"ts6\": { \"$timestamp\": \"1921-05-12-19.10.18.456789\" }, \"ts7\": { \"$timestamp\": \"1921-05-12-19.06.11.543211\" } }" ;
   bson obj ;
   bson_iterator it ;
   bson_init( &obj ) ;
   bson_append_timestamp2( &obj, "ts1", sec, 0 ) ;
   bson_append_timestamp2( &obj, "ts2", sec, -1 ) ;
   bson_append_timestamp2( &obj, "ts3", sec, 1 ) ;
   bson_append_timestamp2( &obj, "ts4", sec, 123456789 ) ;
   bson_append_timestamp2( &obj, "ts5", sec, -123456789 ) ;
   bson_append_timestamp2( &obj, "ts6", -sec, 123456789 ) ;
   bson_append_timestamp2( &obj, "ts7", -sec, -123456789 ) ;
   bson_finish( &obj ) ;
   bson selector ;
   bson_init( &selector ) ;
   bson_append_start_object( &selector, "_id" ) ;
   bson_append_int( &selector, "$include", 0 ) ;
   bson_append_finish_object( &selector ) ;
   bson_finish( &selector ) ;
   bson sel ;
   bson_init( &sel ) ;
   json2bson2( "{ \"_id\": { \"$include\": 0 } }", &sel ) ;
   bson_destroy( &sel ) ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME ,
                        &collection ) ;

   rc = sdbInsert( collection, &obj ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   bson_destroy( &obj ) ;
   // execute query
   rc = sdbQuery1 ( collection, NULL, &selector,
                   NULL, NULL, 0, -1, 0, &cursor ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   bson_destroy( &selector ) ;
   bson_init ( &obj ) ;
   char buff[500] = { 0 } ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bsonToJson( buff, 500, &obj, false, false ) ;
      //printf( "buff is: %s\n", buff ) ;
      ASSERT_EQ( 0, strncmp( expect, buff, strlen( expect ) ) ) ;
      bson_destroy( &obj ) ;
   }

   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST( collection, create_and_remove_id_index )
{
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs         = 0 ;
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor = 0 ;

   INT32 rc               = SDB_OK ;
   const CHAR *pIndexName = "$id" ;
   INT32 count            = 0 ;
   bson obj ;
   bson record ;
   bson updater ;
   bson option ;
   bson_init( &option ) ;

   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get cs
   rc = getCollectionSpace ( db, COLLECTION_SPACE_NAME, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get cl
   rc = getCollection ( db, COLLECTION_FULL_NAME, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "a", 1 ) ;
   bson_finish( &obj ) ;

   bson_init( &record ) ;
   bson_append_start_object( &record, "$set" ) ;
   bson_append_int( &record, "a", 2 ) ;
   bson_append_finish_object( &record ) ;
   bson_finish( &record ) ;

   bson_init( &updater ) ;
   bson_append_start_object( &updater, "$set" ) ;
   bson_append_int( &updater, "a", 10 ) ;
   bson_append_finish_object( &updater ) ;
   rc = bson_finish( &updater ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // test
   rc = sdbDropIdIndex( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check
   rc = sdbGetIndexes( cl, pIndexName, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 0, count ) << "after drop id index, &id index still exist" ;

   sdbReleaseCursor ( cursor ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   count = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 1, count ) ;

   rc = sdbUpdate( cl, &updater, NULL, NULL ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;

   rc = sdbUpsert( cl, &updater, NULL, NULL ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;

   rc = sdbDelete( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;

   // test
   bson_append_bool( &option, "Offline", 0 ) ;
   bson_finish( &option ) ;
   rc = sdbCreateIdIndex( cl, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &option ) ;

   rc = sdbDelete( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   count = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 0, count ) ;

   rc = sdbUpsert( cl, &record, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbReleaseCursor ( cursor ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   count = 0 ;
   bson_destroy( &obj ) ;
   bson_init( &obj ) ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
       bson_destroy( &obj ) ;
       bson_init( &obj ) ;
      count++ ;
   }
   bson_destroy( &obj ) ;
   ASSERT_EQ( 1, count ) ;

   rc = sdbUpdate( cl, &updater, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "a", 10 ) ;
   bson_finish( &obj ) ;
   sdbReleaseCursor ( cursor ) ;
   rc = sdbQuery( cl, &obj, NULL, NULL, NULL, 0, -1, &cursor ) ;
   count = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &record ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 1, count ) ;

   bson_destroy( &record ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &updater ) ;

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
}

void _bulkInsert2Test( sdbCollectionHandle cHandle )
{
   INT32 rc = 0 ;
   INT32 i = 0 ;
   const INT32 num = 10 ;
   bson_iterator it ;
   bson returnObj ;
   bson* obj[num] ;
   bson* obj2[num] ;
   bson* obj3[num] ;
   printf( "in _bulkInsertTest2\n" ) ;

   bson_init( &returnObj ) ;

   // create bson poiter array
   for ( i = 0; i < num; i++ )
   {
      obj[i] = bson_create();
      obj2[i] = bson_create();
      obj3[i] = bson_create();
      rc = bson_append_int( obj[i], "num", i ) ;
      rc = bson_append_int( obj2[i], "num", i + 100 ) ;
      rc = bson_append_int( obj3[i], "_id", i + 1000 ) ;
      rc = bson_append_string( obj[i], "op", "bulkInsert2" ) ;
      rc = bson_append_string( obj2[i], "op", "bulkInsert2" ) ;
      rc = bson_append_string( obj3[i], "op", "bulkInsert2" ) ;
      if ( rc != 0 )
         printf ( "something wrong.\n" ) ;
      rc = bson_finish ( obj[i] ) ;
      rc = bson_finish ( obj2[i] ) ;
      rc = bson_finish ( obj3[i] ) ;
      if ( rc != 0 )
         printf ( "something wrong.\n" ) ;
   }
   // bulk insert
   rc = sdbBulkInsert2 ( cHandle, 0, obj, num, &returnObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_EOO, bson_find( &it, &returnObj, "_id" ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "InsertedNum" ) ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "DuplicatedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   rc = sdbBulkInsert2 ( cHandle, FLG_INSERT_RETURN_OID, obj2, num, &returnObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_ARRAY, bson_find( &it, &returnObj, "_id" ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "InsertedNum" ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "DuplicatedNum" ) ) ;
   printf( "after bulk insert2, return object is: \n" ) ;
   bson_print( &returnObj ) ;
//   for ( i = 0; i < num; ++i )
//   {
//      bson_printf( obj2[i] ) ;
//   }
   bson_destroy( &returnObj ) ;
   bson_init( &returnObj ) ;
   rc = sdbBulkInsert2 ( cHandle, FLG_INSERT_RETURN_OID, obj, num, &returnObj ) ;
   ASSERT_EQ( SDB_IXM_DUP_KEY, rc ) ;
   rc = sdbBulkInsert2 ( cHandle, FLG_INSERT_CONTONDUP | FLG_INSERT_RETURN_OID, obj, num, &returnObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_ARRAY, bson_find( &it, &returnObj, "_id" ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "InsertedNum" ) ) ; 
   ASSERT_EQ( 0, bson_iterator_long( &it) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &returnObj, "DuplicatedNum" ) ) ; 
   ASSERT_EQ( 10, bson_iterator_long( &it) ) ;
   printf( "after bulk insert2 with the duplicate key, return object is: \n" ) ;
   bson_print( &returnObj ) ;

   rc = sdbBulkInsert2 ( cHandle, FLG_INSERT_RETURN_OID, obj3, num, &returnObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_ARRAY, bson_find( &it, &returnObj, "_id" ) ) ;
   printf( "after bulk insert2 with the record which has _id, return object is: \n" ) ;
   bson_print( &returnObj ) ;

   // free memory
   for ( i = 0; i < num; i++ )
   {
      printf ( "after insert2, record is: \n" ) ;
      bson_print( obj[i] ) ;
      bson_print( obj2[i] ) ;
      bson_print( obj3[i] ) ;
      bson_dispose ( obj[i] ) ;
      bson_dispose ( obj2[i] ) ;
      bson_dispose ( obj3[i] ) ;
   }
   bson_destroy( &returnObj ) ;
   printf( "\n" ) ;
}

void _bulkInsertTest( sdbCollectionHandle cHandle )
{
   INT32 rc = 0 ;
   INT32 i = 0 ;
   const INT32 num = 10 ;
   bson* obj[num] ;
   printf( "in _bulkInsertTest\n" ) ;

   // create bson poiter array
   for ( i = 0; i < num; i++ )
   {
      obj[i] = bson_create();
      rc = bson_append_int( obj[i], "num", i ) ;
      rc = bson_append_string( obj[i], "op", "bulkInsert" ) ;
      if ( rc != 0 )
         printf ( "something wrong.\n" ) ;
      rc = bson_finish ( obj[i] ) ;
      if ( rc != 0 )
         printf ( "something wrong.\n" ) ;
   }
   // bulk insert
   rc = sdbBulkInsert ( cHandle, 0, obj, num ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbBulkInsert ( cHandle, FLG_INSERT_CONTONDUP, obj, num ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbBulkInsert ( cHandle, FLG_INSERT_CONTONDUP | FLG_INSERT_RETURN_OID, obj, num ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // free memory
   for ( i = 0; i < num; i++ )
   {
      printf ( "after insert, record is: \n" ) ;
      bson_print( obj[i] ) ;
      bson_dispose ( obj[i] ) ;
   }
   printf( "\n" ) ;
}

void _insert2Test( sdbCollectionHandle cHandle )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;
   bson resultObj ;
   bson_iterator it ;
   bson *obj5 = bson_create() ;
   bson *resultObj3 = bson_create() ;

   printf( "in _insert2Test\n" ) ;

   bson_init( &resultObj ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "insert2", 1 ) ;
   bson_finish( &obj ) ;

   bson_init( &obj2 ) ;
   bson_append_int( &obj2, "insert2", 2 ) ;
   bson_finish( &obj2 ) ;

   bson_init( &obj3 ) ;
   bson_append_string( &obj3, "_id", "aaaaaa" ) ;
   bson_append_int( &obj3, "insert2", 3 ) ;
   bson_finish( &obj3 ) ;

   bson_init( &obj4 ) ;
   bson_append_string( &obj4, "_id", "bbbbbb" ) ;
   bson_append_int( &obj4, "insert2", 4 ) ;
   bson_finish( &obj4 ) ;

   bson_append_int( obj5, "insert2", 5 ) ;
   bson_finish( obj5 ) ;

   // case 1:
   rc = sdbInsert2( cHandle, &obj, 0, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbInsert2( cHandle, &obj, FLG_INSERT_RETURN_OID | FLG_INSERT_CONTONDUP, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_OID, bson_find( &it, &resultObj, "_id" ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &resultObj, "InsertedNum" ) ) ;  
   ASSERT_EQ( BSON_LONG, bson_find( &it, &resultObj, "DuplicatedNum" ) ) ; 
   printf( "after double insert, return obj is: \n" ) ;
   bson_print( &resultObj ) ;

   // case 2:
   bson_destroy( &resultObj ) ;
   bson_init( &resultObj ) ;
   bson_finish( &resultObj ) ;
   rc = sdbInsert2( cHandle, &obj2, 0, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_EOO, bson_find( &it, &resultObj, "_id" ) ) ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &resultObj, "InsertedNum" ) ) ;  
   ASSERT_EQ( BSON_LONG, bson_find( &it, &resultObj, "DuplicatedNum" ) ) ;
   rc = sdbInsert2( cHandle, &obj2, FLG_INSERT_RETURN_OID, &resultObj ) ;
   ASSERT_EQ( SDB_IXM_DUP_KEY, rc ) ;
   ASSERT_TRUE( bson_is_empty( &resultObj ) ) ;   
   bson_destroy( &resultObj ) ;
   bson_init( &resultObj ) ;
   rc = sdbInsert2( cHandle, &obj2, FLG_INSERT_CONTONDUP, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_EOO, bson_find( &it, &resultObj, "_id" ) ) ;
   rc = sdbInsert2( cHandle, &obj2, FLG_INSERT_CONTONDUP | FLG_INSERT_RETURN_OID, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_OID, bson_find( &it, &resultObj, "_id" ) ) ;

   // case 3:
   bson_destroy( &resultObj ) ;
   bson_init( &resultObj ) ;
//   bson_finish( &resultObj ) ;
   rc = sdbInsert2( cHandle, &obj3, 0, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_EOO, bson_find( &it, &resultObj, "_id" ) ) ;
   rc = sdbInsert2( cHandle, &obj4, FLG_INSERT_RETURN_OID, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_STRING, bson_find( &it, &resultObj, "_id" ) ) ;
   printf( "insert3, bson return oid is: \n" ) ;
   bson_print( &resultObj ) ;
   bson_destroy( &resultObj ) ;
   bson_init( &resultObj ) ;
   rc = sdbInsert2( cHandle, &obj4, FLG_INSERT_CONTONDUP, &resultObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_EOO, bson_find( &it, &resultObj, "_id" ) ) ;

   // case 5:
   rc = sdbInsert2( cHandle, obj5, FLG_INSERT_CONTONDUP, resultObj3 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "insert3, bson return oid is: \n" ) ;
   bson_print( resultObj3 ) ;

   bson_destroy( &obj ) ;
   bson_destroy( &obj2 ) ;
   bson_destroy( &obj3 ) ;
   bson_destroy( &obj4 ) ;
   bson_destroy( &resultObj ) ;
   bson_dispose( obj5 ) ;
   bson_dispose( resultObj3 ) ;

   printf( "\n" ) ;
}

void _insert1Test( sdbCollectionHandle cHandle )
{
   INT32 rc = SDB_OK ;
   bson_iterator it ;
   bson_iterator it2 ;
   bson obj ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;

   printf( "in _insert1Test\n" ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "insert1", 1 ) ;
   bson_finish( &obj ) ;

   bson_init( &obj2 ) ;
   bson_append_string( &obj2, "_id", "aaa" ) ;
   bson_append_int( &obj2, "insert1", 2 ) ;
   bson_finish( &obj2 ) ;

   rc = sdbInsert1( cHandle, &obj, &it ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_OID, bson_iterator_type(&it) ) ;
   rc = sdbInsert1( cHandle, &obj2, &it2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BSON_STRING, bson_iterator_type(&it2) ) ;

   printf( "API(insert1): \n" ) ;
   bson_print( &obj ) ;
   bson_print( &obj2 ) ;

   bson_init( &obj3 ) ;
   bson_append_element( &obj3, NULL, &it ) ;
   bson_finish( &obj3 ) ;
   bson_init( &obj4 ) ;
   bson_append_element( &obj4, NULL, &it2 ) ;
   bson_finish( &obj4 ) ;

   printf( "return _id is: \n" ) ;
   bson_print( &obj3 ) ;
   bson_print( &obj4 ) ;

   bson_destroy( &obj ) ;
   bson_destroy( &obj2 ) ;
   bson_destroy( &obj3 ) ;
   bson_destroy( &obj4 ) ;
   printf( "\n" ) ;
}

void _insertTest( sdbCollectionHandle cHandle )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson obj2 ;

   printf( "in _insertTest\n" ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "insert", 1 ) ;
   bson_finish( &obj ) ;

   bson_init( &obj2 ) ;
   bson_append_int( &obj2, "_id", 1 ) ;
   bson_append_int( &obj2, "insert", 2 ) ;
   bson_finish( &obj2 ) ;

   rc = sdbInsert( cHandle, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbInsert( cHandle, &obj2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   printf( "API(insert): \n" ) ;
   bson_print( &obj ) ;
   bson_print( &obj2 ) ;
   printf( "\n" ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &obj2 ) ;

}
TEST(debug, insertTest)
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   const char *key                = "" ;
   int value                      = 0 ;
   bson obj ;
   bson result ;
   bson_iterator it ;
   const char *ret = NULL;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME ,
                        &collection ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   _insertTest( collection ) ;
   _insert1Test( collection ) ;
   _insert2Test( collection ) ;
   _bulkInsertTest( collection ) ;
   _bulkInsert2Test( collection ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(debug, sdbGetIndexTest)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *pIndexName         = "aIndex" ;
   bson indexDef ;
   bson_init( &indexDef ) ;
   bson_append_int( &indexDef, "a", 1 ) ;
   bson_finish( &indexDef ) ;

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

   rc = sdbCreateIndex( collection, &indexDef, pIndexName, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // case 1:
   rc = sdbGetIndexes( collection, "$id", &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 1: \n" ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   // case 2:
   rc = sdbGetIndexes( collection, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 2: \n" ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   // case 3:
   bson_destroy( &indexDef ) ;
   bson_init( &indexDef ) ;
   rc = sdbGetIndex( collection, pIndexName, &indexDef ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 3: \n" ) ;
   bson_print( &indexDef ) ;
   bson_destroy( &indexDef ) ;

   // case 4:
   rc = sdbGetIndexInfo( collection, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 4: \n" ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   // case 5:
   bson_init( &indexDef ) ;
   rc = sdbGetIndex( collection, "", &indexDef ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetIndex( collection, NULL, &indexDef ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetIndex( collection, pIndexName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetIndex( collection, "aaaaaIndex", &indexDef ) ;
   ASSERT_EQ( SDB_IXM_NOTEXIST, rc ) ;
   bson_destroy( &indexDef ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

