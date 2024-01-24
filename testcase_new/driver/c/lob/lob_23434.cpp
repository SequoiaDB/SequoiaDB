/************************************************************************
 * @Description : seqDB-23434:以 SHARE_READ、SHARE_READ|WRITE 模式读写 lob
 * @Modify List : Li Yuanyue
 *                2020-01-18
 ************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lob23434 : public testBase
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
      csName = "lob_23434" ;
      clName = "lob_23434" ;

      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;   
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

TEST_F( lob23434, SDB_LOB_WRITE_And_SDB_LOB_SHAREREAD )
{
   INT32 rc = SDB_OK ;
   sdbLobHandle lob ;
   bson_oid_t oid ;
   char writeBuf[] = "0123456789ABCDEFabcdef" ;

   bson_oid_gen( &oid ) ;

   // create lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_CREATEONLY" ;
   rc = sdbWriteLob( lob, writeBuf, sizeof( writeBuf ) ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // SDB_LOB_WRITE | SDB_LOB_SHAREREAD
   // open lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_WRITE | SDB_LOB_SHAREREAD, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_WRITE | SDB_LOB_SHAREREAD" ;

   // read
   CHAR readBuf[100] = { 0 } ;
   UINT32 len = 10 ;
   UINT32 readLen ;
   rc = sdbReadLob( lob, len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check lob read len" ;
   ASSERT_STREQ( "0123456789", readBuf ) << "fail to check read buf" ;

   // write
   char helloBuf[] = "hello" ;
   rc = sdbWriteLob( lob, helloBuf, sizeof( helloBuf ) ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // close 
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // SDB_LOB_WRITE 
   // open
   rc = sdbOpenLob( cl, &oid, SDB_LOB_SHAREREAD, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_WRITE" ;

   // read
   CHAR readBuf2[100] = { 0 } ;
   len = 10 + 5 ;
   rc = sdbReadLob( lob, len, readBuf2, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check lob read len" ;
   ASSERT_STREQ( "0123456789hello", readBuf2 ) << "fail to check read buf" ;

   // close 
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}