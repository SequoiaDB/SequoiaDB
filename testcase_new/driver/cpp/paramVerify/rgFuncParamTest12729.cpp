/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12729:getNode参数校验
 *                 seqDB-12730:createNode参数校验
 *                 seqDB-12731:removeNode参数校验
 *                 seqDB-12732:attachNode参数校验
 *                 seqDB-12733:detachNode参数校验
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class rgFuncParamTest12729 : public testBase
{
protected:
   const CHAR* rgName ;
   sdbReplicaGroup rg ;
   
   void SetUp()
   {
      testBase::SetUp() ;
   }

   void TearDown()
   {
      testBase::TearDown() ;
   }
   
   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "rgFuncParamTestRg12729" ;
      rc = db.createReplicaGroup( rgName, rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to create rg %s", rgName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.removeReplicaGroup( rgName ) ;
         CHECK_RC( SDB_OK, rc, "fail to remove rg %s", rgName ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( rgFuncParamTest12729, getNode12729 )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbNode node ;
   rc = rg.getNode( NULL, node ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getNode with NULL" ;

   const CHAR* nodeName = "hostname" ;
   rc = rg.getNode( nodeName, node ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getNode with illegal nodeName" ;
   
   rc = rg.getNode( NULL, "11810", node ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getNode with hostname NULL" ;

   rc = rg.getNode( "localhost", NULL, node) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getNode with svcname NULL" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( rgFuncParamTest12729, createNode12730 )
{
   INT32 rc = SDB_OK ;
   
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName = ARGS->rsrvPortBegin() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath[100] ;
   sprintf( dbPath, "%s%s%s%s", nodeDir, "data/", svcName, "/" ) ; 
   cout << "node: " << hostName << " " << svcName << " " << dbPath << endl ;

   // createNode with NULL
   rc = rg.createNode( NULL, svcName, dbPath ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test createNode with hostname NULL" ;
   rc = rg.createNode( hostName, NULL, dbPath ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test createNode with svcName NULL" ;
   rc = rg.createNode( hostName, svcName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test createNode with dbPath NULL" ;

   // createNode with conflict config
   BSONObj config = BSON( "GroupName" << "haha" << "HostName" << "jane" <<
                          "svcname" << "11810" << "dbpath" << "somewhere" ) ;
   rc = rg.createNode( hostName, svcName, dbPath, config ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test createNode with conflict config" ;

   // check hostName svcName dbPath
   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg " << rgName ;
   sdbNode node ;
   rc = rg.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node" ;
   ASSERT_STREQ( hostName, node.getHostName() ) << "fail to check hostName" ;
   ASSERT_STREQ( svcName, node.getServiceName() ) << "fail to check svcName" ;
   sdb db1 ;
   rc = node.connect( db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect node" ;
   sdbCursor cursor ;
   rc = db1.getSnapshot( cursor, SDB_SNAP_SYSTEM ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot sys" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( dbPath, obj.getField( "Disk" ).Obj().getField( "DatabasePath" ).String() ) << "fail to check dbPath" ;
   db1.disconnect() ;
   
   // stop and removeNode
   rc = rg.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop rg " << rgName ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( rgFuncParamTest12729, removeNode12731 )
{
   INT32 rc = SDB_OK ;
   
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName = ARGS->rsrvPortBegin() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath[100] ;
   sprintf( dbPath, "%s%s%s%s", nodeDir, "data/", svcName, "/" ) ;
   cout << "node: " << hostName << " " << svcName << " " << dbPath << endl ;
   
   // removeNode with NULL
   rc = rg.removeNode( NULL, svcName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test removeNode with hostName NULL" ;
   rc = rg.removeNode( hostName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test removeNode with svcName NULL" ;

   // removeNode with conflict config
   // create two node in rg first, then remove
   rc = rg.createNode( hostName, svcName, dbPath ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node" ;
   const CHAR* svcName1 = ARGS->rsrvPortEnd() ;
   CHAR dbPath1[100] ;
   sprintf( dbPath1, "%s%s%s%s", nodeDir, "data/", svcName1, "/" ) ;
   rc = rg.createNode( hostName, svcName1, dbPath1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node" ;

   BSONObj config = BSON( "GroupName" << "rm" << "HostName" << "jane" << "svcname" << "11810" ) ;
   rc = rg.removeNode( hostName, svcName, config ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test removeNode with conflict config" ;
   sdbNode node ;
   rc = rg.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to check removeNode" ; 

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( rgFuncParamTest12729, attachNode12732 )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = rg.attachNode( NULL, "11810", BSON("KeepData" << false) ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test attachNode with hostName NULL" ;
   rc = rg.attachNode( "localhost", NULL, BSON("KeepData" << false) ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test attachNode with svcName NULL" ; 

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( rgFuncParamTest12729, detachNode12733 )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = rg.detachNode( NULL, "11810", BSON("KeepData" << false) ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test detachNode with hostName NULL" ;
   rc = rg.detachNode( "localhost", NULL, BSON("KeepData" << false) ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test detachNode with svcName NULL" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
