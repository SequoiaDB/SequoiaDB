/*
 * @Description   : seqDB-29800:SDB 支持返回静态表统计信息
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.01.05
 * @LastEditTime  : 2023.01.10
 * @LastEditors   : HuangHaimei
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class getCollectionStat29800: public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "getCollectionStat29800" ;
      clName = "getCollectionStat29800" ;
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

TEST_F( getCollectionStat29800, getCollectionStat )
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

   // get cl statistics
   bson res ;
   bson_init( &res ) ;
   rc = sdbCLGetCollectionStat( cl, &res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_iterator it ;
   bson_find( &it, &res, "Collection") ;
   ASSERT_EQ( 0, strncmp( clFullName, bson_iterator_string( &it ), strlen( bson_iterator_string( &it ) ) ) ) ;
   bson_find( &it, &res, "IsDefault" ) ;
   ASSERT_EQ( FALSE, bson_iterator_bool( &it ) ) ;
   bson_find( &it, &res, "IsExpired" ) ;
   ASSERT_EQ( FALSE, bson_iterator_bool( &it ) ) ;
   bson_find( &it, &res, "AvgNumFields" ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "SampleRecords" ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "TotalRecords" ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "TotalDataPages" ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "TotalDataSize" ) ;
   ASSERT_EQ( 290, bson_iterator_long( &it ) ) ;

   bson_destroy( &res ) ;
}