/******************************************************************************
 *
 * Name: snap.c
 * Description: This program demostrates how to connect to SequoiaDB database,
 *              and get database snapshot ( for other types of snapshots/lists,
 *              the steps are very similar )
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 * Auto Compile:
 *    Linux: ./buildApp.sh snap
 *    Win: buildApp.bat snap
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc snap.c common.c -o snap -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Fosnap.obj /c snap.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:snap.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib snap.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc snap.c common.c -o snap.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Fosnapstatic.obj /c snap.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:snapstaic.exe /LIBPATH:..\..\lib\c\debug\static staticsdbcd.lib snapstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./snap <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: snap.exe <hostname> <servicename> <Username> <Username>
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

INT32 main ( INT32 argc, CHAR **argv )
{
   /* initialize local variables */
   INT32 count                       = 0 ;
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUser                       = NULL ;
   CHAR *pPasswd                     = NULL ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   bson obj ;
   INT32 rc                          = 0 ;
   /* verify syntax */
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }

   /* read argument */
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUser        = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   /* connect to database */
   rc = connectTo ( pHostName, pServiceName, pUser, pPasswd, &connection ) ;
   if ( rc )
   {
      printf ( "Failed to connect to database at %s:%s, rc = %d"
               OSS_NEWLINE,
               pHostName, pServiceName, rc ) ;
      exit ( 0 ) ;
   }

   rc = sdbGetSnapshot ( connection, SDB_SNAP_DATABASE, NULL, NULL, NULL,
                         &cursor ) ;
   if ( rc )
   {
      printf ( "Failed to get snapshot, rc = %d\n", rc ) ;
      exit ( 0 ) ;
   }
   while ( TRUE )
   {
      bson_init ( &obj ) ;
      rc = sdbNext ( cursor, &obj ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            printf ( "Failed to fetch next record from collection, rc = %d"
                     OSS_NEWLINE, rc ) ;
         }
         break ;
      }
      printf ( "Record Read [ %d ]: " OSS_NEWLINE, count ) ;
      bson_print ( &obj ) ;
      bson_destroy ( &obj ) ;
      ++ count ;
   }

   /* disconnect from server */
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
   return 0 ;
}
