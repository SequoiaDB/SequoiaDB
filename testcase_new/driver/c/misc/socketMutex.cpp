/*****************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-859
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 *****************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 10

class socketMutexTest : public testBase
{
protected:
   CHAR* csName[ ThreadNum ] ;
   CHAR* clName[ ThreadNum ] ;
   sdbCSHandle cs[ ThreadNum ] ;
   sdbCollectionHandle cl[ ThreadNum ] ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;

      // close cache
      sdbClientConf config ;
      config.enableCacheStrategy = 0 ;
      config.cacheTimeInterval = 0 ;
      rc = initClient( &config ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to init client" ;

      // make csName
      for( INT32 i = 0;i < ThreadNum;++i )
      {
         CHAR tmp[30] = { 0 } ;
         sprintf( tmp, "%s%d", "socketMutexTestCs", i ) ;
         csName[i] = strdup( tmp ) ;
      }

      // make clName
      for( INT32 i = 0;i < ThreadNum;++i )
      {
         CHAR tmp[30] = { 0 } ;
         sprintf( tmp, "%s%d", "socketMutexTestCl", i ) ;
         clName[i] = strdup( tmp ) ;
      }

      // create cs and cl
      // make option { "ReplSize": 0 }
      bson option ;
      bson_init( &option ) ;
      bson_append_int( &option, "ReplSize", 0 ) ;
      bson_finish( &option ) ;
      for( INT32 i = 0;i < ThreadNum;++i )
      {
         rc = sdbCreateCollectionSpace( db, csName[i], SDB_PAGESIZE_4K, &cs[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName[i] ;	
         rc = sdbCreateCollection1( cs[i], clName[i], &option, &cl[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName[i] ;
      }
      bson_destroy( &option ) ;
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;   
      for( INT32 i = 0;i < ThreadNum;++i )
      { 
         rc = sdbDropCollectionSpace( db, csName[i] ) ; 
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName[i] ;
         sdbReleaseCollection( cl[i] ) ;
         sdbReleaseCS( cs[i] ) ;
         free( csName[i] ) ;
         free( clName[i] ) ;
      }
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbCollectionHandle cl ;	// collection
   INT32 cid ;				    // collection id
} ;

// thread_function CRUD with cl
void func_cl( ThreadArg* arg )
{
   sdbCollectionHandle cl = arg->cl ;
   INT32 i = arg->cid ;
   INT32 rc = SDB_OK ;

   // insert record { "a": i }
   bson record ;
   bson_init( &record ) ;
   bson_append_int( &record, "a", i ) ;
   bson_finish( &record ) ;
   rc = sdbInsert( cl, &record ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert record" ;

   // query record find( { "a": i }, { "a": "" } )
   bson select ;
   bson_init( &select ) ;
   bson_append_string( &select, "a", "" ) ;
   bson_finish( &select ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &record, &select, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query record" ;
   sdbReleaseCursor( cursor ) ;

   // update record update( { "$set": { "a": -1 } }, { "a": i } )
   bson update ;
   bson_init( &update ) ;
   bson_append_start_object( &update, "$set" ) ;
   bson_append_int( &update, "a", -1 ) ;
   bson_append_finish_object(&update) ;
   bson_finish( &update ) ;
   rc = sdbUpdate( cl, &update, &record, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update" ;

   // query record find( { "a": -1 }, { "a": "" } )
   bson expect ;
   bson_init( &expect ) ;
   bson_append_int( &expect, "a", -1 ) ;
   bson_finish( &expect ) ;
   rc = sdbQuery( cl, &expect, &select, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check update a:-1" ;

   // destroy bson
   bson_destroy( &record ) ;
   bson_destroy( &select ) ;
   bson_destroy( &update ) ;
   bson_destroy( &expect ) ;

   // close and release cursor
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ(  SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;
}

// thread_function query then close cursor
// main thread will disconnect between threads
void func_closeCursor1( ThreadArg *arg )
{
   sdbCollectionHandle cl = arg->cl ;
   INT32 i = arg->cid ;
   INT32 rc = SDB_OK ;

   // query record find( { "a": i }, { "a": "" } )
   bson record ;
   bson_init( &record ) ;
   bson_append_int( &record, "a", -1 ) ;
   bson_finish( &record ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &record, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_TRUE( rc == SDB_OK || rc == SDB_NOT_CONNECTED || rc == SDB_NETWORK ) 
                << "fail to query record, rc = " << rc ;
   printf( "thread %d, query record return: %d\n", i, rc ) ;
   bson_destroy( &record ) ;

   // close and release cursor
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_TRUE( rc == SDB_OK || rc == SDB_INVALIDARG || rc == SDB_NETWORK ) 
                << "fail to close cursor, rc = " << rc ;
   printf( "thread %d, close cursor return: %d\n", i, rc ) ;
   sdbReleaseCursor( cursor ) ;
}

// thread_function query then close cursor
// main thread will close all cursor between multi threads
void func_closeCursor2( ThreadArg *arg )
{
   sdbCollectionHandle cl = arg->cl ;
   INT32 i = arg->cid ;
   INT32 rc = SDB_OK ;

   // query record find( { "a": i }, { "a": "" } )
   bson record ;
   bson_init( &record ) ;
   bson_append_int( &record, "a", -1 ) ;
   bson_finish( &record ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, &record, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_TRUE( rc == SDB_OK || rc == SDB_NOT_CONNECTED ) << "fail to query record, rc = " << rc ;
   printf( "thread %d, query record return: %d\n", i, rc ) ;
   bson_destroy( &record ) ;

   // close and release cursor
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_TRUE( rc == SDB_OK || rc == SDB_INVALIDARG ) << "fail to close cursor, rc = " << rc ;
   printf( "thread %d, close cursor return: %d\n", i, rc ) ;
   sdbReleaseCursor( cursor ) ;
}

// multi threads operate multi cs cl
// after threads close cursor and stop,main thread close all cursor and disconnect
TEST_F( socketMutexTest, cl )
{
   INT32 rc = SDB_OK ;

   // create multi thread to operate different cl
   Worker* workers[ ThreadNum ] ;
   ThreadArg arg[ ThreadNum ] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].cl = cl[i] ;
      arg[i].cid = i ; 
      workers[i] = new Worker( (WorkerRoutine)func_cl, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

   // close all cursors in the end
   rc = sdbCloseAllCursors( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close all cursor" ;
   sdbDisconnect( db ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      sdbReleaseCollection( cl[i] ) ;
      sdbReleaseCS( cs[i] ) ;
   }
   sdbReleaseConnection( db ) ;

   // after test,reconnect and get cs cl
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      rc = sdbGetCollectionSpace( db, csName[i], &cs[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get cs " << csName[i] ;
      rc = sdbGetCollection1( cs[i], clName[i], &cl[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName[i] ;
   }
}

// diconnect between multi threads
TEST_F( socketMutexTest, disconnect )
{
   INT32 rc = SDB_OK ;

   // create multi thread to operate different cl
   Worker* workers[ ThreadNum ] ;
   ThreadArg arg[ ThreadNum ] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].cl = cl[i] ;
      arg[i].cid = i ;
      workers[i] = new Worker( (WorkerRoutine)func_closeCursor1, &arg[i], false ) ;
      workers[i]->start() ;
   }

   // thread 0-ThreadNum/2 close cursor before main thread disconnect
   for( INT32 i = 0;i < ThreadNum/2;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

   // disconnect between threads
   sdbDisconnect( db ) ;

   // release handle
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      sdbReleaseCollection( cl[i] ) ;
      sdbReleaseCS( cs[i] ) ;
   }
   sdbReleaseConnection( db ) ;

   // thread ThreadNum/2-ThreadNum close cursor after main thread disconnect
   for( INT32 i = ThreadNum/2;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

   // after test,reconnect and get cs cl
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      rc = sdbGetCollectionSpace( db, csName[i], &cs[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get cs " << csName[i] ;
      rc = sdbGetCollection1( cs[i], clName[i], &cl[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName[i] ;
   }	
}

// close all cursors between multi threads
TEST_F( socketMutexTest, closeAllCursor )
{
   INT32 rc = SDB_OK ;

   // create multi thread to operate different cl
   Worker* workers[ ThreadNum ] ;
   ThreadArg arg[ ThreadNum ] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].cl = cl[i] ;
      arg[i].cid = i ;
      workers[i] = new Worker( (WorkerRoutine)func_closeCursor2, &arg[i], false ) ;
      workers[i]->start() ;
   }

   // thread 0-ThreadNum/2 close cursor before main thread closeAllCursor
   for( INT32 i = 0;i < ThreadNum/2;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

   // close all cursors between threads
   rc = sdbCloseAllCursors( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close all cursor" ;

   // thread ThreadNum/2-ThreadNum close cursor after main thread closeAllCursor
   for( INT32 i = ThreadNum/2;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
