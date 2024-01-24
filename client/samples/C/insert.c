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
   bson result ;
   bson_init( &obj ) ;
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
   if( SDB_OK != rc )
   {
      printf("Failed to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   if( SDB_OK != rc )
   {
      printf( "Failed to create collection space, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection in a specified colletion space.
   // Here,we build it up in the new collection.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   if( SDB_OK != rc )
   {
      printf( "Failed to create collection, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // first, build up a bson obj for inserting
   bson_append_string( &obj, "name", "tom" ) ;
   bson_append_int( &obj, "age", 24 ) ;
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      printf( "Failed to create the inserting bson, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   printf( "The inserted record: " ) ;
   bson_print( &obj ) ;

   // then, insert to the specified collection
   rc = sdbInsert2( collection, &obj, FLG_INSERT_RETURN_OID, &result ) ;
   if ( SDB_OK != rc )
   {
      printf("Failed to insert, rc = %d", rc ) ;
      goto error ;
   }

   printf("Insert result: ") ;
   bson_print( &result ) ;

   // drop the specified collection
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   if( SDB_OK != rc )
   {
      printf( "Failed to drop the specified collection, "
              "rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // drop the specified collection space
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   if( SDB_OK != rc )
   {
      printf( "Failed to drop the specified collection space, "
              "rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

done:
   bson_destroy( &obj ) ;
   bson_destroy( &result ) ;
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


