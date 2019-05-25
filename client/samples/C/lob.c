/******************************************************************************
 *
 * Name: lob.c
 * Description: This program demostrates how to large object in database.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh lob
 *    Win: buildApp.bat lob
 * Manual Compile:
 *    Linux: cc lob.c common.c -o lob -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Foquery.obj /c lob.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:lob.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib lob.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc lob.c common.c -o lob.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Foquerystatic.obj /c lob.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:lobstaic.exe /LIBPATH:..\..\lib\c\debug\static staticsdbcd.lib lobstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./lob <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: lob.exe <hostname> <servicename> <Username> <Username>
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more
 *       detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

#define BUFSIZE 1000

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define COLLECTION_FULL_NAME  "foo.bar"



INT32 main ( INT32 argc, CHAR **argv )
{
   // initialize local variables
   CHAR *pHostName             = NULL ;
   CHAR *pServiceName          = NULL ;
   CHAR *pUsr                  = NULL ;
   CHAR *pPasswd               = NULL ;
   // define handles
   sdbConnectionHandle db      = SDB_INVALID_HANDLE ;
   sdbCSHandle cs              = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl      = SDB_INVALID_HANDLE ;
   sdbLobHandle lob            = SDB_INVALID_HANDLE ;

   INT32 rc                    = SDB_OK ;
   CHAR buf[BUFSIZE]           = { 'a' } ;
   bson_oid_t oid ;

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
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &db ) ;
   CHECK_RC ( rc, "Failed to connet to database" ) ;

   // create collection space
   rc = sdbCreateCollectionSpace ( db, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &cs ) ;
   CHECK_RC ( rc, "Failed to create collection space" ) ;

   // create collection
   rc = sdbCreateCollection ( cs, COLLECTION_NAME, &cl ) ;
   CHECK_RC ( rc, "Failed to create collection" ) ;

   // create a large object and than write something to it
   // we need to gen an oid for the creating large object
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   CHECK_RC ( rc, "Failed to create large object" ) ;

   // write something to the newly created large object
   rc = sdbWriteLob( lob, buf, BUFSIZE ) ;
   CHECK_RC ( rc, "Failed to write to large object" ) ;
   
   // close the newly created large object
   rc = sdbCloseLob( &lob ) ;
   CHECK_RC ( rc, "Failed to close large object" ) ;

   printf( "Success to create a large object and write something to it!\n" ) ;
	  
   // drop the collection
   rc = sdbDropCollection( cs,COLLECTION_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection" ) ;

   // drop the collection space
   rc = sdbDropCollectionSpace( db,COLLECTION_SPACE_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection space" ) ;

done:
   // disconnect the connection
   sdbDisconnect ( db ) ;
   // release the handles
//   sdbReleaseLob ( lob ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
   return 0;
error:
   goto done ;
}

