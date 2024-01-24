/**************************************************************************
 * @Description:   test lob seek write operation
 *                 seqDB-13427:未seek写lob
 * @Modify:        Liang xuewang
 *                 2017-12-13
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobWrite13427 : public testBase
{
protected:
   const CHAR *csName ;
   const CHAR *clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      csName = "lobWriteCs13427" ;
      clName = "lobWriteCl13427" ;
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

TEST_F( lobWrite13427, lobWrite )
{
   INT32 rc = SDB_OK ;
   
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   const CHAR* writeBuf1 = "123456789ABCDEabcde" ;
   UINT32 len1 = strlen( writeBuf1 ) ;
   rc = createLob( cl, oid, writeBuf1, len1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR* writeBuf2 = "987654321" ;
   UINT32 len2 = strlen( writeBuf2 ) ;
   rc = writeLob( cl, oid, writeBuf2, len2 ) ;

   SINT64 size = 0 ;
   rc = getLobSize( cl, oid, &size ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len1, size ) << "fail to check lobSize" ; 

   CHAR* readBuf = (CHAR*)malloc( len1+1 ) ;
   memset( readBuf, 0, len1+1 ) ;
   UINT32 readLen = 0 ;
   rc = readLob( cl, oid, readBuf, len1, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( len1, readLen ) << "fail to check readLen" ;
   const CHAR* expBuf = "987654321ABCDEabcde" ;
   ASSERT_STREQ( expBuf, readBuf ) << "fail to check readBuf" ;
   free( readBuf ) ;
}
