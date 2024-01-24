/**************************************************************
 * @Description: test decimal
 *               seqDB-8881 : 接口bsonDecimal::setMax()测试 
 *               seqDB-8882 : 接口bsonDecimal::isMax()测试 
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

TEST( decimal8881, max )
{   
   INT32 rc = SDB_OK;
   FLOAT64 left = 9.5;

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromDouble( left ); 

   //is max?
   BOOLEAN isMax = leftDec.isMax(); // seqDB-8882
   ASSERT_FALSE( isMax ); 

   //set max
   leftDec.setMax(); // seqDB-8881
   isMax = leftDec.isMax();
   ASSERT_TRUE( isMax );  
}
