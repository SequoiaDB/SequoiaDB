/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12690:获取数据组的详细信息/主节点/从节点
 *                 SEQUOIADBMAINSTREAM-2871
 * @Modify:        Liang xuewang Init
 *                 2017-11-28
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

class masterSlaveTest12690 : public testBase
{
protected:
   const CHAR* rgName ;
   sdbReplicaGroup rg ;

   void SetUp()
   {
      testBase::SetUp() ;
   }

   void TearDown()
   {
      testBase::TearDown() ;
   }

   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "masterSlaveTestRg12690" ; 
      rc = db.createReplicaGroup( rgName, rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to create rg %s", rgName ) ;
   done:
      return rc ;
   error:
      goto done ; 
   }
   INT32 fini() 
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.removeReplicaGroup( rgName ) ;
         CHECK_RC( SDB_OK, rc, "fail to remove rg %s", rgName ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }  
} ;

TEST_F( masterSlaveTest12690, masterSlave12690 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   
   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getMaster getSlave in empty group
   sdbNode master, slave ;
   rc = rg.getMaster( master ) ;
   ASSERT_EQ( SDB_CLS_EMPTY_GROUP, rc ) << "fail to test getMaster in empty group" ;
   rc = rg.getSlave( slave ) ;
   ASSERT_EQ( SDB_CLS_EMPTY_GROUP, rc ) << "fail to test getSlave in empty group" ;

   // create two node in rg and start
   CHAR hostName[ MAX_NAME_SIZE+1 ] ;
   memset( hostName, 0, sizeof(hostName) ) ;
   rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* svcName1 = ARGS->rsrvPortBegin() ;
   const CHAR* svcName2 = ARGS->rsrvPortEnd() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR dbPath1[100] ;
   sprintf( dbPath1, "%s%s%s%s", nodeDir, "data/", svcName1, "/" ) ;
   CHAR dbPath2[100] ;
   sprintf( dbPath2, "%s%s%s%s", nodeDir, "data/", svcName2, "/" ) ;
   cout << "node1: " << hostName << " " << svcName1 << " " << dbPath1 << endl ;
   cout << "node2: " << hostName << " " << svcName2 << " " << dbPath2 << endl ;

   rc = rg.createNode( hostName, svcName1, dbPath1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node1" ;
   rc = rg.createNode( hostName, svcName2, dbPath2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node2" ;
   rc = rg.start() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg " << rgName ;

   // get master node and check
   rc = rg.getMaster( master ) ;
   while( SDB_RTN_NO_PRIMARY_FOUND == rc )
   {
      ossSleep( 1000 ) ;
      rc = rg.getMaster( master ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   sdb db1 ;
   rc = master.connect( db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect master" ;
   sdbCursor cursor ;
   rc = db1.getSnapshot( cursor, SDB_SNAP_DATABASE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot db" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_TRUE( obj.getField( "IsPrimary" ).Bool() ) << "fail to check master" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   
   // get slave node and check
   rc = rg.getSlave( slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get slave" ;
   rc = slave.connect( db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect slave" ;
   rc = db1.getSnapshot( cursor, SDB_SNAP_DATABASE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot db" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_FALSE( obj.getField( "IsPrimary" ).Bool() ) << "fail to check slave" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
    
   // get detail and check
   BSONObj detail ;
   rc = rg.getDetail( detail ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get detail" ;
   ASSERT_EQ( rgName, detail.getField( "GroupName" ).String() ) << "fail to check detail" ;  

   // stop slave, master will step down to slave, group will have no master
   // then getMaster getSlave
   rc = slave.stop() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop slave node" ;
   rc = rg.getMaster( master ) ;
   while( SDB_OK == rc )
   {
      ossSleep( 1000 ) ;
      rc = rg.getMaster( master ) ;
   }
   ASSERT_EQ( SDB_RTN_NO_PRIMARY_FOUND, rc ) << "fail to test getMaster" ;
   rc = rg.getSlave( slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test getSlave" ; 

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

