/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14677:setPDLevel设置节点日志级别
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

class setPDLevelTest14677 : public testBase
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

TEST_F( setPDLevelTest14677, setPDLevel )
{
   INT32 rc = SDB_OK ;

   rc = db.setPDLevel( 5 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setPDLevel" ;
}
