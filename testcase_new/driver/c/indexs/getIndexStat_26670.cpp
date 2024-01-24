/*
 * @Description   : seqDB-26670:C驱动功能测试
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.20
 * @LastEditTime  : 2022.07.21
 * @LastEditors   : Lin Suqiang
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class getIndexStat26670: public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* indexName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "getIndexStat26670" ;
      clName = "getIndexStat26670" ;
      indexName = "index_26670" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;

      // create cs cl
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = sdbCreateCollection( cs, clName, &cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

      // insert data
      bson* docs[10] ;
      for( INT32 i = 0; i < 10; i++ )
      {
         docs[i] = bson_create() ;
         rc = bson_append_int( docs[i], "a", i ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         rc = bson_finish( docs[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      rc = sdbBulkInsert( cl, 0, docs, 10 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      for( INT32 i = 0; i < 10; i++ )
      {
         bson_dispose( docs[i] ) ;
      }

      // create index
      bson indexDef ;
      bson_init( &indexDef ) ;
      bson_append_int( &indexDef, "a", 1 ) ;
      bson_finish( &indexDef) ;
      rc = sdbCreateIndex( cl, &indexDef, indexName, FALSE, FALSE ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &indexDef) ;
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

TEST_F( getIndexStat26670, getIndexStat )
{
   INT32 rc = SDB_OK ;
   // analyze
   CHAR clFullName[ 2 * MAX_NAME_SIZE + 2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "Collection", clFullName ) ;
   bson_finish( &option ) ;
   rc = sdbAnalyze( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &option ) ;

   // get index statistics withou MCV
   bson_type type ;
   bson_iterator it ;

   bson resWoMCV ;
   bson_init( &resWoMCV ) ;
   rc = sdbCLGetIndexStat1( cl, indexName, &resWoMCV, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // bson_print( &resWoMCV ) ;
   type = bson_find( &it, &resWoMCV, "MCV" ) ;
   ASSERT_TRUE( BSON_EOO == type ) ;
   bson_destroy( &resWoMCV ) ;

   // get index statistics with MCV
   bson resWithMCV ;
   bson_init( &resWithMCV ) ;
   rc = sdbCLGetIndexStat1( cl, indexName, &resWithMCV, TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // bson_print( &resWithMCV ) ;
   type = bson_find( &it, &resWithMCV, "MCV" ) ;
   ASSERT_TRUE( BSON_EOO != type ) ;
   bson_destroy( &resWithMCV ) ;
}
