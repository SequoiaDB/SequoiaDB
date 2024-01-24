/******************************************************************************************
 * @Description: test case because of ci problems
 *				     src testcase in trunk/testcases/hlt/lob_testcases/lobAbnormalTestcase.cpp
 * 				  test drop cs and write lob at the same time with multi thread
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 ******************************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

class dropCsAndWriteLobTest : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      csName = "dropCsAndWriteLobTestCs" ;
      clName = "dropCsAndWriteLobTestCl" ;
      INT32 rc = SDB_OK ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ; 
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbCollectionHandle cl ;
   const CHAR* lobBuffer ;
   sdbConnectionHandle db ;
   const CHAR* csName ;
   sdbLobHandle lob ;
};

void func_lobWrite(ThreadArg *arg)
{
   sdbCollectionHandle cl = arg->cl ;
   const CHAR* lobBuffer = arg->lobBuffer ;
   sdbLobHandle lob = arg->lob ;
   INT32 lobNum = 20 ;
   INT32 rc = SDB_OK ;

   for( INT32 i = 0;i < lobNum;++i )
   {
      rc = sdbWriteLob( lob, lobBuffer, strlen(lobBuffer) ) ;
      ASSERT_TRUE( rc == SDB_OK || rc == SDB_DMS_NOTEXIST || 
                   rc == SDB_RTN_CONTEXT_NOTEXIST ) << "fail to write lob, i = " << i << " rc = " << rc ;
      printf( "sdbWriteLob, rc = %d\n", rc ) ; 
   }

   printf( "over to excute lob write.\n" ) ;
}

void func_dropCs(ThreadArg *arg)
{
   ossSleep( getRand()*100 ) ;
   sdbConnectionHandle db = arg->db ;
   const CHAR* csName = arg->csName ;
   INT32 rc = SDB_OK ;

   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_TRUE( rc == SDB_OK || rc == SDB_LOCK_FAILED ) << "fail to drop collection space, rc = " << rc ;
   printf( "over to excute drop cs.\n" ) ;
}

TEST_F( dropCsAndWriteLobTest, dropCsAndWriteLob )
{
   INT32 rc = SDB_OK ;

   // generate lob
   UINT32 lobSize = 10*1024*1024 ;
   string str( lobSize, 'x' ) ;
   const CHAR* lobBuffer = str.c_str() ;

   // sdb open lob
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;

   // create thread
   ThreadArg arg ;
   arg.cl = cl ;
   arg.lobBuffer = lobBuffer ;
   arg.db = db ;
   arg.csName = csName ;
   arg.lob = lob ;
   Worker* worker1 = new Worker( (WorkerRoutine)func_lobWrite, &arg, false ) ;
   Worker* worker2 = new Worker( (WorkerRoutine)func_dropCs, &arg, false ) ;
   worker1->start() ;
   worker2->start() ;
   worker2->waitStop() ;
   worker1->waitStop() ;
   delete worker1 ;
   delete worker2 ;

   // sdb close lob
   rc = sdbCloseLob( &lob ) ;
   ASSERT_TRUE( rc == SDB_OK || rc == SDB_RTN_CONTEXT_NOTEXIST ) << "fail to close lob, rc = " << rc ;
}
