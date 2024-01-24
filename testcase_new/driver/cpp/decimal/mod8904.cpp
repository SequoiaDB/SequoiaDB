/**************************************************************
 * @Description: test decimal
 *               seqDB-8904 : 接口bsonDecimal::mod( bsonDecimal &right, bsonDecimal &result )测试 
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

TEST( decimal8904, mod )
{   
   INT32 rc = SDB_OK;
   INT32 left = 99;
   INT32 right = 5; 

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromInt( left );

   bsonDecimal rightDec;     
   rightDec.fromInt( right );

   //mod, eg: c = a mod b
   bsonDecimal resultDec;
   resultDec.init();
   rc = leftDec.mod( rightDec , resultDec ); // seqDB-8904
   ASSERT_EQ( SDB_OK, rc );

   //check
   string resultStr;
   resultStr = resultDec.toString();
   ASSERT_STREQ( "4", resultStr.c_str() );          
}
