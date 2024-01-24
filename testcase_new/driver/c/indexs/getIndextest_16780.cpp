/***************************************************
 * @Description : test case of getIndex 
 *                seqDB-16780: 获取索引
 * @Modify      : wenjing wang
 *                2018-12-11
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const char* idxName = "idx_a" ;
class GetIndexTest16780 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   bson idx ;
  
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      bson_init( &idx ) ;
      bson_append_int( &idx, "a", 1 );
      bson_finish( &idx ) ;
      
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csName = "getIndex16780" ;
      clName = "getIndex16780" ;

      rc = sdbCreateCollectionSpace( db, csName, 4096, &cs ) ;
      if ( rc != -33 ){
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      }

      rc = sdbCreateCollection( cs, clName,  &cl ) ;
      if ( rc != -22 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
      }
      
      rc = sdbCreateIndex( cl, &idx, idxName, TRUE, FALSE ) ;
      if ( rc != -247 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create index " << clName ;
      }
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      bson_destroy( &idx ) ;
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( cl ) ;
      testBase::TearDown() ;
   }
} ;

INT32 checkIdx( bson* idxInfo, const char* idxName, const char* key, BOOLEAN unique)
{
   INT32 rc = 0 ;
   bson_iterator it ;
   bson sub, leaf ;
   bson_init( &sub ) ;
   bson_init( &leaf ) ;
   bson_type type = bson_find( &it, idxInfo, "IndexDef" ) ;
   if ( type == BSON_OBJECT )
   {
      bson_iterator subIt,leafIt ;
      bson_iterator_subobject( &it, &sub );
      type = bson_find( &subIt, &sub, "name" ) ;
      if ( type != BSON_STRING ) 
      {
         rc = -1 ;
         goto done ;
      }
      
      if ( 0 != strcmp( bson_iterator_string(&subIt), idxName) )
      {
         rc = -1 ;
         goto done ;
      }
      
      type = bson_find( &subIt, &sub, "unique" ) ;
      if ( type != BSON_BOOL )
      {
         rc = -1 ;
         goto done ;
      }
      
      if ( bson_iterator_bool(&subIt) != unique )
      {
         rc = -1 ;
         goto done ;
      }
      
      type = bson_find( &subIt, &sub, "key" ) ;
      if ( type != BSON_OBJECT )
      {
         rc = -1 ;
         goto done ;
      }
      
      bson_iterator_subobject( &subIt, &leaf );
      type = bson_find( &leafIt, &leaf, key ) ;
      if ( type != BSON_INT )
      {
         rc = -1 ;
         goto done ;
      }
done:
      bson_destroy ( &leaf ) ;
      bson_destroy( &sub );
      return rc ;
   }
   else
   {
      return -1 ;
   }
}
TEST_F( GetIndexTest16780, getSpecificIndex )
{
   INT32 rc = SDB_OK ;
   
   bson rtnIdx ;
   bson_init ( &rtnIdx );
   
   rc = sdbGetIndex( cl, idxName, &rtnIdx ) ;
   ASSERT_EQ( SDB_OK, rc ) << "get index " << idxName << " failed" ;
   
   rc = checkIdx( &rtnIdx, idxName, "a", TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "get index " << idxName << " failed" ;
   bson_destroy( &rtnIdx ) ;
}

TEST_F( GetIndexTest16780, getAllIndex )
{
   INT32 rc = SDB_OK ;
   const char* indexName = "$id" ;
   bson rtnIdx ;
   bson_init( &rtnIdx ) ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
  
   rc = sdbGetIndexInfo( cl, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "get all index failed" ;
   
   
   while ((rc = sdbNext(cursor, &rtnIdx )) == SDB_OK ){
      rc = checkIdx( &rtnIdx, idxName, "a", TRUE ) ;
      if ( rc == -1 )
      {
         indexName = "$id" ;
         rc = checkIdx( &rtnIdx, indexName, "_id", TRUE ) ;
      }
      else
      {
         indexName = idxName ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "get index " << indexName << " failed" ;
      bson_destroy( &rtnIdx ) ; 
      bson_init( &rtnIdx ) ;
   }
   sdbReleaseCursor( cursor ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "get all index failed" ;
}
