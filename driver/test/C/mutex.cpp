#include <stdio.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "testcommon.h"
#include "client.h"


using namespace std ;

#define NUM 10

#define CONN_FOR_INSERT NUM 
#define CONN_FOR_DELETE NUM
#define CONN_FOR_UPDATE NUM
#define CONN_FOR_QUERY  NUM

#define CONN_NUM ( CONN_FOR_INSERT + CONN_FOR_DELETE + \
                   CONN_FOR_UPDATE + CONN_FOR_QUERY )
#define CS_NUM NUM
#define CL_NUM NUM 
#define RECORD_NUM 5000
#define UPDATE_WAIT 3 // lsec
#define QUERY_WAIT 3
#define DELETE_WAIT 3

#define THREAD_TIME_OUT ( 60 * 6 )

BOOLEAN global_init_flag   = FALSE ;
BOOLEAN case_init_flag     = FALSE ;


//const CHAR *pHostName      = "192.168.20.42" ;
//const CHAR *pSvcName       = "11810" ;
const CHAR *pHostName      = HOST ;
const CHAR *pSvcName       = SERVER ;
const CHAR *pUser          = USER ;
const CHAR *pPassword      = PASSWD ;


sdbConnectionHandle db_arr[ CONN_NUM ] = { 0 } ;
sdbCSHandle cs_arr[ CS_NUM ] = { 0 } ;
vector<string> cs_name_vec ;
vector<string> cl_full_name_vec ;

enum option_type
{
   OPT_INSERT = 0 ,
   OPT_DELETE = 1 ,
   OPT_UPDATE = 2 ,
   OPT_QUERY  = 3
} ;

struct _thread_argument
{
   INT32 conn_id ;
   INT32 opt_type ;
   sdbCollectionHandle cl ;
   INT32 thread_id ;
   INT32 ret ;
} ;
typedef struct _thread_argument thread_argument ;

INT32 _getCLHandles( INT32 num, sdbCollectionHandle *arr )
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
      rc = getCollection( db_arr[0], (*it).c_str(), &(arr[i]) ) ;
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

BOOLEAN _checkRecordNumAndReleaseHandle( sdbCollectionHandle *pArr, INT32 num )
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

   if ( TRUE == run_status_flag )
      return TRUE ;
   else
      return FALSE ;
}

void* func_insert( void* arg )
{
   INT32 rc = SDB_OK ;
   thread_argument *pStru = (thread_argument*)arg ;
   sdbCollectionHandle cl = pStru->cl ;
   pStru->thread_id = gettid() ;
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
                 "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, pStru->conn_id, gettid(),
                 pCLFullName, rc ) ;
         goto error ;
      }
      bson_destroy( &record ) ;

   } 

done:
   pStru->ret = rc ;
   pthread_exit( NULL ) ;
error:
   goto done ;   
}

