/***********************************************************************
 * @Description: testcase for connectionPool
 * seqDB-24543:设置为密码鉴权，更新为密码文件鉴权
 * seqDB-24544:设置为密码文件鉴权，更新为密码鉴权
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

class connPoolAuthTest24543 : public testBase
{
protected:
   sdbConnectionPool pool ;
   sdbConnectionPoolConf conf ;
   string url ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;
	  
      url = ARGS->coordUrl() ;
      rc = db.createUsr("sdbadmin","sequoiadb") ;
      ASSERT_EQ( SDB_OK, rc ) << "conn is invalid" ;
   }

   void TearDown()
   {
      db.removeUsr("sdbadmin", "sequoiadb");
      pool.close() ;
   }
} ;

TEST_F( connPoolAuthTest24543, authSuccessFromCipherFileToPasswd24544 )
{
   BOOLEAN ret = FALSE ;
   INT32 rc = SDB_OK ;
   sdb* conn1 = NULL ;
   sdb* conn2 = NULL ;
   conf.setAuthInfo("sdbadmin", "/home/sdbadmin/sequoiadb/passwd", "");
	
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   if ( conn1 != NULL )
   {
      ret = conn1->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn1 is invalid" ;
   pool.releaseConnection( conn1 ) ;
	
   pool.updateAuthInfo("sdbadmin", "sequoiadb") ;
   
   rc = pool.getConnection( conn2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn2 != NULL )
   {
      ret = conn2->isValid() ;
   }

   ASSERT_EQ( TRUE, ret ) << "conn2 is invalid" ;

   if ( conn1 != NULL )
   {
      ret = conn1->isValid() ;
   }
   ASSERT_EQ( FALSE, ret ) << "conn1 is valid" ;
}

TEST_F( connPoolAuthTest24543, authSuccessFromPasswdToCipherFile24543 )
{
   BOOLEAN ret = FALSE ;
   INT32 rc = SDB_OK ;
   sdb* conn1 = NULL ;
   sdb* conn2 = NULL ;
   conf.setAuthInfo("sdbadmin", "sequoiadb");
	
   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	
   rc = pool.getConnection( conn1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
   if ( conn1 != NULL )
   {
      ret = conn1->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
   pool.releaseConnection( conn1 ) ;

   pool.updateAuthInfo("sdbadmin", "/home/sdbadmin/sequoiadb/passwd", "") ;
   
   rc = pool.getConnection( conn2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn2 != NULL )
   {
      ret = conn2->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;

   if ( conn1 != NULL )
   {
      ret = conn1->isValid() ;
   }
   ASSERT_EQ( FALSE, ret ) << "conn1 is valid" ;
}
