/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-13054:getQueryMeta参数校验
 * @Modify:        Liang xuewang Init
 *                 2017-10-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

class queryMetaTest13054 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;

      csName = "queryMetaTestCs13054" ;
      clName = "queryMetaTestCl13054" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = cs.createCollection( clName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

      BSONObj idxDef = BSON( "a" << 1 ) ;
      rc = cl.createIndex( idxDef, "aIndex", false, false ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;

      BSONObj doc = BSON( "a" << 10 ) ;
      rc = cl.insert( doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ; 
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;

      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( queryMetaTest13054, nullHint )
{
   INT32 rc = SDB_OK ;

   BSONObj cond = BSON( "a" << 10 ) ;
   sdbCursor cursor ;
   rc = cl.getQueryMeta( cursor, cond, _sdbStaticObject, _sdbStaticObject, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "ixscan", obj.getField( "ScanType" ).String() ) ;
   ASSERT_FALSE( obj.getField( "Indexblocks" ).eoo() ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( queryMetaTest13054, emptyHint )
{
   INT32 rc = SDB_OK ;
   
   BSONObj cond = BSON( "a" << 10 ) ;
   BSONObj hint ;
   sdbCursor cursor ;
   rc = cl.getQueryMeta( cursor, cond, _sdbStaticObject, hint, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "ixscan", obj.getField( "ScanType" ).String() ) ;
   ASSERT_FALSE( obj.getField( "Indexblocks" ).eoo() ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( queryMetaTest13054, withHint )
{
   INT32 rc = SDB_OK ;
   
   BSONObj cond = BSON( "a" << 10 ) ;
   BSONObjBuilder builder ;
   builder.appendNull( "" ) ;
   BSONObj hint = builder.obj() ;
   sdbCursor cursor ;
   rc = cl.getQueryMeta( cursor, cond, _sdbStaticObject, hint, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "tbscan", obj.getField( "ScanType" ).String() ) ;
   ASSERT_FALSE( obj.getField( "Datablocks" ).eoo() ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
