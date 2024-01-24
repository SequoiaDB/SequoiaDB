/**************************************************************
 * @Description: opreate cs object after disconnect
 *               seqDB-12740 : opreate cs object after disconnect
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

class opCS12740 : public testBase 
{
protected:
   sdbCollectionSpace cs ;
   const CHAR *pCsName ;

   void SetUp() 
   {
      testBase::SetUp() ;

      pCsName = "cs12740" ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      db.disconnect() ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( opCS12740, opCS )
{
   // test all interfaces of class sdbCollectionSpace except getCSName(), create(), drop()
   // in the order of c++ api doc

   INT32 rc = SDB_OK ;
   const CHAR *pClName = "cl12740" ;
   sdbCollection cl ;
   rc = cs.getCollection( pClName, cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get cl shouldn't succeed" ;
   BSONObj option ;
   rc = cs.createCollection( pClName, option, cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cl(1) shouldn't succeed" ;
   rc = cs.createCollection( pClName, cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cl(2) shouldn't succeed" ;
   rc = cs.dropCollection( pClName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "drop cl shouldn't succeed" ;
}

TEST_F( opCS12740, opCS1 )
{
   INT32 rc = SDB_OK ;

   // renameCollection
   const CHAR* oldClName = "testCl" ;
   const CHAR* newClName = "testCl_1" ;
   rc = cs.renameCollection( oldClName, newClName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "renameCollection shouldn't succeed" ;
}
