/************************************************************
 * @Description: test case for Jira questionaire 
 *				     SEQUOIADBMAINSTREAM-809
 * @Modify:      Liang xuewang
 *				     2016-11-11
 *************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class attachAndDetachNodeTest : public testBase
{
protected:
   sdbReplicaGroupHandle rg, tmpRg ;
   sdbNodeHandle tmpNode ;
   const CHAR* tmpRgName ;
   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   const CHAR* svcName ;

   void SetUp() 
   {
      testBase::SetUp() ;

      // get a data group
      INT32 rc = SDB_OK ;
      vector<string> groups ;
      rc = getGroups( db, groups ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      const CHAR* rgName = groups[0].c_str() ;
      rc = sdbGetReplicaGroup( db, rgName, &rg ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get rg " << rgName ;

      // create tmp node in data group
      svcName = ARGS->rsrvPortBegin() ;
      CHAR dbPath[100] ;
      sprintf( dbPath, "%s%s%s", ARGS->rsrvNodeDir(), "data/", svcName ) ;
      memset( hostName, 0, MAX_NAME_SIZE+1 ) ;
      rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCreateNode( rg, hostName, svcName, dbPath, NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create node " << hostName << ":" << svcName 
                              << " dbPath: " << dbPath ;

      // create tmp data group
      tmpRgName = "attachAndDetachNodeTestRg" ;
      rc = sdbCreateReplicaGroup( db,tmpRgName, &tmpRg ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create rg " << tmpRgName ;
   }
   void TearDown() 
   {
      sdbReleaseNode( tmpNode ) ;
      sdbReleaseReplicaGroup( tmpRg ) ;
      sdbReleaseReplicaGroup( rg ) ;
      testBase::TearDown() ; 
   }  
} ;

TEST_F( attachAndDetachNodeTest, onlyAttachOnlyDetach )
{
   INT32 rc = SDB_OK ;
 
   // detach tmp node from data group
   bson option;
   bson_init( &option );
   bson_append_bool( &option,"KeepData", true ) ;
   bson_finish( &option ) ;
   rc = sdbDetachNode( rg, hostName, svcName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to detach temp node" ;
   rc = sdbGetNodeByHost( rg, hostName, svcName, &tmpNode ) ;
   ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to check detach" ;

   // attach tmp node to tmp data group
   rc = sdbAttachNode( tmpRg, hostName, svcName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to attach tmp node" ;
   rc = sdbGetNodeByHost( tmpRg, hostName, svcName, &tmpNode ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check attach" ;
   bson_destroy( &option ) ;

   // start tempRG
   rc = sdbStartReplicaGroup( tmpRg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start tmp data group " << tmpRgName ;

   // stop tempRG
   rc = sdbStopReplicaGroup( tmpRg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop tmp data group " << tmpRgName ;

   // remove tempRG
   rc = sdbRemoveReplicaGroup( db, tmpRgName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove tmp data group " << tmpRgName ;
}
