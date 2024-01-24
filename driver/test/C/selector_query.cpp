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

// create collection
void selectCreateCL( sdbConnectionHandle *sdb, sdbCollectionHandle *cl )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCSHandle cs = SDB_OK ;
   //sdbCollectionHandle cl = SDB_OK ;
   INT32 rc = SDB_OK ;
   const CHAR *csName = "selector_query_cs" ;
   const CHAR *clName = "selector_query_cl" ;

   // connect to sdb
   rc = sdbConnect( HOST, SERVER, USER, PASSWD, sdb ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   db = *sdb ;
   // create collection space
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
   // create collection
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
   sdbReleaseCS( cs ) ;
}

// test for $elementMatch
TEST( selector, elementMatch )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;
   bson recordObj ;
   const CHAR *pStr = "{ Group: [ { \"GroupInfo\":[ { \"GroupName\":\"group1\", \"SvcType\": 1000, \"SvcName\": 41000 } ] } ] }" ;

   // create colleciton and insert data
   selectCreateCL( &db, &cl ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "SvcName": 41000},...]}]}
   rc = jsonToBson( &recordObj, pStr ) ;
   ASSERT_EQ( TRUE, rc ) ; 

   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"SvcName":41000}}}
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
   // parse object
   bson_destroy( &retObj ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

// test for $elemMatchOne
TEST( selector, elementMatchOne )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;
   const CHAR *pStr = "{ Group: [ { \"GroupInfo\":[ { \"GroupName\":\"group1\", \"SvcType\": 1000, \"SvcName\": 41000 } ] } ] }" ;

   // create colleciton and insert data
   selectCreateCL( &db, &cl ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "SvcName": 41000},...]}]}

   bson recordObj ;
   rc = jsonToBson( &recordObj, pStr ) ;
   ASSERT_EQ( TRUE, rc ) ;
    
   bson_print( &recordObj ) ;
   rc = sdbInsert( cl, &recordObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &recordObj ) ;

   // $elementMatchOne: {"Group.GroupInfo":{"$elemMatchOne":{"Name":41000}}}
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
   // parse object
   bson_destroy( &retObj ) ;
   sdbReleaseCursor( cursor ) ;
   printf( "test over\n" ) ;
   sdbReleaseCollection( cl ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}
// test for $slice
TEST( selector, slice )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   // create colleciton and insert data
   selectCreateCL( &db, &cl ) ;
   // { Group:[ "rg1", "rg2", "rg3", "rg4", "rg5", "rg6" ]}
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

   // $slice: {"Group":{"$slice": [ 1, 3 ] }}}
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
   // parse object
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ; 
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

// test for $default
TEST( selector, _default )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   // create colleciton and insert data
   selectCreateCL( &db, &cl ) ;
   // { Group1: "rg1", Group2:"rg2", Group3: "rg3", Group1:"rg4", Group1: "rg5"}
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

   // $slice: {"Group":{"$default": "defaultValue"}}}
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
   // parse object
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

// test for $include
TEST( selector, include )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   // create colleciton and insert data
   selectCreateCL( &db, &cl ) ;
   // { Group1: "rg1", Group2:"rg2", Group3: "rg3", Group1:"rg4", Group1: "rg5"}
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

   // $slice: {"Group":{"$include": 0}}
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
   // parse object
   bson_destroy( &retObj ) ;
   printf( "test over\n" ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

// test for $include
TEST( selector, includeAbnormal )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCollectionHandle cl = SDB_OK ;
   sdbCursorHandle cursor = SDB_OK ;
   INT32 rc = SDB_OK ;

   // create colleciton and insert data
   selectCreateCL( &db, &cl ) ;
   // { Group1: "rg1", Group2:"rg2", Group3: "rg3", Group1:"rg4", Group1: "rg5"}
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

   // $slice: {"Group":{"$include": 0}}
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
   // parse object
   bson_destroy( &retObj ) ;
*/
   printf( "test over\n" ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}
