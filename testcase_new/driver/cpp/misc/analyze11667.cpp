/**************************************************
 * @Description: test case for c++ driver
 *               seqDB-11667:测试统计信息收集
 * @Modify:      Liangxw
 *               2017-09-22
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

class analyzeTest11667 : public testBase
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
      csName = "analyzeTestCs11667" ;
      clName = "analyzeTestCl11667" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( rc, SDB_OK ) << "fail to create cs " << csName << " cl " << clName ; 
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

// explain query { a:100 }
INT32 explainDoc( sdbCollection& cl, string& scanType )
{
	INT32 rc = SDB_OK ;
	sdbCursor cursor ;
	BSONObj cond = BSON( "a" << 100 ) ;
	BSONObj obj ;
	rc = cl.explain( cursor, cond ) ;
	CHECK_RC( SDB_OK, rc, "fail to explain" ) ;
	rc = cursor.next( obj ) ;
	CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
	scanType = obj.getField( "ScanType" ).String() ;
done:
	return rc ;
error:
	goto done ;
}

TEST_F( analyzeTest11667, explain11667 )
{
	INT32 rc = SDB_OK ;
	string scanType ;
	
	// create index
	BSONObj indexDef = BSON( "a" << 1 ) ;
	rc = cl.createIndex( indexDef, "aIndex", false, false ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;

	// insert 10000 docs { a:100, b:98765 }
	vector<BSONObj> docs ;
	for( INT32 i = 0;i < 10000;i++ )
	{
		BSONObj doc = BSON( "a" << 100 << "b" << "98765" ) ;
		docs.push_back( doc ) ;
	}
	rc = cl.bulkInsert( 0, docs ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to bulk insert docs" ;

	// explain query { a:100 } before analyze
	rc = explainDoc( cl, scanType ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( "ixscan", scanType ) << "fail to check scanType before analyze" ;

	// analyze
	BSONObj option = BSON( "CollectionSpace" << csName ) ;
	rc = db.analyze( option ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to analyze" ;

	// explain query { a:100 }
	rc = explainDoc( cl, scanType ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( "tbscan", scanType ) << "fail to check scanType after analyze" ;
}
