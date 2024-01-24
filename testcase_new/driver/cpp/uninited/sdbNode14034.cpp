/**************************************************************
 * @Description: use sdbNode without init
 *               seqDB-14034:sdbNode对象未创建时调用方法
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

class sdbNode14034 : public testBase 
{
protected:
   sdbNode node ; 
   void SetUp() 
   {
   }
   void TearDown() 
   {
   }
} ;

TEST_F( sdbNode14034, opNode ) 
{
   // test all function of sdbNode except getStatus()

   INT32 rc = SDB_OK ;

   sdb db ;
   rc = node.connect( db ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "connect shouldn't succeed" ;
   EXPECT_FALSE( node.getHostName() ) << "getHostName should return NULL" ;
   EXPECT_FALSE( node.getServiceName() ) << "getServiceName should return NULL" ;
   EXPECT_FALSE( node.getNodeName() ) << "getNodeName should return NULL" ;
   rc = node.stop() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "stop node shouldn't succeed" ;
   rc = node.start() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "start node shouldn't succeed" ;
}

