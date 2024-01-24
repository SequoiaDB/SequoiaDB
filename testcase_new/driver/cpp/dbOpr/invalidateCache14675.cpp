/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14675:invalidateCache清除节点缓存
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

class invalidateCacheTest14675 : public testBase
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

TEST_F( invalidateCacheTest14675, invalidateCache )
{
   INT32 rc = SDB_OK ;

   rc = db.invalidateCache() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to invalidateCache" ;
}
