/*******************************************************************************
 * @Description:    test case for C++ driver data index explain queryMeta
 *                  seqDB-12514:创建/获取/删除索引
 *                  seqDB-12672:查看访问计划
 *                  seqDB-12546:getQueryMeta获取查询元数据信息
 * @Modify:         Liang xuewang Init
 *                  2017-09-12
 *******************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class indexTest12514 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* idxName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "testIndexCs12514" ;
      clName = "testIndexCl12514" ;
      idxName = "testIndex12514" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = cs.createCollection( clName, cl ) ;
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

// 测试创建/获取/删除索引，访问计划，查询元数据( 12514 12672 12546 )
TEST_F( indexTest12514, indexOpr12514 )
{
   INT32 rc = SDB_OK ;

   // create index
   BSONObj idxDef = BSON( "a" << -1 ) ;
   rc = cl.createIndex( idxDef, idxName, false, false ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to create index" ;

   // get index
   sdbCursor cursor ;
   rc = cl.getIndexes( cursor, idxName ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to get index" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to get next" ;
   ASSERT_STREQ( idxName, obj.getField( "IndexDef" ).Obj().getField( "name" ).String().c_str() ) 
                 << "fail to check index name" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
      
   // explain
   BSONObj cond = BSON( "a" << 100 ) ;
   rc = cl.explain( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to explain" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( "ixscan", obj.getField( "ScanType" ).String().c_str() ) << "fail to check scan type" ;
   ASSERT_STREQ( idxName, obj.getField( "IndexName" ).String().c_str() ) << "fail to check scan index" ;
   ASSERT_TRUE( obj.getField( "Indexblocks" ).eoo() ) << "fail to check Indexblocks eoo" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // get query meta
   rc = cl.getQueryMeta( cursor, cond, _sdbStaticObject, _sdbStaticObject, 0, -1 ) ; // error
   ASSERT_EQ( SDB_OK, rc ) << "fail to get query meta" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( "ixscan", obj.getField( "ScanType" ).String().c_str() ) << "fail to check scan type" ;
   ASSERT_STREQ( idxName, obj.getField( "IndexName" ).String().c_str() ) << "fail to check scan index" ;
   ASSERT_FALSE( obj.getField( "Indexblocks" ).eoo() ) << "fail to check Indexblocks not eoo" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop index
   rc = cl.dropIndex( idxName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop index"  ;
   rc = cl.getIndexes( cursor, idxName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get indexes" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check drop index" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
