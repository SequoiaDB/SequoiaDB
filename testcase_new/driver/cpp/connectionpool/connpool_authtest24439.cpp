/***********************************************************************
 * @Description: testcase for connectionPool
 *               seqDB-24439: 通过密码文件鉴权
 *               seqDB-24440: 通过密码鉴权              
 *               seqDB-24441: 密码文件鉴权，提供不匹配的用户
 *               seqDB-24442: 密码文件鉴权，提供错误的密码文件 
 *               seqDB-24443: 密码文件鉴权，提供不正确的token
 *               seqDB-24444: 同时设置密码和密码文件
 * @Modify:      wenjingwang
 *               2021/10/22
 ***********************************************************************/
#include <sdbConnectionPool.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <gtest/gtest.h>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class connPoolAuthTest24439 : public testBase
{
protected:
   sdbConnectionPool pool ;
   sdbConnectionPoolConf conf ;
   string url ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      url = ARGS->coordUrl() ;
   }
   void TearDown()
   {
      pool.close() ;
   }
} ;

TEST_F( connPoolAuthTest24439, authSuccess24439 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   BOOLEAN ret = FALSE ;

   conf.setAuthInfo("sdbadmin", "/home/sdbadmin/sequoiadb/passwd", "sequoiadb");
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   
   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;

   pool.releaseConnection( conn ) ;

   bson::BSONObj obj;
   rc = conn->getSessionAttr(obj, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std:string t = obj.getField("PreferedInstance").toString();
   std::cout << t << std::endl;
}

TEST_F( connPoolAuthTest24439, authSuccessSetPasswordAndCipherFile24444 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   BOOLEAN ret = FALSE ;

   conf.setAuthInfo("sdbadmin", "sequoiadb");
   conf.setAuthInfo("sdbadmin", "/sdbadmin/sequoiadb/passwd", "");
   
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   
   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;

   pool.releaseConnection( conn ) ;
}

TEST_F( connPoolAuthTest24439, authSuccessWithToken24439 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   BOOLEAN ret = FALSE ;

   conf.setAuthInfo("sdbadmin", "/home/sdbadmin/sequoiadb/passwd", "sequoiadb");
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   
   if ( conn != NULL)
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;

   pool.releaseConnection( conn ) ;
}

TEST_F( connPoolAuthTest24439, authFailureWithMismatchedUser24441 )
{
   INT32 rc = SDB_OK ;
   BOOLEAN ret = FALSE ;
   sdb* conn = NULL ;

   conf.setAuthInfo("sequoiadb", "/home/sdbadmin/sequoiadb/passwd", "sequoiadb");
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to get connection from connectionpool" ;
   
}

TEST_F( connPoolAuthTest24439, authFailureWithErrToken24443 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;

   conf.setAuthInfo("sdbadmin", "/home/sdbadmin/sequoiadb/passwd", "sdbadmin");
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to get connection from connectionpool" ;

}

TEST_F( connPoolAuthTest24439, authFailureWithErrCipherFile24442 )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;

   conf.setAuthInfo("sequoiadb", "/sdbadmin/sequoiadb/passwd", "sdbadmin");
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_FNE, rc ) << "fail to get connection from connectionpool" ;

}
