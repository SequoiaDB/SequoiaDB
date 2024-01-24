/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1285
 * @Modify:      Liang xuewang Init
 *			 	     2017-02-28
 **************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "impWorker.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace import ;

#define ThreadNum  5
#define RecordNum  100

class bsonOidTest : public testBase
{
protected:
   sdbCSHandle _cs ;
   sdbCollectionHandle _cl ;
   sdbCollectionHandle cl[ ThreadNum ] ;
   const CHAR* csName ;
   const CHAR* clName ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "bsonOidTestCs" ;
      clName = "bsonOidTestCl" ;
      rc = createNormalCsCl( db, &_cs, &_cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         rc = sdbGetCollection1( _cs, clName, &cl[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName ;
      }
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         sdbReleaseCollection( cl[i] ) ;
      }
      sdbReleaseCollection( _cl ) ;
      sdbReleaseCS( _cs ) ;
      testBase::TearDown() ;
   }
} ;

class ThreadArgs : public WorkerArgs
{
public:
   sdbCollectionHandle clHandle ;
   INT32 tid ;
} ;

void bulkInsert( ThreadArgs* args )
{
   sdbCollectionHandle cl = args->clHandle ;
   INT32 tid = args->tid ;
   INT32 rc = SDB_OK ;

   // bulk insert record
   INT32 i = 0 ;
   bson* rec[ RecordNum ] ;
   while( i < RecordNum )
   {
      rec[i] = bson_create() ;
      bson_append_int( rec[i], "a", i + tid * RecordNum ) ;
      bson_finish( rec[i] ) ;
      i++ ;
   }
   rc = sdbBulkInsert( cl, 0, rec, RecordNum ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to bulk insert in thread " << tid ;
   i = 0 ;
   while( i < RecordNum )
   {
      bson_dispose( rec[i] ) ;
      i++ ;
   }
}

TEST_F( bsonOidTest, multiBulkInsert )
{
   INT32 rc = SDB_OK ;

   Worker* workers[ThreadNum] ;
   ThreadArgs args[ThreadNum] ;
   for( int i = 0;i < ThreadNum;i++ )
   {
      args[i].tid = i ;
      args[i].clHandle = cl[i] ;
      workers[i] = new Worker( (WorkerRoutine)bulkInsert, &args[i], false ) ;
      workers[i]->start() ;
   }
   for( int i = 0;i < ThreadNum;i++ )
   {
      workers[i]->waitStop() ;
   }
   SINT64 count = 0 ;
   rc = sdbGetCount( _cl, NULL, &count ) ;
   ASSERT_EQ( RecordNum * ThreadNum, count ) ;
}
