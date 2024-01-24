/**************************************************************
 * @Description: test case for C++ driver
 *				     seqDB-14682:指定hint执行getCount
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-13
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace bson ;
using namespace sdbclient ;
using namespace std ;

class getCountTest : public testBase
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
      csName = "getCountTestCs14682" ;
      clName = "getCountTestCl14682" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( clName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      testBase::TearDown() ;
   }
} ;

TEST_F( getCountTest, withHint )
{
   INT32 rc = SDB_OK ;
   
   BSONObj idxDef = BSON( "a" << 1 ) ;
   const CHAR* idxName = "aIndex" ;
   rc = cl.createIndex( idxDef, idxName, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;

   for( INT32 i = 0;i < 10;i++ )
   {
      BSONObj doc = BSON( "a" << i ) ;
      rc = cl.insert( doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   }

   BSONObj cond = BSON( "a" << BSON( "$lt" << 6 ) ) ;
   BSONObj hint = BSON( "a" << "" ) ;
   SINT64 count ;
   rc = cl.getCount( count, cond, hint ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 6, count ) << "fail to check count" ;
}
