/**************************************************************
 * @Description: test decimal
 *               seqDB-8877 : 接口bsonDecimal::setZero()测试 
 *               seqDB-8878 : 接口bsonDecimal::isZero()测试 
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <sstream>
#include "client.hpp"
#include "testcommon.hpp"

using namespace std;
using namespace sdbclient;
using namespace bson; 

TEST( decimal8877, zero )
{   
   INT32 rc = SDB_OK;
   FLOAT64 left = 9.5;

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromDouble( left );  

   //set zero
   leftDec.setZero(); // seqDB-8877
   string resultStr;
   resultStr = leftDec.toString();
   ASSERT_STREQ( "0", resultStr.c_str() );  

   //is zero?
   BOOLEAN isZero = leftDec.isZero(); // seqDB-8878
   ASSERT_TRUE( isZero );   
}
