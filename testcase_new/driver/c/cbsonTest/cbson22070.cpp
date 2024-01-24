#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include "client.h"
#include "arguments.hpp"

using namespace std;
#define bufsize 128 

TEST(cbson22070, jsonToBson)
{
   const CHAR expectJSONTimestamp[ bufsize ] = "{ \"myTimestamp1\": { \"$timestamp\": \"1902-01-01-00.00.00.000000\" } }";
   const CHAR expectJSONString[ bufsize ] = "{ \"str\": \"stringTest\" }";
   CHAR actJSONTimestamp[ bufsize ] = { 0 } ;
   CHAR actJSONString [ bufsize ] = { 0 } ;

   bson toObj ;
   bson_init( &toObj ) ;

   bson fromObj ;
   bson_init ( &fromObj) ; 
   bson_append_string( &fromObj, "str", "stringTest" );
   bson_finish ( &fromObj ) ; 

   ASSERT_TRUE( jsonToBson( &toObj, expectJSONTimestamp ) ) ;
   ASSERT_TRUE( bsonToJson( actJSONTimestamp, bufsize, &toObj, false ,false ) ) ;
   ASSERT_STREQ( actJSONTimestamp, expectJSONTimestamp ) ;

   ASSERT_TRUE( bsonToJson( actJSONString, bufsize, &fromObj, false ,false ) ) ;
   ASSERT_STREQ( actJSONString, expectJSONString ) ;

   bson_destroy( &fromObj ) ;
   bson_destroy( &toObj ) ;
}
