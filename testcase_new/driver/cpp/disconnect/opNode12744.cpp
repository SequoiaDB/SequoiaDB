/**************************************************************
 * @Description: opreate node object after disconnect
 *               seqDB-12744 : opreate node object after disconnect
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

class opNode12744 : public testBase 
{
protected:
   sdbNode node ; 

   void SetUp() 
   {
      testBase::SetUp() ;

      if( isStandalone( db ) ) 
         return ;

      INT32 rc = SDB_OK ;
      sdbReplicaGroup rg ;
      rc = db.getReplicaGroup( "SYSCatalogGroup", rg ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get rg" ;
      rc = rg.getMaster( node ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get node" ;
      db.disconnect() ;
   }

   void TearDown() 
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( opNode12744, opNode ) 
{
   if( isStandalone( db ) ) 
   {
      cout << "skip this test for standalone" << endl ; 
      return ;
   }

   // test all interfaces of class sdbNode except connect(), getHostName(), getServiceName(), getNodeName(), getStatus()
   // in the order of c++ api doc

   INT32 rc = SDB_OK ;
   rc = node.stop() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "stop node shouldn't succeed" ;
   rc = node.start() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "start node shouldn't succeed" ;
}

