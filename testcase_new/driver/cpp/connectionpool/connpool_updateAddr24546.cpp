/***********************************************************************
 * @Description: testcase for connectionPool
 * seqDB-24546:更新连接池地址，新地址不包含原地址
 * seqDB-24547:更新连接池地址，新地址包含原有地址
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

class connPoolUpdateAddr24546 : public testBase
{
protected:
   sdbConnectionPool pool ;
   sdbConnectionPoolConf conf ;
   string url ;
   sdb* conn1 = NULL ;
   std::vector<string> addrs ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      url = ARGS->coordUrl() ;
      BOOLEAN ret = FALSE ;
	  
      rc = pool.init( url, conf ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	  
      rc = pool.getConnection( conn1 ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;
	  
      if ( conn1 != NULL )
      {
         ret = conn1->isValid() ;
      }
      ASSERT_EQ( TRUE, ret ) << "conn1 is invalid" ;

   }
   void TearDown()
   {
      pool.releaseConnection( conn1 ) ;
      pool.close() ;
   }
} ;

TEST_F( connPoolUpdateAddr24546, containOldAddr24546 )
{
   INT32 rc = SDB_OK ;
   BOOLEAN ret = FALSE ;
   sdb* conn2;
   
   addrs.push_back("localhost:50000") ;

   rc = pool.updateAddress(addrs) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update address" ;
   
   rc = pool.getConnection( conn2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn2 != NULL )
   {
      ret = conn2->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn2 is invalid" ;
   std::cout << conn2->getAddress() << std::endl;
   
   pool.releaseConnection(conn1);
   if ( conn1 != NULL )
   {
      ret = conn1->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn1 is invalid" ;
}

TEST_F( connPoolUpdateAddr24546, notContainOldAddr24547 )
{
   INT32 rc = SDB_OK ;
   BOOLEAN ret = FALSE ;
   sdb* conn2;

   addrs.push_back("r520-10:11810") ;
   rc = pool.updateAddress(addrs) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update AuthInfo" ;
   
   rc = pool.getConnection( conn2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection from connectionpool" ;

   if ( conn2 != NULL )
   {
      ret = conn2->isValid() ;
   }
   ASSERT_EQ( TRUE, ret ) << "conn is invalid" ;
   std::cout << conn2->getAddress() << std::endl;
   
   pool.releaseConnection(conn1);
}


                                          
