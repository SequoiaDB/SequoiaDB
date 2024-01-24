/**************************************************************
 * @Description: opreate replica group object after disconnect
 *               seqDB-12743 : opreate replica group object after disconnect
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class opRG12743 : public testBase 
{
protected:
   sdbReplicaGroup rg ;
   const CHAR *pRgName ;

   void SetUp() 
   {
      testBase::SetUp() ;
      if( isStandalone( db ) ) 
         return ;

      INT32 rc = SDB_OK ;
      pRgName = "group12743" ;
      rc = db.createReplicaGroup( pRgName, rg ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create rg" ;

      db.disconnect() ;
   }

   void TearDown()
   {
      if( !isStandalone( db ) && shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
         rc = db.removeReplicaGroup( pRgName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to remove rg" ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( opRG12743, opRG ) 
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ; 
      return ;
   }

   // test all interfaces of class sdbReplicaGroup except getNodeNum(), isCatalog(), getName()
   // in the order of c++ api doc
   
   // get detail
   INT32 rc = SDB_OK ;
   BSONObj result ;
   rc = rg.getDetail( result ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get detail shouldn't succeed" ;
   sdbNode node ;
   
   // get master/slave node
   rc = rg.getMaster( node ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get master shouldn't succeed" ;
   rc = rg.getSlave( node ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get slave shouldn't succeed" ;

   // get node
   rc = rg.getNode( "localhost:1234", node ) ; 
   //EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get node(1) shouldn't succeed" ;
   rc = rg.getNode( "localhost", "1234", node ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get node(2) shouldn't succeed" ;

   // create/remove node
   map<string, string> config ;
   rc = rg.createNode( "localhost", "1234", "/tmp/", config ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create node(1) shouldn't succeed" ;
   rc = rg.createNode( "localhost", "1234", "/tmp/" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create node(2) shouldn't succeed" ;
   rc = rg.removeNode( "localhost", "1234" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove node shouldn't succeed" ;

   // start/stop
   rc = rg.stop() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "stop group shouldn't succeed" ;
   rc = rg.start() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "start group shouldn't succeed" ;

   // attach/detach
   rc = rg.attachNode( "localhost", "1234", BSON( "KeepData" << false ) ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "attach node shouldn't succeed" ;
   rc = rg.detachNode( "localhost", "1234", BSON( "KeepData" << false ) ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "detach node shouldn't succeed" ;

   // reelect
   rc = rg.reelect() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "reelect shouldn't succeed" ;
}

