/***************************************************************
 * @Description: test case for c++ driver
 *               seqDB-12746:使用flushConfigure刷新配置
 *               手工测试用例,测试前修改节点配置文件
 * @Modify:      Liangxw
 *               2017-09-22
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class flushConfTest12746 : public testBase
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

TEST_F( flushConfTest12746, flushConf12476 )
{
   INT32 rc = SDB_OK ;

   BSONObj option = BSON( "Global" << false ) ;
   rc = db.flushConfigure( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to flush configure" ;
}
