/**************************************************************************
 * @Description :   test getSlave1 operation
 *                  seqDB-13782:指定多个位置获取备节点
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

class getSlaveMultiPos13782 : public testBase
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
      rgName = "getSlaveTestRg13782" ;
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

// only one node in rg, getSlave1 with 1-7
TEST_F( getSlaveMultiPos13782, oneNode )
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

   INT32 positionArr[7] = { 1, 2, 3, 4, 5, 6, 7 } ;
   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg, positionArr, 7, &slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;

   const CHAR* slaveSvc ;
   rc = getNodeSvcname( slave, &slaveSvc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_STREQ( masterSvc, slaveSvc ) << "fail to check svcName" ;

   sdbReleaseNode( master ) ;
   sdbReleaseNode( slave ) ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg, getSlave1 with 1-7
TEST_F( getSlaveMultiPos13782, twoNodes )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = createNodes( 2 ) ;
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

   INT32 positionArr[7] = { 1, 2, 3, 4, 5, 6, 7 } ;
   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg, positionArr, 7, &slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;
   const CHAR* slaveSvc ;
   rc = getNodeSvcname( slave, &slaveSvc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_STRNE( masterSvc, slaveSvc ) << "fail to check svcName" ;

   sdbReleaseNode( master ) ;
   sdbReleaseNode( slave ) ;
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg, getSlave1 with all master pos
TEST_F( getSlaveMultiPos13782, twoNodesMasterPos )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = createNodes( 2 ) ;
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
   INT32 masterPos = 0 ;
   for( INT32 i = 0;i < nodes.size();i++ )
   {
      if( nodes[i].find( masterSvc ) != -1 )
      {
         masterPos = i + 1 ;
         break ;
      }
   }
   ASSERT_TRUE( masterPos >= 1 && masterPos <= 7 ) << "fail to check masterPos " 
                                                   << masterPos ;

   INT32 positionArr[7] ;
   for( INT32 i = 0;i < 7;i++ )
   {
      positionArr[i] = masterPos ;
   }
   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg, positionArr, 7, &slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;
   const CHAR* slaveSvc ;
   rc = getNodeSvcname( slave, &slaveSvc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_STREQ( masterSvc, slaveSvc ) << "fail to check svcName" ;

   sdbReleaseNode( master ) ;
   sdbReleaseNode( slave ) ;
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// three nodes in rg( stop origin master ), getSlave1 with 1-7
TEST_F( getSlaveMultiPos13782, threeNodes )
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

   // stop origin master to make rg change master
   rc = sdbStopNode( master ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node" ;
   sdbNodeHandle newMaster ;
   const CHAR* newMasterSvc ;
   
   // check master node is exist or not 
   while( ( rc = sdbGetNodeMaster( rg, &newMaster ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
      sdbReleaseNode( newMaster ) ;
   }        
   // check change master
   while( TRUE )
   {
      rc = sdbGetNodeMaster( rg, &newMaster ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get newMaster" ;
      rc = getNodeSvcname( newMaster, &newMasterSvc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      if( strcmp( masterSvc, newMasterSvc ) )
         break ;
      sleep( 1 ) ;
      sdbReleaseNode( newMaster ) ;
   }

   INT32 positionArr[7] = { 1, 2, 3, 4, 5, 6, 7 } ;
   INT32 totalCnt = 50 ;
   INT32 masterCnt = 0 ;
   INT32 newMasterCnt = 0 ;
   for( INT32 i = 0;i < totalCnt;i++ )
   {
      sdbNodeHandle slave ;
      rc = sdbGetNodeSlave1( rg, positionArr, 7, &slave ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;
      const CHAR* slaveSvc ;
      rc = getNodeSvcname( slave, &slaveSvc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      if( !strcmp( slaveSvc, masterSvc ) )
         masterCnt++ ;
      else if( !strcmp( slaveSvc, newMasterSvc ) )
         newMasterCnt++ ;
      sdbReleaseNode( slave ) ;
   }
   ASSERT_NE( 0, masterCnt ) << "fail to check old master cnt" ;
   ASSERT_EQ( 0, newMasterCnt ) << "fail to check new master cnt" ;   

   sdbReleaseNode( master ) ;
   sdbReleaseNode( newMaster ) ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
