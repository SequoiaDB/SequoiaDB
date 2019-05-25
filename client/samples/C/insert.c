/******************************************************************************
 *
 * Name: insert.c
 * Description: This program demostrates how to connect to insert
 *              data into database.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh insert
 *    Win: buildApp.bat insert
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc insert.c common.c -o insert -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Foinsert.obj /c insert.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:insert.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib insert.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc insert.c common.c -o insert.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Foinsertstatic.obj /c insert.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:insertstatic.exe /LIBPATH:..\..\lib\c\debug\static staticsdbcd.lib insertstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./insert <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: insert.exe <hostname> <servicename> <Username> <Username>
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more
 *       detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"


INT32 main ( INT32 argc, CHAR **argv )
{
   // initialize local variables
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUsr                        = NULL ;
   CHAR *pPasswd                     = NULL ;
   // define a connetion handle; use to connect to database
   sdbConnectionHandle connection    = 0 ;
   // define a collection space handle
   sdbCSHandle collectionspace       = 0 ;
   // define a collection handle
   sdbCollectionHandle collection    = 0 ;
   // define a cursor handle for query
   sdbCursorHandle cursor            = 0 ;

   // define local variables
   // initialize them before use
   bson obj ;
   bson_decimal decimal ;
   INT32 rc = SDB_OK ;

   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }

   // connect to database
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   if( rc!=SDB_OK )
   {
      printf( "Failed to create collection space, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection in a specified colletion space.
   // Here,we build it up in the new collection.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   if( rc!=SDB_OK )
   {
      printf( "Failed to create collection, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // first,build up a bson obj for inserting
   bson_init( &obj ) ;
   bson_append_string( &obj, "name", "tom" ) ;
   bson_append_int( &obj, "age", 24 ) ;
   decimal_init( &decimal ) ;
   rc = decimal_from_str( "1.234", &decimal ) ;
   CHECK_RC ( rc, "Failed to get decimal from str" ) ;
   rc = bson_append_decimal( &obj, "score", &decimal ) ;
   CHECK_RC ( rc, "Failed to append decimal" ) ;
   decimal_free( &decimal ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

   printf( "The inserted record is :" ) ;
   bson_print( &obj ) ;

   // then,insert to the specified collection
   rc = sdbInsert ( collection, &obj ) ;
   if ( rc )
   {
      printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   printf( "Success to insert! " OSS_NEWLINE ) ;

   // drop the specified collection
   rc = sdbDropCollection( collectionspace,COLLECTION_NAME ) ;
   if( rc!=SDB_OK )
   {
      printf( "Failed to drop the specified collection, "
              "rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // drop the specified collection space
   rc = sdbDropCollectionSpace( connection,COLLECTION_SPACE_NAME ) ;
   if( rc!=SDB_OK )
   {
      printf( "Failed to drop the specified collection, "
              "rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

done:
   bson_destroy( &obj ) ;
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   return 0;
error:
   goto done ;
}