void* func_delete( void* arg )
{
   INT32 rc = SDB_OK ;
   thread_argument *pStru = (thread_argument*)arg ;
   pStru->thread_id = gettid() ;
   sdbCollectionHandle cl = pStru->cl ;
   SINT64 count = 0 ;
#define RETRY_TIME 10
   INT32 retry_time = RETRY_TIME ;
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
                 __FUNC__, __LINE__, pStru->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ; 
      }
      if ( 0 != count )
      {
         retry_time = RETRY_TIME ;
         printf( "Info: In %s:%d, connect[%d] thread[%d]: %lld records in cl[%s], "
                 "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, pStru->conn_id, gettid(), count, pCLFullName, rc ) ;
         rc = sdbDelete( cl, &matcher, NULL ) ;
         if ( SDB_OK != rc )
         {
            printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to delete record to cl[%s] "
                    "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, pStru->conn_id, gettid(),
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
   pStru->ret = rc ;
   pthread_exit( NULL ) ;
error:
   goto done ;
}

void* func_update( void* arg )
{
   INT32 rc = SDB_OK ;
   thread_argument *pStru = (thread_argument*)arg ;
   pStru->thread_id = gettid() ;
   sdbCollectionHandle cl = pStru->cl ;
   SINT64 count = 0 ;
#define RETRY_TIME 10
   INT32 retry_time = RETRY_TIME ;
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
                 __FUNC__, __LINE__, pStru->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ;
      }
      if ( 0 != count )
      {
         retry_time = RETRY_TIME ;
         printf( "Info: In %s:%d, thread[%d] thread[%d]: %lld records needs to update "
                 "in cl[%s]"OSS_NEWLINE, __FUNC__, __LINE__, pStru->conn_id, gettid(),
                 count, pCLFullName ) ;
         rc = sdbUpdate( cl, &updater, &matcher, NULL ) ;
         if ( SDB_OK != rc )
         {
            printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to update record to cl[%s], "
                    "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, pStru->conn_id, gettid(),
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
   pStru->ret = rc ;
   pthread_exit( NULL ) ;
error:
   goto done ;
}

void* func_query( void* arg )
{
   INT32 rc = SDB_OK ;
   thread_argument *pStru = (thread_argument*)arg ;
   pStru->thread_id = gettid() ;
   sdbCollectionHandle cl = pStru->cl ;
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
                 __FUNC__, __LINE__, pStru->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ;
      }
      rc = sdbGetCount( cl, NULL, &count ) ;
      if ( SDB_OK != rc )
      {
        printf( "Error: In %s:%d, connect[%d] thread[%d]: failed to get count in cl[%s], "
                "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, pStru->conn_id, gettid(), pCLFullName, rc ) ;
         goto error ;
      }
      if ( 0 != count )
      {
         printf( "Info: In %s:%d, connect[%d] thread[%d]: %lld records in cl[%s], "
                 "rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, pStru->conn_id, gettid(), count, pCLFullName, rc ) ;
         continue ;
      }
      else
      {
         break ;
      }
   }

done:
   pStru->ret = rc ;
   pthread_exit( NULL ) ;
error:
   goto done ;
}


class mutexTest : public testing::Test
{
   public:
      mutexTest() {}

   public:
      // run before all the testcase
      static void SetUpTestCase() ;

      // run before all the testcase
      static void TearDownTestCase() ;

      // run before every testcase
      virtual void SetUp() ;

      // run before every testcase
      virtual void TearDown() ;
} ;

void mutexTest::SetUpTestCase()
{
   /// build some connections
   
   INT32 rc = SDB_OK ;
   INT32 i = 0 ;
   INT32 j = 0 ;
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "PreferedInstance", "M" ) ;
   bson_finish( &option ) ;

   global_init_flag = FALSE ;

   for ( i = 0; i < CS_NUM; i++ )
   {
      stringstream ss ;
      ss << "foo" << i ;
      cs_name_vec.push_back( ss.str() ) ;
      for ( j = 0; j < CL_NUM; j++ )
      {
         stringstream s ;
         s << "foo" << i << ".bar" << j ;
         cl_full_name_vec.push_back( s.str() ) ;
      }
   }

   for ( i = 0; i < CONN_NUM; i++ )
   {
      rc = sdbConnect( pHostName, pSvcName, pUser, pPassword, &db_arr[i] ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: %s:%d: failed to connect to database: rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, rc ) ;
         goto error ;
      }
      rc = sdbSetSessionAttr( db_arr[i], &option ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: %s:%d: failed to set session attribute, "
                 "rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, rc ) ;
         goto error ;
      }
   }
   global_init_flag = TRUE ;

   done :
      bson_destroy( &option ) ;
      return ;
   error :
      goto done ;
}

void mutexTest::TearDownTestCase()
{
   /// disconnect connections and distroy connection handles

   for ( INT32 i = 0; i < CONN_NUM; i++ )
   {
      sdbDisconnect( db_arr[i] ) ;
   }

}

void mutexTest::SetUp()
{
   /// drop all the cs in database then create some
   
   INT32 rc = SDB_OK ;
   INT32 i = 0 ;
   sdbCursorHandle cur = 0 ;
   vector<string>::iterator it = cs_name_vec.begin() ;
   vector<string>::iterator it2 = cl_full_name_vec.begin() ;
   bson obj ;
    
   case_init_flag = FALSE ;
   if ( FALSE == global_init_flag )
   {
      goto done ;
   }

   // drop and create cs
   for ( i = 0; i < CS_NUM && it != cs_name_vec.end(); i++, it++ )
   {
      rc = sdbDropCollectionSpace( db_arr[0], (*it).c_str() ) ;
      if ( SDB_DMS_CS_NOTEXIST != rc && SDB_OK != rc )
      {
         printf( "Error: %s:%d, failed to drop cs[%s] in db, rc = %d"OSS_NEWLINE,
                  __FUNC__, __LINE__, (*it).c_str(), rc ) ;
         goto error ; 
      }
      rc = sdbCreateCollectionSpaceV2( db_arr[0], (*it).c_str(),
                                       NULL, &cs_arr[i] ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: %s:%d, failed to create cs[%s], rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, (*it).c_str(), rc ) ;
         goto error ;
      }
   } 

   // create cl
   for ( i = 0; i < CL_NUM && it2 != cl_full_name_vec.end(); i++, it2++ )
   {
      sdbCollectionHandle cl = 0 ;
      rc = getCollection( db_arr[0], (*it2).c_str(), &cl ) ;
      if ( SDB_OK != rc )
      {
         printf( "Error: %s:%d, failed to create cl[%s], rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, (*it2).c_str(), rc ) ;
         goto error ;
      }
      sdbReleaseCollection( cl ) ;
   }

   case_init_flag = TRUE ;

done:
   return ;
error:
   goto done ;
}

void mutexTest::TearDown()
{
   /// drop all the cs

   INT32 rc = SDB_OK ;
   INT32 i = 0 ;
   vector<string>::iterator it = cs_name_vec.begin() ;

   if ( FALSE == case_init_flag )
   {
      return ;
   }
   for ( ; it != cs_name_vec.end(); it++ )
   {
      rc = sdbDropCollectionSpace( db_arr[0], (*it).c_str() ) ;
      if ( SDB_OK != rc )
      {
         printf( "Warning: %s:%d, failed to drop cs[%s] in db: rc = %d"OSS_NEWLINE,
                  __FUNC__, __LINE__, (*it).c_str(), rc ) ;
      }
   }
   // release handle 
   for ( i = 0, it = cs_name_vec.begin(); i < CS_NUM && it != cs_name_vec.end(); i++, it++ )
   {
      sdbReleaseCS( cs_arr[i] ) ;
   }
}

TEST_F( mutexTest, multi_threading )
{
   INT32 rc               = SDB_OK ;
   sdbConnectionHandle db = db_arr[0] ;
   sdbCollectionHandle cl = 0 ;
   BOOLEAN run_status_flag = TRUE ;
   vector<string>::iterator it ;
   SINT64 count = 0 ;
   INT32 ret = 0 ;
#define INSERT_THREAD_NUM CONN_FOR_INSERT 
#define DELETE_THREAD_NUM CONN_FOR_DELETE
#define UPDATE_THREAD_NUM CONN_FOR_UPDATE
#define QUERY_THREAD_NUM CONN_FOR_QUERY
   pthread_t insert_thread[INSERT_THREAD_NUM] ;
   pthread_t delete_thread[DELETE_THREAD_NUM] ;
   pthread_t update_thread[UPDATE_THREAD_NUM] ;
   pthread_t query_thread[QUERY_THREAD_NUM] ;
   thread_argument s_insert[INSERT_THREAD_NUM] ;
   thread_argument s_delete[DELETE_THREAD_NUM] ;
   thread_argument s_update[UPDATE_THREAD_NUM] ;
   thread_argument s_query[QUERY_THREAD_NUM] ;
   sdbCollectionHandle h_insert[ INSERT_THREAD_NUM ] ;
   sdbCollectionHandle h_delete[ DELETE_THREAD_NUM ] ; 
   sdbCollectionHandle h_update[ UPDATE_THREAD_NUM ] ;
   sdbCollectionHandle h_query[ QUERY_THREAD_NUM ] ;
   INT32 i = 0 ;

   if ( FALSE == global_init_flag || FALSE == case_init_flag )
   {
      ASSERT_EQ( 0, 1 ) << "Failed to init to run test case" ;
   }

   /// check
   if ( INSERT_THREAD_NUM > CONN_FOR_INSERT || 
        DELETE_THREAD_NUM > CONN_FOR_DELETE ||
        UPDATE_THREAD_NUM > CONN_FOR_UPDATE ||
        QUERY_THREAD_NUM > CONN_FOR_QUERY )
   {
      ASSERT_EQ( 0, 1 ) << "Error: the amount of option thread should "
         "less then the amount of connection handle" ;
   }

   // get collecton handles
   rc = _getCLHandles( INSERT_THREAD_NUM, h_insert ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = _getCLHandles( DELETE_THREAD_NUM, h_delete ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = _getCLHandles( UPDATE_THREAD_NUM, h_update ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = _getCLHandles( QUERY_THREAD_NUM, h_query ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert thread
   for ( i = 0; i < INSERT_THREAD_NUM; i++ )
   {
      s_insert[i].conn_id = i ;
      s_insert[i].opt_type = OPT_INSERT ;
      s_insert[i].cl = h_insert[i] ;
      s_insert[i].ret = 0 ;
      rc = pthread_create( &insert_thread[i], NULL, func_insert, &s_insert[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to create insert thread" ;
   }
   
   // delete thread
   for ( i = 0; i < DELETE_THREAD_NUM; i++ )
   {
      s_delete[i].conn_id = i ;
      s_delete[i].opt_type = OPT_DELETE ;
      s_delete[i].cl = h_delete[i] ;
      s_delete[i].ret = 0 ;

      rc = pthread_create( &delete_thread[i], NULL, func_delete, &s_delete[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to create delete thread" ;
   }

   // update thread
   for ( i = 0; i < UPDATE_THREAD_NUM; i++ )
   {
      s_update[i].conn_id = i ;
      s_update[i].opt_type = OPT_UPDATE ;
      s_update[i].cl = h_update[i] ;
      s_update[i].ret = 0 ;

      rc = pthread_create( &update_thread[i], NULL, func_update, &s_update[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to create update thread" ;
   }

   // query thread
   for ( i = 0; i < QUERY_THREAD_NUM; i++ )
   {
      s_query[i].conn_id = i ;
      s_query[i].opt_type = OPT_QUERY ;
      s_query[i].cl = h_query[i] ;
      s_query[i].ret = 0 ;

      rc = pthread_create( &query_thread[i], NULL, func_query, &s_query[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to create query thread" ;
   }

   for ( i = 0; i < INSERT_THREAD_NUM; i++ )
   {
      rc = pthread_join( insert_thread[i], NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to join insert thread" ;
   }

   for ( i = 0; i < DELETE_THREAD_NUM; i++ )
   {
      rc = pthread_join( delete_thread[i], NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to join delete thread" ;
   }

   for ( i = 0; i < UPDATE_THREAD_NUM; i++ )
   {
      rc = pthread_join( update_thread[i], NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to join update thread" ;
   }

   for ( i = 0; i < QUERY_THREAD_NUM; i++ )
   {
      rc = pthread_join( query_thread[i], NULL ) ;
      ASSERT_EQ( SDB_OK, rc ) << "Failed to join query thread" ;
   }

   /// check option result. expect all the collection
   for ( i = 0; i < INSERT_THREAD_NUM; i++ )
   {
      if ( SDB_OK  != s_insert[i].ret )
      {
         run_status_flag = FALSE ;
         printf( "Error: thread[%d][%d] return %d"OSS_NEWLINE,
                 s_insert[i].thread_id, s_insert[i].opt_type,
                 s_insert[i].ret ) ;
      }  
   }

   for ( i = 0; i < DELETE_THREAD_NUM; i++ )
   {
      if ( SDB_OK != s_delete[i].ret )
      {
         run_status_flag = FALSE ;
         printf( "Error: thread[%d][%d] return %d"OSS_NEWLINE,
                 s_delete[i].thread_id, s_delete[i].opt_type,
                 s_delete[i].ret ) ;
      }
   } 

   for ( i = 0; i < DELETE_THREAD_NUM; i++ )
   {
      if ( SDB_OK != s_update[i].ret )
      {
         run_status_flag = FALSE ;
         printf( "Error: thread[%d][%d] return %d"OSS_NEWLINE,
                 s_update[i].thread_id, s_update[i].opt_type,
                 s_update[i].ret ) ;
      }
   }

   for ( i = 0; i < QUERY_THREAD_NUM; i++ )
   {
      if ( SDB_OK != s_query[i].ret )
      {
         run_status_flag = FALSE ;
         printf( "Error: thread[%d][%d] return %d"OSS_NEWLINE,
                 s_query[i].thread_id, s_query[i].opt_type,
                 s_query[i].ret ) ;
      }
   }

   ASSERT_EQ( TRUE, run_status_flag ) << "Thread return error" ;

   // check record in database
   if ( FALSE == _checkRecordNumAndReleaseHandle( h_insert, INSERT_THREAD_NUM ) )
   {
      run_status_flag = FALSE ;
   }
   if ( FALSE == _checkRecordNumAndReleaseHandle( h_delete, DELETE_THREAD_NUM ) )
   {
      run_status_flag = FALSE ;
   }
   if ( FALSE == _checkRecordNumAndReleaseHandle( h_update, UPDATE_THREAD_NUM ) )
   {
      run_status_flag = FALSE ;
   }
   if ( FALSE == _checkRecordNumAndReleaseHandle( h_query, QUERY_THREAD_NUM ) )
   {
      run_status_flag = FALSE ;
   }

   ASSERT_EQ( TRUE, run_status_flag ) << "Test failed for error happened" ;
}

