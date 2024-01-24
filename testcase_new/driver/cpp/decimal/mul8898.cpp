/**************************************************************
 * @Description: test decimal
 *               seqDB-8884 : 接口bsonDecimal::toDouble( FLOAT64 *value ) const测试 
 *               seqDB-8898 : 接口bsonDecimal::mul( const bsonDecimal &right, bsonDecimal &result )测试 
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

TEST( decimal8898, mul )
{   
   INT32 rc = SDB_OK;
   FLOAT64 d = 2.5;
   INT32 i = 5; 

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromDouble( d );

   bsonDecimal rightDec;     
   rightDec.fromInt( i );

   //mul, eg: c = a * b
   bsonDecimal resultDec;
   resultDec.init();
   rc = leftDec.mul( rightDec , resultDec ); // seqDB-8898
   ASSERT_EQ( SDB_OK, rc );

   //check
   FLOAT64 resultDouble;
   rc = resultDec.toDouble( &resultDouble ); // seqDB-8884
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( 12.5, resultDouble );   
}
