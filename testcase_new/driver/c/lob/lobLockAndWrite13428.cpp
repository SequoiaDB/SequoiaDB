/**************************************************************************
 * @Description:   test lob seek write operation
 *                 seqDB-13428:lock加锁写lob
 *                 SEQUOIADBMAINSTREAM-2889
 * @Modify:        Liang xuewang
 *                 2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobLockAndWrite13428 : public testBase
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
      csName = "lobLockAndWriteCs13428" ;
      clName = "lobLockAndWriteCl13428" ;
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

TEST_F( lobLockAndWrite13428, lockWrite )
{
   INT32 rc = SDB_OK ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;

   // create, close lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with createonly mode" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ; 

   // open, lock, seek, write, close lob
   const CHAR* writeBuf1 = "123456789ABCDEabcde" ;
   UINT32 len1 = strlen( writeBuf1 ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;
   rc = sdbLockLob( lob, 0, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = sdbWriteLob( lob, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ; 

   // open, lockAndSeek, write, close lob
   const CHAR* writeBuf2 = "987654321" ;
   UINT32 len2 = strlen( writeBuf2 ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;
   rc = sdbLockAndSeekLob( lob, len1, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock and seek lob" ;
   rc = sdbWriteLob( lob, writeBuf2, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob
   UINT32 len = len1 + len2 + 1 ;
   CHAR* readBuf = (CHAR*)malloc( len ) ;
   memset( readBuf, 0, len ) ;
   UINT32 readLen = 0 ;
   rc = readLob( cl, oid, readBuf, len, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len-1, readLen ) << "fail to check readLen" ;
   CHAR* expBuf = (CHAR*)malloc( len ) ;
   memset( expBuf, 0, len ) ;
   sprintf( expBuf, "%s%s", writeBuf1, writeBuf2 ) ;
   ASSERT_STREQ( expBuf, readBuf ) << "fail to check readBuf" ;

   free( readBuf ) ;
   free( expBuf ) ;
}
