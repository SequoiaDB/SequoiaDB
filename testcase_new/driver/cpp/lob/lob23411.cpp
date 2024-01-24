/**************************************************************************
 * @Description:   seqDB-23411:以 SHARE_READ、SHARE_READ|WRITE 模式读写 lob
 * @Modify:        Li Yuanyue
 *                 2021-01-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class lob23411 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      pCsName = "lob23411" ;
      pClName = "lob23411" ;

      // create cs, cl
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << pCsName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( lob23411, SDB_LOB_WRITE_And_SDB_LOB_SHAREREAD )
{
   INT32 rc = SDB_OK ;
   sdbLob lob ;
   OID oid ;

   // create
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;

   // get oid 
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;

   // write
   const CHAR* buf = "0123456789ABCDEFabcdef" ;
   UINT32 lobSize = strlen( buf ) ;
   rc = lob.write( buf, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // close 
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // SDB_LOB_WRITE | SDB_LOB_SHAREREAD
   // open lob
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE | SDB_LOB_SHAREREAD ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_WRITE | SDB_LOB_SHAREREAD" ;

   // read
   CHAR readBuf[100] = { 0 } ;
   UINT32 len = 10 ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check lob read len" ;
   ASSERT_STREQ( "0123456789", readBuf ) << "fail to check read buf" ;

   // write
   const CHAR* helloBuf = "hello" ;
   UINT32 helloLen = strlen( helloBuf ) ;
   rc = lob.write( helloBuf, helloLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // close 
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // SDB_LOB_WRITE 
   // open
   rc = cl.openLob( lob, oid, SDB_LOB_SHAREREAD ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with  SDB_LOB_SHAREREAD" ;

   // read
   CHAR readBuf2[100] = { 0 } ;
   len = 10 + 5 ;
   rc = lob.read( len, readBuf2, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_STREQ( "0123456789hello", readBuf2 ) << "fail to check read buf" ;

   // close 
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}