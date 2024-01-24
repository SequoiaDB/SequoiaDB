/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-30391:insert指定flag值插入单条数据
 * @Modify:        Cheng Jingjing Init
 *                 2023-03-09
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

class insertWithFlag30391 : public testBase
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
      csName = "cs_30391" ;
      clName = "cl_30391" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
      // create index
      BSONObj idxDef = BSON( "a" << 1 ) ;
      const CHAR* idxName = "aIndex" ;
      rc = cl.createIndex( idxDef, idxName, TRUE, FALSE ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;
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

BOOLEAN checkOprNum( const BSONObj& obj, int insertNum, int dupNum )
{
   static const string InsertedNum = "InsertedNum";
   static const string DuplicatedNum = "DuplicatedNum";
   cout << obj.toString() << endl;
   if ( obj.getField( InsertedNum ).Long() != insertNum )
   {
      return FALSE ;
   }
   if  ( obj.getField( DuplicatedNum ).Long() != dupNum )
   {
      return FALSE ;
   }
   return TRUE ;
}

TEST_F( insertWithFlag30391, insertWithFlag_0 )
{
   // test flag = 0
   INT32 rc = SDB_OK ;
   BSONObj result ;

   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "a" << 1 ) ;
   rc = cl.insert( record1, 0, &result ) ;
   ASSERT_EQ( SDB_IXM_DUP_KEY, rc ) << "expected rc is -38, actual rc is " << rc ;
   BOOLEAN ret = checkOprNum( result, 0, 1 );

   // insert a non-conflicting record
   BSONObj record2 = BSON( "a" << 2 ) ;
   rc = cl.insert( record2, 0, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   ret = checkOprNum( result, 1, 0 );

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;
}

TEST_F( insertWithFlag30391, insertWithFlag_1 )
{
   // test flag = 0
   INT32 rc = SDB_OK ;
   BSONObj result ;

   BSONObj doc = BSON( "_id" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "_id" << 1 << "b" << 2 ) ;
   rc = cl.insert( record1, 0, &result ) ;
   ASSERT_EQ( SDB_IXM_DUP_KEY, rc ) << "expected rc is -38, actual rc is " << rc ;
   BOOLEAN ret = checkOprNum( result, 0, 1 );

   // insert a non-conflicting record
   BSONObj record2 = BSON( "_id" << 2 << "b" << 2 ) ;
   rc = cl.insert( record2, 0, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   ret = checkOprNum( result, 1, 0 );

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;
}

TEST_F( insertWithFlag30391, insertWithFlag_2 )
{
   // test flag = FLG_INSERT_CONTONDUP
   INT32 rc = SDB_OK ;
   BSONObj result ;
   BSONObj obj;

   BSONObj doc = BSON( "a" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "a" << 1 << "b" << 2 ) ;
   rc = cl.insert( record1, FLG_INSERT_CONTONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   BOOLEAN ret = checkOprNum( result, 0, 1 ) ;
   BSONObj cond = BSON( "a" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "b" ).Int() ) << "fail to check a" ;

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
}

TEST_F( insertWithFlag30391, insertWithFlag_3 )
{
   // test flag = FLG_INSERT_CONTONDUP|FLG_INSERT_RETURN_OID
   INT32 rc = SDB_OK ;
   BSONObj result ;
   BSONObj obj;

   BSONObj doc = BSON( "_id" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "_id" << 1 << "b" << 2 ) ;
   rc = cl.insert( record1, FLG_INSERT_CONTONDUP|FLG_INSERT_RETURN_OID, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   ASSERT_EQ( 1, result.getField( "_id" ).Int() ) << "fail to check _id" ;
   BOOLEAN ret = checkOprNum( result, 0, 1 ) ;
   BSONObj cond = BSON( "_id" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "b" ).Int() ) << "fail to check a" ;

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
}

TEST_F( insertWithFlag30391, insertWithFlag_4 )
{
   // test flag = FLG_INSERT_REPLACEONDUP
   INT32 rc = SDB_OK ;
   BSONObj result ;
   BSONObj obj;

   BSONObj doc = BSON( "a" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "a" << 1 << "b" << 2 ) ;
   rc = cl.insert( record1, FLG_INSERT_REPLACEONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   BOOLEAN ret = checkOprNum( result, 1, 1 ) ;
   BSONObj cond = BSON( "a" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 2, obj.getField( "b" ).Int() ) << "fail to check a" ;

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
}

TEST_F( insertWithFlag30391, insertWithFlag_5 )
{
   // test flag = FLG_INSERT_CONTONDUP_ID
   INT32 rc = SDB_OK ;
   BSONObj result ;
   BSONObj obj;

   BSONObj doc = BSON( "_id" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "_id" << 1 << "b" << 2 ) ;
   rc = cl.insert( record1, FLG_INSERT_CONTONDUP_ID, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   BOOLEAN ret = checkOprNum( result, 1, 1 ) ;
   BSONObj cond = BSON( "_id" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "b" ).Int() ) << "fail to check a" ;

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
}

TEST_F( insertWithFlag30391, insertWithFlag_6 )
{
   // test flag = FLG_INSERT_REPLACEONDUP_ID
   INT32 rc = SDB_OK ;
   BSONObj result ;
   BSONObj obj;

   BSONObj doc = BSON( "_id" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // insert a conflicting record
   BSONObj record1 = BSON( "_id" << 1 << "b" << 2 ) ;
   rc = cl.insert( record1, FLG_INSERT_REPLACEONDUP_ID, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   BOOLEAN ret = checkOprNum( result, 1, 1 ) ;
   BSONObj cond = BSON( "_id" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 2, obj.getField( "b" ).Int() ) << "fail to check a" ;

   // check records
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
}