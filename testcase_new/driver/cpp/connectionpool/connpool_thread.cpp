/*****************************************************
 * c++驱动连接池并发测试的线程函数
 *
 *****************************************************/
#include "connpool_thread.hpp"
#include "connpool_common.hpp"
#include <ctime>

using namespace std ;

INT32 getRand()
{
   static BOOLEAN inited = TRUE ;
   if( inited )
   {
      srand( time( NULL ) ) ;
      inited = FALSE ;
   }
   return rand()%10 ;
}

void init( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
   ossSleep( getRand() * 100 ) ;
   sdbConnectionPoolConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
}

void init_close( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
   ossSleep( getRand() * 100 ) ;
   sdbConnectionPoolConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   arg->getDs().close() ;
}

void init_conn( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
   ossSleep( getRand() * 100 ) ;
   sdbConnectionPoolConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   arg->getDs().releaseConnection( conn ) ;
}

void dsclose( DsArgs* arg )
{
   ossSleep( getRand() * 100 ) ;
   arg->getDs().close() ;
}

void dsclose_conn( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
   ossSleep( getRand() * 100 ) ;
   sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
   // ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   arg->getDs().releaseConnection( conn ) ;
   arg->getDs().close() ;
}

void connection( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
   ossSleep( getRand() * 100 ) ;
   sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   arg->getDs().releaseConnection( conn ) ;
}

void releaseConn( DsArgs* arg )
{
   ossSleep( getRand() * 100 ) ;
   vector<sdb*> vec = arg->getConnVec() ;
   for( INT32 i = 0;i != vec.size();++i )
   {
      arg->getDs().releaseConnection( vec[i] ) ;
   }
}

