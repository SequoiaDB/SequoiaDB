/*******************************************************************************
 * @Description:    test case for C++ driver
 *                  cursor operation after closeAllCursors
 *                  seqDB-12687:执行closeAllCursors后执行游标操作
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

class cursorTest12687 : public testBase
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
      csName = "testCursorCs12687" ;
      clName = "testCursorCl12687" ;
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

TEST_F( cursorTest12687, oprAfterCloseAllCursors12687 )
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

   // get next current before closeAllCursors 
   BSONObj obj ;
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get current" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;

   // closeAllCursors
   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close all cursors" ;

   // get next current after closeAllCursors
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test get current after closeAllCursors" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test get next after closeAllCursors" ;

   // close cursor after closeAllCursors
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test close after closeAllCursors" ;
}

