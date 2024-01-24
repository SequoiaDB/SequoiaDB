/********************************************************
 * @Description: test case for c driver
 *				     concurrent test with multi node
 *               sync test, no need to check standalone
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 ********************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 5
#define MAX_NAME_LEN 256

class concurrentNodeTest : public testBase
{
protected:
   sdbReplicaGroupHandle rg ;
   const CHAR* rgName ;
   CHAR* svcName[ ThreadNum ] ;
   CHAR* dbPath[ ThreadNum ] ;
   sdbNodeHandle node[ ThreadNum ] ;
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

      rgName = "concurrentNodeTestRg" ;
      rc = sdbCreateReplicaGroup( db, rgName, &rg ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create rg " << rgName ;
      
      for( INT32 i = 0;i < ThreadNum;i++)
      {  
         INT32 portBegin ;
         sscanf( ARGS->rsrvPortBegin(), "%d", &portBegin ) ;
         INT32 number = portBegin + i*10 ;
         CHAR tmp[10] ;
         sprintf( tmp, "%d", number ) ;
         svcName[i] = strdup( tmp ) ;
      }

      for( INT32 i = 0;i < ThreadNum;i++)
      {
         CHAR tmp[100] ;
         sprintf( tmp, "%s%s%s", ARGS->rsrvNodeDir(), "data/", svcName[i] ) ;
         dbPath[i] = strdup( tmp ) ;
      }

      for( INT32 i = 0;i < ThreadNum;i++)
      {
         rc = sdbCreateNode( rg, hostName, svcName[i], dbPath[i], NULL ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create node " << hostName << ":" << svcName[i] 
                                 << "dbPath: " << dbPath[i] ;
         rc = sdbGetNodeByHost( rg, hostName, svcName[i], &node[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to get node " << hostName << ":" << svcName[i] ;
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
         rc = sdbRemoveReplicaGroup( db, rgName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to remove rg " << rgName ;
         for( INT32 i = 0;i < ThreadNum;++i )
         {  
            sdbReleaseNode( node[i] ) ;
            free( svcName[i] ) ;
            free( dbPath[i] ) ;
         }
         sdbReleaseReplicaGroup( rg ) ;
      }
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbNodeHandle node ;	    // node
   INT32 tid ;				    // thread id
   CHAR* hostname ;         // hostname
} ;

void func_node( ThreadArg* arg )
{
   sdbNodeHandle node = arg->node ;
   INT32 i = arg->tid ;
   INT32 rc = SDB_OK ;
   CHAR* hostName = arg->hostname ;

   const CHAR *host, *svc, *nodeName ;
   INT32 nodeId ;
   rc = sdbGetNodeAddr( node, &host, &svc, &nodeName, &nodeId ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node addr,i = " << i ;
   ASSERT_STREQ( hostName, host ) << "fail to check host of node,i = " << i ;
   printf( "%d: nodeName = %s nodeId = %d\n", i, nodeName, nodeId ) ;

   rc = sdbStartNode( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start node " << i ;
   rc = sdbStopNode( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node " << i ;
}

TEST_F( concurrentNodeTest, node )
{
   if ( isStandAlone )
   {
      return ;
   }
   Worker * workers[ThreadNum] ;
   ThreadArg arg[ThreadNum] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].node = node[i] ;
      arg[i].tid = i ;
      arg[i].hostname = hostName ;
      workers[i] = new Worker( (WorkerRoutine)func_node, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
