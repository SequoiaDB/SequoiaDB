/*******************************************************************************
*@Description : test selector query :
*               $elemMatch/$elemMatchOne/$slice/$default/$include
*@Example: query: db.cs.cl.find({},{"a":{$elemMatch:{"b":"abc"}}})
*          query: db.cs.cl.find({},{"a":{$elemMatchOne:{"b":"abc"}}})
*          query: db.cs.cl.find({},{"a":{$slice:{"b":12}}})
*          query: db.cs.cl.find({},{"a":{$default:{"b":"abc"}}})
*          query: db.cs.cl.find({},{"a.b.c.d":{$include:1/0}})
*@Modify List :
*               2015-2-11   xiaojun Hu   Change [adb abnormal test]
*******************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;


void selectCreateCollection( sdb &db, sdbCollection *cl, const CHAR *clName )
{
   sdbCollectionSpace cs ;
   INT32 rc = SDB_OK ;
   const CHAR *csName = "select_query_cs" ;
   const CHAR *hostName = HOST ;
   const CHAR *svcPort = SERVER ;
   const CHAR *usr = USER ;
   const CHAR *passwd = PASSWD ;

   rc = db.connect( hostName, svcPort, usr, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.createCollectionSpace( csName, 4096, cs ) ;
   if( SDB_DMS_CS_EXIST == rc )
   {
      rc = db.getCollectionSpace( csName, cs ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj optionObj = BSON( "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, optionObj, *cl ) ;
   if( SDB_DMS_EXIST == rc )
   {
      rc = cs.dropCollection( clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cs.createCollection( clName, *cl ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST( select, elementMatch )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_elementMatch" ;

   selectCreateCollection( db, &cl, clName ) ;
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo" << BSON( "$elemMatch" <<
                     BSON( "Name" << 41000 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

TEST( select, elementMatchOne )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_elementMatchOne" ;

   selectCreateCollection( db, &cl, clName ) ;
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo" << BSON( "$elemMatchOne" <<
                     BSON( "Name" << 41000 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

TEST( select, slice )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_slice" ;

   selectCreateCollection( db, &cl, clName ) ;
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( "rg1" << "rg2" << "rg3" <<
               "rg4" << "rg5" << "rg6" << "rg7" << "rg8" ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group" << BSON( "$slice" << BSON_ARRAY( -7 << 3 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

TEST( select, _default )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_default" ;

   selectCreateCollection( db, &cl, clName ) ;
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo.GroupName" << BSON( "$default" <<
                     BSON( "Name" << 41000 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

TEST( select, include )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_include" ;

   selectCreateCollection( db, &cl, clName ) ;
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo.GroupName" << BSON( "$include" << 0 ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

TEST( select, includeAbnormal )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_includeAbnormal" ;

   selectCreateCollection( db, &cl, clName ) ;
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo.GroupName" << BSON( "$include" << 1 ) <<
                     "Group.GroupInfo.GroupName" << BSON( "$include" << 1 ) <<
                     "Group.GroupInfo.GroupName" << BSON( "$include" << 0 )) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   cout << "success to test, throw -6[Invalid Arguement]" << endl ;
}
