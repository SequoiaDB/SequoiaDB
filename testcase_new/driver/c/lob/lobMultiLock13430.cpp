/**************************************************************************
 * @Description :   test lob lock operation
 *                  seqDB-13430:锁定多个数据范围后写入lob
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobMultiLock13430 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobMultiLockCs13430" ;
      clName = "lobMultiLockCl13430" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
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

TEST_F( lobMultiLock13430, multiLock )
{
   INT32 rc = SDB_OK ;

   // create lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // multi lock and write lob
   const CHAR* writeBuf1 = "ABCDE" ;
   const UINT32 len1 = strlen( writeBuf1 ) ;
   const CHAR* writeBuf2 = "abcde" ;
   const UINT32 len2 = strlen( writeBuf2 ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;
   rc = sdbLockLob( lob, 0, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = sdbWriteLob( lob, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   
   rc = sdbLockAndSeekLob( lob, len1, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek lob" ;
   rc = sdbWriteLob( lob, writeBuf2, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob
   const INT32 len = len1 + len2 + 1 ;
   CHAR readBuf[ len ] ;
   memset( readBuf, 0, len ) ;
   UINT32 readLen = 0 ;
   rc = readLob( cl, oid, readBuf, len-1, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len-1, readLen ) << "fail to check readLen" ;
   CHAR expBuf[ len ] ;
   memset( expBuf, 0, len ) ;
   sprintf( expBuf, "%s%s", writeBuf1, writeBuf2 ) ;
   ASSERT_STREQ( expBuf, readBuf ) << "fail to check readBuf" ;
}
