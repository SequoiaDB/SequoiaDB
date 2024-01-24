/***************************************************
 * @Description : test case of snapshot AccessPlan 
 *                seqDB-14514:获取访问计划快照
 * @Modify      : Liang xuewang
 *                2018-02-22
 ***************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class snapshotPlanTest14514 : public testBase
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
      csName = "snapshotPlanTestCs14514" ;
      clName = "snapshotPlanTestCl14514" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = cs.createCollection( clName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
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

TEST_F( snapshotPlanTest14514, AccessPlan )
{
   INT32 rc = SDB_OK ;

   BSONObj doc = BSON( "a" << 10 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ; 
   
   BSONObj cond = BSON( "a" << 10 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   CHAR clFullName[ 2*MAX_NAME_SIZE+2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;
   cond = BSON( "Collection" << clFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_ACCESSPLANS, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot AccessPlan" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   cout << obj << endl ;

   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
