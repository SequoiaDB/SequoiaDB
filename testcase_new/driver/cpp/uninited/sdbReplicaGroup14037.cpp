/**************************************************************
 * @Description: test sdbReplicaGroup without init
 *               seqDB-14037:sdbReplicaGroup对象未创建时调用方法
 * @Modify     : Liang xuewang
 *               2018-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class sdbReplicaGroup14037 : public testBase 
{
protected:
   sdbReplicaGroup rg ;
   void SetUp() 
   {
   }
   void TearDown()
   {
   }
} ;

TEST_F( sdbReplicaGroup14037, opRG ) 
{
   // test all function of sdbReplicaGroup
   
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
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get node(1) shouldn't succeed" ;
   rc = rg.getNode( "localhost", "1234", node ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get node(2) shouldn't succeed" ;

   // create/remove node
   rc = rg.createNode( "localhost", "1234", "/tmp/" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create node(2) shouldn't succeed" ;
   rc = rg.removeNode( "localhost", "1234" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove node shouldn't succeed" ;

   // start/stop
   rc = rg.stop() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "stop group shouldn't succeed" ;
   rc = rg.start() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "start group shouldn't succeed" ;

   EXPECT_FALSE( rg.getName() ) << "getName should be NULL" ;
   EXPECT_FALSE( rg.isCatalog() ) << "isCatalog should be FALSE" ;

   // attach/detach
   rc = rg.attachNode( "localhost", "1234", BSON( "KeepData" << false ) ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "attach node shouldn't succeed" ;
   rc = rg.detachNode( "localhost", "1234", BSON( "KeepData" << false ) ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "detach node shouldn't succeed" ;
   rc = rg.reelect() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "reelect shouldn't succeed" ;
}
