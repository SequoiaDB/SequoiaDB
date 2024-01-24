/**************************************************************
 * @Description: get all kind of list about sessions
 *               seqDB-12526 : get all kind of list
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class getListSession12526 : public testBase 
{
protected:
   sdbCursor cursor ;
   BSONObj res ;

   void SetUp()
   {
      testBase::SetUp() ;

      // make current sessions
      INT32 rc = SDB_OK ;
      rc = db.getSnapshot( cursor, SDB_SNAP_SESSIONS ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;

      testBase::TearDown() ;
   }
} ;

TEST_F( getListSession12526, listSessions )
{
   // get list
   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_SESSIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "TID" ) ) ;
}

TEST_F( getListSession12526, listSessionsCurrent )
{
   // get list
   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_SESSIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "TID" ) ) ;
}
