/***************************************************
 * @Description : test cl function para 
 *                seqDB-14281:getDataBlocks参数校验
 *                seqDB-14282:getQueryMeta参数校验
 *                seqDB-14301:alterCL参数校验
 *                seqDB-14304:getCLName/FullName参数校验
 *                seqDB-14305:splitCL参数校验
 *                seqDB-14306:create/get/dropIndex参数校验
 *                seqDB-14307:getCount参数校验
 *                seqDB-14308:insert/bulkInsert参数校验
 *                seqDB-14309:update/upsert参数校验
 *                seqDB-14310:delete参数校验
 *                seqDB-14311:query/queryAndUpdate/queryAndRemove参数校验
 *                seqDB-14312:explain参数校验
 *                seqDB-14319:releaseCollection参数校验
 *                seqDB-14325:aggregate参数校验
 *                seqDB-14326:attach/detachCL参数校验
 *                seqDB-14337:open/truncate/removeLob参数校验
 *                seqDB-14341:listLobs/LobPieces参数校验
 *                seqDB-14346:create/dropIdIndex参数校验
 * @Modify      : Liang xuewang
 *                2018-02-09
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class clParaVerify : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()  
   {
      testBase::SetUp() ;
      csName = "clParaVerifyTestCs" ;
      clName = "clParaVerifyTestCl" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      INT32 rc = SDB_OK ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( clParaVerify, getDataBlocks )
{
   INT32 rc = SDB_OK ;

   // test sdbGetDataBlocks
   sdbCursorHandle cursor ;
   rc = sdbGetDataBlocks( NULL, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDataBlocks( SDB_INVALID_HANDLE, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDataBlocks( cs, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetDataBlocks( cl, NULL, NULL, NULL, NULL, 0, -1, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, getQueryMeta )
{
   INT32 rc = SDB_OK ;

   // test sdbGetQueryMeta
   sdbCursorHandle cursor ;
   rc = sdbGetQueryMeta( NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetQueryMeta( SDB_INVALID_HANDLE, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetQueryMeta( cs, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetQueryMeta( cl, NULL, NULL, NULL, 0, -1, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, alterCl )
{
   INT32 rc = SDB_OK ;
   
   // test sdbAlterCollection
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "ReplSize", 0 ) ;
   bson_finish( &option ) ;
   rc = sdbAlterCollection( NULL, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAlterCollection( SDB_INVALID_HANDLE, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAlterCollection( cs, &option ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   bson_destroy( &option ) ;

   bson_init( &option ) ;
   bson_append_start_object( &option, "Alter" ) ;
   bson_append_int( &option, "ReplSize", 0 ) ;
   bson_append_finish_object( &option ) ;
   bson_finish( &option ) ;
   rc = sdbAlterCollection( NULL, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAlterCollection( SDB_INVALID_HANDLE, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAlterCollection( cs, &option ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   bson_destroy( &option ) ;

   rc = sdbAlterCollection( cl, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   
   bson_init( &option ) ;
   bson_append_int( &option, "Alter", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbAlterCollection( cl, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &option ) ;

   bson_init( &option ) ;
   bson_append_start_object( &option, "Alter" ) ;
   bson_append_int( &option, "ReplSize", 0 ) ;
   bson_append_finish_object( &option ) ;
   bson_append_int( &option, "Options", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbAlterCollection( cl, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &option ) ;
}

TEST_F( clParaVerify, getName )
{
   INT32 rc = SDB_OK ;

   // test sdbGetCLName
   CHAR clName[ MAX_NAME_SIZE+1 ] = { 0 } ;
   INT32 size = MAX_NAME_SIZE+1 ;
   rc = sdbGetCLName( NULL, clName, size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLName( SDB_INVALID_HANDLE, clName, size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLName( cs, clName, size ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetCLName( cl, NULL, size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLName( cl, clName, -1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;
   rc = sdbGetCLName( cl, clName, 0 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;
   rc = sdbGetCLName( cl, clName, 1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;

   // test sdbGetCLFullName
   CHAR clFullName[ 2*MAX_NAME_SIZE+2 ] = { 0 } ;
   size = 2*MAX_NAME_SIZE+2 ;
   rc = sdbGetCLFullName( NULL, clFullName, size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLFullName( SDB_INVALID_HANDLE, clFullName, size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLFullName( cs, clFullName, size ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetCLFullName( cl, NULL, size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLFullName( cl, clFullName, -1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;
   rc = sdbGetCLFullName( cl, clFullName, 0 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;
   rc = sdbGetCLFullName( cl, clFullName, 1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;
}

TEST_F( clParaVerify, splitCl )
{
   INT32 rc = SDB_OK ;

   // test sdbSplitCollection
   const CHAR* srcGroup = "group1" ;
   const CHAR* dstGroup = "group2" ;
   bson splitCond ;
   bson_init( &splitCond ) ;
   bson_append_int( &splitCond, "a", 10 ) ;
   bson_finish( &splitCond ) ;
   rc = sdbSplitCollection( NULL, srcGroup, dstGroup, &splitCond, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollection( SDB_INVALID_HANDLE, srcGroup, dstGroup, &splitCond, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollection( cs, srcGroup, dstGroup, &splitCond, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbSplitCollection( cl, NULL, dstGroup, &splitCond, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollection( cl, srcGroup, NULL, &splitCond, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollection( cl, srcGroup, dstGroup, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbSplitCLAsync
   SINT64 taskID ;
   rc = sdbSplitCLAsync( NULL, srcGroup, dstGroup, &splitCond, NULL, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLAsync( SDB_INVALID_HANDLE, srcGroup, dstGroup, &splitCond, NULL, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLAsync( cs, srcGroup, dstGroup, &splitCond, NULL, &taskID ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbSplitCLAsync( cl, NULL, dstGroup, &splitCond, NULL, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLAsync( cl, srcGroup, NULL, &splitCond, NULL, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLAsync( cl, srcGroup, dstGroup, NULL, NULL, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLAsync( cl, srcGroup, dstGroup, &splitCond, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &splitCond ) ;

   // test sdbSplitCollectionByPercent
   FLOAT64 percent = 50.0 ;
   rc = sdbSplitCollectionByPercent( NULL, srcGroup, dstGroup, percent ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollectionByPercent( SDB_INVALID_HANDLE, srcGroup, dstGroup, percent ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollectionByPercent( cs, srcGroup, dstGroup, percent ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbSplitCollectionByPercent( cl, NULL, dstGroup, percent ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollectionByPercent( cl, srcGroup, NULL, percent ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollectionByPercent( cl, srcGroup, dstGroup, -1.0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollectionByPercent( cl, srcGroup, dstGroup, 0.0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCollectionByPercent( cl, srcGroup, dstGroup, 101.0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   
   // test sdbSplitCLByPercentAsync
   rc = sdbSplitCLByPercentAsync( NULL, srcGroup, dstGroup, percent, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( SDB_INVALID_HANDLE, srcGroup, dstGroup, percent, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( cs, srcGroup, dstGroup, percent, &taskID ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbSplitCLByPercentAsync( cl, NULL, dstGroup, percent, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( cl, srcGroup, NULL, percent, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( cl, srcGroup, dstGroup, -1.0, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( cl, srcGroup, dstGroup, 0.0, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( cl, srcGroup, dstGroup, 101.0, &taskID ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   // rc = sdbSplitCLByPercentAsync( cl, srcGroup, dstGroup, percent, NULL ) ;
   // ASSERT_EQ( SDB_INVALIDARG, rc ) ;  // didn't check taskID in sdbSplitCLByPercentAsync
}

TEST_F( clParaVerify, index )
{
   INT32 rc = SDB_OK ;

   // sdbCreateIndex same with sdbCreateIndex1, no need to test
   // test sdbCreateIndex1
   bson indexDef ;
   bson_init( &indexDef ) ;
   bson_append_int( &indexDef, "a", 1 ) ;
   bson_finish( &indexDef ) ;
   const CHAR* indexName = "aIndex" ;
   INT32 sortBufSize = 64 ;
   rc = sdbCreateIndex1( NULL, &indexDef, indexName, false, false, sortBufSize ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIndex1( SDB_INVALID_HANDLE, &indexDef, indexName, false, false, sortBufSize ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIndex1( cs, &indexDef, indexName, false, false, sortBufSize ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCreateIndex1( cl, NULL, indexName, false, false, sortBufSize ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIndex1( cl, &indexDef, NULL, false, false, sortBufSize ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIndex1( cl, &indexDef, indexName, false, false, -1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &indexDef ) ;

   // test sdbGetIndexes
   sdbCursorHandle cursor ;
   rc = sdbGetIndexes( NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetIndexes( SDB_INVALID_HANDLE, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetIndexes( cs, NULL, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetIndexes( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;  // should be SDB_INVALIDARG ?
   
   // test sdbDropIndex
   rc = sdbDropIndex( NULL, indexName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropIndex( SDB_INVALID_HANDLE, indexName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropIndex( cs, indexName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbDropIndex( cl, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, getCount )
{
   INT32 rc = SDB_OK ;

   // sdbGetCount same with sdbGetCount1, no need to test
   // test sdbGetCount1
   SINT64 count ;
   rc = sdbGetCount1( NULL, NULL, NULL, &count ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCount1( SDB_INVALID_HANDLE, NULL, NULL, &count ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCount1( cs, NULL, NULL, &count ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetCount1( cl, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, insert )
{
   INT32 rc = SDB_OK ;

   // sdbInsert same with sdbInsert1, no need to test
   // test sdbInsert1
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "a", 1 ) ;
   bson_finish( &obj ) ;
   rc = sdbInsert1( NULL, &obj, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbInsert1( SDB_INVALID_HANDLE, &obj, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbInsert1( cs, &obj, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbInsert1( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &obj ) ;

   // test sdbBulkInsert
   const INT32 num = 10 ;
   bson* docs[num] ;
   for( INT32 i = 0;i < num-1;i++ )
   {
      docs[i] = bson_create() ;
      bson_append_int( docs[i], "a", i ) ;
      bson_finish( docs[i] ) ;
   }
   docs[num-1] = NULL ;
   rc = sdbBulkInsert( NULL, 0, docs, num ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbBulkInsert( SDB_INVALID_HANDLE, 0, docs, num ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbBulkInsert( cs, 0, docs, num ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbBulkInsert( cl, 0, NULL, num ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbBulkInsert( cl, 0, docs, -1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbBulkInsert( cl, 0, docs, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbBulkInsert( cl, 0, docs, num ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   for( INT32 i = 0;i < num-1;i++ )
   {
      bson_dispose( docs[i] ) ;
   }
}

TEST_F( clParaVerify, updateUpsert )
{
   INT32 rc = SDB_OK ;
   
   // sdbUpdate1 sdbUpsert sdbUpsert1 sdbUpsert2 same with sdbUpdate,
   // no need to test
   // test sdbUpdate
   rc = sdbUpdate( NULL, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUpdate( SDB_INVALID_HANDLE, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUpdate( cs, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( clParaVerify, del )
{
   INT32 rc = SDB_OK ;

   // test sdbDelete
   rc = sdbDelete( NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDelete( SDB_INVALID_HANDLE, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDelete( cs, NULL, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( clParaVerify, query )
{
   INT32 rc = SDB_OK ;

   // sdbQuery sdbQueryAndRemove same with sdbQuery1, no need to test
   // test sdbQuery1
   sdbCursorHandle cursor = 0;
   rc = sdbQuery1( NULL, NULL, NULL, NULL, NULL, 0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbQuery1( SDB_INVALID_HANDLE, NULL, NULL, NULL, NULL, 0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbQuery1( cs, NULL, NULL, NULL, NULL, 0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbQuery1( cl, NULL, NULL, NULL, NULL, 0, -1, 0, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson orderby ;
   bson_init(&orderby) ;
   bson_append_string(&orderby, "sort", "INVALID") ;
   bson_finish(&orderby) ;
   rc = sdbQuery1( cl, NULL, NULL, &orderby, NULL, 0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc );
   bson_destroy(&orderby);

   // test sdbQueryAndUpdate
   rc = sdbQueryAndUpdate( cl, NULL, NULL, NULL, NULL, NULL, 0, -1, 0, TRUE, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, explain )
{
   INT32 rc = SDB_OK ;

   // test sdbExplain
   sdbCursorHandle cursor ;
   rc = sdbExplain( NULL, NULL, NULL, NULL, NULL, 0, -1, 0, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExplain( SDB_INVALID_HANDLE, NULL, NULL, NULL, NULL, 0, -1, 0, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExplain( cs, NULL, NULL, NULL, NULL, 0, -1, 0, NULL, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( clParaVerify, release )
{
   // test sdbReleaseCollection
   sdbReleaseCollection( NULL ) ;
   sdbReleaseCollection( SDB_INVALID_HANDLE ) ;
   sdbReleaseCollection( cs ) ;
}

TEST_F( clParaVerify, aggregate )
{
   INT32 rc = SDB_OK ;

   // test sdbAggregate
   const SINT32 num = 2 ;
   const CHAR* ops[num] = {
      "{ $match: { $and: [ { no: { $gt: 1002 } },{ dep: \"IT Academy\" } ] } }",
      "{ $project: { no: 1, \"info.name\": 1, major: 1 } }"
      } ;
   bson* obj[num+1] ;
   for( INT32 i = 0;i < num;i++ )
   {
      obj[i] = bson_create() ;
      jsonToBson( obj[i], ops[i] ) ;
   }
   obj[num] = NULL ;
   sdbCursorHandle cursor ;
   rc = sdbAggregate( NULL, obj, num, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAggregate( SDB_INVALID_HANDLE, obj, num, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAggregate( cs, obj, num, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbAggregate( cl, NULL, num, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAggregate( cl, obj, num+1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAggregate( cl, obj, -1, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAggregate( cl, obj, 0, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAggregate( cl, obj, num, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ; 
   for( INT32 i = 0;i < num;i++ )
   {
      bson_dispose( obj[i] ) ;
   }
}

TEST_F( clParaVerify, attachDetach )
{
   INT32 rc = SDB_OK ;

   // test sdbAttachCollection
   bson option ;
   bson_init( &option ) ;
   const CHAR* op = "{ LowBound: { a: 10 }, UpBound: { a: 100 } }" ;
   jsonToBson( &option, op ) ;
   CHAR clFullName[ 2*MAX_NAME_SIZE+2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;
   rc = sdbAttachCollection( NULL, clFullName, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAttachCollection( SDB_INVALID_HANDLE, clFullName, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAttachCollection( cs, clFullName, &option ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   CHAR longClFullName[ 2*MAX_NAME_SIZE+3 ] = { 0 } ;
   memset( longClFullName, 'x', 2*MAX_NAME_SIZE+2 ) ;
   rc = sdbAttachCollection( cl, longClFullName, &option ) ;  // check name len is MAX_NAME_SIZE ?
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAttachCollection( cl, clFullName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &option ) ;

   // test sdbDetachCollection
   rc = sdbDetachCollection( NULL, clFullName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDetachCollection( SDB_INVALID_HANDLE, clFullName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDetachCollection( cs, clFullName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbDetachCollection( cl, longClFullName ) ;  // check name len is MAX_NAME_SIZE ?
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, lob )
{
   INT32 rc = SDB_OK ;
 
   // test sdbOpenLob
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob ;
   rc = sdbOpenLob( NULL, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbOpenLob( SDB_INVALID_HANDLE, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbOpenLob( cs, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbOpenLob( cl, &oid, 2, &lob ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   // rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, NULL ) ;  // core dump
   // ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbRemoveLob
   rc = sdbRemoveLob( NULL, &oid ) ;  // check oid NULL repeatedly in sdbRemoveLob ?
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveLob( SDB_INVALID_HANDLE, &oid ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveLob( cs, &oid ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbRemoveLob( cl, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   
   // test sdbTruncateLob
   rc = sdbTruncateLob( NULL, &oid, 0 ) ;  // check oid NULL repeatedly in sdbTruncateLob ?
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateLob( SDB_INVALID_HANDLE, &oid, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateLob( cs, &oid, 0 ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbTruncateLob( cl, NULL, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateLob( cl, &oid, -1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, listLobs )
{
   INT32 rc = SDB_OK ;

   // test sdbListLobs
   sdbCursorHandle cursor ;
   rc = sdbListLobs( NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListLobs( SDB_INVALID_HANDLE, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListLobs( cs, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbListLobs( cl, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbListLobPieces
   rc = sdbListLobPieces( NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListLobPieces( SDB_INVALID_HANDLE, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListLobPieces( cs, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbListLobPieces( cl, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( clParaVerify, idIndex )
{
   INT32 rc = SDB_OK ;

   // test sdbCreateIdIndex
   rc = sdbCreateIdIndex( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIdIndex( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIdIndex( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   // test sdbDropIdIndex
   rc = sdbDropIdIndex( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropIdIndex( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropIdIndex( cs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}
