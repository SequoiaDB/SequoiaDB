/**************************************************************
 * @Description: test case for Jira questionaire Task
 *				     SEQUOIADBMAINSTREAM-2165
 *				     seqDB-10995:unloadCS，指定的option可生效
 *				     seqDB-10996:unloadCS，指定的option不生效
 *				     seqDB-10997:loadCS，指定的option可生效
 *				     seqDB-10998:loadCS，指定的option不生效
 * @Modify     : Liang xuewang Init
 *			 	     2017-01-22
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class loadUnloadCSTest : public testBase
{
protected:
   sdbReplicaGroupHandle rg ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* rgName ;
   const CHAR* csName ;
   const CHAR* clName ;

   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
   
   INT32 init()
   {
      INT32 rc = SDB_OK ;
      vector<string> groups ;
      bson option ;

      // get data group
      rc = getGroups( db, groups ) ;
      CHECK_RC( SDB_OK, rc, "fail to get data groups" ) ;
      rgName = groups[0].c_str() ;
      rc = sdbGetReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to get data group %s", rgName ) ;

      // create cs
      csName = "loadUnloadCSTestCs" ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cs %s", csName ) ;

      // create cl in data group
      clName = "loadUnloadCSTestCl" ;
      bson_init( &option ) ;
      bson_append_string( &option, "Group", rgName ) ;
      bson_append_int( &option, "ReplSize", 0 ) ;
      bson_finish( &option ) ;
      rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
      bson_destroy( &option ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cl %s", clName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      CHECK_RC( SDB_OK, rc, "fail to drop cs %s", csName ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      sdbReleaseReplicaGroup( rg ) ;
   done: 
      return rc ;
   error:
      goto done ;
   }
} ;

INT32 checkCsExist( sdbConnectionHandle db, const CHAR* csName, BOOLEAN* exist )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;
   bson_init( &obj ) ;
   *exist = FALSE ;
   rc = sdbListCollectionSpaces( db, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list cs" ) ;
   while( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      const CHAR* name = bson_iterator_string( &it ) ;
      if( !strcmp( name, csName ) )
      {
         *exist = TRUE ;
         break ;
      }
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }

done:
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

INT32 checkBasicOperation( sdbCollectionHandle cl )
{
   INT32 rc = SDB_OK ;
   CHAR clFullName[200] ;
   bson record ;
   bson_init( &record ) ;
   bson selector ;
   bson_init( &selector ) ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;

   // insert
   bson_append_int( &record, "a", 1 ) ;
   bson_finish( &record ) ;
   rc = sdbInsert( cl, &record ) ;
   CHECK_RC( SDB_OK, rc, "fail to insert record" ) ;

   // query
   bson_append_string( &selector, "a", "" ) ;
   bson_finish( &selector ) ;
   rc = sdbQuery( cl, &record, &selector, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to query record" ) ;
   rc = sdbCloseCursor( cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;

done:
   bson_destroy( &record ) ;
   bson_destroy( &selector ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

TEST_F( loadUnloadCSTest, validOption )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )  
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   const CHAR* hostName = NULL ;
   const CHAR* svcName = NULL ;
   const CHAR* nodeName = NULL ;
   INT32 nodeId = -1 ;

   // check cl basic Operation
   rc = checkBasicOperation( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get node hostName and svcname
   rc = sdbGetNodeSlave( rg, &node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get slave node" ;
   rc = sdbGetNodeAddr( node, &hostName, &svcName, &nodeName, &nodeId ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node addr" ;
   printf( "node: hostName=%s svcName=%s nodeName=%s nodeId=%d\n", hostName, svcName, nodeName, nodeId ) ;

   // unload cs on node
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "HostName", hostName ) ;
   bson_append_string( &option, "svcname", svcName ) ;
   bson_finish( &option ) ;
   rc = sdbUnloadCollectionSpace( db, csName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to unload cs" ;

   // check unload
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
   rc = sdbConnect( hostName, svcName, ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   BOOLEAN exist ;
   rc = checkCsExist( db, csName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check unload cs" ;

   // load cs on node
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   rc = sdbLoadCollectionSpace( db, csName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to load cs" ;

   // check load
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
   rc = sdbConnect( hostName, svcName, ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   rc = checkCsExist( db, csName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check load cs" ;

   // basic operation after load
   sdbReleaseCS( cs ) ;
   sdbReleaseCollection( cl ) ;
   rc = sdbGetCollectionSpace( db, csName, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs " << csName ;
   rc = sdbGetCollection1( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName ;
   rc = checkBasicOperation( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;	

   // reconnect to sdb
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( loadUnloadCSTest, invalidOption )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) ) 
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "GroupName", "SYSCatalogGroup" ) ;
   bson_finish( &option ) ;
   rc = sdbUnloadCollectionSpace( db, csName, &option ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to check unload cs on SYSCatalogGroup" ;
   rc = sdbLoadCollectionSpace( db, csName, &option ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to check load cs on SYSCatalogGroup" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ; 
}
