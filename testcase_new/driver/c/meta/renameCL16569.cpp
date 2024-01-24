/**************************************************************
 * @Description:  test rename CL
 *                seqDB-16569
 * @Author     :  liuxiaoxuan Init
 * @Date       :  2018-11-28
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testBase.hpp"
#include "testcommon.hpp"
#include "arguments.hpp"


class reNameCLTest16569 : public testBase
{
protected:
   sdbCSHandle cs ;  
   sdbCollectionHandle cl ;   
   const CHAR* csName ;  
   const CHAR* clOldName ;
   const CHAR* clNewName ;
 
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csName = "csname_16569";
      clOldName = "oldclname_16569" ;
      clNewName = "newclname_16569" ;
      testBase::SetUp() ;
   }
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( cl ) ;
      testBase::TearDown() ;
   }

};

INT32 checkCLExist( sdbConnectionHandle db, const CHAR* clFullName, BOOLEAN* exist )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;
   bson_init( &obj ) ;
   *exist = FALSE ;

   rc = sdbListCollections( db, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list cs" ) ;

   while( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      const CHAR* name = bson_iterator_string( &it ) ;
      if( !strcmp( name, clFullName ) )
      {
         *exist = TRUE ;
         break ;
      }
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }

done:
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

TEST_F( reNameCLTest16569, renamecl )
{
   INT32 rc = SDB_OK ;
   // create cs
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   // create cl
   rc = sdbCreateCollection( cs, clOldName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clOldName ;

   // rename cl 
   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "Global", true ) ;
   bson_finish( &option ) ;
   rc = sdbRenameCollection( cs, clOldName, clNewName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename old cl: " << clOldName << " new cl: " << clNewName ;
   bson_destroy( &option ) ;

   // check rename cl
   CHAR clOldFullName[100] ;
   sprintf( clOldFullName, "%s.%s", csName, clOldName ) ;
   CHAR clNewFullName[100] ;
   sprintf( clNewFullName, "%s.%s", csName, clNewName ) ;
   BOOLEAN exist ;
   rc = checkCLExist( db, clOldFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check rename cl" ;
   rc = checkCLExist( db, clNewFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cl" ;

   // insert with new cl
   sdbReleaseCollection( cl ) ;
   rc = sdbGetCollection( db, clNewFullName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clNewFullName ;
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 10 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // update with new cl
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 10 ) ;
   bson_finish( &cond ) ;
   bson rule ;
   bson_init( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", -1 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish( &rule ) ;
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update" ;
   bson_destroy( &cond ) ;
   bson_destroy( &rule ) ;

   // delete with new cl
   rc = sdbDelete( cl, &doc, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete" ;
   bson_destroy( &doc ) ;
   
   // query with new cl
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   sdbReleaseCursor( cursor ) ;
}
