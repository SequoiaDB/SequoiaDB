#include "client.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

const CHAR *CL_NAME = "foo.bar" ;
const CHAR *HOST = "localhost" ;
const UINT16 SVC = 11810 ;
sdb db ;
sdbCollection cl ;
BOOLEAN INITED = FALSE ;

INT32 getCollection()
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace cs ;

   if ( INITED )
   {
      goto done ;
   }

   rc = db.connect( HOST, SVC ) ;
   if ( SDB_OK != rc )
   {
      cerr << "failed to connect to sdb:" << rc << endl ;
      goto error ;
   }

   db.dropCollectionSpace( "foo" ) ;

   rc = db.createCollectionSpace( "foo", 4096, cs ) ;
   if ( SDB_OK != rc )
   {
      cerr << "failed to create cs:" << rc << endl ;
      goto error ;
   }

   rc = cs.createCollection( "bar", BSONObj(), cl ) ;
   if ( SDB_OK != rc )
   {
      cerr << "failed to create cl:" << rc << endl ;
      goto error ;
   }

/*
   rc = db.getCollection( CL_NAME, cl ) ;
   if ( SDB_OK != rc )
   {
      cerr << "failed to get cl:" << rc << endl ;
      goto error ;
   }
*/
   INITED = TRUE ;
done:
   return rc ;
error:
   db.disconnect() ;
   INITED = FALSE ;
   goto done ;
}

