/**************************************************
 * @Description: test case for c driver
 *				     concurrent test with multi lob
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 **************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 5

class concurrentLobTest : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   sdbLobHandle lob[ ThreadNum ] ;
   bson_oid_t oid[ ThreadNum ] ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "concurrentLobTestCs" ;
      clName = "concurrentLobTestCl" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         bson_oid_gen( &oid[i] ) ;
         rc = sdbOpenLob( cl, &oid[i], SDB_LOB_CREATEONLY, &lob[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
      }   
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

class ThreadArg : public WorkerArgs
{
public:
   sdbLobHandle lob ;          // lob handle
   INT32 id ;				    // lob id
} ;

void func_lobWrite( ThreadArg* arg )
{
   sdbLobHandle lob = arg->lob ;
   INT32 i = arg->id ;
   INT32 rc = SDB_OK ;

   UINT32 size = 24*1024*1024 ;
   CHAR* lobBuffer = (CHAR*)malloc( size ) ;
   if( !lobBuffer )
   {
      printf( "out of memory for lobBuffer in lobWrite.\n" ) ;
      return ;
   }
   memset( lobBuffer, 'a', size ) ;

   // write lob
   rc = sdbWriteLob( lob, lobBuffer, size ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob " << i ;
   // close lob
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob " << i ;

   free( lobBuffer ) ;
}

void func_lobRead( ThreadArg* arg )
{
   sdbLobHandle lob = arg->lob ;
   INT32 i = arg->id ;
   INT32 rc = SDB_OK ;

   UINT32 size = 10*1024*1024 ;
   CHAR* lobBuffer = (CHAR*)calloc( size, sizeof(CHAR) ) ;
   if( !lobBuffer )
   {
      printf( "out of memory for lobBuffer in lobRead.\n" ) ;
      return ;
   }
   UINT32 readlen = 0 ;

   // read lob
   rc = sdbReadLob( lob, size, lobBuffer, &readlen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob " << i ;
   ASSERT_EQ( size, readlen ) << "fail to check read lob length,i = " << i ;
   // close lob
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob " << i ;

   free( lobBuffer ) ;
}

TEST_F( concurrentLobTest, lob )
{
   INT32 rc = SDB_OK ;

   // create multi thread to operate different lob write
   Worker * workers[ThreadNum] ;
   ThreadArg arg[ThreadNum] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].lob = lob[i] ;
      arg[i].id = i ; 
      workers[i] = new Worker( (WorkerRoutine)func_lobWrite, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

   // open lob with read mode
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      rc = sdbOpenLob( cl, &oid[i], SDB_LOB_READ, &lob[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode,i = " << i ;
   }

   // create multi thread to operate different lob read
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].lob = lob[i] ;
      arg[i].id = i ; 
      workers[i] = new Worker( (WorkerRoutine)func_lobRead, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
