/**************************************************************
 * @Description: opreate lob object after disconnect
 *               seqDB-12745 : opreate lob object after disconnect
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

#define BUF_LEN 128

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class opLob12745 : public testBase
{
protected:
   const char *pCsName ;
   const char *pClName ;
   sdbCollection cl ;
   sdbLob wLob ;

   void SetUp()
   {
      testBase::SetUp() ;

      pCsName = "opLob12745" ;
      pClName = "opLob12745" ;
      sdbCollectionSpace cs ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
      rc = cl.createLob( wLob ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create wLob" ;

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

   INT32 createAndWriteALob( sdbLob &lob, OID &oid )
   {
      INT32 rc = SDB_OK ;
      CHAR buf[BUF_LEN] ;

      rc = cl.createLob( lob ) ;
      CHECK_RC( SDB_OK, rc, "fail to create lob" ) ;

      rc = lob.write( buf, BUF_LEN ) ; CHECK_RC( SDB_OK, rc, "fail to write lob" ) ; 
      rc = lob.getOid( oid ) ;
      CHECK_RC( SDB_OK, rc, "fail to get lob oid" ) ;
      rc = lob.close() ;
      CHECK_RC( SDB_OK, rc, "fail to close lob" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( opLob12745, opLob )
{
   // test all interfaces of class sdbLob except close(), isClosed(), getOid(), getSize(), getCreateTime()
   // read() and seek() would succeed because when openLob() is executed, lob is in cache.
   // if call read()/seek() by wLob, rc would be -6.
   // in the order of c++ api doc

   INT32 rc = SDB_OK ;
   CHAR buf[BUF_LEN] ;
   rc = wLob.write( buf, BUF_LEN ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "write lob shouldn't succeed" ;
}
