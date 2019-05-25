/******************************************************************************
 *
 * Name: sampledb.c
 * Description: This program demostrates how to create a sample collectionspace
 *              in a given database.
 *              This program will also populate some testing data and create
 *              indexes
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh sampledb
 *    Win: buildApp.bat sampledb
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc sampledb.c common.c -o sampledb -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /Fosampledb.obj /c sampledb.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:sampledb.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib sampledb.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc sampledb.c common.c -o sampledb.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /Fosampledbstatic.obj /c sampledb.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:sampledbstaic.exe /LIBPATH:..\..\lib\c\debug\static staticsdbcd.lib sampledbstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./sampledb <hostname> <servicename> \
 *           <Username> <Username>
 *    Win: sampledb.exe <hostname> <servicename> <Username> <Username>
 *
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

#define SAMPLE_DATA_FILE_NAME "sampledb.dat"
#define MAX_BUF_SZ 16384

INT32 loadFromFile ( const CHAR *pFileName,
                     sdbConnectionHandle connection )
{
   CHAR *p = NULL ;
   FILE *pFile = NULL ;
   CHAR buffer [ MAX_BUF_SZ + 1 ] ;
   CHAR *pCollectionName = NULL ;
   bson obj ;
   bson_init ( &obj ) ;
   pFile = fopen ( pFileName, "r" ) ;
   if ( !pFile )
   {
      printf ( "Not able to open file %s\n", pFileName ) ;
      return SDB_IO ;
   }
   while ( TRUE )
   {
      memset ( buffer, 0, sizeof(buffer) ) ;
      p = fgets ( buffer, MAX_BUF_SZ, pFile ) ;
      if ( !p )
      {
         break ;
      }
      if ( isComment ( p ) )
         continue ;
      if ( NULL != ( pCollectionName = loadTag ( p ) ) )
      {
         printf ( "Tag: %s\n", pCollectionName ) ;
      }
      else if ( loadJSON ( p, &obj ) )
      {
         printf ( "JSON: %s\n", p ) ;
         bson_destroy ( &obj ) ;
         bson_init ( &obj ) ;
      }
   }
   bson_destroy ( &obj ) ;
   fclose( pFile ) ;
   return SDB_OK ;
}

/* main function */
INT32 main ( INT32 argc, CHAR **argv )
{
   /* initialize local variables */
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUser                       = NULL ;
   CHAR *pPasswd                     = NULL ;
   sdbConnectionHandle connection    = 0;
   INT32 rc                          = 0 ;
   /* verify syntax */
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
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
      printf ( "Failed to connect to database at %s:%s, rc = %d\n",
               pHostName, pServiceName, rc ) ;
      goto error ;
   }

   rc = loadFromFile ( SAMPLE_DATA_FILE_NAME, connection ) ;
   if ( rc )
   {
      printf ( "Failed to load from file %s, rc = %d\n",
               SAMPLE_DATA_FILE_NAME, rc ) ;
      goto error ;
   }

   sdbDisconnect( connection ) ;

done:
   /* dispose connection */
   sdbReleaseConnection ( connection ) ;
   return rc ;
error:
   goto done ;
}
