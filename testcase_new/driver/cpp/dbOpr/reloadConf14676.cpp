/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14676:reloadConf重新加载节点配置
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

class reloadConfTest14676 : public testBase
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

TEST_F( reloadConfTest14676, reloadConf )
{
   INT32 rc = SDB_OK ;

   rc = db.reloadConfig() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to reloadConf" ;
}
