/**************************************************************************
 * @Description :   test lob lock operation
 *                  seqDB-14129:getModificationTime接口参数校验
 *                  SEQUOIADBMAINSTREAM-2889
 * @Modify      :   Liang xuewang
 *                  2017-12-14
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobModTime14129 : public testBase
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
      csName = "lobModTimeCs14129" ;
      clName = "lobModTimeCl14129" ;
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

TEST_F( lobModTime14129, lobHandle )
{
   INT32 rc = SDB_OK ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   UINT64 modTime ;
   rc = sdbGetLobModificationTime( lob, &modTime ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getModTime with invalid lob handle" ;

   rc = sdbGetLobModificationTime( db, &modTime ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) << "fail to test getModTime with db handle" ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = sdbGetLobModificationTime( lob, &modTime ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getModTime with closed lob" ;
}

TEST_F( lobModTime14129, millis )
{
   INT32 rc = SDB_OK ;

   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob with create mode" ;
   
   /*
   rc = sdbGetLobModificationTime( lob, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getModificationTime with millis NULL" ;
   */   

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
