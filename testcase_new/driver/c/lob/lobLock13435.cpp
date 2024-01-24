/**************************************************************************
 * @Description :   test lob lock operation
 *                  seqDB-13435:lock接口参数校验
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobLock13435 : public testBase
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
      csName = "lobLockCs13435" ;
      clName = "lobLockCl13435" ;
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

TEST_F( lobLock13435, lobHandle )
{
   INT32 rc = SDB_OK ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbLockLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test lockLob with invalid lob handle" ;

   rc = sdbLockLob( db, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) << "fail to test lockLob with db handle" ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = sdbLockLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test lockLob with closed lob" ;
}

TEST_F( lobLock13435, offset )
{
   INT32 rc = SDB_OK ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with create mode" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
      
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with write mode" ;
   rc = sdbLockLob( lob, -1, 10 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test lockLob with offset -1" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobLock13435, length )
{
   INT32 rc = SDB_OK ;
   
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with create mode" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with write mode" ;
   
   rc = sdbLockLob( lob, 0, -2 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test lockLob with length -2" ;
   rc = sdbLockLob( lob, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test lockLob with length -1" ;
   rc = sdbLockLob( lob, 0, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test lockLob with length 0" ;
   rc = sdbLockLob( lob, 0, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test lockLob with length 1" ;

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
