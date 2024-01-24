/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12688:创建/获取/启动/停止/激活/枚举/删除数据分区组
 *                 seqDB-12693:获取不存在的数据组
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

class rgOprTest12688 : public testBase
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

TEST_F( rgOprTest12688, rgOpr12688 )
{
   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   const CHAR* rgName = "rgOprTestGroup12688" ;
   sdbReplicaGroup rg ;

   // create rg
   rc = db.createReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create rg " << rgName ;

   // get rg
   rc = db.getReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get rg " << rgName ;

   // list rg
   sdbCursor cursor ;
   BSONObj obj ;
   BOOLEAN found = FALSE ;
   rc = db.listReplicaGroups( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list rg" ;
   while( !cursor.next( obj ) )
   {
      if( obj.getField( "GroupName" ).String() == rgName )
      {
         found = TRUE ;
         break ;
      }
   }
   ASSERT_TRUE( found ) << "fail to check list rg " << rgName ;

   // check rgName isCatalog
   ASSERT_STREQ( rgName, rg.getName() ) << "fail to check rgName" ;
   ASSERT_EQ( FALSE, rg.isCatalog() ) << "fail to check isCatalog" ;

   // create node
   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName = ARGS->rsrvPortBegin() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath[100] ;
   sprintf( dbPath, "%s%s%s%s", nodeDir, "data/", svcName, "/" ) ;
   cout << "node: " << hostName << " " << svcName << " " << dbPath << endl ;
   rc = rg.createNode( hostName, svcName, dbPath ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node" ;
   
   // start rg
   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg " << rgName ;
   
   // connect node to check start 
   sdb db1 ;
   rc = db1.connect( hostName, svcName, ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect node" ;
   db1.disconnect() ;

   // stop rg
   rc = rg.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop rg " << rgName ;

   // connect node to check stop
   rc = db1.connect( hostName, svcName, ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_NET_CANNOT_CONNECT, rc ) << "fail to check stop rg " << rgName ; 

   // activate rg
   rc = db.activateReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to activate rg " << rgName ;

   // connect node to check activate
   rc = db1.connect( hostName, svcName, ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect node" ;
   db1.disconnect() ; 
   
   // remove rg
   rc = db.removeReplicaGroup( rgName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove rg " << rgName ;
   rc = db.getReplicaGroup( rgName, rg ) ; 
   ASSERT_EQ( SDB_CLS_GRP_NOT_EXIST, rc ) << "fail to test remove rg" ;
}

TEST_F( rgOprTest12688, notExistRg12693 )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   const CHAR* rgName = "notExistRg12693" ;
   sdbReplicaGroup rg ;
   rc = db.getReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_CLS_GRP_NOT_EXIST, rc ) << "fail to test get not exist rg " << rgName ;
}
