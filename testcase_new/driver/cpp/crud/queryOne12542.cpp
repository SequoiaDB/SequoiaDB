/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12542:queryOne查询单条记录（存在/不存在）
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

class queryOneTest12542 : public testBase
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
      csName = "queryOneTestCs12542" ;
      clName = "queryOneTestCl12542" ;
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

TEST_F( queryOneTest12542, queryOne12542 )
{
   INT32 rc = SDB_OK ;

   // bulkInsert docs
   vector<BSONObj> docs ;
   INT32 docNum = 10 ;
   BSONObj doc ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      doc = BSON( "a" << 1 << "b" << i ) ;
      docs.push_back( doc ) ;
   }
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulkInsert" ;
   
   // queryOne exist
   BSONObj cond = BSON( "a" << 1 ) ;
   BSONObj obj ;
   rc = cl.queryOne( obj, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to queryOne" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a" ;
   INT32 bVal = obj.getField( "b" ).Int() ;
   ASSERT_TRUE( bVal >= 0 && bVal < docNum ) << "fail to check b" ;

   // queryOne not exist
   cond = BSON( "a" << 2 ) ;
   rc = cl.queryOne( obj, cond ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to test queryOne not exist" ; 
}
