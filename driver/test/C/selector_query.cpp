/*******************************************************************************
*@Description : test selector query :
*               $elemMatch/$elemMatchOne/$slice/$default/$include
*@Example: query: db.cs.cl.find({},{"a":{$elemMatch:{"b":"abc"}}})
*          query: db.cs.cl.find({},{"a":{$elemMatchOne:{"b":"abc"}}})
*          query: db.cs.cl.find({},{"a":{$slice:{"b":12}}})
*          query: db.cs.cl.find({},{"a":{$default:{"b":"abc"}}})
*          query: db.cs.cl.find({},{"a.b.c.d":{$include:1/0}})
*@Modify list :
*               2015-02-04  xiaojun Hu  Init
*******************************************************************************/

#include <stdio.h>
#include <gtest/gtest.h>
#include <string>
#include "testcommon.h"
#include "client.h"
#include "jstobs.h"

void selectCreateCL( sdbCollectionHandle *cl )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCSHandle cs = SDB_OK ;
   INT32 rc = SDB_OK ;
   const CHAR *csName = "selector_query_cs" ;
   const CHAR *clName = "selector_query_cl" ;

   rc = sdbConnect( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson csOptions ;
   bson_init( &csOptions ) ;
   bson_append_int( &csOptions, "PageSize", 65536 ) ;
   bson_append_finish_object( &csOptions ) ;
   rc = sdbCreateCollectionSpaceV2( db, csName, &csOptions, &cs ) ;
   if( SDB_DMS_CS_EXIST == rc )
   {
      rc = sdbGetCollectionSpace( db, csName, &cs ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &csOptions ) ;
   bson clOptions ;
   bson_init( &clOptions ) ;
   bson_append_int( &clOptions, "ReplSize", 0 ) ;
   bson_append_bool( &clOptions, "Compressed", true ) ;
   bson_append_finish_object( &clOptions ) ;
   rc = sdbDropCollection( cs, clName ) ;
   if( SDB_DMS_NOTEXIST == rc )
   {
      printf( "collection:%s don't exist\n", clName ) ;
   }
   else
   {
      printf( "success to drop collections\n" ) ;
   }
   rc = sdbCreateCollection1( cs, clName, &clOptions, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &clOptions ) ;
}

TEST( selector, elementMatch )
{
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;
   bson recordObj ;
   const CHAR *pStr = "{ Group: [ { \"GroupInfo\":[ { \"GroupName\":\"group1\", \"SvcType\": 1000, \"SvcName\": 41000 } ] } ] }" ;

   selectCreateCL( &cl ) ;
   rc = jsonToBson( &recordObj, pStr ) ;
   ASSERT_EQ( TRUE, rc ) ; 

   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   bson selectObj ;
   bson_init( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group.GroupInfo" ) ;
   bson_append_start_object( &selectObj, "$elemMatch" ) ;
   bson_append_int( &selectObj, "SvcName", 41000 ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_finish( &selectObj ) ;
   bson_print( &selectObj ) ;
   rc = sdbQuery( cl, NULL, &selectObj, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &selectObj ) ;
   printf( "query over\n" ) ;
   printf( "<return record>:\n" ) ;
   bson retObj ;
   bson_init( &retObj ) ;
   while( SDB_DMS_EOC != (rc = sdbNext( cursor, &retObj )) )
   {
      bson_print( &retObj ) ;
   }
   bson_destroy( &retObj ) ;
}

TEST( selector, elementMatchOne )
{
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;
   const CHAR *pStr = "{ Group: [ { \"GroupInfo\":[ { \"GroupName\":\"group1\", \"SvcType\": 1000, \"SvcName\": 41000 } ] } ] }" ;

   selectCreateCL( &cl ) ;

   bson recordObj ;
   rc = jsonToBson( &recordObj, pStr ) ;
   ASSERT_EQ( TRUE, rc ) ;
    
   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   bson selectObj ;
   bson_init( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group.GroupInfo" ) ;
   bson_append_start_object( &selectObj, "$elemMatchOne" ) ;
   bson_append_int( &selectObj, "SvcName", 41000 ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_finish( &selectObj ) ;
   bson_print( &selectObj ) ;
   rc = sdbQuery( cl, NULL, &selectObj, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &selectObj ) ;
   printf( "query over\n" ) ;
   printf( "<return record>:\n" ) ;
   bson retObj ;
   bson_init( &retObj ) ;
   while( SDB_DMS_EOC != (rc = sdbNext( cursor, &retObj )) )
   {
      bson_print( &retObj ) ;
   }
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
}
TEST( selector, slice )
{
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   selectCreateCL( &cl ) ;
   bson recordObj ;
   bson subObj ;
   bson_init( &recordObj ) ;
   bson_append_start_array( &recordObj, "Group" ) ;
   bson_append_string( &recordObj, "0", "rg1" ) ;
   bson_append_string( &recordObj, "1", "rg2" ) ;
   bson_append_string( &recordObj, "2", "rg3" ) ;
   bson_append_string( &recordObj, "3", "rg4" ) ;
   bson_append_string( &recordObj, "4", "rg5" ) ;
   bson_append_string( &recordObj, "5", "rg6" ) ;
   bson_append_finish_array( &recordObj ) ;
   bson_finish( &recordObj ) ;
   printf( "<Inserted Record>:\n" ) ;
   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   bson selectObj ;
   bson_init( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group" ) ;
   bson_append_start_array( &selectObj, "$slice" ) ;
   bson_append_int( &selectObj, "0", 1 ) ;
   bson_append_int( &selectObj, "1", 3 ) ;
   bson_append_finish_array( &selectObj ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_finish( &selectObj ) ;
   printf( "<Query Conditions>:\n" ) ;
   bson_print( &selectObj ) ;
   rc = sdbQuery( cl, NULL, &selectObj, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &selectObj ) ;
   printf( "query over\n" ) ;
   printf( "<Return Record>:\n" ) ;
   bson retObj ;
   bson_init( &retObj ) ;
   while( SDB_DMS_EOC != (rc = sdbNext( cursor, &retObj )) )
   {
      bson_print( &retObj ) ;
   }
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
}

TEST( selector, _default )
{
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   selectCreateCL( &cl ) ;
   bson recordObj ;
   bson subObj ;
   bson_init( &recordObj ) ;
   bson_append_string( &recordObj, "Group1", "rg1" ) ;
   bson_append_string( &recordObj, "Group2", "rg2" ) ;
   bson_append_string( &recordObj, "Group3", "rg3" ) ;
   bson_append_string( &recordObj, "Group1", "rg4" ) ;
   bson_append_string( &recordObj, "Group1", "rg5" ) ;
   bson_finish( &recordObj ) ;
   printf( "<Inserted Record>:\n" ) ;
   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   bson selectObj ;
   bson_init( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group1" ) ;
   bson_append_string( &selectObj, "$default", "defaultValue" ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_finish( &selectObj ) ;
   printf( "<Query Conditions>:\n" ) ;
   bson_print( &selectObj ) ;
   rc = sdbQuery( cl, NULL, &selectObj, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &selectObj ) ;
   printf( "query over\n" ) ;
   printf( "<Return Record>:\n" ) ;
   bson retObj ;
   bson_init( &retObj ) ;
   while( SDB_DMS_EOC != (rc = sdbNext( cursor, &retObj )) )
   {
      bson_print( &retObj ) ;
   }
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
}

TEST( selector, include )
{
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   selectCreateCL( &cl ) ;
   bson recordObj ;
   bson subObj ;
   bson_init( &recordObj ) ;
   bson_append_string( &recordObj, "Group1", "rg1" ) ;
   bson_append_string( &recordObj, "Group2", "rg2" ) ;
   bson_append_string( &recordObj, "Group3", "rg3" ) ;
   bson_append_string( &recordObj, "Group1", "rg4" ) ;
   bson_append_string( &recordObj, "Group1", "rg5" ) ;
   bson_finish( &recordObj ) ;
   printf( "<Inserted Record>:\n" ) ;
   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   bson selectObj ;
   bson_init( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group1" ) ;
   bson_append_int( &selectObj, "$include", 0 ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_finish( &selectObj ) ;
   printf( "<Query Conditions>:\n" ) ;
   bson_print( &selectObj ) ;
   rc = sdbQuery( cl, NULL, &selectObj, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &selectObj ) ;
   printf( "query over\n" ) ;
   printf( "<Return Record>:\n" ) ;
   bson retObj ;
   bson_init( &retObj ) ;
   while( SDB_DMS_EOC != (rc = sdbNext( cursor, &retObj )) )
   {
      bson_print( &retObj ) ;
   }
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
}

TEST( selector, includeAbnormal )
{
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   selectCreateCL( &cl ) ;
   bson recordObj ;
   bson subObj ;
   bson_init( &recordObj ) ;
   bson_append_string( &recordObj, "Group1", "rg1" ) ;
   bson_append_string( &recordObj, "Group2", "rg2" ) ;
   bson_append_string( &recordObj, "Group3", "rg3" ) ;
   bson_append_string( &recordObj, "Group1", "rg4" ) ;
   bson_append_string( &recordObj, "Group1", "rg5" ) ;
   bson_finish( &recordObj ) ;
   printf( "<Inserted Record>:\n" ) ;
   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   bson selectObj ;
   bson_init( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group1" ) ;
   bson_append_int( &selectObj, "$include", 0 ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group1" ) ;
   bson_append_int( &selectObj, "$include", 1 ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_append_start_object( &selectObj, "Group1" ) ;
   bson_append_int( &selectObj, "$include", 0 ) ;
   bson_append_finish_object( &selectObj ) ;
   bson_finish( &selectObj ) ;
   printf( "<Query Conditions>:\n" ) ;
   bson_print( &selectObj ) ;
   rc = sdbQuery( cl, NULL, &selectObj, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &selectObj ) ;
   printf( "query over, here throw -6[Invalid Argument]\n" ) ;
/*
   printf( "<Return Record>:\n" ) ;
   bson retObj ;
   bson_init( &retObj ) ;
   while( SDB_DMS_EOC != (rc = sdbNext( cursor, &retObj )) )
   {
      bson_print( &retObj ) ;
   }
   bson_destroy( &retObj ) ;
*/
   printf( "test over\n" ) ;
}
