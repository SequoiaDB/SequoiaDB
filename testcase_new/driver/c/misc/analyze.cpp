/********************************************************
 * @Description: test case for c driver
 *               seqDB-11666:c驱动支持统计
 * @Modify:      Liangxw
 *               2017-09-26
 ********************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class analyzeTest : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      csName = "analyzeTestCs" ;
      clName = "analyzeTestCl" ;
      INT32 rc = SDB_OK ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

      bson indexDef ;
      bson_init( &indexDef ) ;
      bson_append_int( &indexDef, "a", 1 ) ;
      bson_finish( &indexDef ) ;
      rc = sdbCreateIndex( cl, &indexDef, "aIndex", false, false ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create index" ;

      INT32 num = 10000 ;
      bson* docs[num] ;
      for( INT32 i = 0;i < num;++i )
      {
         docs[i] = bson_create() ;
         bson_append_int( docs[i], "a", 100 ) ;
         bson_append_string( docs[i], "b", "12345" ) ;
         bson_finish( docs[i] ) ;
      }
      rc = sdbBulkInsert( cl, 0, docs, num ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to bulk insert" ;
      for( INT32 i = 0;i < num;i++ )
      {
         bson_dispose( docs[i] ) ;
      }
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

// query doc { a:100 } and explain
INT32 explainDoc( sdbCollectionHandle cl, CHAR* scanType )
{
   INT32 rc = SDB_OK ;
   bson doc, obj ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson_init( &doc ) ;
   bson_init( &obj ) ;
   bson_iterator it ;

   bson_append_int( &doc, "a", 100 ) ;
   bson_finish( &doc ) ;
   rc = sdbExplain( cl, &doc, NULL, NULL, NULL, 0, 0, -1, NULL, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to explain" ) ;
   rc = sdbNext( cursor, &obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to get next of cursor" ) ;
   bson_find( &it, &obj, "ScanType" ) ;
   strcpy( scanType, bson_iterator_string( &it ) ) ;

done:
   bson_destroy( &doc ) ;
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

TEST_F( analyzeTest, explain )
{
   INT32 rc = SDB_OK ;

   // explain query before analyze, use idx-scan
   char scanType[20] ;
   rc = explainDoc( cl, scanType ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_STREQ( "ixscan", scanType ) ;

   // analyze
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "CollectionSpace", csName ) ;
   bson_finish( &option ) ;
   rc = sdbAnalyze( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to analyze cs " << csName ;
   bson_destroy( &option ) ;

   // query record after analyze, use tb-scan
   rc = explainDoc( cl, scanType ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_STREQ( "tbscan", scanType ) ;
}
