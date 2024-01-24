/************************************************************************
 * @Description : Test lob abnormal use:
 *                not exist lob
 *                not closed lob 
 *                write lob with read mode.
 * @Modify List : Liang xuewang
 *                2016-11-21
 ************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobAbnormalTest : public testBase
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
      csName = "lobAbnormalTestCs" ;
      clName = "lobAbnormalTestCl" ;
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

TEST_F( lobAbnormalTest, notExistLob )
{
   INT32 rc = SDB_OK ;
   sdbLobHandle lob ;

   // read not exist lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_FNE, rc ) << "fail to test read not exist lob" ;

   // get not exist lob size
   SINT64 size ;
   rc = sdbGetLobSize( lob, &size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test get not exist lob size" ;

   // get not exist lob create time
   UINT64 mills ;
   rc = sdbGetLobCreateTime( lob, &mills ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test get not exist lob create time" ;

   // close not exist lob
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test close not exist lob" ;

   // remove not exist lob
   rc = sdbRemoveLob( lob, &oid ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to remove not exist lob" ;   
}

TEST_F( lobAbnormalTest, notClosedLob )
{
   INT32 rc = SDB_OK ;
   sdbLobHandle lob ;

   // create lob buffer
   INT32 lobSize = 1024 * 1024 * 2 ;
   CHAR* lobWriteBuffer = (CHAR*)malloc( lobSize ) ;
   if( !lobWriteBuffer )
   {
      printf( "out of memory for lob buffer.\n" ) ;
      return ;
   }
   memset( lobWriteBuffer, 'a', lobSize ) ;

   // open lob, write lob, without close lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob"  ;

   // read lob when lob is not closed
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_LOB_IS_IN_USE, rc ) << "fail to test open lob when lob is not closed" ;

   // remove lob when lob is not closed
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_LOB_IS_IN_USE, rc ) << "fail to test remove lob when lob is not closed" ;

   // close lob
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to close lob in the end" ;

   // free buffer
   free( lobWriteBuffer ) ;
}

TEST_F( lobAbnormalTest, writeLobWithReadMode )
{
   INT32 rc = SDB_OK ;
   sdbLobHandle lob ;

   // create lob buffer
   INT32 lobSize = 1024 * 1024 * 2 ;
   CHAR* lobWriteBuffer = (CHAR*)malloc( lobSize ) ;
   if( !lobWriteBuffer )
   {
      printf( "out of memory for lob buffer.\n" ) ;
      return ;
   }
   memset( lobWriteBuffer, 'a', lobSize ) ; 

   // open lob, write lob, close lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // write lob with read mode
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test write lob with read mode" ;

   // close lob
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob in the end" ;

   // free buffer
   free( lobWriteBuffer ) ;
}
