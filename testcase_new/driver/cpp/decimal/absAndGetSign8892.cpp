/**************************************************************
 * @Description: test decimal
 *               seqDB-8892 : 接口bsonDecimal::getSign() const测试 
 *               seqDB-8901 : 接口bsonDecimal::abs()测试 
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

TEST( decimal8892, absAndgetSign )
{   
   INT32 rc = SDB_OK;
   INT32 left = -1;

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromInt( left );  

   //get sign
   INT16 sign = 0;
   sign = leftDec.getSign(); // seqDB-8892
   cout << "sign = " << sign << endl;
   ASSERT_NE( 0, sign );

   //abs
   INT32 absResult;
   rc = leftDec.abs(); // seqDB-8901
   ASSERT_EQ( SDB_OK, rc );

   //check
   string resultStr;
   resultStr = leftDec.toString();
   ASSERT_STREQ( "1", resultStr.c_str() );  
}
