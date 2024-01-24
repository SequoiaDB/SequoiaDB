/**************************************************************************
 * @Description :   test getSlave1 operation
 *                  seqDB-13781:指定一个位置获取备节点           
 *                  SEQUOIADBMAINSTREAM-2981
 * @Modify      :   Liang xuewang
 *                  2017-12-14
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

class getSlaveOnePos13781 : public testBase
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
      rgName = "getSlaveTestRg13781" ;
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

   // check rg has no master or not
   INT32 isMasterExist( BOOLEAN* hasMaster )
   {
      INT32 rc = SDB_OK ;
      const CHAR* csName = "getSlaveTestCs13781" ;
      const CHAR* clName = "getSlaveTestCl13781" ;
      sdbCSHandle cs ;
      sdbCollectionHandle cl ;
      bson option ;
      bson_init( &option ) ;
      bson_append_string( &option, "Group", rgName ) ;
      bson_finish( &option ) ;

      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cs, rc = %d", rc ) ;
      rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
      if( rc == SDB_OK )
      {
         *hasMaster = TRUE ;
      }
      else if( rc == SDB_CLS_NOT_PRIMARY )
      {
         *hasMaster = FALSE ;
         rc = SDB_OK ;
      }
      else
      {
         printf( "fail to create cl, rc = %d\n", rc ) ;
         goto error ;
      }
      rc = sdbDropCollectionSpace( db, csName ) ;
      CHECK_RC( SDB_OK, rc, "fail to drop cs, rc = %d", rc ) ;

   done:
      bson_destroy( &option ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      return rc ;
   error:
      goto done ;
   }
} ;

// only one node in rg
TEST_F( getSlaveOnePos13781, oneNode )
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

   INT32 positionArr[1] = { 1 } ;
   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg, positionArr, 1, &slave ) ;
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

// two nodes in rg, getSlave1 with master slave pos
// two nodes in rg, getSlave1 with 1 2 3 4 5 6 7
TEST_F( getSlaveOnePos13781, twoNodes )
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
   sdbReleaseNode( master ) ;

   vector<string> nodes ;
   rc = getGroupNodes( db, rgName, nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   INT32 positionArr[1] ;
   for( INT32 i = 1;i <= 7;i++ )
   {
      positionArr[0] = i ; 
      sdbNodeHandle slave ;
      rc = sdbGetNodeSlave1( rg, positionArr, 1, &slave ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1, i = " << i ;
      const CHAR* slaveSvc ;
      rc = getNodeSvcname( slave, &slaveSvc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      INT32 idx = (i-1) % 2 ;
      INT32 pos = nodes[idx].find( slaveSvc ) ;
      ASSERT_NE( -1, pos ) << "fail to check svcName, i = " << i 
                              << " " << nodes[idx] << " " << slaveSvc ;
      sdbReleaseNode( slave ) ;
   }
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg( no master ), getSlave1 with 1 2 pos
// only one node normal
TEST_F( getSlaveOnePos13781, twoNodesNoMaster )
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
   vector<string> nodes ;
   rc = getGroupNodes( db, rgName, nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // stop master to make rg has no master
   rc = sdbStopNode( master ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop master" ;
   sdbReleaseNode( master ) ;

   BOOLEAN hasMaster = TRUE ;
   rc = isMasterExist( &hasMaster ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, hasMaster ) << "fail to check no master" ;

   INT32 positionArr[1] ;
   for( INT32 i = 1;i <= 2;i++ )
   {
      positionArr[0] = i ;
      sdbNodeHandle slave ;
      rc = sdbGetNodeSlave1( rg, positionArr, 1, &slave ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1, i = " << i ;
      const CHAR* slaveSvc ;
      rc = getNodeSvcname( slave, &slaveSvc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      INT32 idx = (i-1) % 2 ;
      INT32 pos = nodes[idx].find( slaveSvc ) ;
      ASSERT_NE( -1, pos ) << "fail to check svcName, i = " << i 
                              << " " << nodes[idx] << " " << slaveSvc ;
      sdbReleaseNode( slave ) ;
   }
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
