/**************************************************************
 * @Description: get all kind of snapshot about contexts
 *               seqDB-12662 : get all kind of snapshot
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

class getSnapshotContext12662 : public testBase 
{
protected:
   sdbCursor cursor ;
   BSONObj res ;

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;

      testBase::TearDown() ;
   }
} ;

TEST_F( getSnapshotContext12662, snapshotContexts )
{
   // snapshot contexts
   INT32 rc = SDB_OK ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CONTEXTS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check snapshot
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "Contexts" ) ) ;

   // snapshot current contexts
   rc = db.getSnapshot( cursor, SDB_SNAP_CONTEXTS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check snapshot
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "Contexts" ) ) ; 
}
