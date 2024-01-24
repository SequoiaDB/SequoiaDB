/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12689:创建/启动/停止节点/获取节点信息
 *                 seqDB-12692:获取不存在的节点
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

class nodeOprTest12689 : public testBase
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
      rgName = "nodeOprTestRg12689" ; 
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

TEST_F( nodeOprTest12689, nodeOpr12689 ) 
{
   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // create two node in rg( remove node cannot remove the only one node )
   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName1 = ARGS->rsrvPortBegin() ;
   const CHAR* svcName2 = ARGS->rsrvPortEnd() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath1[100] ;
   sprintf( dbPath1, "%s%s%s%s", nodeDir, "data/", svcName1, "/" ) ;
   CHAR dbPath2[100] ;
   sprintf( dbPath2, "%s%s%s%s", nodeDir, "data/", svcName2, "/" ) ;
   cout << "node1: " << hostName << " " << svcName1 << " " << dbPath1 << endl ;
   cout << "node2: " << hostName << " " << svcName2 << " " << dbPath2 << endl ;
   rc = rg.createNode( hostName, svcName1, dbPath1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node1" ;
   rc = rg.createNode( hostName, svcName2, dbPath2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node2" ;

   // get node
   sdbNode node1 ;
   CHAR nodeName1[100] ;
   sprintf( nodeName1, "%s%s%s", hostName, ":", svcName1 ) ;
   rc = rg.getNode( hostName, svcName1, node1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node1" ;
   rc = rg.getNode( nodeName1, node1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node1" ;

   // check node info
   ASSERT_STREQ( hostName, node1.getHostName() ) << "fail to check hostName" ;
   ASSERT_STREQ( svcName1, node1.getServiceName() ) << "fail to check svcName1" ;
   ASSERT_STREQ( nodeName1, node1.getNodeName() ) << "fail to check nodeName1" ;

   // start node and connect
   rc = node1.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start node1" ;
   sdb db1 ;
   rc = node1.connect( db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect node1" ;
   ASSERT_TRUE( db1.isValid() ) << "fail to check connect node1" ;
   db1.disconnect() ;

   // stop node and check
   rc = node1.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node1" ;
   rc = node1.connect( db1 ) ;
   ASSERT_EQ( SDB_NET_CANNOT_CONNECT, rc ) << "fail to check stop node1" ;

   // remove node and check
   rc = rg.removeNode( hostName, svcName1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove node1" ;
   rc = rg.getNode( hostName, svcName1, node1 ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to check remove node1" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( nodeOprTest12689, notExistNode12692 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   
   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbNode node ;
   const CHAR* nodeName = "localhost:11810" ;
   rc = rg.getNode( nodeName, node ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to test get not exist node " << nodeName ;

   const CHAR* hostName = "localhost" ;
   const CHAR* svcName = "11810" ;
   rc = rg.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to test get not exist node " << hostName << " " << svcName ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
