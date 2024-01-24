/**************************************************************************
 * @Description :   test getSlave operation
 *                  seqDB-13785:指定多个位置获取备节点
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
using namespace bson ;
using namespace sdbclient ;

#define MAX_NAME_LEN 256

class getSlaveMultiPos13785 : public testBase
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
      rgName = "getSlaveTestRg13785" ;
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
         cout << "create node " << host << ":" << svc << ", dbpath: " << dbpath << endl ;
         rc = rg.createNode( host, svc, dbpath ) ;
         CHECK_RC( SDB_OK, rc, "fail to create node, rc = %d", rc ) ;
      }      
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

// only one node in rg, getSlave with 1-7
TEST_F( getSlaveMultiPos13785, oneNode )
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
   for( INT32 i = 1;i <= 7;i++ )
   {
      positions.push_back( i ) ;
   }
   sdbNode slave ;
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;
   const CHAR* slaveSvc = slave.getServiceName() ;
   ASSERT_STREQ( masterSvc, slaveSvc ) << "fail to check svcName" ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg, getSlave with 1-7
TEST_F( getSlaveMultiPos13785, twoNodes )
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
   const CHAR* masterSvc = master.getServiceName() ;

   vector<INT32> positions ;
   for( INT32 i = 1;i <= 7;i++ )
   {
      positions.push_back( i ) ;
   }
   sdbNode slave ;
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave1" ;
   const CHAR* slaveSvc = slave.getServiceName() ;
   ASSERT_STRNE( masterSvc, slaveSvc ) << "fail to check svcName" ;
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// two nodes in rg, getSlave with all master pos
TEST_F( getSlaveMultiPos13785, twoNodesMasterPos )
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
   const CHAR* masterSvc = master.getServiceName() ;

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

   vector<INT32> positions ;
   for( INT32 i = 0;i < 7;i++ )
   {
      positions.push_back( masterPos ) ;
   }
   sdbNode slave ;
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave" ;
   const CHAR* slaveSvc = slave.getServiceName() ;
   ASSERT_STREQ( masterSvc, slaveSvc ) << "fail to check svcName" ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

// three nodes in rg( stop origin master ), getSlave with 1-7
TEST_F( getSlaveMultiPos13785, threeNodes )
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

   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg" ;

   sdbNode master ;
   while( ( rc = rg.getMaster( master ) ) == SDB_RTN_NO_PRIMARY_FOUND )
   {
      sleep( 1 ) ;
   }                 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   const CHAR* masterSvc = master.getServiceName() ;

   // stop origin master to make rg change master
   rc = master.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node" ;
   sdbNode newMaster ;
   const CHAR* newMasterSvc ;
   const int totalTimeLen = 30 ;
   int alreadySleep = 0 ;
   do
   {
      rc = rg.getMaster( newMaster ) ;
      if ( rc == SDB_RTN_NO_PRIMARY_FOUND )
      {
         sleep( 1 ) ;
         alreadySleep +=1 ;
      }
      if ( alreadySleep < totalTimeLen 
           && rc == SDB_RTN_NO_PRIMARY_FOUND ){
         continue ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to get newMaster" ;
      newMasterSvc = newMaster.getServiceName() ;
      if( strcmp( masterSvc, newMasterSvc ) )
         break ;
   }while(TRUE) ;

   vector<INT32> positions ;
   for( INT32 i = 1;i <= 7;i++ )
   {
      positions.push_back( i ) ;
   }
   INT32 totalCnt = 50 ;
   INT32 masterCnt = 0 ;
   INT32 newMasterCnt = 0 ;
   for( INT32 i = 0;i < totalCnt;i++ )
   {
      sdbNode slave ;
      rc = rg.getSlave( slave, positions ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave" ;
      const CHAR* slaveSvc = slave.getServiceName() ;
      if( !strcmp( slaveSvc, masterSvc ) )
         masterCnt++ ;
      else if( !strcmp( slaveSvc, newMasterSvc ) )
         newMasterCnt++ ;
   }
   ASSERT_NE( 0, masterCnt ) << "fail to check old master cnt" ;
   ASSERT_EQ( 0, newMasterCnt ) << "fail to check new master cnt" ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
