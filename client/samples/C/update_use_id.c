/******************************************************************************
 *
 * Name: update_use_id.c
 * Description: This program demostrates how to use the "_id" to update record.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh update_use_id
 *    Win: buildApp.bat update_use_id
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc update_use_id.c common.c -o update_use_id -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Foupdate_use_id.obj /c update_use_id.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:update_use_id.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib update_use_id.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc update_use_id.c common.c -o update_use_id.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Foupdate_use_idstatic.obj /c update_use_id.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:update_use_idstaic.exe /LIBPATH:..\..\lib\c\debug\static staticsdbcd.lib update_use_idstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./update_use_id <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: update_use_id.exe <hostname> <servicename> <Username> <Username>
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more
 *       detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

#define NUM_RECORD 5

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define COLLECTION_FULL_NAME  "foo.bar"

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
   sdbCursorHandle cursor1           = 0 ;
   // define local variables
   // initialize them before use
   INT32 rc    = SDB_OK ;
   bson obj ;
   bson rule ;
   bson record ;
   bson updatecondition ;
   bson tmp ;
   bson_iterator it ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }
   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

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
      printf("Failed to create collection space, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection in a specified colletion space.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to create collection, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   
   // prepare record
   bson_init( &record ) ;
   bson_append_int( &record, "age", 10 ) ;
   rc = bson_finish( &record ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

   // insert record into database
   rc = sdbInsert( collection, &record ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to insert, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   bson_destroy( &record ) ;
   // query all the record in this collection
   rc = sdbQuery(collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to query, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // get the record from cursor
   bson_init(&obj) ;
   bson_init(&tmp) ;
   rc=sdbNext( cursor, &obj ) ;
   if ( rc!= SDB_OK )
   {
      printf("Failed to get next, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   rc = bson_copy( &tmp, &obj ) ;
   CHECK_RC ( rc, "Failed to copy bson" ) ;
   printf("Before update, the record is:\n") ;
   bson_print( &tmp ) ;
   bson_destroy( &tmp ) ;
   // set the update condition using "_id"
   bson_find( &it, &obj, "_id" ) ;
   bson_init( &updatecondition ) ;
   bson_append_element( &updatecondition, NULL, &it ) ;
   rc = bson_finish( &updatecondition ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;
   // set the update rule
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 99 ) ;
   bson_append_finish_object ( &rule ) ;
   rc = bson_finish ( &rule ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

   // update
   rc = sdbUpdate(collection, &rule, &updatecondition, NULL ) ;
   if ( rc!=SDB_OK )
   {
      printf("Failed to update, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   bson_destroy(&obj) ;
   bson_destroy(&rule) ;
   bson_destroy(&updatecondition) ;
   // query all the record in this collection again
   rc = sdbQuery(collection, NULL, NULL, NULL, NULL, 0, -1, &cursor1 ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to query, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // get record from cursor1
   bson_init(&obj) ;
   rc=sdbNext( cursor1, &obj ) ;
   if ( rc!= SDB_OK )
   {
      printf("Failed to get next, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   printf("after update, the record is:\n") ;
   bson_print( &obj ) ;
   bson_destroy(&obj) ;

done:
   // drop collection space
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   if ( rc != SDB_OK )
   {
     printf("Failed to drop collection space, rc = %d" OSS_NEWLINE, rc ) ;
   }
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   return 0;
error:
   goto done ;
}
