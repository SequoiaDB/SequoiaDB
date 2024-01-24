/*********************************************************
 * @Description: test case for c driver
 *				     concurrent test with multi rg
 *               sync test, no need to check standalone
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 *********************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 5
#define MAX_NAME_LEN 256

class concurrentRgTest : public testBase
{
protected:
   sdbReplicaGroupHandle rg[ ThreadNum ] ;
   CHAR* rgName[ ThreadNum ] ;
   CHAR hostName[ MAX_NAME_LEN ] ;
   BOOLEAN isStandAlone ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      isStandAlone = FALSE ;
      if ( isStandalone(db) )
      {
         isStandAlone = TRUE ;
         return ;
      }
      memset( hostName, 0, sizeof(hostName) ) ;
      rc = getDBHost( db, hostName, MAX_NAME_LEN-1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      for( INT32 i = 0;i < ThreadNum;++i )
      {
         CHAR tmp[50] = { 0 } ;
         sprintf( tmp, "%s%d", "concurrentRgTestRg", i ) ;
         rgName[i] = strdup( tmp ) ;
      }

      for( INT32 i = 0;i < ThreadNum;++i )
      {
         rc = sdbCreateReplicaGroup( db, rgName[i], &rg[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create rg " << rgName[i] ;
      }
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if ( isStandAlone )
      {
         return ;
      }

      if( !HasFailure() )
      { 
         for( INT32 i = 0;i < ThreadNum;++i )
         {
            rc = sdbRemoveReplicaGroup( db, rgName[i] ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to remove rg " << rgName[i] ;
            sdbReleaseReplicaGroup( rg[i] ) ;
            free( rgName[i] ) ;
         }
      }
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   CHAR* rgName ;	// replicaGroupName
   INT32 rid ;				    // rg id
   CHAR* hostname ;         // hostname
} ;

void func_rg( ThreadArg* arg )
{
   sdbReplicaGroupHandle rg ;
   CHAR* rgName = arg->rgName ;
   INT32 i = arg->rid ;
   INT32 rc = SDB_OK ;
   CHAR* hostName = arg->hostname ;
   sdbConnectionHandle sdb ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &sdb ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = sdbGetReplicaGroup( sdb, rgName, &rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get group" ;

   CHAR svcName1[10] ;
   sprintf( svcName1, "%d", atoi( ARGS->rsrvPortBegin() ) + 2 * i * 10 ) ;
   CHAR svcName2[10] ;
   sprintf( svcName2, "%d", atoi( ARGS->rsrvPortBegin() ) + 2 * i * 10 + 10 ) ;

   CHAR dbPath1[100], dbPath2[100] ;
   sprintf( dbPath1, "%s%s%s", ARGS->rsrvNodeDir(), "data/", svcName1 ) ;
   sprintf( dbPath2, "%s%s%s", ARGS->rsrvNodeDir(), "data/", svcName2 ) ;

   rc = sdbCreateNode( rg, hostName, svcName1, dbPath1, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node1 in rg " << i << ",node: " << hostName << ":"
                           << svcName1 << " dbpath: " << dbPath1 ;

   rc = sdbCreateNode( rg, hostName, svcName2, dbPath2, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node2 in rg " << i << ",node: " << hostName << ":"
                           << svcName2 << " dbpath: " << dbPath2 ;

   rc = sdbStartReplicaGroup( rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg " << i ;

   rc = sdbStopReplicaGroup( rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop rg " << i ;

   // remove node1 success
   // cannot remove node2 which is the only one node in rg
   rc = sdbRemoveNode( rg, hostName, svcName1, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove node1 in rg " << i ;
   rc = sdbRemoveNode( rg, hostName, svcName2, NULL ) ;
   ASSERT_EQ( SDB_CATA_RM_NODE_FORBIDDEN, rc ) << "fail to test remove node2 in rg " << i ;
}

TEST_F( concurrentRgTest, replicaGroup )
{
   if ( isStandAlone )
   {
      return ;
   }
   Worker* workers[ThreadNum] ;
   ThreadArg arg[ThreadNum] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].rgName = rgName[i] ;
      arg[i].rid = i ;
      arg[i].hostname = hostName ;
      workers[i] = new Worker( (WorkerRoutine)func_rg, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
