/**************************************************
 * @Description: test case for c++ driver
 *               seqDB-15381: use invalid flag to query
 * @Modify:      Suqiang Ling
 *               2018-06-01
 **************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class invalidFlag15381 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   INT32 docNum ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "invalidFlag15381" ;
      clName = "invalidFlag15381" ;
      INT32 i ;
      docNum = 20 ;

      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ; 
      vector<BSONObj> docs ;
      for( i = 0; i < docNum; i++ )
      {
         BSONObj doc = BSON( "a" << 1 ) ;
         docs.push_back( doc ) ;
      }
      rc = cl.bulkInsert( 0, docs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert documents" ;
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

INT32 checkDocs( sdbCursor &cursor, const INT32 expDocNum )
{
	INT32 rc = SDB_OK ;
	BSONObj obj ;
   INT32 actDocNum = 0 ;
   while( !( rc = cursor.next( obj ) ) )
   {
      INT32 value = obj.getField( "a" ).Int() ;
      if( 1 != value )
      {
         rc = SDB_TEST_ERROR ;
         CHECK_RC( SDB_OK, rc, "wrong document value " + value ) ;
      }
      ++actDocNum;
   }
	CHECK_RC( SDB_DMS_EOC, rc, "fail to get next" ) ;
   rc = SDB_OK ;
   CHECK_RC( expDocNum, actDocNum, "wrong docNum" ) ;
done:
	return rc ;
error:
	goto done ;
}

TEST_F( invalidFlag15381, test )
{
	INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj obj ;
   // dft: short of default
	BSONObj dftCond   = _sdbStaticObject ;
	BSONObj dftSelect = _sdbStaticObject ;
	BSONObj dftOrder  = _sdbStaticObject ;
	BSONObj dftHint   = _sdbStaticObject ;
   INT64 dftSkipNum  = 0 ;
   INT64 dftReturnNum= -1 ;

   INT32 invalidFlag = 0x00001000 ;
	BSONObj update = BSON( "$set" << BSON( "a" << 1 ) ) ;

   rc = cl.query( cursor, dftCond, dftSelect, dftOrder, dftHint, dftSkipNum, invalidFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = checkDocs( cursor, docNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.queryOne( obj, dftCond, dftSelect, dftOrder, dftHint, dftSkipNum, invalidFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.queryAndUpdate( cursor, update, dftCond, dftSelect, dftOrder, dftHint, 
                           dftSkipNum, dftReturnNum, invalidFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = checkDocs( cursor, docNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.queryAndRemove( cursor, dftCond, dftSelect, dftOrder, dftHint, dftSkipNum, invalidFlag ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = checkDocs( cursor, docNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
