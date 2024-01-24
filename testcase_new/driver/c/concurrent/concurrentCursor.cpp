/*************************************************
 * @Description: test case for c driver
 *				     concurrent test with multi cursor
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 5
#define recordNum 100

class concurrentCursorTest : public testBase
{
protected:
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCursorHandle cursor[ ThreadNum ] ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "concurrentCursorTestCs" ;
      clName = "concurrentCursorTestCl" ;
      bson cond ;
      bson_init( &cond ) ;
      bson sel ; 
      bson_init( &sel ) ;
   
      // create cs cl
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
      // insert records { a: i, flag: 1 }
      for( INT32 i = 0;i < recordNum;i++ )
      {  
         bson obj ;
         bson_init( &obj ) ;
         bson_append_int( &obj, "a", i ) ;
         bson_append_int( &obj, "flag", 1 ) ;
         bson_finish( &obj ) ;
         rc = sdbInsert( cl, &obj ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to insert record" ;
         bson_destroy( &obj ) ;
      }
      // query record
      bson_append_int( &cond, "flag", 1 ) ;
      bson_finish( &cond ) ;
      bson_append_string( &sel, "a", "" ) ;
      bson_finish( &sel ) ;
      for( INT32 i = 0;i < ThreadNum;i++ )
      {  
         rc = sdbQuery( cl, &cond, &sel, NULL, NULL, 0, -1, &cursor[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to query record" ;
      }
      bson_destroy( &cond ) ;
      bson_destroy( &sel ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      
      if( shouldClear() )
      {
         rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         sdbReleaseCursor( cursor[i] ) ;
      }
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbCursorHandle cursor ;    // cursor handle
   INT32 id ;				    // cursor id
} ;

void func_cursor( ThreadArg* arg )
{
   sdbCursorHandle cursor = arg->cursor ;
   INT32 i = arg->id ;
   INT32 rc = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   INT32 value = 0 ;
   while( !( rc = sdbNext( cursor, &obj ) ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      ASSERT_EQ( value, bson_iterator_int( &it ) ) << "fail to check cursor " << i ;
      value++ ;
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }
   bson_destroy( &obj ) ;
}

TEST_F( concurrentCursorTest, cursor )
{
   INT32 rc = SDB_OK ;

   Worker* workers[ ThreadNum ] ;
   ThreadArg arg[ ThreadNum ] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].cursor = cursor[i] ;
      arg[i].id = i ; 
      workers[i] = new Worker((WorkerRoutine)func_cursor, &arg[i], false) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
