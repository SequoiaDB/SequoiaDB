/*****************************************************************************
 * @Description : seqDB-8196:同时创建删除同一个存储过程
 *                seqDB-8197:同时删除执行同一个存储过程
 * @Modify List : Ting YU
 *                2016-09-12
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <string.h>
#include <client.h>
#include "testcommon.hpp"
#include "arguments.hpp"

void* create_pcd( void* arg ) ;
void* remove_pcd( void* arg ) ;
void* excute_pcd( void* arg ) ;

TEST( procedure, cocurrentCreateRemoveExcute )
{  
   INT32 rc = SDB_OK;  

   sdbConnectionHandle db ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ; 
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone!\n" ) ;
      sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
      return ;
   }   

   // create procedure thread
   pthread_t ctid; 
   rc = pthread_create( &ctid, NULL, create_pcd, NULL ) ;
   ASSERT_EQ( 0, rc ) << "fail to create thread" ;  

   // remove procedure thread
   pthread_t rtid; 
   rc = pthread_create( &rtid, NULL, remove_pcd, NULL ) ;
   ASSERT_EQ( 0, rc ) << "fail to create thread" ;

   // excute procedure thread
   pthread_t etid; 
   rc = pthread_create( &etid, NULL, excute_pcd, NULL ) ;
   ASSERT_EQ( 0, rc ) << "fail to create thread" ;  

   // wait sub thread end   
   void* c_tret; 
   pthread_join( ctid, &c_tret ) ;   
   ASSERT_EQ( (long)c_tret, 0 ) << "error in create_pcd thread" ;

   void* r_tret ; 
   pthread_join( rtid, &r_tret ) ;   
   ASSERT_EQ( (long)r_tret, 0 ) << "error in remove_pcd thread" ;

   void* e_tret; 
   pthread_join( etid, &e_tret ) ;   
   ASSERT_EQ( (long)e_tret, 0 ) << "error in excute_pcd thread" ;
}

void* excute_pcd( void* arg )
{  
   printf( "---begin thread: excute procedure\n" ) ;      
   INT32 rc ;
   sdbConnectionHandle db ;
   bson obj ;
   bson_iterator it ;

   // new db
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db );
   if( rc != SDB_OK )
   {
      printf( "fail to connect, rc = %d\n", rc ) ; 
      pthread_exit( (void*)2 ) ;    
   }

   // excute procedure multi times
   for( INT32 i = 0;i < 20;i++ )
   {
      sdbCursorHandle cursor ;
      bson errmsg  ;
      bson_init( &errmsg ) ;
      SDB_SPD_RES_TYPE valueType = SDB_SPD_RES_TYPE_VOID  ;
      CHAR code[] = "c_driver_procedure_sum(100,2)";
      rc = sdbEvalJS( db, code, &valueType, &cursor, &errmsg ) ;
      if( rc == SDB_OK )
      {  // ok, expect 0 or -152                     
         bson_init( &obj ) ;        
         rc = sdbNext( cursor, &obj );                    
         bson_find( &it, &obj, "value" );          
         if( bson_iterator_int(&it) != 102 )
         {
            printf( "fail to excute procedure, times = %d, sum result = %d\n", 
                    i, bson_iterator_int( &it ) ) ;  
            pthread_exit( (void*)3 ) ; 
         }   
         bson_destroy( &obj ) ;
         bson_init( &obj ) ;
      }
      else if( rc == SDB_SPT_EVAL_FAIL )
      {  // ok, expect 0 or -152 
      }
      else
      {
         printf( "fail to excute procedure, times = %d, rc = %d\n", i, rc ) ;  
         pthread_exit( (void*)1 ) ; 
      }
   }

   // release db
   sdbDisconnect ( db );
   sdbReleaseConnection( db );

   pthread_exit( (void*)0 ) ;   
}

void* create_pcd( void* arg )
{  
   printf( "---begin thread: create procedure\n" ) ;      
   INT32 rc ;   
   sdbConnectionHandle db ;

   // new db
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db );
   if( rc != SDB_OK )
   {
      printf( "fail to connect, rc = %d\n", rc ) ; 
      pthread_exit( (void*)2 ) ;    
   }

   // create procedure multi times
   for( INT32 i = 0;i < 20;i++ )
   {
      rc = sdbCrtJSProcedure( db, "function c_driver_procedure_sum(x, y){return x+y;}" ) ;
      if( rc == SDB_OK || rc == SDB_FMP_FUNC_EXIST )
      {
         // ok, expect 0 or -342           
      }
      else
      {
         printf( "fail to create procedure, times = %d, rc = %d\n", i, rc );  
         pthread_exit( (void*)1 ) ; 
      }        
   }

   // release db
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;

   pthread_exit( (void*)0 ) ;   
}

void* remove_pcd( void* arg )
{
   printf( "---begin thread: remove procedure\n" ) ;  
   INT32 rc ;
   sdbConnectionHandle db ;

   // new db
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db );
   if( rc != SDB_OK )
   {
      printf( "fail to connect, rc = %d\n", rc ); 
      pthread_exit( (void*)2 ) ;    
   }

   // remove procedure multi times
   for( INT32 i = 0;i < 20;i++ )
   {
      rc = sdbRmProcedure( db, "c_driver_procedure_sum" ) ;      
      if( rc == SDB_OK || rc == SDB_FMP_FUNC_NOT_EXIST )
      {
         // ok, expect 0 or -233           
      }
      else
      {
         printf( "fail to remove procedure, times = %d, rc = %d\n", i, rc ); 
         pthread_exit( (void*)1 ) ; 
      }
   }

   // release db
   sdbDisconnect( db ) ; 
   sdbReleaseConnection( db ) ;

   pthread_exit( (void*)0 ) ; 
}
