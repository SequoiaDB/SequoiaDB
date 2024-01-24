#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"
#include "base64c.h"

#define USERDEF       "sequoiadb"
#define PASSWDDEF     "sequoiadb"

using namespace std;

TEST(cbson, empty)
{
   INT32 rc = SDB_OK ;
   ASSERT_EQ( rc, SDB_OK ) ;
}

TEST(cbson, test)
{
   bson obj ;
   const char *pStr = "{\"_id\":{\"$oid\":\"0123456789abcdef01234567\"}}" ;
   bson_init( &obj ) ;
   BOOLEAN flag = jsonToBson( &obj, pStr ) ;
   if ( TRUE == flag )
   {
      printf( "Success, bson is: \n" ) ;
      bson_print( &obj ) ;
   }
   else
   {
      printf( "Failed\n" ) ;
   }
   bson_destroy( &obj ) ;
}


TEST(cbson, binary)
{
   INT32 rc = SDB_OK ;
   bson obj ;
   const char *str = "{ \"key\": { \"$binary\" : \"aGVsbG8gd29ybGQ=\", \"$type\": \"1\" } }" ;
   bson_init( &obj ) ;
   BOOLEAN flag = jsonToBson2( &obj, str, FALSE, FALSE ) ;
   ASSERT_EQ( flag, TRUE ) ;
   bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
}


