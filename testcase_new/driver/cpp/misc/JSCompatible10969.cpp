/**************************************************************
 * @Description: test case of $numberLong JSCompatible 
 *				     seqDB-10969:java_numberlong类型显示格式校验  
 * @Modify     : Liang xuewang
 *			 	     2017-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <string>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class JSCompatibleTest10969 : public testBase
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
      csName = "JSCompatibleTestCs10969" ;
      clName = "JSCompatibleTestCl10969" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( JSCompatibleTest10969, JSfalse10969 )
{
   INT32 rc = SDB_OK ;

   // insert int/long/double max min 
   INT32 a[] = { -2147483648, 0, 2147483647 } ;  // -2^31 0 2^31-1
   SINT64 b[] = { -9223372036854775808, -9007199254740992, -9007199254740991, 1, 
                  9007199254740991, 9007199254740992, 9223372036854775807 } ; // -2^63 -2^53 -2^53+1 1 2^53-1 2^53 2^63-1

   BSONObjBuilder builder ;
   CHAR key[10] ;
   for( INT32 i = 0;i < sizeof(a)/sizeof(a[0]);i++ )
   {
      sprintf( key, "%s%d", "int", i ) ;
      builder.append( key, a[i] ) ;
   }
   for( INT32 i = 0;i < sizeof(b)/sizeof(b[0]);i++ )
   {
      sprintf( key, "%s%d", "long", i ) ;
      builder.append( key, b[i] ) ;
   }
   BSONObj doc = builder.obj() ;

   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // query data
   sdbCursor cursor ;
   BSONObj selector = BSON( "_id" << BSON( "$include" << 0 ) ) ;
   rc = cl.query( cursor, _sdbStaticObject, selector ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;

   const CHAR* expect = "{ \"int0\": -2147483648, \"int1\": 0, \"int2\": 2147483647,"
                         " \"long0\": -9223372036854775808, \"long1\": -9007199254740992,"
                         " \"long2\": -9007199254740991, \"long3\": 1, \"long4\": 9007199254740991,"
                         " \"long5\": 9007199254740992, \"long6\": 9223372036854775807 }" ; 
   string str = obj.toString() ;
   const CHAR* real = str.c_str() ;
   ASSERT_STREQ( expect, real ) << "fail to check query" ;
}

TEST_F( JSCompatibleTest10969, JStrue10969 )
{
   INT32 rc = SDB_OK ;

   BSONObj::setJSCompatibility( true ) ;

   // insert int/long/double max min 
   INT32 a[] = { -2147483648, 0, 2147483647 } ;  // -2^31 0 2^31-1
   SINT64 b[] = { -9223372036854775808, -9007199254740992, -9007199254740991, 1,
                  9007199254740991, 9007199254740992, 9223372036854775807 } ; // -2^63 -2^53 -2^53+1 1 2^53-1 2^53 2^63-1
   BSONObjBuilder builder ;
   CHAR key[10] ;
   for( INT32 i = 0;i < sizeof(a)/sizeof(a[0]);i++ )
   {
      sprintf( key, "%s%d", "int", i ) ;
      builder.append( key, a[i] ) ;
   }
   for( INT32 i = 0;i < sizeof(b)/sizeof(b[0]);i++ )
   {
      sprintf( key, "%s%d", "long", i ) ;
      builder.append( key, b[i] ) ;
   }
   BSONObj doc = builder.obj() ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // query data
   sdbCursor cursor ;
   BSONObj selector = BSON( "_id" << BSON( "$include" << 0 ) ) ;
   rc = cl.query( cursor, _sdbStaticObject, selector ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query data" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to get next in cursor" ;
   const CHAR* expect = "{ \"int0\": -2147483648, \"int1\": 0, \"int2\": 2147483647,"
                         " \"long0\": { \"$numberLong\": \"-9223372036854775808\" },"
                         " \"long1\": { \"$numberLong\": \"-9007199254740992\" },"
                         " \"long2\": -9007199254740991, \"long3\": 1, \"long4\": 9007199254740991,"
                         " \"long5\": { \"$numberLong\": \"9007199254740992\" },"
                         " \"long6\": { \"$numberLong\": \"9223372036854775807\" } }" ;
   string str = obj.toString() ;
   const CHAR* real = str.c_str() ;
   ASSERT_STREQ( expect, real ) << "fail to check query" ;
}
