/******************************************************************************
 *
 * Name: bsonUsage.c
 * Description: This program demostrates how to build c bson.
 * Parameters:
 *
 * Auto Compile:
 *    Linux: ./buildApp.sh bsonUsage
 *    Win: buildApp.bat bsonUsage
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: cc bsonUsage.c common.c -o bsonUsage -I../../include -L../../lib -lsdbc
 *    Win:
 *       cl /FobsonUsage.obj /c bsonUsage.c /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *       link /OUT:bsonUsage.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib bsonUsage.obj common.obj
 *       copy ..\..\lib\c\debug\dll\sdbcd.dll .
 *    Static Linking:
 *    Linux: cc bsonUsage.c common.c -o bsonUsage.static -I../../include -O0
 *           -ggdb ../../lib/libstaticsdbc.a -lm -ldl -lpthread
 *    Win:
 *       cl /FobsonUsagestatic.obj /c bsonUsage.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       cl /Focommonstatic.obj /c common.c /I..\..\include /wd4047 /DSDB_STATIC_BUILD
 *       link /OUT:bsonUsagestaic.exe /LIBPATH:..\..\c\debug\static\lib staticsdbcd.lib bsonUsagestatic.obj commonstatic.obj
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./bsonUsage
 *    Win: bsonUsage.exe
 * Note: While the appended data invalid, C BSON API will return error code,
 *       we need to handle this kind of error. Please see bson.h for more
 *       detail.
 ******************************************************************************/
#include <stdio.h>
#include "common.h"


// { "field_int": 100, "field_doule": 3.14, "field_str": "hello world", "field_bool": true, "field_null": null }
void genMiscFieldsRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_int( &obj, "field_int", 100 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_double( &obj, "field_doule", 3.14 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string ( &obj, "field_str", "hello world" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_bool( &obj, "field_bool", 1 ) ; // means true
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_null( &obj, "field_null" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the misc fields record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { "field_oid": { "$oid": "56ab1bf749cd933667000000" } }
void genOIDRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_oid_t oid;
   bson_oid_gen( &oid );
   bson_init( &obj ) ;
   rc = bson_append_oid( &obj, "field_oid", &oid );
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the oid field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

//{ "field_date": { "$date": "2015-01-01" }, "field_timestamp": { "$timestamp": "2015-01-01-00.00.00.000000" } }
void genDateAndTimestampRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_date_t datet = 1420041600000 ; // 2015-01-01
   bson_timestamp_t timestamp = { 0, 1420041600 } ; // 2015-01-01-00.00.00.000000

   bson_init( &obj ) ;
   rc = bson_append_date( &obj, "field_date", datet ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_timestamp( &obj, "field_timestamp", &timestamp ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the date and timestamp fields record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { "field_binary": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "0" } } 
void genBinaryRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   // gen binary data
   CHAR *pStr = "aGVsbG8gd29ybGQ=" ; // the base64 code of "hello world"
   int len = getDeBase64Size ( pStr ) ;
   char *out = (char *)malloc( len ) ;
   memset( out, 0, len ) ;
   base64Decode( pStr, out, len ) ;
   printf( "\"%s\" is the base64 code of \"%s\"\n", pStr, out ) ;

   // build bson
   bson_init( &obj ) ;
   rc = bson_append_binary( &obj, "field_binary", 0, out, strlen(out) ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the binary field record is: \n" ) ;
   // note: we put "hello world" into obj in binary format,
   // but bson_print display it as base64 code format
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   free( out ) ;
   return ;
error:
   goto done ;
}

// { "field_regex": { "$regex": "^abc", "$options": "i" } }
void genRegexRecord()
{
   INT32 rc = SDB_OK ;
   const CHAR *pPatten = "^abc" ;
   const CHAR *pOptions = "i" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_regex( &obj, "field_regex", pPatten, pOptions ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the regex field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { info:{ name:"Tom", age: 27, phone:["13800138000", 02066666666]}}
void genObjectRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_start_object( &obj, "info" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string( &obj, "name", "Tom" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int( &obj, "age", 27 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_start_array( &obj, "phone" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string( &obj, "0", "13800138000" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string( &obj, "1", "02066666666" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_finish_array( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_finish_object( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the object field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { arr: [ 0, 1, 2 ] }
void genArrayRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init ( &obj ) ;
   rc = bson_append_start_array ( &obj, "arr" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int ( &obj, "0", 0 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int ( &obj, "1", 1 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int ( &obj, "2", 2 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_finish_array ( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish ( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the array field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

INT32 main ( INT32 argc, CHAR **argv )
{
   genMiscFieldsRecord() ;
   genOIDRecord() ;
   genArrayRecord() ;
   genObjectRecord() ;
   genDateAndTimestampRecord() ;
   genBinaryRecord() ;
   genRegexRecord() ;

   return 0;
}
