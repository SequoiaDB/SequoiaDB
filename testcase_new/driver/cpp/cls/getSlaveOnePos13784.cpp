/**************************************************************************
 * @Description :   test getSlave operation
 *                  seqDB-13784:指定一个位置获取备节点        
 *                  SEQUOIADBMAINSTREAM-2981
 * @Modify      :   Liang xuewang
 *                  2017-12-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

#define MAX_NAME_LEN 256

class getSlaveOnePos13784 : public testBase
{
protected:
   const CHAR* rgName ;
   sdbReplicaGroup rg ;
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
      rgName = "getSlaveTestRg13784" ;
      rc = db.createReplicaGroup( rgName, rg ) ;
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
      rc = db.removeReplicaGroup( rgName ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove rg %s, rc = %d", rgName, rc ) ;
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
         cout << "create node " << host << ":" << svc << " dbpath: " << dbpath << endl ;
         rc = rg.createNode( host, svc, dbpath ) ;
         CHECK_RC( SDB_OK, rc, "fail to create node, rc = %d", rc ) ;
      }      
   done:
      return rc ;
   error:
      goto done ;
   }

   // check rg has no master or not
   INT32 isMasterExist( BOOLEAN* hasMaster )
   {
      INT32 rc = SDB_OK ;
      const CHAR* csName = "getSlaveTestCs13784" ;
      const CHAR* clName = "getSlaveTestCl13784" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;
      BSONObj option = BSON( "Group" << rgName ) ;

      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cs, rc = %d", rc ) ;
      rc = cs.createCollection( clName, option, cl ) ;
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
         cout << "fail to create cl, rc = " << rc << endl ;
         goto error ;
      }
      rc = db.dropCollectionSpace( csName ) ;
      CHECK_RC( SDB_OK, rc, "fail to drop cs, rc = %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
} ;

// only one node in rg
TEST_F( getSlaveOnePos13784, oneNode )
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

   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg" ;

   sdbNode master ;
   while( ( rc = rg.getMaster( master ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   const CHAR* masterSvc = master.getServiceName() ;

   vector<INT32> positions ;
   positions.push_back( 1 ) ;
   sdbNode slave ;
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave" ;

   const CHAR* slaveSvc = slave.getServiceName() ;

   ASSERT_STREQ( masterSvc, slaveSvc ) << "fail to check svcName" ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg, getSlave with master slave pos
// two nodes in rg, getSlave with 1 2 3 4 5 6 7
TEST_F( getSlaveOnePos13784, twoNodes )
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

   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg" ;

   sdbNode master ;
   while( ( rc = rg.getMaster( master ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
   }                 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;

   vector<string> nodes ;
   rc = getGroupNodes( db, rgName, nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   vector<INT32> positions ;
   for( INT32 i = 1;i <= 7;i++ )
   {
      positions.push_back( i ) ;
      sdbNode slave ;
      rc = rg.getSlave( slave, positions ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave, i = " << i ;
      const CHAR* slaveSvc = slave.getServiceName() ;
      INT32 idx = (i-1) % 2 ;
      INT32 pos = nodes[idx].find( slaveSvc ) ;
      ASSERT_NE( -1, pos ) << "fail to check svcName, i = " << i 
                              << " " << nodes[idx] << " " << slaveSvc ;
      positions.clear() ;
   }
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg( no master ), getSlave with 1 2 pos
// only one node normal
TEST_F( getSlaveOnePos13784, twoNodesNoMaster )
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

   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg" ;

   sdbNode master ;
   while( ( rc = rg.getMaster( master ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
   }                 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   vector<string> nodes ;
   rc = getGroupNodes( db, rgName, nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // stop master to make rg has no master
   rc = master.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop master" ;

   BOOLEAN hasMaster = TRUE ;
   rc = isMasterExist( &hasMaster ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, hasMaster ) << "fail to check no master" ;

   vector<INT32> positions ;
   for( INT32 i = 1;i <= 2;i++ )
   {
      positions.push_back( i ) ;
      sdbNode slave ;
      rc = rg.getSlave( slave, positions ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave, i = " << i ;
      const CHAR* slaveSvc = slave.getServiceName() ;
      INT32 idx = (i-1) % 2 ;
      INT32 pos = nodes[idx].find( slaveSvc ) ;
      ASSERT_NE( -1, pos ) << "fail to check svcName, i = " << i 
                              << " " << nodes[idx] << " " << slaveSvc ;
      positions.clear() ;
   }
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
