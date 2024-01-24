/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12520:执行事务操作并提交
 *                 seqDB-12675:执行事务操作并回滚
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

class transTest12520 : public testBase
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
      csName = "transTestCs12520" ;
      clName = "transTestCl12520" ;
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

TEST_F( transTest12520, transCommit12520 )
{
   INT32 rc = SDB_OK ;
   
   BSONObj doc = BSON( "a" << 1 ) ;
   
   rc = db.transactionBegin() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to begin trans" ;

   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   rc = db.transactionCommit() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to commit trans" ;

   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
   sdbCursor cursor ;
   BSONObj cond = BSON( "a" << 1 ) ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( transTest12520, transRollback12675 )
{
   INT32 rc = SDB_OK ;
   
   BSONObj doc = BSON( "a" << 1 ) ;
   
   rc = db.transactionBegin() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to begin trans" ;

   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   rc = db.transactionRollback() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rollback trans" ;

   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 0, count ) << "fail to check count" ;
}
