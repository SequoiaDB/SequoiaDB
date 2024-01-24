/**************************************************************************
 * @Description :   test lob lock operation
 *                  seqDB-14130:seekLob参数校验
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobSeek14130 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   bson_oid_t oid ;
   SINT64 size ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobSeekCs14130" ;
      clName = "lobSeekCl14130" ;
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
   INT32 init()
   {
      INT32 rc = SDB_OK ;   
      bson_oid_gen( &oid ) ;
      const CHAR* buf = "123456789ABCDE" ;
      UINT32 len = strlen( buf ) ;
   
      rc = createLob( cl, oid, buf, len ) ;
      CHECK_RC( SDB_OK, rc, "fail to create and write lob, rc = %d", rc ) ;
      rc = getLobSize( cl, oid, &size ) ;
      if( size != len )
      {
         printf( "fail to check lob size, expect: %d, actual: %d\n", len, size ) ;
         rc = SDB_TEST_ERROR ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( lobSeek14130, lobHandle )
{
   INT32 rc = SDB_OK ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with invalid lob handle" ;

   rc = sdbSeekLob( db, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) << "fail to test seekLob with db handle" ;

   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with closed lob" ;
}

TEST_F( lobSeek14130, lobSeekSet )
{
   INT32 rc = SDB_OK ;

   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
      
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with read mode" ;

   rc = sdbSeekLob( lob, -1, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with SDB_LOB_SEEK_SET-1" ;
   rc = sdbSeekLob( lob, size+1, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with SDB_LOB_SEEK_SET+size+1" ;

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobSeek14130, lobSeekCur )
{
   INT32 rc = SDB_OK ;
  
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
      
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with read mode" ;

   rc = sdbSeekLob( lob, -1, SDB_LOB_SEEK_CUR ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with SDB_LOB_SEEK_CUR-1" ;
   rc = sdbSeekLob( lob, size+1, SDB_LOB_SEEK_CUR ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with SDB_LOB_SEEK_CUR+size+1" ;

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobSeek14130, lobSeekEnd )
{
   INT32 rc = SDB_OK ;
  
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
      
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with read mode" ;

   rc = sdbSeekLob( lob, -1, SDB_LOB_SEEK_END ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with SDB_LOB_SEEK_END-(-1)" ;
   rc = sdbSeekLob( lob, size+1, SDB_LOB_SEEK_END ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with SDB_LOB_SEEK_END-(size+1)" ;

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobSeek14130, invalidWhence )
{
   INT32 rc = SDB_OK ;

   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with createonly mode" ;
   rc = sdbSeekLob( lob, 0, (SDB_LOB_SEEK)3 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test seekLob with whence=3" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
