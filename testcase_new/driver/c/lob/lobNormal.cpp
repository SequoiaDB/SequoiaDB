/************************************************************************
 * @Description : Test lob basic operation: 
 *                write lob, read lob, seek read lob,
 *                drop lob, get lob size,
 *                get lob create time, 
 * @Modify List : Liang xuewang
 *                2016-11-21
 ************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobNormalTest : public testBase
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
      csName = "lobNormalTestCs" ;
      clName = "lobNormalTestCl" ;
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

TEST_F( lobNormalTest, basicOperation )
{
   sdbLobHandle lob ;
   INT32 rc = SDB_OK ;

   // create lob buffer( write and read )
   INT32 lobSize = 1024 * 1024 * 2 ;
   CHAR* lobWriteBuffer = (CHAR*)malloc( lobSize ) ;
   CHAR* lobReadBuffer  = (CHAR*)malloc( lobSize ) ;
   if( !lobWriteBuffer || !lobReadBuffer )
   {
      printf( "out of memory for lob buffer.\n" ) ;
      return ;
   }
   memset( lobWriteBuffer, 'a', lobSize ) ;
   memset( lobReadBuffer, 0, lobSize ) ;

   // open lob, write lob, close lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after write" ;

   // open lob, get lobsize, close lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   SINT64 size ;
   rc = sdbGetLobSize( lob, &size ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get lob size" ;
   ASSERT_EQ( lobSize, size ) << "fail to check lob size" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after get lob size" ;

   // open lob, read lob, close lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   UINT32 readSize ;
   rc = sdbReadLob( lob, lobSize, lobReadBuffer, &readSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( lobSize, readSize ) << "fail to check read lob size" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after read" ;

   // open lob, seek read lob, close lob
   memset( lobReadBuffer, 0, lobSize ) ;
   INT32 seekSize = lobSize/2 + 31 ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   rc = sdbSeekLob( lob, seekSize, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = sdbReadLob( lob, lobSize, lobReadBuffer, &readSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob after seek" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after seek read" ;

   // remove lob in the end
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to remove lob" ;

   // free buffer
   free( lobWriteBuffer ) ;
   free( lobReadBuffer ) ;
}

TEST_F( lobNormalTest, getLobCreateTime )
{
   sdbLobHandle lob       = SDB_INVALID_HANDLE ;
   INT32 rc = SDB_OK ;

   // create lob buffer
   INT32 lobSize = 1024 * 1024 * 2 ;
   CHAR* lobWriteBuffer = (CHAR*)malloc( lobSize ) ;
   if( !lobWriteBuffer )
   {
      printf( "out of memory for lob buffer.\n" ) ;
      return ;
   }
   memset( lobWriteBuffer, 'a', lobSize ) ;

   // get system current time
   struct timeval tv ;
   gettimeofday( &tv, NULL ) ;
   // sleep 100ms 
   usleep( 1000 * 100 ) ; 

   // open lob, write lob, close lob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob";
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after write" ;

   // open lob, get lob create time, close lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   UINT64 createtime ;
   rc = sdbGetLobCreateTime( lob, &createtime ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get lob create time" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after get lob create time" ;

   // compare system time and lob create time
   UINT64 systime = tv.tv_sec * 1000 + tv.tv_usec / 1000 ;
   printf( "SystemTime= %ld\nCreateTime= %ld\n", systime, createtime ) ;
   ASSERT_LE( systime, createtime ) << "fail to check lob create time" ;

   // free buffer
   free( lobWriteBuffer ) ;
}
