/*******************************************************************************
 * @Description:    test case for C++ driver data cursor operation after close
 *                  seqDB-12685:游标关闭后执行游标操作（next current）
 *                  seqDB-12686:重复关闭游标
 * @Modify:         Liang xuewang Init
 *                  2017-09-12
 *******************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class cursorTest12685 : public testBase
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
      csName = "testCursorCs12685" ;
      clName = "testCursorCl12685" ;
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

// 测试关闭游标后执行操作，重复关闭游标( 12685 12686 )
TEST_F( cursorTest12685, oprAfterClose12685 )
{
   INT32 rc = SDB_OK ;

   // insert docs
   vector<BSONObj> docs ;
   INT32 docNum = 100 ;
   BSONObj doc ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      doc = BSON( "a" << i ) ;
      docs.push_back( doc ) ;
   }
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulk insert" ;

   // query
   BSONObj cond = BSON( "a" << BSON( "$gt" << 75 ) ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   // get next current before cursor close
   BSONObj obj ;
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get current" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;

   BSONObj obj1,obj2 ;
   rc = cursor.next( obj1, false ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   
   rc = cursor.current( obj2, false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ ( obj1.getIntField("a"), obj2.getIntField("a")) ;
   cout << obj1.toString() << endl;
   // close cursor
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail close cursor" ;

   // get next current after cursor close
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test get current after cursor close" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test get next after cursor close" ;

   // close again
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test close after cursor close" ;
}
