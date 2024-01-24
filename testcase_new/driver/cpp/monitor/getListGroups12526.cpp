/**************************************************************
 * @Description: get all kind of list (groups)
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

class getListGroups12526 : public testBase 
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

TEST_F( getListGroups12526, listGroups )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_GROUPS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "Group" ) ) ;
}
