/**************************************************************************
 * @Description :   test getSlave1 operation
 *                  seqDB-13780:getSlave1参数校验
 *                  SEQUOIADBMAINSTREAM-2981
 * @Modify      :   Liang xuewang
 *                  2017-12-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace std ;

class getSlaveParaCheck13780 : public testBase
{
protected:
   const CHAR* rgName ;
   sdbReplicaGroupHandle rg ;
      
   void SetUp()
   {
      testBase::SetUp() ;
   }  
   void TearDown()
   {
      testBase::TearDown() ;
   }
   
   // create rg
   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "getSlaveTestRg13780" ;
      rc = sdbCreateReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to create rg %s, rc = %d", rgName, rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // remove rg
   INT32 release()
   {
      INT32 rc = SDB_OK ;
      rc = sdbRemoveReplicaGroup( db, rgName ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove rg %s, rc = %d", rgName, rc ) ;
      sdbReleaseReplicaGroup( rg ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( getSlaveParaCheck13780, illegalRg )
{
   INT32 rc = SDB_OK ;

   sdbReplicaGroupHandle rg1 = SDB_INVALID_HANDLE ;
   
   INT32 positionArr[7] = { 1, 2, 3, 4, 5, 6, 7 } ;
   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg1, positionArr, 7, &slave ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 with illegalRg" ;
   sdbReleaseNode( slave ) ;
}

TEST_F( getSlaveParaCheck13780, illegalArr )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg, NULL, 1, &slave ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 with positionArr NULL" ;

   INT32 positionArr1[4] = { 1, 4, 9, 5 } ; 
   rc = sdbGetNodeSlave1( rg, positionArr1, 4, &slave ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 with positionArr has >7" ;
   
   INT32 positionArr2[4] = { 0, 4, 6, 5 } ;
   rc = sdbGetNodeSlave1( rg, positionArr2, 4, &slave ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 with positionArr has <1" ;

   sdbReleaseNode( slave ) ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( getSlaveParaCheck13780, illegalPositionCount )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   INT32 positionArr[7] = { 1, 2, 3, 4, 5, 6, 7 } ;
   sdbNodeHandle slave ;
   rc = sdbGetNodeSlave1( rg, positionArr, 0, &slave ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 with positionCount <=0" ;

   rc = sdbGetNodeSlave1( rg, positionArr, 8, &slave ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 wit positionCount >7" ;
   
   sdbReleaseNode( slave ) ;
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( getSlaveParaCheck13780, illegalNode )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   INT32 positionArr[7] = { 1, 2, 3, 4, 5, 6, 7 } ;
   rc = sdbGetNodeSlave1( rg, positionArr, 7, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave1 with illegalNode" ;
   
   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
