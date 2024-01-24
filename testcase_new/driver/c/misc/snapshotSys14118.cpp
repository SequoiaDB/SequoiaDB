/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-3184
 *               seqDB-14118:并发获取系统快照
 * @Modify:      Liang xuewang Init
 *			 	     2018-01-16
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <pthread.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

#define THREAD_NUM   10
#define MAX_STRSIZE  50

class snapshotSysTest14118 : public testBase
{
protected:
   CHAR dataHostName[ MAX_STRSIZE ] ;
   CHAR dataSvcName[ MAX_STRSIZE ] ;
   CHAR dataFsName[ MAX_STRSIZE ] ;

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
      const CHAR* rgName ;
      vector<string> nodes ;
      INT32 idx ;
      string tmp ;

      // get data node hostname svcname
      rc = getGroups( db, groups ) ;
      CHECK_RC( SDB_OK, rc, "fail to get groups, rc = %d", rc ) ;
      rgName = groups[0].c_str() ;
      rc = getGroupNodes( db, rgName, nodes ) ;
      CHECK_RC( SDB_OK, rc, "fail to get group nodes, rc = %d", rc ) ;
      idx = nodes[0].find( ":" ) ;
      tmp = nodes[0].substr( 0, idx ) ;
      strcpy( dataHostName, tmp.c_str() ) ;
      tmp = nodes[0].substr( idx+1 ) ;
      strcpy( dataSvcName, tmp.c_str() ) ;
      printf( "node: %s:%s\n", dataHostName, dataSvcName ) ;   
   done:
      return rc ;
   error:
      goto done ; 
   }
} ;

INT32 getDiskName( sdbConnectionHandle db, CHAR* name )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor ;
   bson obj ;
   bson_init( &obj ) ;
   bson_iterator it ;
   bson subobj ;
   bson_init( &subobj ) ;
   bson_iterator subit ;
   bson_type type ;

   rc = sdbGetSnapshot( db, SDB_SNAP_SYSTEM, NULL, NULL, NULL, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to snapshot system, rc = %d", rc ) ;
   rc = sdbNext( cursor, &obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to get next, rc = %d", rc ) ;
   bson_find( &it, &obj, "Disk" ) ;
   bson_iterator_subobject( &it, &subobj ) ;
   type = bson_find( &subit, &subobj, "Name" ) ;
   if( type == BSON_EOO )
   {
      printf( "No Disk.Name in snapshot system\n" ) ;
      goto done ;
   }
   strcpy( name, bson_iterator_string( &subit ) ) ;
   rc = sdbCloseCursor( cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to close cursor, rc = %d", rc ) ;

done:
   bson_destroy( &subobj ) ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

struct ThreadArg
{
   sdbConnectionHandle db ;
   INT32 tid ;
} ;

void* func_snapshotSys( void* arg )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db = ((ThreadArg*)arg)->db ;
   INT32 tid = ((ThreadArg*)arg)->tid ;
   CHAR* name = (CHAR*)malloc( MAX_STRSIZE * sizeof(CHAR) ) ;
   rc = getDiskName( db, name ) ;
   if( rc != SDB_OK )
   {
      printf( "fail to get Disk.Name in thread %d, rc = %d\n", tid, rc ) ;
      return NULL ;
   }
   pthread_exit( (void*)name ) ;
}

TEST_F( snapshotSysTest14118, snapshotSys )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }   

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
        
   sdbConnectionHandle dataDb ; 
   rc = sdbConnect( dataHostName, dataSvcName, ARGS->user(), ARGS->passwd(), &dataDb ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect data node" ;
   rc = getDiskName( dataDb, dataFsName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "node Disk.Name: %s\n", dataFsName ) ;
   sdbDisconnect( dataDb ) ;
   sdbReleaseConnection( dataDb ) ;

   sdbConnectionHandle dataDbs[ THREAD_NUM ] ;
   for( INT32 i = 0;i < THREAD_NUM;i++ )
   {
      rc = sdbConnect( dataHostName, dataSvcName, ARGS->user(), ARGS->passwd(), &dataDbs[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect data node, i = " << i ;
   }

   pthread_t tids[ THREAD_NUM ] ;
   ThreadArg args[ THREAD_NUM ] ;
   for( INT32 i = 0;i < THREAD_NUM;++i )
   {
      args[i].db = dataDbs[i] ;
      args[i].tid = i ;
      rc = pthread_create( &tids[i], NULL, func_snapshotSys, (void*)&args[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create thread " << i ;
   }
   for( INT32 i = 0;i < THREAD_NUM;++i )
   {
      CHAR* name ;
      rc = pthread_join( tids[i], (void**)&name ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to join thread " << i ;
      ASSERT_TRUE( name ) ;
      ASSERT_STREQ( dataFsName, name ) << "fail to check Disk.Name in thread " << i ;
      free( name ) ;
      sdbDisconnect( dataDbs[i] ) ;
      sdbReleaseConnection( dataDbs[i] ) ;
   }
}
