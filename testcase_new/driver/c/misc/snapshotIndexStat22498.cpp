/**************************************************************
 * @Description: 获取索引统计信息快照
 *				     seqDB-22498 
 * @Modify:      liuxiaoxuan
 *			 	     2020-07-30
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"

class snapshotIndexStat22498 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* fullCLName ;
   const CHAR* indexName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp() 
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "snapshotIndexStat22498" ;
      clName = "snapshotIndexStat22498" ;
      fullCLName = "snapshotIndexStat22498.snapshotIndexStat22498" ;
      indexName = "idx22498" ;
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
         bson_append_int( docs[i], "a", i ) ;
         bson_finish( docs[i] ) ;
      }
      rc = sdbBulkInsert( cl, 0, docs, 10 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      for( INT32 i = 0; i < 10; i++ )
      {
         bson_dispose( docs[i] ) ;
      }

      // create index
      bson obj ;
      bson_init( &obj ) ;
      bson_append_int( &obj, "a", 1 ) ;
      bson_finish( &obj ) ;
      rc = sdbCreateIndex ( cl, &obj, indexName, FALSE, FALSE ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy ( &obj ) ;

      // analyze index
      bson_init( &obj ) ;
      bson_append_string( &obj, "Collection", fullCLName ) ;
      bson_append_string( &obj, "Index", indexName ) ;
      bson_finish( &obj ) ; 
      rc = sdbAnalyze( db, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy ( &obj ) ;
   }
   void TearDown()
   {
      if ( shouldClear() ) 
      {   
         INT32 rc = SDB_OK ;
         rc = sdbDropCollectionSpace( db, csName ) ; 
         ASSERT_EQ( SDB_OK, rc ) ; 
      }   

      sdbReleaseCollection( cl ) ; 
      sdbReleaseCS( cs ) ; 
      testBase::TearDown() ;
   }
} ;

TEST_F( snapshotIndexStat22498, snapshotIndexStat )
{
   INT32 rc = SDB_OK ;

   sdbCursorHandle cursor ;
   bson matcher ;
   bson obj ;
   bson subObj ;

   // get snapshot of indexstat
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Collection", fullCLName ) ;
   bson_append_string( &matcher, "Index", indexName ) ;
   bson_finish( &matcher ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_INDEXSTATS, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BOOLEAN isExpect = FALSE ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_iterator it, subit ;

   if ( isStandalone( db ) ) 
   {  
      if ( BSON_STRING == bson_find( &it, &obj, "GroupName" ) )
      {
         isExpect = TRUE;
      }
   } 
   else
   {
      bson_find( &it, &obj, "StatInfo" ) ;
      bson_iterator_subiterator( &it, &subit ) ;
      while( bson_iterator_more( &subit ) )
      { 
         bson_init( &subObj ) ;
         bson_iterator_subobject( &subit, &subObj ) ;
         bson_iterator i1;
         // check stat info
         if ( BSON_STRING != bson_find( &i1, &subObj, "GroupName" )
               || BSON_ARRAY != bson_find( &i1, &subObj, "Group" ) )
         {
            isExpect = FALSE ;
            break ;
         }
         else
         {
            isExpect = TRUE ;
         }
         bson_destroy( &subObj ) ;
         bson_iterator_next( &subit ) ;
      }
      bson_destroy( &subObj ) ;
   }

   if( !isExpect )
   {
      bson_print( &obj ) ;
   }

   bson_destroy( &obj ) ;
   bson_destroy( &matcher ) ;
   sdbReleaseCursor( cursor ) ;

   // check result
   ASSERT_TRUE( isExpect ) ;
}
