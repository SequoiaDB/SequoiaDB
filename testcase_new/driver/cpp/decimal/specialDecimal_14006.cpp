/**************************************************************
 * @Description: test decimal
 *               seqDB-14006:插入特殊decimal值 
 *               seqDB-14007:删除特殊decimal值
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

class specialDecimalTest14006 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "specialDecimalTestCs14006" ;
      clName = "specialDecimalTestCl14006" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( specialDecimalTest14006, min )
{   
   INT32 rc = SDB_OK;

   bsonDecimal dec ;    
   dec.setMin() ;
   ASSERT_TRUE( dec.isMin() ) << "fail to check min" ;
   
   BSONObj doc = BSON( "a" << dec ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert min" ;
   sdbCursor cursor ;
   rc = cl.query( cursor, doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bsonDecimal res = obj.getField( "a" ).Decimal() ;
   ASSERT_TRUE( res.isMin() ) << "fail to check query res" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   rc = cl.del( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to del" ;
   rc = cl.query( cursor, doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check query after del" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( specialDecimalTest14006, max )
{   
   INT32 rc = SDB_OK;

   bsonDecimal dec ;    
   dec.setMax() ;
   ASSERT_TRUE( dec.isMax() ) << "fail to check max" ;
   
   BSONObj doc = BSON( "a" << dec ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert max" ;
   sdbCursor cursor ;
   rc = cl.query( cursor, doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bsonDecimal res = obj.getField( "a" ).Decimal() ;
   ASSERT_TRUE( res.isMax() ) << "fail to check query res" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   rc = cl.del( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to del" ;
   rc = cl.query( cursor, doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check query after del" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
