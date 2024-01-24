/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12541:测试upsert
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

class upsertTest12541 : public testBase
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
      csName = "upsertTestCs12541" ;
      clName = "upsertTestCl12541" ;
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

TEST_F( upsertTest12541, upsertMatch12541 )
{
   INT32 rc = SDB_OK ;

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   // upsert with cond match exist
   BSONObj cond = BSON( "a" << 1 ) ;
   BSONObj rule = BSON( "$set" << BSON( "a" << 10 ) ) ;
   rc = cl.upsert( rule, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;

   // check upsert
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 10, obj.getField( "a" ).Int() ) << "fail to check query result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}

TEST_F( upsertTest12541, upsertNotMatch12541 )
{
   INT32 rc = SDB_OK ;

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   // upsert with cond match not exist
   BSONObj cond = BSON( "a" << 2 ) ;
   BSONObj rule = BSON( "$set" << BSON( "a" << 10 ) ) ;
   BSONObj setOnInsert = BSON( "a" << 2 ) ;
   rc = cl.upsert( rule, cond, _sdbStaticObject, setOnInsert ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;

   // check upsert
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 2, obj.getField( "a" ).Int() ) << "fail to check query result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}
