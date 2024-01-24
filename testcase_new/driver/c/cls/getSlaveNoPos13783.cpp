/**************************************************************************
 * @Description :   test getSlave1 operation
 *                  seqDB-13783:使用getSlave获取备节点
 *                  SEQUOIADBMAINSTREAM-2981
 * @Modify      :   Liang xuewang
 *                  2017-12-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace std ;

#define MAX_NAME_LEN 256

class getSlaveNoPos13783 : public testBase
{
protected:
   const CHAR* rgName ;
   sdbReplicaGroupHandle rg ;
   CHAR host[ MAX_NAME_LEN ] ;
   CHAR svc[ MAX_NAME_LEN ] ;
   CHAR dbpath[ MAX_NAME_LEN ] ;

   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }

   // create rg
   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "getSlaveTestRg13783" ;
      rc = sdbCreateReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to create rg %s, rc = %d", rgName, rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // remove rg
   INT32 release()
   {
      INT32 rc = SDB_OK ;
      rc = sdbRemoveReplicaGroup( db, rgName ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove rg %s, rc = %d", rgName, rc ) ;
      sdbReleaseReplicaGroup( rg ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // create nodes in rg, use RSRVPORTBEGIN
   INT32 createNodes( INT32 nodeNum )
   {
      INT32 rc = SDB_OK ;
      memset( host, 0, MAX_NAME_LEN ) ;
      rc = getDBHost( db, host, MAX_NAME_LEN-1 ) ;
      CHECK_RC( SDB_OK, rc, "fail to gethostname, rc = %d", rc ) ;
      for( INT32 i = 0;i < nodeNum;i++ )
      {
         INT32 port = atoi( ARGS->rsrvPortBegin() ) + i*10 ;
         memset( svc, 0, MAX_NAME_LEN ) ;
         sprintf( svc, "%d", port ) ;
         memset( dbpath, 0, MAX_NAME_LEN ) ;
         sprintf( dbpath, "%s%s%s", ARGS->rsrvNodeDir(), "data/", svc ) ;
         printf( "create node %s:%s, dbpath: %s\n", host, svc, dbpath ) ;
         rc = sdbCreateNode( rg, host, svc, dbpath, NULL ) ;
         CHECK_RC( SDB_OK, rc, "fail to create node, rc = %d", rc ) ;
      }      
   done:
      return rc ;
   error:
      goto done ;
   }

   // get node svc name
   INT32 getNodeSvcname( sdbNodeHandle node, const CHAR** svcName )
   {
      INT32 rc = SDB_OK ;
      const CHAR* hostName ;
      const CHAR* nodeName ;
      INT32 nodeId ;
      rc = sdbGetNodeAddr( node, &hostName, svcName, &nodeName, &nodeId ) ;
      CHECK_RC( SDB_OK, rc, "fail to get node addr, rc = %d", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

// only one node in rg, getSlave
TEST_F( getSlaveNoPos13783, oneNode )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = createNodes( 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbStartReplicaGroup( rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg" ;

   sdbNodeHandle master ;
   while( ( rc = sdbGetNodeMaster( rg, &master ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
      sdbReleaseNode( master ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   const CHAR* masterSvc ;
   rc = getNodeSvcname( master, &masterSvc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave( rg, &slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave" ;

   const CHAR* slaveSvc ;
   rc = getNodeSvcname( slave, &slaveSvc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_STREQ( masterSvc, slaveSvc ) << "fail to check svcName" ;

   sdbReleaseNode( master ) ;
   sdbReleaseNode( slave ) ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// three nodes in rg, getSlave
TEST_F( getSlaveNoPos13783, threeNodes )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = createNodes( 3 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbStartReplicaGroup( rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg" ;

   sdbNodeHandle master ;
   while( ( rc = sdbGetNodeMaster( rg, &master ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
      sdbReleaseNode( master ) ;
   }                 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   const CHAR* masterSvc ;
   rc = getNodeSvcname( master, &masterSvc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   vector<string> nodes ;
   rc = getGroupNodes( db, rgName, nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 3, nodes.size() ) << "fail to check node num" ;
   INT32 masterIdx = 0 ;
   for( INT32 i = 0;i < 3;i++ )
   {
      if( nodes[i].find( masterSvc ) != -1 )
      {
         masterIdx = i ;
         break ;
      }
   }
   
   INT32 totalCnt = 50 ;
   INT32 cnt[3] = { 0, 0, 0 } ;
   for( INT32 i = 0;i < totalCnt;i++ )
   {
      sdbNodeHandle slave ;
      rc = sdbGetNodeSlave( rg, &slave ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;
      const CHAR* slaveSvc ;
      rc = getNodeSvcname( slave, &slaveSvc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      for( INT32 j = 0;j < 3;j++ )
      {
         if( nodes[j].find( slaveSvc ) != -1 )
         {
            cnt[j]++ ;
            break ;
         }
      }
      sdbReleaseNode( slave ) ;
   }
   printf( "cnt: %d %d %d\n", cnt[0], cnt[1], cnt[2] ) ;
   ASSERT_EQ( totalCnt, cnt[0]+cnt[1]+cnt[2] ) << "fail to check totalCnt" ;
   ASSERT_EQ( 0, cnt[masterIdx] ) << "fail to check master cnt" ;

   sdbReleaseNode( master ) ;
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
