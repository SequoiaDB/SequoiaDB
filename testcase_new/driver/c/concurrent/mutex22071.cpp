#include <stdio.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "impWorker.hpp"
#include "client.h"

using namespace import ;

#define THREAD_NUM 2
#define TOTAL_THREAD_NUM 8
#define CS_NUM 2
#define CL_NUM 2 
#define RECORD_NUM 5000
#define UPDATE_WAIT 3 // lsec
#define QUERY_WAIT 3
#define DELETE_WAIT 3
#define THREAD_TIME_OUT ( 60 * 6 )
#define OPT_INSERT 0
#define OPT_DELETE 1
#define OPT_UPDATE 2
#define OPT_QUERY  3

vector<string> cs_name_vec ;
vector<string> cl_full_name_vec ;

class mutexTest22071: public testBase
{
   protected:
      sdbCSHandle cs_arr[ CS_NUM ] ; 

      void SetUp()
      {   
         testBase::SetUp() ;
         INT32 rc = SDB_OK ;
         INT32 i = 0;
         INT32 j = 0;
         for ( i = 0; i < CS_NUM; i++ )
         {
            cs_arr[i] = 0 ;
         }
         for ( i = 0; i < CS_NUM; i++ )
         {
            stringstream ss ;
            ss << "mutexcs22071" << i ;
            cs_name_vec.push_back( ss.str() ) ;
            for ( j = 0; j < CL_NUM; j++ )
            {
               stringstream s ;
               s << "mutexcs22071" << i << ".mutexcl22071" << j ;
               cl_full_name_vec.push_back( s.str() ) ;
            }
         }

         // create cs
         vector<string>::iterator it = cs_name_vec.begin() ;
         vector<string>::iterator it2 = cl_full_name_vec.begin() ;
         for ( i = 0; i < CS_NUM && it != cs_name_vec.end(); i++, it++ )
         {  
            rc = sdbCreateCollectionSpaceV2( db, (*it).c_str(),
                  NULL, &cs_arr[i] ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to create cs: " << (*it).c_str() ;
         }

         // create cl
         for ( i = 0; i < CL_NUM && it2 != cl_full_name_vec.end(); i++, it2++ )
         {
            sdbCollectionHandle cl = 0 ;
            rc = getCollection( db, (*it2).c_str(), &cl ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to create cl: " << (*it2).c_str() ;
            sdbReleaseCollection( cl ) ;
         }
      }

      void TearDown()
      {
         INT32 rc = SDB_OK ;
         vector<string>::iterator it = cs_name_vec.begin() ;
         if( shouldClear() )
         {
            // drop cs
            for ( ; it != cs_name_vec.end(); it++ )
            {
               rc = sdbDropCollectionSpace( db, (*it).c_str() ) ;
               ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs: " << (*it).c_str() ;
            }
            // release cs handle 
            it = cs_name_vec.begin() ;
            for ( INT32 i = 0; i < CS_NUM && it != cs_name_vec.end(); i++, it++ )
            {
               sdbReleaseCS( cs_arr[i] ) ;
            }
         }
         testBase::TearDown() ;
      }
};

class ThreadArg : public WorkerArgs
{
public:
     sdbCollectionHandle cl ;    // cl handle
     INT32 conn_id ;
     INT32 opt_type ;               
     INT32 thread_id ;
} ;

INT32 _getCLHandles( INT32 num, sdbConnectionHandle db, sdbCollectionHandle *arr )
{
   INT32 rc = 0 ;
   INT32 i = 0 ;
   vector<string>::iterator it = cl_full_name_vec.begin() ;   

   if ( num > CL_NUM | num < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   for ( i = 0; i < num && it != cl_full_name_vec.end(); i++, it++ )
   {
      rc = getCollection( db, (*it).c_str(), &(arr[i]) ) ;
   }

   if ( num != i )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   if ( num != i )
   {
      INT32 j = 0 ;
      for ( ; j < i; j++ )
      {
         sdbReleaseCS( arr[i] ) ;
      }
   }
   goto done ; 
}

BOOLEAN checkRecordNumAndReleaseHandle( sdbCollectionHandle *pArr, INT32 num )
{
   INT32 rc = SDB_OK ;
   INT32 i = 0 ;
   SINT64 count = 0 ;
   sdbCollectionHandle cl = 0 ;
   BOOLEAN run_status_flag = TRUE ;
   CHAR pCLFullName[ NAME_LEN + 1 ] = { 0 } ;

   for ( i = 0; i < num; i++ )
   {
      cl = pArr[i] ;
      rc = sdbGetCLFullName( cl, pCLFullName, NAME_LEN ) ;
      if ( SDB_OK != rc )
      {
         run_status_flag = FALSE ;
         printf( "Error: failed to get collection's name, "
                 "rc = %d"OSS_NEWLINE, rc ) ;
         goto release ;
      }
      rc = sdbGetCount( cl, NULL, &count ) ;
      if ( SDB_OK != rc )
      {
         run_status_flag = FALSE ;
         printf( "Error: failed to get count from collection[%s] to check, "
                 " rc = %d"OSS_NEWLINE, pCLFullName, rc ) ;
         goto release ;
      }
      if ( 0 != count )
      {
         run_status_flag = FALSE ;
         printf( "Error: %lld records leave in collection[%s]"OSS_NEWLINE,
                 count, pCLFullName ) ;
         goto release ;
      }

   release:
      sdbReleaseCollection( cl ) ;
   }

   return run_status_flag ;
}

void* func_insert( ThreadArg* arg )
{
   INT32 rc = SDB_OK ;
   sdbCollectionHandle cl = arg->cl ;
   arg->thread_id = gettid() ;
   CHAR pCLFullName[ NAME_LEN + 1 ] = { 0 } ;
   bson record ;
   
   rc = sdbGetCLFullName( cl, pCLFullName, NAME_LEN ) ;
   if ( SDB_OK != rc )
   {
      printf( "Error: %s:%d, failed to get collection full name, rc = %d",
            __FUNC__, __LINE__, rc ) ;
      goto error ;
   }

   for ( INT32 i = 0; i < RECORD_NUM; i++ )
   {
      bson_init( &record ) ;
      bson_append_int( &record, "a", i ) ;
      bson_append_string( &record, "b", "test" ) ;
      bson_finish( &record ) ;
      rc = sdbInsert( cl, &record ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: In %s:%d:,connect[%d] thread[%d]: failed to insert record to cl[%s], "
               "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, arg->conn_id, gettid(),
               pCLFullName, rc ) ;
         goto error ;
      }
      bson_destroy( &record ) ;
   }

done:
   return NULL ;
error:
   goto done ;   
}

void* func_delete( ThreadArg* arg )
{
   INT32 rc = SDB_OK ;
   arg->thread_id = gettid() ;
   sdbCollectionHandle cl = arg->cl ;
   SINT64 count = 0 ;
   INT32 retry_time = 10 ;
   CHAR pCLFullName[ NAME_LEN + 1 ] = { 0 } ;
   struct timeval tv_start, tv_end ;
   bson matcher ;

   bson_init( &matcher ) ;
   bson_append_start_object( &matcher, "a" ) ;
   bson_append_int( &matcher, "$et", -1 ) ;
   bson_append_finish_object( &matcher ) ;
   bson_finish( &matcher ) ;

   rc = sdbGetCLFullName( cl, pCLFullName, NAME_LEN ) ;
   if ( SDB_OK != rc )
   {
      printf( "Error: %s:%d, failed to get collection full name, rc = %d",
            __FUNC__, __LINE__, rc ) ;
      goto error ;
   }

   // get begin time
   if ( 0 != gettimeofday( &tv_start, NULL ) )
   {
      printf( "Error: %s:%d, failed to get begin time", __FUNC__, __LINE__ ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while( TRUE )
   {
      sleep( DELETE_WAIT ) ;
      // check timeout
      if ( 0 != gettimeofday( &tv_end, NULL ) )
      {
         printf( "Error: %s:%d, failed to get end time", __FUNC__, __LINE__ ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( ( tv_end.tv_sec - tv_start.tv_sec ) > THREAD_TIME_OUT )
      {
         printf( "Warning: %s:%d, thread[%d] timeout"OSS_NEWLINE,
                 __FUNC__, __LINE__, gettid() ) ;
         rc = SDB_TIMEOUT ;
         goto error ;
      }
      // begin
      rc = sdbGetCount( cl, NULL, &count ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to get count in cl[%s], "
                 "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, arg->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ; 
      }
      if ( 0 != count )
      {
         retry_time = 10 ;
         printf( "Info: In %s:%d, connect[%d] thread[%d]: %lld records in cl[%s], "
                 "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, arg->conn_id, gettid(), count, pCLFullName, rc ) ;
         rc = sdbDelete( cl, &matcher, NULL ) ;
         if ( SDB_OK != rc )
         {
            printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to delete record to cl[%s] "
                    "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, arg->conn_id, gettid(),
                    pCLFullName, rc ) ;
            goto error ;
         }
      }
      else
      {
         retry_time-- ;
         if ( 0 == retry_time )
         {
            break ;
         }
         else
         {
            usleep( 500000 ) ; // 500ms
            continue ;
         }
      }
   }

done:
   bson_destroy( &matcher ) ;
   return NULL ;
error:
   goto done ;
}

void* func_update( ThreadArg* arg )
{
   INT32 rc = SDB_OK ;
   arg->thread_id = gettid() ;
   sdbCollectionHandle cl = arg->cl ;
   SINT64 count = 0 ;
   INT32 retry_time = 10 ;
   struct timeval tv_start, tv_end ;
   CHAR pCLFullName[ NAME_LEN + 1 ] = { 0 } ;

   bson matcher ;
   bson updater ;

   bson_init( &matcher ) ;
   bson_append_start_object( &matcher, "a" ) ;
   bson_append_int( &matcher, "$gte", 0 ) ;
   bson_append_int( &matcher, "$lt", RECORD_NUM ) ;
   bson_append_finish_object( &matcher ) ;
   bson_finish( &matcher ) ;

   bson_init( &updater ) ;
   bson_append_start_object( &updater, "$set" ) ;
   bson_append_int( &updater, "a", -1 ) ;
   bson_append_finish_object( &updater ) ;
   bson_finish( &updater ) ;

   rc = sdbGetCLFullName( cl, pCLFullName, NAME_LEN ) ;
   if ( SDB_OK != rc )
   {
      printf( "Error: %s:%d: failed to get collection full name, rc = %d",
            __FUNC__, __LINE__, rc ) ;
      goto error ;
   }
   
   // get begin time
   if ( 0 != gettimeofday( &tv_start, NULL ) )
   {
      printf( "Error: %s:%d, failed to get begin time", __FUNC__, __LINE__ ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while( TRUE )
   {
      sleep( UPDATE_WAIT ) ;
      if ( 0 != gettimeofday( &tv_end, NULL ) )
      {
         printf( "Error: %s:%d, failed to get end time", __FUNC__, __LINE__ ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( ( tv_end.tv_sec - tv_start.tv_sec ) > THREAD_TIME_OUT )
      {
         printf( "Warning: %s:%d, thread[%d] timeout"OSS_NEWLINE,
                  __FUNC__, __LINE__, gettid() ) ;
         rc = SDB_TIMEOUT ;
         goto error ;
      }
      // begin
      rc = sdbGetCount( cl, &matcher, &count ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to get count in "
                 "cl[%s], rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, arg->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ;
      }
      if ( 0 != count )
      {
         retry_time = 10 ;
         printf( "Info: In %s:%d, thread[%d] thread[%d]: %lld records needs to update "
                 "in cl[%s]"OSS_NEWLINE, __FUNC__, __LINE__, arg->conn_id, gettid(),
                 count, pCLFullName ) ;
         rc = sdbUpdate( cl, &updater, &matcher, NULL ) ;
         if ( SDB_OK != rc )
         {
            printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to update record to cl[%s], "
                    "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, arg->conn_id, gettid(),
                    pCLFullName, rc ) ;
            goto error ;
         }
      }
      else
      {
         retry_time-- ;
         if ( 0 == retry_time )
         {
            break ;
         }
         else
         {
            usleep( 500000 ) ; // 500 ms
         }
      }
   }

done:
   bson_destroy( &matcher ) ;
   bson_destroy( &updater ) ;
   return NULL ;
error:
   goto done ;
}

void* func_query( ThreadArg* arg )
{
   INT32 rc = SDB_OK ;
   arg->thread_id = gettid() ;
   sdbCollectionHandle cl = arg->cl ;
   sdbCursorHandle cursor = 0 ;
   SINT64 count = 0 ;
   struct timeval tv_start, tv_end ;
   CHAR pCLFullName[ NAME_LEN ] = { 0 } ;

   rc = sdbGetCLFullName( cl, pCLFullName, NAME_LEN ) ;
   if ( SDB_OK != rc )
   {
      printf( "Error: %s:%d, failed to get collection full name, rc = %d",
            __FUNC__, __LINE__, rc ) ;
      goto error ;
   }

   // get begin time
   if ( 0 != gettimeofday( &tv_start, NULL ) )
   {
      printf( "Error: %s:%d, failed to get begin time", __FUNC__, __LINE__ ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while( TRUE )
   {
      sleep( QUERY_WAIT ) ;
      if ( 0 != gettimeofday( &tv_end, NULL ) )
      {
         printf( "Error: %s:%d, failed to get end time", __FUNC__, __LINE__ ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( ( tv_end.tv_sec - tv_start.tv_sec ) > THREAD_TIME_OUT )
      {
         printf( "Warning: %s:%d, thread[%d] timeout"OSS_NEWLINE,
                 __FUNC__, __LINE__, gettid() ) ;
         rc = SDB_TIMEOUT ;
         goto error ;
      }      
      // begin
      rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ; 
      if ( SDB_OK != rc )
      {
        printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to query in cl[%s], "
                "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, arg->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ;
      }
      rc = sdbGetCount( cl, NULL, &count ) ;
      if ( SDB_OK != rc )
      {
        printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to get count in cl[%s], "
                "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, arg->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ;
      }
      if ( 0 != count )
      {
         printf( "Info: In %s:%d, connect[%d] thread[%d]: %lld records in cl[%s], "
                 "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, arg->conn_id, gettid(), count, pCLFullName, rc ) ;
         continue ;
      }
      else
      {
         break ;
      }
   }

done:
   return NULL ;
error:
   goto done ;
}


TEST_F( mutexTest22071, multi_threading )
{
   INT32 rc               = SDB_OK ;

   sdbCollectionHandle cl_insert[ THREAD_NUM ] ;
   sdbCollectionHandle cl_delete[ THREAD_NUM ] ;
   sdbCollectionHandle cl_update[ THREAD_NUM ] ;
   sdbCollectionHandle cl_query[ THREAD_NUM ] ;

   // create multi thread to crud
   Worker* workers[ TOTAL_THREAD_NUM ] ;
   ThreadArg insertArg[ THREAD_NUM ] ;
   ThreadArg deleteArg[ THREAD_NUM ] ;
   ThreadArg updateArg[ THREAD_NUM ] ;
   ThreadArg queryArg[ THREAD_NUM ] ;

   // get collecton handles
   rc = _getCLHandles( THREAD_NUM, db, cl_insert ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = _getCLHandles( THREAD_NUM, db, cl_delete ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = _getCLHandles( THREAD_NUM, db, cl_update ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = _getCLHandles( THREAD_NUM, db, cl_query ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   // insert thread
   for( INT32 i = 0; i < THREAD_NUM; ++i )
   {
      insertArg[i].cl = cl_insert[i] ;
      insertArg[i].conn_id = i ;
      insertArg[i].opt_type = OPT_INSERT ;
      workers[i] = new Worker( (WorkerRoutine)func_insert, &insertArg[i], false ) ;
      workers[i]->start() ;
   }

   // delete thread
   for ( INT32 i = 0; i < THREAD_NUM; i++ )
   {
      deleteArg[i].cl = cl_delete[i] ;
      deleteArg[i].conn_id = i ;
      deleteArg[i].opt_type = OPT_DELETE ;
      workers[i + THREAD_NUM] = new Worker( (WorkerRoutine)func_delete, &deleteArg[i], false ) ;
      workers[i + THREAD_NUM]->start() ;
   }

   // update thread
   for ( INT32 i = 0; i < THREAD_NUM; i++ )
   {
      updateArg[i].cl = cl_update[i] ;
      updateArg[i].conn_id = i ;
      updateArg[i].opt_type = OPT_UPDATE ; 
      workers[i + 2 * THREAD_NUM] = new Worker( (WorkerRoutine)func_update, &updateArg[i], false ) ;
      workers[i + 2 * THREAD_NUM]->start() ;
   }

   // query thread
   for ( INT32 i = 0; i < THREAD_NUM; i++ )
   {
      queryArg[i].cl = cl_query[i] ;
      queryArg[i].conn_id = i ;
      queryArg[i].opt_type = OPT_QUERY ;
      workers[i + 3 * THREAD_NUM] = new Worker( (WorkerRoutine)func_query, &queryArg[i], false ) ;
      workers[i + 3 * THREAD_NUM]->start() ;
   }

   for( INT32 i = 0; i < TOTAL_THREAD_NUM; ++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

   // check record in database
   ASSERT_TRUE( checkRecordNumAndReleaseHandle( cl_insert, THREAD_NUM ) ) ;
   ASSERT_TRUE( checkRecordNumAndReleaseHandle( cl_delete, THREAD_NUM ) ) ;
   ASSERT_TRUE( checkRecordNumAndReleaseHandle( cl_update, THREAD_NUM ) ) ;
   ASSERT_TRUE( checkRecordNumAndReleaseHandle( cl_query, THREAD_NUM ) ) ;
}
