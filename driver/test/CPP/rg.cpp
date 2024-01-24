// TODO: need to change pNodeHostName, pNodeSvcName
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

sdb db ;
sdbCollectionSpace cs ;
sdbCollection cl ;
sdbReplicaGroup rg ;
sdbCursor cur ;

BOOLEAN is_cluster         = FALSE ;
BOOLEAN connect_flag       = FALSE ;
BOOLEAN create_rg_flag     = FALSE ;
BOOLEAN create_node_flag   = FALSE ;
BOOLEAN create_node_flag2  = FALSE ;

const CHAR *pHostName      = HOST ;
const CHAR *pSvcName       = SERVER ;
const CHAR *pUser          = USER ;
const CHAR *pPassword      = PASSWD ;

const CHAR *pGroupName     = "testGroupInCpp" ;
const CHAR *pNodeHostName  = "susetzb" ;
const CHAR *pNodeSvcName   = "31100" ;
const CHAR *pNodePath      = "/opt/sequoiadb/database/data/31100" ;
const CHAR *pNodeHostName2 = "susetzb" ;
const CHAR *pNodeSvcName2  = "31200" ;
const CHAR *pNodePath2     = "/opt/sequoiadb/database/data/31200" ;
#define tmp_buf_size 1024
CHAR tmp_buf[tmp_buf_size + 1] = { 0 } ;

/*
 * @description: test for replica group's api
 * @author: tanzhaobo
 */
//class replicaGroupTest : public testing::Environment
class replicaGroupTest : public testing::Test
{
   public:
      replicaGroupTest() {}

   public:
      // run before all the testcase
      static void SetUpTestCase() ;

      // run before all the testcase
      static void TearDownTestCase() ;

      // run before every testcase
      virtual void SetUp() ;

      // run before every testcase
      virtual void TearDown() ;
} ;

void replicaGroupTest::SetUpTestCase()
{
   INT32 rc = SDB_OK ;
   BSONObj option ;

   // connect
   rc = db.connect( pHostName, pSvcName, pUser, pPassword ) ;
   if ( SDB_OK != rc )
   {
      cout << "Error: Failed to connect to database: rc = " << rc << endl ;
      return ;
   }
   else
   {
      connect_flag = TRUE ;
   }
   // check it's in cluster env or not
   if ( TRUE == isCluster( db ) )
   {
      is_cluster = TRUE ;
   }
   else
   {
      return ;
   }
   // create rg
   rc = db.createReplicaGroup( pGroupName, rg ) ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to create replica group[%s] rc = %d",
               pGroupName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   else
   {
      create_rg_flag = TRUE ;
   }
   // create node
   option = BSON( "logfilenum" << 1 ) ;
   rc = rg.createNode( pNodeHostName, pNodeSvcName, pNodePath, option ) ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to create data node[%s:%s] in replica group[%s], "
               "rc = %d", pNodeHostName, pNodeSvcName, pGroupName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   // start node
   rc = rg.start() ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to start data node[%s:%s] in replica group, "
               "rc = %d", pNodeHostName, pNodeSvcName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   else
   {
      create_node_flag = TRUE ;
   }
} 

void replicaGroupTest::TearDownTestCase()
{
   INT32 rc = SDB_OK ;
   if ( TRUE == is_cluster )
   {
      rc = db.removeReplicaGroup( pGroupName ) ;
      if ( SDB_OK != rc )
      {
         sprintf( tmp_buf, "Error: Failed to remove replica group[%s], rc = %d",
                  pGroupName, rc ) ;
         cout << tmp_buf << endl ;
      }
   }
   db.disconnect() ; 
}

void replicaGroupTest::SetUp()
{
   INT32 rc = SDB_OK ;
   BSONObj option ;

   if ( FALSE == is_cluster )
      return ;

   create_node_flag2 = FALSE ;
   // create node
   option = BSON( "logfilenum" << 1 ) ;
   rc = rg.createNode( pNodeHostName, pNodeSvcName2, pNodePath2, option ) ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to create data node[%s:%s] in "
               "replica group[%s], rc = %d", pNodeHostName,
               pNodeSvcName2, pGroupName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   // start node
   rc = rg.start() ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to start data node in replica "
               "group[%s], rc = %d", pGroupName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   else
   {
      create_node_flag2 = TRUE ;
   }
}

void replicaGroupTest::TearDown()
{
   INT32 rc = SDB_OK ;

   if ( FALSE == is_cluster )
      return ;

   rc = rg.removeNode( pNodeHostName2, pNodeSvcName2 ) ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to remove data node[%s:%s] in replica "
               "group[%s], rc = %d", pNodeHostName2, pNodeSvcName2,
               pGroupName, rc ) ;
      cout << tmp_buf << endl ;
   }
   create_node_flag2 = FALSE ;
}

replicaGroupTest *rg_env ;

INT32 _tmain( INT32 argc, CHAR* argv[] )
{
   testing::InitGoogleTest( &argc, argv ) ;
   return RUN_ALL_TESTS() ;
}

TEST_F( replicaGroupTest, not_connect )
{
   sdbReplicaGroup rg ;
   BSONObj obj ;
   INT32 rc = SDB_OK ;

   rc = rg.getDetail( obj ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST_F( replicaGroupTest, init_test )
{
   if ( FALSE == is_cluster )
      return ;
   ASSERT_EQ( TRUE, connect_flag ) << "Error: Failed to connect to database" ;
   ASSERT_EQ( TRUE, create_rg_flag ) << "Error: Failed to create rg in database" ; 
   ASSERT_EQ( TRUE, create_node_flag ) << "Error: Failed to create data node in database" ;
   ASSERT_EQ( TRUE, create_node_flag2 ) << "Error: Failed to create data node in database" ;
}

TEST_F( replicaGroupTest, detachNode )
{
   if ( FALSE == is_cluster )
   {
      cout << "Warning: it's not a cluter environment." << endl ;
      return ;
   }
   ASSERT_EQ( TRUE, connect_flag ) << "Error: Failed to connect to database" ;
   ASSERT_EQ( TRUE, create_rg_flag ) << "Error: Failed to create rg in database" ;
   ASSERT_EQ( TRUE, create_node_flag ) << "Error: Failed to create data node in database" ;
   ASSERT_EQ( TRUE, create_node_flag2 ) << "Error: Failed to create data node in database" ;

   INT32 rc = SDB_OK ;
   sdbNode node ;
   BSONObj dump ;
   // detach node 
   rc = rg.detachNode( pNodeHostName2, pNodeSvcName2, dump ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to detach data node from group " <<
      pGroupName << ", rc = " << rc ;

   // check
   rc = rg.getNode( pNodeHostName2, pNodeSvcName2, node ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "What we expect is "
      "SDB_CLS_NODE_NOT_EXIST, but rc = " << rc ;

   // attach node
   rc = rg.attachNode( pNodeHostName2, pNodeSvcName2, dump ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to attach data node to group " <<
      pGroupName << ", rc = " << rc ;
  
   // check 
   rc = rg.getNode( pNodeHostName2, pNodeSvcName2, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to get data node from group " <<
      pGroupName << ", rc = " << rc ;

}

// TODO:
/*
getNodeNum
getDetail
getMaster
getSlave
stop
start
getName
isCatalog


*/

