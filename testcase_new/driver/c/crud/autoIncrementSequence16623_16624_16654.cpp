/************************************************************

 * @Description: test case for task 
 *				     SEQUOIADBMAINSTREAM-3959
 * @author:      liuxiaoxuan
 *				     2019-02-22
 *************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
#include <string>

class autoIncrementSequence16623_16624_16654 : public testBase
{
protected:
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csName = "csname_16623_16624_16654" ;
      clName = "clname_16623_16624_16654" ;
      clFullName = "csname_16623_16624_16654.clname_16623_16624_16654" ; 
      testBase::SetUp() ;
   }
   void TearDown() 
   {
      if( !isStandalone( db ) && shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( cl ) ;
      testBase::TearDown() ; 
   }  
} ;

TEST_F( autoIncrementSequence16623_16624_16654, createDropAutoIncrement16623_16624 )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = 0 ;

   if ( isStandalone( db ) )
   {
      return ;      
   }

   // create cs
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   bson field ;
   bson_init( &field ) ;
   bson_append_string( &field, "Field", "a" ) ;
   bson_finish( &field ) ;

   // invalid cl handle
   rc = sdbCreateAutoIncrement( cl, &field ) ;
   ASSERT_EQ( -6, rc ) << "should create autoIncrement fail with invalid handle" ;
   rc = sdbDropAutoIncrement( cl, &field ) ;
   ASSERT_EQ( -6, rc ) << "should drop autoIncrement fail with invalid handle" ;
   bson_destroy( &field ) ;

   // create cl
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // empty field
   bson emptyField ;
   bson_init( &emptyField ) ;
   bson_finish( &emptyField ) ;
   rc = sdbCreateAutoIncrement( cl, &emptyField ) ;   
   ASSERT_EQ( -6, rc ) << "should create autoIncrement fail with empty field" ;
   rc = sdbDropAutoIncrement(cl, &emptyField) ;
   ASSERT_EQ( -6, rc ) << "should drop autoIncrement fail with empty field" ;
   bson_destroy( &emptyField ) ;

   // invalid field
   bson invaildField ;
   bson_init( &invaildField ) ;
   bson_append_int( &invaildField, "Field", 1 ) ;
   bson_finish( &invaildField ) ;
   rc = sdbCreateAutoIncrement( cl, &invaildField ) ;
   ASSERT_EQ( -6, rc ) << "should create autoIncrement fail with invalid field" ;
   rc = sdbDropAutoIncrement(cl, &invaildField) ;
   ASSERT_EQ( -6, rc ) << "should drop autoIncrement fail with invalid field" ;
   bson_destroy( &invaildField ) ; 

   // create autoincrement
   bson_init( &field ) ;
   bson_append_string( &field, "Field", "a" ) ;
   bson_append_int( &field, "Increment", 2 ) ;
   bson_append_int( &field, "StartValue", 2 ) ;
   bson_append_int( &field, "MinValue", 2 ) ;
   bson_append_int( &field, "CacheSize", 2000 ) ;
   bson_append_int( &field, "AcquireSize", 1500 ) ;
   bson_append_bool( &field, "Cycled", true ) ;
   bson_finish( &field ) ; 
   rc = sdbCreateAutoIncrement( cl, &field ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create autoincrement " ;
   bson_destroy( &field ) ;

   // insert
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_append_string( &doc, "b", "testb" ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert ( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &doc ) ;

   // get sequence name
   bson matcher ;
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", clFullName ) ;
   bson_finish( &matcher ) ;
   bson autoIncObj ;
   bson_init( &autoIncObj ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_CATALOG, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot " << rc ;
   rc = sdbNext( cursor, &autoIncObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;

   bson_iterator it, sub ;
   bson_find( &it, &autoIncObj, "AutoIncrement" ) ;
   bson_iterator_subiterator( &it, &sub ) ;
   
   const char* sequenceName ;
   while( bson_iterator_more( &sub ) )
   {
      bson subObj ;
      bson_init( &subObj ) ;
      bson_iterator_subobject( &sub, &subObj ) ;
      bson_iterator i1 ;
      bson_find( &i1, &subObj, "SequenceName" ) ;
      sequenceName = bson_iterator_string( &i1 ) ;

      bson_destroy( &subObj ) ;
      bson_iterator_next( &sub ) ;
  }
  printf( "sequenceName: %s\n", sequenceName ) ;
  bson_destroy( &autoIncObj ) ;
  bson_destroy( &matcher ) ;
  sdbCloseCursor( cursor ) ;
  sdbReleaseCursor( cursor ) ;

  // check sequence snapshot
  bson_init( &matcher ) ;
  bson_append_string( &matcher, "Name", sequenceName ) ;
  bson_finish( &matcher ) ;
  rc = sdbGetSnapshot( db, SDB_SNAP_SEQUENCES, &matcher, NULL, NULL, &cursor ) ;
  bson_init( &autoIncObj ) ;
  rc = sdbNext( cursor, &autoIncObj ) ;
  ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;

  INT32 acquireSize, cacheSize, increment, minValue, startValue ;
  BOOLEAN cycled ;

  bson_find( &it, &autoIncObj, "AcquireSize" ) ;
  acquireSize = bson_iterator_int( &it ) ;
  ASSERT_EQ( 1500, acquireSize ) << "actual acquireSize " << acquireSize ;

  bson_find( &it, &autoIncObj, "CacheSize" ) ;
  cacheSize = bson_iterator_int( &it ) ;
  ASSERT_EQ( 2000, cacheSize ) << "actual cacheSize " << cacheSize ;

  bson_find( &it, &autoIncObj, "Cycled" ) ;
  cycled = bson_iterator_bool( &it ) ;
  ASSERT_TRUE( cycled ) ;

  bson_find( &it, &autoIncObj, "Increment" ) ;
  increment = bson_iterator_int( &it ) ;
  ASSERT_EQ( 2, increment ) << "actual increment " << increment ;

  bson_find( &it, &autoIncObj, "MinValue" ) ;
  minValue = bson_iterator_int( &it ) ;
  ASSERT_EQ( 2, minValue ) << "actual minValue " << minValue ;

  bson_find( &it, &autoIncObj, "StartValue" ) ;
  startValue = bson_iterator_int( &it ) ;
  ASSERT_EQ( 2, startValue ) << "actual startValue " << startValue ;
  bson_destroy( &autoIncObj ) ;
  bson_destroy( &matcher ) ;
  sdbCloseCursor( cursor ) ;
  sdbReleaseCursor( cursor ) ;

  // drop not exist autoincrement 
  bson nonExistField ;
  bson_init( &nonExistField ) ;
  bson_append_string( &nonExistField, "Field", "non_exist" ) ;
  bson_finish( &nonExistField ) ;
  rc = sdbDropAutoIncrement( cl, &nonExistField ) ;
  ASSERT_EQ( -333, rc ) << "fail to drop not exisit autoincrement" ;
  bson_destroy( &nonExistField ) ;

  // drop autoincrement
  bson_init( &field ) ;
  bson_append_string( &field, "Field", "a" ) ;
  bson_finish( &field ) ;
  rc = sdbDropAutoIncrement( cl, &field );
  ASSERT_EQ( SDB_OK, rc ) << "fail to drop autoincrement " << sequenceName ;
  bson_destroy( &field ) ;

  // check sequence list
  bson_init( &autoIncObj );
  bson_init( &matcher ) ;
  bson_append_string( &matcher, "Name", sequenceName ) ;
  bson_finish( &matcher ) ;
  rc = sdbGetList( db, SDB_LIST_SEQUENCES, &matcher, NULL, NULL, &cursor ) ;
  ASSERT_EQ( SDB_OK, rc ) << "fail to list" << rc ;
  rc = sdbNext( cursor, &autoIncObj ) ;
  bson_print( &autoIncObj ) ;
  ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to next" << rc ;
  //ASSERT_TRUE( bson_is_empty( &autoIncObj )) ;
  bson_destroy( &matcher ) ;
  bson_destroy( &autoIncObj ) ;
  sdbReleaseCursor( cursor ) ;
}


TEST_F( autoIncrementSequence16623_16624_16654, createAlterCollection16654 )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = 0 ;

   if ( isStandalone( db ) )
   {
      return ;
   }

   // create cs 
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   // create cl with autoincrement
   bson field ;
   bson_init( &field ) ;
   bson_append_start_object( &field, "AutoIncrement" ) ;
   bson_append_string( &field, "Field", "a" ) ;
   bson_append_int( &field, "Increment", 5 ) ;
   bson_append_int( &field, "CacheSize", 3000 ) ;
   bson_append_int( &field, "AcquireSize", 100 ) ;
   bson_append_finish_object( &field ) ;
   bson_finish( &field ) ;
   rc = sdbCreateCollection1( cs, clName, &field, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   bson_destroy( &field ) ;

   // insert
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert ( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &doc ) ;

   // get sequence name
   bson matcher;
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", clFullName ) ;
   bson_finish( &matcher ) ;
   bson autoIncObj ;
   bson_init( &autoIncObj ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_CATALOG, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &autoIncObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_iterator it, sub ;
   bson_find( &it, &autoIncObj, "AutoIncrement" ) ;
   bson_iterator_subiterator( &it, &sub ) ;
 
   const char* sequenceName ;
   while( bson_iterator_more( &sub ) )
   {
      bson subObj ;
      bson_init( &subObj ) ;
      bson_iterator_subobject( &sub, &subObj ) ;
      bson_iterator i1 ;
      bson_find( &i1, &subObj, "SequenceName" ) ;
      sequenceName = bson_iterator_string( &i1 ) ;

      bson_destroy( &subObj ) ;
      bson_iterator_next( &sub ) ;
   }
   printf( "sequenceName: %s\n", sequenceName ) ;
   bson_destroy( &matcher ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;

   // check sequence snapshot before alter
   bson_init( &matcher ) ;
   bson_append_string( &matcher, "Name", sequenceName ) ;
   bson_finish( &matcher ) ;
   rc = sdbGetSnapshot( db, SDB_SNAP_SEQUENCES, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &autoIncObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   INT32 acquireSize, cacheSize, increment;
   bson_find( &it, &autoIncObj, "AcquireSize" ) ;
   acquireSize = bson_iterator_int( &it ) ;
   ASSERT_EQ( 100, acquireSize ) << "actual acquireSize " << acquireSize ;

   bson_find( &it, &autoIncObj, "CacheSize" ) ;
   cacheSize = bson_iterator_int( &it ) ;
   ASSERT_EQ( 3000, cacheSize ) << "actual cacheSize " << cacheSize ;

   bson_find( &it, &autoIncObj, "Increment" ) ;
   increment = bson_iterator_int( &it ) ;
   ASSERT_EQ( 5, increment ) << "actual increment " << increment ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;

   // alter autoincrement
   bson_init( &field );
   bson_append_start_object( &field, "AutoIncrement" ) ;
   bson_append_string( &field, "Field", "a" ) ;
   bson_append_int( &field, "Increment", 10 ) ;
   bson_append_int( &field, "CacheSize", 2500 ) ;
   bson_append_int( &field, "AcquireSize", 500 ) ;
   bson_append_finish_object( &field ) ;
   bson_finish( &field ) ;
   rc = sdbAlterCollection( cl, &field ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &field ) ;
   
   // check sequence snapshot after alter
   rc = sdbGetSnapshot( db, SDB_SNAP_SEQUENCES, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &autoIncObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_find( &it, &autoIncObj, "AcquireSize" ) ;
   acquireSize = bson_iterator_int( &it ) ;
   ASSERT_EQ( 500, acquireSize ) << "actual acquireSize " << acquireSize ;

   bson_find( &it, &autoIncObj, "CacheSize" ) ;
   cacheSize = bson_iterator_int( &it ) ;
   ASSERT_EQ( 2500, cacheSize ) << "actual cacheSize " << cacheSize ;

   bson_find( &it, &autoIncObj, "Increment" ) ;
   increment = bson_iterator_int( &it ) ;
   ASSERT_EQ( 10, increment ) << "actual increment " << increment ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;

   // alter autoincrement
   bson_init( &field ) ;
   bson_append_start_object( &field, "AutoIncrement" ) ;
   bson_append_string( &field, "Field", "a" ) ;
   bson_append_int( &field, "Increment", 20 ) ;
   bson_append_int( &field, "CacheSize", 5000 ) ;
   bson_append_int( &field, "AcquireSize", 800 ) ;
   bson_append_finish_object( &field ) ;
   bson_finish( &field ) ;
   rc = sdbCLSetAttributes( cl, &field ) ;
   bson_destroy( &field ) ;
  
   // check sequence snapshot after alter
   rc = sdbGetSnapshot( db, SDB_SNAP_SEQUENCES, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbNext( cursor, &autoIncObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_find( &it, &autoIncObj, "AcquireSize" ) ;
   acquireSize = bson_iterator_int( &it ) ;
   ASSERT_EQ( 800, acquireSize ) << "actual acquireSize " << acquireSize ;

   bson_find( &it, &autoIncObj, "CacheSize" ) ;
   cacheSize = bson_iterator_int( &it ) ;
   ASSERT_EQ( 5000, cacheSize ) << "actual cacheSize " << cacheSize ;

   bson_find( &it, &autoIncObj, "Increment" ) ;
   increment = bson_iterator_int( &it ) ;
   ASSERT_EQ( 20, increment ) << "actual increment " << increment ;
   bson_destroy( &autoIncObj ) ;
   bson_destroy( &matcher ) ;

   // drop autoincrement
   bson_init( &field ) ;
   bson_append_string( &field, "Field", "a" ) ;
   bson_finish( &field ) ;
   rc = sdbDropAutoIncrement( cl, &field ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop autoincrement " << sequenceName ;
   bson_destroy( &field ) ;
   sdbReleaseCursor( cursor ) ;
}
