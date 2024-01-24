/**************************************************************
 * @Description: opreate cursor object after disconnect
 *               seqDB-12742 : opreate cursor object after disconnect
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

class opCursor12742 : public testBase
{
protected:
   const char *pCsName ;
   const char *pClName ;
   sdbCollection cl ;
   sdbCursor cursor ;

   void SetUp()
   {
      testBase::SetUp() ;

      pCsName = "cs12742" ;
      pClName = "cl12742" ;
      sdbCollectionSpace cs ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
      rc = cl.query( cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get cursor" ;
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

TEST_F( opCursor12742, opCursor )
{
   // test all interfaces of class sdbCursor except close()
   // in the order of c++ api doc
   
   INT32 rc = SDB_OK ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   EXPECT_EQ( SDB_DMS_EOC, rc ) << "get next shouldn't succeed" ;
   rc = cursor.current( obj ) ;
   EXPECT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "get current shouldn't succeed" ;
}
