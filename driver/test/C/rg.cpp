#include <stdio.h>
#include <gtest/gtest.h>
#include "client.h"
#include "testcommon.h"
#include <string>
#include <iostream>
#include <stdio.h>

using namespace std ;

sdbConnectionHandle db ;
sdbCSHandle cs ;
sdbCollectionHandle cl ;
sdbReplicaGroupHandle rg ;
sdbCursorHandle cur ;

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
class replicaGroupTest : public testing::Test
{
   public:
      replicaGroupTest() {}

   public:
      static void SetUpTestCase() ;

      static void TearDownTestCase() ;

      virtual void SetUp() ;

      virtual void TearDown() ;
} ;

void replicaGroupTest::SetUpTestCase()
{
   INT32 rc = SDB_OK ;
   bson option ;

   rc = sdbConnect( pHostName, pSvcName, pUser, pPassword, &db ) ;
   if ( SDB_OK != rc )
   {
      cout << "Error: Failed to connect to database: rc = " << rc << endl ;
      return ;
   }
   else
   {
      connect_flag = TRUE ;
   }
   if ( TRUE == isCluster( db ) )
   {
      is_cluster = TRUE ;
   }
   else
   {
      return ;
   }
   
   rc = sdbCreateReplicaGroup( db, pGroupName, &rg ) ;
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
   bson_init( &option ) ;
   bson_append_int( &option, "logfilenum", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbCreateNode( rg, pNodeHostName, pNodeSvcName, pNodePath, &option ) ;
   bson_destroy( &option ) ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to create data node[%s:%s] in replica group[%s], "
               "rc = %d", pNodeHostName, pNodeSvcName, pGroupName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   rc = sdbStartReplicaGroup( rg ) ;
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
      rc = sdbRemoveReplicaGroup( db, pGroupName ) ;
      if ( SDB_OK != rc )
      {
         sprintf( tmp_buf, "Error: Failed to remove replica group[%s], rc = %d",
                  pGroupName, rc ) ;
         cout << tmp_buf << endl ;
      }
   }
   sdbDisconnect( db ) ; 
}

void replicaGroupTest::SetUp()
{
   INT32 rc = SDB_OK ;
   bson option ;
   
   if ( FALSE == is_cluster )
      return ;

   create_node_flag2 = FALSE ;
   bson_init( &option ) ;
   bson_append_int( &option, "logfilenum", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbCreateNode( rg, pNodeHostName, pNodeSvcName2, pNodePath2, &option ) ;
   if ( SDB_OK != rc )
   {
      sprintf( tmp_buf, "Error: Failed to create data node[%s:%s] in "
               "replica group[%s], rc = %d", pNodeHostName,
               pNodeSvcName2, pGroupName, rc ) ;
      cout << tmp_buf << endl ;
      return ;
   }
   rc = sdbStartReplicaGroup( rg ) ;
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

   rc = sdbRemoveNode( rg, pNodeHostName2, pNodeSvcName2, NULL ) ;
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

TEST_F( replicaGroupTest, init_test )
{
   if ( FALSE == is_cluster )
      return ;
   ASSERT_EQ( TRUE, connect_flag ) << "Error: Failed to connect to database" ;
   ASSERT_EQ( TRUE, create_rg_flag ) << "Error: Failed to create rg in database" ; 
   ASSERT_EQ( TRUE, create_node_flag ) << "Error: Failed to create data node in database" ;
   ASSERT_EQ( TRUE, create_node_flag2 ) << "Error: Failed to create data node in database" ;
}

TEST_F( replicaGroupTest, getRGName )
{
   if ( FALSE == is_cluster )
      return ;
   ASSERT_EQ( TRUE, connect_flag ) << "Error: Failed to connect to database" ;
   ASSERT_EQ( TRUE, create_rg_flag ) << "Error: Failed to create rg in database" ;
   ASSERT_EQ( TRUE, create_node_flag ) << "Error: Failed to create data node in database" ;
   ASSERT_EQ( TRUE, create_node_flag2 ) << "Error: Failed to create data node in database" ;

   INT32 rc                 = SDB_OK ;
   CHAR pBuffer[ NAME_LEN ] = { 0 } ;
   CHAR *pBuffer2           = NULL ;

   rc = sdbGetRGName( rg, pBuffer, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, pGroupName, strlen(pGroupName) ) ) ;

   rc = sdbGetRGName( rg, pBuffer, 1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;
 
   rc = sdbGetRGName( rg, pBuffer2, 1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( replicaGroupTest, detachNode )
{
   if ( FALSE == is_cluster )
   {
      cout << "Warning: it's not a cluster environment." << endl ;
      return ;
   }
   ASSERT_EQ( TRUE, connect_flag ) << "Error: Failed to connect to database" ;
   ASSERT_EQ( TRUE, create_rg_flag ) << "Error: Failed to create rg in database" ;
   ASSERT_EQ( TRUE, create_node_flag ) << "Error: Failed to create data node in database" ;
   ASSERT_EQ( TRUE, create_node_flag2 ) << "Error: Failed to create data node in database" ;

   INT32 rc = SDB_OK ;
   sdbNodeHandle node ;
   rc = sdbDetachNode( rg, pNodeHostName2, pNodeSvcName2, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to detach data node from group " <<
      pGroupName << ", rc = " << rc ;

   rc = sdbGetNodeByHost( rg, pNodeHostName2, pNodeSvcName2, &node ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "What we expect is "
      "SDB_CLS_NODE_NOT_EXIST, but rc = " << rc ;

   rc = sdbAttachNode( rg, pNodeHostName2, pNodeSvcName2, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to attach data node to group " <<
      pGroupName << ", rc = " << rc ;
  
   rc = sdbGetNodeByHost( rg, pNodeHostName2, pNodeSvcName2, &node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to get data node from group " <<
      pGroupName << ", rc = " << rc ;

}