TEST(cbson, esc_problem)
{
   sdbConnectionHandle connection  = 0 ;
   sdbConnectionHandle connection1 = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj1 ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;
   bson obj5 ;
   bson obj6 ;
   bson obj7 ;
   bson obj8 ;
   bson obj9 ;
   bson temp ;
   bson record ;
   BOOLEAN flag = FALSE ;
   const CHAR *str1 = "{\"1\":\"1\\\"1\"}";
   const CHAR *str2 = "{\"2\":\"2\\\\2\"}";
   const CHAR *str3 = "{\"3\":\"3\\/3\"}";
   const CHAR *str4 = "{\"4\":\"123\\b4\"}";
   const CHAR *str5 = "{\"5\":\"5\\f5\"}";
   const CHAR *str6 = "{\"6\":\"6\\n6\"}";
   const CHAR *str7 = "{\"7\":\"7\\r7\"}";
   const CHAR *str8 = "{\"8\":\"8\\t8\"}";
   const CHAR *str9 = "{\"a\":\"a\\u0041a\"}";
   const int num = 10;
   const int bufSize = 100;
   CHAR* result[num];
   int i = 0;
   for ( ; i < num; i++ )
   {
      result[i] = (CHAR *)malloc(bufSize);
      if ( result[i] == NULL)
         printf( "Failed to malloc.\n" );
   }
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME, &cl ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // TO DO:
   bson_init( &obj1 ) ;
   bson_init( &obj2 ) ;
   bson_init( &obj3 ) ;
   bson_init( &obj4 ) ;
   bson_init( &obj5 ) ;
   bson_init( &obj6 ) ;
   bson_init( &obj7 ) ;
   bson_init( &obj8 ) ;
   bson_init( &obj9 ) ;
   flag = jsonToBson( &obj1, str1 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj2, str2 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj3, str3 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj4, str4 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj5, str5 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj6, str6 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj7, str7 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj8, str8 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj9, str9 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   // insert
   rc = sdbInsert( cl, &obj1 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj2 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj3 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj4 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj5 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj6 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj7 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj8 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj9 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   bson_init ( &temp );
   i = 0;
   while ( !( rc = sdbNext( cursor, &temp ) ) )
   {
      rc = bsonToJson( result[i], bufSize, &temp, FALSE, FALSE );
//      bson_print( &temp );
      ASSERT_EQ ( TRUE, rc );
      printf( "bson is: %s\n", result[i] );
      i++;
      bson_destroy( &temp );
      bson_init( &temp );
   }
   bson_destroy( &temp ) ;
   bson_destroy( &obj1 ) ;
   bson_destroy( &obj2 ) ;
   bson_destroy( &obj3 ) ;
   bson_destroy( &obj4 ) ;
   bson_destroy( &obj5 ) ;
   bson_destroy( &obj6 ) ;
   bson_destroy( &obj7 ) ;
   bson_destroy( &obj8 ) ;
   bson_destroy( &obj9 ) ;

   for ( i = 0; i < num; i++ )
   {
      free( result[i] );
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;

}


/*
TEST(cbson, regex)
{
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs    = 0 ;
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   bson rule ;
   bson cond ;
   CHAR buf[64] = { 0 } ;
   const CHAR* regex = "^31" ;
   const CHAR* options = "i" ;
   INT32 num = 10 ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu,"%d",i ) ;
      strcat( buff, "31" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 30 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu, "%d", i ) ;
      strcat( buff, "41" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 40 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // cond
   bson_init ( &cond ) ;

//   bson_append_regex( &cond, "name", regex, options ) ;

////
//   bson_append_start_object( &cond, "name" ) ;
//   bson_append_string ( &cond, "$regex", "^31" ) ;
//   bson_append_string ( &cond, "$options", "i" ) ;
//   bson_append_finish_object( &cond ) ;
////
   rc = bson_finish( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // rule
   bson_init ( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 999 ) ;
   bson_append_finish_object( &rule ) ;
   rc = bson_finish ( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // update with regex expression
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the records
   displayRecord( &cursor ) ;

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
}
*/

TEST( cbson, base64Encode )
{
   INT32 rc = SDB_OK ;
   const CHAR *str = "hello world" ;
   const CHAR *str2 = "aGVsbG8gd29ybGQ=" ;
   INT32 strLen = strlen( str) ;
   // encode
   INT32 len = getEnBase64Size( strLen ) ;
   CHAR *out = (CHAR *)malloc( len ) ;
   memset( out, 0, len ) ;
   base64Encode( str, strLen, out, len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout << "out is: " << out << std::endl ;
   ASSERT_EQ( 0, strcmp( str2, out) ) ;
   free ( out ) ;
}

TEST( cbson, base64Decode )
{
   INT32 rc = SDB_OK ;
   const CHAR *str = "aGVsbG8gd29ybGQ=" ;
   const CHAR *str2 = "hello world" ;
   INT32 len = getDeBase64Size( str ) ;
   CHAR *out = (CHAR *)malloc( len ) ;
   memset( out, 0, len ) ;
   base64Decode( str, out, len ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   std::cout << "out is: " << out << std::endl ;
   ASSERT_EQ( 0, strcmp(str2, out ) ) ;
   free ( out ) ;
}

TEST( cbson, bsonArray )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_long( &obj, "a", 1 ) ;
   bson_append_start_array( &obj, "b" ) ;
   bson_append_int( &obj, "0", 6 ) ;
   bson_append_string( &obj, "1", "7" ) ;
   bson_append_start_object( &obj, "c" ) ;
   bson_append_string( &obj, "key", "value" ) ;
   bson_append_finish_object( &obj ) ;
   bson_append_finish_array( &obj ) ;
   bson_append_string( &obj, "d", "str" ) ;
   rc = bson_finish( &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
}

TEST(cbson, dateType)
{
   sdbConnectionHandle    db = 0 ;
   sdbCollectionHandle    cl = 0 ;
   sdbCursorHandle cursor    = 0 ;
   INT32 rc                  = SDB_OK ;

   // normal
   const CHAR* ppNormalDate[] = {
      // the dates which are in the [1900-01-01, 9999-12-31]
      "{ \"myDate1\": { \"$date\": \"1900-01-01\" } }",
      "{ \"myDate2\": { \"$date\": \"9999-12-31\" } }",
      // the dates which are in the [0001-01-01T00:00:00.000000Z, 9999-12-31T23:59:59.999999Z]
      "{ \"myDate3\": { \"$date\": \"1900-01-01T00:00:00.000000Z\" } }",
      "{ \"myDate4\": { \"$date\": \"9999-12-31T12:59:59.999999Z\" } }",
      "{ \"myDate5\": { \"$date\": \"1900-01-01T00:00:00.000000-0100\" } }",
      "{ \"myDate6\": { \"$date\": \"1900-01-01T00:00:00.000000+0100\" } }",
      "{ \"myDate7\": { \"$date\": \"9999-12-31T23:59:59.999999-0100\" } }",
      "{ \"myDate8\": { \"$date\": \"9999-12-31T12:59:59.999999+0100\" } }",
      "{ \"myDate11\": { \"$date\": {\"$numberLong\":\"-30610339200000\" } } }", // 999-12-31
      "{ \"myDate12\": { \"$date\": {\"$numberLong\":\"-30610252800000\" } } }", // 1000-01-01
      "{ \"myDate13\": { \"$date\": {\"$numberLong\":\"-30610224000000\" } } }", // 1000-01-01T08:00:00:000000Z
      "{ \"myDate14\": { \"$date\": {\"$numberLong\":\"-2209017600000\" } } }", // 1899-01-01
      "{ \"myDate15\": { \"$date\": {\"$numberLong\":\"-2240553600000\" } } }", // 1900-01-01
      "{ \"myDate16\": { \"$date\": {\"$numberLong\":\"-2208988800000\" } } }", // 1900-01-01T08:00:00:000000Z
      "{ \"myDate17\": { \"$date\": {\"$numberLong\":\"0\" } } }", // 1970-01-01T08:00:00.000000Z
      "{ \"myDate18\": { \"$date\": {\"$numberLong\":\"946656000000\" } } }", // 2000-01-01
      "{ \"myDate19\": { \"$date\": {\"$numberLong\":\"253402185600000\" } } }", // 9999-12-31
      "{ \"myDate20\": { \"$date\": {\"$numberLong\":\"253402275599000\" } } }" // 9999-12-31T00:00:00:000000Z
   } ;

   const CHAR* ppAbnormalDate[] = {
      // the dates which are not in [1900-01-01, 9999-12-31]
      "{ \"myDate1\": { \"$date\": \"-1-12-31\" } }",
      "{ \"myDate2\": { \"$date\": \"10000-01-01\" } }",
      // the dates which are not in [0001-01-01T00:00:00.000000Z, 9999-12-31T23:59:59.999999Z]
      "{ \"myDate1\": { \"$date\": \"0000-01-01T00:00:00.000000Z\" } }",
      "{ \"myDate2\": { \"$date\": \"0000-01-01T23:59:59.999999-0100\" } }",
      "{ \"myDate3\": { \"$date\": \"-0001-01-01T00:00:00.000000Z\" } }",
      "{ \"myDate4\": { \"$date\": \"10000-01-01T00:00:00.000000Z\" } }",
      "{ \"myDate5\": { \"$date\": \"10000-01-01T00:00:00.000000+0100\" } }"
   } ;
   /// case1: test abnormal dates
#define bufsize 1024
   CHAR buffer[ bufsize ] = { 0 } ;
   bson obj ;
   INT32 i = 0 ;
   INT32 num = 10;
   bson_init( &obj ) ;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDelete( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check abnormal type
   for( i = 0; i < sizeof(ppAbnormalDate)/sizeof(const CHAR*); i++ )
   {
      bson_destroy( &obj ) ;
      if ( TRUE == jsonToBson( &obj, ppAbnormalDate[i] ) )
      {
         rc = SDB_INVALIDARG ;
         ASSERT_EQ( SDB_OK, rc ) << "i is: " << i << ", record is: " << ppAbnormalDate[i] ;
      }
   }

   /// case2
   {
   // insert
   printf( "The inserted records are as below: \n" ) ;
   for ( i = 0; i < sizeof(ppNormalDate)/sizeof(const CHAR*); i++ )
   {
      printf( "%s\n", ppNormalDate[i] ) ;
      if ( !jsonToBson( &obj, ppNormalDate[i] ) )
      {
         rc = SDB_INVALIDARG ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   rc = sdbQuery ( cl, NULL, NULL,
                   NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   bson_init ( &obj ) ;
   i = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      bson_iterator_next( &it ) ;
      bson_type type = bson_iterator_next( &it ) ;

      if ( !bsonToJson( buffer, bufsize, &obj, false ,false ) )
      {
         rc = SDB_SYS ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      printf( "Type is: %d, record is: %s\n", (int)type, buffer ) ;
      ASSERT_EQ( BSON_DATE, (int)type ) ;
      bson_destroy( &obj ) ;
   }
   }

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(cbson, timestampType)
{
   sdbConnectionHandle    db = 0 ;
   sdbCollectionHandle    cl = 0 ;
   sdbCursorHandle cursor    = 0 ;
   INT32 rc                  = SDB_OK ;

   // normal
   const CHAR* ppNormalTimestamp[] = {
      "{ \"myTimestamp1\": { \"$timestamp\": \"1902-01-01-00:00:00.000000\" } }", 
      "{ \"myTimestamp2\": { \"$timestamp\": \"1902-01-01T00:00:00.000000+0800\" } }",
      "{ \"myTimestamp3\": { \"$timestamp\": \"1902-01-01T00:00:00.000000Z\" } }",
      "{ \"myTimestamp4\": { \"$timestamp\": \"2037-12-31-23:59:59.999999\" } }",
      "{ \"myTimestamp5\": { \"$timestamp\": \"2037-12-31T23:59:59.999999+0800\" } }",
      "{ \"myTimestamp6\": { \"$timestamp\": \"2037-12-31T23:59:59.999999Z\" } }"
   } ;

//   // normal
//   const CHAR* ppNormalTimestamp[] = {
//      "{ \"myTimestamp1\": { \"$timestamp\": \"1901-12-14-04.45.52.000000\" } }", // if you find it can't pass,
//                                                                                  // please check your system,
//                                                                                  // whether it is "+0800 in Beijing",
//                                                                                  // but not "+0800 in Shanghai" or the
//                                                                                  // other place
//      "{ \"myTimestamp2\": { \"$timestamp\": \"2038-01-19-11.14.07.999999\" } }",
//      "{ \"myTimestamp3\": { \"$timestamp\": \"1901-12-13T20:45:52.000000Z\" } }",
//      "{ \"myTimestamp4\": { \"$timestamp\": \"1901-12-14T04:45:52.000000+0800\" } }",
//      "{ \"myTimestamp5\": { \"$timestamp\": \"2038-01-19T03:14:07.999999Z\" } }",
//      "{ \"myTimestamp6\": { \"$timestamp\": \"2038-01-19T11:14:07.999999+0800\" } }"
//   } ;
//
//
//   const CHAR* ppAbnormalTimestamp[] = {
//      "{ \"myTimestamp1\": { \"$timestamp\": \"1901-12-14-04.45.51.000000\" } }",
//      "{ \"myTimestamp2\": { \"$timestamp\": \"2038-01-19-11.14.08.000000\" } }",
//      "{ \"myTimestamp3\": { \"$timestamp\": \"1901-12-13T20:45:51.999999Z\" } }",
//      "{ \"myTimestamp4\": { \"$timestamp\": \"1901-12-14T04:45:51.999999+0800\" } }",
//      "{ \"myTimestamp5\": { \"$timestamp\": \"2038-01-19T03:14:08.000000Z\" } }",
//      "{ \"myTimestamp6\": { \"$timestamp\": \"2038-01-19T11:14:08.000000+0800\" } }"
//   } ;

   // abnormal
#define bufsize 1024
   CHAR buffer[ bufsize ] = { 0 } ;
   bson obj ;
   INT32 i = 0 ;
   INT32 num = 10;
   bson_init( &obj ) ;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDelete( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

//   // check abnormal type
//   for( i = 0; i < sizeof(ppAbnormalTimestamp)/sizeof(const CHAR*); i++ )
//   {
//      bson_destroy( &obj ) ;
//      if ( jsonToBson( &obj, ppAbnormalTimestamp[i] ) )
//      {
//         rc = SDB_INVALIDARG ;
//         ASSERT_EQ( SDB_OK, rc ) << ppAbnormalTimestamp[i] ;
//      }
//   }

   /// case1
   {
   // insert
   printf( "The inserted records are as below: \n" ) ;
   for ( i = 0; i < sizeof(ppNormalTimestamp)/sizeof(const CHAR*); i++ )
   {
      printf( "%s\n", ppNormalTimestamp[i] ) ;
      if ( !jsonToBson( &obj, ppNormalTimestamp[i] ) )
      {
         rc = SDB_INVALIDARG ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   rc = sdbQuery ( cl, NULL, NULL,
                   NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   printf( "The records queried are as below:" OSS_NEWLINE ) ;
   bson_init ( &obj ) ;
   i = 0 ;
   while( SDB_OK == ( rc = sdbNext( cursor, &obj ) ) )
   {
      i++ ;
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      bson_iterator_next( &it ) ;
      bson_type type = bson_iterator_next( &it ) ;

      if ( !bsonToJson( buffer, bufsize, &obj, false ,false ) )
      {
         rc = SDB_SYS ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      printf( "Type is: %d, record is: %s\n", (int)type, buffer ) ;
      ASSERT_EQ( BSON_TIMESTAMP, (int)type ) ;
      bson_destroy( &obj ) ;
   }
   }

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(cbson, cbson_jsCompatibility_toString)
{
   const char *pExpect1 = "{ \"a\": 9223372036854775807, \"b\": -9223372036854775808, \"c\": 2147483648, \"d\": -2147483649, \"e\": 0 }";
   const char *pExpect2 = "{ \"a\": { \"$numberLong\": \"9223372036854775807\" }, \"b\": { \"$numberLong\": \"-9223372036854775808\" }, \"c\": 2147483648, \"d\": -2147483649, \"e\": 0 }";
   bson obj;
   bson_init( &obj );
   bson_append_long( &obj, "a", 9223372036854775807LL );   // max long
   bson_append_long( &obj, "b", (-9223372036854775807LL-1) );  // min long
   bson_append_long( &obj, "c", 2147483648LL );            // max int + 1
   bson_append_long( &obj, "d", -2147483649LL );           // min int - 1
   bson_append_long( &obj, "e", 0 );                       // 0
   bson_finish( &obj );


   cout << "disable js compatibility: " ;
   bson_print( &obj );
   ASSERT_EQ( 0, bson_compare(pExpect1, &obj) );

   bson_set_js_compatibility( 1 );
   cout << "enable js compatibility: " ;
   bson_print( &obj );
   ASSERT_EQ( 0, bson_compare(pExpect2, &obj) );

   bson_set_js_compatibility( 0 );
   cout << "disable js compatibility: " ;
   bson_print( &obj );
   ASSERT_EQ( 0, bson_compare(pExpect1, &obj) );
   bson_destroy( &obj ) ;
}

