/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12544:queryAndRemove查询并删除
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

class queryAndRemoveTest12544 : public testBase
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
      csName = "queryAndRemoveTestCs12544" ;
      clName = "queryAndRemoveTestCl12544" ;
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

TEST_F( queryAndRemoveTest12544, queryAndRemove12544 )
{
   INT32 rc = SDB_OK ;

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   // queryAndRemove
   BSONObj cond = BSON( "a" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.queryAndRemove( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to queryAndRemove" ;

   // query before cursor next
   sdbCursor cursor1 ;
   BSONObj obj ;
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to get next" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor1" ;

   // cursor next
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a value" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;  

   // query after cursor next
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to test query after queryAndRemove" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor1" ;
}
