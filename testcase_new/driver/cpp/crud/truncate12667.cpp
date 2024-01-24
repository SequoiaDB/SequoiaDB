/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12667:测试truncate/getCount
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

class truncateTest12667 : public testBase
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
      csName = "truncateTestCs12667" ;
      clName = "truncateTestCl12667" ;
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

TEST_F( truncateTest12667, truncate12667 )
{
   INT32 rc = SDB_OK ;

   // bulkInsert docs
   vector<BSONObj> docs ;
   INT32 docNum = 10 ;
   BSONObj doc ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      doc = BSON( "a" << i << "b" << "test" ) ;
      docs.push_back( doc ) ;
   }
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulkInsert" ;

   // getCount before truncate
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( docNum, count ) << "fail to check count" ;

   // truncate
   rc = cl.truncate() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncate" ;
   
   // getCount after truncate
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 0, count ) << "fail to check count after truncate" ;
}
