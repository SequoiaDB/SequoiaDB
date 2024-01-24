/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12671:不指定更新规则(update)执行queryAndUpdate
 *                 seqDB-12722:split参数校验
 *                 seqDB-12723:splitAsync参数校验
 *                 seqDB-12724:createIndex参数校验
 *                 seqDB-12725:getIndexes参数校验
 *                 seqDB-12726:dropIndex参数校验
 *                 seqDB-12727:attachCollection参数校验
 *                 seqDB-12728:detachCollection参数校验
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class clFuncParamTest12671 : public testBase
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
      csName = "clFuncParamTestCs12671" ;
      clName = "clFuncParamTestCl12671" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
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

TEST_F( clFuncParamTest12671, queryAndUpdate12671 )
{
   INT32 rc = SDB_OK ;
   
   sdbCursor cursor ;
   BSONObj update ;
   rc = cl.queryAndUpdate( cursor, update ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test queryAndUpdate with empty update" ;
}

TEST_F( clFuncParamTest12671, split12722 )
{
   INT32 rc = SDB_OK ;

   BSONObj begin = BSON( "a" << 10 ) ;
   BSONObj end = BSON( "a" << 100 ) ;
   rc = cl.split( "", "group2", begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with srcGroup empty" ;
   rc = cl.split( NULL, "group2", begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with srcGroup NULL" ;
   rc = cl.split( "group1", "", begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with dstGroup empty" ;
   rc = cl.split( "group1", NULL, begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with dstGroup NULL" ;

   rc = cl.split( "", "group2", 50 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with srcGroup empty" ;
   rc = cl.split( NULL, "group2", 50 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with srcGroup NULL" ;
   rc = cl.split( "group1", "", 50 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with dstGroup empty" ;
   rc = cl.split( "group1", NULL, 50 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with dstGroup NULL" ;
   rc = cl.split( "group1", "group2", 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with percent 0" ;
   rc = cl.split( "group1", "group2", 101 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test split with percent 101" ;
}

TEST_F( clFuncParamTest12671, splitAsync12723 )
{
   INT32 rc = SDB_OK ;

   BSONObj begin = BSON( "a" << 10 ) ;
   BSONObj end = BSON( "a" << 100 ) ;
   SINT64 taskID ;
   rc = cl.splitAsync( taskID, "", "group2", begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with srcGroup empty" ;
   rc = cl.splitAsync( taskID, NULL, "group2", begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with srcGroup NULL" ;
   rc = cl.splitAsync( taskID, "group1", "", begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with dstGroup empty" ;
   rc = cl.splitAsync( taskID, "group1", NULL, begin, end ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with dstGroup NULL" ;

   rc = cl.splitAsync( "", "group2", 50, taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with srcGroup empty" ;
   rc = cl.splitAsync( NULL, "group2", 50, taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with srcGroup NULL" ;
   rc = cl.splitAsync( "group1", "", 50, taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with dstGroup empty" ;
   rc = cl.splitAsync( "group1", NULL, 50, taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with dstGroup NULL" ;
   rc = cl.splitAsync( "group1", "group2", 0, taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with percent 0" ;
   rc = cl.splitAsync( "group1", "group2", 101, taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test splitAsync with percent 101" ;
}

TEST_F( clFuncParamTest12671, createIndex12724 )
{
   INT32 rc = SDB_OK ;
   
   BSONObj idxDef = BSON( "a" << 1 ) ;
   rc = cl.createIndex( idxDef, NULL, false, false ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test createIndex with NULL" ;
   
   rc = cl.createIndex( idxDef, "aIndex", false, false, -10 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test createIndex with sortBufferSize -10" ;
}

TEST_F( clFuncParamTest12671, getIndexes12725 )
{
   INT32 rc = SDB_OK ;
   
   sdbCursor cursor ;
   rc = cl.getIndexes( cursor, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test getIndexes with NULL" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "$id", obj.getField( "IndexDef" ).Obj().getField( "name" ).String() ) << "fail to check $id index" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( clFuncParamTest12671, dropIndex12726 )
{
   INT32 rc = SDB_OK ;

   rc = cl.dropIndex( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test dropIndex with NULL" ;
}

TEST_F( clFuncParamTest12671, attachCollection12727 )
{
   INT32 rc = SDB_OK ;

   BSONObj option = BSON( "LowBound" << BSON( "a" << 10 ) << "UpBound" << BSON( "a" << 100 ) ) ;
   string longClFullName( 128, 'x' ) ;
   const CHAR* clFullName = longClFullName.c_str() ;
   rc = cl.attachCollection( clFullName, option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test attachCollection with longClFullName" ;

   rc = cl.attachCollection( NULL, option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to attachCollection with NULL" ;
}

TEST_F( clFuncParamTest12671, detachCollection12728 )
{
   INT32 rc = SDB_OK ;

   string longClFullName( 128, 'x' ) ;
   const CHAR* clName = longClFullName.c_str() ;
   rc = cl.detachCollection( clName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to detachCollection with longClFullName" ;

   rc = cl.detachCollection( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to detachCollection with NULL" ;
}
