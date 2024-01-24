/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14679:msg向节点发送消息
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

class msgTest14679 : public testBase
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

TEST_F( msgTest14679, msg )
{
   INT32 rc = SDB_OK ;

   const CHAR* message = "Hello, SequoiaDB" ;
   rc = db.msg( message ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to msg" ;
}
