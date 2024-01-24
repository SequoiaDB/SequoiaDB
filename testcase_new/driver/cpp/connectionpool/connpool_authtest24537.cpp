/***********************************************************************
 * @Description: testcase for connectionPool
 * seqDB-24537:更新密码文件鉴权
 * seqDB-24538:更新密码鉴权
 * seqDB-24539:更新密码文件鉴权，提供不匹配的用户
 * seqDB-24540:更新密码文件鉴权，提供错误的密码文件
 * seqDB-24541:更新密码文件，提供不正确的token
 * seqDB-24542:更新密码鉴权，用户名与密码不匹配
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

class connPoolAuthTest24537 : public testBase
{
protected:
   sdbConnectionPool pool ;
   sdbConnectionPoolConf conf ;
   string url ;
   sdb* conn = NULL ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      url = ARGS->coordUrl() ;
      BOOLEAN ret = FALSE ;
	  
      conf.setConnCntInfo(1,1,1,10);
      rc = pool.init( url, conf ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

      rc = pool.getConnection( conn ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
	  
      if ( conn != NULL )
      {
         ret = conn->isValid() ;
         ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
		  
         rc = conn->createUsr("sdbadmin","sequoiadb") ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create user" ;
      }
   }
   
   void TearDown()
   {
      if ( conn != NULL )
      {
         conn->removeUsr("sdbadmin", "sequoiadb");
         pool.releaseConnection( conn ) ;
      }
      pool.close() ;
   }
} ;

TEST_F( connPoolAuthTest24537, authSuccess24537 )
{
   INT32 rc = SDB_OK ;
   BOOLEAN ret = FALSE ;
   if ( conn != NULL )
   {
      pool.releaseConnection( conn ) ;
      conn = NULL ;
   }
   
   pool.updateAuthInfo("sdbadmin", "/sdbadmin/sequoiadb/passwd", "") ;
   
   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
}

TEST_F( connPoolAuthTest24537, authSuccesUpdatePassword24538 )
{
   INT32 rc = SDB_OK ;
   BOOLEAN ret = FALSE ;
   if ( conn != NULL )
   {
      pool.releaseConnection( conn ) ;
      conn = NULL ;
   }
   
   pool.updateAuthInfo("sdbadmin", "sequoiadb");
   
   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
}

TEST_F( connPoolAuthTest24537, authSuccessWithToken24439 )
{
   INT32 rc = SDB_OK ;
   BOOLEAN ret = FALSE ;
   if ( conn != NULL )
   {
      pool.releaseConnection( conn ) ;
      conn = NULL ;
   }
   
   pool.updateAuthInfo("sdbadmin", "/sdbadmin/sequoiadb/passwd", "sequoiadb");
   
   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
}

TEST_F( connPoolAuthTest24537, authFailureWithMismatchedUser24539 )
{
   INT32 rc = SDB_OK ;
   sdb* con = NULL ;
   BOOLEAN ret = FALSE;

   pool.updateAuthInfo("sequoiadb", "/home/sdbadmin/sequoiadb/passwd", "") ;
   
   rc = pool.getConnection( con ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to get connection from connectionpool" ;
   
   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
}

TEST_F( connPoolAuthTest24537, authFailureWithErrToken24541 )
{
   INT32 rc = SDB_OK ;
   sdb* con = NULL ;
   BOOLEAN ret = FALSE;
  
   pool.updateAuthInfo("sdbadmin", "/home/sdbadmin/sequoiadb/passwd", "sdbadmin") ;

   rc = pool.getConnection( con ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to get connection from connectionpool" ;
   
   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;

}

TEST_F( connPoolAuthTest24537, authFailureWithErrCipherFile24540 )
{
   INT32 rc = SDB_OK ;
   sdb* con = NULL ;
   BOOLEAN ret = FALSE;

   pool.updateAuthInfo("sdbadmin", "/sdbadmin/sequoiadb/passwd", "sdbadmin") ;

   rc = pool.getConnection( con ) ;
   ASSERT_EQ( SDB_FNE, rc ) << "fail to get connection from connectionpool" ;
   
   if ( conn != NULL )
   {
      ret = conn->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;

}

TEST_F( connPoolAuthTest24537, authFailureNotMatchUsr24542 )
{
   INT32 rc = SDB_OK ;
   sdb* con = NULL ;
   BOOLEAN ret = FALSE;

   pool.updateAuthInfo("sdbadmin","sdbadmin") ;

   vector<sdb*> conns ;
   for ( int i =0; i < 20; ++i )
   {
      sdb* con = NULL ; 
      rc = pool.getConnection( con ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   
      if ( con != NULL )
      {
         ret = con->isValid() ;
      }

      conns.push_back( con );
   
      ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
   }
}
                                          
