/**************************************************************
 * @Description: test decimal
 *               seqDB-8883 : 接口bsonDecimal::toInt( INT32 *value ) const测试 
 *               seqDB-8895 : 接口bsonDecimal::add( const bsonDecimal &right, bsonDecimal &result )测试 
 *               seqDB-8896 : 接口bsonDecimal::add( const bsonDecimal &right )测试 
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

TEST( decimal8895, add )
{   
   INT32 rc = SDB_OK;
   INT32 i1 = 4;
   INT32 i2 = 104;   

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromInt( i1 );

   bsonDecimal rightDec;     
   rightDec.fromInt( i2 );

   //add, eg: c = a + b
   bsonDecimal resultDec;
   resultDec.init();
   rc = leftDec.add( rightDec , resultDec ); // seqDB-8895
   ASSERT_EQ( SDB_OK, rc );

   //check
   INT32 resultInt;
   rc = resultDec.toInt( &resultInt ); // seqDB-8883
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( i1+i2, resultInt );

   //add, eg: a += b
   rc = leftDec.add( rightDec ); // seqDB-8896
   ASSERT_EQ( SDB_OK, rc );

   //check
   rc = leftDec.toInt( &resultInt );
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( i1+i2, resultInt );
}
