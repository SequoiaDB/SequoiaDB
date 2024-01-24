/******************************************************************************
 *
 * Name: subArrayLen.c
 * Description: This program demostrates how to get the length of the sub array
 *              int a bson.
 * Auto Compile:
 *    Linux: ./buildApp.sh subArrayLen
 *    Win: buildApp.bat subArrayLen
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc subArrayLen.c common.c -o subArrayLen -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /FosubArrayLen.obj /c subArrayLen.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:subArrayLen.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib subArrayLen.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc subArrayLen.c common.c -o subArrayLen.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /FosubArrayLenstatic.obj /c subArrayLen.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:subArrayLenstaic.exe /LIBPATH:..\..\lib\c\debug\dll staticsdbcd.lib subArrayLenstatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./subArrayLen
 *    Win: subArrayLen.exe
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more
 *       detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"

#define FIELD1 "field1"
#define FIELD2 "field2"

INT32 buildOneRecord ( bson* obj )
{
   INT32 rc = SDB_OK ;
   bson_init ( obj ) ;
   bson_append_int ( obj, FIELD1, 100 ) ;
   bson_append_start_array ( obj, FIELD2 ) ;
   bson_append_string ( obj, "0", "a" ) ;
   bson_append_string ( obj, "1", "b" ) ;
   bson_append_string ( obj, "2", "c" ) ;
   bson_append_finish_array ( obj ) ;
   rc = bson_finish ( obj ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 rc        = SDB_OK ;
   const char *key = NULL ;
   int count       = 0 ;
   bson obj ;
   bson_iterator it ;
   bson_iterator itt ;

   bson_init ( &obj ) ;
   // build one record
   rc = buildOneRecord ( &obj ) ;
   if ( rc )
   {
      printf ( "Failed to build record.\n" ) ;
      goto error ;
   }
   // print record
   bson_print ( &obj ) ;
   // get the array field
   bson_iterator_init ( &it, &obj ) ;
   while ( BSON_EOO != bson_iterator_next( &it ) )
   {
      key = bson_iterator_key ( &it ) ;
      if ( !strcmp( FIELD2, key ) )
      {
         break ;
      }
   }
   // use sub iterator to run through array
   bson_iterator_subiterator( &it, &itt ) ;
   while ( BSON_EOO != bson_iterator_next( &itt ) )
   {
      count++ ;
   }
   printf ("The length of sub array is %d\n", count ) ;
done :
   bson_destroy ( &obj ) ;
   return 0 ;
error :
   goto done ;
}

