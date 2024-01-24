/**********************************************************
* @Description:  test case for jira questionare 
*                SEQUOIADBMAINSTREAM-1518
* @Modify:       Liangxw
*                2017-09-26
**********************************************************/
#include <client.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "arguments.hpp"

TEST( bsonArray, normal )
{
	INT32 rc = BSON_OK ;
	bson obj ;
	bson_init( &obj ) ;
	rc = bson_append_start_array( &obj, "arr" ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to append start array" ;
	rc = bson_append_string( &obj, "0", "a" ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to append first a" ;
	rc = bson_append_string( &obj, "1", "b" ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to append second b" ;
	rc = bson_append_finish_array( &obj ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to finish array" ;
	rc = bson_finish( &obj ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to finish bson" ;
	bson_print( &obj ) ;
	bson_destroy( &obj ) ;
}

TEST( bsonArray, abnormal )
{
	INT32 rc = BSON_OK ;
	bson obj ;
	bson_init( &obj ) ;
	rc = bson_append_start_array( &obj, "arr" ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to append start array" ;
	rc = bson_append_int( &obj, "", 1 ) ;
	ASSERT_EQ( BSON_ERROR, rc ) << "fail to test append array element with space" ;
	rc = bson_append_int( &obj, "a", 2 ) ;
	ASSERT_EQ( BSON_ERROR, rc ) << "fail to test append array element with a" ;
	rc = bson_append_finish_array( &obj ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to finish array" ;
	rc = bson_finish( &obj ) ;
	ASSERT_EQ( BSON_OK, rc ) << "fail to finish bson" ;
	bson_print( &obj ) ;
	bson_destroy( &obj ) ;
}

TEST( bsonArray, trace )
{
	INT32 rc = SDB_OK ;
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   CHAR* component = "cls, dms, mth" ;  // component must be valid
   CHAR* breakpoint = "_dmsStorageData::_onAllocExtent" ; // breakpoint must be valid
   UINT32 buffSize = 1024 ;  // the range of trace bufferSize is [ 1, 1024 ]

   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = sdbTraceStart( db, buffSize, component, breakpoint, NULL, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to trace start" ;
   rc = sdbTraceStop( db, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to trace stop" ;

   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

TEST( bsonArray, waitTask )
{
	INT32 rc = SDB_OK ;
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
	sdbCSHandle cs = SDB_INVALID_HANDLE ;
	sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
	const CHAR* csName = "testWaitTaskCs" ;
	const CHAR* clName = "testWaitTaskCl" ;

	rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone.\n" ) ;
      sdbDisconnect( db ) ; 
      sdbReleaseConnection( db ) ;
      return ;
   }	

	// get srcGroup and dstGroup
	vector<string> groups ;
	rc = getGroups( db, groups ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
	if( groups.size() < 2 )  // check data group num
	{
		printf( "Data group num is %ld, too few.\n", groups.size() ) ;
		sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
		return ; 
	} 
	const CHAR* srcGroup = groups[0].c_str() ;
	const CHAR* dstGroup = groups[1].c_str() ;
	printf( "src group: %s, dst group: %s\n", srcGroup, dstGroup ) ;

	// create cs cl in srcGroup
	rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
	bson option ;
	bson_init( &option ) ;
	bson_append_string( &option, "Group", srcGroup ) ;
	bson_append_start_object( &option, "ShardingKey" ) ;
	bson_append_int( &option, "a", 1 ) ;
	bson_append_finish_object( &option ) ;
	bson_finish( &option ) ;
	rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

	// split cl async
	SINT64 taskID ;
	rc = sdbSplitCLByPercentAsync( cl, srcGroup, dstGroup, 50, &taskID ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to split cl async" ;
	
	// wait tasks
	SINT64 taskIDs[1] ;
	taskIDs[0] = taskID ;
	rc = sdbWaitTasks( db, taskIDs, 1 ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to waitTasks" ;

	// drop cs 
	rc = sdbDropCollectionSpace( db, csName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
	
	// disconnect, release handle, destroy bson
	sdbDisconnect( db ) ;
	bson_destroy( &option ) ;
	sdbReleaseConnection( db ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseCollection( cl ) ;
}
