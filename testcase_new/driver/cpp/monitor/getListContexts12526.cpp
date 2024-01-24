/**************************************************************
 * @Description: get all kind of list about contexts
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

class getListContext12526 : public testBase 
{
protected:
   sdbCursor cursor ;
   BSONObj res ;

   void SetUp()
   {
      testBase::SetUp() ;

      // make current contexts
      INT32 rc = SDB_OK ;
      rc = db.getSnapshot( cursor, SDB_SNAP_CONTEXTS ) ;
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

TEST_F( getListContext12526, listContexts )
{
   // get list
   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_CONTEXTS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check list result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "Contexts" ) ) ;
}

TEST_F( getListContext12526, listContextsCurrent )
{
   // get list
   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_CONTEXTS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check list result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "Contexts" ) ) ; 
}
