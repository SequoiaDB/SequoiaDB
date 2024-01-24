/**************************************************************************
 * @Description :  test lob lock operation
 *                 seqDB-13431:锁定多个数据范围有交集，写入lob
 *                 SEQUOIADBMAINSTREAM-2889  
 * @Modify      :  Liang xuewang
 *                 2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobMultiLock13431 : public testBase
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
      csName = "lobMultiLockCs13431" ;
      clName = "lobMultiLockCl13431" ;
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

TEST_F( lobMultiLock13431, multiLock )
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

   // lock and write lob 
   const CHAR* writeBuf = "12345ABCDE" ;
   const UINT32 len = strlen( writeBuf ) ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;
   rc = sdbLockLob( lob, 0, 0.8*len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = sdbLockLob( lob, 0.2*len, 0.8*len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = sdbWriteLob( lob, writeBuf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob and check
   CHAR readBuf[ len+1 ] ;
   memset( readBuf, 0, len+1 ) ;
   UINT32 readLen = 0 ;
   rc = readLob( cl, oid, readBuf, len, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( writeBuf, readBuf ) << "fail to check readBuf" ;
}
