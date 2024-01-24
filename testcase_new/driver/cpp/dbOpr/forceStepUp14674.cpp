/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14674:forceStepUp强制升主
 *               forceStepUp 只用于无主的编目组
 *               且当前节点的LSN为组内最大值才能升主
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-12
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class forceStepUpTest14674 : public testBase
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

TEST_F( forceStepUpTest14674, forceStepUp )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   sdbReplicaGroup rg ;
   sdbNode slave ;
   const CHAR* rgName = "SYSCatalogGroup" ;
   vector<string> nodes ;
   rc = getGroupNodes( db, rgName, nodes ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = db.getReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get rg " << rgName ;
   rc = rg.getSlave( slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get slave" ;
   cout << slave.getNodeName() << endl ;

   sdb cata ;
   rc = slave.connect( cata ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect slave" ;
   rc = cata.forceStepUp() ;  // catalog group should have master, so forceStepUp will fail
   ASSERT_EQ( SDB_CLS_CAN_NOT_STEP_UP, rc ) << "fail to test forceStepUp" ;
} 
