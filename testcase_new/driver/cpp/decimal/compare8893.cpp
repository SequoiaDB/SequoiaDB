/**************************************************************
 * @Description: test decimal
 *               seqDB-8873 : 接口bsonDecimal::bsonDecimal& operator= ( const bsonDecimal &right )测试
 *               seqDB-8893 : 接口bsonDecimal::compare( const bsonDecimal &right ) const测试
 *               seqDB-8894 : 接口bsonDecimal::compare( int right ) const测试 
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

TEST( decimal8893, compare ) 
{   
   INT32 rc = SDB_OK;
   int result; 

   //decimal
   bsonDecimal dec1;
   dec1.fromInt( 104 );   
   bsonDecimal dec2 = dec1; // seqDB-8873

   //compare
   result = dec1.compare( dec2 ); // seqDB-8893
   ASSERT_EQ( 0, result );  

   //compare
   result = dec2.compare( 103 ); // seqDB-8894
   ASSERT_EQ( 1, result );        
}
