/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12518:聚集操作
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

class aggregateTest12518 : public testBase
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
      csName = "aggregateTestCs12518" ;
      clName = "aggregateTestCl12518" ;
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

TEST_F( aggregateTest12518, aggregate12518 )
{
   INT32 rc = SDB_OK ;

   BSONObj doc = BSON( "no" << "20092320" << "name" << "lxw" << "major" << "biology" ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   vector<BSONObj> vec ;
   BSONObj match = BSON( "$match" << BSON( "name" << "lxw" ) ) ;
   BSONObj project = BSON( "$project" << BSON( "no" << 1 << "major" << 1 ) ) ;
   vec.push_back( match ) ;
   vec.push_back( project ) ;
   sdbCursor cursor ;
   rc = cl.aggregate( cursor, vec ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to aggregate" ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "20092320", obj.getField( "no" ).String() ) << "fail to check no" ; 
   ASSERT_EQ( "biology", obj.getField( "major" ).String() ) << "fail to check major" ;
   ASSERT_TRUE( obj.getField( "name" ).eoo() ) << "fail to check name" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
