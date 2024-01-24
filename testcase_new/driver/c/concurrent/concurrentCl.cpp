/***********************************************************
 * @Description: testcase for c driver
 *               concurrent test with multi cl
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 ***********************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum   5

class concurrentClTest : public testBase
{
protected:
   const CHAR* csName ;
   CHAR* clName[ ThreadNum ] ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl[ ThreadNum ] ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "concurrentClTestCs" ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         CHAR tmp[100] = { 0 } ;
         sprintf( tmp, "%s%d", "concurrentClTestCl", i ) ; 
         clName[i] = strdup( tmp ) ;
      }
      for( int i = 0;i < ThreadNum;i++ )
      {  
         rc = sdbCreateCollection( cs, clName[i], &cl[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName[i] ;
      }     
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

         for( INT32 i = 0;i < ThreadNum;i++ )
         {
            sdbReleaseCollection( cl[i] ) ;
            free( clName[i] ) ;
         }
      }
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;


class ThreadArg : public WorkerArgs
{
public:
   sdbCollectionHandle cl ;    // cl handle
   INT32 id ;				    // cl id
} ;

void func_cl( ThreadArg* arg )
{
   sdbCollectionHandle cl = arg->cl ;
   INT32 i = arg->id ;
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
   bson_append_finish_object( &update ) ;
   bson_finish( &update ) ;
   rc = sdbUpdate( cl, &update, &record, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update record" ;

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
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;
}

TEST_F( concurrentClTest, collection )
{
   INT32 rc = SDB_OK ;

   // create multi thread to operate different cl
   Worker* workers[ThreadNum] ;
   ThreadArg arg[ThreadNum] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].cl = cl[i] ;
      arg[i].id = i ; 
      workers[i] = new Worker( (WorkerRoutine)func_cl, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
