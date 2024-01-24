/**************************************************************
 * @Description: test decimal
 *               seqDB-8891 : 接口bsonDecimal::getPrecision( INT32 *precision, INT32 *scale ) const测试 
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

TEST( decimal8891, getPrecision ) // seqDB-8891
{   
   INT32 rc = SDB_OK;
   INT32 precision = -1;
   INT32 scale = -1; 

   //decimal
   bsonDecimal dec;
   dec.init( 10, 2 );
   dec.fromDouble( 100.123 );   

   //getPrecision
   rc = dec.getPrecision( &precision, &scale );
   ASSERT_EQ( SDB_OK, rc ); 
   ASSERT_EQ( 10, precision );
   ASSERT_EQ( 2, scale );

   //getPrecision
   precision = -1;
   precision = dec.getPrecision();
   ASSERT_EQ( 10, precision );       
}
