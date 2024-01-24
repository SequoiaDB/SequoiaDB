/******************************************************************************
 *
 * Name: update.c
 * Description: This program demostrates how to update data.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh update
 *    Win: buildApp.bat update
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc update.c common.c -o update -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Foupdate.obj /c update.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:update.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib update.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc update.c common.c -o update.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Foupdatestatic.obj /c update.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:updatestaic.exe /LIBPATH:..\..\lib\c\debug\static staticsdbcd.lib updatestatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./update <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: update.exe <hostname> <servicename> <Username> <Username>
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
   INT32 rc                          = SDB_OK ;
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

   // define local variables
   // initialize them before use
   bson obj ;
   bson modifier ;
   bson result ;
   bson_init( &obj ) ;
   bson_init( &modifier ) ;
   bson_init( &result ) ;

   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // connect to database
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   CHECK_RC ( rc, "Failed to connet to database" ) ;

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   CHECK_RC ( rc, "Failed to create collection space" ) ;

   // create collection in a specified colletion space.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   CHECK_RC ( rc, "Failed to create collection" ) ;

   // insert records to the collection
   bson_append_string( &obj, "name", "tom" ) ;
   bson_append_int( &obj, "age", 24 ) ;
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      printf( "Failed to create the inserting bson, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   rc = sdbInsert ( collection, &obj ) ;
   CHECK_RC ( rc, "Failed to insert record" ) ;

   // update the record
   bson_append_start_object ( &modifier, "$set" ) ;
   bson_append_int ( &modifier, "age", 19 ) ;
   bson_append_finish_object ( &modifier ) ;
   rc = bson_finish( &modifier ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      printf( "Failed to create the modifier bson, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   printf ( "The update rule is: " ) ;
   bson_print( &modifier ) ;

   // update
   rc = sdbUpdate2( collection, &modifier, NULL, NULL, 0, &result ) ;
   CHECK_RC ( rc, "Failed to update" ) ;

   printf("Update result: ") ;
   bson_print( &result ) ;

   // drop the specified collection
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection" ) ;

   // drop the specified collection space
   rc = sdbDropCollectionSpace ( connection, COLLECTION_SPACE_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection space" ) ;

done:
   bson_destroy( &obj ) ;
   bson_destroy ( &modifier ) ;
   bson_destroy ( &result ) ;
   // disconnect the connection
   if ( connection )
   {
      sdbDisconnect( connection ) ;
   }
   // release the local variables
   if ( collection )
   {
      sdbReleaseCollection( collection ) ;
   }
   if ( collectionspace )
   {
      sdbReleaseCS( collectionspace ) ;
   }
   if ( connection )
   {
      sdbReleaseConnection( connection ) ;
   }
   return 0;
error:
   goto done ;
}

