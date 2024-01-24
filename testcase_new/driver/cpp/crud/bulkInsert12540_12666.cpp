/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12540:bulkInsert批量插入数据
 *                 seqDB-12666:bulkInsert批量插入数据，数组为空
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class bulkInsertTest12540 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "bulkInsertTestCs12540" ;
      clName = "bulkInsertTestCl12540" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( bulkInsertTest12540, bulkInsert12540 )
{
   INT32 rc = SDB_OK ;

   // make doc with _id, without _id
   vector<BSONObj> docs ;
   BSONObj doc1 = BSON( "_id" << 1 << "a" << 1 ) ;
   BSONObj doc2 = BSON( "a" << 2 ) ;
   docs.push_back( doc1 ) ;
   docs.push_back( doc2 ) ;

   // bulkInsert docs
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulkInsert docs to cl " << clName ;

   // check docs num
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;

   // query doc with _id, check _id
   BSONObj cond = BSON( "a" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query doc" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "_id" ).Int() ) << "fail to check _id" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // query doc without _id, check _id
   sdbCursor cursor1 ;
   cond = BSON( "a" << 2 ) ;
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query doc" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( jstOID, obj.getField( "_id" ).type() ) << "fail to check _id" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}

TEST_F( bulkInsertTest12540, bulkInsert12540_1 )
{
   INT32 rc = SDB_OK ;

   vector<BSONObj> docs ;
   BSONObj doc1 = BSON( "_id" << 1 << "a" << 1 ) ;
   BSONObj doc2 = BSON( "_id" << 2 << "a" << 2 ) ;
   BSONObj doc3 = BSON( "_id" << 1 << "a" << 3 ) ;
   docs.push_back( doc1 ) ;
   docs.push_back( doc2 ) ;
   docs.push_back( doc3 ) ;

   // test SDB_INSERT_CONTONDUP_ID
   rc = cl.bulkInsert( FLG_INSERT_CONTONDUP_ID, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulkInsert docs to cl " << clName ;

   // check docs num
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;

   // query doc with _id, check _id
   BSONObj cond = BSON( "_id" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query doc" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check _id" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // test SDB_INSERT_REPLACEONDUP_ID
   rc = cl.truncate() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncate" ;
   rc = cl.bulkInsert( FLG_INSERT_REPLACEONDUP_ID, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulkInsert docs to cl " << clName ;

   // check docs num
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;

   // query doc with _id, check _id
   sdbCursor cursor1 ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query doc" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 3, obj.getField( "a" ).Int() ) << "fail to check _id" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}


TEST_F( bulkInsertTest12540, bulkInsert12666 )
{
   INT32 rc = SDB_OK ;

   vector<BSONObj> docs ;
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulkInsert empty docs" ;

   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( 0, count ) << "fail to check count" ;

   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}
