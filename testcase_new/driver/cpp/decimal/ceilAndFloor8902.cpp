/**************************************************************
 * @Description: test decimal
 *               seqDB-8902 : 接口bsonDecimal::ceil( bsonDecimal &result )测试 
 *               seqDB-8903 : 接口bsonDecimal::floor( bsonDecimal &result )测试 
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

TEST( decimal8902, ceilAndfloor )
{   
   INT32 rc = SDB_OK;
   FLOAT64 left = 9.5;

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromDouble( left );  

   //ceil
   bsonDecimal resultDec1;
   rc = leftDec.ceil( resultDec1 ); // seqDB-8902
   ASSERT_EQ( SDB_OK, rc );

   //check
   string resultStr1;
   resultStr1 = resultDec1.toString();
   ASSERT_STREQ( "10", resultStr1.c_str() );

   //floor
   bsonDecimal resultDec2;
   rc = leftDec.floor( resultDec2 ); // seqDB-8903
   ASSERT_EQ( SDB_OK, rc );

   //check
   string resultStr2;
   resultStr2 = resultDec2.toString();
   ASSERT_STREQ( "9", resultStr2.c_str() );   
}
