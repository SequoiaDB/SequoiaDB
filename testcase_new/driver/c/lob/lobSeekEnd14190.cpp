/**************************************************************************
 * @Description :   test lob lock operation
 *                  seqDB-14190:指定SDB_LOB_SEEK_END偏移创建、写lob
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobSeekEnd14190 : public testBase
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
      csName = "lobSeekEndCs14190" ;
      clName = "lobSeekEndCl14190" ;
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

TEST_F( lobSeekEnd14190, createonlyMode )
{
   INT32 rc = SDB_OK ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with createonly mode" ;
   
   // write buf "ABCDE12345" to lob
   const CHAR* writeBuf1 = "ABCDE12345" ;
   UINT32 len1 = strlen( writeBuf1 ) ;
   rc = sdbWriteLob( lob, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // seek end, then write buf "54321" to lob
   // after write, lob content: "ABCDE54321"
   const CHAR* writeBuf2 = "54321" ;
   UINT32 len2 = strlen( writeBuf2 ) ;
   rc = sdbSeekLob( lob, len2, SDB_LOB_SEEK_END ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seekLob" ;
   rc = sdbWriteLob( lob, writeBuf2, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // check lob size
   SINT64 size ;
   rc = getLobSize( cl, oid, &size ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len1, size ) << "fail to check lob size" ;

   // check lob content
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with read mode" ;
   CHAR* readBuf = (CHAR*)malloc( len1+1 ) ;
   memset( readBuf, 0, len1+1 ) ;
   UINT32 readLen ;
   rc = sdbReadLob( lob, len1, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len1, readLen ) << "fail to check readLen" ;
   const CHAR* expBuf = "ABCDE54321" ;
   ASSERT_STREQ( expBuf, readBuf ) << "fail to check readBuf" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobSeekEnd14190, writeMode )
{
   INT32 rc = SDB_OK ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   const CHAR* writeBuf1 = "ABCDE12345" ;
   UINT32 len1 = strlen( writeBuf1 ) ;
   rc = createLob( cl, oid, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with write mode" ;

   const CHAR* writeBuf2 = "54321" ;
   UINT32 len2 = strlen( writeBuf2 ) ;
   rc = sdbSeekLob( lob, len2, SDB_LOB_SEEK_END ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seekLob" ;
   rc = sdbWriteLob( lob, writeBuf2, len2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // check lob size
   SINT64 size ;
   rc = getLobSize( cl, oid, &size ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len1, size ) << "fail to check lob size" ;

   // check lob content
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with read mode" ;
   CHAR* readBuf = (CHAR*)malloc( len1+1 ) ;
   memset( readBuf, 0, len1+1 ) ;
   UINT32 readLen ;
   rc = sdbReadLob( lob, len1, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len1, readLen ) << "fail to check readLen" ;
   const CHAR* expBuf = "ABCDE54321" ;
   ASSERT_STREQ( expBuf, readBuf ) << "fail to check readBuf" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
