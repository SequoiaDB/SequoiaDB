#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include "deleteFile.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

/*************************************
 *           test begine             *
 *************************************/

TEST(node, not_connect)
{
   sdbNode node ;
   BSONObj obj ;
   INT32 rc = SDB_OK ;

   rc = node.getStatus() ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST(node,getStatus)
{
   sdb connect ;
   sdbShard shard ;
   sdbNode node ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // connect to database
   rc = connect.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connect.getShard("group1", shard) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = shard.getNode("ubuntu-dev1", "51000", node);
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbNodeStatus status = SDB_NODE_UNKNOWN;
   status = node.getStatus();
   cout << "status = " << status << endl ;
   // disconnect the connection
   connect.disconnect() ;
}

// TODO:
//all
