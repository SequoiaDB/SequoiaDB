#include <stdio.h>
#include <gtest/gtest.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "client.h"
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "impWorker.hpp"
#include <time.h>
#include <sys/time.h>

using namespace import ;

#define THREAD_NUM 10
#define INSERT_NUM 2000 

class concurrentInsert11187: public testBase
{
protected:
    const CHAR* csName ;
    const CHAR* clName ;
    sdbCSHandle cs ;
    sdbCollectionHandle cl ;

    void SetUp()  
    {   
       testBase::SetUp() ;
       INT32 rc = SDB_OK ;
       csName = "concurrentInsertCs11187" ;
       clName = "concurrentInsertCl11187" ;
       cs = SDB_INVALID_HANDLE ;
       cl = SDB_INVALID_HANDLE ;
       rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ; 
       ASSERT_EQ( SDB_OK, rc ) ; 

       // insert datas
       bson* docs[5] ; 
       for( INT32 i = 0; i < 5; i++ )
       {   
          docs[i] = bson_create() ;
          bson_append_int( docs[i], "a", i ) ; 
          bson_finish( docs[i] ) ; 
       }   
       rc = sdbBulkInsert( cl, 0, docs, 5 ) ; 
       ASSERT_EQ( SDB_OK, rc ) ; 
       for( INT32 i = 0; i < 5; i++ )
       {
          bson_dispose( docs[i] ) ;
       }
    }
    void TearDown()
    {
       INT32 rc = SDB_OK ;
       rc = sdbDropCollectionSpace( db, csName ) ;
       ASSERT_EQ( SDB_OK, rc ) ;
       sdbReleaseCollection( cl ) ;
       sdbReleaseCS( cs ) ;
       testBase::TearDown() ;
    }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbConnectionHandle conn ;
} ;

void func_insert( ThreadArg* arg ) 
{
   INT32 rc = SDB_OK ;
   sdbCollectionHandle collection = 0 ; 
   int count                      = 0 ;
   SINT64 totalNum                = 0 ;
   bson obj ;
   bson *objList [ INSERT_NUM ] ;

   // count the total number of records before bulkInsert
   const CHAR* clFullName = "concurrentInsertCs11187.concurrentInsertCl11187" ; 
   rc = getCollection ( arg->conn, clFullName, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) << "getCollection() fail!";
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) << "sdbGetCount() fail!";

   printf("Before bulk insert, the total number of records is %lld\n",totalNum ) ;
   // allocate memory and add data
   for ( count = 0; count < INSERT_NUM; count++ )
   {
      objList[count] = bson_create() ;
      if ( !jsonToBson( objList[count],"{firstname:\"john\",\
                                  lastname:\"smith\",age:50}" ) )
      {
         printf ( "failed to convert json to bson." OSS_NEWLINE ) ;
         continue ;
      }
   }
   // bulk insert,if the argument "flags" is set FLG_INSERT_CONTONDUP,
   // datebase will not stop bulk insert while one failed with dup key
   rc = sdbBulkInsert ( collection, 0, objList, INSERT_NUM ) ;
   // count the total number of records after insert
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) << "sdbGetCount() fail!";
   printf("After bulk insert, thread[ %d ]'s total number of records is %lld\n", gettid(), totalNum ) ;

   // free memory that allocated by bson_create()
   for ( count = 0; count < INSERT_NUM; count++ )
   {
      bson_dispose ( objList[count] ) ;
   }

   sdbReleaseCollection ( collection ) ;
}

TEST_F( concurrentInsert11187, insert )
{
   // create multi thread to insert
   Worker* worker[ THREAD_NUM ] ; 
   ThreadArg args[ THREAD_NUM ] ;
   
   // insert thread
   for( INT32 i = 0; i < THREAD_NUM; ++i )
   {
      args[i].conn = db ;
      worker[i] = new Worker( (WorkerRoutine)func_insert, &args[i], false ) ;
      worker[i]->start() ;
   } 

   timeval starttime, endtime ;
   gettimeofday( &starttime, 0 ) ;

   for( INT32 i = 0; i < THREAD_NUM; ++i )
   {
      worker[i]->waitStop() ;
      delete worker[i] ;
   }

   gettimeofday( &endtime, 0 ) ;
   double timeuse = 1000000 * ( endtime.tv_sec - starttime.tv_sec ) +
                                endtime.tv_usec - starttime.tv_usec ;
   timeuse /= 1000000 ;
   printf( "time consumption is %lf s \n", timeuse ) ;
}

