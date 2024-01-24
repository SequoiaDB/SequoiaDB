#include <stdio.h>
#include <gtest/gtest.h>
#include <string>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "client.h"

class collectionTest: public testBase
{
protected:
    const CHAR* csName ;
    const CHAR* clName ;
    const CHAR* clFullName ;
    sdbCSHandle cs ;
    sdbCollectionHandle cl ;

    void SetUp()  
    {
       testBase::SetUp() ;
       INT32 rc = SDB_OK ;
       csName = "collectiontest_10403_10405" ;
       clName = "collectiontest_10403_10405" ;
       clFullName = "collectiontest_10403_10405.collectiontest_10403_10405" ;
       cs = SDB_INVALID_HANDLE ;
       cl = SDB_INVALID_HANDLE ;
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

TEST_F( collectionTest,sdbGetName_10403 )
{
   sdbCursorHandle cursor         = 0 ;
   CHAR pBuffer[ NAME_LEN + 1 ]   = { 0 } ;
   CHAR pBuffer2[ 1 ]             = { 0 } ;
   CHAR *pBuffer3                 = NULL ;
   INT32 rc                       = SDB_OK ;

   rc = sdbGetCSName( cs, pBuffer, NAME_LEN + 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, csName, strlen( csName ) ) ) ;

   rc = sdbGetCLName( cl, pBuffer, NAME_LEN + 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, clName, strlen( clName ) ) ) ;
     
