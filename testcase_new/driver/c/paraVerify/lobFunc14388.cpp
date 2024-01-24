/**************************************************************
 * @Description: test lob func para
 *               seqDB-14338:write/read/closeLob参数校验
 *               seqDB-14339:lock/seek/lockAndSeek参数校验
 *               seqDB-14340:getLobSize/CreateTime/modTime参数校验
 * @Modify:      Liang xuewang Init
 *               2018-02-02
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobFuncTest14388 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   bson_oid_t oid ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;

      csName = "lobFuncTestCs14388" ;
      clName = "lobFuncTestCl14388" ;
      bson_oid_gen( &oid ) ;
      sdbLobHandle lob = SDB_INVALID_HANDLE ;
      const CHAR* buf = "ABCDEabcde" ;
      UINT32 len = strlen( buf ) ;

      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
      rc = sdbWriteLob( lob, buf, len ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
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

TEST_F( lobFuncTest14388, getSize )
{
   INT32 rc = SDB_OK ;

   SINT64 size ;
   rc = sdbGetLobSize( NULL, &size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobSize( SDB_INVALID_HANDLE, &size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobSize( cs, &size ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetLobSize( lob, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( lobFuncTest14388, getCreateTime )
{
   INT32 rc = SDB_OK ;

   UINT64 millis ;
   rc = sdbGetLobCreateTime( NULL, &millis ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobCreateTime( SDB_INVALID_HANDLE, &millis ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobCreateTime( cs, &millis ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetLobCreateTime( lob, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( lobFuncTest14388, getModTime )
{
   INT32 rc = SDB_OK ;

   UINT64 millis ;
   rc = sdbGetLobModificationTime( NULL, &millis ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobModificationTime( SDB_INVALID_HANDLE, &millis ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobModificationTime( cs, &millis ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetLobModificationTime( lob, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
