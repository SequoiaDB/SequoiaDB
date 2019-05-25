#include <stdio.h>
#include <gtest/gtest.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "client.h"
#include "testcommon.h"
#include <time.h>
#include <sys/time.h>

#define NUM_RECORD            5

const int N                   = 10 ;
const SINT64 NUM              = 5000 ;

pthread_mutex_t mutex ;
pthread_cond_t cond ;
int thread_amount = 0 ;
int cnt = 1 ;

void initEnv()
{
   sdbConnectionHandle connection    = 0 ;
   sdbCSHandle cs                    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;

   const CHAR *pHostName             = HOST ;
   const CHAR *pServiceName          = SERVER ;
   const CHAR *pUsr                  = USER ;
   const CHAR *pPasswd               = PASSWD ;

   bson obj ;
   INT32 count = 0;
   INT32 rc    = SDB_OK ;
   bson objList [ NUM_RECORD ] ;
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   if( rc != SDB_OK )
      printf( "*** initEnv():sdbConnect() fail! ***\n" ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   if( rc != SDB_OK )
      printf( "*** initEnv():getCollectionSpace() fail! ***\n" ) ;
   rc = getCollection ( connection,
                        COLLECTION_FULL_NAME,
                        &collection ) ;
   if( rc != SDB_OK )
      printf( "*** initEnv():getCollection() fail! ***\n" ) ;
   sdbDelete( collection, NULL, NULL ) ;
   createNameList ( &objList[0], NUM_RECORD ) ;
   for ( count = 0; count < NUM_RECORD; count++ )
   {
      rc = sdbInsert ( collection, &objList[count] ) ;
      if ( rc )
      {
         printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
      }
      bson_destroy ( &objList[count] ) ;
   }
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection ( connection ) ;
}


void sdbBulkInsert()
{
   sdbConnectionHandle connection = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   const CHAR *pHostName             = HOST ;
   const CHAR *pServiceName          = SERVER ;
   const CHAR *pUsr                  = USER ;
   const CHAR *pPasswd               = PASSWD ;

   int count                      = 0 ;
   SINT64 totalNum                = 0 ;
   bson obj ;
   bson *objList [ NUM ] ;

   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   if( rc != SDB_OK )
      printf( "*** sdbBulkInsert():sdbConnect() fail! ***\n" ) ;
   rc = getCollection ( connection, COLLECTION_FULL_NAME , &collection ) ;
   if( rc != SDB_OK )
      printf( "*** sdbBulkInsert():getCollection() fail! ***\n" ) ;

   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   if( rc != SDB_OK )
      printf( "*** sdbBulkInsert():sdbGetCount() fail! ***\n" ) ;

   printf("Before bulk insert, the total number of records is %lld\n",totalNum ) ;
   for ( count = 0; count < NUM; count++ )
   {
      objList[count] = bson_create() ;
      if ( !jsonToBson ( objList[count],"{firstName:\"John\",\
                                  lastName:\"Smith\",age:50}" ) )
      {
         printf ( "Failed to convert json to bson." OSS_NEWLINE ) ;
         continue ;
      }
   }
   rc = sdbBulkInsert ( collection, 0, objList, NUM ) ;
   rc = sdbGetCount ( collection, NULL, &totalNum ) ;
   if( rc != SDB_OK )
      printf( "*** sdbBulkInsert():sdbGetCount() fail! ***\n" ) ;
   printf("After bulk insert,the total number of records is %lld\n",totalNum ) ;

   for ( count = 0; count < NUM; count++ )
   {
      bson_dispose ( objList[count] ) ;
   }

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
}

void *p_thread( void *arg )
{
   int i = cnt ;
   cnt++ ;
   pthread_mutex_lock( &mutex ) ;

   printf( "thread %d wait...\n", i ) ;
   while( 0 == thread_amount )
   {
      pthread_cond_wait( &cond, &mutex ) ;   // must be placed between lock and unlock
   }

   pthread_mutex_unlock( &mutex ) ;
   sdbBulkInsert() ;

   printf( "thread %d end!!!\n", i ) ;
}


void *awake( void *arg )
{
   pthread_mutex_lock( &mutex ) ;
   printf( "awake now !\n" ) ;
   thread_amount++ ;
   pthread_cond_broadcast( &cond ) ;
   pthread_mutex_unlock( &mutex ) ;
}

/*
int main()
{
   initEnv() ;

   pthread_t tt[N] ;
   int i = 0 ;

   pthread_mutex_init( &mutex, NULL ) ;
   pthread_cond_init( &cond, NULL ) ;

   for( i=0; i<N-1; ++i )
   {
      pthread_create( tt+i, NULL, p_thread, NULL ) ;
      sleep( 1 ) ;
   }

   pthread_create( tt+N-1, NULL, awake, NULL ) ;
   sleep( 1 ) ;

   timeval starttime, endtime ;
   gettimeofday( &starttime, 0 ) ;
   for( i=0; i<N-1; ++i )
   {
      pthread_join( tt[i], NULL ) ;
   }
   gettimeofday( &endtime, 0 ) ;
   double timeuse = 1000000 * ( endtime.tv_sec - starttime.tv_sec ) +
                                endtime.tv_usec - starttime.tv_usec ;
   timeuse /= 1000000 ;
   printf( "time consumption is %lf s \n", timeuse ) ;
   pthread_exit( 0 ) ;
   return 0 ;
}
*/

TEST( concurrent, insert )
{
   initEnv() ;

   pthread_t tt[N] ;
   int i = 0 ;

   pthread_mutex_init( &mutex, NULL ) ;
   pthread_cond_init( &cond, NULL ) ;

   for( i=0; i<N-1; ++i )
   {
      pthread_create( tt+i, NULL, p_thread, NULL ) ;
      sleep( 1 ) ;
   }

   pthread_create( tt+N-1, NULL, awake, NULL ) ;
   sleep( 1 ) ;

   timeval starttime, endtime ;
   gettimeofday( &starttime, 0 ) ;
   for( i=0; i<N-1; ++i )
   {
      pthread_join( tt[i], NULL ) ;
   }
   gettimeofday( &endtime, 0 ) ;
   double timeuse = 1000000 * ( endtime.tv_sec - starttime.tv_sec ) +
                                endtime.tv_usec - starttime.tv_usec ;
   timeuse /= 1000000 ;
   printf( "time consumption is %lf s \n", timeuse ) ;
}