   rc = sdbGetCLFullName( cl, pBuffer, NAME_LEN + 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, clFullName, strlen( clFullName ) ) ) ;

   rc = sdbGetCLFullName( cl, pBuffer2, 1 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;

   rc = sdbGetCLFullName( cl, pBuffer3, 1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseCursor ( cursor ) ;
}

TEST_F( collectionTest,sdbCreateGetDropIndex_10405 )
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   // build a bson for index definition
   bson_append_int( &obj, "name", 1 ) ;
   bson_append_int( &obj, "age", -1 ) ;
   bson_finish( &obj ) ;
   // create index
   rc = sdbCreateIndex ( cl, &obj, INDEX_NAME, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;

   // get the index
   rc = sdbGetIndexes( cl, INDEX_NAME, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check result
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   bson_iterator it ;
   bson_find( &it, &obj, "unique" ) ;
   ASSERT_FALSE( bson_iterator_bool( &it ) ) ;
   bson_destroy ( &obj ) ;

   // drop index
   rc = sdbDropIndex( cl, INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbReleaseCursor ( cursor ) ;
}

TEST_F( collectionTest,sdbCreateGetDropIndex_10405_2 )
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;

   bson obj ;
   bson_init( &obj ) ;
   // build a bson for index definition
   bson_append_int( &obj, "name", 1 ) ;
   bson_append_int( &obj, "age", -1 ) ;
   bson_finish( &obj ) ;
   // create index
   bson options ;
   bson_init( &options ) ;
   bson_append_int( &options, "SortBufferSize", 1024 ) ;
   bson_finish( &options ) ;
   rc = sdbCreateIndex2 ( cl, &obj, INDEX_NAME, &options ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &options ) ;
   bson_destroy( &obj ) ;

   // get the index
   rc = sdbGetIndexes( cl, INDEX_NAME, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check result
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   bson_iterator it ;
   bson_find( &it, &obj, "unique" ) ;
   ASSERT_FALSE( bson_iterator_bool( &it ) ) ;
   bson_destroy ( &obj ) ;

   // drop index
   rc = sdbDropIndex( cl, INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbReleaseCursor ( cursor ) ;
}

TEST_F( collectionTest,sdbInsert_22028_16587 )
{
   INT32 rc                       = SDB_OK ;
   sdbCursorHandle cursor         = 0 ;

   // delete all before insert
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   // sdbInsert, insert a Chinese record
   bson obj ;
   bson_init( &obj ) ;
   createChineseRecord( &obj ) ;
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;
   // check sdbInsert result
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj ) ; 
   rc = sdbNext( cursor, &obj ) ; 
   bson_iterator it ;
   bson_find( &it, &obj, "姓名" ) ;
   ASSERT_EQ( 0, strncmp( "张三", bson_iterator_string( &it ), strlen( bson_iterator_string( &it ) ) ) ) ;
   bson_find( &it, &obj, "年龄" ) ;
   ASSERT_EQ( 25, bson_iterator_int( &it ) ) ;

   bson_destroy ( &obj ) ; 
   sdbCloseCursor( cursor ) ;

   // sdbInsert1, insert record into the current collection
   bson_init ( &obj ) ;
   bson_append_string( &obj, "_id", "abc" );
   bson_append_int ( &obj, "a", 1 ) ;
   bson_finish ( &obj ) ;
   rc = sdbInsert1 ( cl, &obj, &it ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
   // check sdbInsert1 result
   bson matcher ;
   bson_init( &matcher ) ;
   bson_append_int( &matcher, "a", 1 ) ;  
   bson_finish( &matcher ) ;
   rc = sdbQuery( cl, &matcher, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_find( &it, &obj, "_id" ) ;
   ASSERT_EQ( 0, strncmp( "abc", bson_iterator_string( &it ), strlen( bson_iterator_string( &it ) ) ) ) ;

   bson_destroy ( &obj ) ;
   bson_destroy ( &matcher ) ;
   sdbCloseCursor( cursor ) ;

   //sdbInsert2, insert record into the current collection
   bson ret ;
   bson_init( &ret );
   bson_init ( &obj ) ;
   bson_append_int ( &obj, "a", 1 ) ;
   bson_finish ( &obj ) ;
   rc = sdbInsert2 ( cl, &obj, FLG_INSERT_RETURN_OID, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check sdbInsert2 result
   ASSERT_EQ( BSON_OID, bson_find( &it, &ret, "_id") ) ;
   bson_destroy ( &obj ) ;
   bson_destroy( &ret );

   sdbReleaseCursor ( cursor ) ;
}

TEST_F( collectionTest,sdbBulkInsert_22028_16588 )
{
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 10 ;
   int count                      = 0 ;
   SINT64 totalNum                = 0 ;
   bson ret ;
   bson *objList [ NUM ] ;

   // delete all before insert
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;

   // allocate memory and add data
   for ( count = 0; count < NUM; count++ )
   {
      objList[count] = bson_create() ;
      rc = bson_append_int ( objList[count], "_id", count ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = bson_append_int ( objList[count], "NUM", count ) ;
      bson_finish ( objList[count] ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   // sdbBulkInsert
   rc = sdbBulkInsert ( cl, 0, objList, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check result
   rc = sdbGetCount ( cl, NULL, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( NUM, totalNum ) ;

   // delete all the records in collection
   rc = sdbDelete( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // sdbBulkInsert2
   bson_init( &ret ); 
   rc = sdbBulkInsert2( cl, 0, objList, NUM, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &ret ) ;
   // check
   rc = sdbGetCount ( cl, NULL, &totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( NUM, totalNum ) ;

   // free memory that allocated by bson_create()
   for ( count = 0; count < NUM; count++ )
   {
      bson_dispose ( objList[count] ) ;
   }
}

TEST_F( collectionTest,sdbUpsert_sdbGetCount_22029_22030 )
{
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;
   bson rule ;
   bson condition ;

   // delete all before upsert
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   // build up condition
   bson_init( &rule ) ;
   bson_append_int( &rule, "age", 50 ) ;
   bson_finish( &rule ) ;
   bson_init( &condition ) ;
   bson_append_start_object ( &condition, "$set" ) ;
   bson_append_start_object( &condition, "address" ) ;
   bson_append_string( &condition, "hometown", "guangzhou" ) ;
   bson_append_finish_object( &condition ) ;
   bson_append_finish_object ( &condition ) ;
   bson_finish ( &condition ) ;
   // Upsert
   rc = sdbUpsert( cl, &condition, &rule, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check result, get count
   bson matcher ;
   bson_init( &matcher ) ;
   bson_append_int( &matcher, "age", 50 ) ;
   bson_finish( &matcher ) ;
   rc = sdbGetCount ( cl, &matcher, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 1, count ) ;
          
   bson_destroy( &rule );
   bson_destroy( &condition );
   bson_destroy( &matcher ) ;
}

TEST_F( collectionTest, aggregate_22031 )
{
   INT32 rc                          = SDB_OK ;
   // initialize local variables
   sdbCursorHandle cursor            = 0 ;
   BOOLEAN flag                      = false ;
   SINT64 count                      = 0 ;
   int iNUM                          = 2 ;
   int rNUM                          = 4 ;
   int i                             = 0 ;
   bson *objList [rNUM] ; 
   bson *ob[4] ;
   bson obj ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{$match:{status:\"A\"}}" ;
   command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;
   record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}" ;
   record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}" ;
   record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}" ;
   record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}" ;

   // delete all before insert
   rc = sdbDelete( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      objList[i] = bson_create() ;
      flag = jsonToBson( objList[i], record[i] ) ;
      bson_print( objList[i] ) ;
      ASSERT_TRUE( flag ) ;
      bson_finish( objList[i] ) ; 
   }
   rc = sdbBulkInsert ( cl, 0, objList, rNUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
      
   // build bson array
   for( i=0; i<iNUM; i++ )
   {
      ob[i] = bson_create() ;
      flag = jsonToBson ( ob[i], command[i] ) ;
      bson_print( ob[i] ) ;
      ASSERT_TRUE( flag ) ;
      bson_finish( ob[i] ) ;
   }
   // aggregate
   rc = sdbAggregate( cl, ob, iNUM, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check result
   bson_init( &obj ) ;
   while( !sdbNext( cursor, &obj ) ) 
   {   
      count++;
      bson_destroy( &obj ) ; 
      bson_init( &obj ) ; 
   }   
   bson_destroy( &obj ) ;
   ASSERT_EQ( 2, count ) ;
 
   // free memory which is malloc by bson_create()
   for( i=0; i<iNUM; i++ )
   {
      bson_dispose( ob[i] ) ;
   }
   for ( i=0; i<rNUM; i++ )
   {   
      bson_dispose ( objList[i] ) ; 
   }   

   //release the local variables
   sdbReleaseCursor ( cursor ) ;
}

// the follow test need to dasign more robust
TEST_F( collectionTest,sdbSplitCollection_10407 )
{
   sdbCollectionHandle collection = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 count                   = 0 ;

   if( isStandalone( db ) ) 
   {   
      printf( "Run mode is standalone\n" ) ; 
      return ;
   }   

   // get data groups
   vector<string> groups ;
   rc = getGroups( db, groups ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   if( groups.size() < 2 ) 
   {   
      printf( "data groups num: %d, too few\n", groups.size() ) ; 
      return ;
   }   
   const CHAR* srcGroup = groups[0].c_str() ;
   const CHAR* dstGroup = groups[1].c_str() ;

   // build shardingkey and replsize info:{ShardingKey:{age:1}, ReplSize:1}
   bson obj ;
   bson_init ( &obj ) ;
   bson_append_start_object ( &obj, "ShardingKey" ) ;
   bson_append_int ( &obj, "age", 1 ) ;
   bson_append_finish_object ( &obj ) ;
   bson_append_int ( &obj, "ReplSize", 1 ) ;
   bson_append_string ( &obj, "Group", srcGroup ) ;
   bson_finish ( &obj ) ;
   // create cl
   rc = sdbCreateCollection1 ( cs, COLLECTION_NAME1, &obj, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &obj ) ;

   // build split condition
   bson_init ( &obj ) ;
   bson_append_int ( &obj, "age", 50 ) ;
   bson_finish ( &obj ) ;
   // collection split
   rc = sdbSplitCollection ( collection, srcGroup, dstGroup,
                             &obj, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;

   // check split result
   rc = sdbExplain( collection, NULL, NULL, NULL, NULL, 0, 0, -1, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   bson_init ( &obj ) ;
   while( !sdbNext( cursor, &obj ) ) 
   {  
      bson_iterator it ;
      bson_find( &it, &obj, "GroupName" ) ; 
      string groupName = bson_iterator_string( &it ) ; 
      if( groupName != srcGroup && groupName != dstGroup )
      {  
         bson_print( &obj ) ; 
         break ;
      }  
      
      count++;
      bson_destroy( &obj ) ; 
      bson_init( &obj ) ; 
   }  
   bson_destroy ( &obj ) ; 
   ASSERT_EQ( 2, count ) ;

   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection ( collection ) ;
}

TEST_F( collectionTest, sdbQueryAndRemove_22032 )
{
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   SINT64 NUM                     = 100 ;
   SINT64 count                   = 0 ;
   INT32 i                        = 0 ;
   const CHAR *pIndexName1        = "test_index22032_1" ;
   const CHAR *pIndexName2        = "test_index22032_2" ;
   const CHAR *pField1            = "testQueryAndUpdate22032_1" ;
   const CHAR *pField2            = "testQueryAndUpdate22032_2" ;

   bson index ;
   bson obj ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;

   // delete all before insert
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   // create index
   bson_init( &index ) ;
   bson_append_int( &index, pField1, -1 ) ;
   bson_finish( &index ) ;
   rc = sdbCreateIndex( cl, &index, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &index ) ;

   bson_init( &index ) ;
   bson_append_int( &index, pField2, 1 ) ;
   bson_finish( &index ) ;
   rc = sdbCreateIndex( cl, &index, pIndexName2, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &index ) ;

   // insert datas
   bson* docs[ NUM ] ; 
   for( i = 0; i < NUM; i++ )
   {   
      docs[i] = bson_create() ;
      bson_append_int( docs[i], pField1, i ) ; 
      bson_append_int( docs[i], pField2, i ) ; 
      bson_finish( docs[i] ) ; 
   }   
   rc = sdbBulkInsert( cl, 0, docs, NUM ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   for( i = 0; i < NUM; i++ )
   {   
      bson_dispose( docs[i] ) ; 
   }   

   /// in case: use extend sort
   bson_init( &selector ) ;
   bson_append_string( &selector, pField2, "" ) ;
   bson_finish( &selector ) ;

   bson_init( &orderBy ) ;
   bson_append_int( &orderBy, pField2, 1 ) ;
   bson_finish( &orderBy ) ;

   bson_init( &obj ) ;
   bson_append_int( &obj, "$gte", 0 ) ;
   bson_finish( &obj ) ;
   bson_init( &condition ) ;
   bson_append_bson( &condition, pField1, &obj ) ;
   bson_finish( &condition ) ;

   bson_init( &hint ) ;
   bson_append_string( &hint, "", pIndexName1 ) ;
   bson_finish( &hint ) ;

   rc = sdbQueryAndRemove( cl, &condition, &selector, &orderBy, &hint,
                           0, -1, 0, &cursor ) ;
   ASSERT_EQ( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, rc ) ;
   bson_destroy( &hint ) ;

   /// in case: does not use extend sort
   bson_init( &hint ) ;
   bson_append_string( &hint, "", pIndexName2 ) ;
   bson_finish( &hint ) ;

   // test
   rc = sdbQueryAndRemove( cl, &condition, &selector, &orderBy, &hint,
                           50, 10, 0x00000080, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   // check
   i = 0 ;
   bson_init( &obj ) ;
   while ( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      const CHAR *pKey = bson_iterator_key( &it ) ;
      ASSERT_EQ( 0, strncmp( pKey, pField2, strlen(pField2) ) ) ;
      INT32 value =  bson_iterator_int( &it ) ;
      ASSERT_EQ( 50 + i, value ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
      i++ ;
   }
   bson_destroy( &obj ) ;
   ASSERT_EQ( 10, i ) ;

   // realse
   sdbReleaseCursor ( cursor ) ;
}
