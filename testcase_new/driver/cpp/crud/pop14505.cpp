/***************************************************
 * @Description : test case of pop 
 *                seqDB-14505:pop固定集合的纪录
 * @Modify      : Liang xuewang
 *                2018-02-22
 ***************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace std ;
using namespace bson ;
using namespace sdbclient ;

class popTest14505 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   INT32 logicalID1 ;
   INT32 logicalID2 ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "popTestCs14505" ;
      clName = "popTestCl14505" ;

      // create Capped cs
      BSONObj csOption = BSON( "Capped" << true ) ;
      rc = db.createCollectionSpace( csName, csOption, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

      // create Capped cl
      BSONObj clOption = BSON( "Capped" << true << "Size" << 1024 <<
                               "AutoIndexId" << false ) ;
      rc = cs.createCollection( clName, clOption, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

      // insert 2 doc and get logicalID1 logicalID2
      BSONObj doc = BSON( "a" << 0 ) ;
      rc = cl.insert( doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
      doc = BSON( "a" << 1 ) ;
      rc = cl.insert( doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

      sdbCursor cursor ;   
      BSONObj sort = BSON( "_id" << 1 ) ;
      rc = cl.query( cursor, _sdbStaticObject, _sdbStaticObject, sort ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
      BSONObj obj ;
      rc = cursor.next( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
      cout << obj << endl ;
      logicalID1 = obj.getField( "_id" ).Long() ;

      rc = cursor.next( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
      cout << obj << endl ; 
      logicalID2 = obj.getField( "_id" ).Long() ;

      cout << "logicalID1: " << logicalID1 << ", logicalID2: " << logicalID2 << endl ;

      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
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

TEST_F( popTest14505, directionNegative )
{
   INT32 rc = SDB_OK ;

   BSONObj option = BSON( "LogicalID" << logicalID1 << 
                          "Direction" << -1 ) ;
   rc = cl.pop( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to pop" ;

   // |--doc1--|--doc2--| 
   // LogicalID points to doc1 
   // Direction -1 means pop doc1 and docs after doc1, pop doc1 and doc2
   // so cl will have no doc
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 0, count ) << "fail to check count" ;
}

TEST_F( popTest14505, directionPositive )
{
   INT32 rc = SDB_OK ;

   BSONObj option = BSON( "LogicalID" << logicalID2 <<
                          "Direction" << 1 ) ;
   rc = cl.pop( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to pop" ;

   // |--doc1--|--doc2--| 
   // LogicalID points to doc2 
   // Direction 1 means pop doc2 and docs before doc2, pop doc2 and doc1
   // so cl will have no doc
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 0, count ) << "fail to check count" ;
}
