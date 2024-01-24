/**************************************************************************
 * @Description:   test lob lock lockAndSeek when create read mode
 *                 seqDB-14123:lock加锁创建、读取lob
 *                 SEQUOIADBMAINSTREAM-2889
 * @Modify:        Liang xuewang
 *                 2017-01-17
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobLockCR14123 : public testBase
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
      csName = "lobLockCRCs14123" ;
      clName = "lobLockCRCl14123" ;
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

TEST_F( lobLockCR14123, lockLob )
{
   INT32 rc = SDB_OK ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;

   // create, lock, write, close lob
   const CHAR* writeBuf1 = "123456789ABCDEabcde" ;
   UINT32 len1 = strlen( writeBuf1 ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with createonly mode" ;
   rc = sdbLockLob( lob, len1, 20 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = sdbWriteLob( lob, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ; 

   // open, lock, read, close lob
   UINT32 len = len1 + 1 ;
   CHAR* readBuf = (CHAR*)malloc( len ) ;
   memset( readBuf, 0, len ) ;
   UINT32 readLen ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   rc = sdbLockLob( lob, len1, 20 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = sdbReadLob( lob, len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len1, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( writeBuf1, readBuf ) << "fail to check readBuf" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   free( readBuf ) ;
}

TEST_F( lobLockCR14123, lockAndSeekLob )
{
   INT32 rc = SDB_OK ;
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;

   // create, lockAndSeek, write, close lob
   // after write, lob content: "----------ABCDEabcde",random content before ABCDEabcde
   const CHAR* writeBuf1 = "ABCDEabcde" ;
   UINT32 len1 = strlen( writeBuf1 ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with createonly mode" ;
   rc = sdbLockAndSeekLob( lob, len1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeekLob" ;
   rc = sdbWriteLob( lob, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   SINT64 size ;
   rc = getLobSize( cl, oid, &size ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 2*len1, size ) << "fail to check lobSize" ;

   // open, lockAndSeek, read, close lob
   // after read, readBuf content: "ABCDEabcde"
   CHAR* readBuf = (CHAR*)malloc( len1+1 ) ;
   memset( readBuf, 0, len1+1 ) ;
   UINT32 readLen ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   rc = sdbLockAndSeekLob( lob, len1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeekLob" ;
   rc = sdbReadLob( lob, len1, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len1, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( writeBuf1, readBuf ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   free( readBuf ) ;
}
