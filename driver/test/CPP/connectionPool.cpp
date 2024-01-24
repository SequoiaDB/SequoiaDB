/*******************************************************************************
*@Description : Test connection pool of C++ driver, include _maxIdleCountTest
*@Modify List :
*               2021-10-14   QinCheng Yang
*******************************************************************************/

#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "sdbConnectionPoolComm.hpp"
#include "sdbConnectionPool.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>
#include <vector>

using namespace std ;
using namespace sdbclient ;

TEST( connectionPool, _maxIdleCountTest )
{
   // check connection pool is ok when maxIdleCount is 0s
   INT32 rc               = SDB_OK ;
   INT32 maxIdleCount     = 0 ;
   string host            = HOST ;
   string svcName         = SERVER ;
   string address         = host + ":" + svcName ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   sdb* db1 ;
   sdb* db2 ;

   conf.setAuthInfo( USER, PASSWD ) ;
   conf.setConnCntInfo( 10, 10, maxIdleCount, 500 ) ;

   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check idle conn num after enable
   ASSERT_EQ( maxIdleCount, connPool.getIdleConnNum() ) ;

   rc = connPool.getConnection( db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connPool.getConnection( db2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check idle conn num after getConn
   ASSERT_EQ( maxIdleCount, connPool.getIdleConnNum() ) ;

   connPool.releaseConnection( db1 ) ;
   connPool.releaseConnection( db2 ) ;
   // check idle conn num after release conn
   ASSERT_EQ( maxIdleCount, connPool.getIdleConnNum() ) ;

   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST( connectionPool, updateAuthInfoTest )
{
   // check connection pool is ok when update authentication information
   INT32 rc                   = SDB_OK ;
   string host                = HOST ;
   string svcName             = SERVER ;
   string address             = host + ":" + svcName ;
   string userName1           = "updateAuthInfoTestU1" ;
   string userName2           = "updateAuthInfoTestU2" ;
   string pwd                 = "123";
   string invalidCipherFile   = "invalidCipherFile" ; // a invalid value
   string token               = "" ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   sdb* conn1 ;
   sdb* conn2 ;
   sdb* conn3 ;
   sdb db ;
   BSONObj obj ;

   // 1. create user
   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.createUsr( userName1.c_str(), pwd.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.createUsr( userName2.c_str(), pwd.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // 2. set conf
   conf.setAuthInfo( userName1, pwd ) ;
   conf.setAuthInfo( userName1, invalidCipherFile, token ) ;
   conf.setConnCntInfo( 1, 1, 1, 500 ) ;  // make sure idle conn num is always 1

   // 3. create conn pool
   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // 4. drop user
   rc = db.removeUsr( userName1.c_str(), pwd.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // 5. check conn
   rc = connPool.getConnection( conn1 ) ;  // get old conn
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = conn1->getSessionAttr( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connPool.getConnection( conn2 ) ;  // create conn
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) ;

   // 6. updateAuthInfo
   rc = db.removeUsr( userName2.c_str(), pwd.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connPool.updateAuthInfo( USER, PASSWD ) ;

   // 7. get conn and use
   rc = connPool.getConnection( conn3 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = conn3->getSessionAttr( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // 8. release conn
   connPool.releaseConnection( conn1 ) ;
   connPool.releaseConnection( conn3 ) ;

   // 9. close conn pool
   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   db.disconnect() ;
}

TEST( connectionPool, updateAddrParamTest )
{
   INT32 rc                  = SDB_OK ;
   string host               = HOST ;
   string svcName            = SERVER ;
   string address            = host + ":" + svcName ;
   string invalidAddr        = host + svcName ;
   vector<string> addrList ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   INT32 count ;

   addrList.push_back( invalidAddr ) ;
   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: parameter checking
   rc = connPool.updateAddress( addrList ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 2: repeat address
   addrList.pop_back() ;
   addrList.push_back( address ) ;
   addrList.push_back( address ) ;

   rc = connPool.updateAddress( addrList ) ;
   count = connPool.getNormalAddrNum() ;
   ASSERT_EQ( 1, count ) ;

   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST( connectionPool, updateAddrTest )
{
   INT32 rc                   = SDB_OK ;
   string host                = "localhost" ;
   string svcName1            = CONNPOOL_LOCAL_SERVER1 ;
   string svcName2            = CONNPOOL_LOCAL_SERVER2 ;
   string address1            = host + ":" + svcName1 ;
   string address2            = host + ":" + svcName2 ;
   string invalidAddr         = address1 + "11" ;
   string expAddr1            = "127.0.0.1:" + svcName1 ;
   string expAddr2            = "127.0.0.1:" + svcName2 ;
   vector<string> addrList ;
   vector<string> newAddrList ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   sdb* conn1 ;
   sdb* conn2 ;
   sdb* conn3 ;
   sdb* conn4 ;
   sdb* conn5 ;
   sdb* conn6 ;

   // 1. create conn pool
   addrList.push_back( address1 ) ;
   addrList.push_back( invalidAddr ) ;
   conf.setConnectStrategy( SDB_CONN_STY_LOCAL ) ;
   conf.setConnCntInfo( 4, 4, 4, 10 ) ;
   conf.setSyncCoordInterval( 0 ) ;

   rc = connPool.init( addrList, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, connPool.getAbnormalAddrNum() ) ;
   rc = connPool.getConnection( conn1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr1, conn1->getAddress() ) ;

   // 2. update address
   newAddrList.push_back( address2 ) ;
   rc = connPool.updateAddress( newAddrList ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // 3. check addr list
   ASSERT_EQ( 0, connPool.getAbnormalAddrNum() ) ;
   ASSERT_EQ( 1, connPool.getLocalAddrNum() ) ;
   ASSERT_EQ( 1, connPool.getNormalAddrNum() ) ;

   // 4. check idle pool
   rc = connPool.getConnection( conn2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, conn2->getAddress() ) ;


   rc = connPool.getConnection( conn3 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, conn3->getAddress() ) ;

   // 5. check release
   connPool.releaseConnection( conn1 ) ;

   rc = connPool.getConnection( conn4 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, conn4->getAddress() ) ;

   rc = connPool.getConnection( conn5 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, conn5->getAddress() ) ;

   rc = connPool.getConnection( conn6 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, conn6->getAddress() ) ;

   rc = connPool.getConnection( conn1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, conn1->getAddress() ) ;

   // 6. clear
   connPool.releaseConnection( conn1 ) ;
   connPool.releaseConnection( conn2 ) ;
   connPool.releaseConnection( conn3 ) ;
   connPool.releaseConnection( conn4 ) ;
   connPool.releaseConnection( conn5 ) ;
   connPool.releaseConnection( conn6 ) ;
   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connPool.updateAddress( newAddrList ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_CLOSE, rc ) ;
}

TEST( connectionPool, noInitTest )
{
   INT32 rc                  = SDB_OK ;
   string host               = HOST ;
   string svcName            = SERVER ;
   string address            = host + ":" + svcName ;
   vector<string> addrList ;
   sdbConnectionPool connPool ;
   sdb* conn ;

   addrList.push_back( address ) ;

   rc = connPool.getConnection( conn ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_NOT_INIT, rc ) ;

   connPool.releaseConnection( conn ) ;

   rc = connPool.updateAddress( addrList ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_NOT_INIT, rc ) ;

   rc = connPool.close() ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_NOT_INIT, rc ) ;
}

TEST( connectionPool, closedTest )
{
   INT32 rc                  = SDB_OK ;
   string host               = HOST ;
   string svcName            = SERVER ;
   string address            = host + ":" + svcName ;
   vector<string> addrList ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   sdb conn ;
   sdb* conn1 ;
   sdb* conn2 ;

   addrList.push_back( address ) ;
   rc = conn.connect( host.c_str(), svcName.c_str(), USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;

   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connPool.getConnection( conn1 ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_CLOSE, rc ) ;

   conn2 = &conn ;
   connPool.releaseConnection( conn2 ) ;
   ASSERT_EQ( &conn, conn2 ) ;
   connPool.releaseConnection( conn2 ) ;

   rc = connPool.updateAddress( addrList ) ;
   ASSERT_EQ( SDB_CLIENT_CONNPOOL_CLOSE, rc ) ;

   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST( connectionPool, releaseConnectionTest )
{
   INT32 rc                  = SDB_OK ;
   string host               = HOST ;
   string svcName            = SERVER ;
   string address            = host + ":" + svcName ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   sdb conn ;
   sdb* conn1 ;
   sdb* conn2 ;

   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1
   rc = connPool.getConnection( conn1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   connPool.releaseConnection( conn1 ) ;
   ASSERT_EQ( NULL, conn1 ) ;

   // case 2
   rc = conn.connect( host.c_str(), svcName.c_str(), USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   conn2 = &conn ;
   connPool.releaseConnection( conn2 ) ;
   ASSERT_EQ( &conn, conn2 ) ;

   rc = connPool.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

