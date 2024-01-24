/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12663:偏移读lob（覆盖边界值，超出边界）
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
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

class lobSeekTest12663 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   INT32 lobSize ;
   sdbLob lob ;
   OID oid ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobSeekTestCs12663" ;
      clName = "lobSeekTestCl12663" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

      rc = cl.createLob( lob ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
      const CHAR* buf = "0123456789ABCDEFabcdef" ;
      lobSize = strlen( buf ) ;
      rc = lob.write( buf, lobSize ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
      rc = lob.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
      oid = lob.getOid() ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( lobSeekTest12663, lobSeekNormal12663 )
{
   INT32 rc = SDB_OK ;

   // open lob
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   
   // seek lob with SDB_LOB_SEEK_SET
   SINT64 offset = 5 ;
   SDB_LOB_SEEK whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;
   
   // read lob
   UINT32 len = 5 ;
   CHAR readBuf[100] = { 0 } ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check read len" ;
   ASSERT_STREQ( "56789", readBuf ) << "fail to check read buf" ;
   memset( readBuf, 0, 100 ) ;  
 
   // seek lob with SDB_LOB_SEEK_CUR
   whence = SDB_LOB_SEEK_CUR ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;
   
   // read lob 
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check read len" ;
   ASSERT_STREQ( "Fabcd", readBuf ) << "fail to check read buf" ;
   memset( readBuf, 0, 100 ) ;  

   // seek lob with SDB_LOB_SEEK_END
   whence = SDB_LOB_SEEK_END ;
   rc = lob.seek( offset, whence ) ;
   
   // read lob 
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( offset, readLen ) << "fail to check read len" ;
   ASSERT_STREQ( "bcdef", readBuf ) << "fail to check read buf" ;

   // close lob
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobSeekTest12663, lobSeekBoundary12663 )
{
   INT32 rc = SDB_OK ;
 
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;

   // seek at the begining with SDB_LOB_SEEK_SET
   SINT64 offset = 0 ;
   SDB_LOB_SEEK whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   UINT32 len = 10 ;
   CHAR readBuf[100] = { 0 } ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check read len" ;
   ASSERT_STREQ( "0123456789", readBuf ) << "fail to check read buf" ;
   memset( readBuf, 0, 100 ) ;

   // seek at the begining with SDB_LOB_SEEK_CUR
   offset = -10 ;
   whence = SDB_LOB_SEEK_CUR ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check read len" ;
   ASSERT_STREQ( "0123456789", readBuf ) << "fail to check read buf" ;
   memset( readBuf, 0, 100 ) ;

   // seek at the begining with SDB_LOB_SEEK_END
   offset = lobSize ;
   whence = SDB_LOB_SEEK_END ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check read len" ;
   ASSERT_STREQ( "0123456789", readBuf ) << "fail to check read buf" ;
   memset( readBuf, 0, 100 ) ;

   // seek at the end with SDB_LOB_SEEK_SET
   offset = lobSize ;
   whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_EOF, rc ) << "fail to test read at the end" ;
   ASSERT_EQ( 0, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "", readBuf ) << "fail to check readBuf" ;
   
   // seek at the end with SDB_LOB_SEEK_CUR
   offset = 0 ;
   whence = SDB_LOB_SEEK_CUR ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;  
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_EOF, rc ) << "fail to test read at the end" ;
   ASSERT_EQ( 0, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "", readBuf ) << "fail to check readBuf" ;

   // seek at the end with SDB_LOB_SEEK_END
   offset = 0 ;
   whence = SDB_LOB_SEEK_END ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_EOF, rc ) << "fail to test read at the end" ;
   ASSERT_EQ( 0, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "", readBuf ) << "fail to check readBuf" ;
   
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobSeekTest12663, lobSeekOutOfBoundary12663 )
{
   INT32 rc = SDB_OK ;
   
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   
   // seek out of left boundary with SDB_LOB_SEEK_SET
   SINT64 offset = -10 ;
   SDB_LOB_SEEK whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seek out of left boundary" ;
   UINT32 len = 2 ;
   CHAR readBuf[100] = { 0 } ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "01", readBuf ) << "fail to check readBuf" ;
   memset( readBuf, 0, 100 ) ;

   // seek out of left boundary with SDB_LOB_SEEK_CUR
   whence = SDB_LOB_SEEK_CUR ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seek out of left boundary" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "23", readBuf ) << "fail to check readBuf" ;
   memset( readBuf, 0, 100 ) ;

   // seek out of left boundary with SDB_LOB_SEEK_END
   offset = lobSize + 10 ;
   whence = SDB_LOB_SEEK_END ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seek out of left boundary" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "45", readBuf ) << "fail to check readBuf" ;
   memset( readBuf, 0, 100 ) ;

   // seek out of right boundary with SDB_LOB_SEEK_SET 
   whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seek out of right boundary" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "67", readBuf ) << "fail to check readBuf" ;
   memset( readBuf, 0, 100 ) ;

   // seek out of right boundary with SDB_LOB_SEEK_CUR
   whence = SDB_LOB_SEEK_CUR ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seek out of right boundary" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "89", readBuf ) << "fail to check readBuf" ;
   memset( readBuf, 0, 100 ) ;

   // seek out of right boundary with SDB_LOB_SEEK_END
   whence = SDB_LOB_SEEK_END ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seek out of left boundary" ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( "AB", readBuf ) << "fail to check readBuf" ;
   memset( readBuf, 0, 100 ) ;

   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
