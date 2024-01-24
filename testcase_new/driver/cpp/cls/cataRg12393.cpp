/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12393:创建/获取/删除编目节点组
 *                 手工测试用例，不加入scons脚本
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class cataRgTest12393 : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
   }

   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( cataRgTest12393, cataRg12393 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName = ARGS->rsrvPortBegin() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath[100] ;
   sprintf( dbPath, "%s%s%s%s", nodeDir, "data/", svcName, "/" ) ;
   BSONObj config ;

   // create cata group
   rc = db.createReplicaCataGroup( hostName, svcName, dbPath, config ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cata group" ;
   
   // get cata group
   sdbReplicaGroup rg ;
   rc = db.getReplicaGroup( 1, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cata group" ;

   // check group name, isCatalog
   ASSERT_STREQ( "SYSCatalogGroup", rg.getName() ) << "fail to check cata group name" ;
   ASSERT_TRUE( rg.isCatalog() ) << "fail to check isCatalog" ;

   // remove rg 
   rc = db.removeReplicaGroup( "SYSCatalogGroup" ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove cata group" ; 
}
