/**************************************************************
 * @Description: use sdbCursor without init
 *               seqDB-14033:sdbCursor对象未创建时调用方法
 * @Modify     : Liang xuewang
 *               2018-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class sdbCursor14033 : public testBase
{
protected:
   sdbCursor cursor ;
   void SetUp()
   {
   }
   void TearDown()
   {
   }
} ;

TEST_F( sdbCursor14033, opCursor )
{
   // test all function of sdbCursor
   
   INT32 rc = SDB_OK ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get next shouldn't succeed" ;
   rc = cursor.current( obj ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get current shouldn't succeed" ;
   rc = cursor.close() ;
   EXPECT_EQ( SDB_OK, rc ) << "close should be ok" ;
}
