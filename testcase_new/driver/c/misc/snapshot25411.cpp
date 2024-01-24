/*
 * @Description   : SEQUOIADBMAINSTREAM-7485：C 驱动需要支持死锁检测功能 seqDB-25411
 * @Author        : xiao zhenfan
 * @CreateTime    : 2022.02.25
 * @LastEditTime  : 2022.03.12
 * @LastEditors   : xiao zhenfan
 */
#include <gtest/gtest.h>
#include <client.h>
#include <pthread.h>
#include <time.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"


using namespace import ;

#define ThreadNum 2
int thread1Continue = 0 ;
int thread2Continue = 0 ;

void wait_ctrl( int *control )  //用于控制两个线程中的删除操作在更新操作完成之后再发生
{
   clock_t start,end ;
   start = time( NULL ) ;
   do
   {
      sleep(1);
      end = time( NULL ) ;
   }while( *control == 0 || difftime(end,start)>60 );
}

class snapshot25411 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;
   const CHAR* indexName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   void SetUp() 
   { 
      testBase::SetUp() ;  
      INT32 rc = SDB_OK ;
      csName = "cs_25411" ;
      clName = "cl_25411" ;
      clFullName = "cs_25411.cl_25411" ;
      cs = 0 ;
      cl = 0 ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
      // insert datas
      bson* docs[5] ; 
      for( INT32 i = 0; i < 2; i++ )
      {   
         docs[i] = bson_create() ;
         bson_append_int( docs[i], "_id", i ) ; 
         bson_finish( docs[i] ) ; 
      }   
      rc = sdbBulkInsert( cl, 0, docs, 2 ) ; 
      ASSERT_EQ( SDB_OK, rc ) ; 
      for( INT32 i = 0; i < 2; i++ )
      {   
         bson_dispose( docs[i] ) ; 
      }
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   sdbCollectionHandle cl ;	// collection
   sdbConnectionHandle db ;	// connection
   INT32 updateCond ; //更新的条件
   INT32 updateValue ; //更新后的值
   INT32 deleteCond ; //删除的条件
} ;

void func_thread ( ThreadArg* arg )
{
   sdbCollectionHandle cl = arg->cl ;
   sdbConnectionHandle db = arg->db ;
   INT32 updateCond = arg->updateCond ;
   INT32 updateValue = arg->updateValue ;
   INT32 deleteCond = arg->deleteCond ;
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "transactiontimeout", 5 ) ;
   bson_finish( &obj ) ;
   //设置事务超时时间为5s
   rc = sdbUpdateConfig( db, &obj, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "thread1 fail to sdbUpdateConfig rc=" << rc ;
   bson_destroy( &obj ) ;

   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "_id", updateCond ) ;
   bson_finish( &cond ) ;
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "_id", updateValue ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   //开启事务
   rc = sdbTransactionBegin( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "thread1 fail to begin trans rc=" << rc ;
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "thread1 fail to update rc=" << rc ;
   bson_destroy( &cond ) ;
   bson_destroy( &rule ) ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "_id", deleteCond ) ;
   bson_finish( &cond ) ;
   if( updateCond == 0 ) 
   {
      thread1Continue = 1;
      wait_ctrl( &thread2Continue ) ;
   }
   else 
   {
      thread2Continue = 1;
      wait_ctrl( &thread1Continue ) ;
   }
   rc = sdbDelete( cl, &cond, NULL ) ;
   if ( (rc != SDB_OK) && (rc!= -13) )
   {
      std::cout << "Thread1 failed to delete, rc= " << rc << std::endl ;
   }
   bson_destroy( &cond ) ;
   rc = sdbTransactionRollback( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "thread1 fail to rollback trans rc=" << rc ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "transactiontimeout", 60 ) ;
   bson_finish( &obj ) ;
   //将事务超时时间恢复为默认值60s
   rc = sdbUpdateConfig( db, &obj, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "thread1 fail to sdbUpdateConfig rc=" << rc ;
   bson_destroy( &obj ) ;
}

TEST_F( snapshot25411, lockwaitANDdeadlock )
{
   sdbConnectionHandle db1 = SDB_INVALID_HANDLE ;
   sdbConnectionHandle db2 = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl1 ;
   sdbCollectionHandle cl2 ;
   INT32 rc = SDB_OK ;
   INT32 rc1, rc2 ;
   //创建两个线程，两个线程使用不同的sdb连接
   Worker* workers[ ThreadNum ] ;
   ThreadArg arg[ ThreadNum ] ;
   rc1 = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   rc2 = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db2 ) ;
   ASSERT_EQ( SDB_OK, rc1 ) << "fail to sdbConnect,rc = " << rc  ;
   ASSERT_EQ( SDB_OK, rc2 ) << "fail to sdbConnect,rc = " << rc  ;
   rc1 = sdbGetCollection( db1, clFullName, &cl1 );
   rc2 = sdbGetCollection( db2, clFullName, &cl2 );

   arg[0].db = db1 ;
   arg[1].db = db2 ;
   arg[0].cl = cl1 ;
   arg[1].cl = cl2 ;
   arg[0].updateCond = 0 ;
   arg[1].updateCond = 1 ;
   arg[0].updateValue = 10 ;
   arg[1].updateValue = 20 ;
   arg[0].deleteCond = 1 ;
   arg[1].deleteCond = 0 ;
   
   workers[0] = new Worker( (WorkerRoutine)func_thread, &arg[0], false ) ;
   workers[1] = new Worker( (WorkerRoutine)func_thread, &arg[1], false ) ;
   workers[0]->start() ;
   workers[1]->start() ;
   wait_ctrl( &thread1Continue ) ;
   wait_ctrl( &thread2Continue ) ;
   sdbCursorHandle cursor ;
   //获取死锁快照
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSDEADLOCK, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbGetSnapshot SDB_SNAP_TRANSDEADLOCK rc=" << rc ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbNext rc=" << rc ;
   bson_type type ;
   bson_iterator it ;
   type = bson_find( &it, &obj, "DeadlockID" ) ;
   ASSERT_EQ( BSON_INT, type ) << "type of 'DeadlockID' should be 'BSON_INT',actually is "<< type ;
   bson_destroy( &obj ) ;
   //获取事务等待快照
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSWAITS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbGetSnapshot SDB_SNAP_TRANSWAITS rc=" << rc ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbNext rc=" << rc ;
   type = bson_find( &it, &obj, "WaiterTransID" ) ;
   ASSERT_EQ( BSON_STRING, type ) << "type of 'DeadlockID' should be 'BSON_STRING',actually is "<< type ;
   bson_destroy( &obj ) ;
   for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;
   sdbDisconnect( db1 ) ;
   sdbReleaseConnection( db1 ) ;
   sdbDisconnect( db2 ) ;
   sdbReleaseConnection( db2 ) ;
}
