/**************************************************************
 * @Description: test decimal
 *               seqDB-8885 : 接口bsonDecimal::toLong( INT64 *value ) const测试 
 *               seqDB-8888 : 接口bsonDecimal::fromLong( INT64 value )测试
 *               seqDB-8897 : 接口bsonDecimal::sub( const bsonDecimal &right, bsonDecimal &result )测试 
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

TEST( decimal8897, sub )
{   
   INT32 rc = SDB_OK;
   INT64 l1 = 9223372036854775806;
   INT64 l2 = -1;   

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromLong( l1 ); // seqDB-8888

   bsonDecimal rightDec;     
   rightDec.fromInt( l2 );

   //sub, eg: c = a - b
   bsonDecimal resultDec;
   resultDec.init();
   rc = leftDec.sub( rightDec , resultDec ); // seqDB-8897
   ASSERT_EQ( SDB_OK, rc );

   //check
   INT64 resultLong;
   rc = resultDec.toLong( &resultLong ); // seqDB-8885
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( l1-l2, resultLong );   
}
