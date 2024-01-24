/**************************************************************
 * @Description: test decimal
 *               seqDB-8865 : 接口BSONObjBuilder::appendDecimal测试（不带精度）
 *               seqDB-8866 : 接口BSONObjBuilder::appendDecimal测试（带精度）
 *               seqDB-8867 : 接口BSONObjBuilder::append测试 
 *               seqDB-8868 : 接口BSONElement::numberDecimal测试 
 *               seqDB-8869 : 接口BSONElement::Decimal测试 
 *               seqDB-8870 : 接口BSONElement::Val(bsonDecimal& v)测试
 *               seqDB-8871 : 接口bsonDecimal::bsonDecimal测试
 *               seqDB-8872 : 接口bsonDecimal::bsonDecimal( const bsonDecimal &right )测试 
 *               seqDB-8874 : 接口bsonDecimal::~bsonDecimal()测试
 *               seqDB-8875 : 接口bsonDecimal::init()测试 
 *               seqDB-8876 : 接口bsonDecimal::init( INT32 precision, INT32 scale )测试
 *               seqDB-8886 : 接口bsonDecimal::toString() const测试 
 *               seqDB-8889 : 接口bsonDecimal::fromDouble( FLOAT64 value )测试 
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

TEST( decimal8865, append )
{   
   INT32 rc = SDB_OK;

   //decimal
   bsonDecimal dec1; // seqDB-8871
   dec1.fromInt( 104 );

   bsonDecimal dec2( dec1 ); // seqDB-8872

   bsonDecimal dec3;
   dec3.init(); 
   dec3.fromDouble( 123.12345 ); // seqDB-8875

   bsonDecimal dec4;
   dec4.init( 10, 3 ); // seqDB-8876
   dec4.fromDouble( 0.12345 ); // seqDB-8889

   //bson
   BSONObj obj;
   BSONObjBuilder b;
   b.append( "a", dec1 ); // seqDB-8867
   b.append( "b", dec2 );
   b.append( "c", dec3 );
   b.append( "d", dec4 );
   b.appendDecimal( "e", "12345.7" ); // seqDB-8865
   b.appendDecimal( "f", "12345.12345", 10, 4 ); // seqDB-8866
   obj = b.obj();

   //bson --> decimal
   BSONElement aEle = obj.getField( "a" );
   bsonDecimal aDec = aEle.numberDecimal(); // seqDB-8868
   BSONElement bEle = obj.getField( "b" );
   bsonDecimal bDec = bEle.numberDecimal();   
   BSONElement cEle = obj.getField( "c" );
   bsonDecimal cDec = cEle.numberDecimal();
   BSONElement dEle = obj.getField( "d" );
   bsonDecimal dDec = dEle.numberDecimal();
   BSONElement eEle = obj.getField( "e" );
   bsonDecimal eDec = eEle.Decimal(); // seqDB-8869
   BSONElement fEle = obj.getField( "f" );
   bsonDecimal fDec;
   fEle.Val(fDec); // seqDB-8870

   //check
   ASSERT_STREQ( "104",          aDec.toString().c_str() ); // seqDB-8886
   ASSERT_STREQ( "104",          bDec.toString().c_str() ); 
   ASSERT_STREQ( "123.12345",    cDec.toString().c_str() ); 
   ASSERT_STREQ( "0.123",        dDec.toString().c_str() ); 
   ASSERT_STREQ( "12345.7",      eDec.toString().c_str() ); 
   ASSERT_STREQ( "12345.1235",   fDec.toString().c_str() );      
}
