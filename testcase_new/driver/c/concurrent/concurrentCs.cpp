/*************************************************************
 * @Description: test case for c driver
 *               concurrent test with multi cs
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 **************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 5

class concurrentCsTest : public testBase
{
protected:
   sdbCSHandle cs[ ThreadNum ] ;
   CHAR* csName[ ThreadNum ] ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      for( INT32 i = 0;i < ThreadNum;i++ )
      {  
         CHAR tmp[100] = { 0 } ;
         sprintf( tmp, "%s%d", "concurrentCsTestCs", i ) ;
         csName[i] = strdup( tmp ) ;
      }
      for( INT32 i = 0;i < ThreadNum;i++ )
      {  
         rc = sdbCreateCollectionSpace( db, csName[i], SDB_PAGESIZE_4K, &cs[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName[i] ;
      }
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         for( INT32 i = 0;i < ThreadNum;i++ )
         {
            rc = sdbDropCollectionSpace( db, csName[i] ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName[i] ;
            sdbReleaseCS( cs[i] ) ;
            free( csName[i] ) ;
         }
      }
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbCSHandle cs ;            // cs handle
   INT32 id ;				    // cs id
} ;

void func_cs( ThreadArg* arg )
{
   INT32 i = arg->id ;
   sdbCSHandle cs = arg->cs ;

   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   char clName[100] ;
   sprintf( clName, "%s%d", "concurrentCsTestCl", i ) ;

   INT32 rc = SDB_OK ;
   // create cl
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName << " in cs " << i ;
   // release cl before get cl
   sdbReleaseCollection( cl ) ;
   // get cl
   rc = sdbGetCollection1( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName << " in cs " << i ;
   // drop cl
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl in cs " << i ;
   // get cs name
   CHAR tmp[100] ;
   rc = sdbGetCSName( cs, tmp, sizeof( tmp ) ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs name of cs " << i ;
   // release cl
   sdbReleaseCollection( cl ) ;
}

TEST_F( concurrentCsTest, collectionspace )
{
   INT32 rc = SDB_OK ;

   // create multi thread to operate different cl
   Worker* workers[ThreadNum] ;
   ThreadArg arg[ThreadNum] ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      arg[i].id = i ;
      arg[i].cs = cs[i] ;
      workers[i] = new Worker( (WorkerRoutine)func_cs, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
