/*****************************************************************
 * @Description:    test case for C++ driver data idIndex
 *                  seqDB-12515:创建/获取/删除ID索引 
 * @Modify:         Liang xuewang Init
 *                  2017-09-12
 *****************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class idIndexTest12515 : public testBase
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
      csName = "testIdIndexCs12515" ;
      clName = "testIdIndexCl12515" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      BSONObj option = BSON( "AutoIndexId" << false ) ;
      rc = cs.createCollection( clName, option, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;   
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

// 测试创建/获取/删除ID索引
TEST_F( idIndexTest12515, idIndexOpr12515 )
{
   INT32 rc = SDB_OK ;

   // insert
   BSONObj doc = BSON( "a" << 12 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // create id index
   rc = cl.createIdIndex() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create id index" ;

   // get id index
   sdbCursor cursor ;
   rc = cl.getIndexes( cursor, "$id" ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get id index" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( "$id", obj.getField( "IndexDef" ).Obj().getField( "name" ).String().c_str() )
                 << "fail to check get id index" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // update with id index
   BSONObj rule = BSON( "$inc" << BSON( "a" << 1 ) ) ;
   rc = cl.update( rule ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update with id index" ;

   // drop id index
   rc = cl.dropIdIndex() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop id index" ;
   rc = cl.getIndexes( cursor, "$id" ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get id index" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check drop id index" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // update without id index
   rc = cl.update( rule ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) << "fail to check update without id index" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}