/// record: {a:[1,2]}
///         {a:[3,4]}
/// match: {"a.$0":1 }
/// return: {a:[1,2]}
TEST( mthmatcher, dollar_match_1 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 3 << 4 ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << 2 ) ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   con = BSON( "a.$0" << 1 ) ;
   rc = cl.queryOne( obj, con, BSON( "_id" << BSON( "$include" << 0 ) ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( 0, obj.woCompare( r1 ) ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;
}

/// record: {a:[1,2]}
///          {a:[3,4]}
/// match: {"a.$0":1 }, set a.$0 to 5
/// final   {a:[5, 2]}
///         {a:[3,4])
TEST( mthmatcher, dollar_match_2 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 3 << 4 ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << 2 ) ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rule = BSON( "$set" << BSON( "a.$0" << 5) ) ;
   con = BSON( "a.$0" << 1 ) ;
   rc = cl.update( rule, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, cnt ) ;

   rc = cl.getCount( cnt, BSON( "a.$0" << 5 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;
   
}

/// record: {a:[1,[2,3]]}
///         {a:[1, [4,5]]}
/// match: {"a.$0.$1":2}
/// return {a:[1,[2,3]]}
TEST( mthmatcher, dollar_match_3 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 1 << BSON_ARRAY( 4 << 5 ) ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << BSON_ARRAY( 2 << 3 ) ) ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   con = BSON( "a.$0.$1" << 2 ) ;
   rc = cl.queryOne( obj, con, BSON( "_id" << BSON( "$include" << 0 ) ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( 0, obj.woCompare( r1 ) ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;   
}

/// record: {a:[1,[2,3]]}
///         {a:[1, [4,5]]}
/// match {"a.$0.$1:2} set {"a.$0.$1": 6}
/// final {a:[1,[6,3]}
//        {a:[1, [4,5]]}
TEST( mthmatcher, dollar_match_4 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 1 << BSON_ARRAY( 4 << 5 ) ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << BSON_ARRAY( 2 << 3 ) ) ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rule = BSON( "$set" << BSON( "a.$0.$1" << 6) ) ;
   con = BSON( "a.$0.$1" << 2 ) ;
   rc = cl.update( rule, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, cnt ) ;

   rc = cl.getCount( cnt, BSON( "a.$0.$1" << 6 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;
}

/// record: {a:[1, {b:[2,3]}]}
///         {a:[1, {b:[4,5]}]}
/// match {"a.$0.b.$1":2}
/// return {a:[1, {b:[2,3]}]}
TEST( mthmatcher, dollar_match_5 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 1 << BSON( "b" << BSON_ARRAY( 4 << 5 ) ) ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << BSON( "b" << BSON_ARRAY( 2 << 3 ) ) ) ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   con = BSON( "a.$0.b.$1" << 2 ) ;
   rc = cl.queryOne( obj, con, BSON( "_id" << BSON( "$include" << 0 ) ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( 0, obj.woCompare( r1 ) ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;   
}

/// record: {a:[1, {b:[2,3]}]}
///         {a:[1, {b:[4,5]}]}
/// match {"a.$0.b.$1":2} set {"a.$0.b.$1":6}
/// final   {a:[1, {b:[6,3]}]}}
//          {a:[1, {b:[4,5]}]}
TEST( mthmatcher, dollar_match_6 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 1 << BSON( "b" << BSON_ARRAY( 4 << 5 ) ) ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << BSON( "b" << BSON_ARRAY( 2 << 3 ) ) ) );
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rule = BSON( "$set" << BSON( "a.$0.b.$1" << 6) ) ;
   con = BSON( "a.$0.b.$1" << 2 ) ;
   rc = cl.update( rule, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, cnt ) ;

   rc = cl.getCount( cnt, BSON( "a.$0.b.$1" << 6 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;
}

/// record {a:[1,2,3], b:1}
//         {a:[1,2,3], b:4}
//  match  {"a.$0":{$field:"b"}}
//  return {a:[1,2,3], b:1}
TEST( mthmatcher, dollar_match_7 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 4 ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 1 ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   con = BSON( "a.$0" << BSON( "$field" << "b" ) ) ;
   rc = cl.queryOne( obj, con, BSON( "_id" << BSON( "$include" << 0 ) ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( 0, obj.woCompare( r1 ) ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;  
}

/// record {a:[1,2,3], b:1}
//         {a:[1,2,3], b:4}
//  match  {"a.$0":{$field:"b"}}  set {"a.$0": 5}
//  final      {a:[5,2,3], b:1}
//             {a:[1,2,3], b:4}
TEST( mthmatcher, dollar_match_8 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 4 ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 1 ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rule = BSON( "$set" << BSON( "a.$0" << 5) ) ;
   con = BSON( "a.$0" << BSON( "$field" << "b" ) ) ;
   rc = cl.update( rule, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, cnt ) ;

   rc = cl.getCount( cnt, BSON( "a.$0" << 5 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;
}

/// index {a.b:1}
// match {a.$0.b.$1:1}
// ix scan
TEST( mthmatcher, dollar_match_9 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;
   sdbCursor cursor ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.createIndex( BSON( "a.b" << 1 ), "a", FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.explain( cursor, BSON( "a.$0.b.$1" << 1 )) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONElement e = obj.getField( "ScanType" ) ;
   ASSERT_STREQ( e.valuestrsafe(), "ixscan" ) ;
   e = obj.getField( "IndexName" ) ;
   ASSERT_STREQ( e.valuestrsafe(), "a" ) ;

   cursor.close() ;
}

/// record {a:[{b:[{c:1}, {c:2}]}]}
///        {a:[{b:[{c:3}, {c:4}]}]}
//  match {"a.$0.b.$1.c":1}
//  return {a:[{b:[{c:1}, {c:2}]}]}
TEST( mthmatcher, dollar_match_10 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( BSON( "b" << BSON_ARRAY( BSON( "c" << 3) << BSON( "c" << 4 ))) ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( BSON( "b" << BSON_ARRAY( BSON( "c" << 1) << BSON( "c" << 2 ))) ) ) ; 
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   con = BSON( "a.$0.b.$1.c" << 1 ) ;
   rc = cl.queryOne( obj, con, BSON( "_id" << BSON( "$include" << 0 ) ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( 0, obj.woCompare( r1 ) ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;
}

/// record {a:[{b:[{c:1}, {c:2}]}]}
//         {a:[{b:[{c:3}, {c:4}]}]}
//   match {"a.$0.b.$1.c":1}  set {"a.$0.b.$1.c":5}
//   final      {a:[{b:[{c:51}, {c:2}]}]}
//              {a:[{b:[{c:3}, {c:4}]}]}
TEST( mthmatcher, dollar_match_11 )
{
   INT32 rc = SDB_OK ;
   SINT64 cnt = 0 ;
   BSONObj r1, r2, rule, con, obj ;

   rc = getCollection() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r2 = BSON( "a" << BSON_ARRAY( BSON( "b" << BSON_ARRAY( BSON( "c" << 3) << BSON( "c" << 4 ))) ) ) ;
   rc = cl.insert( r2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   r1 = BSON( "a" << BSON_ARRAY( BSON( "b" << BSON_ARRAY( BSON( "c" << 1) << BSON( "c" << 2 ))) ) ) ;
   rc = cl.insert( r1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rule = BSON( "$set" << BSON( "a.$0.b.$1.c" << 5) ) ;
   con = BSON( "a.$0.b.$1.c" << 1 ) ;
   rc = cl.update( rule, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.getCount( cnt, con ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, cnt ) ;

   rc = cl.getCount( cnt, BSON( "a.$0.b.$1.c" << 5 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, cnt ) ;   
}

