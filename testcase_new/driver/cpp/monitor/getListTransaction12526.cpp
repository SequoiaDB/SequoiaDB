/**************************************************************
 * @Description: get all kind of list about transaction
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

class getListTransaction12526 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;

   sdbCursor cursor ;
   BSONObj res ;

   void SetUp()
   {
      testBase::SetUp() ;

      pCsName = "getListTransaction12526" ;
      pClName = "getListTransaction12526" ;
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

TEST_F( getListTransaction12526, listTransactions )
{
   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_TRANSACTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( getListTransaction12526, listTransactionsCurrent )
{
   INT32 rc = SDB_OK ;
   rc = db.getList( cursor, SDB_LIST_TRANSACTIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
}
