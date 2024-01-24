/**************************************************************************
 * @Description:   test lob seek operation and seekWrite operation 
 *                 seqDB-13425:seek偏移创建、写、读lob
 *                 seqDB-13426:write模式下未加锁写lob
 * @Modify:        Liang xuewang
 *                 2017-12-13
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobSeekAndWrite13425_13426 : public testBase
{
protected:
   const CHAR *csName ;
   const CHAR *clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      csName = "lobSeekAndWriteCs13425" ;
      clName = "lobSeekAndWriteCl13425" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( lobSeekAndWrite13425_13426, lobSeekAndWrite )
{
   INT32 rc = SDB_OK ;

   // open lob with createonly mode, seek write lob
   sdbLobHandle lob ;
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with create mode" ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   const CHAR* writeBuf = "123456789ABCDEabcde" ;
   UINT32 len = strlen( writeBuf ) ;
   rc = sdbWriteLob( lob, writeBuf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // open lob with write mode, seek, write lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with write mode" ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_END ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = sdbWriteLob( lob, writeBuf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob without seek
   CHAR* readBuf = (CHAR*)malloc( len*2+1 ) ;
   memset( readBuf, 0, len*2+1 ) ;
   UINT32 readLen = 0 ; 
   rc = readLob( cl, oid, readBuf, len*2, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len*2, readLen ) << "fail to check readLen" ;
   CHAR expBuf[len*2+1] ;
   memset( expBuf, 0, len*2+1 ) ;
   sprintf( expBuf, "%s%s", writeBuf, writeBuf ) ;
   ASSERT_STREQ( expBuf, readBuf ) << "fail to check readBuf" ;

   // read lob with seek
   memset( readBuf, 0, len*2+1 ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   rc = sdbSeekLob( lob, len, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = sdbReadLob( lob, len*2, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( writeBuf, readBuf ) << "fail to check readBuf" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   free( readBuf ) ;
}
