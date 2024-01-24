/**************************************************************
 * @Description: get all kind of snapshot about transaction
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

class getSnapshotTrans12662 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;

   sdbCursor cursor ;
   BSONObj res ;

   void SetUp()
   {
      testBase::SetUp() ;

      pCsName = "getSnapshotTrans12662" ;
      pClName = "getSnapshotTrans12662" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      // create cs, cl
      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      // make current transaction
      rc = db.transactionBegin() ;
      ASSERT_EQ( SDB_OK, rc ) ;
      BSONObj doc = BSON( "a" << 1 ) ;
      rc = cl.insert( doc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }


   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.transactionCommit() ;
         ASSERT_EQ( SDB_OK, rc ) ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }

      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;

      testBase::TearDown() ;
   }
} ;

TEST_F( getSnapshotTrans12662, snapshotTransactions )
{
   INT32 rc = SDB_OK ;
   rc = db.getSnapshot( cursor, SDB_SNAP_TRANSACTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( getSnapshotTrans12662, snapshotTransactionsCurrent )
{
   INT32 rc = SDB_OK ;
   rc = db.getSnapshot( cursor, SDB_SNAP_TRANSACTIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
}
