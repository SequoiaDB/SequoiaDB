/**************************************************************************
 * @Description :   test lob read operation
 *                  seqDB-13433:读取lob中空切片数据
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobReadEmpty13433 : public testBase
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
      csName = "lobReadEmptyCs13433" ;
      clName = "lobReadEmptyCl13433" ;
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
   void printContent( const CHAR content[], INT32 len )
   {
      for( INT32 i = 0;i < len;i++ )
      {
         printf( "%c ", content[i] ) ;
      }
      printf( "\n" ) ;
   }
} ;

TEST_F( lobReadEmpty13433, readEmpty )
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

   // write lob: ABCDE12345-----54321EDCBA"(random content in middle)
   const CHAR* writeBuf1 = "ABCDE12345" ;
   const UINT32 len1 = strlen( writeBuf1 ) ;
   const CHAR* writeBuf2 = "54321EDCBA" ;
   const UINT32 len2 = strlen( writeBuf2 ) ;
   const UINT32 emptyLen = 5 ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;
   rc = sdbLockAndSeekLob( lob, 0, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek lob" ;
   rc = sdbWriteLob( lob, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbLockAndSeekLob( lob, len1 + emptyLen, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek lob" ;
   rc = sdbWriteLob( lob, writeBuf2, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob total empty
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with read mode" ;
   rc = sdbSeekLob( lob, len1, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   UINT32 len = emptyLen + 1 ;
   CHAR readBuf1[ len ] ;
   memset( readBuf1, 0, len ) ;
   UINT32 readLen = 0 ;
   rc = sdbReadLob( lob, emptyLen, readBuf1, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( emptyLen, readLen ) << "fail to check readLen" ;
   CHAR expBuf1[ emptyLen+1 ] ;
   memset( expBuf1, 0, emptyLen+1 ) ;
   printf( "readBuf1: " ) ;
   printContent( readBuf1, len ) ;
   // ASSERT_EQ( SDB_OK, memcmp( expBuf1, readBuf1, len ) ) << "fail to check readBuf1" ;

   // read lob partial empty
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   len = len1 + emptyLen + len2 + 1 ;
   CHAR readBuf2[ len ] ;
   memset( readBuf2, 0, len ) ;
   rc = sdbReadLob( lob, len-1, readBuf2, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len-1, readLen ) << "fail to check readLen" ;
   CHAR expBuf2[ len ] ;
   memset( expBuf2, 0, len ) ;
   memcpy( expBuf2, writeBuf1, len1 ) ;
   memcpy( expBuf2+len1+emptyLen, writeBuf2, len2 ) ;
   printf( "readBuf2: " ) ;
   printContent( readBuf2, len ) ;
   ASSERT_STREQ( expBuf2, readBuf2 ) << "fail to check readBuf2" ;
   ASSERT_STREQ( expBuf2+len1+emptyLen, readBuf2+len1+emptyLen ) << "fail to check readBuf2" ;
   // ASSERT_EQ( SDB_OK, memcmp( expBuf2, readBuf2, len ) ) << "fail to check readBuf2" ;
}
