/**************************************************************************
 * @Description:   test case for CPP driver
 *                 seqDB-15861:timestamp的increase边界值测试      
 * @Modify:        wenjing wang Init
 *                 2018-04-26
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace bson ;
class timestampTest:public testBase
{
protected:
   void SetUp()
   {
   }
   void TearDown()
   {
   }
};

int getTSFromConObj( long long time, unsigned int inc )
{
   BSONObjBuilder builder ;
   builder.appendTimestamp("ts", time, inc);
   BSONObj obj = builder.obj() ;
   std::cout << obj.toString() << std::endl ;
   return obj.getField("ts").timestampTime() ;
}

TEST_F( timestampTest, inRange   )
{
   int ms = getTSFromConObj( 1000, 0 ) ;
   
   ASSERT_EQ( ms, 1000 ) << "The microseconds is not expected" ;
   
   ms = getTSFromConObj( 1000, 100 ) ;
   
   ASSERT_EQ( ms, 1000 ) << "The microseconds is not expected" ;
   
   ms = getTSFromConObj( 1000,999999 ) ;
   
   ASSERT_EQ( ms, 1000 ) << "The microseconds is not expected" ;
}

TEST_F( timestampTest, outRange   )
{
   int ms = getTSFromConObj( 1000, 1000000 ) ;
   ASSERT_EQ( ms, 2000 ) << "The microseconds is not expected" ;
}
