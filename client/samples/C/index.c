/******************************************************************************
 *
 * Name: index.c
 * Description: This program demostrates how to connect to create index.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh index
 *    Win: buildApp.bat index
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc index.c common.c -o index -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Foindex.obj /c index.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:index.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib index.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc index.c common.c -o index.static -I../../include -O0
 *              -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Foindexstatic.obj /c index.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:indexstatic.exe /LIBPATH:..\..\lib\c\debug\dll staticsdbcd.lib indexstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./index <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: index.exe <hostname> <servicename> <Username> <Username>
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more
 *       detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define INDEX_NAME            "index"

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
      exit ( 1 ) ;
   }

   // connect to database
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   CHECK_RC ( rc, "Failed to connet to database" ) ;

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   CHECK_RC ( rc, "Failed to create collection space" ) ;

   // create collection in a specified colletion space.
   // Here,we build it up in the new collection.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   CHECK_RC ( rc, "Failed to create collection" ) ;

   // build a bson for index definition
   bson_init( &obj ) ;
   rc = bson_append_int( &obj, "name", 1 ) ;
   CHECK_RC ( rc, "Failed to append data" ) ;
   rc = bson_append_int( &obj, "age", -1 ) ;
   CHECK_RC ( rc, "Failed to append data" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;
   printf("The index to build is: ") ;
   bson_print ( &obj ) ;

   // create index
   rc = sdbCreateIndex ( collection, &obj, INDEX_NAME, FALSE, FALSE ) ;
   CHECK_RC ( rc, "Failed to create index" ) ;

   bson_destroy ( &obj ) ;
   printf("Suceess to build index!" OSS_NEWLINE ) ;

done:
   // drop collection space
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   if ( rc != SDB_OK )
   {
      printf ( "Failed to drop collection space, rc = %d"OSS_NEWLINE, rc ) ;
   }
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   return rc ;
error:
   goto done ;
}

