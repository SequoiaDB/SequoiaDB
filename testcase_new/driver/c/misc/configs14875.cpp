/********************************************************
 * @Description: test case for c driver
 *               seqDB-14875:更新删除节点配置
 * @Modify:      Liangxw
 *               2018-03-30
 ********************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class confTest14875 : public testBase
{
protected:
   sdbReplicaGroupHandle rg ;
   CHAR host[ MAX_NAME_SIZE+1 ] ;
   CHAR svc[ MAX_NAME_SIZE+1 ] ;
   CHAR dbPath[ MAX_NAME_SIZE+1 ] ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      rg = SDB_INVALID_HANDLE ;
      memset( host, 0, MAX_NAME_SIZE+1 ) ;
      memset( svc, 0, MAX_NAME_SIZE+1 ) ;
      memset( dbPath, 0, MAX_NAME_SIZE+1 ) ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }

   INT32 init()
   {
      INT32 rc = SDB_OK ;
      vector<string> groups ;
      const CHAR* rgName ;
      sdbNodeHandle node ;

      rc = getGroups( db, groups ) ;
      CHECK_RC( SDB_OK, rc, "fail to get groups" ) ;
      rgName = groups[0].c_str() ;
      rc = sdbGetReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to get group" ) ;

      rc = getDBHost( db, host, MAX_NAME_SIZE ) ;
      CHECK_RC( SDB_OK, rc, "fail to get hostname" ) ;
      strcpy( svc, ARGS->rsrvPortBegin() ) ;
      sprintf( dbPath, "%s%s%s", ARGS->rsrvNodeDir(), "data/", svc ) ;
      printf( "create node: %s:%s, dbPath: %s\n", host, svc, dbPath ) ;

      rc = sdbCreateNode( rg, host, svc, dbPath, NULL ) ;
      CHECK_RC( SDB_OK, rc, "fail to create node" ) ;
      rc = sdbGetNodeByHost( rg, host, svc, &node ) ;
      CHECK_RC( SDB_OK, rc, "fail to get node" ) ;
      rc = sdbStartNode( node ) ;
      CHECK_RC( SDB_OK, rc, "fail to start node" ) ;
      sdbReleaseNode( node ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      
      printf( "remove node: %s:%s\n", host, svc ) ;
      rc = sdbRemoveNode( rg, host, svc, NULL ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove node" ) ;
      sdbReleaseReplicaGroup( rg ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 getNodeWeight( INT32* weight )
   {
      sdbCursorHandle cursor ;
      bson cond ;
      bson obj ;   
      bson_iterator it ;
      INT32 rc = SDB_OK ;

      bson_init( &cond ) ;
      bson_append_string( &cond, "HostName", host ) ;
      bson_append_string( &cond, "svcname", svc ) ;
      bson_finish( &cond ) ;
      rc = sdbGetSnapshot( db, SDB_SNAP_CONFIGS, &cond, NULL, NULL, &cursor ) ;
      CHECK_RC( SDB_OK, rc, "fail to snapshot conf" ) ;
      bson_destroy( &cond ) ;
   
      bson_init( &obj ) ;
      rc = sdbNext( cursor, &obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
      
      bson_find( &it, &obj, "weight" ) ;
      *weight = bson_iterator_int( &it ) ;
      bson_destroy( &obj ) ;
      
      rc = sdbCloseCursor( cursor ) ;
      CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;
      sdbReleaseCursor( cursor ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( confTest14875, upDelConf )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone" ) ;
      return ;
   }
   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // update weight to 20 on data node
   bson config ;
   bson_init( &config ) ;
   bson_append_int( &config, "weight", 20 ) ;
   bson_finish( &config ) ;
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "HostName", host ) ;
   bson_append_string( &option, "svcname", svc ) ;
   bson_finish( &option ) ;
   rc = sdbUpdateConfig( db, &config, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update conf" ;
   bson_destroy( &config ) ;

   // check update
   INT32 weight = 0 ;
   rc = getNodeWeight( &weight ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 20, weight ) << "fail to check update" ;

   // delete weight
   bson_init( &config ) ; 
   bson_append_int( &config, "weight", 1 ) ;
   bson_finish( &config ) ;
   rc = sdbDeleteConfig( db, &config, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete conf" ;

   // check delete
   rc = getNodeWeight( &weight ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 10, weight ) << "fail to check delete" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
