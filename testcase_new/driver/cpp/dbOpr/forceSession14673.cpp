/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14673:forceSession终止会话
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-12
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class forceSessionTest14673 : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( forceSessionTest14673, forceSession )
{
   INT32 rc = SDB_OK ;
   
   sdbCursor cursor ;
   BSONObj cond = BSON( "Global" << false ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_SESSIONS_CURRENT, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSnapshot" ;
   
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   SINT64 sessionId = obj.getField( "SessionID" ).numberLong() ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   
   rc = db.forceSession( sessionId ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to forceSession" ;
}
