/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2148
 * @Modify     : Liang xuewang Init
 *			 	     2017-01-06
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

class forceSessionTest : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

INT32 getCurrentSessionId( sdbConnectionHandle db, SINT64* sessionId )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition, selector, obj ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &obj ) ;
   bson_iterator it ;

   // list current session
   bson_append_bool( &condition, "Global", false ) ;
   bson_finish( &condition ) ;
   bson_append_string( &selector, "SessionID", "" ) ;
   bson_finish( &selector ) ;
   rc = sdbGetList( db, SDB_LIST_SESSIONS_CURRENT, &condition, &selector, NULL, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list current session" ) ;

   // get session id
   rc = sdbNext( cursor, &obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
   bson_iterator_init( &it, &obj ) ;
   *sessionId = bson_iterator_int( &it ) ;

done:
   bson_destroy( &condition ) ;                            
   bson_destroy( &selector ) ;
   bson_destroy( &obj ) ;	
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

INT32 getNodeSessionIds( sdbConnectionHandle db, const CHAR* nodeName, SINT64 sessionId[], 
                         CHAR sessionType[1024][20], INT32 size )
{
   INT32 rc = SDB_OK ;
   bson condition, selector, obj ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &obj ) ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   INT32 i = 0 ;

   // get node sessions
   bson_append_string( &condition, "NodeName", nodeName ) ;
   bson_append_bool( &condition, "Global", false ) ;
   bson_finish( &condition ) ;
   bson_append_string( &selector, "SessionID", "" ) ;
   bson_append_string( &selector, "Type", "" ) ;
   bson_finish( &selector ) ;
   rc = sdbGetList( db, SDB_LIST_SESSIONS, &condition, &selector, NULL, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list sessions on node %s", nodeName ) ;

   // get session ids
   while( !sdbNext( cursor, &obj ) && i < size )
   {
      bson_iterator it ;
      bson_find( &it, &obj, "SessionID" ) ;
      sessionId[i] = bson_iterator_int( &it ) ;
      bson_find( &it, &obj, "Type" ) ;
      strcpy( sessionType[i], bson_iterator_string( &it ) ) ;
      i++ ;
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }

done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

TEST_F( forceSessionTest, currentSession )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone.\n" ) ;
      return ;
   }

   SINT64 sessionId = -1 ;
   INT32 groupId[] = { 1, 2, 1000 } ;   // catalogRG/coordRG/dataRG
   const CHAR* hostName = NULL ;
   const CHAR* svcName = NULL ;
   const CHAR* nodeName = NULL ;
   INT32 nodeId = -1 ;

   for( INT32 i = 0;i < sizeof(groupId)/sizeof(groupId[0]);i++ )
   {
      // get node addr and connect
      sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
      sdbNodeHandle node = SDB_INVALID_HANDLE ;
      rc = sdbGetReplicaGroup1( db, groupId[i], &rg ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get group " << groupId[i] ;
      rc = sdbGetNodeSlave( rg, &node ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get slave node of group " << groupId[i] ;
      rc = sdbGetNodeAddr( node, &hostName, &svcName, &nodeName, &nodeId ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get node addr " << nodeName  ;
      printf( "node: hostName=%s svcName=%s nodeName=%s nodeId=%d\n", hostName, svcName, nodeName, nodeId ) ;
   
      sdbConnectionHandle nodeDb ;
      rc = sdbConnect( hostName, svcName, ARGS->user(), ARGS->passwd(), &nodeDb ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect node" ;

      // force node current session
      rc = getCurrentSessionId( nodeDb, &sessionId ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get current session id before force";
      rc = sdbForceSession( nodeDb, sessionId, NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to test force curent session" ;

      // check session id
      SINT64 sessionId1 = -1 ;
      rc = getCurrentSessionId( nodeDb, &sessionId1 ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get current session id after force" ;
      ASSERT_EQ( sessionId, sessionId1 ) << "fail to check session id unchanged when force current session" ;	

      sdbDisconnect( nodeDb ) ;
      sdbReleaseConnection( nodeDb ) ;
   }
}

TEST_F( forceSessionTest, withOption )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone.\n" ) ; 
      return ;
   }
   SINT64 oldSessionId = -1 ;
   SINT64 newSessionId = -1 ;

   const CHAR* groupName = "SYSCatalogGroup" ;
   const INT32 groupId = 1 ;
   const CHAR* hostName = NULL ;
   const CHAR* svcName = NULL ;
   const CHAR* nodeName = NULL ;
   INT32 nodeId = -1 ;

   // get node addr and connect
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   rc = sdbGetReplicaGroup1( db, groupId, &rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get SYSCatalog group" ;
   rc = sdbGetNodeSlave( rg, &node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get slave node of group " ;
   rc = sdbGetNodeAddr( node, &hostName, &svcName, &nodeName, &nodeId ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node addr " << nodeName  ;

   sdbConnectionHandle nodeDb ;
   rc = sdbConnect( hostName, svcName, ARGS->user(), ARGS->passwd(), &nodeDb ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect node " << nodeName ;
   printf( "node: hostName=%s svcName=%s nodeName=%s nodeId=%d\n", hostName, svcName, nodeName, nodeId ) ;

   // get session ids on node	
   SINT64 sessionIds[1024] ;
   CHAR sessionTypes[1024][20] ;
   memset( sessionIds, 0, sizeof(sessionIds) ) ;
   rc = getNodeSessionIds( nodeDb, nodeName, sessionIds, sessionTypes, 1024 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get agent session id to force
   SINT64 sessionId = -1 ;
   for( INT32 i = 0;i < 1024 && sessionIds[i] != 0;i++ )
   {
      if( strcmp( sessionTypes[i], "Agent" ) == 0 )
      {
         sessionId = sessionIds[i] ;
         break ;
      }
   }
   if( sessionId == -1 ) 
   {
      printf( "no agent session in catalog node.\n" ) ;
      return ;
   }

   // make option
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "HostName", hostName ) ;
   bson_append_string( &option, "svcname", svcName ) ;
   bson_append_int( &option, "NodeID", nodeId ) ;
   bson_append_int( &option, "GroupID", groupId ) ;
   bson_append_string( &option, "GroupName", groupName ) ;
   bson_finish( &option ) ;

   // force session with db connected to coord
   rc = sdbForceSession( db, sessionId, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to force session " << sessionId ;	 

   sdbDisconnect( nodeDb ) ;
   sdbReleaseConnection( nodeDb ) ;
}	
