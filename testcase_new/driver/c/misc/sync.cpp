/**************************************************************
 * @Description: test case of sync 
 *				     TestLink 9368  
 *               seqDB-9368:c驱动sdbSyncDB验证
 * @Modify     : Liang xuewang Init
 *			 	     2017-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "arguments.hpp"

TEST( sdbSyncDB, normal )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;

   // connect to sdb	
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;

   // get data group and data node
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   INT32 groupId = 1000 ;
   rc = sdbGetReplicaGroup1( db, groupId, &rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get data group" ;
   CHAR* rgName = NULL ;
   rc = sdbGetReplicaGroupName( rg, &rgName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get data group name" ;
   rc = sdbGetNodeSlave( rg, &node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node of data group" ;
   const CHAR* hostName = NULL ;
   const CHAR* svcName = NULL ;
   const CHAR* nodeName = NULL ;
   INT32 nodeId = -1 ;
   rc = sdbGetNodeAddr( node, &hostName, &svcName, &nodeName, &nodeId ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node addr" ;
   printf( "node: hostName=%s svcName=%s nodeName=%s nodeId=%d\n", hostName, svcName, nodeName, nodeId ) ;

   // create domain with data group
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   const CHAR* domainName = "syncTestDomain" ;
   bson option ;
   bson_init( &option ) ;
   bson_append_start_array( &option, "Groups" ) ;
   bson_append_string( &option, "0", rgName ) ;
   bson_append_finish_array( &option ) ;
   bson_finish( &option ) ;
   rc = sdbCreateDomain( db, domainName, &option, &domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create domain " << domainName ;
   bson_destroy( &option ) ;

   // create cs cl in domain
   const CHAR* csName = "syncTestCs" ;
   bson_init( &option ) ;
   bson_append_string( &option, "Domain", domainName ) ;
   bson_finish( &option ) ;
   rc = sdbCreateCollectionSpaceV2( db, csName, &option, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   bson_destroy( &option ) ;
   const CHAR* clName = "syncTestCl" ;
   bson_init( &option ) ; 
   bson_append_int( &option, "ReplSize", -1 ) ; 
   bson_finish( &option ) ; 
   rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   bson_destroy( &option ) ;

   // sync cs in data node
   bson_init( &option ) ;
   bson_append_int( &option, "Deep", 1 ) ;
   bson_append_bool( &option, "Block", false ) ;
   bson_append_string( &option, "CollectionSpace", csName ) ;
   bson_append_bool( &option, "Global", false ) ;
   bson_append_int( &option, "GroupId", groupId ) ;
   bson_append_string( &option, "GroupName", rgName ) ;
   bson_append_int( &option, "NodeID", nodeId ) ;
   bson_append_string( &option, "HostName", hostName ) ;
   bson_append_string( &option, "svcname", svcName ) ;
   bson_finish( &option ) ;
   rc = sdbSyncDB( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sync db" ;
   bson_destroy( &option ) ;

   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   rc = sdbDropDomain( db, domainName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain " << domainName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseDomain( domain ) ;
   sdbReleaseConnection( db ) ;
}
