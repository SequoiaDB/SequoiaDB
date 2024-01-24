/**************************************************************
 * @Description: test decimal
 *               seqDB-8890 : 接口bsonDecimal::fromString( const CHAR *value )测试
 *               seqDB-8899 : 接口bsonDecimal::div( const bsonDecimal &right, bsonDecimal &result )测试
 *               seqDB-8900 : 接口bsonDecimal::div( INT64 right, bsonDecimal &result )测试 
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

TEST( decimal8899, div )
{   
   INT32 rc = SDB_OK;
   char s[] = "100";
   INT64 l = 5; 

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromString( s ); // seqDB-8890

   bsonDecimal rightDec;     
   rightDec.fromLong( l );

   //div, eg: c = a / b
   bsonDecimal resultDec1;
   resultDec1.init();
   rc = leftDec.div( rightDec , resultDec1 ); // seqDB-8899
   ASSERT_EQ( SDB_OK, rc );

   //check
   string resultStr1;
   resultStr1 = resultDec1.toString();
   cout << "str = " << resultStr1 << endl;
   ASSERT_STREQ( "20.0000000000000000", resultStr1.c_str() );   

   //div, eg: c = a / b
   bsonDecimal resultDec2;
   resultDec2.init();
   rc = leftDec.div( l , resultDec2 ); // seqDB-8900
   ASSERT_EQ( SDB_OK, rc );

   //check
   string resultStr2;
   resultStr2 = resultDec2.toString();
   cout << "str = " << resultStr2 << endl;
   ASSERT_STREQ( "20.0000000000000000", resultStr2.c_str() );    
}
