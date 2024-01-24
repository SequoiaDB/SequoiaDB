/**************************************************************
 * @Description: insert and query decimal.
 *               seqDB-8887 : 接口bsonDecimal::fromInt( INT32 value )测试 
 *               seqDB-12754 : 构造bsonDecimal数据并插入查询 
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
#include "testBase.hpp"

using namespace std;
using namespace sdbclient;
using namespace bson; 

class decimal8887 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollection cl ;

   void SetUp() 
   {
      testBase::SetUp() ;

      pCsName = "decimal8887" ;
      pClName = "decimal8887" ;
      sdbCollectionSpace cs ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown() 
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( decimal8887, insert_query )
{   
   INT32 rc = SDB_OK ;

   //decimal
   bsonDecimal decimal;
   decimal.fromInt( 1000 ); // seqDB-8887

   //bson
   BSONObj obj;
   BSONObjBuilder b;
   b.append( "a", decimal );
   obj = b.obj();

   //insert
   rc = cl.del();
   ASSERT_EQ( SDB_OK, rc );   
   rc = cl.insert( obj );
   ASSERT_EQ( SDB_OK, rc );

   //query
   INT32 i = 0;
   sdbCursor cursor;
   cl.query( cursor );
   BSONObj getOBj;
   while( !( cursor.next(getOBj) ) )
   {  
      //check value
      BSONElement e = getOBj.getField( "a" );
      bsonDecimal getDec = e.numberDecimal();     
      ASSERT_STREQ( "1000", getDec.toString().c_str() );     
      i++;
   } 
   ASSERT_EQ( 1, i );
   rc = cursor.close();
   ASSERT_EQ( SDB_OK, rc );
}
