/**************************************************************
 * @Description: get all kind of snapshot(catalog)
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

class getSnapshotCata12662 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;

   sdbCursor cursor ;
   BSONObj res ;

   void SetUp() 
   {
      testBase::SetUp() ;

      pCsName = "getSnapshotCata12662" ;
      pClName = "getSnapshotCata12662" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown() 
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }

      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;

      testBase::TearDown() ;
   }
} ;

TEST_F( getSnapshotCata12662, snapshotCollections )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ;
      return ;
   }

   // get snapshot
   INT32 rc = SDB_OK ;
   string clFullNameStr = string( pCsName ) + "." + string( pClName ) ;
   BSONObj cond = BSON( "Name" << clFullNameStr.c_str() ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // check result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( res.hasField( "CataInfo" ) ) ;
}
