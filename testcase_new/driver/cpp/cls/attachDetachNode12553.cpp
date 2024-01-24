/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12553:数据组挂载/卸载节点
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

class attachDetachNodeTest12553 : public testBase
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
      rgName = "attachDetachNodeTestRg12553" ; 
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

TEST_F( attachDetachNodeTest12553, attachDetachNode12553 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // get a existed data group rg1
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get groups" ;
   ASSERT_GT( groups.size(), 0 ) << "no data group" ;
   const CHAR* rgName1 = groups[0].c_str() ;
   sdbReplicaGroup rg1 ;
   rc = db.getReplicaGroup( rgName1, rg1 ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to get rg " << rgName1 ;

   // create node in rg1
   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName = ARGS->rsrvPortBegin() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath[100] ;
   sprintf( dbPath, "%s%s%s%s", nodeDir, "data/", svcName, "/" ) ;
   cout << "node: " << hostName << " " << svcName << " " << dbPath << endl ;
   rc = rg1.createNode( hostName, svcName, dbPath ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node" ;

   // detach node from rg1
   BSONObj opt = BSON("KeepData" << true);
   rc = rg1.detachNode( hostName, svcName, opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to detach node" ;
   sdbNode node ;
   rc = rg1.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to check detach" ;

   // attach node to rg
   rc = rg.attachNode( hostName, svcName, opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to attach node" ;
   rc = rg.getNode( hostName, svcName, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check attach" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
