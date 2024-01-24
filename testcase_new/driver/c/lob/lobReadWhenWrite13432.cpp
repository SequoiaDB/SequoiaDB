/**************************************************************************
 * @Description :   test lob seek read write operation
 *                  seqDB-13432:加锁写入过程中读取lob
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobReadWhenWrite13432 : public testBase
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
      csName = "lobReadWhenWriteCs13432" ;
      clName = "lobReadWhenWriteCl13432" ;
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

TEST_F( lobReadWhenWrite13432, readWhenWrite )
{
   INT32 rc = SDB_OK ;

   // create lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle wlob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   rc = sdbCloseLob( &wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // seek and write lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;
   const CHAR* writeBuf = "ABCDE" ;
   const UINT32 len = strlen( writeBuf ) ;
   rc = sdbLockAndSeekLob( wlob, 0, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek lob" ;
   rc = sdbWriteLob( wlob, writeBuf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // read lob when write lob not closed
   sdbLobHandle rlob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &rlob ) ;
   ASSERT_EQ( SDB_LOB_IS_IN_USE, rc ) << "fail to test read lob when write" ;
   rc = sdbCloseLob( &wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // check lob
   CHAR readBuf[ len+1 ] ;
   memset( readBuf, 0, len+1 ) ;
   UINT32 readLen = 0 ;
   rc = readLob( cl, oid, readBuf, len, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( writeBuf, readBuf ) << "fail to check readBuf" ;  
}
