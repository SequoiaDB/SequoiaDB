/**************************************************************
 * @Description: test decimal
 *               seqDB-8879 : 接口bsonDecimal::setMin()测试 
 *               seqDB-8880 : 接口bsonDecimal::isMin()测试 
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

TEST( decimal8879, min )
{   
   INT32 rc = SDB_OK;
   FLOAT64 left = 9.5;

   //construnct decimal
   bsonDecimal leftDec;     
   leftDec.fromDouble( left ); 

   //is min?
   BOOLEAN isMin = leftDec.isMin(); // seqDB-8880
   ASSERT_FALSE( isMin ); 

   //set min
   leftDec.setMin(); // seqDB-8879
   isMin = leftDec.isMin();
   ASSERT_TRUE( isMin );  
}
