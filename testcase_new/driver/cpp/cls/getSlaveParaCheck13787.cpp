/**************************************************************************
 * @Description :   test getSlave operation
 *                  seqDB-13787:getSlave参数校验
 *                  SEQUOIADBMAINSTREAM-2981
 * @Modify      :   Liang xuewang
 *                  2017-12-20
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace std ;
using namespace bson ;
using namespace sdbclient ;

class getSlaveParaCheck13787 : public testBase
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
   
   // create rg
   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "getSlaveTestRg13787" ;
      rc = db.createReplicaGroup( rgName, rg ) ;
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
      rc = db.removeReplicaGroup( rgName ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove rg %s, rc = %d", rgName, rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( getSlaveParaCheck13787, illegalRg )
{
   INT32 rc = SDB_OK ;

   sdbReplicaGroup rg1 ;
   
   vector<INT32> positions ;
   for( INT32 i = 1;i <= 7;i++ )
   {
      positions.push_back( i ) ;
   }
   sdbNode slave ;
   rc = rg1.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) << "fail to test getSlave with illegalRg" ;
}

TEST_F( getSlaveParaCheck13787, illegalPosition )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   vector<INT32> positions ;
   positions.push_back( 8 ) ;
   sdbNode slave ;
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave with positions has >7" ;
  
   positions.clear() ;
   positions.push_back( 0 ) ; 
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test getSlave with positions has <1" ;

   positions.clear() ;
   positions.push_back( 1 ) ;
   rc = rg.getSlave( slave, positions ) ;
   ASSERT_EQ( SDB_CLS_EMPTY_GROUP, rc ) << "fail to test getSlave with empty group" ;

   rc = release() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
