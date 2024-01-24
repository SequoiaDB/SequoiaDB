/**************************************************************************
 * @Description :   test lob getModificationTime operation
 *                  when open lob with createonly mode or write mode,
 *                       modTime is updated after close;
 *                  when open lob with read mode, 
 *                       modTime won't change after close.
 *                  seqDB-13429:获取lob修改时间
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobModTime13429 : public testBase
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
      csName = "lobModTimeCs13429" ;
      clName = "lobModTimeCl13429" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
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

TEST_F( lobModTime13429, ModifyTime )
{
   INT32 rc = SDB_OK ;

   // open lob with createonly mode, before close, modTime = createTime
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;

   UINT64 createTime1 ;
   UINT64 modTime1 ;
   rc = sdbGetLobCreateTime( lob, &createTime1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getCreateTime" ;
   rc = sdbGetLobModificationTime( lob, &modTime1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_EQ( createTime1, modTime1 ) << "fail to check modTime before close" ;

   // after close, modTime is updated, createTime still the same
   rc = sdbCloseLob( &lob ) ;                                         
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ;

   UINT64 createTime2 ;
   UINT64 modTime2 ;
   rc = sdbGetLobCreateTime( lob, &createTime2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getCreateTime" ;
   rc = sdbGetLobModificationTime( lob, &modTime2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_EQ( createTime1, createTime2 ) << "fail to check createTime after close" ;
   ASSERT_LT( createTime1, modTime2 ) << "fail to check modTime after close" ;

   // open lob won't change modTime
   sdbLobHandle lob1 ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE, &lob1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with write mode" ; 
   UINT64 modTime2_1 ;
   rc = sdbGetLobModificationTime( lob1, &modTime2_1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_EQ( modTime2, modTime2_1 ) << "fail to check open won't change modTime" ;
   rc = sdbCloseLob( &lob1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // before close, write lob won't change the modTime
   const CHAR* writeBuf = "ABCDE" ;
   UINT32 len = strlen( writeBuf ) ;
   rc = sdbWriteLob( lob, writeBuf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   UINT64 modTime3 ;
   rc = sdbGetLobModificationTime( lob, &modTime3 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_EQ( modTime2, modTime3 ) << "fail to check modTime after write" ;

   // after close, modTime is updated
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   UINT64 modTime4 ;
   rc = sdbGetLobModificationTime( lob, &modTime4 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_LT( modTime3, modTime4 ) << "fail to check modTime after close" ;
   
   // before close, read lob won't change the modTime
   CHAR* readBuf = (CHAR*)malloc( len+1 ) ;
   memset( readBuf, 0, len+1 ) ;
   UINT32 readLen = 0 ;
   rc = sdbReadLob( lob, len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   free( readBuf ) ;

   UINT64 modTime5 ;
   rc = sdbGetLobModificationTime( lob, &modTime5 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_EQ( modTime4, modTime5 ) << "fail to check modTime after read" ;

   // after close, read lob won't change the modTime, still the same
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;

   UINT64 modTime6 ;
   rc = sdbGetLobModificationTime( lob, &modTime6 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getModificationTime" ;
   ASSERT_EQ( modTime5, modTime6 ) << "fail to check modTime after close" ;

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
