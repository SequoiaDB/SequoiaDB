/***********************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9524:提供不合法的_initConnCount
 *               seqDB-9525:提供不合法的 _deltaIncCount
 *               seqDB-9526:提供不合法的 _keepAliveTimeout <checkInterval 
 *               seqDB-9527:checkInterval设置为0
 *               seqDB-9528:init时url不符合格式要求
 *               seqDB-9529:init时vUrls.size为0，push的url不符合格式要求                 
 * @Modify:      Liangxw
 *               2019-09-05
 ***********************************************************************/
#include <sdbConnectionPool.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <gtest/gtest.h>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class invalidArgTest9524 : public testBase
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;
   string url ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      url = ARGS->coordUrl() ;
   }
   void TearDown()
   {
      ds.close() ;
   }
} ;

// 测试使用不合法的连接配置参数( 9524-9525 )
TEST_F( invalidArgTest9524, connCntInfo9524 )
{
   INT32 rc = SDB_OK ;

   // _initConnCount<0
   conf.setConnCntInfo( -3, 10, 20, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with initConnCount -3" ;

   // _initConnCount=0
   conf.setConnCntInfo( 0, 10, 20, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with initConnCount 0" ;
   ds.close() ;

   // _initConnCount>maxIdleCount,change to maxIdleCount
   conf.setConnCntInfo( 25, 10, 20, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with initConnCount>maxIdleCount" ;
   ASSERT_EQ( conf.getMaxIdleCount(), ds.getIdleConnNum() ) << "fail to check idle conn num" ;
   ds.close() ;

   // _deltaIncCount<0
   conf.setConnCntInfo( 10, -3, 20, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with deltaIncCount -3" ;

   // _deltaIncCount=0
   conf.setConnCntInfo( 10, 0, 20, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with deltaIncCount 0" ;

   // _deltaIncCount>maxIdleCount,change to maxIdleCount
   conf.setConnCntInfo( 10, 25, 20, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with deltaIncCount>maxIdleCount" ;
   vector<sdb*> vec ;
   while( ds.getIdleConnNum() > SDB_CONNPOOL_TOPRECREATE_THRESHOLD )
   {
      sdb* conn = NULL ;
      rc = ds.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
      vec.push_back( conn ) ;
      // cout << ds.getIdleConnNum() << endl ;
   }
   ossSleep( 2000 ) ;
   // cout << ds.getIdleConnNum() << endl ;
   ASSERT_EQ( ds.getIdleConnNum()-SDB_CONNPOOL_TOPRECREATE_THRESHOLD, conf.getMaxIdleCount() )
              <<  "fail to check deltaIncCount" ;
   for( int i = 0;i < vec.size();i++ )
   {
      ds.releaseConnection( vec[i] ) ;
   }
   ds.close() ;

   // _maxIdleCount<0
   conf.setConnCntInfo( 10, 10, -3, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with maxIdleCount -3" ;

   // _maxIdleCount=0
   conf.setConnCntInfo( 10, 10, 0, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with maxIdleCount 0" ;
   
   // _maxIdleCount=maxCount
   conf.setConnCntInfo( 10, 10, 500, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with maxIdleCount=maxCount" ;
   ds.close() ;

   // _maxIdleCount>maxCount
   conf.setConnCntInfo( 10, 10, 600, 500 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with maxIdleCount>maxCount" ;

   // _maxCount<0
   conf.setConnCntInfo( 10, 10, 20, -3 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with maxCount -3" ;

   // _maxCount=0
   conf.setConnCntInfo( 10, 10, 20, 0 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with maxCount 0" ;

   // _maxCount=2147483647
   conf.setConnCntInfo( 10, 10, 20, 2147483647 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with maxCount 2147483647" ;
}

// 测试使用不合法的连接配置参数( 9526-9527 )
TEST_F( invalidArgTest9524, checkIntervalInfo9526 )
{
   INT32 rc = SDB_OK ;

   // _checkInterval<0
   conf.setCheckIntervalInfo( -3, 0 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with checkInterval -3" ;

   // _checkInterval=0
   conf.setCheckIntervalInfo( 0, 0 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with checkInterval 0" ;

   // _checkInterval>_keepAliveTimeout
   conf.setCheckIntervalInfo( 60, 30 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with checkInterval>keepAliveTimeout" ;

   // _keepAliveTimeout<0
   conf.setCheckIntervalInfo( 30, -3 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with keepAliveTimeout -3" ;
}

// 测试使用不合法的连接配置参数
TEST_F( invalidArgTest9524, coordInterval9526 )
{
   INT32 rc = SDB_OK ;

   // _syncCoordInterval<0
   conf.setSyncCoordInterval( -3 ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with syncCoordInterval -3" ;
}

// 测试使用不合法的连接配置参数
TEST_F( invalidArgTest9524, connectStrategy9526 )
{
   INT32 rc = SDB_OK ;   

   // _connectStrategy不为0-3
   conf.setConnectStrategy( SDB_CONN_STRATEGY( 5 ) ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with connectStrategy 5" ;
}

//  url格式检验，检验空地址和格式不符合xxxx:xxxx的地址，init时不会报错但getConnection时会报错
TEST_F( invalidArgTest9524, url9528 )
{
   INT32 rc = SDB_OK ;   

   // init with illeagal url
   string url_wrong1 = "something" ;
   string url_wrong2 = "something::00000" ;
   rc = ds.init( url_wrong1, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with url " << url_wrong1 ;
   rc = ds.init( url_wrong2, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with url " << url_wrong2 ;

   // init with legal url
   string url_right1 = "something:" ;
   string url_right2 = ":000000" ;
   string url_right3 = ":" ;
   rc = ds.init( url_right1, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with url " << url_right1 ;
   ds.close() ;
   rc = ds.init( url_right2, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with url " << url_right2 ;
   ds.close() ;
   rc = ds.init( url_right3, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init with url " << url_right3 ;
}

// url列表测试
TEST_F( invalidArgTest9524, urlist9529 )
{
   INT32 rc = SDB_OK ;   
   
   // init with illeagal url arr
   vector<string> urlArray1( 10, "" ) ;
   rc = ds.init( urlArray1, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with illeagal url arr" ;

   // init with empty url arr
   vector<string> urlArray2 ;
   rc = ds.init( urlArray2, conf ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test init with empty url arr" ;
}
