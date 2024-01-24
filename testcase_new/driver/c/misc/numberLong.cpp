/*****************************************************************************
 *@Description : seqDB-7546:c_输入strict格式，查询显示
 *               seqDB-7547:c_strict格式的参数校验
 *               seqDB-7548:c_strict格式的边界值校验
 *@Modify List : Ting YU
 *               2016-03-29
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <string.h>
#include <jstobs.h>
#include <client.h>
#include "testcommon.hpp"
#include "arguments.hpp"

TEST( numberLong, boundary )
{         
   //create cl
   INT32 rc = SDB_OK ;
   const CHAR* csName = "numberLongTestCs" ;
   const CHAR* clName = "numberLongTestCl" ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

   SINT64 longVals[] = { 9223372036854775807, -9223372036854775808 } ;
   for( INT32 i = 0;i < 2;i++ )
   {
      bson_iterator it ;
      SINT64 longVal = longVals[i] ;

      //number to string
      CHAR longStr[25] ;
      sprintf( longStr, "%lld", longVal ) ;

      //json to bson 
      CHAR recJson[100] ;
      sprintf( recJson, "%s%s%s", "{ \"_id\": { \"$numberLong\": \"", longStr, "\" } }") ;
      bson obj ;
      bson_init( &obj ) ;
      jsonToBson( &obj, recJson );
      ASSERT_EQ( SDB_OK, rc ) << "fail to jsonToBson" ;  
      rc = bson_finish( &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to finish bson" ;
      bson_find( &it, &obj, "_id" ) ;
      ASSERT_EQ( longVal, bson_iterator_long( &it ) ) << "fail to check value" ;

      //insert 
      rc = sdbDelete( cl, NULL, NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to delete" ;     
      rc = sdbInsert ( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

      //query
      sdbCursorHandle cursor ;
      rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

      INT32 cnt = 0;
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
      while( !sdbNext( cursor, &obj ) )
      {   
         bson_find( &it, &obj, "_id" ) ;      
         ASSERT_EQ( longVal, bson_iterator_long( &it ) ) << "fail to check value" ;    

         bson_destroy( &obj );
         bson_init( &obj );
         cnt++;
      }
      ASSERT_EQ( 1, cnt ) << "fail to check cnt" ;
      bson_destroy( &obj );
   }
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

TEST( numberLong, outofBoundary )
{                    
   BOOLEAN rc = FALSE ;
   bson obj ;

   CHAR recJson1[] = "{ \"a\": { \"$numberLong\": 9223372036854775808 } }" ;     
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson1 ); 
   ASSERT_EQ( FALSE, rc ) ;
   bson_destroy( &obj ) ;

   char recJson2[] = "{ \"a\": { \"$numberLong\": -9223372036854775809 } }" ;
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson2 ); 
   ASSERT_EQ( FALSE, rc ) ;      
   bson_destroy( &obj ) ;
}

TEST( numberLong, errorFormat )
{            
   BOOLEAN rc ;
   bson obj ;

   //1. error fomart: {"$numberLong":"123.6"}       
   char recJson1[] = "{ \"a\": { \"$numberLong\": \"123.5\" } }" ;     
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson1 ) ; 
   ASSERT_EQ( FALSE, rc ) ;
   bson_destroy( &obj ) ;

   //2. error fomart: {"$numberLong":123}       
   char recJson2[] = "{ \"a\": { \"$numberLong\": 123.5 } }" ;
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson2 ) ; 
   ASSERT_EQ( FALSE, rc ) ;
   bson_destroy( &obj ) ;  
}
