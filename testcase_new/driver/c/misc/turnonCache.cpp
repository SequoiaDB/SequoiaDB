/*****************************************************
 * @Description: test case for c driver
 *				     
 * @Modify:      Liang xuewang
 *				     2017-09-27
 *****************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "impWorker.hpp"

#define SOCKET int

struct Socket
{
   SOCKET      rawSocket ;
#ifdef SDB_SSL
   SSLHandle*  sslHandle ;
#endif
} ;

typedef struct _htbNode
{
   UINT64 lastTime ;
   CHAR *name ;
} htbNode ;

typedef struct _hashTable
{
   UINT32  capacity ;
   htbNode **node ;
} hashTable ;

struct _Node
{
   ossValuePtr data ;
   struct _Node *next ;
} ;

typedef struct _Node Node ;
typedef INT32 BOOL ;

enum BOOLEANVAL
{
    BOOLEAN_FALSE = 0 ,
    BOOLEAN_TRUE
} ;

#define ossMutex   pthread_mutex_t

struct _sdbConnectionStruct
{
   // generic variables, to validate which type does this handle belongs to
   INT32 _handleType ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;
   Node *_cursors ;
   Node *_sockets ;
   hashTable *_tb ;

   UINT64 reserveSpace1 ;
   ossMutex _sockMutex ;
} ;

typedef struct _sdbConnectionStruct sdbConnectionStruct ;

INT32 connect( sdbConnectionHandle* db, INT32 timeLen = 0 )
{
   INT32 rc = SDB_OK ;
   
   // 初始化客户端，开启缓存
   sdbClientConf conf;
   conf.enableCacheStrategy = TRUE ;
   conf.cacheTimeInterval = timeLen ;
   rc = initClient( &conf ) ;
   CHECK_RC( SDB_OK, rc, "fail to init client" ) ;
   
   //建立连接,判断hashtable是否为空
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), db ) ;
   CHECK_RC( SDB_OK, rc, "fail to connect" ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 getTimeofGetCS( sdbConnectionHandle db,
                      const CHAR* csName,
                      sdbCSHandle* cs,
                      clock_t* diff )
{
   timeval begin, end ;
   INT32 rc = SDB_OK ;
   gettimeofday( &begin, NULL ) ;
   rc = sdbGetCollectionSpace( db, csName, cs ) ;
   gettimeofday( &end, NULL ) ;
   *diff = ( end.tv_sec - begin.tv_sec ) * 1000000 + end.tv_usec - begin.tv_usec ;
   return rc ;
}

INT32 getTimeofGetCLByFullName( sdbConnectionHandle db, 
                                const CHAR* csName,
                                const CHAR* clName, 
                                sdbCollectionHandle* cl,
                                clock_t* diff )
{
   CHAR fullName[256] ;
   timeval begin, end ;
   INT32 rc = SDB_OK ;
   snprintf( fullName, sizeof(fullName), "%s.%s", csName, clName ) ;
   gettimeofday( &begin, NULL ) ;
   rc = sdbGetCollection( db, fullName, cl ) ;
   gettimeofday( &end, NULL ) ;
   *diff = ( end.tv_sec - begin.tv_sec ) * 1000000 + end.tv_usec - begin.tv_usec ;
   return rc ;
}

INT32 getTimeofGetCLByName( sdbCSHandle cs, 
                            const CHAR* clName, 
                            sdbCollectionHandle* cl,
                            clock_t* diff )
{
   INT32 rc = SDB_OK ;
   timeval begin, end ;
   gettimeofday( &begin, NULL ) ;
   rc = sdbGetCollection1( cs, clName, cl ) ;
   gettimeofday( &end, NULL ) ;
   *diff = ( end.tv_sec - begin.tv_sec ) * 1000000 + end.tv_usec - begin.tv_usec ;
   return rc ;
}

// 测试开启缓存后创建cs cl正常
TEST( turnonCache, createCollection )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* csName = "turnonCacheTestCs" ;
   const CHAR* clName = "turnonCacheTestCl" ;
   
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
 
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

// 测试新建连接两次获取相同cs，第一次走查询，第二次走缓存
TEST( turnonCache17276, getCollectionSpace )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbConnectionHandle db1 ;
   sdbCSHandle cs, cs1, cs2 ;
   const char* csName = "turnonCacheTestCs17276" ;
   clock_t diff1, diff2 ;
  
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
  
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   
   // first time to get cs, since db1 has no cache yet, 
   // it will query coord, then insert cs into local cache
   rc = getTimeofGetCS( db1, csName, &cs1, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // second time to get cs, get cs from local cache
   rc = getTimeofGetCS( db1, csName, &cs2, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // local cache takes less time than query coord
   ASSERT_LT( diff2, diff1 ) << "fail to check cache" ;
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbReleaseCS( cs1 ) ;
   sdbReleaseCS( cs2 ) ;
   sdbDisconnect( db ) ;
   sdbDisconnect( db1 ) ;
   sdbReleaseConnection( db ) ;
   sdbReleaseConnection( db1 ) ;
}

// 测试新建连接两次获取相同cl，第一次走查询，第二次走缓存
TEST( turnonCache, getCollection )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbConnectionHandle db1 ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl, cl1, cl2 ;
   const CHAR* csName = "turnonCacheTestCs" ;
   const CHAR* clName = "turnonCacheTestCl" ;
   clock_t diff1, diff2 ;
   
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   
   rc = getTimeofGetCLByFullName( db1, csName, clName, &cl1, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = getTimeofGetCLByFullName( db1, csName, clName, &cl2, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_LT(diff2, diff1);
 
   rc = sdbDropCollectionSpace( db, csName ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbDisconnect( db1 ) ;
   sdbReleaseCollection( cl1 ) ;
   sdbReleaseCollection( cl2 ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
   sdbReleaseConnection( db1 ) ; 
}

// 测试新建连接两次获取相同cl，第一次走查询，第二次走缓存
TEST( turnonCache, getCollection1 )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbConnectionHandle db1 ;
   sdbCSHandle cs, cs1 ;
   sdbCollectionHandle cl, cl1, cl2 ;
   const CHAR* csName = "turnonCacheTestCs" ;
   const CHAR* clName = "turnonCacheTestCl" ;
   clock_t diff1, diff2 ;
   
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   
   rc = sdbGetCollectionSpace( db1, csName, &cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = getTimeofGetCLByName( cs1, clName, &cl1, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getTimeofGetCLByName( cs1, clName, &cl2, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_LT( diff2, diff1 ) ;
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbDisconnect( db1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCollection( cl1 ) ;
   sdbReleaseCollection( cl2 ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCS( cs1 ) ;
   sdbReleaseConnection( db ) ;
   sdbReleaseConnection( db1 ) ;   
}

// 测试缓存超时前后获取相同cs
TEST( turnonCache17277, getCollectionSpaceOfTimeOut )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs, cs1, cs2 ;
   const CHAR* csName = "turnonCacheTestCs17277" ;
   clock_t diff1, diff2 ;
   
   //初始化客户端，启用缓存，超时1s
   rc = connect( &db, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   
   // 统计获取CS的时间
   rc = getTimeofGetCS( db, csName, &cs1, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
      
   //睡眠让cache超时
   ossSleep( 2000 ) ;
   
   // 再次统计获取CS的时间
   rc = getTimeofGetCS( db, csName, &cs2, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_LT( diff1, diff2 ) ;
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCS( cs1 ) ;
   sdbReleaseCS( cs2 ) ;
   sdbReleaseConnection( db ) ;
}

// 测试缓存超时前后获取相同cl
TEST( turnonCache17278, getCollectionOfTimeOut )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl, cl1, cl2 ;
   const CHAR* csName = "turnonCacheTestCs17278" ;
   const CHAR* clName = "turnonCacheTestCl17278" ;
   clock_t diff1, diff2 ;
   
   //初始化客户端，启用缓存，超时1s
   rc = connect( &db, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // 统计获取CL的时间
   rc = getTimeofGetCLByName( cs, clName, &cl1, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // 让cache超时
   ossSleep( 2000 ) ;
   rc = getTimeofGetCLByName( cs, clName, &cl2, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_LT( diff1, diff2 ) ;
  
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCollection( cl1 ) ;
   sdbReleaseCollection( cl2 ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

// 测试删除cs后获取cs
TEST( turnonCache17279, getCollectionSpaceAfterDrop)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   const CHAR* csName = "turnonCacheTestCs17279" ;
   
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   rc = sdbGetCollectionSpace( db, csName, &cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;
   
   sdbDisconnect( db ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

// 测试删除cl后获取cl
TEST( turnonCache17280, getCollectionAfterDrop )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl, cl1, cl2 ;
   const CHAR* csName = "turnonCacheTestCs17280" ;
   const CHAR* clName = "turnonCacheTestCl17280" ;
   const CHAR* fullName = "turnonCacheTestCs17280.turnonCacheTestCl17280" ;
   
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   
   rc = sdbGetCollection( db, fullName, &cl1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << fullName ;
   
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
   
   rc = sdbGetCollection1( cs, clName, &cl2 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to check drop cl" ;
   
   rc = sdbGetCollection( db, fullName, &cl2 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to check drop cl" ;
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCollection( cl1 ) ;
   sdbReleaseCollection( cl2 ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

// 测试执行操作使缓存内的时间更新
TEST( turnonCache, testUpdateTimeStamp )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbConnectionStruct* strConn ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* csName = "turnonCacheTestCs" ;
   const CHAR* clName = "turnonCacheTestCl" ;
   UINT64 csTimeStamp = time( NULL ) ;
   UINT64 clTimeStamp = time( NULL ) ;
     
   rc = connect( &db, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   
   strConn = (sdbConnectionStruct*)db ;
   hashTable* ht = strConn->_tb ;
   for( INT32 i = 0;i < ht->capacity;++i )
   {
      htbNode* node = ht->node[i] ;
      if( node )
      {
         if( 0 == strncmp( node->name, csName, strlen(node->name) ) )
         {
            csTimeStamp = node->lastTime ;   
         }
         else
         {
            clTimeStamp = node->lastTime ; 
         }
      }
   } 
   
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "_id", 0 ) ;
   bson_finish( &obj ) ;
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   for( INT32 i = 0;i < ht->capacity;++i )
   {
      htbNode* node = ht->node[i] ;
      if( node )
      {
         if( 0 == strncmp( node->name, csName, strlen(node->name) ) )
         {
            ASSERT_LE( csTimeStamp, node->lastTime ) << "fail to check cs lastTime" ; 
         }
         else
         {
            ASSERT_LE( clTimeStamp, node->lastTime ) << "fail to check cl lastTime" ;  
         }
      }
   } 
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

// 新建连接删除cl后使用旧连接获取cl（缓存超时）
TEST( turnonCache, getCLOfTimeOutandDropbyOtherConn )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db, db1 ;
   sdbCSHandle cs, cs1 ;
   sdbCollectionHandle cl, cl1 ;
   const CHAR* csName = "turnonCacheTestCs" ;
   const CHAR* clName = "turnonCacheTestCl" ;
   const CHAR* fullName = "turnonCacheTestCs.turnonCacheTestCl" ;
 
   rc = connect( &db, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
 
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   
   //新建连接，删除cl
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   rc = sdbGetCollectionSpace( db1, csName, &cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs " << csName ;
   rc = sdbDropCollection( cs1, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
   
   // 让cache超时
   ossSleep( 2000 ) ;
   rc = sdbGetCollection( db, fullName, &cl1 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to check drop cl" ;
   rc = sdbGetCollection1( cs, clName, &cl1 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to check drop cl" ;

   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   sdbDisconnect( db ) ;
   sdbDisconnect( db1 ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCollection( cl1 ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCS( cs1 ) ;
   sdbReleaseConnection( db ) ;
   sdbReleaseConnection( db1 ) ; 
}

// 新建连接删除cs后使用旧连接获取cs（缓存超时）
TEST( turnonCache, getCSOfTimeOutandDropbyOtherConn )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db, db1 ;
   sdbCSHandle cs, cs1 ;
   const CHAR* csName = "turnonCacheTestCs" ;
   const CHAR* clName = "turnonCacheTestCl" ;
   
   rc = connect( &db, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   
   //新建连接，删除上述cs
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   rc = sdbDropCollectionSpace( db1, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   
   // 让cache超时
   ossSleep( 2000 ) ;

   rc = sdbGetCollectionSpace( db, csName, &cs1 ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to check drop cs" ;
   
   sdbDisconnect( db ) ;
   sdbDisconnect( db1 ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseCS( cs1 ) ;
   sdbReleaseConnection( db ) ;
   sdbReleaseConnection( db1 ) ;
}

// 删除cs后获取cs下的多个cl
TEST( turnonCache17281, getMulCLAfterDropCS)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs, cs1 ;
   sdbCollectionHandle cl[5], cl1[5] ;
   const CHAR* csName = "turnonCacheTestCs17281" ;
   CHAR clName[127] ;
   CHAR fullName[256] ;
  
   rc = connect( &db, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   
   for( INT32 i = 0;i < 5;++i )
   {
      snprintf( clName, sizeof(clName), "%s%d", "turnonCacheTestCl17281", i ) ;
      rc = sdbCreateCollection( cs, clName, &cl[i] ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   }
   
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   
   INT32 expectRes = SDB_DMS_NOTEXIST ;
   if( isStandalone( db ) )
   {
      expectRes = SDB_DMS_CS_NOTEXIST ;
   }

   for( INT32 i = 0;i < 5;++i )
   {
      snprintf( fullName, sizeof(fullName), "%s.%s%d", csName, "turnonCacheTestCl17281", i ) ;
      rc = sdbGetCollection( db, fullName, &cl1[i] ) ;
      ASSERT_EQ( expectRes, rc ) << "fail to test get cl " << fullName ;
   }
 
   sdbDisconnect( db ) ;
   for( INT32 i = 0;i < 5;++i )
   {
      sdbReleaseCollection( cl[i] ) ;
   } 
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}
